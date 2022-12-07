// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend class ABlasterCharacter;
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Healing */
	void Heal(float Amount, float Duration);

	/* Speeding up */
	void SpeedUp(float Scale, float Duration);

protected:
	virtual void BeginPlay() override;

private:
	/* Initialized in BlasterCharacter.cpp, PostInitializeComponents(). */
	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;


	/**
	 *	Healing
	 */


	UPROPERTY(EditAnywhere, Category = Health)
	float HealingTimerRate = 0.02f;
	
	FTimerHandle HealingTimerHandle;
	float HealthAmount;
	float HealthDuration;
	float TimeRemaining;
	void Healing();


	/**
	 *	Speeding
	 */

	
	FTimerHandle SpeedingTimerHandle;
	float BaseWalkSpeed;
	float BaseWalkSpeedCrouched;
	float AimWalkSpeed;
	float AimWalkSpeedCrouched;
	void ResetSpeed();
};
