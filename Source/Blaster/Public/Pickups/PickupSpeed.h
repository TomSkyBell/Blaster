// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "PickupSpeed.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupSpeed : public APickup
{
	GENERATED_BODY()
	
public:
	APickupSpeed();
	
protected:
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	/* Buff scale factor */
	UPROPERTY(EditAnywhere, Category = Buff)
	float SpeedBuff = 2.f;

	UPROPERTY(EditAnywhere, Category = Buff)
	float Duration = 5.f;
};
