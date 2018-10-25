// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionController.h"
#include "Public/MotionControllerComponent.h"
#include "NewPickupActorInterface.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
//#include "Components/StaticMeshComponent.h"
#include "HandAnimInstance.h"
#include "SteamVRChaperoneComponent.h"
#include "Classes/Kismet/GameplayStatics.h"

// Sets default values
AMyMotionController::AMyMotionController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyMotionController::BeginPlay() {
	Super::BeginPlay();
	steamVRChaperoneComponent = this->FindComponentByClass<USteamVRChaperoneComponent>();
	skeletalMeshComponent = this->FindComponentByClass<USkeletalMeshComponent>();
	handAnimInstance = Cast<UHandAnimInstance>(skeletalMeshComponent->AnimScriptInstance);
	handAnimInstance->GripState2 = 2;

	sphereCollider = this->FindComponentByClass<USphereComponent>();
	sphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AMyMotionController::OnBeginOverlap);
	sphereCollider->OnComponentHit.AddDynamic(this, &AMyMotionController::OnHit);
	motionController = this->FindComponentByClass<UMotionControllerComponent>();
}

void AMyMotionController::SetHand(EControllerHand hand) {
	Hand = hand;
	if (Hand == EControllerHand::Left) {
		skeletalMeshComponent->SetWorldScale3D(FVector(1, 1, -1)); // La mano izquierda va espejada.
		motionController->MotionSource = _TEXT("Left");
	}
	else motionController->MotionSource = _TEXT("Right");

}

// Called every frame
void AMyMotionController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	UpdateHandAnimation();
	SetHandCollider();
}

void AMyMotionController::GrabActor() {
	wantsToGrip = true;
	AActor * tmp = GetActorNearHand();
	if (tmp != nullptr) {
		attachedActor = tmp;
		INewPickupActorInterface::Execute_Pickup(attachedActor, motionController);
		RumbleController(0.7f);
	}
}

void AMyMotionController::ReleaseActor() {
	wantsToGrip = false;
	if (attachedActor != nullptr) {
		INewPickupActorInterface::Execute_Drop(attachedActor);
		RumbleController(0.2f);
		attachedActor = nullptr;
	}
}

AActor* AMyMotionController::GetActorNearHand() {
	float nearestOverlap = 100000.0f;
	AActor* nearestOverlapActor = nullptr;
	TArray<AActor*> actors;
	sphereCollider->GetOverlappingActors(actors);
	FVector spherePosition = sphereCollider->GetComponentLocation();
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

void AMyMotionController::RumbleController(float intensity) {
	APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);	
	pc->PlayHapticEffect(HapticEffect, Hand, intensity);
}

void AMyMotionController::UpdateHandAnimation() {
	if (attachedActor != nullptr || wantsToGrip) {
		handAnimInstance->GripState2 = 2; // Grab.
	}
	else {
		if (GetActorNearHand() != nullptr)
			handAnimInstance->GripState2 = 1; // Can Grab.
		else
			handAnimInstance->GripState2 = 0; // OPen.
	}
}

void AMyMotionController::SetHandCollider() {
	switch (handAnimInstance->GripState2) {
	case 0:
	case 1:
		skeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case 2:
		skeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}


void AMyMotionController::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	UStaticMeshComponent* sm = Cast<UStaticMeshComponent>(OtherComp);
	if(sm!= nullptr && sm->IsSimulatingPhysics() )
		RumbleController(0.2f);
}

void AMyMotionController::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	RumbleController( (NormalImpulse.Size() / 1500.0f)*0.8f );
}

void AMyMotionController::SetupRoomscaleOutline() {
	TArray <FVector> bounds = steamVRChaperoneComponent->GetBounds();
}