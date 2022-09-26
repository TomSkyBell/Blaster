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
	// Class Reference. ProjectileClass can be populated with AProjectile or anything derived from AProjectile.
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	void FireProjectile(const FVector& TraceHitTarget);
	void EjectProjectileShell();
};
