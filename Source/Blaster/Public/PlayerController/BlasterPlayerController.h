// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Value);
	void UpdatePlayerDefeats(int32 Value);
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;
	
};
