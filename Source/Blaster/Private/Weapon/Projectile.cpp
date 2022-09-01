// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Blaster.h"
#include "Net/UnrealNetwork.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	// ProjectileMovementComponent updates the position of another component during its tick.
	// If not SetUpdatedComponent(), then automatically set the root component as the updated component.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 3000.f;
	ProjectileMovementComponent->MaxSpeed = 3500.f;
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
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		// NetMulticast the hit react montage.
		BlasterCharacter->PlayHitReactMontage();
	}
	// Multicast the hit impact.
	MulticastHitImpact(OtherActor);
	
	// Destroy() works something like a multicast function, it will propagate to the clients and clients do the same work, knowing what happened.
	// Compared to MulticastRPC, this way can lower the bandwidth.
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void AProjectile::MulticastHitImpact_Implementation(AActor* OtherActor)
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
}

