// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BlasterGameState->BlueTeam.Num() > BlasterGameState->RedTeam.Num())
			{
				BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				BlasterGameState->RedTeam.Emplace(BlasterPlayerState);
			}
			else
			{
				BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				BlasterGameState->BlueTeam.Emplace(BlasterPlayerState);
			}
		}
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			if (BlasterGameState->BlueTeam.Contains(BlasterPlayerState)) BlasterGameState->BlueTeam.Remove(BlasterPlayerState);
			if (BlasterGameState->RedTeam.Contains(BlasterPlayerState)) BlasterGameState->RedTeam.Remove(BlasterPlayerState);
		}
	}
}

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		for (auto PlayerState : BlasterGameState->PlayerArray)
		{
			ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState);
			if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->BlueTeam.Num() > BlasterGameState->RedTeam.Num())
				{
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
					BlasterGameState->RedTeam.Emplace(BlasterPlayerState);
				}
				else
				{
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
					BlasterGameState->BlueTeam.Emplace(BlasterPlayerState);
				}
			}
		}
	}
}

float ATeamGameMode::CalculateDamage(AController* Attacher, AController* Victim, float BaseDamage) const
{
	ABlasterPlayerState* AttackerPlayerState = Attacher->GetPlayerState<ABlasterPlayerState>();
	ABlasterPlayerState* VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();

	// 同一队伍不受伤害
	if (AttackerPlayerState && VictimPlayerState && AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return 0.f;

	return BaseDamage;
}
