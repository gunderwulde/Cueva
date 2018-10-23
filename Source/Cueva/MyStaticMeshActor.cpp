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
		UStaticMeshComponent* sm = GetStaticMeshComponent();
		sm->SetSimulatePhysics(false);
		GetRootComponent()->AttachToComponent(controller, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AMyStaticMeshActor::Drop_Implementation(){
	UStaticMeshComponent* sm = GetStaticMeshComponent();
	sm->SetSimulatePhysics(true);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}