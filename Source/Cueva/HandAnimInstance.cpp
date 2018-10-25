// Fill out your copyright notice in the Description page of Project Settings.

#include "HandAnimInstance.h"

//This function is like PostInitAnimtree in UE3 
void UHandAnimInstance::NativeInitializeAnimation() {

	//Very Important Line 
	Super::NativeInitializeAnimation();
	//Cache the owning pawn for use in Tick 
}

//Tick 
void UHandAnimInstance::NativeUpdateAnimation(float DeltaTimeX) {
	//Very Important Line 
	Super::NativeUpdateAnimation(DeltaTimeX);
	Grip = FMath::FInterpTo(Grip, GripState2/2.0f, DeltaTimeX, 7.0f); // 7.0f -> Velocidad de interpolación.
}