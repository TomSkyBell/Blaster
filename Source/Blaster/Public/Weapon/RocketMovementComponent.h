// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API URocketMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

protected:
	/** Handle the next step projectile will take when it hits something */
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override;

	/** Handle the projectile bounce event */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;
	
};
