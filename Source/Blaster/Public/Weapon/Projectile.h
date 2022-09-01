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

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
		);

	virtual void Destroyed() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHitImpact(AActor* OtherActor);
	
	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForPawn;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* HitEffectForStone;
	
	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForPawn;

	UPROPERTY(EditAnywhere)
	class USoundBase* HitSoundForStone;

};
