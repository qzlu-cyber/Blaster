// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "SpeedPickup.generated.h"

UCLASS()
class BLASTER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedSphere,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	) override;

private:
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 1600.f; // buff 后行走速度
	UPROPERTY(EditAnywhere)
	float BaseCrouchSpeed = 850.f; // buff 后下蹲行走速度
	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 10.f; // buff 持续时间
};
