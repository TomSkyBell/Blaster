// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& TraceHitTarget)
{
	// Common game logic (weapon functionality)
	// Call the code in the parent's function. It's something like clone the parent's code here.
	Super::Fire(TraceHitTarget);

	// Can't be tested on one machine, should be tested over the network.
	if (!HasAuthority()) return;
	
	// Respective game logic (weapon functionality)
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector ToTargetDirection = TraceHitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = ToTargetDirection.Rotation();
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			// Set weapon's owner in UCombatComponent::EquipWeapon(), and the owner is the character. Set Owner for later use.
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if (World)
			{
				// Spawn a child object (ProjectileClass* ) in the world and return the base object reference (AProjectile* )
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
					);
			}
		}
	}
}
