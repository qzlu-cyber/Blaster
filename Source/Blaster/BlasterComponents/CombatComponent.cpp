// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Weapon/Projectile.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Components/SlateWrapperTypes.h"
#include "Sound/SoundCue.h"

#define TRACE_LENGTH 80000.f

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
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedWeaponAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::InitializeCarriedAmmoMap()
{
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_AssaultRifle, StartingARAmmo); // 初始化 AR 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_RocketLauncher, StartingRLAmmo); // 初始化 RL 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_Pistol, StartingPistolAmmo); // 初始化 Pistol 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_SubmachineGun, StartingSMGAmmo); // 初始化 SMG 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_Shotgun, StartingShotgunAmmo); // 初始化 Shotgun 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_SniperRifle, StartingSniperAmmo); // 初始化 Sniper 弹药
	CarriedAmmoMap.Emplace(EWeaponTypes::EWT_GrenadeLauncher, StartingGrenadeAmmo); // 初始化 Grenade 弹药
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

		if (Character->HasAuthority()) InitializeCarriedAmmoMap();
	}
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled() && !Character->GetDisableGameplay())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		
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
			FVector Start = CrosshairWorldPosition;
			// 修改射线起点，避免射线与角色碰撞
			if (Character)
			{
				const float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
				Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
			}
			const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

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

void UCombatComponent::OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon)
{
	if (EquippedWeapon && Character)
	{
		// SetWeaponState 处理了碰撞属性，当角色试图拿起被丢弃的武器时，此时武器是有物理属性的，这就无法把武器 attach 到角色的手上
		// 所以就需要在拾起武器前将碰撞和物理属性都给禁用
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);

		// 播放装备武器的音效
		PlayEquipWeaponSound();
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
		Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
	}

	if (LastEquippedWeapon)
	{
		if (LastEquippedWeapon->GetBlasterOwnerPlayerController()) LastEquippedWeapon->GetBlasterOwnerPlayerController()->SetWeaponHUDVisibility(ESlateVisibility::Hidden);
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || EquippedWeapon == nullptr || ActorToAttach == nullptr) return;

	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_SubmachineGun;

	const FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket) HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
}

void UCombatComponent::PlayEquipWeaponSound()
{
	if (EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) CarriedWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) PlayerController->SetCarriedAmmoHUD(CarriedWeaponAmmo);
}

void UCombatComponent::UpdateGrenades()
{
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) PlayerController->SetGrenadeHUD(Grenades);
}

void UCombatComponent::ShowAttachedGrenade(bool bShow)
{
	if (Character && Character->GetGrenadeMeshComponent()) Character->GetGrenadeMeshComponent()->SetVisibility(bShow);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	// Combat 不为 Unoccupied 状态无法装备武器
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// 角色已装备武器时再次拾取需要先丢弃已有武器
	if (EquippedWeapon) DropWeapon();

	// 装配武器
	// 两件事会被复制，一是武器上的武器状态本身，二是附加到角色的行为
	// 但并不能保证其中哪一个会先传到客户端，不能假定先复制武器状态，然后再复制附着动作。因为情况并非总是如此
	// 为了避免这一假设，可以在 client 端（OnRep_EquippedWeapon）也做这两件事
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);

	// 播放装备武器的音效
	PlayEquipWeaponSound();
	
	EquippedWeapon->SetOwner(Character); // 设置武器的 owner
	EquippedWeapon->SetAmmoHUD(); // 设置武器的弹药 HUD
	UpdateCarriedAmmo(); // 更新武器总携带的弹药并设置 HUD
	
	// 检查是否需要换弹
	if (EquippedWeapon->IsEmptyAmmo() && CarriedWeaponAmmo > 0) Reload();
	
	/// EquipWeapon 只会在 server 端执行，以下代码只会在 server 端执行
	/// 为了使 client 和 server 同步，client 端的设置交由 OnRep_EquippedWeapon() 函数处理
	Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 设置角色不跟随移动方向旋转
	Character->bUseControllerRotationYaw = true; // 设置角色跟随控制器的旋转
}

