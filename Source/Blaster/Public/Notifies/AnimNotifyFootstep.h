// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "AnimNotifyFootstep.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UAnimNotifyFootstep : public UAnimNotify_PlaySound
{
	GENERATED_BODY()

	// If you don't override this function, unreal engine will do this notify for you in the animation sequence.
	// The Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) is deprecated.
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
