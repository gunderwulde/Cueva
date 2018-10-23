// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMotionController.h"
#include "NewPickupActorInterface.h"
#include "Components/SphereComponent.h"
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
	TArray<USphereComponent> StaticComps;

	// Capturo el collider tipo esfera que tiene la mano.
	SphereCollider = Cast <USphereComponent>( this->GetComponentByClass(USphereComponent::StaticClass()) );
	if (SphereCollider!= nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FP: AMyMotionController::SphewreCollider encontrada"));
	}


}

// Called every frame
void AMyMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AMyMotionController::GrabActor2() {
	WantsToGrip2 = true;
}

bool AMyMotionController::GetActorNearHand2(TScriptInterface<INewPickupActorInterface> &Interface) {
	Interface = NULL;
	return false;
}

void AMyMotionController::RumbleController2(float intensity) {
	APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);	
	pc->PlayHapticEffect(HapticEffect, Hand, intensity);
}
