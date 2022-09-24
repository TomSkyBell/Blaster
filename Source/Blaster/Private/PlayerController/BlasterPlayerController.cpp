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

void ABlasterPlayerController::UpdateAnnouncement(int32 Countdown)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetAnnouncement() || !BlasterHUD->GetAnnouncement()->Announce_0 ||
		!BlasterHUD->GetAnnouncement()->Announce_1 || !BlasterHUD->GetAnnouncement()->TimeText) return;

	if (Countdown <= 0)
	{
		BlasterHUD->GetAnnouncement()->Announce_0->SetText(FText());
		BlasterHUD->GetAnnouncement()->Announce_1->SetText(FText());
		BlasterHUD->GetAnnouncement()->TimeText->SetText(FText());
		return;
	}
	const int32 Minute = Countdown / 60.f;
	const int32 Second = Countdown - 60 * Minute;
	const FString CountdownString = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	BlasterHUD->GetAnnouncement()->TimeText->SetText(FText::FromString(CountdownString));
}

void ABlasterPlayerController::UpdateMatchCountDown(int32 Countdown)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->MatchCountdown) return;

	if (Countdown <= 0)
	{
		BlasterHUD->GetCharacterOverlay()->MatchCountdown->SetText(FText());
		return;
	}
	const int32 Minute = Countdown / 60.f;
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
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	const int32 SecondsLeft = FMath::CeilToInt32(TimeLeft);
	if (SecondsLeft != CountdownInt)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			UpdateAnnouncement(SecondsLeft);
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
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD) return;
	
	if (MatchState == MatchState::InProgress)
	{
		if (!BlasterHUD->GetAnnouncement()) return;
		
		BlasterHUD->AddCharacterOverlay();
		BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		if (!BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetAnnouncement() ||
			!BlasterHUD->GetAnnouncement()->Announce_0 || !BlasterHUD->GetAnnouncement()->Announce_1) return;
		
		BlasterHUD->GetCharacterOverlay()->RemoveFromViewport();
		BlasterHUD->GetAnnouncement()->Announce_0->SetText(FText::FromString("Game Ready, Starts in:"));
		BlasterHUD->GetAnnouncement()->Announce_1->SetText(FText::FromString(""));
		BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);
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
	
	ClientJoinMidGame(BlasterGameMode->GetLevelStartingTime(), BlasterGameMode->GetWarmupTime(), BlasterGameMode->GetMatchTime(),
		BlasterGameMode->GetCooldownTime(), BlasterGameMode->GetMatchState());
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(float LevelStarting, float Warmup, float Match, float Cooldown, FName State)
{
	LevelStartingTime = LevelStarting;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	MatchState = State;
	
	// If the player is joining mid-game and the game is now in progress, the UI should switch to the MatchState's UI, so the
	// player should be notified which game state is now going on.
	OnMatchStateSet(MatchState);
}
