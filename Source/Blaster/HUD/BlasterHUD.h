// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	UPROPERTY()
	class UTexture2D* CrosshairsCenter;
	UPROPERTY()
	UTexture2D* CrosshairsLeft;
	UPROPERTY()
	UTexture2D* CrosshairsRight;
	UPROPERTY()
	UTexture2D* CrosshairsTop;
	UPROPERTY()
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

	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(const FString& Attacker, const FString& Victim);

private:
	void DrawCrosshair(UTexture2D* Texture, const FVector2D& DrawPoint, const FVector2D& Spread);

public:
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay; // 角色信息 UI

	UPROPERTY()
	class UAnnouncement* Announcement; // 公告 UI

private:
	UPROPERTY()
	APlayerController* OwningPlayerController;
	
	FHUDPackage HUDPackage;

	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category="Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass; // 角色信息 UI 类
	UPROPERTY(EditAnywhere, Category="Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;
	UPROPERTY(EditAnywhere, Category="Announcements")
	TSubclassOf<UUserWidget> ElimAnnouncementClass;
};
