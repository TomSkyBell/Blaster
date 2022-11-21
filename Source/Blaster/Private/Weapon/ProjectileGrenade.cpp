// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/BlasterProjectileMovementComponent.h"

AProjectileGrenade::AProjectileGrenade()
{
	// The reason why we use the projectile movement component here rather than the customized blaster projectile movement component
	// is that the bounce feature is not compatible with the customized one.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Grenade Movement Component"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->InitialSpeed = 2000.f;
	ProjectileMovementComponent->MaxSpeed = 2000.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.5f;

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
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);

	// Once the projectile is spawned and shot out, we start the destroy timer.
	// Be aware that once the projectile is destroyed on the server (server maybe quicker) or a client, the other users' projectile have
	// already been destroyed because the projectile is a replicant, so the result is the functionality before Destroy() not implemented
	// in time, that's why we need to move the logic into Destroyed() to multicast the functionality.
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ThisClass::DestroyTimerFinished, DestroyDelay, false);
}

void AProjectileGrenade::Destroyed()
{
	Super::Destroyed();

	// Multicast the functionality.
	ExplodeDamage();
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

void AProjectileGrenade::ExplodeDamage()
{
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
}
