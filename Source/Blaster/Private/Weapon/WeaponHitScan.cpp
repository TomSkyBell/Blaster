// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponHitScan.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"

void AWeaponHitScan::Fire(const FVector& TraceHitTarget)
{
	Super::Fire(TraceHitTarget);

	FireBeam(TraceHitTarget);
}

void AWeaponHitScan::FireBeam(const FVector& TraceHitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		const FVector Start = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
		const FVector Direction = TraceHitTarget - Start;
		const FVector End = Start + Direction * 1.25f;	// Why only 1.25f here because the hit target's location is actually know, we just want to know who is it.

		// Actually we already know the hit target's location, but we still need to know who it is.
		FHitResult HitResult;
		if (GetWorld())
		{
			GetWorld()->LineTraceSingleByChannel(
				HitResult,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);
		}
		if (!HitResult.bBlockingHit) return;

		// Spawn emitter both on the server and clients.
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		if (InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			
			UGameplayStatics::SpawnEmitterAtLocation(
				this,
				HitParticle,
				TraceHitTarget
			);
		}
		// We only want the Damage Process be executed on the server.
		if (HasAuthority())
		{
			UGameplayStatics::ApplyDamage(
				HitResult.GetActor(),
				Damage,
				InstigatorPawn->GetController(),
				this,
				UDamageType::StaticClass());
		}
	}
}
