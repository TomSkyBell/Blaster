// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyReload.h"
#include "BlasterComponents/CombatComponent.h"
#include "Character/BlasterCharacter.h"

void UAnimNotifyReload::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		APawn* PawnOwner = MeshComp->GetAnimInstance()->TryGetPawnOwner();
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PawnOwner);
		if (BlasterCharacter && BlasterCharacter->GetCombat())
		{
			BlasterCharacter->GetCombat()->ReloadAnimNotify();
		}
	}
}
