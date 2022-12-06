// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupHealth.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/BlasterCharacter.h"
#include "BlasterComponents/BuffComponent.h"

APickupHealth::APickupHealth()
{
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara Component"));
	NiagaraComponent->SetupAttachment(RootComponent);
}

void APickupHealth::Destroyed()
{
	Super::Destroyed();

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HealingEffect, GetActorLocation());
}

void APickupHealth::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Heal
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (UBuffComponent* Buff = BlasterCharacter->GetBuff())
		{
			Buff->Heal(HealingAmount, Duration);
		}
	}

	// Destroy
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}
