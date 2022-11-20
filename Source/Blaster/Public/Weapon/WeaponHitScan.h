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
	/* Fire without scatter like a pistol */
	void FireHitScan(const FVector& TraceHitTarget);

	/* Fire with scatter for the weapon like shotgun. */
	void FireHitScanScatter(const FVector& TraceHitTarget);

	/* Common logic for FireHitScan with and without scatter. */
	void HitScan(const FVector& Start, const FVector& End);

	/* Particle effect when the weapon hits something. */
	UPROPERTY(EditAnywhere)
	UParticleSystem* HitParticle;

	/* Particle effect for the line trace. */
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticle;

	/* Weapon Damage */
	UPROPERTY(EditAnywhere)
	float Damage = 15.f;

	/* To check the weapon type is a scatter weapon. */
	UPROPERTY(EditAnywhere)
	bool bUseScatter = false;

	/* The distance of shot is limited for scatter weapon like shotgun. */
	UPROPERTY(EditAnywhere)
	uint32 ScatterDist = 10000;

	/* Scatter effect parameter */
	UPROPERTY(EditAnywhere)
	float ScatterAngle = 0.f;

	/* Scatter numbers of line */
	UPROPERTY(EditAnywhere)
	uint32 ScatterNum = 10;
};
