// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/GameModes/BlasterGameMode.h"
#include "Blaster/HUD/Announcement.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD) BlasterHUD->AddAnnouncement();
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

void ABlasterPlayerController::SetHUDTime()
{
	const uint32 SecondsLeft = FMath::CeilToInt(MatchCountdown - GetServerTime());
	// 每秒更新 HUD
	if (CountdownInt != SecondsLeft) SetCountdownHUD(MatchCountdown - GetServerTime());

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	HandleMatchHasStarted();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	HandleMatchHasStarted();
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	if (MatchState == MatchState::InProgress)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			BlasterHUD->AddCharacterOverlay();
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden); // 游戏正式开始后隐藏 AnnouncementUI
		}
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
	ClientServerDeltaTime =  CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime() const
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDeltaTime;
}
