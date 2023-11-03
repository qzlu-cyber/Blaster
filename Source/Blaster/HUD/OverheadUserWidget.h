// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UOverheadUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDisplayText(const FString& Text);
	UFUNCTION(BlueprintCallable)
	void ShowDisplayText(APawn* InPawn);

protected:
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;
};
