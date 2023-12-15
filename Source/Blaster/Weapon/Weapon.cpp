// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Casing.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"

#define TRACE_LENGTH 80000.f

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true; // 设置为 replicated，如果不设置，client 端就不能复制变量，WeaponState 属性在 client 端不会同步
	SetReplicateMovement(true); // 设置为 replicated，确保 client 端能够同步武器的位置

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	RootComponent = WeaponMesh;

	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block); // 设置碰撞响应
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 设置忽略 Pawn 的碰撞
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 设置初始化时不碰撞，等待被拾取
	WeaponMesh->SetRenderCustomDepth(true); // 启用 CustomDepth，用于绘制 Outline
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE); // 设置 CustomDepth 的值，用于绘制 Outline
	WeaponMesh->MarkRenderStateDirty(); // 标记 RenderState 为 dirty，确保 CustomDepth 的值能够被正确设置

	WeaponCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Weapon Collision"));
	WeaponCollision->SetupAttachment(WeaponMesh);

	WeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 在 server 端设置碰撞

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
	PickupWidget->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget) PickupWidget->SetVisibility(false);

	/// 已经设置了只有 server 才能装备武器，因此该操作是安全的，可以在 client 本地立即显示 pickup widget
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 设置为 overlap, 角色可以拾取
	// 在委托中绑定回调函数
	WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	WeaponCollision->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 注册需要同步的属性
void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

/**
 * @brief 角色和武器触发重叠时，显示拾取提示
 * @param OverlappedComponent 触发重叠的组件
 * @param OtherActor 触发重叠的角色
 * @param OtherComp 角色的碰撞组件
 * @param OtherBodyIndex 角色的碰撞体索引
 * @param bFromSweep 是否是由碰撞体移动触发的重叠
 * @param SweepResult 碰撞结果 
 */
void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);
	if (Character) Character->SetOverlappingWeapon(this);
}

/**
 * @brief 角色和武器结束重叠时，隐藏拾取提示
 * @param OverlappedComponent 触发重叠的组件
 * @param OtherActor 触发重叠的角色
 * @param OtherComp 角色的碰撞组件
 * @param OtherBodyIndex 角色的碰撞体索引
 */
void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);
	if (Character) Character->SetOverlappingWeapon(nullptr);
}

