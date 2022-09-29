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

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

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
	
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
		);

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
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	class USoundBase* TracerSound;

	UPROPERTY()
	class UAudioComponent* TracerSoundComponent;
	
	void HandleHitImpact(AActor* OtherActor);
	
	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForPawn;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForStone;
	
	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForPawn;

	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForStone;

};
