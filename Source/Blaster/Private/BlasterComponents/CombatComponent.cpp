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

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	
	UpdateHUDCrosshairs(DeltaTime);
	AimZooming(DeltaTime);
	
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);

	// Carried Ammo is correlated with the HUD Ammo, HUD can only be updated on the owning client, so we should declare
	// the Ammo as COND_OwnerOnly except that the Ammo need shared among the clients.
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
}

void UCombatComponent::SetWeaponRelatedProperties()
{
	if (!EquippedWeapon) return;
	
	// If the new weapon has the mode we got, then do nothing, else we change the fire mode.
	if (bAutomaticFire && !EquippedWeapon->GetCanAutoFire() ||
		!bAutomaticFire && !EquippedWeapon->GetCanSemiAutoFire())
	bAutomaticFire = !bAutomaticFire;
}

// This function is invoked from the server.
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!BlasterCharacter || !WeaponToEquip) return;

	if (EquippedWeapon) EquippedWeapon->Dropped();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if (const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	// SetOwner() is a replication process, no need to redo it in OnRep.
	EquippedWeapon->SetOwner(BlasterCharacter);
	EquippedWeapon->SetHUDAmmo();
	AccessCarriedAmmoMap();
	SetHUDCarriedAmmo();

	if (BlasterCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippedSound, BlasterCharacter->GetActorLocation(), FRotator::ZeroRotator);
	}
	
	// The server solely set the properties, the clients' are set in the OnRep function.
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
	/**************************************** Repnotify & Multicast Sequence Conflicts *******************************************/
	
	// SetWeaponState() can be only executed from the server, but we still set up here because of the replication delay, specifically,
	// AttachActor needs the weapon to be set simulated physics, and this property is set when we do SetWeaponState() and it needs some
	// time to replicate from the server to the client, which means when we attach actor on the server, the multicast speed is faster than
	// the RepNotify, so when the client attach actor, it will fail because the Simulate Physics is not updated in time.
	if (!EquippedWeapon || !BlasterCharacter || !BlasterCharacter->GetMesh()) return;
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if (const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;

	if (BlasterCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippedSound, BlasterCharacter->GetActorLocation(), FRotator::ZeroRotator);
	}
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

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// Multicast is invoked from the server, so the function will run on the server and all other clients.
	MulticastFire(TraceHitTarget);
}

// If this function is invoked from the client, then it will only run on the client which makes no sense.
// If this function is invoked from the server, then it will run on the server and all the clients.
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// The replicated variable 'CombatState' should be checked on the server because on the server it's updated
	// while on the client it may not, which causes some unexpected error.
	if (!BlasterCharacter || !EquippedWeapon || CombatState != ECombatState::ECS_Unoccupied) return;

	AimFactor += EquippedWeapon->GetRecoilFactor();
	BlasterCharacter->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
	
}

void UCombatComponent::Fire()
{
	if (!CanFire()) return;
	
	ServerFire(HitTarget);
	if (bAutomaticFire) StartFireTimer();
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// Everytime we call the RPC, the data will be sent across the network. And with multiplayer game, the less data we sent, the better.
	// It's only for things which are very important in the game such as shooting will need RPC.
	bFireButtonPressed = bPressed;
	Fire();
}

bool UCombatComponent::CanFire() const
{
	return bRefireCheck && bFireButtonPressed && EquippedWeapon && EquippedWeapon->GetAmmo() > 0;
}

void UCombatComponent::StartFireTimer()
{
	if (!BlasterCharacter || !EquippedWeapon) return;

	bRefireCheck = false;
	BlasterCharacter->GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::FireTimerFinished, EquippedWeapon->GetFireRate(), false);
}

void UCombatComponent::FireTimerFinished()
{
	bRefireCheck = true;
	Fire();
}

void UCombatComponent::SwitchFireModeButtonPressed()
{
	if (!EquippedWeapon) return;
	
	if (bAutomaticFire && EquippedWeapon->GetCanSemiAutoFire() ||
		!bAutomaticFire && EquippedWeapon->GetCanAutoFire())
			bAutomaticFire = !bAutomaticFire;
}

void UCombatComponent::SetHUDCarriedAmmo()
{
	if (!BlasterCharacter) return;
	
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(BlasterCharacter->Controller);
	if (BlasterPlayerController) BlasterPlayerController->UpdateCarriedAmmo(CarriedAmmo);
}

void UCombatComponent::AccessCarriedAmmoMap()
{
	if (!EquippedWeapon) return;
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
}

void UCombatComponent::UpdateCarriedAmmoMap()
{
	if (!EquippedWeapon) return;
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	SetHUDCarriedAmmo();
	UpdateCarriedAmmoMap();
}

void UCombatComponent::ReloadAnimNotify()
{
	if (!BlasterCharacter || !EquippedWeapon) return;
	
	if (BlasterCharacter->HasAuthority())
	{
		BlasterCharacter->SetCombatState(ECombatState::ECS_Unoccupied);
		ReloadAmmoAmount();
		Fire();
		
		EquippedWeapon->SetHUDAmmo();
		SetHUDCarriedAmmo();

		/**************************************  Simulation On Server  ****************************************/
		// At the start of the game, the simulated proxy on the server, which means server does the simulation
		// for the working of this proxy, will transmit the same data as the owning client of this proxy. But
		// with the time advances, these two(owning client and the simulated client on the server) have the different
		// data, for instance when we press the button on the owning client, the server won't know about it, when we
		// are doing the replication on the server, the OnRep_ only work on the clients, and the simulated proxy's data
		// won't simultaneously updated with the clients, so the server won't know about the change, which will lead
		// to a wrong simulation, that's why we do the same work of OnRep_ on the server.
		UpdateCarriedAmmoMap();
	}
}

void UCombatComponent::ServerReloadButtonPressed_Implementation()
{
	if (!BlasterCharacter) return;

	// CombatState Replication, we put reload logic in AnimMontage's Notify
	CombatState = ECombatState::ECS_Reloading;
	BlasterCharacter->PlayReloadMontage();
}

void UCombatComponent::ReloadButtonPressed()
{
	if (IsCarriedAmmoEmpty() || CombatState == ECombatState::ECS_Reloading ||
		!EquippedWeapon || EquippedWeapon->IsAmmoFull()) return;

	ServerReloadButtonPressed();
}

void UCombatComponent::ReloadAmmoAmount()
{
	if (!EquippedWeapon || !EquippedWeapon->IsAmmoValid()) return;
	
	if (CarriedAmmo >= (EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo()))
	{
		// Sequence is important
		CarriedAmmo -= EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo();
		EquippedWeapon->SetAmmo(EquippedWeapon->GetClipSize());
	}
	else
	{
		// Sequence is important
		EquippedWeapon->SetAmmo(EquippedWeapon->GetAmmo() + CarriedAmmo);
		CarriedAmmo = 0;
	}
}

void UCombatComponent::OnRep_CombatState()
{
	if (!BlasterCharacter) return;

	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// Fire check is inside the Fire().
		Fire();
		break;
	case ECombatState::ECS_Reloading:
		BlasterCharacter->PlayReloadMontage();
		break;
	case ECombatState::ECS_MAX:
		break;
	}
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
	HitTarget = HitResult.ImpactPoint;
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
