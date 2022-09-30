// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileBullet();

protected:
	/** Apply Damage when on hit */
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

private:
	/** We customize the ProjectileMovementComponent */
	UPROPERTY(VisibleAnywhere)
	class UBlasterProjectileMovementComponent* BulletMovementComponent;
};
