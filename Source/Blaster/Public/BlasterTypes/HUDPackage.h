#pragma once

#include "HUDPackage.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	FHUDPackage(UTexture2D* Center = nullptr, UTexture2D* Left = nullptr,UTexture2D* Right = nullptr ,UTexture2D* Top = nullptr ,UTexture2D* Bottom = nullptr, float Spread = 0.f, FColor Color = FColor::White):
		CrosshairsCenter(Center), CrosshairsLeft(Left), CrosshairsRight(Right), CrosshairsTop(Top), CrosshairsBottom(Bottom), CrosshairsCurrentSpread(Spread), CrosshairsColor(Color) {}
	
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairsCurrentSpread;
	FColor CrosshairsColor;
};
