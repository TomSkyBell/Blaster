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
#include "Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize the CarriedAmmoMap.
	InitCarriedAmmoMap();
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// BlasterCharacter is initialized in BlasterCharacter.cpp, PostInitializeComponents() function.
	if (BlasterCharacter)
	{
		UpdateCharacterSpeed();
		DefaultFOV = BlasterCharacter->GetFollowCamera()->FieldOfView;
		InterpFOV = DefaultFOV;
		if (EquippedWeapon) CrosshairSpread = EquippedWeapon->CrosshairsMinSpread;
	}
	// Initialize the CarriedAmmoMap.
	InitCarriedAmmoMap();
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
	DOREPLIFETIME_CONDITION(UCombatComponent, Grenade, COND_OwnerOnly);
}

void UCombatComponent::UpdateCharacterSpeed()
{
	if (BlasterCharacter && BlasterCharacter->GetCharacterMovement())
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = bAiming ? AimCrouchWalkSpeed : BaseCrouchWalkSpeed;
	}
}

// This function is invoked from the server.
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!BlasterCharacter || !WeaponToEquip) return;

	// Drop equipped weapon.
	if (EquippedWeapon) EquippedWeapon->Dropped();

	// Set weapon and its state.
	EquippedWeapon = WeaponToEquip;
	bAutomaticFire = EquippedWeapon->GetCanAutoFire();
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
	AttachWeaponToRightHand();
	
	// Owner is built-in replicated variable.
	EquippedWeapon->SetOwner(BlasterCharacter);

	// Show the HUD: weapon type, ammo amount, carried ammo amount.
	SetHUDWeaponType();
	EquippedWeapon->SetHUDAmmo();
	SetCarriedAmmoFromMap(EquippedWeapon->GetWeaponType());	// Set carried ammo and display the HUD.
	
	// Play equip sound.
	if (BlasterCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippedSound, BlasterCharacter->GetActorLocation(), FRotator::ZeroRotator);
	}
	
	// The server solely set the properties, the clients' are set in the OnRep function.
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	/**************************************** Repnotify & Multicast Sequence Conflicts *******************************************/
	
	// SetWeaponState() can be only executed from the server, but we still set up here because of the replication delay, specifically,
	// AttachActor needs the weapon to be set simulated physics false, and this property is set when we do SetWeaponState() and it needs
	// time to replicate from the server to the client, which means when we attach actor on the server, the multicast speed is faster than
	// the RepNotify, so when the client attach actor, it will fail because the Simulate Physics is not updated in time.
	if (!EquippedWeapon || !BlasterCharacter || !BlasterCharacter->GetMesh()) return;

	bAutomaticFire = EquippedWeapon->GetCanAutoFire();
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if (const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		// Automatically propagated to the clients, that's why we don't need to do attachment on the client again.
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
	
	SetHUDWeaponType();
	if (BlasterCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquippedSound, BlasterCharacter->GetActorLocation(), FRotator::ZeroRotator);
	}
}

void UCombatComponent::SetCombatState(const ECombatState State)
{
	CombatState = State;
	HandleCombatState();
}

void UCombatComponent::SetCarriedAmmo(int32 Amount)
{
	CarriedAmmo = Amount;
	HandleCarriedAmmo();
}

void UCombatComponent::HandleCarriedAmmo()
{
	if (!EquippedWeapon) return;
	
	SetHUDCarriedAmmo();
	UpdateCarriedAmmoMap({EquippedWeapon->GetWeaponType(), CarriedAmmo});

	// Jump to the end section of animation when the carried ammo is not enough to fulfill the clip or the clip has been fulfilled during reloading.
	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		(EquippedWeapon->IsAmmoFull() || IsCarriedAmmoEmpty() && !EquippedWeapon->IsAmmoFull()))
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (!BlasterCharacter || !EquippedWeapon) return;
	
	// If the ownership is self, then we'd better do the 'variable assignment' work in 'Set Aiming' right away rather than waiting for
	// the 'ServerSetAiming' because ServerSetAiming is the RPC which needs time to transfer the replication from the server to the client.
	bAiming = bIsAiming;
	UpdateCharacterSpeed();
	// If the ownership is client, it'll be invoked from the server; if the ownership is server, it'll be invoked from the server as well.
	ServerSetAiming(bIsAiming);

	// Sniper scope effect when aiming. Be aware of IsLocallyControlled check.
	if (BlasterCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		BlasterCharacter->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	
	// CharacterMovementComponent is a powerful built-in component, supporting network replication,
	// so if we wanna change the MaxWalkSpeed, we should do it on the server to let all the machines known.
	UpdateCharacterSpeed();
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
	// Be aware that sequence is important, because when we fire out the last ammo, the ammo will be 0
	// and reload will be executed immediately.
	if (EquippedWeapon && EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}
	if (CanFire())
	{
		ServerFire(HitTarget);
		StartFireTimer();
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// Everytime we call the RPC, the data will be sent across the network. And with multiplayer game, the less data we sent, the better.
	// It's only for things which are very important in the game such as shooting will need RPC.
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) Fire();
}

