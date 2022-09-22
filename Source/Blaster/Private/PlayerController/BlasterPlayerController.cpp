// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/BlasterPlayerController.h"
#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/AnnouncementWidget.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	
	ServerCheckMatchState();
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	CheckTimeSync(DeltaTime);
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		// OnPossess() is only executed from the server, so we make a replicated variable IsRespawned to make each controller's
		// HUD be refreshed since respawned.
		BlasterCharacter->SetIsRespawned(true);
		RefreshHUD();
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController()) RequestServerTimeFromClient(GetWorld()->GetTimeSeconds());
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterPlayerController, MatchState, COND_OwnerOnly);
}

void ABlasterPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->HealthBar ||
		!BlasterHUD->GetCharacterOverlay()->HealthText) return;
	
	BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(Health / MaxHealth);
	const FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
}

void ABlasterPlayerController::UpdatePlayerScore(float Value)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->Score) return;
	
	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Value));
	BlasterHUD->GetCharacterOverlay()->Score->SetText(FText::FromString(ScoreText));
}

void ABlasterPlayerController::UpdatePlayerDefeats(int32 Value)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->Defeats) return;
	
	const FString DefeatsText = FString::Printf(TEXT("%d"), Value);
	BlasterHUD->GetCharacterOverlay()->Defeats->SetText(FText::FromString(DefeatsText));
	BlasterHUD->GetCharacterOverlay()->DisplayDefeatedMsg(ESlateVisibility::Visible);
}

void ABlasterPlayerController::UpdateWeaponAmmo(int32 AmmoAmount)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount) return;
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	BlasterHUD->GetCharacterOverlay()->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::UpdateCarriedAmmo(int32 AmmoAmount)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount) return;
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::UpdateWeaponType(const FString& WeaponType)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->WeaponType) return;
	
	BlasterHUD->GetCharacterOverlay()->WeaponType->SetText(FText::FromString(WeaponType));
}

void ABlasterPlayerController::UpdateWarmupCountdown(float Countdown)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetAnnouncement() || !BlasterHUD->GetAnnouncement()->WarmupTimeText) return;

	const int32 Minute = FMath::FloorToInt32(Countdown / 60.f);
	const int32 Second = Countdown - 60 * Minute;
	const FString WarmupString = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	BlasterHUD->GetAnnouncement()->WarmupTimeText->SetText(FText::FromString(WarmupString));
}

void ABlasterPlayerController::UpdateMatchCountDown(float Countdown)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->MatchCountdown) return;

	const int32 Minute = FMath::FloorToInt32(Countdown / 60.f);
	const int32 Second = Countdown - 60 * Minute;
	const FString MatchCountdown = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	BlasterHUD->GetCharacterOverlay()->MatchCountdown->SetText(FText::FromString(MatchCountdown));
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	const int32 SecondsLeft = FMath::CeilToInt32(TimeLeft);
	if (SecondsLeft != CountdownInt)
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			UpdateWarmupCountdown(SecondsLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			UpdateMatchCountDown(SecondsLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::RefreshHUD()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay()) return;
	
	BlasterHUD->GetCharacterOverlay()->DisplayDefeatedMsg(ESlateVisibility::Hidden);
	UpdatePlayerHealth(100.f, 100.f);
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	HandleMatchHasStarted();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	HandleMatchHasStarted();
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	if (MatchState == MatchState::InProgress)
	{
		BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
		if (BlasterHUD)
		{
			BlasterHUD->AddCharacterOverlay();
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}
	}	
}

void ABlasterPlayerController::RequestServerTimeFromClient_Implementation(float ClientRequestTime)
{
	ReportServerTimeToClient(ClientRequestTime, GetWorld()->GetTimeSeconds());
}

void ABlasterPlayerController::ReportServerTimeToClient_Implementation(float ClientRequestTime, float ServerReportTime)
{
	const float CurrClientTime = GetWorld()->GetTimeSeconds();
	const float TripRound = CurrClientTime - ClientRequestTime;
	const float CurrServerTime = ServerReportTime + 0.5f * TripRound;
	SyncDiffTime = CurrServerTime - CurrClientTime;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	SyncRunningTime += DeltaTime;
	if (IsLocalController() && SyncRunningTime > SyncFreq)
	{
		RequestServerTimeFromClient(GetWorld()->GetTimeSeconds());
		SyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	const ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(GetWorld()->GetAuthGameMode());
	if (!BlasterGameMode) return;
	
	ClientJoinMidGame(BlasterGameMode->GetLevelStartingTime(), BlasterGameMode->GetWarmupTime(), BlasterGameMode->GetMatchTime(), BlasterGameMode->GetMatchState());
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(float LevelStarting, float Warmup, float Match, FName State)
{
	LevelStartingTime = LevelStarting;
	WarmupTime = Warmup;
	MatchTime = Match;
	MatchState = State;
	
	// If the player is joining mid-game and the game is now in progress, the UI should switch to the MatchState's UI, so the
	// player should be notified which game state is now going on.
	OnMatchStateSet(MatchState);
}
