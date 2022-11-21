// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * Grenade has a feature of bouncing and destroyed after seconds.
 */
UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();

protected:
	virtual void BeginPlay() override;

	/* When the timer finishes, we spawn the explode particle effect and destroy the grenade. */
	virtual void DestroyTimerFinished() override;

private:
	/* The feature of the grenade, it's not a OnHit() callback, but a OnBounce() callback. */
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	/* Bounce sound */
	UPROPERTY(EditAnywhere)
	class USoundCue* BounceSound;

	/* Explode particle effect */
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ExplodeEffect;

	/* Explode sound */
	UPROPERTY(EditAnywhere)
	class USoundCue* ExplodeSound;
};
