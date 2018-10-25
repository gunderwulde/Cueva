// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionControllerPawn.h"
#include "MyMotionController.h"
#include "Components/InputComponent.h"
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
	auto components = this->GetComponents();
	for (auto component : components) {
		if (component->GetFName() == "VROrigin") {
			VROriginRef = Cast<USceneComponent>(component);
			break;
		}
	}

	Super::BeginPlay();
	SetupPlayerHeight();
}

// Called every frame
void AMyMotionControllerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyMotionControllerPawn::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAction<MyMotionControllerDelegate>("GrabLeft", IE_Pressed, this, &AMyMotionControllerPawn::OnGrab, &LeftController);
	InputComponent->BindAction<MyMotionControllerDelegate>("GrabLeft", IE_Released, this, &AMyMotionControllerPawn::OnRelease, &LeftController);

	InputComponent->BindAction<MyMotionControllerDelegate>("GrabRight", IE_Pressed, this, &AMyMotionControllerPawn::OnGrab, &RightController );
	InputComponent->BindAction<MyMotionControllerDelegate>("GrabRight", IE_Released, this, &AMyMotionControllerPawn::OnRelease, &RightController );
}

void AMyMotionControllerPawn::SpawnControllers(UClass* Class) {

	LeftController = (AMyMotionController*)GetWorld()->SpawnActor(Class);
	LeftController->SetHand(EControllerHand::Left);
	LeftController->SetOwner(this);
	LeftController->AttachToComponent(VROriginRef, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false));


	RightController = (AMyMotionController*)GetWorld()->SpawnActor(Class);
	RightController->SetHand(EControllerHand::Right);
	RightController->SetOwner(this);
	RightController->AttachToComponent(VROriginRef, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false) );
}

void AMyMotionControllerPawn::OnGrab(AMyMotionController** controller) {
	if (controller != nullptr && (*controller) != nullptr)
		(*controller)->GrabActor();
}

void AMyMotionControllerPawn::OnRelease(AMyMotionController** controller) {
	if(controller!=nullptr && (*controller) != nullptr)
		(*controller)->ReleaseActor();
}


void AMyMotionControllerPawn::SetupPlayerHeight() {
}