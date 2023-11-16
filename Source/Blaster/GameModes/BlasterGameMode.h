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
	/// 角色死亡时调用
	/// @param EliminatedPlayer 被淘汰的角色
	/// @param VictimPlayerController 被淘汰的角色的控制器
	/// @param AttackerController 攻击者的控制器
	void PlayerEliminated(
		class ABlasterCharacter* EliminatedPlayer,
		class ABlasterPlayerController* VictimPlayerController,
		ABlasterPlayerController* AttackerController
	);
};
