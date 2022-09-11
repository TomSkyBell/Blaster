// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::UpdateScore()
{
	SetScore(GetScore() + ScoreAmount);

	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(GetOwningController());
	if (!BlasterPlayerController) return;
	
	BlasterPlayerController->UpdatePlayerScore(GetScore());
}

void ABlasterPlayerState::OnRep_Score()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(GetOwningController());
	if (!BlasterPlayerController) return;
	
	BlasterPlayerController->UpdatePlayerScore(GetScore());
}
