// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 150.f;
	BaseCrouchWalkSpeed = 300.f;
	AimCrouchWalkSpeed = 150.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BaseCrouchWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}


// This function is executed on the server.
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!BlasterCharacter || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		// Automatically propagated to the clients
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	// Set the owner of this Actor, used primarily for network replication. 
	EquippedWeapon->SetOwner(BlasterCharacter);

	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// If the invoker is self, then we'd better do the work in 'Set Aiming' right away rather than waiting for
	// the 'ServerSetAiming' because ServerSetAiming is the RPC which needs time to transfer the replication
	// from the server to the client. The main effect of the 'ServerSetAiming' is to notify the other clients
	// of the change of the invoker's replicated variable, not to notify the invoker itself (though it can).
	bAiming = bIsAiming;
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = bIsAiming ? AimCrouchWalkSpeed : BaseCrouchWalkSpeed;
	}
	ServerSetAiming(bIsAiming);
}

// This function is executed on the server.
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

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (BlasterCharacter)
	{
		BlasterCharacter->PlayFireMontage(bAiming);
	}
}



