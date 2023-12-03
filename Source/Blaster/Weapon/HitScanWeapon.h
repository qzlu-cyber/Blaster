// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactEffect; // 击中特效
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamEffect; // 射线特效
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash; // 枪口火焰特效
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound; // 开火音效
	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound; // 击中音效
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

private:
	/// Trace end with scatter 
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;
};
