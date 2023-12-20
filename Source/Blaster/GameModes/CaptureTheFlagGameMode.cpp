// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagGameMode.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/CaptureTheFlag/FlagZone.h"
#include "Blaster/GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	const ETeam ZoneTeam = Zone->GetTeam();
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
	if (BlasterGameState)
	{
		if (ZoneTeam == ETeam::ET_BlueTeam) BlasterGameState->UpdateBlueTeamScore(5.f);
		if (ZoneTeam == ETeam::ET_RedTeam) BlasterGameState->UpdateRedTeamScore(5.f);
	}
}