void UCombatComponent::DropWeapon()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	// 卸下武器
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
	EquippedWeapon->GetWeaponMesh()->DetachFromComponent(DetachmentTransformRules);

	EquippedWeapon->SetOwner(nullptr);
	if (EquippedWeapon->GetBlasterOwnerPlayerController()) EquippedWeapon->GetBlasterOwnerPlayerController()->SetWeaponHUDVisibility(ESlateVisibility::Hidden);
	EquippedWeapon = nullptr;
}

void UCombatComponent::OnRep_IsAiming(bool bLastIsAiming)
{
	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bLastIsAiming ? BaseWalkSpeed : AimWalkSpeed;

	// 角色是当前客户端控制的角色且装备的是狙击枪时，显示狙击枪的准星
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_SniperRifle) Character->ShowSniperScopeWidget(bIsAiming);
}

void UCombatComponent::OnRep_CarriedWeaponAmmo()
{
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
	if (PlayerController) PlayerController->SetCarriedAmmoHUD(CarriedWeaponAmmo);

	// 当装备的武器是霰弹枪，且正在换弹且没有多余子弹了，直接跳转
	if (EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_Shotgun &&
	CombatState == ECombatState::ECS_Reloading &&
	CarriedWeaponAmmo == 0) JumpToShotgunEnd();
}

void UCombatComponent::Aiming(bool bAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	bIsAiming = bAiming;

	if (Character) Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;

	// 角色是当前客户端控制的角色且装备的是狙击枪时，显示狙击枪的准星
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_SniperRifle) Character->ShowSniperScopeWidget(bIsAiming);
}

bool UCombatComponent::CanFire() const
{
	if (!EquippedWeapon) return false;

	// 针对霰弹枪做例外，在其填入子弹后就可射击
	if (!EquippedWeapon->IsEmptyAmmo() &&
		bCanFire &&
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_Shotgun) return true;

	return !EquippedWeapon->IsEmptyAmmo() && bCanFire && (CombatState == ECombatState::ECS_Unoccupied);
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon && !Character) return;

	// 启动计时器，在 FireDelay 时间后执行 FireTimerFinished()
	Character->GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay()
	);
}

void UCombatComponent::FireTimerFinished()
{
	/// 无论是否按下鼠标左键开火或是否使用连发武器，都要先将 bCanFire 设置为 true
	/// 这样，如果使用的是半自动或非自动武器，仍会在 FireDelay 后将 bCanFire 设置为 true
	/// 对于非自动武器，必须等待一个强制延迟后才能再次开火
	bCanFire = true;
	// 如果按下开火键且武器是连发武器
	if (bIsFire && EquippedWeapon->GetAutomaticFire()) Shoot();

	// 检查是否需要换弹
	if (EquippedWeapon->IsEmptyAmmo() && CarriedWeaponAmmo > 0) Reload();
}

void UCombatComponent::Shoot()
{
	if (CanFire())
	{
		bCanFire = false;
		/// 当按下开火键时，要么是在客户端控制角色，要么是在服务端控制角色
		/// 当在服务端时，调用 Server 类型 RPC ServerFire，服务器执行 MulticastFire，然后服务端和所有客户端播放 Montage 动画并调用武器开火函数
		/// 当在客户端调用 Server 类型 RPC ServerFire 时，服务器执行 MulticastFire，然后服务端和所有客户端播放 Montage 动画并调用武器开火函数
		/// 再由服务端 Replicates 数据并同步给客户端
		ServerFire(HitTarget);

		CrosshairShootingFactor = 0.75f;

		StartFireTimer(); // 启动计时器
	}
}

void UCombatComponent::Fire(bool bFire)
{
	if (!EquippedWeapon) return;

	bIsFire = bFire;
	
	// 从 client 端调用 server 类型的 RPC，将在 server 端执行
	if (bIsFire) Shoot();
}

void UCombatComponent::ThrowGrenade()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return; // 防止不断按投掷手雷键时，动画一直从头播放
	
	if (Grenades == 0) return; // 手雷数量为 0 时不可投掷
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage(); // 当前调用的 client 播放投掷手雷动画

		AttachActorToLeftHand(EquippedWeapon); // 投掷手雷时先将装备的武器固定到左手上

		ShowAttachedGrenade(true); // 显示手雷
	}

	if (Character && !Character->HasAuthority()) ServerThrowGrenade(); // 如果是 server 端调用 ThrowGrenade 则不再重复播放

	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		
		UpdateGrenades();
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::FinishReloading()
{
	if (!Character) return;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		// 确保换弹动画结束后才更新 HUD
		UpdateAmmos();
	}

	// 换弹之后，如果角色按下开火键，继续射击
	if (bIsFire) Shoot();
}

