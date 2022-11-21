// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/BlasterProjectileMovementComponent.h"

AProjectileGrenade::AProjectileGrenade()
{
	// Customize the projectile movement component.
	BlasterProjectileMovementComponent = CreateDefaultSubobject<UBlasterProjectileMovementComponent>(TEXT("Grenade Movement Component"));
	BlasterProjectileMovementComponent->bRotationFollowsVelocity = true;
	BlasterProjectileMovementComponent->SetIsReplicated(true);
	BlasterProjectileMovementComponent->bShouldBounce = true;
	BlasterProjectileMovementComponent->InitialSpeed = 2000.f;
	BlasterProjectileMovementComponent->MaxSpeed = 2000.f;
	BlasterProjectileMovementComponent->ProjectileGravityScale = 0.5f;

	// We don't want the grenade destroyed immediately after it's hit, we will call the destroy manually after the timer finished.
	bOnHitDestroy = false;	
}

void AProjectileGrenade::BeginPlay()
{
	// The grenade launcher doesn't have a tracer sound and its tracer is not made by particle system but a niagara system,
	// so we don't need to call Super::BeginPlay().
	// And most importantly, we don't want to bind the OnHit delegate in parent class because grenade has a bounce feature, not hit.
	AActor::BeginPlay();

	// Spawn a niagara system component for control the niagara system.
	SpawnTrailSystem();
	
	// We should bind to a OnProjectileBounce delegate.
	BlasterProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);

	// Once the projectile is spawned and shot out, we start the destroy timer.
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ThisClass::DestroyTimerFinished, DestroyDelay, false);
}

void AProjectileGrenade::DestroyTimerFinished()
{
	// Play particle effect
	UGameplayStatics::SpawnEmitterAtLocation(
		this,
		ExplodeEffect,
		GetActorLocation(),
		GetActorRotation()
	);

	// Play sound effect
	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ExplodeSound,
			GetActorLocation(),
			GetActorRotation(),
			ExplodeSound->GetVolumeMultiplier(),
			ExplodeSound->GetPitchMultiplier(),
			0.f,
			ExplodeSound->AttenuationSettings
		);
	}

	// ApplyDamage logic can only be executed on the server.
	if (HasAuthority())
	{
		if (const APawn* ProjectileInstigator = GetInstigator())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				DamageFalloff,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				ProjectileInstigator->Controller
			);
		}
	}

	// Destroy the projectile and the niagara system attached to it will also be destroyed.
	Destroy();
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
		this,
		BounceSound,
		GetActorLocation(),
		GetActorRotation(),
		BounceSound->GetVolumeMultiplier(),
		BounceSound->GetPitchMultiplier(),
		0.f,
		BounceSound->AttenuationSettings
		);
	}
}
