// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "ElimAnnouncement.h"

#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController; // 获取玩家控制器
	if (OwningPlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(OwningPlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
	if (OwningPlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(OwningPlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::AddElimAnnouncement(const FString& Attacker, const FString& Victim)
{
	OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
	if (OwningPlayerController && ElimAnnouncementClass)
	{
		UElimAnnouncement* ElimAnnouncement = CreateWidget<UElimAnnouncement>(OwningPlayerController, ElimAnnouncementClass);
		ElimAnnouncement->SetElimAnnouncementText(Attacker, Victim);
		ElimAnnouncement->AddToViewport();

		// 将较新的 ElimAnnouncement 移动到上方
		for (auto Msg : ElimAnnouncements)
		{
			if (Msg && Msg->GetElimAnnouncementBox())
			{
				UCanvasPanelSlot* CanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->GetElimAnnouncementBox());
				if (CanvasPanelSlot)
				{
					const FVector2D Position = CanvasPanelSlot->GetPosition();
					const FVector2D NewPosition = FVector2D(Position.X, Position.Y - CanvasPanelSlot->GetSize().Y);
					CanvasPanelSlot->SetPosition(NewPosition);
				}
			}
		}

		ElimAnnouncements.Emplace(ElimAnnouncement);

		// 设置定时器，定时删除 ElimAnnouncement
		FTimerHandle ElimAnnouncementTimerHandle;
		FTimerDelegate ElimAnnouncementTimerDelegate;
		ElimAnnouncementTimerDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncement);
		GetWorldTimerManager().SetTimer(ElimAnnouncementTimerHandle, ElimAnnouncementTimerDelegate, ElimAnnouncementTime, false);
	}
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UUserWidget* WidgetToRemove)
{
	if (WidgetToRemove) WidgetToRemove->RemoveFromParent();
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter = FVector2D(ViewportSize.X / 2, ViewportSize.Y / 2);

		const float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairsCenter)
		{
			const FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsLeft)
		{
			const FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsRight)
		{
			const FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsTop)
		{
			const FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsBottom)
		{
			const FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D& DrawPoint, const FVector2D& Spread)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D DrawPosition = FVector2D(
		DrawPoint.X - (TextureWidth / 2.f) + Spread.X,
		DrawPoint.Y - (TextureHeight / 2.f + Spread.Y)
	);

	DrawTexture(
		Texture,
		DrawPosition.X,
		DrawPosition.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		HUDPackage.CrosshairColor
	);
}
