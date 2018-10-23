// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
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
	class AActor* attachedActor;
	bool WantsToGrip2;
	AActor* AttachedActor2;

	UPROPERTY(EditAnywhere, Category = "MotionController")
		class UMotionControllerComponent* motionController2;

	UPROPERTY(EditAnywhere, Category = "MotionController")
		UHapticFeedbackEffect_Base* HapticEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionController", Meta = (DisplayName = "Hand", ExposeOnSpawn = true))
		EControllerHand Hand;

	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void GrabActor2();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void ReleaseActor2();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		class AActor* GetActorNearHand2();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void RumbleController2(float intensity);
};
