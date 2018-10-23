// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "NewPickupActorInterface.h"
#include "CoreMinimal.h"
#include "MyStaticMeshActor.generated.h"

/**
 * 
 */
UCLASS()
class CUEVA_API AMyStaticMeshActor : public AStaticMeshActor, public INewPickupActorInterface
{
	GENERATED_BODY()
	
	AMyStaticMeshActor();
public:
	UFUNCTION(Category = "NewPickupActorInterface", BlueprintNativeEvent, BlueprintCallable)
	void Pickup(class USceneComponent* controller);
	virtual void Pickup_Implementation(class USceneComponent* controller) override;

	UFUNCTION(Category = "NewPickupActorInterface", BlueprintNativeEvent, BlueprintCallable)
	void Drop();
	virtual void Drop_Implementation() override;

	
	
};
