// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Blaster.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_SKELETAL_MESH, ECollisionResponse::ECR_Block);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the particle effect when fire.
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
			);
	}
	if (TracerSound)
	{
		TracerSoundComponent = UGameplayStatics::SpawnSoundAttached(
			TracerSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			true,
			TracerSound->GetVolumeMultiplier(),
			TracerSound->GetPitchMultiplier(),
			0.f,
			TracerSound->AttenuationSettings,
			(USoundConcurrency*)nullptr
		);
	}
	CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);

	// This implementation works fine here, but Stephen said sometimes it fails.
	// CollisionBox->IgnoreActorWhenMoving(GetOwner(), true);
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

// OnHit is executed both on the server and clients.
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
	if (bOnHitDestroy)
	{
		Destroy();
	}
	else
	{
		// Call Destroy() after a period of time.
		GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ThisClass::DestroyTimerFinished, DestroyDelay, false);
	}
}

void AProjectile::SpawnTrailSystem()
{
	// Spawn a niagara system component for control the niagara system.
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false);
	}
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	// Multicast the hit effect and hit sound.
	HandleHitImpact();
}

void AProjectile::HandleHitImpact()
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
	
	// Since we manually call the destroy() after timer finished, so we need to hide the mesh and disable the collision first.
	if (ProjectileMesh) ProjectileMesh->SetVisibility(false);
	if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Deactivate the tracer and tracer sound.
	if (TracerComponent) TracerComponent->Deactivate();
	if (TracerSoundComponent) TracerSoundComponent->Deactivate();
}

