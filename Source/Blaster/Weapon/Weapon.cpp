// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Casing.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true; // 设置为 replicated，如果不设置，client 端就不能复制变量，WeaponState 属性在 client 端不会同步

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	RootComponent = WeaponMesh;

	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block); // 设置碰撞响应
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 设置忽略 Pawn 的碰撞
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 设置初始化时不碰撞，等待被拾取

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

	if (HasAuthority())
	{
		WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 设置为 overlap, 角色可以拾取

		// 在委托中绑定回调函数
		WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		WeaponCollision->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
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
	DOREPLIFETIME(AWeapon, Ammo);
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

// 当 WeaponState 属性改变时调用
void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped:
			ShowWeaponPickupWidget(false);
			WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EWeaponState::EWS_Dropped: // 被丢弃
			WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 开启武器的碰撞
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		default: break;
	}
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped: // 被拾取
			ShowWeaponPickupWidget(false);  // 拾取武器后隐藏拾取提示
			// 装备武器后禁用武器的碰撞，避免武器碰撞到角色后在 server 端生成重叠事件再次显示拾取提示
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EWeaponState::EWS_Dropped: // 被丢弃
			WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 开启武器的碰撞
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		default: break;
	}
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
}

void AWeapon::OnRep_Ammo()
{
	SetAmmoHUD();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	// 如果 owner 为 nullptr，说明武器被丢弃，此时不需要更新 AmmoHUD
	// Bug: 已装备武器再拾取时先丢弃再装备新武器，装备之后 WeaponHUD 仍然不显示，需要开枪之后才变得正常，将此逻辑放入 CombatComponent 的 OnRep_EquippedWeapon() 函数中就可正常显示 WeaponHUD
	// if (Owner == nullptr) if (BlasterOwnerPlayerController) BlasterOwnerPlayerController->SetWeaponHUDVisibility(ESlateVisibility::Hidden);
	// else
	SetAmmoHUD();
}

ABlasterPlayerController* AWeapon::GetBlasterOwnerPlayerController()
{
	if (BlasterOwnerCharacter) return Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);

	return nullptr;
}
