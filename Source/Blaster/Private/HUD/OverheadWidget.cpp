// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();
}

bool UOverheadWidget::Initialize()
{
	if(!Super::Initialize()) return false;

	FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UOverheadWidget::OnLevelRemovedFromWorld);

	return true;
}

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (InPawn->Controller == nullptr) return;
	const ENetRole RemoteRole = InPawn->Controller->GetRemoteRole();
	FString Role;
	switch(RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	const FString RemoteRoleString = FString::Printf(TEXT("Remote Role is: %s"), *Role);
	SetDisplayText(RemoteRoleString);
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
	const APlayerState* PlayerState = InPawn->GetPlayerState<APlayerState>();
	if (PlayerState)
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		const FString PlayerNameString = FString::Printf(TEXT("Player Name is: %s"), *PlayerName);
		SetDisplayText(PlayerNameString);
	}
}

