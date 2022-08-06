// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// The detailed config can be set in blueprint.
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	SetRootComponent(CollisionBox);

	// ProjectileMovementComponent updates the position of another component during its tick.
	// If not SetUpdatedComponent(), then automatically set the root component as the updated component.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the particle effect when fire.
	if (Tracer)
	{
		UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
			);
	}
	
	// There are 2 ways to implement:
	// 1. We are not checking the 'HasAuthority()', and then for the reason that the projectile is spawned through the
	// MulticastRPC, so every Client and the Server can do the OnHit() on their own machine.
	
	// 2. We check the 'HasAuthority()' to let the OnHit only work on the Server machine, and place the OnHit content
	// like PlaySound, SpawnEmitter in a Destroy() function, which something works like a MulticastRPC, it can propagate to
	// the clients so that all the clients can do the OnHit content on their own machine.
	
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

void AProjectile::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
	)
{
	// Destroy() works something like a multicast function, it will propagate to the clients and clients do the same work, knowing what happened.
	// Compared to MulticastRPC, this way can lower the bandwidth.
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	
	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, HitEffect, GetActorLocation());
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}
}


void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

