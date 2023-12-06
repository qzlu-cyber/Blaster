// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION()
	void StartPickupSpawnTimer(AActor* DestroyedActor);
	void SpawnTimerFinished();

private:
	void SpawnPickup();

private:
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses; // 该 spawn point 可以生成的 pickup 类型

	UPROPERTY()
	APickup* SpawnedPickup = nullptr; // 生成的 pickup

	FTimerHandle SpawnTimerHandle; // 用于计时，当计时结束时，生成 pickup
	UPROPERTY(EditAnywhere)
	float RespawnTime = 60.f; // 生成 pickup 的时间间隔

	bool bInitializeSpawn = true; // 是否是在游戏开始时生成 pickup
};
