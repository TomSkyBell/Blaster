// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// BlasterCharacter is initialized in BlasterCharacter.cpp, PostInitializeComponents() function.
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BaseCrouchWalkSpeed;
		DefaultFOV = BlasterCharacter->GetFollowCamera()->FieldOfView;
		InterpFOV = DefaultFOV;
		if (EquippedWeapon) CrosshairSpread = EquippedWeapon->CrosshairsMinSpread;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateHUDCrosshairs(DeltaTime);
	AimZooming(DeltaTime);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}


// This function is invoked from the server.
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!BlasterCharacter || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	// Set the owner of this Actor, used primarily for network replication. 
	EquippedWeapon->SetOwner(BlasterCharacter);

	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;

	// It's ideal and easy to think of setting the cross hair hud once we equip the weapon, but we need to take care that
	// this functionality is implemented on the server and the server cannot get the client's hud since the client's remote
	// role is autonomous proxy and it's not allowed. (Other side, it makes sense, right? We cannot set the hud remotely from
	// another machine.) That's why we finally choose to put the functionality into the tick part.
	// SetHUDCrosshairs();
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// If the ownership is self, then we'd better do the 'variable assignment' work in 'Set Aiming' right away rather than waiting for
	// the 'ServerSetAiming' because ServerSetAiming is the RPC which needs time to transfer the replication from the server to the client.
	bAiming = bIsAiming;
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = bIsAiming ? AimCrouchWalkSpeed : BaseCrouchWalkSpeed;
	}
	// If the ownership is client, it'll be invoked from the server; if the ownership is server, it'll be invoked from the server as well.
	ServerSetAiming(bIsAiming);
}


void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	
	// CharacterMovementComponent is a powerful built-in component, supporting network replication,
	// so if we wanna change the MaxWalkSpeed, we should do it on the server to let all the machines known.
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = bIsAiming ? AimCrouchWalkSpeed : BaseCrouchWalkSpeed;
	}
}

// ===============================================================================================================
// ********************************************  Multicast RPC  **************************************************
// ===============================================================================================================

// 1. If a client_A is calling ServerRPC, it means the server can know what client_A is doing, but if client_A is not doing locally, the Server
// won't multicast to Client_A, which means Server screen can display what Client_A is doing, but Client screen won't display unless
// Client_A itself locally do the work.
// 2. Likewise, if all the clients do the RPC but not do its work locally, the Server won't multicast to them, so the clients' screen won't
// display their work, no more to say the clients can know each other's work.
// 3. If all the clients do their work locally, now the Server and the client itself can know what it's doing, but the clients can't know each
// other's work, so the MulticastRPC is introduced. The clients just needn't do their own work locally but let the MulticastRPC executed from the
// Server, so the Server will make all the clients do its own work and other clients' work.


void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// Multicast is invoked from the server, so the function will run on the server and all other clients.
	MulticastFire(TraceHitTarget);
}

// If this function is invoked from the client, then it will only run on the client which makes no sense.
// If this function is invoked from the server, then it will run on the server and all the clients.
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!BlasterCharacter || !EquippedWeapon) return;

	BlasterCharacter->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
	
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// Everytime we call the RPC, the data will be sent across the network. And with multiplayer game, the less data we sent, the better.
	// It's only for things which are very important in the game such as shooting will need RPC.
	bFireButtonPressed = bPressed;

	if (!bFireButtonPressed || !EquippedWeapon) return;

	AimFactor += EquippedWeapon->GetRecoilFactor();

	// We can't calculate the HitResult in the multicast, or the machine will use its own screen to calculate the HitResult, which is
	// different from the owning actor's HitResult. So we should do the work by the actor who pressed the fire button, and transmit
	// the result over the network.
	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	ServerFire(HitResult.ImpactPoint);
	
}



