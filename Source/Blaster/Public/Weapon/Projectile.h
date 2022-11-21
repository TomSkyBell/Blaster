// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** We customize the ProjectileMovementComponent to override the UProjectileMovementComponent engine provides.
	 * We don't initialize it in parent class but let the child class use this feature. */
	UPROPERTY(VisibleAnywhere)
	class UBlasterProjectileMovementComponent* BlasterProjectileMovementComponent;

	/** Instead of using particle system -- 'Tracer' in the parent class, we use Niagara system here to combine two emitters together
	 * RocketFireFlash emitter and TrailSmoke emitter. This feature is for child class use. */
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	/* This component is not in the BP panel and we initialize it when BeginPlay(). This feature is for child class use. */
	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	/* Spawn a niagara system for the projectile. */
	void SpawnTrailSystem();

	/** If we hope destroy the projectile immediately once it's hit */
	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bOnHitDestroy = true;

	/** Timer delay for the destruction */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float DestroyDelay = 3.f;

	/** Timer for the destruction */
	FTimerHandle DestroyTimerHandle;

	/** Delegate function for the destroy timer */
	virtual void DestroyTimerFinished();

	/* OnHit callback function */
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
		);

	/* Once the actor is destroyed, this event happens. */
	virtual void Destroyed() override;
	
	/** Projectile Damage of the weapon */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float Damage = 10.f;

	/** Projectile Minimum Damage for some weapon with a damage falloff effect */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float MinDamage = 0.f;

	/** Radius of the full damage area from the origin */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float DamageInnerRadius = 50.f;

	/** Radius of the minimum damage area from the origin */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float DamageOuterRadius = 100.f;

	/** Falloff exponent of damage from full damage to minimum damage */
	UPROPERTY(EditAnywhere, Category = Weapon)
	float DamageFalloff = 1.f;

private:
	/* Collision box */
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	/* Projectile mesh */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	/* Tracer effect of the projectile */
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	/* Tracer component */
	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	/* Tracer sound */
	UPROPERTY(EditAnywhere)
	class USoundBase* TracerSound;

	/* Tracer sound component */
	UPROPERTY()
	class UAudioComponent* TracerSoundComponent;

	/* Play sound and particle effect. */
	void HandleHitImpact(AActor* OtherActor);

	/* Particle effect when pawn is hit. */
	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForPawn;

	/* Particle effect when stone is hit. */
	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForStone;

	/* Sound effect when pawn is hit. */
	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForPawn;

	/* Sound effect when stone is hit. */
	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForStone;

};
