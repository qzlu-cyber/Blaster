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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetBlasterHealthHUD(float Health, float MaxHealth);
	void SetBlasterShieldHUD(float Shield, float MaxShield);
	void SetScoreHUD(float Score);
	void SetDeathHUD(int32 DeathAmount);
	void SetWeaponAmmoHUD(int32 WeaponAmmo);
	void SetCarriedAmmoHUD(int32 CarriedAmmo);
	void SetWeaponHUDVisibility(const ESlateVisibility& SlateVisibility);
	void SetSecondaryWeaponAmmoHUD(int32 WeaponAmmo);
	void SetSecondaryCarriedAmmoHUD(int32 CarriedAmmo);
	void SetSecondaryWeaponHUDVisibility(const ESlateVisibility& SlateVisibility);
	void SetGrenadeHUD(int32 GrenadeAmount);
	void SetCountdownHUD(float Countdown);
	void SetAnnouncementCountdownHUD(float Countdown);
	virtual void OnPossess(APawn* InPawn) override;

	virtual float GetServerTime() const;

	void OnMatchStateSet(FName State);

protected:
	virtual void BeginPlay() override;

	void PollInit();

private:
	void SetHUDTime();

	/// 同步 clients 和 server 的时间
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOfServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName State, float TimeOfMatch, float TimeOfWarmup, float TimeOfCooldown, float TimeOfStartingLevel);

	UFUNCTION()
	void OnRep_MatchState();

	void HandleMatchHasStarted();
	void HandleCooldown();

private:
	// PlayerController 才可以拿到 HUD
	UPROPERTY()
	class ABlasterHUD* BlasterHUD; // Blaster HUD

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float WarmupTime = 0.f;
	float MatchTime = 0.f;
	float StartingLevelTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0.f;
	float ClientServerDeltaTime;
	
	UPROPERTY(EditAnywhere)
	float TimeSyncFrequency = 5.f; // 5 秒同步一次时间
	float TimeSinceLastSync = 0.f; // 上次同步时间

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	float HealthHUD;
	float MaxHealthHUD;
	bool bInitializeHealth = false;
	float ShieldHUD;
	float MaxShieldHUD;
	bool bInitializeShield = false;
	float ScoreHUD;
	bool bInitializeScore = false;
	int32 DeathHUD;
	bool bInitializeDeath = false;
	int32 GrenadeHUD;
	bool bInitializeGrenade = false;
	int32 WeaponAmmoHUD;
	bool bInitializeWeaponAmmo = false;
	int32 CarriedAmmoHUD;
	bool bInitializeCarriedAmmo = false;
	int32 SecondaryWeaponAmmoHUD;
	bool bInitializeSecondaryWeaponAmmo = false;
	int32 SecondaryCarriedAmmoHUD;
	bool bInitializeSecondaryCarriedAmmo = false;
};
