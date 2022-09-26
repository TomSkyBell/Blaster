// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/BlasterPlayerController.h"

// OnHit is executed from the server, OnHit has been bound to delegate by HasAuthority() check, so no need to recheck internal.
void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// The Owner/Instigator is set in SpawnParams when we spawn the projectile
	const APawn* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator) return;
	
	UGameplayStatics::ApplyDamage(OtherActor, Damage, ProjectileInstigator->GetController(), this, UDamageType::StaticClass());

	// Destroy() will be called, so Super::OnHit should be called at last.
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

