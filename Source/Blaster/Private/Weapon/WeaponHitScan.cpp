// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponHitScan.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Weapon/WeaponType.h"

void AWeaponHitScan::Fire(const FVector& TraceHitTarget)
{
	Super::Fire(TraceHitTarget);

	FireHitScan(TraceHitTarget);
}

void AWeaponHitScan::FireHitScan(const FVector& TraceHitTarget)
{
	// A map to store the pair of <HitResult.GetActor(), TotalHitNums> so that we can do the ApplyDamage() once for each actor rather than loop it.
	TMap<AActor*, float> DamageForEachActor;
	
	const FVector& Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	const FVector& Dir = TraceHitTarget - Start;
	
	// Scatter multiple lines when firing. For-loop each line.
	for (uint32 i = 0; i < ScatterNum; i++)
	{
		// Randomize the shooting direction for the scatter effect. ScatterDir is a normalized vector.
		const FVector& ScatterDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(Dir, ScatterAngle);
		const FVector& End = Start + ScatterDir * TRACE_LENGTH;
		HitScan(DamageForEachActor, Start, End);
	}
	// We only want the Damage Process be executed on the server.
	if (HasAuthority())
	{
		// Loop the map and apply the total damage for each actor.
		for (const auto& Pair: DamageForEachActor)
		{
			if (const APawn* InstigatorPawn = Cast<APawn>(GetOwner()))
			{
				UGameplayStatics::ApplyDamage(
					Pair.Key,
					Pair.Value,
					InstigatorPawn->GetController(),
					this,
					UDamageType::StaticClass());
			}
		}
		
	}
}

void AWeaponHitScan::HitScan(TMap<AActor*, float>& DamageForEachActor, const FVector& Start, const FVector& End)
{
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
		HitResult.ImpactPoint
	);

	// Spawn beam particle both on server and clients and set the parameter (end location) of the particle emitter (particle system component).
	if (UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
		this,
		BeamParticle,
		Start
	))
	{
		Beam->SetVectorParameter(FName("Target"), HitResult.ImpactPoint);
	}

	// Insert the 'damage for each actor' pair into the map.
	if (DamageForEachActor.Contains(HitResult.GetActor()))
	{
		DamageForEachActor[HitResult.GetActor()] += Damage;
	}
	else
	{
		DamageForEachActor.Emplace(HitResult.GetActor(), Damage);
	}
}
