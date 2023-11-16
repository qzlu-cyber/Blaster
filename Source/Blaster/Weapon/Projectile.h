// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

	// 在游戏过程中 Actor 被销毁（击中目标）时调用
	// 不仅可以在 server 端调用，由于 Projectile 具有 replicates 属性， 复制后，client 端也将调用此函数
	// 所以，在此函数中播放命中特效和音效，可以保证所有客户端都能看到和听到
	virtual void Destroyed() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 子弹命中回调
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	float Damage = 20.f; // 子弹的伤害值

private:
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // 子弹的粒子特效

	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* TracerComponent; // 子弹的粒子特效组件

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // 子弹的命中特效

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound; // 子弹的命中音效
};
