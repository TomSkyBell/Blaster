// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "Weapon/WeaponType.h"
#include "PickupAmmo.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupAmmo : public APickup
{
	GENERATED_BODY()

public:
	APickupAmmo();
	
protected:
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	) override;

private:
	UPROPERTY(EditAnywhere, Category = Ammo)
	int32 PickupAmmo = 30;

	UPROPERTY(EditAnywhere, Category = Ammo)
	EWeaponType AmmoType;
};
