// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponType.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),

	EWS_Max UMETA(DisplayName = "DefaultMax"),
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& TraceHitTarget);
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	void SetWeaponState(EWeaponState State);
	void Dropped();
	void SpendRound();
	void SetHUDAmmo();
	
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

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	/* Weapon Type */
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess), Category = "Weapon Properties")
	EWeaponType WeaponType;

	/** Class Reference. ProjectileClass can be populated with AProjectile or anything derived from AProjectile. */
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

private:
	UPROPERTY()
	class ABlasterCharacter* WeaponOwnerCharacter;
	
	UPROPERTY()
	class ABlasterPlayerController* WeaponOwnerController;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Component")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Component")
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Component", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* PickupWidget;

	/* FOV of aiming */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float Aim_FOV = 45.f;

	/* Zoom speed when aim in. */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float ZoomInSpeed = 20.f;

	/* Zoom speed when aim off. */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float ZoomOutSpeed = 0.f;

	/* Accuracy */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AimAccuracy = .1f;

	/* Recoil */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float RecoilFactor = .75f;

	/* The animation of the weapon when it's firing. */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	/* Weapon State */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties", ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState;
	
	UFUNCTION()
	void OnRep_WeaponState();

	void HandleWeaponState();

	/* Speed of firing */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float FireRate = .05f;

	/* If the weapon has auto-fire mode. */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool CanAutoFire = true;

	/* If the weapon has semi-auto-fire mode. */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool CanSemiAutoFire = true;

	/* Current ammo amount */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties", ReplicatedUsing = OnRep_Ammo)
	int32 Ammo = 30;

	/* Clip size */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 ClipSize = 45;

	/* Ammo replication notify */
	UFUNCTION()
	virtual void OnRep_Ammo();

	/* We need to make sure the owner exists when we update the ammo HUD which depends on the owner. */
	virtual void OnRep_Owner() override;

	/* When the weapon is dropped, we should reset the ownership. */
	void ResetOwnership();

	/* RenderCustomDepthPass -- Stencil */
	void SetCustomDepth(bool bEnabled);

public:
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetAim_FOV() const { return Aim_FOV; }
	FORCEINLINE float GetZoomInSpeed() const { return ZoomInSpeed; }
	FORCEINLINE float GetZoomOutSpeed() const { return ZoomOutSpeed; }
	FORCEINLINE float GetAimAccuracy() const { return AimAccuracy; }
	FORCEINLINE float GetRecoilFactor() const { return RecoilFactor; }
	FORCEINLINE float GetFireRate() const { return FireRate; }
	FORCEINLINE bool GetCanAutoFire() const { return CanAutoFire; }
	FORCEINLINE bool GetCanSemiAutoFire() const { return CanSemiAutoFire; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE void SetAmmo(const int32 AmmoAmount) { Ammo = AmmoAmount; }
	FORCEINLINE int32 GetClipSize() const { return ClipSize; }
	FORCEINLINE void SetClipSize(const int32 Size) { ClipSize = Size; }
	FORCEINLINE bool IsAmmoEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsAmmoFull() const { return Ammo == ClipSize; }
	FORCEINLINE bool IsAmmoValid() const { return Ammo >=0 && ClipSize >= 0 && Ammo <= ClipSize; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE void SetWeaponType(const EWeaponType Type) { WeaponType = Type; }

	/**
	* Textures for the weapon cross hairs
	*/

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairsMaxSpread;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairsMinSpread;

	/**
	 *	Weapon's sound effect when equipped
	 */
	UPROPERTY(EditAnywhere, Category = SoundEffect)
	USoundBase* EquippedSound;
};
