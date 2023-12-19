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
#include "Blaster/HUD/ReturnToMainMenu.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (EnhancedInputSubsystem) EnhancedInputSubsystem->AddMappingContext(InputMapping, 1000);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		if (ExitAction)
		{
			EnhancedInputComponent->BindAction(ExitAction, ETriggerEvent::Triggered, this, &ABlasterPlayerController::ShowReturnToMainMenuWidget);
		}
	}
}

void ABlasterPlayerController::ShowReturnToMainMenuWidget(const FInputActionValue& Value)
{
	if (ReturnToMainMenuWidgetClass == nullptr) return;

	if (ReturnToMainMenuWidget == nullptr) ReturnToMainMenuWidget = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidgetClass);
	if (ReturnToMainMenuWidget)
	{
		bReturnToMainMenuWidgetShown = !bReturnToMainMenuWidgetShown;
		if (bReturnToMainMenuWidgetShown) ReturnToMainMenuWidget->MenuSetup();
		else ReturnToMainMenuWidget->MenuTeardown();
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamsText);
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
				if (bInitializeHealth) SetBlasterHealthHUD(HealthHUD, MaxHealthHUD);
				if (bInitializeShield) SetBlasterShieldHUD(ShieldHUD, MaxShieldHUD);
				if (bInitializeScore) SetScoreHUD(ScoreHUD);
				if (bInitializeDeath) SetDeathHUD(DeathHUD);
				if (bInitializeCarriedAmmo) SetCarriedAmmoHUD(CarriedAmmoHUD);
				if (bInitializeWeaponAmmo) SetWeaponAmmoHUD(WeaponAmmoHUD);
				if (bInitializeSecondaryCarriedAmmo) SetSecondaryCarriedAmmoHUD(SecondaryCarriedAmmoHUD);
				if (bInitializeSecondaryWeaponAmmo) SetSecondaryWeaponAmmoHUD(SecondaryWeaponAmmoHUD);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent() && bInitializeGrenade)
				{
					SetGrenadeHUD(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	if (HasAuthority()) return;
	
	PingCheckLastTime += DeltaSeconds;
	if (PingCheckLastTime >= CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetCompressedPing() * 4 >= HighPingThreshold)
			{
				HighPingWarning();
				ServerReportPingStatus(true);
			}
			else
			{
				StopHighPingWarning();
				ServerReportPingStatus(false);
			}
		}
		PingCheckLastTime = 0.f;
	}
}

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Self && Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			if (Self == Attacker && Self != Victim) BlasterHUD->AddElimAnnouncement(FString::Printf(TEXT("你")), Victim->GetPlayerName());
			else if (Self == Victim && Self != Attacker) BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), FString::Printf(TEXT("你")));
			else if (Self == Attacker && Self == Victim) BlasterHUD->AddElimAnnouncement(FString::Printf(TEXT("你")), FString::Printf(TEXT("自己")));
			else if (Attacker == Victim && Attacker != Self) BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), FString::Printf(TEXT("自己")));
			else BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
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

	CheckPing(DeltaSeconds);

	PollInit();
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bPingTooHigh)
{
	PingTooHighDelegate.Broadcast(bPingTooHigh);
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
		bInitializeHealth = true;
		HealthHUD = Health;
		MaxHealthHUD = MaxHealth;
	}
}

