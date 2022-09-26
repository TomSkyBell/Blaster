// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/BlasterPlayerController.h"
#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "GameMode/BlasterGameMode.h"
#include "GameState/BlasterGameState.h"
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
}

void ABlasterPlayerController::DisplayDefeatedMsg()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->DefeatedMsg ||
		!BlasterHUD->GetCharacterOverlay()->DefeatedMsgAnim) return;

	UCharacterOverlay* CharacterOverlay = BlasterHUD->GetCharacterOverlay();
	CharacterOverlay->DefeatedMsg->SetVisibility(ESlateVisibility::Visible);
	CharacterOverlay->PlayAnimation(CharacterOverlay->DefeatedMsgAnim);
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

	UAnnouncementWidget* Announcement = BlasterHUD->GetAnnouncement();
	if (Countdown <= 0)
	{
		Announcement->Announce_0->SetText(FText());
		Announcement->Announce_1->SetText(FText());
		Announcement->TimeText->SetText(FText());
		return;
	}
	const int32 Minute = Countdown / 60.f;
	const int32 Second = Countdown - 60 * Minute;
	const FString CountdownString = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	Announcement->TimeText->SetText(FText::FromString(CountdownString));
}

void ABlasterPlayerController::UpdateMatchCountDown(int32 Countdown)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->MatchCountdown) return;

	UCharacterOverlay* CharacterOverlay = BlasterHUD->GetCharacterOverlay();
	if (Countdown > 0 && Countdown <= 30)
	{
		// Urgent countdown effect, turns red and play blink animation. (animation no need to loop, because update is loop every 1 second)
		CharacterOverlay->MatchCountdown->SetColorAndOpacity((FLinearColor(1.f, 0.f, 0.f)));
		CharacterOverlay->PlayAnimation(CharacterOverlay->TimeBlink);
	}
	else if (Countdown <= 0)
	{
		CharacterOverlay->MatchCountdown->SetText(FText());
		CharacterOverlay->MatchCountdown->SetColorAndOpacity((FLinearColor(1.f, 1.f, 1.f)));
		CharacterOverlay->StopAnimation(CharacterOverlay->TimeBlink);
		return;
	}
	const int32 Minute = Countdown / 60.f;
	const int32 Second = Countdown - 60 * Minute;
	const FString MatchCountdown = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	CharacterOverlay->MatchCountdown->SetText(FText::FromString(MatchCountdown));
}

void ABlasterPlayerController::UpdateTopScorePlayer()
{
	const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!BlasterGameState) return;

	auto PlayerStates = BlasterGameState->GetTopScorePlayerStates();
	if (PlayerStates.IsEmpty()) return;

	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->TopScorePlayer) return;
	
	FString PlayerName;
	for (const auto& State: PlayerStates)
	{
		if (!State) return;
		PlayerName.Append(FString::Printf(TEXT("%s\n"), *State->GetPlayerName()));
	}
	BlasterHUD->GetCharacterOverlay()->TopScorePlayer->SetText(FText::FromString(PlayerName));
}

void ABlasterPlayerController::UpdateTopScore()
{
	const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!BlasterGameState) return;

	const auto PlayerStates = BlasterGameState->GetTopScorePlayerStates();
	const float TopScore = BlasterGameState->GetTopScore();
	if (PlayerStates.IsEmpty()) return;
	
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->TopScore) return;
	
	FString TopScoreString;
	for (int32 i = 0; i < PlayerStates.Num(); ++i)
	{
		TopScoreString.Append(FString::Printf(TEXT("%d\n"), FMath::CeilToInt32(TopScore)));
	}
	BlasterHUD->GetCharacterOverlay()->TopScore->SetText(FText::FromString(TopScoreString));
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
	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetCharacterOverlay()->DefeatedMsg) return;

	UCharacterOverlay* CharacterOverlay = BlasterHUD->GetCharacterOverlay();
	CharacterOverlay->DefeatedMsg->SetVisibility(ESlateVisibility::Hidden);
	if (CharacterOverlay->IsAnimationPlaying(CharacterOverlay->DefeatedMsgAnim))
	{
		CharacterOverlay->StopAnimation(CharacterOverlay->DefeatedMsgAnim);
	}
	UpdatePlayerHealth(100.f, 100.f);
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	HandleMatchState();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	HandleMatchState();
}

void ABlasterPlayerController::HandleMatchState()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (!BlasterHUD) return;
	
	if (MatchState == MatchState::InProgress)
	{
		if (!BlasterHUD->GetCharacterOverlay()) BlasterHUD->AddCharacterOverlay();

		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		if (!BlasterHUD->GetCharacterOverlay() || !BlasterHUD->GetAnnouncement() ||
			!BlasterHUD->GetAnnouncement()->Announce_0 || !BlasterHUD->GetAnnouncement()->Announce_1 ||
			!BlasterHUD->GetAnnouncement()->WinText) return;
		
		BlasterHUD->GetCharacterOverlay()->RemoveFromViewport();
		BlasterHUD->GetAnnouncement()->Announce_0->SetText(FText::FromString("New Match Starts in:"));
		BlasterHUD->GetAnnouncement()->Announce_1->SetText(FText::FromString(""));
		BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);

		// When the match ends, we show the winner announcement.
		const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
		const ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (!BlasterGameState || !BlasterPlayerState) return;

		FString WinString;
		auto PlayerStates = BlasterGameState->GetTopScorePlayerStates();
		if (PlayerStates.Num() == 0)
		{
			WinString = "There is no winner.";
		}
		else if (PlayerStates.Num() == 1 && PlayerStates[0] == BlasterPlayerState)
		{
			WinString = "You are the winner!";
		}
		else if (PlayerStates.Num() == 1)
		{
			WinString = "Winner:\n";
			WinString.Append(FString::Printf(TEXT("%s\n"), *PlayerStates[0]->GetPlayerName()));
		}
		else if (PlayerStates.Num() > 1)
		{
			WinString = "Players tied for the win:\n";
			for (const auto& State: PlayerStates)
			{
				WinString.Append(FString::Printf(TEXT("%s\n"), *State->GetPlayerName()));
			}
		}
		BlasterHUD->GetAnnouncement()->WinText->SetText(FText::FromString(WinString));
		BlasterHUD->GetAnnouncement()->WinText->SetVisibility(ESlateVisibility::Visible);
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
