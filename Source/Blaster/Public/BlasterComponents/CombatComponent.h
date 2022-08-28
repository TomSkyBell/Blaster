// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	// RPC, declared in protected domain, so that its child class could override the rpc function
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	// FVector_NetQuantize is the child of FVector. It is less precise than FVector for a lower network bandwidth.
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);	

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	
	void TraceUnderCrosshairs(FHitResult& HitResult);

private:
	/**
	 * Cross hair HUD and the player controller, player character
	 */
	class ABlasterCharacter* BlasterCharacter;
	class ABlasterPlayerController* BlasterPlayerController;
	class ABlasterHUD* BlasterHUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;
	
	UPROPERTY(EditAnywhere)
	float BaseCrouchWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimCrouchWalkSpeed;

	void FireButtonPressed(bool bPressed);
	bool bFireButtonPressed;
	
	/**
	* Set the cross hairs texture of the HUD from the weapon once the weapon is equipped.
	* Set cross hair functions should be locally controlled, it shouldn't be rendered on other machines.
	*/
	void SetCrosshairSpread(float DeltaTime);
	void SetHUDCrosshairs();
	float VelocityFactor;
	
	UPROPERTY(EditAnywhere)
	float VelocityFactor_InterpSpeed;
	
	float AirFactor;
	
	UPROPERTY(EditAnywhere)
	float AirFactor_InterpSpeed;
};
