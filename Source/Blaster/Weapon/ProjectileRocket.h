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

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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
	// 导弹飞行时的音效
	UPROPERTY(EditAnywhere)
	class USoundCue* ProjectileLoop;
	UPROPERTY()
	class UAudioComponent* ProjectileLoopComponent;
	UPROPERTY(EditAnywhere)
	class USoundAttenuation* ProjectileLoopAttenuation;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 2000.f;
};