bool UCombatComponent::CanFire() const
{
	return bRefireCheck && EquippedWeapon && !EquippedWeapon->IsAmmoEmpty();
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
	if (bAutomaticFire && bFireButtonPressed) Fire();
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

void UCombatComponent::InitCarriedAmmoMap()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, 10);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, 5);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, 30);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, 45);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, 5);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, 3);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, 4);
}

void UCombatComponent::SetCarriedAmmoFromMap(EWeaponType WeaponType)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		SetCarriedAmmo(CarriedAmmoMap[WeaponType]);
	}
}

int32 UCombatComponent::GetCarriedAmmoFromMap(EWeaponType WeaponType)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		return CarriedAmmoMap[WeaponType];
	}
	return -1;
}

void UCombatComponent::UpdateCarriedAmmoMap(const TPair<EWeaponType, int32>& CarriedAmmoPair)
{
	if (CarriedAmmoMap.Contains(CarriedAmmoPair.Key))
	{
		CarriedAmmoMap[CarriedAmmoPair.Key] = CarriedAmmoPair.Value;
	}
}

void UCombatComponent::SetHUDWeaponType()
{
	if (!BlasterCharacter || !EquippedWeapon) return;

	FString WeaponType;
	switch(EquippedWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_AssaultRifle:
		WeaponType = FString("Assault Rifle");
		break;
	case EWeaponType::EWT_RocketLauncher:
		WeaponType = FString("Rocket Launcher");
		break;
	case EWeaponType::EWT_Pistol:
		WeaponType = FString("Pistol");
		break;
	case EWeaponType::EWT_SMG:
		WeaponType = FString("SMG");
		break;
	case EWeaponType::EWT_Shotgun:
		WeaponType = FString("Shotgun");
		break;
	case EWeaponType::EWT_SniperRifle:
		WeaponType = FString("Sniper Rifle");
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		WeaponType = FString("Grenade Launcher");
		break;
	case EWeaponType::EWT_MAX:
		WeaponType = FString("");
		break;
	}
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(BlasterCharacter->Controller);
	if (BlasterPlayerController) BlasterPlayerController->UpdateWeaponType(WeaponType);
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	HandleCarriedAmmo();
}

void UCombatComponent::ReloadAnimNotify()
{
	if (!BlasterCharacter || !EquippedWeapon) return;

	/* Rep notify problem. bFireButtonPressed and bAutomaticFire are local variables, so we should not check
	 * authority here, or the client will immediately call Fire() while the Ammo is not updated. */
	BlasterCharacter->SetCombatState(ECombatState::ECS_Unoccupied);
	ReloadAmmoAmount();	// Ammo and CarriedAmmo Rep Notify triggered.
	
	// Local variable, so we needn't check IsLocallyControlled().
	if (bFireButtonPressed && bAutomaticFire) Fire();
}

void UCombatComponent::ShotgunShellAnimNotify()
{
	// Once from the server 'PlayReloadMontage' is called, it has a multicast effect that the client will also plays the
	// montage, so we need to check the authority.
	if (BlasterCharacter && BlasterCharacter->HasAuthority())
	{
		if (!EquippedWeapon || !BlasterCharacter->GetMesh()) return;

		// Recover the Combat State, though we are still reloading, but the shotgun reload mechanism is special, it can be
		// interrupted by the fire button after reload an ammo, so we reset the combat state.
		BlasterCharacter->SetCombatState(ECombatState::ECS_Unoccupied);
		
		// For shotgun, change 1 ammo per reload.
		EquippedWeapon->SetAmmo(EquippedWeapon->GetAmmo() + 1);
		EquippedWeapon->SetHUDAmmo();
		SetCarriedAmmo(GetCarriedAmmo() - 1);
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = BlasterCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && BlasterCharacter->GetReloadMontage())
	{
		// Interrupt the animation right now.
		AnimInstance->Montage_JumpToSection(FName("EndShotgun"));
	}
}

void UCombatComponent::ThrowGrenadeAnimNotify()
{
	if (BlasterCharacter && BlasterCharacter->HasAuthority())
	{
		SetCombatState(ECombatState::ECS_Unoccupied);

		// AttachActor() executed on server has a multicast effect.
		AttachWeaponToRightHand();
	}
}

