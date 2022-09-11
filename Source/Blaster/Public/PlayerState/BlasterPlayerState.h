// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	void UpdateScore();
	virtual void OnRep_Score() override;

private:
	UPROPERTY(EditAnywhere, Category = Score)
	float ScoreAmount = 5.f;

	class ABlasterPlayerController* BlasterPlayerController;
};
