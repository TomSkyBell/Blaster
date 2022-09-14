// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ACasing::ACasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	
	// SimulationGeneratesHitEvents in blueprint. The property means that if the hit is cause by the physics simulation like
	// free fall rather than the external effect, then whether we turn the property on/off will results in the enable/disable
	// of the Hit event. If the hit is caused by the external effect, then it'll always be fired, unrelated with this property.
	// In this case, the bullet shell is impulsed and then free falls, if we turn off the property, then the collision between
	// the shell and the floor will not generate the OnHit event.
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectImpulse = 3.f;
	ShellEjectImpulseRange = FRotator(45.f, 45.f, 45.f);
}

// Called when the game starts or when spawned
void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	AddImpulseRandomly(ShellEjectImpulseRange);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (FallSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FallSound, GetActorLocation());
		CasingMesh->SetNotifyRigidBodyCollision(false);	// Stop firing the hit event cuz the sound will be played multiple times when the shell is rolling (physics simulation)
	}
}


// Called every frame
void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ACasing::AddImpulseRandomly(const FRotator& ImpulseRotatorRange) const
{
	FRotator ImpulseRotator;
	ImpulseRotator.Yaw = UKismetMathLibrary::RandomFloatInRange(-ImpulseRotatorRange.Yaw, ImpulseRotatorRange.Yaw);
	ImpulseRotator.Pitch = UKismetMathLibrary::RandomFloatInRange(-ImpulseRotatorRange.Pitch, ImpulseRotatorRange.Pitch);
	ImpulseRotator.Roll = UKismetMathLibrary::RandomFloatInRange(-ImpulseRotatorRange.Roll, ImpulseRotatorRange.Roll);

	CasingMesh->AddImpulse(ImpulseRotator.Vector() * ShellEjectImpulse);
}

