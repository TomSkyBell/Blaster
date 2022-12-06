// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	
	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Pickup Collision"));
	PickupCollision->SetupAttachment(RootComponent);
	PickupCollision->SetWorldScale3D(FVector(5.f, 5.f, 5.f));
	PickupCollision->AddWorldOffset(FVector(0.f, 0.f, 85.f));
	PickupCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	PickupMesh->SetupAttachment(PickupCollision);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Stencil -- Outline effect
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(DEPTH_BLUE);
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
		PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereBeginOverlap);
		SetReplicateMovement(true);
		GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &APickup::Turn, TurnTimerRate, true);
	}
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ABlasterCharacter>(OtherActor)) Destroy();
}

void APickup::Turn()
{
	AddActorWorldRotation(FRotator(0.f, BaseTurnRate * TurnTimerRate, 0.f));
}
