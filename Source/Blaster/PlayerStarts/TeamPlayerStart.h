// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/Team.h"

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TeamPlayerStart.generated.h"

UCLASS()
class BLASTER_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }

private:
	UPROPERTY(EditAnywhere)	
	ETeam Team;
};
