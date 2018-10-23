// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionController.h"
#include "Public/MotionControllerComponent.h"
#include "NewPickupActorInterface.h"
#include "Components/SphereComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Classes/Kismet/GameplayStatics.h"


// Sets default values
AMyMotionController::AMyMotionController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


}

// Called when the game starts or when spawned
void AMyMotionController::BeginPlay()
{
	Super::BeginPlay();
	SphereCollider = this->FindComponentByClass<USphereComponent>();
	motionController2 = this->FindComponentByClass<UMotionControllerComponent>();
}

void AMyMotionController::SetHand(EControllerHand hand) {
	Hand = hand;
	if (Hand == EControllerHand::Left) {
		FindComponentByClass<USkinnedMeshComponent>()->SetWorldScale3D(FVector(1, 1, -1)); // La mano izquierda va espejada.
		motionController2->MotionSource = _TEXT("Left");
	}
	else motionController2->MotionSource = _TEXT("Right");

}

// Called every frame
void AMyMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyMotionController::GrabActor2() {
	WantsToGrip2 = true;
	AActor * tmp = GetActorNearHand2();
	if (tmp != nullptr) {
		attachedActor = tmp;
		INewPickupActorInterface::Execute_Pickup(attachedActor, motionController2);
		RumbleController2(0.7f);
	}
}

void AMyMotionController::ReleaseActor2() {
	WantsToGrip2 = false;
	if (attachedActor != nullptr) {
		INewPickupActorInterface::Execute_Drop(attachedActor);
		RumbleController2(0.2f);
		attachedActor = nullptr;
	}
}

AActor* AMyMotionController::GetActorNearHand2() {
	float nearestOverlap = 100000.0f;
	AActor* nearestOverlapActor = nullptr;
	TArray<AActor*> actors;
	SphereCollider->GetOverlappingActors(actors);
	FVector spherePosition = SphereCollider->GetComponentLocation();
	for (int32 i = 0; i != actors.Num(); ++i) {
		INewPickupActorInterface* pickable = Cast<INewPickupActorInterface>(actors[i]);
		if (pickable) {
			FVector dif = actors[i]->GetActorLocation() - spherePosition;
			float size = dif.Size();
			if (size < nearestOverlap) {
				nearestOverlap = size;
				nearestOverlapActor = actors[i];
			}
		}
	}
	return nearestOverlapActor;
}

void AMyMotionController::RumbleController2(float intensity) {
	APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);	
	pc->PlayHapticEffect(HapticEffect, Hand, intensity);
}
