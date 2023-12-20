// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/Team.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlagZone.generated.h"

UCLASS()
class BLASTER_API AFlagZone : public AActor
{
	GENERATED_BODY()

public:
	AFlagZone();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	FORCEINLINE ETeam GetTeam() const { return Team; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	ETeam Team;
	
	UPROPERTY(EditAnywhere)
	class USphereComponent* SphereComponent;
};
