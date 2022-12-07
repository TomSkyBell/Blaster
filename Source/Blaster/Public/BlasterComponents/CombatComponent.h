// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/CombatState.h"
#include "Components/ActorComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/WeaponType.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/* Change the character movement speed on the server has a multicast effect. */
	void UpdateCharacterSpeed();
	FORCEINLINE float GetBaseWalkSpeed() const { return BaseWalkSpeed; }
	FORCEINLINE void SetBaseWalkSpeed(float Speed) { BaseWalkSpeed = Speed; }
	FORCEINLINE float GetBaseWalkSpeedCrouched() const { return BaseCrouchWalkSpeed; }
	FORCEINLINE void SetBaseWalkSpeedCrouched(float Speed) { BaseCrouchWalkSpeed = Speed; }
	FORCEINLINE float GetAimWalkSpeed() const { return AimWalkSpeed; }
	FORCEINLINE void SetAimWalkSpeed(float Speed) { AimWalkSpeed = Speed; }
	FORCEINLINE float GetAimWalkSpeedCrouched() const { return AimCrouchWalkSpeed; }
	FORCEINLINE void SetAimWalkSpeedCrouched(float Speed) { AimCrouchWalkSpeed = Speed; }
	void EquipWeapon(class AWeapon* WeaponToEquip);
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	void SetCombatState(const ECombatState State);
	FORCEINLINE bool IsCarriedAmmoEmpty() const { return CarriedAmmo <= 0; }
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	void SetCarriedAmmo(int32 Amount);
	void SetHUDCarriedAmmo();
	void HandleCarriedAmmo();
	int32 GetCarriedAmmoFromMap(EWeaponType WeaponType);
	void SetCarriedAmmoFromMap(EWeaponType WeaponType);
	void UpdateCarriedAmmoMap(const TPair<EWeaponType, int32>& CarriedAmmoPair);
	FORCEINLINE bool IsGrenadeEmpty() const { return Grenade <= 0; }
	FORCEINLINE int32 GetGrenadeAmount() const { return Grenade; }
	void SetGrenadeAmount(int32 Amount);
	
	/* Reload Animation Notify, we call it directly in AnimNotifyReload.cpp */
	void ReloadAnimNotify();
	void Reload();

	/* Reload the shotgun AnimNotify. */
	UFUNCTION(BlueprintCallable)
	void ShotgunShellAnimNotify();

	/* Jump to end section of the animation */
	void JumpToShotgunEnd();

	/* Throw the grenade AnimNotify. */
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeAnimNotify();

	/* Launch the grenade AnimNotify. */
	UFUNCTION(BlueprintCallable)
	void LaunchGrenadeAnimNotify();

protected:
	virtual void BeginPlay() override;

	/* Aiming, replicated, animation */
	void SetAiming(bool bIsAiming);

	/* RPC, declared in protected domain, so that its child class could override the rpc function. */
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	/* FVector_NetQuantize is the child of FVector. It is less precise than FVector for a lower network bandwidth. */
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);	

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/* Cross hair algorithm */
	void TraceUnderCrosshairs(FHitResult& HitResult);

private:
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


	/**
	 *	Aiming properties
	 */

	
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

	
	/* 
	 *	Combat State
	 */

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState;

	UFUNCTION()
	void OnRep_CombatState();

	void HandleCombatState();

	
	/** 
	 * Fire
	 */

	
	void Fire();
	void FireButtonPressed(bool bPressed);
	bool CanFire() const;
	void StartFireTimer();
	void FireTimerFinished();
	void SwitchFireModeButtonPressed();

	bool bFireButtonPressed = false;
	bool bAutomaticFire = true;

	FTimerHandle FireTimer;

	/* For the automatic fire, we need to check the fire internal if the fire rate is slow. For example, when we fire a
	 *	cannon, it needs time to cool down so there is a big interval. If we don't check and rapidly pressed the button,
	 *	the timer will not take effect because it's always initialized once the button is pressed. */
	bool bRefireCheck = true;
	

	/**
	 * Cross hair trace
	 */
	
	
	/* Set the cross hairs texture of the HUD from the weapon once the weapon is equipped. Set cross hair functions should be
	 * locally controlled, it shouldn't be rendered on other machines. */
	void UpdateCrosshairSpread(float DeltaTime);
	void UpdateHUDCrosshairs(float DeltaTime);
	void SetHUDPackage();

	/* Interpolate the speed of the change of FOV when aiming. */
	void AimZooming(float DeltaTime);

	/* HitTarget can only be calculated on the local machine, because 'TraceUnderCrosshair' is a machine-related function.
	 * HitTarget can be transmitted as a parameter in the RPCs to let the server know. */
	FVector HitTarget;
	
	float VelocityFactor = 0.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float VelocityFactor_InterpSpeed = 4.f;
	
	float AirFactor = 0.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AirFactor_InterpSpeed = 4.f;

	float AimFactor = 1.f;
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AimFactor_InterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float DefaultZoomOutSpeed = 10.f;
	
	float DefaultFOV;
	float InterpFOV;
	float CrosshairSpread;
	
	FColor CrosshairColor = FColor::White;


	/**
	 *	Reload Ammo
	 */

	
	void InitCarriedAmmoMap();
	void SetHUDWeaponType();
	void ReloadAmmoAmount();

	UFUNCTION(Server, Reliable)
	void ServerReload();
	
	/* Carried Ammo, right part of xxx/xxx, which means the total ammo except for the part in the clip. */
	UPROPERTY(EditAnywhere, Category = Ammo, ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo = 0;

	UFUNCTION()
	void OnRep_CarriedAmmo();
	
	UPROPERTY(EditAnywhere, Category = Ammo)
	TMap<EWeaponType, int32> CarriedAmmoMap;

	
	/* 
	 *	Throw Grenade
	 */

	
	void ThrowGrenade();
	
	/* Attach the weapon to hand when throwing the grenade */
	void AttachWeaponToLeftHand();
	void AttachWeaponToRightHand();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	/* Used to transmit the local variable -- HitTarget */
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	/* Show the grenade when throwing and hide the grenade when launching it. */
	void ShowGrenadeAttached(bool IsVisible);

	/* Projectile class, grenade */
	UPROPERTY(EditAnywhere, Category = TSubclass)
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = Ammo, ReplicatedUsing = OnRep_Grenade)
	int32 Grenade = 4;

	UFUNCTION()
	void OnRep_Grenade();
	void HandleGrenadeRep();

	UPROPERTY(EditAnywhere, Category = Ammo)
	int32 MaxGrenade = 4;
};
