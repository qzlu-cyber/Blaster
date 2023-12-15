// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Blaster/Blaster.h"

#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

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
	/// 将碰撞体设为 SkeletaMesh 而非 CapsuleComponent
	//CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // 响应 Pawn 碰撞
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletaMesh, ECR_Block); // 响应 SkeletalMesh 碰撞
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

	if (HasAuthority()) // 只在服务器端绑定命中特效和音效
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimeFinished,
		DestroyTime,
		false
	);
}

void AProjectile::DestroyTimeFinished()
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem, // NiagaraSystem
			GetRootComponent(), // AttachToComponent
			FName(), // AttachPointName
			GetActorLocation(), // Location
			GetActorRotation(), // Rotation
			EAttachLocation::KeepWorldPosition, // LocationType
			false // bAutoDestroy
		);
	}
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator(); // 获取发射者
	if (FiringPawn && HasAuthority()) // 确保只在 server 端造成伤害
		{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			TArray<AActor*> IgnoreActors;
			IgnoreActors.Emplace(GetOwner()); // 不对发射者造成伤害
			// 造成伤害
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // WorldContextObject
				Damage, // BaseDamage
				5.f, // MinimumDamageRadius
				GetActorLocation(), // Origin
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.f, // DamageFalloff, 线性
				UDamageType::StaticClass(), // DamageTypeClass
				IgnoreActors, // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatedByController
			);
		}
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& HitResult)
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles)
	{
		// 在命中位置播放命中特效
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			GetActorTransform()
		);
	}
	if (ImpactSound)
	{
		// 在命中位置播放命中音效
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	}
}

