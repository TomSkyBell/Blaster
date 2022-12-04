// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	PickupMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	PickupMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	
	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Pickup Collision"));
	PickupCollision->SetupAttachment(PickupMesh);
	PickupCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// Enable it on server.

	// Stencil -- Outline effect
	PickupMesh->SetRenderCustomDepth(true);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
	PickupWidget->SetupAttachment(RootComponent);
	PickupWidget->SetVisibility(false);
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickup::Destroyed()
{
	Super::Destroyed();

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundPickup,
		GetActorLocation()
	);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereBeginOverlap);
		PickupCollision->OnComponentEndOverlap.AddDynamic(this, &APickup::OnSphereEndOverlap);
	}
	// Synchronize the movement when we drop the pick up or the pick up is push by other force.
	// Virtual function better not be called in ctor.
	SetReplicateMovement(true);
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void APickup::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

