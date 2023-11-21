// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(ScoreAmount + GetScore());
	
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->Controller) : BlasterPlayerController;
		if (BlasterPlayerController) BlasterPlayerController->SetScoreHUD(GetScore());
	}
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->Controller) : BlasterPlayerController;
		if (BlasterPlayerController) BlasterPlayerController->SetScoreHUD(GetScore());
	}
}
