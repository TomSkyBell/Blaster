// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	
public:
	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Value);
	void UpdatePlayerDefeats(int32 Value);
	void UpdateWeaponAmmo(int32 AmmoAmount);
	void RefreshHUD();

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;
	
};
