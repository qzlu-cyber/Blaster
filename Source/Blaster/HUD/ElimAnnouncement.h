// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetElimAnnouncementText(const FString& Attacker, const FString& Victim);

private:
	UPROPERTY(meta=(BindWidget))
	class UVerticalBox* ElimAnnouncementBox;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* ElimAnnouncementText;
};
