// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "BlasterComponents/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	// All the machines can see the weapon, only the Server has the authority.
	// If bReplicates = false, all the machines have authority.
	bReplicates = true;
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	SetRootComponent(WeaponMesh);

	// Collision Preset --- Block all the other things except the pawn (so we can step over it once drop the weapon),
	// and the camera to avoid zooming effect.
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Area Sphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// Enable it when we on the server machine

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Weapon Widget"));
	PickupWidget->SetupAttachment(RootComponent);
	PickupWidget->SetVisibility(false);

	CrosshairsMaxSpread = 64.f;
	CrosshairsMinSpread = 5.f;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Only the server has the authority to handle the collision event, so on each client will not see the effect of collision
	// unless the collision is executed on the server.
	if (HasAuthority())
	{
		// Overlap events are only generated on the server.
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}
	// 1. There is a risk when we call virtual function in a constructor, so we call here instead.
	// 2. We should set it to true when we drop the weapon and false when we equip the weapon, because when we hold the weapon, the
	// character movement is replicated, and the weapon is attached to the character, so it's position can be replicated.
	// 3. The reason why we need to replicate the movement is that, the weapon is set physics simulated, once it falls or push by
	// other force, the simulation will work and if the movement is not replicated, the weapon 'll be show up at different position on clients and server.
	SetReplicateMovement(true);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	
	// Weapon Ammo is correlated with the HUD Ammo, HUD can only be updated on the owning client, so we should declare
	// the Ammo as COND_OwnerOnly except that the Ammo need shared among the clients.
	DOREPLIFETIME_CONDITION(AWeapon, Ammo, COND_OwnerOnly);
}


void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& TraceHitTarget)
{
	// This is common logic, feature can be added by override.
	if (FireAnimation && WeaponMesh)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	SpendRound();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	HandleWeaponState();
}

void AWeapon::Dropped()
{
	// Drop is executed from the server.
	// Drop is a rep + multicast process, SetWeaponState() is a replication work in which we can see its source code.
	// DetachFromActor() is like AttachActor(), which is a multicast process, so as the SetOwner(). So we don't need
	// to set a multicast RPC for the Dropped().
	SetWeaponState(EWeaponState::EWS_Dropped);
	const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(DetachmentTransformRules);
	SetOwner(nullptr);
	ResetOwnership();
}

void AWeapon::SpendRound()
{
	Ammo -= 1;
	
	SetHUDAmmo();
	
}

void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();

	WeaponOwnerCharacter = WeaponOwnerCharacter ? WeaponOwnerCharacter : Cast<ABlasterCharacter>(GetOwner());
	
	// Jump to the end section of animation when the clip is full when reloading the shotgun.
	if (WeaponOwnerCharacter->GetCombat() && WeaponType == EWeaponType::EWT_Shotgun && IsAmmoFull())
	{
		WeaponOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
}

void AWeapon::SetHUDAmmo()
{
	// We need to make sure the owner exists when we update the HUD, that's reason why we don't choose to put the logic
	// into the OnRep_WeaponState(), because the replication conflict between WeaponState and Owner, which we don't know
	// which replication is faster. (If weapon state is faster, then the owner is not available.)
	WeaponOwnerCharacter = WeaponOwnerCharacter ? WeaponOwnerCharacter : Cast<ABlasterCharacter>(GetOwner());
	if (!WeaponOwnerCharacter) return;

	WeaponOwnerController = WeaponOwnerController ? WeaponOwnerController : Cast<ABlasterPlayerController>(WeaponOwnerCharacter->Controller);
	if (!WeaponOwnerController) return;
	
	WeaponOwnerController->UpdateWeaponAmmo(Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	// Recover the ownership, otherwise if a player drops the weapon and another player picks up the weapon, it will
	// update the other player's HUD. 
	if (GetOwner() == nullptr) ResetOwnership();
	else SetHUDAmmo();
}

void AWeapon::ResetOwnership()
{
	WeaponOwnerCharacter = nullptr;
	WeaponOwnerController = nullptr;
}

void AWeapon::HandleWeaponState()
{
	// Change the weapon's pickup widget in Server World to be invisible 
	switch(WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		
		// SetWeaponState() is not always executed from the server, so we need to guarantee it on the server.
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		// When we equip the weapon, we don't have to replicate the movement because the weapon is attached to the character and the character movement is replicated.
		SetReplicateMovement(false);

		// If weapon type is SMG, we just disable the physics simulation of the weapon mesh because physics simulation is incompatible with attaching actor behavior.
		// But it doesn't affect the strap physics simulation in Weapon Mesh-->Physics Asset-->Physics Type in blueprint.
		if (WeaponType == EWeaponType::EWT_SMG)
		{
			WeaponMesh->SetSimulatePhysics(false);
		}
		else
		{
			// Set physics simulation, be aware of the sequence.
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		break;
		
	case EWeaponState::EWS_Dropped:
		// SetWeaponState() is not always executed from the server, so we need to guarantee it on the server.
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		// When we drop the weapon, the weapon's movement is effected by the physics simulation like fall, push by a force.etc, so we need to replicate movement again.
		SetReplicateMovement(true);
		
		// Set physics simulation, be aware of the sequence.
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;

	case EWeaponState::EWS_Initial:
		break;
		
	case EWeaponState::EWS_Max:
		break;
	}
}

// Change the weapon's pickup widget in clients' world to be invisible
void AWeapon::OnRep_WeaponState()
{
	HandleWeaponState();
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(this); 
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr); 
	}
}
