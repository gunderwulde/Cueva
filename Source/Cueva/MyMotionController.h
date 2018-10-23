// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyMotionController.generated.h"

UCLASS()
class CUEVA_API AMyMotionController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyMotionController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	class USphereComponent * SphereCollider;

	UPROPERTY(EditAnywhere, Category = "MotionController")
		bool WantsToGrip2;
	UPROPERTY(EditAnywhere, Category = "MotionController")
		AActor* AttachedActor2;
	UPROPERTY(EditAnywhere, Category = "MotionController")
		UHapticFeedbackEffect_Base* HapticEffect;
	UPROPERTY(EditAnywhere, Category = "MotionController")
		EControllerHand Hand;
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void GrabActor2();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		bool GetActorNearHand2(TScriptInterface<class INewPickupActorInterface> &Interface);
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void RumbleController2(float intensity);
};
