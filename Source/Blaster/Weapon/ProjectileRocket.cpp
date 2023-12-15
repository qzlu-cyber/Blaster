// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "RocketMovementComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket Movement Component"));
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

#if WITH_EDITOR
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	/// 当 ProjectileRocket 击中时，需要其停止生成 TrailSmoke 、隐藏 Mesh 并播放动画和音效，等待计时器结束后销毁
	/// 但 OnHit() 在父类中只能由 server 端调用，为了实现上述需求，要在子类 client 端绑定 OnHit() 回调
	/// 这样所有 machines 都能调用 OnHit()，实现上述需求
	if (!HasAuthority()) CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

	// 生成 TrailSmoke
	SpawnTrailSystem();

	if (ProjectileLoop && ProjectileLoopAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop, // Sound
			GetRootComponent(), // AttachToComponent
			FName(), // AttachPointName
			GetActorLocation(), // Location
			EAttachLocation::KeepWorldPosition, // LocationType
			false, // bStopWhenAttachedToDestroyed
			.25f, // VolumeMultiplier
			.55f, // PitchMultiplier
			0.f, // StartTime
			ProjectileLoopAttenuation, // AttenuationSettings
			nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& HitResult)
{
	if (OtherActor == GetOwner()) return; // 不对发射者造成伤害
	
	ExplodeDamage();

	/// 如果直接调用 Super::OnHit()，则 TrailSmoke 会立即消失；如果延迟调用 Super::OnHit() 则爆炸效果和声音也会一并延迟
	StartDestroyTimer();
	
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
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ImpactSound,
			GetActorLocation(),
			1.5f,
			1.5f
		);
	}

	// 隐藏 RocketMesh
	if (ProjectileMesh) ProjectileMesh->SetVisibility(false);
	if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 停止 TrailSmoke
	if (TrailComponent && TrailComponent->GetSystemInstanceController()) TrailComponent->GetSystemInstanceController()->Deactivate();

	// 停止 ProjectileLoop
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying()) ProjectileLoopComponent->Stop();
}

void AProjectileRocket::Destroyed()
{
	
}
