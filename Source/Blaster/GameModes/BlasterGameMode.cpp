// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}


ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	StartingLevelTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
		if (CountdownTime <= 0.f) StartMatch();
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
		if (CountdownTime < 0.f) SetMatchState(MatchState::Cooldown);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
		if (CountdownTime <= 0.f) RestartGame();
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// 遍历 server 上的所有 PlayerController 并设置它们的 MatchState
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayerController)  BlasterPlayerController->OnMatchStateSet(MatchState);
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedPlayer,
                                        ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimPlayerController ? Cast<ABlasterPlayerState>(VictimPlayerController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	if (AttackerPlayerState && VictimPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState) // 有攻击者和被攻击者
	{
		AttackerPlayerState->AddToScore(1.0f); // 攻击者增加击杀次数
		BlasterGameState->UpdateTopScore(AttackerPlayerState); // 检查是否需要更新最高得分玩家
		VictimPlayerState->AddToDeath(1); // 被攻击者增加死亡次数
	}
	
	if (EliminatedPlayer) EliminatedPlayer->Elim(false);
}

void ABlasterGameMode::PlayerRespawn(ACharacter* ElimmedPlayer, AController* ElimmedPayerController)
{
	if (ElimmedPlayer)
	{
		ElimmedPlayer->Reset(); // 重置角色
		ElimmedPlayer->Destroy(); // 销毁角色
	}
	if (ElimmedPayerController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 PlayerStartIndex = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedPayerController, PlayerStarts[PlayerStartIndex]); // 重生角色
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* LeavingPlayer)
{
	if (LeavingPlayer == nullptr) return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	// 如果退出游戏的玩家是 TopScoringPlayers 之一，删除
	if (BlasterGameState &&
		BlasterGameState->TopScoringPlayers.Contains(LeavingPlayer))  BlasterGameState->TopScoringPlayers.Remove(LeavingPlayer);

	ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(LeavingPlayer->GetPawn());
	if (LeavingCharacter) LeavingCharacter->Elim(true); // 播放退出动画
}
