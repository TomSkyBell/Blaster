// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

private:
	UPROPERTY(VisibleAnywhere, Category = Weapon)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = Weapon, ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState;
	
	UFUNCTION()
	void OnRep_WeaponState();

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

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetAim_FOV() const { return Aim_FOV; }
	FORCEINLINE float GetZoomInSpeed() const { return ZoomInSpeed; }
	FORCEINLINE float GetZoomOutSpeed() const { return ZoomOutSpeed; }
	FORCEINLINE float GetAimAccuracy() const { return AimAccuracy; }
	FORCEINLINE float GetRecoilFactor() const { return RecoilFactor; }

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
};
