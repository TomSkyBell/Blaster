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
	
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess))
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
	
	UPROPERTY(VisibleAnywhere, Category = Weapon)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = Weapon, ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState;
	
	UFUNCTION()
	void OnRep_WeaponState();

	void HandleWeaponState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = Weapon)
	class UAnimationAsset* FireAnimation;

	/**
	 *	Change camera's FOV when aim zooming
	 */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float Aim_FOV = 45.f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float ZoomInSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float ZoomOutSpeed = 0.f;

	/**
	 *	Weapon's cross hair's spread when zooming and shooting
	 */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float AimAccuracy = .1f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float RecoilFactor = .75f;

	/**
	 *	Weapon's firing properties
	 */
	UPROPERTY(EditAnywhere, Category = Fire)
	float FireRate = .05f;

	UPROPERTY(EditAnywhere, Category = Fire)
	bool CanAutoFire = true;

	UPROPERTY(EditAnywhere, Category = Fire)
	bool CanSemiAutoFire = true;
	
	UPROPERTY(EditAnywhere, Category = Fire, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo = 30;

	UPROPERTY(EditAnywhere, Category = Fire)
	int32 ClipSize = 45;

	UFUNCTION()
	virtual void OnRep_Ammo();

	/**
	 *	We need to make sure the owner exists when we update the ammo HUD which depends on the owner.
	 */
	virtual void OnRep_Owner() override;

	void ResetOwnership();

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
