// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
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
	CollisionBox->SetCollisionObjectType(ECC_Projectile);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Projectile, ECollisionResponse::ECR_Ignore);

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

// We gonna make the OnHit function a server-controlled function, just like overlap function. See, on the client end, once OnHit triggered,
// we cannot see the server's playing montage cuz the client cannot control the server to play montage. So why don't we directly put the
// logic on the server and use a multi-RPC to wait for the multicast result from the server.
void AProjectile::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
	)
{
	// Hit impact.
	HandleHitImpact(OtherActor);
	
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

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
}

void AProjectile::HandleHitImpact(AActor* OtherActor)
{
	if (Cast<ABlasterCharacter>(OtherActor))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffectForPawn, GetActorLocation());
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSoundForPawn, GetActorLocation());
	}
	else
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffectForStone, GetActorLocation());
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSoundForStone, GetActorLocation());
	}
	
	// Since we manually call the destroy() after timer finished, so we need to hide the mesh and disable the collision first.
	if (ProjectileMesh) ProjectileMesh->SetVisibility(false);
	if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Deactivate the tracer and tracer sound.
	if (TracerComponent) TracerComponent->Deactivate();
	if (TracerSoundComponent) TracerSoundComponent->Deactivate();
}

