// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"

#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		//   OnPossess is not implemented on each owning client and server though it's indeed visually destroyed on each machine,
		// but logically it's destroyed and respawned on the server.
		//   Besides, OnPossess is not a multicast, so once the pawn is destroyed and respawned on the server, the destroyed and
		// respawned can take effect on the clients as well, but as for the OnPossess, it could only be done on the server.
		//   Something to note is that OnPossess() is executed after the constructor, so if we set IsRespawned as false in default
		// once the pawn is constructed, then OnPossess will make it true and the RepNotify will work. In this way, we don't need
		// to recover the replicated value to be false.
		BlasterCharacter->SetIsRespawned(true);
		RefreshHUD();
	}
}

void ABlasterPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	// First check the BlasterHUD
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->HealthBar ||
		!BlasterHUD->GetCharacterOverlay()->HealthText) return;
	
	BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(Health / MaxHealth);
	const FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
}

void ABlasterPlayerController::UpdatePlayerScore(float Value)
{
	// First check the BlasterHUD
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->Score) return;
	
	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Value));
	BlasterHUD->GetCharacterOverlay()->Score->SetText(FText::FromString(ScoreText));
}

void ABlasterPlayerController::UpdatePlayerDefeats(int32 Value)
{
	// First check the BlasterHUD
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->Defeats) return;
	
	const FString DefeatsText = FString::Printf(TEXT("%d"), Value);
	BlasterHUD->GetCharacterOverlay()->Defeats->SetText(FText::FromString(DefeatsText));
	BlasterHUD->GetCharacterOverlay()->DisplayDefeatedMsg(ESlateVisibility::Visible);
}

void ABlasterPlayerController::UpdateWeaponAmmo(int32 AmmoAmount)
{
	// First check the BlasterHUD
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->AmmoAmount) return;
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	BlasterHUD->GetCharacterOverlay()->AmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::RefreshHUD()
{
	// First check the BlasterHUD
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay()) return;
	
	BlasterHUD->GetCharacterOverlay()->DisplayDefeatedMsg(ESlateVisibility::Hidden);
	UpdatePlayerHealth(100.f, 100.f);
}

