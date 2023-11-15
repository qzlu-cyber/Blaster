// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter = FVector2D(ViewportSize.X / 2, ViewportSize.Y / 2);

		if (HUDPackage.CrosshiresCenter) DrawCrosshire(HUDPackage.CrosshiresCenter, ViewportCenter);
		if (HUDPackage.CrosshiresLeft) DrawCrosshire(HUDPackage.CrosshiresLeft, ViewportCenter);
		if (HUDPackage.CrosshiresRight) DrawCrosshire(HUDPackage.CrosshiresRight, ViewportCenter);
		if (HUDPackage.CrosshiresTop) DrawCrosshire(HUDPackage.CrosshiresTop, ViewportCenter);
		if (HUDPackage.CrosshiresBottom) DrawCrosshire(HUDPackage.CrosshiresBottom, ViewportCenter);
	}
}

void ABlasterHUD::DrawCrosshire(UTexture2D* Texture, const FVector2D& DrawPoint)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D DrawPosition = FVector2D(
		DrawPoint.X - (TextureWidth / 2.f),
		DrawPoint.Y - (TextureHeight / 2.f)
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
		FLinearColor::White
	);
}
