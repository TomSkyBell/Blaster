// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Bullet Movement"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = 15000.f;
	ProjectileMovementComponent->MaxSpeed = 15000.f;
}

// OnHit is executed from the server, OnHit has been bound to delegate by HasAuthority() check, so no need to recheck internal.
void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// The Owner/Instigator is set in SpawnParams when we spawn the projectile
	const APawn* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator) return;

	// We only want the Damage Process be executed on the server.
	if (HasAuthority())
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, ProjectileInstigator->GetController(), this, UDamageType::StaticClass());
	}
	
	// Destroy() will be called, so Super::OnHit should be called at last.
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

