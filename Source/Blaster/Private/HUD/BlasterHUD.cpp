// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"
#include "Components/TextBlock.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/AnnouncementWidget.h"
#include "Character/BlasterCharacter.h"
#include "BlasterComponents/CombatComponent.h"
#include "PlayerController/BlasterPlayerController.h"

// The DrawHUD function will be automatically called when we set the default HUD as BP_BlasterHUD in BP_GameMode settings.
void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		ViewportCenter = ViewportSize * .5f;
		DrawCrosshairs(HUDPackage.CrosshairsCenter, FVector2D(0.f, 0.f));
		DrawCrosshairs(HUDPackage.CrosshairsLeft, FVector2D(-HUDPackage.CrosshairsCurrentSpread, 0.f));
		DrawCrosshairs(HUDPackage.CrosshairsRight, FVector2D(HUDPackage.CrosshairsCurrentSpread, 0.f));
		DrawCrosshairs(HUDPackage.CrosshairsTop, FVector2D(0.f, -HUDPackage.CrosshairsCurrentSpread));
		DrawCrosshairs(HUDPackage.CrosshairsBottom, FVector2D(0.f, HUDPackage.CrosshairsCurrentSpread));
	}
}

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

	AddAnnouncement();
}

void ABlasterHUD::AddCharacterOverlay()
{
	// APlayerController* PlayerController = GetOwningPlayerController();
	if (CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(GetWorld(), CharacterOverlayClass, FName("Character Overlay"));
		if (!CharacterOverlay) return;

		Refresh();
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	// APlayerController* PlayerController = GetOwningPlayerController();
	if (AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncementWidget>(GetWorld(), AnnouncementClass, FName("Announcement"));
		if (!Announcement) return;
		
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::Refresh()
{
	if (CharacterOverlay && CharacterOverlay->DefeatedMsg)
	{
		CharacterOverlay->DefeatedMsg->SetVisibility(ESlateVisibility::Hidden);
		if (CharacterOverlay->IsAnimationPlaying(CharacterOverlay->DefeatedMsgAnim))
		{
			CharacterOverlay->StopAnimation(CharacterOverlay->DefeatedMsgAnim);
		}
	}
	// We need player controller and player character because the data is stored there.
	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetOwningPlayerController()))
	{
		BlasterPlayerController->UpdatePlayerHealth(100.f, 100.f);
		const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(BlasterPlayerController->GetCharacter());
		if (BlasterCharacter && BlasterCharacter->GetCombat())
		{
			BlasterPlayerController->UpdateGrenade(BlasterCharacter->GetCombat()->GetGrenadeAmount());
		}
	}
}

void ABlasterHUD::DrawCrosshairs(UTexture2D* Texture, const FVector2D& Spread)
{
	if (!Texture) return;
	
	DrawTexture(
		Texture,
		ViewportCenter.X - Texture->GetSizeX() * .5f + Spread.X,
		ViewportCenter.Y - Texture->GetSizeY() * .5f + Spread.Y,
		Texture->GetSizeX(),
		Texture->GetSizeY(),
		0.f,
		0.f,
		1.f,
		1.f,
		HUDPackage.CrosshairsColor
	);
}

