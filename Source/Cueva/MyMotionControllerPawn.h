// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyMotionControllerPawn.generated.h"

UCLASS()
class CUEVA_API AMyMotionControllerPawn : public APawn
{
	GENERATED_BODY()

	class USceneComponent* VROriginRef;
	class AMyMotionController *LeftController;
	class AMyMotionController *RightController;

	DECLARE_DELEGATE_OneParam(MyMotionControllerDelegate, AMyMotionController**);
	void OnGrab(class AMyMotionController** controller);
	void OnRelease(class AMyMotionController** controller);
	void OnTeleportInit(class AMyMotionController** controller);
	void OnTeleportEnd(class AMyMotionController** controller);

	void ThumbLeftX(float amount);
	void ThumbLeftY(float amount);

	void ThumbRightX(float amount);
	void ThumbRightY(float amount);

	void SetupPlayerHeight();
	void SpawnControllers();

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
	
	UPROPERTY(EditDefaultsOnly, Category = "MotionController")
		class UClass* HandClass;
};
