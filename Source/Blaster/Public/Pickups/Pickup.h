// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/* Set the scale and the extent in child class. */
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* PickupCollision;

private:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneComponent;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, Category = Effect)
	class UNiagaraComponent* NiagaraComponent;

	UPROPERTY(EditAnywhere, Category = Effect)
	class UNiagaraSystem* NiagaraEffect;

	UPROPERTY(EditAnywhere, Category = Effect)
	class USoundCue* SoundPickup;

	UPROPERTY(EditAnywhere, Category = Effect)
	float BaseTurnRate = 45.f;

	FTimerHandle TurnTimerHandle;

	UPROPERTY(EditAnywhere, Category = Effect)
	float TurnTimerRate = 0.02f;
	
	void Turn();

	/* If we put SpawnBuffEffectAttached() in Destroyed() to replicate. The client side won't know the character it attaches to.
	 * Even we declare a member variable and set it in OnSphereBeginOverlap() because The client side's pickup's member variable is not set.
	 * Even when we set owner and do it in OnRep_Owner() because the pickup has been destroyed() and its replication doesn't work. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBuffEffectAttached(class ABlasterCharacter* AttachedCharacter) const;
};
