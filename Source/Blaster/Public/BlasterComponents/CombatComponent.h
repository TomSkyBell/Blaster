// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/CombatState.h"
#include "Components/ActorComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/WeaponType.h"
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
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE void SetCombatState(const ECombatState State) { CombatState = State; }

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
	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere, Category = Movement)
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = Movement)
	float AimWalkSpeed = 150.f;
	
	UPROPERTY(EditAnywhere, Category = Movement)
	float BaseCrouchWalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = Movement)
	float AimCrouchWalkSpeed = 150.f;

	void Fire();
	void FireButtonPressed(bool bPressed);
	bool bFireButtonPressed;

	/**
	 *	Set a timer for the automatic fire.
	 */
	FTimerHandle FireTimer;

	/**
	 *	bAutomaticFire is not need to be known by other clients and the server. It's just a signal for the local machine,
	 *	and the local machine will decide to use this signal to multicast fire with a timer or not. So it has not to be
	 *	transmitted by RPCs and we should set it locally.
	 */
	bool bAutomaticFire = true;

	/**
	 *	For the automatic fire, we need to check the fire internal if the fire rate is slow. For example, when we fire a
	 *	cannon, it needs time to cool down so there is a big interval. If we don't check and rapidly pressed the button,
	 *	the timer will not take effect because it's always initialized once the button is pressed.
	 */
	bool bRefireCheck = true;
	bool CanFire() const;
	void StartFireTimer();
	void FireTimerFinished();
	void SwitchFireModeButtonPressed();

	/**
	 *	HitTarget can only be calculated on the local machine, because 'TraceUnderCrosshair' is a machine-related function.
	 *	HitTarget can be transmitted as a parameter in the RPCs to let the server know.
	 */
	FVector HitTarget;
	
	/**
	* Set the cross hairs texture of the HUD from the weapon once the weapon is equipped.
	* Set cross hair functions should be locally controlled, it shouldn't be rendered on other machines.
	*/
	void UpdateCrosshairSpread(float DeltaTime);
	void UpdateHUDCrosshairs(float DeltaTime);
	void SetHUDPackage();
	void SetWeaponRelatedProperties();
	
	float VelocityFactor = 0.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float VelocityFactor_InterpSpeed = 4.f;
	
	float AirFactor = 0.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AirFactor_InterpSpeed = 4.f;

	float AimFactor = 1.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AimFactor_InterpSpeed = 10.f;

	/**
	 *	Interpolate the speed of the change of FOV when aiming.
	 */
	void AimZooming(float DeltaTime);
	float DefaultFOV;
	float InterpFOV;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float DefaultZoomOutSpeed = 10.f;

	/**
	 *	Change the cross hair's spread when jumping, moving, aiming.
	 */
	float CrosshairSpread;
	
	/**
	 *	Change the cross hair's color when aiming at the enemy.
	 */
	FColor CrosshairColor = FColor::White;

	/**
	 * Carried Ammo
	 */
	UPROPERTY(EditAnywhere, Category = Ammo, ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo = 0;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(EditAnywhere, Category = Ammo)
	TMap<EWeaponType, int32> CarriedAmmoMap;

	/**
	 *	Reload
	 */
	UFUNCTION(Server, Reliable)
	void ServerReloadButtonPressed();
	
	void ReloadButtonPressed();

	/**
	 *	Combat State
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState;

	UFUNCTION()
	void OnRep_CombatState();
};