int32 UCombatComponent::AmountToReload()
{
	if (!Character || !EquippedWeapon) return 0;
	
	// 最多可装填弹药数量
	int32 RoomInMag = EquippedWeapon->GetMaxAmmoCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 当前携带的弹药数量少于 RoomInMag 时可全部装填，否则只能装填 RoomInMag 数量的弹药。故取二者最小值
		int32 Least = FMath::Min(RoomInMag, CarriedWeaponAmmo);
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

void UCombatComponent::UpdateAmmos()
{
	if (!Character || !EquippedWeapon) return;
	
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		EquippedWeapon->AddAmmo(-ReloadAmount);
		// server 端更新 HUD
		PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController) PlayerController->SetCarriedAmmoHUD(CarriedWeaponAmmo);
	}
}

void UCombatComponent::UpdateShotgunAmmo()
{
	if (!Character || !EquippedWeapon) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1; // 每次装填 1 发子弹
		CarriedWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		EquippedWeapon->AddAmmo(-1);
		// server 端更新 HUD
		PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController) PlayerController->SetCarriedAmmoHUD(CarriedWeaponAmmo);

		// 确保霰弹枪有子弹即可射击
		bCanFire = true;

		if (EquippedWeapon->IsFullAmmo() || CarriedWeaponAmmo == 0)
		{
			// 已装满或没有霰弹枪子弹后，在 ReloadMontage 中直接跳转到 ShotgunEnd 片段
			JumpToShotgunEnd();
		}
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		const FName Section = FName("ShotgunEnd");
		AnimInstance->Montage_JumpToSection(Section);
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;

	AttachActorToRightHand(EquippedWeapon); // 投掷结束，将武器固定到右手
}

void UCombatComponent::LaunchGrenade()
{
	// 此时手雷掷出，隐藏手心里的手雷
	ShowAttachedGrenade(false);

	// client 端将本地计算的 HitTarget 发送给 server 端
	if (Character && Character->IsLocallyControlled()) ServerLaunchGrenade(HitTarget);
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority()) UpdateShotgunAmmo();
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon) return;

	// 还有弹药可换且弹夹不满且处于空闲状态才可换弹
	if (CarriedWeaponAmmo > 0 &&
		EquippedWeapon->GetAmmo() < EquippedWeapon->GetMaxAmmoCapacity() &&
		CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerReload();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
		case ECombatState::ECS_Unoccupied:
			if (bIsFire) Shoot();
			break;
		case ECombatState::ECS_ThrowingGrenade:
			if (Character)
			{
				Character->PlayThrowGrenadeMontage();

				AttachActorToLeftHand(EquippedWeapon);

				ShowAttachedGrenade(true);
			}
			break;
		default: break;
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateGrenades();
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && Character->GetGrenadeMeshComponent() && GrenadeClass)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			const FVector StartingLocation = Character->GetGrenadeMeshComponent()->GetComponentLocation();
			FVector ToHitTarget = Target - StartingLocation;
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner();
			SpawnParameters.Instigator = Character;
			
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToHitTarget.Rotation(),
				SpawnParameters
			);
		}
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0) return;
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage();

		AttachActorToLeftHand(EquippedWeapon);
		
		ShowAttachedGrenade(true);

		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		
		UpdateGrenades();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!Character || !EquippedWeapon) return;
	
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 从 server 端调用 NetMulticast 类型的 RPC，将在 server 和所有 client 上执行
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!EquippedWeapon) return;

	// 如果处于 ECS_Unoccupied 状态，才能开火，否则必须等到 CombatState 变为 ECS_Unoccupied 且复制到 client 端后才能开火
	if (Character && (CombatState == ECombatState::ECS_Unoccupied))
	{
		Character->PlayFireWeaponMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}

	// 针对霰弹枪例外
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponTypes::EWT_Shotgun)
	{
		Character->PlayFireWeaponMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		// 射击后更新 CombatState 以保证其之后还可换弹
		CombatState = ECombatState::ECS_Unoccupied;
	}
}
