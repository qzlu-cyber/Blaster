// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void UpdateTopScore(class ABlasterPlayerState* ScoringPlayer);

	void UpdateBlueTeamScore();
	void UpdateRedTeamScore();

private:
	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();
	
public:
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers; // 保存得分最高的玩家，可能有多个

	TArray<ABlasterPlayerState*> RedTeam;
	TArray<ABlasterPlayerState*> BlueTeam;
	
private:
	float TopScore = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_RedTeamScore)
	float RedTeamScore = 0.f;
	UPROPERTY(ReplicatedUsing=OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;
};
