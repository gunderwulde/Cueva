// Fill out your copyright notice in the Description page of Project Settings.

#include "MyStaticMeshActor.h"
#include "MyMotionController.h"
#include "Components/StaticMeshComponent.h"

AMyStaticMeshActor::AMyStaticMeshActor()
{
}

void AMyStaticMeshActor::Pickup_Implementation(USceneComponent* controller)
{
	if (controller) {
		UE_LOG(LogTemp, Warning, TEXT("AMyStaticMeshActor::Pickup"));
		UStaticMeshComponent* sm = GetStaticMeshComponent();
		sm->SetSimulatePhysics(false);
		GetRootComponent()->AttachToComponent(controller, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AMyStaticMeshActor::Drop_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("AMyStaticMeshActor::Drop"));
	UStaticMeshComponent* sm = GetStaticMeshComponent();
	sm->SetSimulatePhysics(true);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}