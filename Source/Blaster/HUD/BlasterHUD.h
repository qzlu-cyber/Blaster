// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	class UTexture2D* CrosshiresCenter;
	UTexture2D* CrosshiresLeft;
	UTexture2D* CrosshiresRight;
	UTexture2D* CrosshiresTop;
	UTexture2D* CrosshiresBottom;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& NewHUDPackage) { HUDPackage = NewHUDPackage; }

private:
	void DrawCrosshire(class UTexture2D* Texture, const FVector2D& DrawPoint);

private:
	FHUDPackage HUDPackage;
};
