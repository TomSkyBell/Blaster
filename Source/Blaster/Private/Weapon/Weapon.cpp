// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
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

	// Collision Preset --- Block all the other things except the pawn (so we can step over it once drop the weapon)
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
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
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
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
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	WeaponState_RepNotify();
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

void AWeapon::WeaponState_RepNotify()
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
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
		
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
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
	WeaponState_RepNotify();
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
