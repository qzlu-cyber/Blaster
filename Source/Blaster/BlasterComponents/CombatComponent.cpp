// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		// 将准星的坐标转为世界坐标
		FVector CrosshairWorldPosition, CrosshairWorldDirection;
		bool bScreenToWorld =  UGameplayStatics::DeprojectScreenToWorld(
			UGameplayStatics::GetPlayerController(GetWorld(), 0),
			CrosshairLocation,
			CrosshairWorldPosition,
			CrosshairWorldDirection
		);
		if (bScreenToWorld)
		{
			const FVector Start = CrosshairWorldPosition;
			const FVector End = Start + CrosshairWorldDirection * 10000.f;

			GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);

			if (HitResult.GetActor()
				&& HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>()) HUDPackage.CrosshairColor = FLinearColor::Red;
			else HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (!Character || !Character->Controller) return;

	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(PlayerController->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
			}
			/// 计算绘制放大准星的偏移量
			/// 根据角色的速度映射，[0, 600] -> [0, 1]
			const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetMovementComponent()->IsFalling()) CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			else CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);

			if (bIsAiming) CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			else CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon) return;

	if (bIsAiming)
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	else
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed); // 恢复 FOV 使用统一插值速度

	if (Character && Character->GetFollowCamera())
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
		Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	// 装配武器
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

	EquippedWeapon->SetOwner(Character); // 设置武器的 owner
	/// EquipWeapon 只会在 server 端执行，以下代码只会在 server 端执行
	/// 为了使 client 和 server 同步，client 端的设置交由 OnRep_EquippedWeapon() 函数处理
	Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
	Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
}

void UCombatComponent::DropWeapon()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	// 从右手插槽分离武器
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		// 设置武器的位置和旋转
		FVector WeaponLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f; // 适当的位置
		FRotator WeaponRotation = Character->GetActorRotation(); // 适当的旋转

		WeaponLocation.Z = 2.f;
		WeaponRotation.Pitch = -90.f;

		EquippedWeapon->SetActorLocation(WeaponLocation);
		EquippedWeapon->SetActorRotation(WeaponRotation);

		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	// 卸下武器
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Dropped);

	EquippedWeapon->SetOwner(nullptr);
	EquippedWeapon = nullptr;
}

void UCombatComponent::OnRep_IsAiming(bool bLastIsAiming)
{
	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bLastIsAiming ? BaseWalkSpeed : AimWalkSpeed;
}

void UCombatComponent::Aiming(bool bAiming)
{
	bIsAiming = bAiming;

	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::Fire(bool bFire)
{
	if (!EquippedWeapon) return;
	
	// 从 client 端调用 server 类型的 RPC，将在 server 端执行
	if (bFire)
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		/// 当按下开火键时，要么是在客户端控制角色，要么是在服务端控制角色
		/// 当在服务端时，调用 Server 类型 RPC ServerFire，服务器执行 MulticastFire，然后服务端和所有客户端播放 Montage 动画并调用武器开火函数
		/// 当在客户端调用 Server 类型 RPC ServerFire 时，服务器执行 MulticastFire，然后服务端和所有客户端播放 Montage 动画并调用武器开火函数
		/// 再由服务端 Replicates 数据并同步给客户端
		ServerFire(HitResult.ImpactPoint);
		
		CrosshairShootingFactor = 0.75f;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 从 server 端调用 NetMulticast 类型的 RPC，将在 server 和所有 client 上执行
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	Character->PlayFireWeaponMontage(bIsAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}
