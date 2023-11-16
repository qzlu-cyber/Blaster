// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	float CrosshairSpread;

	FLinearColor CrosshairColor;
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

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& NewHUDPackage) { HUDPackage = NewHUDPackage; }

private:
	void DrawCrosshair(class UTexture2D* Texture, const FVector2D& DrawPoint, const FVector2D& Spread);

	void AddCharacterOverlay();

public:
	class UCharacterOverlay* CharacterOverlay; // 角色信息 UI

private:
	FHUDPackage HUDPackage;

	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category="Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass; // 角色信息 UI 类
};
