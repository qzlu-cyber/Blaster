// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();

	virtual void Destroyed() override;

protected:
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& HitResult
	) override;

	virtual void BeginPlay() override;

private:
	void DestroyTimeFinished();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	UPROPERTY()
	class UNiagaraComponent* TrailComponent;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem; // Rocket Launcher TrailSmoke

	// TrailSmoke 延迟销毁
	FTimerHandle DestroyTimer;
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	// 导弹飞行时的音效
	UPROPERTY(EditAnywhere)
	class USoundCue* ProjectileLoop;
	UPROPERTY()
	class UAudioComponent* ProjectileLoopComponent;
	UPROPERTY(EditAnywhere)
	class USoundAttenuation* ProjectileLoopAttenuation;
};
