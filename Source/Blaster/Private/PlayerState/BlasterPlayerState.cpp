// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

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

void ABlasterPlayerState::UpdateDefeats()
{
	Defeats += 1;

	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(GetOwningController());
	if (!BlasterPlayerController) return;
	
	BlasterPlayerController->UpdatePlayerDefeats(Defeats);
}

void ABlasterPlayerState::OnRep_Defeats()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(GetOwningController());
	if (!BlasterPlayerController) return;
	
	BlasterPlayerController->UpdatePlayerDefeats(Defeats);
}

