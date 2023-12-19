// Fill out your copyright notice in the Description page of Project Settings.


#include "ElimAnnouncement.h"

#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(const FString& Attacker, const FString& Victim)
{
	if (ElimAnnouncementBox && ElimAnnouncementText)
	{
		FText ElimAnnouncement = FText::FromString(FString::Printf(TEXT("%s 击杀了 %s"), *Attacker, *Victim));
		ElimAnnouncementText->SetText(ElimAnnouncement);
	}
}
