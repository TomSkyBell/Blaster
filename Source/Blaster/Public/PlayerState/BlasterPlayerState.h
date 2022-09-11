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
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
	void UpdateScore();
	virtual void OnRep_Score() override;

	void UpdateDefeats();
	
	UFUNCTION()
	virtual void OnRep_Defeats();

private:
	UPROPERTY(EditAnywhere)
	float ScoreAmount = 5.f;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats = 0;

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
};
