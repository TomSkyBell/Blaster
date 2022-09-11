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
	FORCEINLINE class UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }

protected:
	virtual void BeginPlay() override;
	
private:
	/**
	 *	Access the character's overlay widget.
	 */
	UPROPERTY(EditAnywhere, Category = PlayerStats)
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	void AddCharacterOverlay();
	
	/**
	 *	Draw HUD cross hairs
	 */
	void DrawCrosshairs(UTexture2D* Texture, const FVector2D& Spread);
	
	FHUDPackage HUDPackage;
	FVector2D ViewportCenter;
};
