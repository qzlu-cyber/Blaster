// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/GameModes/BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetBlasterHealthHUD(HealthHUD, MaxHealthHUD);
				SetScoreHUD(ScoreHUD);
				SetDeathHUD(DeathHUD);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					SetGrenadeHUD(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();

	TimeSinceLastSync += DeltaSeconds;
	if (IsLocalController() && TimeSinceLastSync > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSinceLastSync = 0.f; // 重置同步时间
	}

	PollInit();
}

/// ReceivedPlayer() 函数只在客户端 PlayerController 上执行，不在服务器上执行
/// ReceivedPlayer() 函数是在客户端 PlayerController 的玩家对象（Pawn）与服务器进行绑定时执行的
/// 当客户端 PlayerController 创建并与服务器建立连接后，服务器会将相应的玩家对象分配给该客户端的 PlayerController
/// 此时，客户端的 PlayerController 将执行 ReceivedPlayer() 函数来接收该玩家对象
void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	if (IsLocalController()) ServerRequestServerTime(GetWorld()->GetTimeSeconds());
}

// 重写父类 OnPossess 函数，一旦拥有 BlasterCharacter(Pawn) 就更新角色的状态
void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter) SetBlasterHealthHUD(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
}

void ABlasterPlayerController::SetBlasterHealthHUD(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		const FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HealthHUD = Health;
		MaxHealthHUD = MaxHealth;
	}
}

void ABlasterPlayerController::SetScoreHUD(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount)
	{
		const FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		ScoreHUD = Score;
	}
}

void ABlasterPlayerController::SetDeathHUD(int32 DeathAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DeathAmount)
	{
		const FString DeathText = FString::Printf(TEXT("%d"), DeathAmount);
		BlasterHUD->CharacterOverlay->DeathAmount->SetText(FText::FromString(DeathText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		DeathHUD = DeathAmount;
	}
}

void ABlasterPlayerController::SetWeaponAmmoHUD(int32 WeaponAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		SetWeaponHUDVisibility(ESlateVisibility::Visible);
		const FString WeaponAmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
	}
}

void ABlasterPlayerController::SetCarriedAmmoHUD(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount)
	{
		SetWeaponHUDVisibility(ESlateVisibility::Visible);
		const FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void ABlasterPlayerController::SetGrenadeHUD(int32 GrenadeAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadeAmount)
	{
		SetWeaponHUDVisibility(ESlateVisibility::Visible);
		const FString GrenadeText = FString::Printf(TEXT("%d"), GrenadeAmount);
		BlasterHUD->CharacterOverlay->GrenadeAmount->SetText(FText::FromString(GrenadeText));
	}
	else
	{
		GrenadeHUD = GrenadeAmount;
	}
}

void ABlasterPlayerController::SetWeaponHUDVisibility(const ESlateVisibility& SlateVisibility)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount &&
		BlasterHUD->CharacterOverlay->MainWeaponImage &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount &&
		BlasterHUD->CharacterOverlay->Slash)
	{
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->Slash->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->MainWeaponImage->SetVisibility(SlateVisibility);
	}
}

void ABlasterPlayerController::SetCountdownHUD(float Countdown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText)
	{
		const uint32 Minutes = FMath::FloorToInt(Countdown / 60);
		const uint32 Seconds = Countdown - (Minutes * 60);
		
		const FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetAnnouncementCountdownHUD(float Countdown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime)
	{
		if (Countdown < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		const uint32 Minutes = FMath::FloorToInt(Countdown / 60);
		const uint32 Seconds = Countdown - (Minutes * 60);
		
		const FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + StartingLevelTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + StartingLevelTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + StartingLevelTime;
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft + StartingLevelTime);
	
	// 每秒更新 HUD
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) SetAnnouncementCountdownHUD(TimeLeft);
		else if (MatchState == MatchState::InProgress) SetCountdownHUD(TimeLeft);
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	if (MatchState == MatchState::InProgress) HandleMatchHasStarted();
	else if (MatchState == MatchState::Cooldown) HandleCooldown();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress) HandleMatchHasStarted();
	else if (MatchState == MatchState::Cooldown) HandleCooldown();
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement) BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden); // 游戏正式开始后隐藏 Announcement
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	if (MatchState == MatchState::Cooldown)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			if (BlasterHUD->CharacterOverlay) BlasterHUD->CharacterOverlay->RemoveFromParent(); // 移除 CharacterOverlayUI
			if (BlasterHUD->Announcement && BlasterHUD->Announcement->AnnouncementText && BlasterHUD->Announcement->InfoText)
			{
				BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible); // 游戏结束后显示 AnnouncementUI
				BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(FString(TEXT("新游戏"))));

				const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				const ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
				if (BlasterGameState && BlasterPlayerState)
				{
					TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
					FString InfoTextString;
				
					if (TopPlayers.Num() == 0) InfoTextString = FString(TEXT("没人获胜..."));
					else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState) InfoTextString = FString(TEXT("恭喜你赢了！"));
					else if (TopPlayers.Num() == 1) InfoTextString = FString::Printf(TEXT("赢家是：\n%s"), *TopPlayers[0]->GetPlayerName());
					else if (TopPlayers.Num() > 1)
					{
						InfoTextString = FString(TEXT("赢家是：\n"));
						for (const auto TiedPlayer : TopPlayers) InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}

					BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
				}
			}
		}

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
		if (BlasterCharacter) BlasterCharacter->SetDisableGameplay(true); // 禁止响应某些输入
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeOfServerReceivedClientRequest)
{
	// 计算 RTT
	const float RTT = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	// 根据 RTT 估算当前服务器时间
	const float CurrentServerTime = TimeOfServerReceivedClientRequest + (0.5f * RTT);
	// 计算客户端和服务器的时间差
	ClientServerDeltaTime = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	if (BlasterGameMode)
	{
		MatchTime = BlasterGameMode->MatchTime;
		WarmupTime = BlasterGameMode->WarmupTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		StartingLevelTime = BlasterGameMode->StartingLevelTime;
		MatchState = BlasterGameMode->GetMatchState(); // 此时 client 端会调用 OnRep_MatchState(), 要确保 MatchTime、WarmupTime、StartingLevelTime 也要同步到 client
		// 使用 client RPC 一同更新 MatchTime、WarmupTime、StartingLevelTime
		ClientJoinMidGame(MatchState, MatchTime, WarmupTime, CooldownTime, StartingLevelTime);

		if (BlasterHUD && MatchState == MatchState::WaitingToStart) BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName State, float TimeOfMatch, float TimeOfWarmup, float TimeOfCooldown, float TimeOfStartingLevel)
{
	MatchTime = TimeOfMatch;
	WarmupTime = TimeOfWarmup;
	CooldownTime = TimeOfCooldown;
	StartingLevelTime = TimeOfStartingLevel;
	MatchState = State;

	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart) BlasterHUD->AddAnnouncement();
}

float ABlasterPlayerController::GetServerTime() const
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDeltaTime;
}
