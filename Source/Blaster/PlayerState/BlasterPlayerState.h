// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRep_Score() override;
	void AddToScore(float Score); // 用于为 server 端更新 Score
	UFUNCTION()
	virtual void OnRep_Death();
	void AddToDeath(int32 DeathAmount);

private:
	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	UPROPERTY(ReplicatedUsing=OnRep_Death)
	int32 Death; // 记录玩家死亡次数
};
