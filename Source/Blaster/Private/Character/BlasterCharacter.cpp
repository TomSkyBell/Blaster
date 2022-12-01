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
#include "Blaster/Blaster.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/BlasterPlayerController.h"

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

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	Combat->SetIsReplicated(true);

	TimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("Timeline Component"));

	// Avoid the zooming effect (camera overlaps with the character)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	
	if (HasAuthority()) OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	AimOffset(DeltaTime);
	
	FABRIK_IK_LeftHand();

	CorrectWeaponRotation(DeltaTime);

	HideCharacterIfClose();
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

void ABlasterCharacter::CorrectWeaponRotation(float DeltaTime)
{
	// TraceUnderCorsshairs is a machine-related function, so the HitResult cannot be known by the other machine unless by RPC.
	if (!Combat) return;
	
	
	// To adjust the hand rotation, to nearly match the two trace line, not precisely match. (This is a trick)
	if (!Combat->EquippedWeapon || !Combat->EquippedWeapon->GetWeaponMesh()) return;
	const FTransform RightHandTransform = Combat->EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Combat->HitTarget, RightHandTransform.GetLocation());
	RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 20.f);
}

void ABlasterCharacter::HideCharacterIfClose()
{
	if (!IsLocallyControlled()) return;
	if (!FollowCamera || !GetMesh()) return;
	if (!Combat || !Combat->EquippedWeapon || !Combat->EquippedWeapon->GetWeaponMesh()) return;
	
	const bool bHideCharacter = ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold);
	GetMesh()->SetVisibility(!bHideCharacter);

	// When the camera is close to the character, we make the character and the weapon invisible, so in this situation when the character is eliminated,
	// the weapon is dropped and it's still invisible, so we need to recover its visibility.
	const bool bHideWeapon = bHideCharacter && Combat->EquippedWeapon->GetWeaponState() == EWeaponState::EWS_Equipped;
	Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(!bHideWeapon);
}

void ABlasterCharacter::Eliminated()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastEliminated();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::RespawnTimerFinished, TimerDelay, false);
}

void ABlasterCharacter::RespawnTimerFinished()
{
	if (!GetWorld()) return;
	
	if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHealth();
	PlayHitReactMontage();
}

void ABlasterCharacter::OnRep_IsRespawned()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
	if (!BlasterPlayerController) return;
	
	BlasterPlayerController->RefreshHUD();
}

void ABlasterCharacter::MulticastEliminated_Implementation()
{
	// Disable the collision.
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetMesh()) GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Disable the movement of inertial even though we lose the control. 
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
	if (BlasterPlayerController)
	{
		// Play the defeated HUD effect
		BlasterPlayerController->DisplayDefeatedMsg();

		// Disable the controller input.
		DisableInput(BlasterPlayerController);
		
		// We need to make sure bFireButtonPressed is cleared or the weapon will keep firing because the input is disabled
		FireButtonReleased();

		// We need to make sure aim button is released so that the sniper scope widget is hidden from the screen.
		AimButtonReleased();
	}
	IsAiming() ? PlayDeathIronMontage() : PlayDeathHipMontage();

	StartDissolve();
	PlayElimBotEffect();
	OverheadWidget->DestroyComponent(true);
}

// Receive Damage is executed from the server due to ApplyDamage() in OnHit(), so no need to recheck by HasAuthority().
// In this function, DamagedActor == this, so it's a little bit over-defined.
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	UpdateHealth();
	PlayHitReactMontage();

	if (Health <= 0.f && GetWorld())
	{
		if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
		{
			BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatedBy);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::UpdateHealth()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
	if (!BlasterPlayerController) return;

	BlasterPlayerController->UpdatePlayerHealth(Health, MaxHealth);
}

void ABlasterCharacter::PlayHitReactMontage() const
{
	// If health <= 0.f, it should play the eliminated animation rather than the hit react.
	// We can make a check if (Health <= 0.f) here because it's in the OnRep function, in which the Health updates before check.
	if (!Combat || !Combat->EquippedWeapon || Health <= 0.f) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName SectionName = FName("FromLeft");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayDeathHipMontage() const
{
	// Since health is a replicated variable, it may not update immediately due to the network state, so we cannot check if (Health < 0.f) here
	// unless we are in a OnRep_Health() function.
	// The montage is played only when the character is holding a weapon.
	if (!Combat || !Combat->EquippedWeapon) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathHipMontage)
	{
		AnimInstance->Montage_Play(DeathHipMontage);
		const FName SectionName = FName("Death Hip 1");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayDeathIronMontage() const
{
	// Since health is a replicated variable, it may not update immediately due to the network state, so we cannot check if (Health < 0.f) here.
	// unless we are in a OnRep_Health() function.
	// The montage is played only when the character is holding a weapon.
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathIronMontage)
	{
		AnimInstance->Montage_Play(DeathIronMontage);
		const FName SectionName = FName("Death Iron 1");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage() const
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("AssaultRifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SMG:
			SectionName = FName("SMG");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		case EWeaponType::EWT_MAX:
			SectionName = FName("MAX");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage() const
{
	if (!GetMesh()) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::UpdateMaterial(float CurveValue)
{
	if (!DynamicDissolveMatInst) return;
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Dissolve"), CurveValue);
}

void ABlasterCharacter::StartDissolve()
{
	if (!TimelineComponent || !DissolveCurve || !GetMesh()) return;

	// Initialize the dynamic material.
	DynamicDissolveMatInst = UMaterialInstanceDynamic::Create(DissolveMatInst, this);
	if (!DynamicDissolveMatInst) return;

	GetMesh()->SetMaterial(0, DynamicDissolveMatInst);
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Dissolve"), 0.55f);
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Glow"), 200.f);

	// Initialize the timeline component.
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateMaterial);
	TimelineComponent->AddInterpFloat(DissolveCurve, DissolveTrack);
	TimelineComponent->Play();
}

void ABlasterCharacter::PlayElimBotEffect()
{
	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 200.f);
	UGameplayStatics::SpawnEmitterAtLocation(this, ElimBot, SpawnLocation);
	UGameplayStatics::PlaySoundAtLocation(this, ElimBotSound, SpawnLocation, FRotator::ZeroRotator);
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
	PlayerInputComponent->BindAction("SwitchFireMode", IE_Pressed, this, &ThisClass::SwitchFireModeButtonPressed);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ThisClass::ThrowButtonPressed);

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
	// Functionality which is implemented on the server to let all the machines know.
	ServerEquipButtonPressed();
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

void ABlasterCharacter::SwitchFireModeButtonPressed()
{
	if (Combat)
	{
		Combat->SwitchFireModeButtonPressed();
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
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
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, IsRespawned);
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

ECombatState ABlasterCharacter::GetCombatState() const
{
	return Combat ? Combat->GetCombatState() : ECombatState::ECS_MAX;
}

void ABlasterCharacter::SetCombatState(ECombatState State)
{
	if (Combat)
	{
		Combat->SetCombatState(State);
	}
}
