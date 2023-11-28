// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaSeconds) override;
	
	/// 角色死亡时调用
	/// @param EliminatedPlayer 被淘汰的角色
	/// @param VictimPlayerController 被淘汰的角色的控制器
	/// @param AttackerController 攻击者的控制器
	void PlayerEliminated(
		class ABlasterCharacter* EliminatedPlayer,
		class ABlasterPlayerController* VictimPlayerController,
		ABlasterPlayerController* AttackerController
	);

	void PlayerRespawn(ACharacter* ElimmedPlayer, AController* ElimmedPayerController);

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f; // 热身时间
	float CountdownTime = 0.f; // StartMatch 倒计时
	float StartingLevelTime = 0.f; // 进入关卡的时间
};
