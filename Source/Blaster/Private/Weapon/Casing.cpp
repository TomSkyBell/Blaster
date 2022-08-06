// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"

// Sets default values
ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// We don't need to set bReplicates = true, because we spawn it in a multicast way so the machine can locally spawn and see it.
	// bReplicates = true;
	
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	
	// SimulationGeneratesHitEvents in blueprint. The property means that if the hit is cause by the physics simulation like
	// free fall. In this case, the bullet shell is impulsed and then free falls, if we turn off the property, then the
	// collision between the shell and the floor will not generate the OnHit event.
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectImpulse = 3.f;
}

// Called when the game starts or when spawned
void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectImpulse);
}

// Called every frame
void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

