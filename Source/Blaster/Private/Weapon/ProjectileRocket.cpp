// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Weapon/BlasterProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	// Create a new customized projectile movement component.
	RocketMovementComponent = CreateDefaultSubobject<UBlasterProjectileMovementComponent>(TEXT("Bullet Movement"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->InitialSpeed = 2000.f;
	RocketMovementComponent->MaxSpeed = 2000.f;
	RocketMovementComponent->ProjectileGravityScale = 0.f;
	
	// We don't want the rocket destroyed immediately after it's hit, so we need to call the destroy manually after the timer finished.
	bOnHitDestroy = false;	
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

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

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// The Owner/Instigator is set in SpawnParams when we spawn the projectile.
	const APawn* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator) return;

	// If we hit ourselves, it'll not trigger the HitImpact.
	if (OtherActor == GetOwner()) return;

	// We only want the Damage Process be executed on the server.
	if (HasAuthority())
	{
		UGameplayStatics::ApplyRadialDamageWithFalloff(this, Damage, MinDamage, GetActorLocation(), DamageInnerRadius, DamageOuterRadius, DamageFalloff,
		UDamageType::StaticClass(), TArray<AActor*>(), this, ProjectileInstigator->GetController());
	}

	// We use the niagara system instead of particle system in the parent, so we need to override part of the functionality.
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	
	// Super::OnHit be called, but destroy need to be manually called after the timer finished.
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
