// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponHitScan.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void AWeaponHitScan::Fire(const FVector& TraceHitTarget)
{
	Super::Fire(TraceHitTarget);

	FireHitScan(TraceHitTarget);
}

void AWeaponHitScan::FireHitScan(const FVector& TraceHitTarget)
{
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash")))
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
		
		// Spawn hit particle both on server and clients.
		UGameplayStatics::SpawnEmitterAtLocation(
			this,
			HitParticle,
			TraceHitTarget
		);

		// Spawn beam particle both on server and clients and set the parameter (end location) of the particle emitter (particle system component).
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
			this,
			BeamParticle,
			Start
		);
		if (Beam) Beam->SetVectorParameter(FName("Target"), TraceHitTarget);

		// We only want the Damage Process be executed on the server.
		if (HasAuthority())
		{
			APawn* InstigatorPawn = Cast<APawn>(GetOwner());
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			if (InstigatorPawn)
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
}
