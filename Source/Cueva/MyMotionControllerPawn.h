// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyMotionControllerPawn.generated.h"

UCLASS()
class CUEVA_API AMyMotionControllerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMyMotionControllerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AMyMotionController *LeftController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AMyMotionController *RightController;


	UFUNCTION(BlueprintCallable, Category = "MyMotionControllerPawn")
		void SpawnControllers(UClass* Class, USceneComponent* MyVROrigin);
	
};
