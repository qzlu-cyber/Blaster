// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedSphere,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:	
	virtual void Tick(float DeltaTime) override;

private:
	void BindOverlapTimerFinished();

private:
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* OverlapSphere;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;
	
	UPROPERTY(EditAnywhere)
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraComponent* PickupEffectComponent; // pickup 特效

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* PickEffect; // 拾取后 pickup 销毁的特效

	FTimerHandle BindOverlapTimer;
	UPROPERTY(EditAnywhere)
	float BindOverlapTime = 0.5f; // 生成后多久开始检测碰撞
};
