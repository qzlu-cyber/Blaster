// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPingTooHighDelegate, bool, bPingTooHigh);

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

	void HighPingWarning();
	void StopHighPingWarning();
	
	void InitialTeamText();
	void SetBlueTeamScoreHUD(int32 BlueTeamScore);
	void SetRedTeamScoreHUD(int32 RedTeamScore);
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual float GetServerTime() const;

	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	void PollInit();
	void CheckPing(float DeltaSeconds);

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

private:
	void SetHUDTime();

	/// 同步 clients 和 server 的时间
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOfServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bPingTooHigh);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName State, float TimeOfMatch, float TimeOfWarmup, float TimeOfCooldown, float TimeOfStartingLevel);

	UFUNCTION()
	void OnRep_MatchState();

	void HandleMatchHasStarted(bool bTeamsMatch = false);
	void HandleCooldown();

	void ShowReturnToMainMenuWidget(const struct FInputActionValue& Value);

	UFUNCTION()
	void OnRep_ShowTeamsText();

	FString GetInfoText(const TArray<class ABlasterPlayerState*>& Players);
	FString GetTeamsInfoText(class ABlasterGameState* BlasterGameState);

public:
	FPingTooHighDelegate PingTooHighDelegate;

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
	float SingleTripTime; // client 和 server 之间的单程时间
	
	UPROPERTY(EditAnywhere)
	float TimeSyncFrequency = 5.f; // 5 秒同步一次时间
	float TimeSinceLastSync = 0.f; // 上次同步时间

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 100.f; // 延迟超过 100ms 就显示高延迟图片
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 10.f; // 10 秒检测一次 Ping
	float PingCheckLastTime = 0.f; // 上次检测 Ping 的时间

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	class UInputMappingContext* InputMapping;
	// return to main menu action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="EnhancedInput|Action", meta=(AllowPrivateAccess="true"))
	class UInputAction* ExitAction;

	UPROPERTY(EditAnywhere, Category=HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidgetClass;
	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenuWidget;
	bool bReturnToMainMenuWidgetShown = false;

	UPROPERTY(ReplicatedUsing=OnRep_ShowTeamsText)
	bool bShowTeamsText = false;

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
