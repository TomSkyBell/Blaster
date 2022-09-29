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
	/** We customize the ProjectileMovementComponent */
	UPROPERTY(VisibleAnywhere)
	class UBlasterProjectileMovementComponent* RocketMovementComponent;
	
	/** Instead of using particle system -- 'Tracer' in the parent class, we use Niagara system here to combine two emitters together
	 * RocketFireFlash emitter and TrailSmoke emitter */
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;
};
