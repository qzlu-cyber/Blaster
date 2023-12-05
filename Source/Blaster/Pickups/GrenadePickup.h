// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GrenadePickup.generated.h"

UCLASS()
class BLASTER_API AGrenadePickup : public APickup
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
	int32 GrenadeAmount = 1; // 手雷数量
};