/**
 * Project a line trace from the center of the screen to the target.
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& HitResult)
{
	// 'TraceUnderCrosshair' is a machine-related function, so it shouldn't be called by another machine.
	if (!BlasterCharacter || !BlasterCharacter->IsLocallyControlled()) return;
	
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D CrosshairScreenLocation(ViewportSize * 0.5f);
	FVector CrosshairWorldLocation;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairScreenLocation,
		CrosshairWorldLocation,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldLocation;
		// A little trick about the distance and direction
		const float DistanceToCharacter = (CrosshairWorldLocation - BlasterCharacter->GetActorLocation()).Size();
		Start += (DistanceToCharacter + 100.f) * CrosshairWorldDirection;
		const FVector End = CrosshairWorldLocation + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
		if (HitResult.GetActor())
		{
			const bool bIsImplemented = HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>();
			CrosshairColor = bIsImplemented ? FColor::Red : FColor::White;
		}
		else
		{
			CrosshairColor = FColor::White;
		}
	}
}

void UCombatComponent::UpdateHUDCrosshairs(float DeltaTime)
{
	if (!BlasterCharacter || !BlasterCharacter->IsLocallyControlled()) return;
	if (!BlasterCharacter->Controller) return;
	
	// Instanced by casting.
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(BlasterCharacter->Controller);
	if (!BlasterPlayerController) return;

	// If a client is on a machine, and its remote role is autonomous proxy, then the server cannot get its hud. (The server cannot get hud from
	// an autonomous proxy). Besides, it makes sense that the hud can only be display/owned by the local machine and can't be transmitted.
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(BlasterPlayerController->GetHUD());

	UpdateCrosshairSpread(DeltaTime);
	SetHUDPackage();
	
}

void UCombatComponent::UpdateCrosshairSpread(float DeltaTime)
{
	if (!BlasterCharacter || !BlasterHUD || !EquippedWeapon) return;

	// Interpolate the spread when the player is moving
	if (BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
	{
		VelocityFactor = FMath::FInterpTo(VelocityFactor, 1.f, DeltaTime, VelocityFactor_InterpSpeed);
	}
	else
	{
		VelocityFactor = FMath::FInterpTo(VelocityFactor, 0.f, DeltaTime, VelocityFactor_InterpSpeed);
	}

	// Interpolate the spread when the player is jumping
	if (BlasterCharacter->GetCharacterMovement()->IsFalling())
	{
		AirFactor = FMath::FInterpTo(AirFactor, 1, DeltaTime, 1.f);
	}
	else
	{
		AirFactor = FMath::FInterpTo(AirFactor, 0, DeltaTime, AirFactor_InterpSpeed);
	}
	if (bAiming)
	{
		AimFactor = FMath::FInterpTo(AimFactor, EquippedWeapon->GetAimAccuracy(), DeltaTime, AimFactor_InterpSpeed);
	}
	else
	{
		AimFactor = FMath::FInterpTo(AimFactor, 1.f, DeltaTime, AimFactor_InterpSpeed);
	}
	const float FinalFactor = VelocityFactor + AirFactor;
	
	// The DrawHUD function will be automatically called when we set the default HUD as BP_BlasterHUD in BP_GameMode settings.
	CrosshairSpread = 
		FinalFactor * (EquippedWeapon->CrosshairsMaxSpread - EquippedWeapon->CrosshairsMinSpread) +
		AimFactor * EquippedWeapon->CrosshairsMinSpread;
}

void UCombatComponent::SetHUDPackage()
{
	if (BlasterHUD == nullptr) return;

	if (EquippedWeapon)
	{
		BlasterHUD->SetHUDPackage(
			FHUDPackage(
				EquippedWeapon->CrosshairsCenter,
				EquippedWeapon->CrosshairsLeft,
				EquippedWeapon->CrosshairsRight,
				EquippedWeapon->CrosshairsTop,
				EquippedWeapon->CrosshairsBottom,
				CrosshairSpread,
				CrosshairColor
			)
		);
	}
	// In cases that we change a weapon, we need to refresh first.
	else
	{
		BlasterHUD->SetHUDPackage(
			FHUDPackage( nullptr, nullptr, nullptr, nullptr, nullptr, 0.f)
		);
	}
}


void UCombatComponent::AimZooming(float DeltaTime)
{
	if (!BlasterCharacter || !EquippedWeapon || !BlasterCharacter->IsLocallyControlled()) return;
	
	if (bAiming)
	{
		InterpFOV = FMath::FInterpTo(InterpFOV, EquippedWeapon->GetAim_FOV(), DeltaTime, EquippedWeapon->GetZoomInSpeed());
	}
	else
	{
		InterpFOV = FMath::FInterpTo(InterpFOV, DefaultFOV, DeltaTime, DefaultZoomOutSpeed);
	}
	BlasterCharacter->GetFollowCamera()->FieldOfView = InterpFOV;
}


