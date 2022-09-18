 // Fill out your copyright notice in the Description page of Project Settings.


 #include "Character/BlasterAnimInstance.h"
 #include "Character/BlasterCharacter.h"
 #include "GameFramework/CharacterMovementComponent.h"
 #include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

 void UBlasterAnimInstance::NativeInitializeAnimation()
 {
 	Super::NativeInitializeAnimation();

 	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
 }

 void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
 {
 	Super::NativeUpdateAnimation(DeltaSeconds);

 	if (BlasterCharacter == nullptr)
 	{
 		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
 	}
 	if (BlasterCharacter == nullptr) return;

 	if (BlasterCharacter->IsLocallyControlled()) bIsLocallyControlled = true;
 	else bIsLocallyControlled = false;

 	FVector Velocity = BlasterCharacter->GetVelocity();
 	Velocity.Z = 0.f;
 	Speed = Velocity.Size();

 	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
 	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
 	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
 	bIsCrouched = BlasterCharacter->bIsCrouched;
 	bAiming = BlasterCharacter->IsAiming();

 	// Offset yaw for strafing
 	// GetBaseAimRotation() is a global rotation, not local to the character.
 	const FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
 	// GetVelocity() is also a global rotation
 	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
 	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
 	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 5.f);
 	YawOffset = DeltaRotation.Yaw;

 	LastFrameLean = NewFrameLean;
 	NewFrameLean = BlasterCharacter->GetActorRotation();
 	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(NewFrameLean, LastFrameLean);
 	const float TargetLean = Delta.Yaw / DeltaSeconds;
 	const float Interp = FMath::FInterpTo(Lean, TargetLean, DeltaSeconds, 5.f);
 	Lean = FMath::Clamp(Interp, -90.f, 90.f);

 	AO_Yaw = BlasterCharacter->GetAO_Yaw();
 	AO_Pitch = BlasterCharacter->GetAO_Pitch();

 	LeftHandTransform = BlasterCharacter->GetLeftHandTransform();

 	TurningInPlace = BlasterCharacter->GetTuringInPlace();

 	RightHandRotation = BlasterCharacter->GetRightHandRotation();

 	bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
 	bUseAimOffset = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
 	bTransformRightHand = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
 }

