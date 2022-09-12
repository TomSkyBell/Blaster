// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::DisplayDefeatedMsg(ESlateVisibility Param)
{
	if (DefeatedMsg) DefeatedMsg->SetVisibility(Param);
	
	if (Param == ESlateVisibility::Visible)
	{
		PlayAnimation(DefeatedMsgAnim);
	}
	else {}
}
