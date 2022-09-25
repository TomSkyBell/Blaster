// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Once a player is eliminated, we then need to update the array: TopScorePlayerStates and update the TopScore player in the HUD */
	void UpdateTopScorePlayerStates(class ABlasterPlayerState* PlayerState);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Normally, the top score of this game, no need to be replicated because TArray<APlayerStates> contains the info
	 * But the reason why I still make it replicated is that The PlayerState's score is also a replicated variable, which
	 * will cause replication conflict with the PlayerStates Array, so that on the client, the TopScore is not updated */
	UPROPERTY(ReplicatedUsing = OnRep_TopScore)
	float TopScore = 0.f;

	UFUNCTION()
	void OnRep_TopScore();

	/** The common code within OnRep_TopScore() */
	void HandleTopScore();

	/** An array contains the top score players' states */
	UPROPERTY(ReplicatedUsing = OnRep_TopScorePlayerStates)
	TArray<class ABlasterPlayerState*> TopScorePlayerStates;

	UFUNCTION()
	void OnRep_TopScorePlayerStates();

	/** The common code within OnRep_TopScorePlayerStates() */
	void HandleTopScorePlayerStates();

public:
	FORCEINLINE float GetTopScore() const { return TopScore; }
	 FORCEINLINE const TArray<class ABlasterPlayerState*>& GetTopScorePlayerStates() const { return TopScorePlayerStates; }
};
