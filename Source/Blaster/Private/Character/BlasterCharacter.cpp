// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "BlasterComponents/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	TurnRate = 50.f;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	Combat->SetIsReplicated(true);

	// Avoid the zooming effect (camera overlaps with the character)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	AimOffset(DeltaTime);
	
	FABRIK_IK_LeftHand();

	CorrectWeaponRotation();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f && !bIsInAir)
	{
		// Default set to false, we set it to true in 'TurnInPlace' function.
		bUseControllerRotationYaw = false;

		// If we aim, we turn in place when needed, if we just click aim once, we should wait for the 'Turn in place' finished
		// so we need to check the TurningInPlace condition.
		if (IsAiming() || TurningInPlace != ETurningInPlace::ETIP_NotTurning) TurnInPlace(DeltaTime);
	}
	else if (Speed > 0.f || bIsInAir)
	{
		// Reset the AimOffset-related properties
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	}
	AO_Pitch = GetBaseAimRotation().Pitch;

	// Due to Package issue over the network, we need to fix it on the AO_Pitch.
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// 1. Offset the 'RotateRootBone' node in AnimBP when in idle
	// 2. When the turning rotation is too big like 150 degrees, we can make a trick by 'bUseControllerRotationYaw' to make the character looks like
	// he rotates 150 degrees even though we actually make him rotate only 90 degrees.
	bUseControllerRotationYaw = true;

	if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
	{
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, LastAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		Interp_AO_Yaw = AO_Yaw;
		if (AO_Yaw > 90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (AO_Yaw < -90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	}
	else
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0, DeltaTime, 10.f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(Interp_AO_Yaw) <  1.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::FABRIK_IK_LeftHand()
{
	if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh() && GetMesh())
	{
		// Get the socket transform in the world space
		LeftHandTransform = Combat->EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		// Transform the socket into the bone space ---- relative to the bone 'hand_r'
		GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}

void ABlasterCharacter::CorrectWeaponRotation()
{
	// TraceUnderCorsshairs is a machine-related function, so the HitResult cannot be known by the other machine unless by RPC.
	if (!Combat) return;
	FHitResult HitResult;
	Combat->TraceUnderCrosshairs(HitResult);
	
	// To adjust the hand rotation, to nearly match the two trace line, not precisely match. (This is a trick)
	if (!Combat->EquippedWeapon || !Combat->EquippedWeapon->GetWeaponMesh()) return;
	const FTransform RightHandTransform = Combat->EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
	RightHandRotation = UKismetMathLibrary::FindLookAtRotation(HitResult.ImpactPoint, RightHandTransform.GetLocation());
}



/**
 * @brief  Player Input
 * @param 
 */
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("TurnRight", this, &ThisClass::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUpAtRate);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::TurnAtRate(float Value)
{
	AddControllerYawInput(Value * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ABlasterCharacter::LookUpAtRate(float Value)
{
	AddControllerPitchInput(Value * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			// RPC called from the client
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched) UnCrouch();
	else Super::Jump();
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (GetCharacterMovement()->IsFalling()) return;
	
	if (bIsCrouched) UnCrouch();
	else Crouch();
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}



// =============================    Replication Work     ====================================//

// RPC executed from the server, but server just does the work for the client instead.
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// If OverlappingWeapon is replicated, then it'll be only effect on the owner so that the players will not interfere with each other 
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->BlasterCharacter = this;
	}
}

// This function is executed on the server.
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// When the overlap ends, we should hide the PickupWidget's text. To make that, we need firstly 'ShowPickupWidget(false)' before we assign the
	// Weapon (Weapon == nullptr when overlap ends) to the OverlappingWeapon. Otherwise, we have no chance to change the PickupWidget's text.
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}
	// Replication works.
	OverlappingWeapon = Weapon;
	
	// Overlap begins, show the PickupWidget's text.
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

// 1. If OverlappingWeapon is changed, then the OnRep_ function will be called.
// 2. The server will not do the OnRep_ function, this just be done on each client.
// 3. Only the client notified with the change of the property can do the OnRep_ function,
//    which means when we declare COND_OwnerOnly, not all of the clients will do the RepNotify function.
// 4. LastWeapon is the last replicated value before the new replication.
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// Overlapping weapon is a pointer, so it doesn't belong to a specific object, it is a weapon in the scene, which means it's global.
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}



void ABlasterCharacter::PlayFireMontage(bool bAiming) const
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bAiming ? FName("Aim_Fire") : FName("Hip_Fire");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}


bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const
{
	return (Combat && Combat->bAiming);
}

bool ABlasterCharacter::IsFireButtonPressed() const
{
	return (Combat && Combat->bFireButtonPressed);
}


