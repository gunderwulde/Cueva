// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionControllerPawn.h"
#include "MyMotionController.h"
#include "Engine/World.h"

// Sets default values
AMyMotionControllerPawn::AMyMotionControllerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyMotionControllerPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyMotionControllerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyMotionControllerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMyMotionControllerPawn::SpawnControllers(UClass* Class, USceneComponent* MyVROrigin ) {
	FVector Location(0.0f, 0.0f, 0.0f);
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	LeftController = (AMyMotionController*)GetWorld()->SpawnActor(Class, &Location, &Rotation);
	LeftController->SetHand(EControllerHand::Left);
	LeftController->SetOwner(this);
	LeftController->AttachToComponent(MyVROrigin, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false));


	RightController = (AMyMotionController*)GetWorld()->SpawnActor(Class, &Location, &Rotation);
	RightController->SetHand(EControllerHand::Right);
	RightController->SetOwner(this);
	RightController->AttachToComponent(MyVROrigin, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false) );
}