void ABlasterPlayerController::SetBlasterShieldHUD(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText)
	{
		const float ShieldPercent = Shield / MaxShield;
		const FString ShieldText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		ShieldHUD = Shield;
		MaxShieldHUD = MaxShield;
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
		bInitializeScore = true;
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
		bInitializeDeath = true;
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
	else
	{
		bInitializeWeaponAmmo = true;
		WeaponAmmoHUD = WeaponAmmo;
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
	else
	{
		bInitializeCarriedAmmo = true;
		CarriedAmmoHUD = CarriedAmmo;
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
		bInitializeGrenade = true;
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

void ABlasterPlayerController::SetSecondaryWeaponAmmoHUD(int32 WeaponAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->SecondaryWeaponAmmoAmount)
	{
		SetSecondaryWeaponHUDVisibility(ESlateVisibility::Visible);
		const FString SecondaryWeaponAmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
		BlasterHUD->CharacterOverlay->SecondaryWeaponAmmoAmount->SetText(FText::FromString(SecondaryWeaponAmmoText));
	}
	else
	{
		bInitializeSecondaryWeaponAmmo = true;
		SecondaryWeaponAmmoHUD = WeaponAmmo;
	}
}

void ABlasterPlayerController::SetSecondaryCarriedAmmoHUD(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->SecondaryCarriedAmmoAmount)
	{
		SetSecondaryWeaponHUDVisibility(ESlateVisibility::Visible);
		const FString SecondaryCarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->SecondaryCarriedAmmoAmount->SetText(FText::FromString(SecondaryCarriedAmmoText));
	}
	else
	{
		bInitializeSecondaryCarriedAmmo = true;
		SecondaryCarriedAmmoHUD = CarriedAmmo;
	}
}

void ABlasterPlayerController::SetSecondaryWeaponHUDVisibility(const ESlateVisibility& SlateVisibility)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->SecondaryWeaponAmmoAmount &&
		BlasterHUD->CharacterOverlay->SecondaryWeaponImage &&
		BlasterHUD->CharacterOverlay->SecondaryCarriedAmmoAmount &&
		BlasterHUD->CharacterOverlay->SecondarySlash)
	{
		BlasterHUD->CharacterOverlay->SecondaryWeaponAmmoAmount->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->SecondaryCarriedAmmoAmount->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->SecondarySlash->SetVisibility(SlateVisibility);
		BlasterHUD->CharacterOverlay->SecondaryWeaponImage->SetVisibility(SlateVisibility);
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

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation, 0.f, 10);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::HideTeamText()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamText &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->RedTeamText &&
		BlasterHUD->CharacterOverlay->RedTeamScore)
	{
		BlasterHUD->CharacterOverlay->BlueTeamText->SetText(FText());
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->RedTeamText->SetText(FText());
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
	}
}

void ABlasterPlayerController::InitialTeamText()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamText &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->RedTeamText &&
		BlasterHUD->CharacterOverlay->RedTeamScore)
	{
		const FString Zero = FString::Printf(TEXT("%d"), 0);
		const FString BlueTeamText = FString::Printf(TEXT("同盟国"));
		const FString RedTeamText = FString::Printf(TEXT("轴心国"));
		BlasterHUD->CharacterOverlay->BlueTeamText->SetText(FText::FromString(BlueTeamText));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->CharacterOverlay->RedTeamText->SetText(FText::FromString(RedTeamText));
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));

		UE_LOG(LogTemp, Warning, TEXT("InitialTeamText"))
	}
}

void ABlasterPlayerController::SetBlueTeamScoreHUD(int32 BlueTeamScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamScore)
	{
		const FString ScoreText = FString::Printf(TEXT("%d"), BlueTeamScore);
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetRedTeamScoreHUD(int32 RedTeamScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->RedTeamScore)
	{
		const FString ScoreText = FString::Printf(TEXT("%d"), RedTeamScore);
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	
	if (MatchState == MatchState::InProgress) HandleMatchHasStarted(bTeamsMatch);
	else if (MatchState == MatchState::Cooldown) HandleCooldown();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress) HandleMatchHasStarted();
	else if (MatchState == MatchState::Cooldown) HandleCooldown();
}

void ABlasterPlayerController::OnRep_ShowTeamsText()
{
	if (bShowTeamsText) InitialTeamText();
	else HideTeamText();
}

void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if (HasAuthority()) bShowTeamsText = bTeamsMatch;
	
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement) BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden); // 游戏正式开始后隐藏 Announcement

		if (!HasAuthority()) return;
		
		if (bTeamsMatch) InitialTeamText();
		else HideTeamText();
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
	SingleTripTime = RTT * 0.5f;
	// 根据 RTT 估算当前服务器时间
	const float CurrentServerTime = TimeOfServerReceivedClientRequest + SingleTripTime;
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
