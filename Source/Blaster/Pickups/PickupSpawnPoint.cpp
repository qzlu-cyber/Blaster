// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) SpawnPickup();
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup()
{
	const int32 NumOfSpawnClasses = PickupClasses.Num();
	if (NumOfSpawnClasses > 0)
	{
		const int32 Selection = FMath::RandRange(0, NumOfSpawnClasses - 1);
		
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// 当 pickup 被销毁时，重新启动生成 pickup 的计时器
		if (HasAuthority() && SpawnedPickup) SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartPickupSpawnTimer);
	}
}

void APickupSpawnPoint::StartPickupSpawnTimer(AActor* DestroyedActor)
{
	// 定时器结束时，重新生成 pickup
	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&APickupSpawnPoint::SpawnTimerFinished,
		RespawnTime
	);
}

void APickupSpawnPoint::SpawnTimerFinished()
{
	if (HasAuthority()) SpawnPickup();
}

