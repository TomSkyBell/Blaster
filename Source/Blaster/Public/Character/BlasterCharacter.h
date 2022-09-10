// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"


class FOnTimelineFloat;
UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	
protected:
	virtual void BeginPlay() override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void EquipButtonPressed();
	virtual void Jump() override;
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void SwitchFireModeButtonPressed();

public:
	void SetOverlappingWeapon(class AWeapon* Weapon);
	void PlayFireMontage(bool bAiming) const;
	void Eliminated();
	
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = Input)
	float TurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	// Replication only works when the variable is changed
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	// The function only works when the variable replicated 
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UPROPERTY(VisibleAnywhere)
	class ABlasterPlayerController* BlasterPlayerController;

	FRotator LastAimRotation;
	float AO_Yaw, Interp_AO_Yaw;
	float AO_Pitch;
	void AimOffset(float DeltaTime);	// Interpolation

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);	// Interpolation
	
	FTransform LeftHandTransform;
	void FABRIK_IK_LeftHand();
	
	FRotator RightHandRotation;
	void CorrectWeaponRotation(float DeltaTime);

	/**
	 *	Animation Montage
	 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathHipMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathIronMontage;

	/**
	 *	Set a threshold between camera and the character to avoid blocking.
	 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThreshold = 200.f;

	/**
	 *	Check if camera is too close with the character, if so, hide the character to avoid blocking the vision.
	 */
	void HideCharacterIfClose();

	/**
	 *	Player status.
	 */
	UPROPERTY(EditAnywhere, Category = PlayerStats)
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = PlayerStats, ReplicatedUsing = OnRep_Health)
	float Health = 100.f;

	FTimerHandle RespawnTimer;

	UPROPERTY(EditAnywhere, Category = PlayerStats)
	float TimerDelay = 3.f;
	
	void RespawnTimerFinished();

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHealth();
	void PlayHitReactMontage() const;
	void PlayDeathHipMontage() const;
	void PlayDeathIronMontage() const;

	/**
	 *	Dissolve Effect
	 */
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UTimelineComponent* TimelineComponent;

	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve;

	FOnTimelineFloat DissolveTrack;

	UFUNCTION()
	void UpdateMaterial(float CurveValue);

	void StartDissolve();

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMatInst;

	/**
	 *	Eliminated Particle Effect -- Robot
	 */
	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBot;

	UPROPERTY(EditAnywhere, Category = Elim)
	USoundBase* ElimBotSound;

	void PlayElimBotEffect();

public:
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE FTransform GetLeftHandTransform() const { return LeftHandTransform; }
	FORCEINLINE FRotator GetRightHandRotation() const { return RightHandRotation; }
	FORCEINLINE ETurningInPlace GetTuringInPlace() const { return TurningInPlace; }
	bool IsFireButtonPressed() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
};
