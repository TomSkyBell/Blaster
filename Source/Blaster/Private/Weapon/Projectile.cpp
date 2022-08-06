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
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
							Tracer,
							CollisionBox,
							FName(),
							GetActorLocation(),
							GetActorRotation(),
							EAttachLocation::KeepWorldPosition
							);
	}
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

