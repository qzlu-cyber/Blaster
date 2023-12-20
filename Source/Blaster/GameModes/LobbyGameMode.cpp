// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		const int32 NumberOfPlayer = GameState.Get()->PlayerArray.Num();
		const int32 DesiredNumPublicConnections = Subsystem->GetDesiredNumPublicConnections();
		if (NumberOfPlayer == 1)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true; // 设置无缝旅行
				const FString MatchType = Subsystem->GetDesiredMatchType();
				UE_LOG(LogTemp, Warning, TEXT("MatchType: %s"), *MatchType)
				if (MatchType == "FreeForAll") World->ServerTravel(FString("/Game/Maps/Blaster?listen"));
				else if (MatchType == "Teams") World->ServerTravel(FString("/Game/Maps/Teams?listen"));
				else if (MatchType == "CTF") World->ServerTravel(FString("/Game/Maps/CTF?listen"));
			}
		}
	}
}
