// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyReload.h"
#include "Character/BlasterCharacter.h"

void UAnimNotifyReload::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		APawn* PawnOwner = MeshComp->GetAnimInstance()->TryGetPawnOwner();
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PawnOwner))
		{
			if (BlasterCharacter->HasAuthority())
			{
				// For clients, they do their ServerRPC in OnRep_()
				BlasterCharacter->SetCombatState(ECombatState::ECS_Unoccupied);

				// For the server itself, it solely do a ServerRPC.
				if (BlasterCharacter->IsLocallyControlled()) BlasterCharacter->Fire();
			}
		}
	}
}
