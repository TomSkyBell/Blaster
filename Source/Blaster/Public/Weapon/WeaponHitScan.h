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
	void FireHitScan(const FVector& TraceHitTarget);

	UPROPERTY(EditAnywhere)
	UParticleSystem* HitParticle;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticle;

	UPROPERTY(EditAnywhere)
	float Damage = 15.f;
};
