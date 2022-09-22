// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterPlayerController* VictimController, class ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(class ABlasterCharacter* EliminatedCharacter, class AController* EliminatedController);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMatchStateSet() override;
	
private:
	/** Warmup time*/
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	/** Countdown time since the players have entered the map*/
	float CountdownTime = 10.f;

	/** The time cost for entering the map */
	float LevelStartingTime = 0.f;
};
