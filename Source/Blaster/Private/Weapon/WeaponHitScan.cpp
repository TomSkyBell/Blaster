// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponHitScan.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"

void AWeaponHitScan::Fire(const FVector& TraceHitTarget)
{
	Super::Fire(TraceHitTarget);

	if (bUseScatter)
	{
		FireHitScanScatter(TraceHitTarget);
	}
	else
	{
		FireHitScan(TraceHitTarget);
	}
}

void AWeaponHitScan::FireHitScanScatter(const FVector& TraceHitTarget)
{
	const FVector& Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	const FVector& Dir = TraceHitTarget - Start;
	
	// Scatter multiple lines when firing. For-loop each line.
	for (uint32 i = 0; i < ScatterNum; i++)
	{
		// Randomize the shooting direction for the scatter effect.
		const FVector& ScatterDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(Dir, ScatterAngle);
		
		// Scatter weapon has a distance limit -- ScatterDist
		const FVector& End = Start + ScatterDir * ScatterDist;
		HitScan(Start, End);
	}
}

void AWeaponHitScan::FireHitScan(const FVector& TraceHitTarget)
{
	const FVector& Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	const FVector Direction = TraceHitTarget - Start;

	// 1.25f is used to make sure the line pierce through the HitActor. If it's 1.f, then End == TraceHitTarget.
	const FVector End = Start + Direction * 1.25f;
	HitScan(Start, End);
}

void AWeaponHitScan::HitScan(const FVector& Start, const FVector& End)
{
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

	// We only want the Damage Process be executed on the server.
	if (HasAuthority())
	{
		if (const APawn* InstigatorPawn = Cast<APawn>(GetOwner()))
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
