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
	virtual void Tick(float DeltaSeconds) override;
	virtual void ReceivedPlayer() override;

public:
	void SetBlasterHealthHUD(float Health, float MaxHealth);
	void SetScoreHUD(float Score);
	void SetDeathHUD(int32 DeathAmount);
	void SetWeaponAmmoHUD(int32 WeaponAmmo);
	void SetCarriedAmmoHUD(int32 CarriedAmmo);
	void SetWeaponHUDVisibility(const ESlateVisibility& SlateVisibility);
	void SetCountdownHUD(float Countdown);
	virtual void OnPossess(APawn* InPawn) override;

	virtual float GetServerTime() const;

protected:
	virtual void BeginPlay() override;

private:
	void SetHUDTime();

	/// 同步 clients 和 server 的时间
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOfServerReceivedClientRequest);

private:
	// PlayerController 才可以拿到 HUD
	UPROPERTY()
	class ABlasterHUD* BlasterHUD; // Blaster HUD

	float MatchCountdown = 120.f;
	uint32 CountdownInt = 0.f;
	float ClientServerDeltaTime;
	
	UPROPERTY(EditAnywhere)
	float TimeSyncFrequency = 5.f; // 5 秒同步一次时间
	float TimeSinceLastSync = 0.f; // 上次同步时间
};
