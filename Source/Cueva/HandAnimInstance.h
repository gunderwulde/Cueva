// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HandAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class CUEVA_API UHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int GripState2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Grip;

};
