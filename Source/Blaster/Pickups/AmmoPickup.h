// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Pickup.h"
#include "Blaster/Weapon/WeaponTypes.h"

#include "CoreMinimal.h"
#include "AmmoPickup.generated.h"

UCLASS()
class BLASTER_API AAmmoPickup : public APickup
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
	int32 AmmoAmount; // 子弹数量

	UPROPERTY(EditAnywhere)
	EWeaponTypes WeaponType; // 哪种武器的子弹
};
