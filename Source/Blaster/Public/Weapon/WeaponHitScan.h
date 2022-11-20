// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "WeaponHitScan.generated.h"

/**
 * The difference between hit- scan weapon and projectile weapon is that it does't spawn the projectile because its speed is very fast
 * so once we fire, the target is damaged immediately.
 */
UCLASS()
class BLASTER_API AWeaponHitScan : public AWeapon
{
	GENERATED_BODY()

protected:
	virtual void Fire(const FVector& TraceHitTarget) override;

private:
	/* Fire, can be with or without scatter effect. */
	void FireHitScan(const FVector& TraceHitTarget);

	/* Common logic for FireHitScan with and without scatter. */
	void HitScan(TMap<AActor*, float>& DamageForEachActor, const FVector& Start, const FVector& End);

	/* Particle effect when the weapon hits something. */
	UPROPERTY(EditAnywhere)
	UParticleSystem* HitParticle;

	/* Particle effect for the line trace. */
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticle;

	/* Weapon Damage, normally it should be in projectile class, but here the hit scan weapon supposes it doesn't need projectiles. */
	UPROPERTY(EditAnywhere)
	float Damage = 15.f;

	/* Scatter effect parameter, if a weapon is not a scatter weapon, then its value is 0. */
	UPROPERTY(EditAnywhere)
	float ScatterAngle = 0.f;

	/* Scatter numbers, if a weapon is not a scatter weapon, then its value is 1. */
	UPROPERTY(EditAnywhere)
	uint32 ScatterNum = 1;
};
