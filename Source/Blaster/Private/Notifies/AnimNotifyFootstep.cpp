// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyFootstep.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"

// Optional Challenge: Crouch Footstep Sounds (Only the local player can hear)
//		I Only have one question that is not resolved -- If the PlaySound is replicated ? Cause when I didn't override the Notify function,
// the engine will execute the parent version of Notify, and the result is that the players can hear the footstep sound each other, seems like
// it has a replication effect, which means the TObject<USound> or maybe the notify is replicated, but how ? I search through the source code,
// didn't find anything related with replication.
//		Besides, if it is replicated. When we check by 'IsLocallyControlled()', if the ownership is the server, the server will do the replication
// work, which means the footstep sound will replicated to the clients, but the result is versus. 
void UAnimNotifyFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		APawn* PawnOwner = MeshComp->GetAnimInstance()->TryGetPawnOwner();
		if (const ACharacter* BlasterCharacter = Cast<ABlasterCharacter>(PawnOwner))
		{
			if(BlasterCharacter->IsLocallyControlled())
			{
				const FVector Location = MeshComp->GetSocketLocation(FName("Root"));
				UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), Sound, Location, FRotator::ZeroRotator);
			}
		}
	}
}
