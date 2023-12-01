// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "RocketMovementComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket Movement Component"));
	ProjectileMovementComponent->InitialSpeed = 2000.f;
	ProjectileMovementComponent->MaxSpeed = 2000.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	/// 当 ProjectileRocket 击中时，需要其停止生成 TrailSmoke 、隐藏 Mesh 并播放动画和音效，等待计时器结束后销毁
	/// 但 OnHit() 在父类中只能由 server 端调用，为了实现上述需求，要在子类 client 端绑定 OnHit() 回调
	/// 这样所有 machines 都能调用 OnHit()，实现上述需求
	if (!HasAuthority()) CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

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

	if (ProjectileLoop && ProjectileLoopAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop, // Sound
			GetRootComponent(), // AttachToComponent
			FName(), // AttachPointName
			GetActorLocation(), // Location
			EAttachLocation::KeepWorldPosition, // LocationType
			false, // bStopWhenAttachedToDestroyed
			1.f, // VolumeMultiplier
			1.f, // PitchMultiplier
			0.f, // StartTime
			ProjectileLoopAttenuation, // AttenuationSettings
			nullptr,
			false
		);
	}
}

void AProjectileRocket::DestroyTimeFinished()
{
	Destroy();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& HitResult)
{
	if (OtherActor == GetOwner()) return; // 不对发射者造成伤害
	
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
				200.f, // DamageInnerRadius
				500.f, // DamageOuterRadius
				1.f, // DamageFalloff, 线性
				UDamageType::StaticClass(), // DamageTypeClass
				IgnoreActors, // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatedByController
			);
		}
	}

	/// 如果直接调用 Super::OnHit()，则会 TrailSmoke 会立即消失；如果延迟调用 Super::OnHit() 则爆炸效果和声音也会一并延迟
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimeFinished,
		DestroyTime
	);
	
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

	// 隐藏 RocketMesh
	if (RocketMesh) RocketMesh->SetVisibility(false);
	if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 停止 TrailSmoke
	if (TrailComponent && TrailComponent->GetSystemInstanceController()) TrailComponent->GetSystemInstanceController()->Deactivate();

	// 停止 ProjectileLoop
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying()) ProjectileLoopComponent->Stop();
}

void AProjectileRocket::Destroyed()
{
	
}
