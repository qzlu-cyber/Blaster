// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	AFlag();

	void ResetFlag();

	FORCEINLINE UStaticMeshComponent* GetFlahMesh() const { return FlagMesh; }

protected:
	virtual void BeginPlay() override;
	
	virtual void OnEquipped() override;
	virtual void OnDropped() override;

private:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* FlagMesh;

	FTransform InitialTransform;
};
