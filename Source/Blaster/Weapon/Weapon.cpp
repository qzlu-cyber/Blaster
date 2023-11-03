// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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
	if (Character) Character->SetWeaponOverlapping(this);
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
	if (Character) Character->SetWeaponOverlapping(nullptr);
}

void AWeapon::ShowWeaponPickupWidget(bool bShowWidget)
{
	if (PickupWidget) PickupWidget->SetVisibility(bShowWidget);
}

