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


// This function is invoked from the server.
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

// RPC executed on the server.
// If a client_A is doing the ServerRPC, it means the client itself locally knows what it's doing, and through the ServerRPC, it can make
// the server know what it's doing, so the result is that the server's screen can display what the client is doing, but the other clients
// do not know what client_A is doing. For this instance, bAiming is a replicated variable, so the other clients do know that client_A's
// bAiming has been changed, but as for what else client_A is doing can't be known. So this is the shortcoming of the ServerRPC which info
// cannot be shared between clients.
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

// RPC invoked from the server
void UCombatComponent::ServerFire_Implementation(bool bPressed)
{
	// Multicast is invoked from the server, so the function will run on the server and all other clients.
	MulticastFire(bPressed);
}

// If this function is invoked from the client, then it will only run on the client which makes no sense.
// If this function is invoked from the server, then it will run on the server and all the clients.
void UCombatComponent::MulticastFire_Implementation(bool bPressed)
{
	bFireButtonPressed = bPressed;
	
	if (!EquippedWeapon) return;
	if (BlasterCharacter && bFireButtonPressed)
	{
		BlasterCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	// Everytime we call the RPC, the data will be sent across the network. And with multiplayer game, the less data we sent, the better.
	// It's only for things which are very important in the game such as shooting will need RPC.
	bFireButtonPressed = bPressed;
	
	ServerFire(bPressed);
}

