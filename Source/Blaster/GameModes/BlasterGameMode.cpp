// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedPlayer,
                                        ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimPlayerController ? Cast<ABlasterPlayerState>(VictimPlayerController->PlayerState) : nullptr;

	if (AttackerPlayerState && VictimPlayerState && AttackerPlayerState != VictimPlayerState) // 有攻击者和被攻击者
	{
		AttackerPlayerState->AddToScore(1.0f); // 攻击者加分
		// VictimPlayerState->AddToScore(-1.0f); // 被攻击者减分
	}
	
	if (EliminatedPlayer) EliminatedPlayer->Elim();
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
