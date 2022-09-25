// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/BlasterGameState.h"

#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"
#include "PlayerState/BlasterPlayerState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABlasterGameState, TopScorePlayerStates);
	DOREPLIFETIME(ABlasterGameState, TopScore);
}

void ABlasterGameState::UpdateTopScorePlayerStates(ABlasterPlayerState* PlayerState)
{
	if (!PlayerState) return;
	
	if (TopScorePlayerStates.Num() == 0)
	{
		TopScorePlayerStates.AddUnique(PlayerState);
		TopScore = PlayerState->GetScore();
		HandleTopScore();
		HandleTopScorePlayerStates();
	}
	else if (TopScore == PlayerState->GetScore())
	{
		TopScorePlayerStates.AddUnique(PlayerState);
		HandleTopScore();
		HandleTopScorePlayerStates();
	}
	else if (TopScore < PlayerState->GetScore())
	{
		TopScorePlayerStates.Empty();
		TopScorePlayerStates.AddUnique(PlayerState);
		TopScore = PlayerState->GetScore();
		HandleTopScore();
		HandleTopScorePlayerStates();
	}
}

void ABlasterGameState::OnRep_TopScore()
{
	HandleTopScore();	
}

void ABlasterGameState::HandleTopScore()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!BlasterPlayerController) return;
	
	// Updating the TopScore in the HUD
	BlasterPlayerController->UpdateTopScore();
}

void ABlasterGameState::OnRep_TopScorePlayerStates()
{
	HandleTopScorePlayerStates();
}

void ABlasterGameState::HandleTopScorePlayerStates()
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!BlasterPlayerController) return;
	
	// Updating the TopScorePlayer in the HUD
	BlasterPlayerController->UpdateTopScorePlayer();
}
