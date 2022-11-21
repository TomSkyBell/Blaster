// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
	
public:
	AProjectileRocket();

protected:
	virtual void BeginPlay() override;
	
	/** Apply Radial Damage when on hit */
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

private:
	/** We customize the ProjectileMovementComponent to override the UProjectileMovementComponent engine provides. */
	UPROPERTY(VisibleAnywhere)
	class UBlasterProjectileMovementComponent* BlasterProjectileMovementComponent;
};
