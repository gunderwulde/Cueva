// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionControllerPawn.h"
#include "MyMotionController.h"
#include "HeadMountedDisplay.h"
#include "Components/InputComponent.h"
#include "IXRTrackingSystem.h"
#include "IMotionController.h"
#include "Engine.h"
#include "Engine/World.h"

// Sets default values
AMyMotionControllerPawn::AMyMotionControllerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyMotionControllerPawn::BeginPlay() {
	auto components = this->GetComponents();
	for (auto component : components) {
		if (component->GetFName() == "VROrigin") {
			VROriginRef = Cast<USceneComponent>(component);
			break;
		}
	}

	Super::BeginPlay();
	SetupPlayerHeight();
	SpawnControllers();
}

// Called every frame
void AMyMotionControllerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	LeftController->SetTeleportRotation();
	RightController->SetTeleportRotation();

}

// Called to bind functionality to input
void AMyMotionControllerPawn::SetupPlayerInputComponent(UInputComponent* InputComponent) {
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAction<MyMotionControllerDelegate>("GrabLeft", IE_Pressed, this, &AMyMotionControllerPawn::OnGrab, &LeftController);
	InputComponent->BindAction<MyMotionControllerDelegate>("GrabLeft", IE_Released, this, &AMyMotionControllerPawn::OnRelease, &LeftController);
	InputComponent->BindAction<MyMotionControllerDelegate>("GrabRight", IE_Pressed, this, &AMyMotionControllerPawn::OnGrab, &RightController);
	InputComponent->BindAction<MyMotionControllerDelegate>("GrabRight", IE_Released, this, &AMyMotionControllerPawn::OnRelease, &RightController);

	InputComponent->BindAction<MyMotionControllerDelegate>("TeleportLeft", IE_Pressed, this, &AMyMotionControllerPawn::OnTeleportInit, &LeftController);
	InputComponent->BindAction<MyMotionControllerDelegate>("TeleportLeft", IE_Released, this, &AMyMotionControllerPawn::OnTeleportEnd, &LeftController);
	InputComponent->BindAction<MyMotionControllerDelegate>("TeleportRight", IE_Pressed, this, &AMyMotionControllerPawn::OnTeleportInit, &RightController);
	InputComponent->BindAction<MyMotionControllerDelegate>("TeleportRight", IE_Released, this, &AMyMotionControllerPawn::OnTeleportEnd, &RightController);

	InputComponent->BindAxis("MotionControllerThumbLeft_X", this, &AMyMotionControllerPawn::ThumbLeftX);
	InputComponent->BindAxis("MotionControllerThumbLeft_Y", this, &AMyMotionControllerPawn::ThumbLeftY);
	InputComponent->BindAxis("MotionControllerThumbRight_X", this, &AMyMotionControllerPawn::ThumbRightX);
	InputComponent->BindAxis("MotionControllerThumbRight_Y", this, &AMyMotionControllerPawn::ThumbRightY);

}

void AMyMotionControllerPawn::SpawnControllers() {
	LeftController = (AMyMotionController*)GetWorld()->SpawnActor(HandClass);
	LeftController->SetHand(EControllerHand::Left);
	LeftController->SetOwner(this);
	LeftController->AttachToComponent(VROriginRef, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false));

	RightController = (AMyMotionController*)GetWorld()->SpawnActor(HandClass);
	RightController->SetHand(EControllerHand::Right);
	RightController->SetOwner(this);
	RightController->AttachToComponent(VROriginRef, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false) );

	LeftController->other = RightController;
	RightController->other = LeftController;
}

void AMyMotionControllerPawn::OnGrab(AMyMotionController** controller) {
	if (controller != nullptr && (*controller) != nullptr)
		(*controller)->GrabActor();
}

void AMyMotionControllerPawn::OnRelease(AMyMotionController** controller) {
	if (controller != nullptr && (*controller) != nullptr)
		(*controller)->ReleaseActor();
}

void AMyMotionControllerPawn::OnTeleportInit(AMyMotionController** controller) {
	if (controller != nullptr && (*controller) != nullptr) {
		(*controller)->ActivateTeleporter();		
	}
}

void AMyMotionControllerPawn::OnTeleportEnd(AMyMotionController** controller) {
	if (controller != nullptr && (*controller) != nullptr) {
		(*controller)->ExecuteTeleportation2();
	}
}

void AMyMotionControllerPawn::ThumbLeftX(float amount) {
	LeftController->SetThumbX(amount);
}

void AMyMotionControllerPawn::ThumbLeftY(float amount) {
	LeftController->SetThumbY(amount);
}

void AMyMotionControllerPawn::ThumbRightX(float amount) {
	LeftController->SetThumbX(amount);
}

void AMyMotionControllerPawn::ThumbRightY(float amount) {
	LeftController->SetThumbY(amount);
}

void AMyMotionControllerPawn::SetupPlayerHeight() {
	FName name = "none";
	if ( GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice() && GEngine->XRSystem->GetHMDDevice()->IsHMDConnected()) {
		name = GEngine->XRSystem->GetSystemName();
	}
	if(name== "OculusHMD" || name=="SteamVR") { // Suelo
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Floor);

	}
}