// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetBlasterHealthHUD(float Health, float MaxHealth);
	void SetScoreHUD(float Score);
	void SetDeathHUD(int32 DeathAmount);
	void SetWeaponAmmoHUD(int32 WeaponAmmo);
	void SetWeaponHUDVisibility(const ESlateVisibility& SlateVisibility);
	virtual void OnPossess(APawn* InPawn) override;

protected:
	virtual void BeginPlay() override;

private:
	// PlayerController 才可以拿到 HUD
	UPROPERTY()
	class ABlasterHUD* BlasterHUD; // Blaster HUD
};
