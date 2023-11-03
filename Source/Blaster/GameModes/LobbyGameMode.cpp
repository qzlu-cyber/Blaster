// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GameState)
	{
		const int32 NumberOfPlayer = GameState.Get()->PlayerArray.Num();
		if (NumberOfPlayer == 1)
		{
			bUseSeamlessTravel = true; // 设置无缝旅行
			if (UWorld* World = GetWorld()) World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
		}
	}
}
