// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		ViewportCenter = ViewportSize * .5f;
		DrawCrosshairs(HUDPackage.CrosshairsCenter);
		DrawCrosshairs(HUDPackage.CrosshairsLeft);
		DrawCrosshairs(HUDPackage.CrosshairsRight);
		DrawCrosshairs(HUDPackage.CrosshairsTop);
		DrawCrosshairs(HUDPackage.CrosshairsBottom);
	}
}

void ABlasterHUD::DrawCrosshairs(UTexture2D* Texture)
{
	if (!Texture) return;

	// Texture is rendered at the top-left corner of the screen by default, so to put the texture in the center of the screen,
	// we need to calculate the offset away from the top-left corner. offset_x = ViewportCenter.X - Texture->GetSizeX() * .5f,
	// offset_y = ViewportCenter.Y - Texture->GetSizeY() * .5f
	// Remember we don't need to calculate the cross hair's offset away from the center, because the cross hair's position is
	// predefined in the texture.
	DrawTexture(
		Texture,
		ViewportCenter.X - Texture->GetSizeX() * .5f,
		ViewportCenter.Y - Texture->GetSizeY() * .5f,
		Texture->GetSizeX(),
		Texture->GetSizeY(),
		0.f,
		0.f,
		1.f,
		1.f,
		FLinearColor::White
	);
}

