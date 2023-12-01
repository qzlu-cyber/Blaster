// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"


// Sets default values
AProjectileBullet::AProjectileBullet()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 15000.f; // 设置初始速度
	ProjectileMovementComponent->MaxSpeed = 15000.f; // 设置最大速度
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 设置飞行时旋转
	ProjectileMovementComponent->SetIsReplicated(true);
}

// Called when the game starts or when spawned
void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& HitResult)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); // 获取子弹的拥有者，在 ProjectileWeapon 子弹的拥有者被设置为武器的拥有者
	if (OwnerCharacter)
	{
		AController* OwnerController = Cast<AController>(OwnerCharacter->Controller); // 获取拥有者的控制器
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass()); // 对命中的 Actor 造成伤害
		}
	}
	
	// 父类中销毁了 Actor，最后执行父类的 OnHit 函数
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, HitResult);
}

// Called every frame
void AProjectileBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

