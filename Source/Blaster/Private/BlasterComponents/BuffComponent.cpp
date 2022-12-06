// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::Heal(float Amount, float Duration)
{
	TimeRemaining = Duration;
	HealthAmount = Amount;
	HealthDuration = Duration;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HealingTimerHandle, this, &UBuffComponent::Healing, HealingTimerRate, true);
	}
}

void UBuffComponent::Healing()
{
	if (!BlasterCharacter) return;
	
	// Time's up, stop healing. Or if character's health is full, then we stop healing.
	TimeRemaining -= HealingTimerRate;
	if (TimeRemaining < 0.f || BlasterCharacter->GetHealth() >= BlasterCharacter->GetMaxHealth())
	{
		if (const UWorld* World = GetWorld())
		{
			// Clear the timer and immediately return, or it will continue to executed the rest part of the code.
			World->GetTimerManager().ClearTimer(HealingTimerHandle);
			return;
		}
	}
	// Calculate the amount of healing per timer called.
	const float HealingAmountPerTick = HealthAmount * HealingTimerRate / HealthDuration;
	BlasterCharacter->SetHealth(BlasterCharacter->GetHealth() + HealingAmountPerTick);
}

