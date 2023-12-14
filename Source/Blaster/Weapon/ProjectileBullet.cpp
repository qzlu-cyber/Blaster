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
	ProjectileMovementComponent->InitialSpeed = InitialSpeed; // 设置初始速度
	ProjectileMovementComponent->MaxSpeed = InitialSpeed; // 设置最大速度
	ProjectileMovementComponent->bRotationFollowsVelocity = true; // 设置飞行时旋转
	ProjectileMovementComponent->SetIsReplicated(true);
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

// Called when the game starts or when spawned
void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	FPredictProjectilePathParams PredictParams;
	PredictParams.ActorsToIgnore.Add(this);
	PredictParams.StartLocation = GetActorLocation(); // 设置起始位置
	PredictParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed; // 设置发射速度
	PredictParams.ProjectileRadius = 5.f;
	PredictParams.MaxSimTime = 4.f; // 设置最大模拟时间
	PredictParams.SimFrequency = 30.f; // 设置模拟频率，越高越精确
	PredictParams.DrawDebugTime = 5.f;
	PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PredictParams.bTraceWithCollision = true; // 设置是否使用碰撞
	PredictParams.bTraceWithChannel = true;
	PredictParams.TraceChannel = ECC_Visibility; // 设置碰撞通道

	FPredictProjectilePathResult PredictResult;

	UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);
	
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& HitResult)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()); // 获取子弹的拥有者，在 ProjectileWeapon 子弹的拥有者被设置为武器的拥有者
	if (OwnerCharacter && GetOwner() != OtherActor) // 如果拥有者存在且命中的不是拥有者自己
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

