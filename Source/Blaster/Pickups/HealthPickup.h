// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

UCLASS()
class BLASTER_API AHealthPickup : public APickup
{
	GENERATED_BODY()

public:
	AHealthPickup();

	virtual void Destroyed() override;

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
	float HealAmount = 100.f; // 恢复的血量
	UPROPERTY(EditAnywhere)
	float HealTime = 2.f; // 恢复血量的时间

	UPROPERTY(EditAnywhere)
	class UNiagaraComponent* PickupEffectComponent; // pickup 特效

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* PickEffect; // 拾取后 pickup 销毁的特效
};
