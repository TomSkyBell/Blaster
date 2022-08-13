// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/HUDPackage.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	FORCEINLINE void SetHUDPackage(FHUDPackage Package) { HUDPackage = Package; }
	FORCEINLINE void SetHUDSpread(float Spread) { HUDPackage.CrosshairsCurrentSpread = Spread; }

private:
	FHUDPackage HUDPackage;
	FVector2D ViewportCenter;

	void DrawCrosshairs(UTexture2D* Texture, const FVector2D& Spread);
};