void UCombatComponent::LaunchGrenadeAnimNotify()
{
	// 1. Spawn on the server and replicate on all clients because grenade is a replicate.
	// 2. Besides, use a server RPC to transmit the local variable HitTarget.
	// 3. Last, the client calling server RPC must own the actor. If client2 execute client1's Server RPC, it will create error.
	if (BlasterCharacter && BlasterCharacter->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
	// Hide the grenade mesh on all machine.
	ShowGrenadeAttached(false);
}

void UCombatComponent::Reload()
{
	ServerReload();
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!BlasterCharacter || IsCarriedAmmoEmpty() || CombatState != ECombatState::ECS_Unoccupied ||
		!EquippedWeapon || EquippedWeapon->IsAmmoFull()) return;

	// CombatState Replication, we put reload logic in AnimMontage's Notify
	CombatState = ECombatState::ECS_Reloading;
	BlasterCharacter->PlayReloadMontage();
}

void UCombatComponent::ReloadAmmoAmount()
{
	if (!EquippedWeapon || !EquippedWeapon->IsAmmoValid()) return;

	// If the carried ammo is enough to full the clip after reloading.
	if (CarriedAmmo >= (EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo()))
	{
		// Sequence is important
		const int32 AmmoSupplied = EquippedWeapon->GetClipSize() - EquippedWeapon->GetAmmo();
		SetCarriedAmmo(CarriedAmmo - AmmoSupplied);
		EquippedWeapon->SetAmmo(EquippedWeapon->GetClipSize());
	}
	// If the carried ammo cannot full the clip after reloading.
	else
	{
		// Sequence is important
		EquippedWeapon->SetAmmo(EquippedWeapon->GetAmmo() + CarriedAmmo);
		SetCarriedAmmo(0);
	}
}

void UCombatComponent::ThrowGrenade()
{
	ServerThrowGrenade();
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (!BlasterCharacter || !BlasterCharacter->GetMesh()) return;
	
	// Spawn and launch grenade.
	if (const UStaticMeshComponent* GrenadeMesh = BlasterCharacter->GetGrenadeAttached())
	{
		const FVector Dir = Target - GrenadeMesh->GetComponentLocation();

		// The socket location is in the collision range of hand's capsule mesh, so we need to spawn a bit further from the
		// original location to avoid collision.
		constexpr float SafeDist = 50.f;
		const FVector SpawnLocation = GrenadeMesh->GetComponentLocation() + Dir.GetSafeNormal() * SafeDist;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = BlasterCharacter;
		SpawnParams.Instigator = BlasterCharacter;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(
				ProjectileClass,
				SpawnLocation,
				Dir.Rotation(),
				SpawnParams
			);
		}
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (!BlasterCharacter || CombatState != ECombatState::ECS_Unoccupied || IsGrenadeEmpty()) return;
	
	SetCombatState(ECombatState::ECS_Throwing);
	SetGrenadeAmount(Grenade - 1);

	// AttachActor() executed on server has a multicast effect.
	AttachWeaponToLeftHand();
	
	BlasterCharacter->PlayThrowGrenadeMontage();
	
}

void UCombatComponent::AttachWeaponToLeftHand()
{
	if (!BlasterCharacter || !BlasterCharacter->GetMesh() || !EquippedWeapon) return;
	
	if (const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("LeftHandSocket")))
	{
		// AttachActor() executed on server has a multicast effect, so no need to call it in OnRep_().
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
}

void UCombatComponent::AttachWeaponToRightHand()
{
	if (!BlasterCharacter || !BlasterCharacter->GetMesh() || !EquippedWeapon) return;
		
	if (const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		// AttachActor() executed on server has a multicast effect, so no need to call it in OnRep_().
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
}

void UCombatComponent::ShowGrenadeAttached(bool IsVisible)
{
	if (!BlasterCharacter || !BlasterCharacter->GetGrenadeAttached()) return;
	
	BlasterCharacter->GetGrenadeAttached()->SetVisibility(IsVisible);
}

void UCombatComponent::SetGrenadeAmount(int32 Amount)
{
	Grenade = FMath::Clamp(Amount, 0, MaxGrenade);

	HandleGrenadeRep();
}

void UCombatComponent::OnRep_Grenade()
{
	HandleGrenadeRep();
}

void UCombatComponent::HandleGrenadeRep()
{
	if (!BlasterCharacter || !BlasterCharacter->IsLocallyControlled()) return;
	
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(BlasterCharacter->GetController());
	if (BlasterPlayerController)
	{
		BlasterPlayerController->UpdateGrenade(Grenade);
	}
}

void UCombatComponent::OnRep_CombatState()
{
	HandleCombatState();
}

void UCombatComponent::HandleCombatState()
{
	if (!BlasterCharacter) return;

	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		break;
	case ECombatState::ECS_Reloading:
		BlasterCharacter->PlayReloadMontage();
		break;
	case ECombatState::ECS_Throwing:
		BlasterCharacter->PlayThrowGrenadeMontage();
		ShowGrenadeAttached(true);
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
