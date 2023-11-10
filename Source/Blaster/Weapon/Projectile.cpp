// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true; // 子弹需要同步

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox); // 设置 CollisionBox 为根组件
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // 子弹要飞行，设置碰撞类型为 WorldDynamic
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 设置碰撞检测和物理模拟
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore); // 忽略所有碰撞
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 响应 Visibility 碰撞
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 响应 WorldStatic 碰撞

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 15000.f; // 设置初始速度
	ProjectileMovementComponent->MaxSpeed = 15000.f; // 设置最大速度
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 设置飞行时旋转
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		// 将粒子特效附加到 CollisionBox 上
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			NAME_None,
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

