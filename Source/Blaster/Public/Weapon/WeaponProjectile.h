// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "WeaponProjectile.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AWeaponProjectile : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& TraceHitTarget) override;
	
private:
	void FireProjectile(const FVector& TraceHitTarget);
	void EjectProjectileShell();
};
