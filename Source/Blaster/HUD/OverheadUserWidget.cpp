// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadUserWidget.h"

#include "Components/TextBlock.h"


void UOverheadUserWidget::NativeDestruct()
{
	RemoveFromParent(); // 从视口移除 Widget
	Super::NativeDestruct();
}

void UOverheadUserWidget::SetDisplayText(const FString& Text, const FColor& Color)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(Text));
		DisplayText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UOverheadUserWidget::ShowDisplayText(APawn* InPawn)
{
	const ENetRole LocalPlayer = InPawn->GetLocalRole(); // 获取本地角色
	FString Role;
	switch (LocalPlayer)
	{
		case ROLE_Authority: Role = "Authority"; break;
		case ROLE_AutonomousProxy: Role = "AutonomousProxy"; break;
		case ROLE_SimulatedProxy: Role = "SimulatedProxy"; break;
		case ROLE_None: Role = "None"; break;
		case ROLE_MAX: Role = "MAX"; break;
	}
	const FString LocalRoleString = FString::Printf(TEXT("LocalPlayer: %s"), *Role);
}