void AWeapon::ShowWeaponPickupWidget(bool bShowWidget)
{
	if (PickupWidget) PickupWidget->SetVisibility(bShowWidget);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation) WeaponMesh->PlayAnimation(FireAnimation, false);
	// 生成弹壳
	if (CasingClass)
	{
		// 获取生成位置
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform AmmoEjectTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			UWorld* World = GetWorld();
			if (World) World->SpawnActor<ACasing>(CasingClass, AmmoEjectTransform.GetLocation(), AmmoEjectTransform.GetRotation().Rotator());
		}
	}
	// 减少弹药并更新 AmmoHUD
	SpendRound();
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();
	// 获取 MuzzleFlash 的位置
	FTransform MuzzleFlashTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	FVector TraceStart = MuzzleFlashTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere; // 球心
	const FVector RandomVectorInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius); // 随机散射方向
	const FVector TraceEnd = SphereCenter + RandomVectorInSphere; // TraceEnd 位置
	const FVector ToTraceEnd = TraceEnd - TraceStart;

	return FVector(TraceStart + ToTraceEnd * TRACE_LENGTH / ToTraceEnd.Size());
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::OnEquipped()
{
	ShowWeaponPickupWidget(false);  // 拾取武器后隐藏拾取提示
	// 装备武器后禁用武器的碰撞，避免武器碰撞到角色后在 server 端生成重叠事件再次显示拾取提示
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponTypes::EWT_SubmachineGun) // 如果是冲锋枪，启用 strap 模拟物理的特性
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	WeaponMesh->SetRenderCustomDepth(false); // 装备武器后禁用 Outline

	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter && bUseServerSideRewind)
	{
		BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
		if (BlasterOwnerPlayerController && HasAuthority() && !BlasterOwnerPlayerController->PingTooHighDelegate.IsBound())
		{
			BlasterOwnerPlayerController->PingTooHighDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowWeaponPickupWidget(false);  // 拾取武器后隐藏拾取提示
	// 装备武器后禁用武器的碰撞，避免武器碰撞到角色后在 server 端生成重叠事件再次显示拾取提示
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponTypes::EWT_SubmachineGun) // 如果是冲锋枪，启用 strap 模拟物理的特性
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	WeaponMesh->SetRenderCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	WeaponMesh->MarkRenderStateDirty();
	
	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
		if (BlasterOwnerPlayerController && HasAuthority() && BlasterOwnerPlayerController->PingTooHighDelegate.IsBound())
		{
			BlasterOwnerPlayerController->PingTooHighDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnDropped()
{
	if (HasAuthority()) WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 开启武器的碰撞
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block); // 设置碰撞响应
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 设置忽略 Pawn 的碰撞
	WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 设置忽略 Camera 的碰撞
	/// 绘制 Outline
	WeaponMesh->SetRenderCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();

	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
		if (BlasterOwnerPlayerController && HasAuthority() && BlasterOwnerPlayerController->PingTooHighDelegate.IsBound())
		{
			BlasterOwnerPlayerController->PingTooHighDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped: // 被拾取
			OnEquipped();
			break;
		case EWeaponState::EWS_EquippedSecondary: // 被拾取为副武器
			OnEquippedSecondary();
			break;
		case EWeaponState::EWS_Dropped: // 被丢弃
			OnDropped();
			break;
		default: break;
	}
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;

	OnWeaponStateSet();
}

// 当 WeaponState 属性改变时调用
void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::SetAmmoHUD()
{
	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerPlayerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
		if (BlasterOwnerPlayerController) BlasterOwnerPlayerController->SetWeaponAmmoHUD(Ammo);
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MaxAmmoCapacity);

	SetAmmoHUD();

	/// client 和 server 都会执行该函数
	/// 当是 server 端时，将更新后的 Ammo 通过 RPC 发送给对应的 client
	/// 当是 client 时，则增加一次 server 端还未处理 Ammo 的更新请求，表示 server 端的更新还没到达此 client
	if (HasAuthority()) ClientUpdateAmmo(Ammo);
	
	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (!HasAuthority() && BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled()) ++Sequence;
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	
	Ammo = ServerAmmo; // client 端更新
	/// 执行更正
	--Sequence; // 调用该函数一次，代表 server 处理了一次 Ammo 的更新
	Ammo -= Sequence; // 由于 Ammo 一次只消耗一颗，所以 Sequence 可以代表 client 已经消耗了多少颗弹药，但是 server 端还没有复制给 client

	SetAmmoHUD();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MaxAmmoCapacity);
	// server 端更新 AmmoHUD
	SetAmmoHUD();

	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;

	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MaxAmmoCapacity);
	
	BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterOwnerCharacter &&
		BlasterOwnerCharacter->GetCombatComponent() &&
		IsFullAmmo()) BlasterOwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();

	SetAmmoHUD();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	// 如果 owner 为 nullptr，说明武器被丢弃，此时不需要更新 AmmoHUD
	// Bug: 已装备武器再拾取时先丢弃再装备新武器，装备之后 WeaponHUD 仍然不显示，需要开枪之后才变得正常，将此逻辑放入 CombatComponent 的 OnRep_EquippedWeapon() 函数中就可正常显示 WeaponHUD
	// if (Owner == nullptr) if (BlasterOwnerPlayerController) BlasterOwnerPlayerController->SetWeaponHUDVisibility(ESlateVisibility::Hidden);
	// else
	BlasterOwnerCharacter = Cast<ABlasterCharacter>(Owner);
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this) SetAmmoHUD();
}

ABlasterPlayerController* AWeapon::GetBlasterOwnerPlayerController()
{
	if (BlasterOwnerCharacter) return Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);

	return nullptr;
}
