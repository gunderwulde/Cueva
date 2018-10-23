// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NewPickupActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNewPickupActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CUEVA_API INewPickupActorInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(Category = "NewPickupActorInterface", BlueprintNativeEvent, BlueprintCallable)
	void Pickup(class USceneComponent* controller);
	UFUNCTION(Category = "NewPickupActorInterface", BlueprintNativeEvent, BlueprintCallable)
	void Drop();
};
