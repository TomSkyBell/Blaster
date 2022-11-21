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

	/* Destroyed() has an effect for multicast. The reason why we choose to use Destroyed() is that once Destroy() works
	 * on one player, the actor have been destroyed on all other players' side because the actor is a replicant. And then,
	 * all the functionality before Destroy() lose effect. So we need to move the logic into the destroyed(). */
	virtual void Destroyed() override;

private:
	/* The feature of the grenade, it's not a OnHit() callback, but a OnBounce() callback. */
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
	/* Explode damage */
	void ExplodeDamage();

	/* Bounce sound */
	UPROPERTY(EditAnywhere)
	class USoundCue* BounceSound;
	
};
