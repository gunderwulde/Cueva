// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Components/StaticMeshComponent.h"
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
	class USteamVRChaperoneComponent* steamVRChaperoneComponent;
	class USphereComponent * sphereCollider;
	class USkeletalMeshComponent* skeletalMeshComponent;
	class UHandAnimInstance* handAnimInstance;
	class AActor* attachedActor;
	bool  wantsToGrip;

	class UMotionControllerComponent* motionController;
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
		UHapticFeedbackEffect_Base* HapticEffect;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
		UStaticMeshComponent* TeleportMark;

	EControllerHand Hand;
	void SetHand(EControllerHand hand);

	void GrabActor();
	void ReleaseActor();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		class AActor* GetActorNearHand();
	UFUNCTION(BlueprintCallable, Category = "MotionController")
		void RumbleController(float intensity);

	void UpdateHandAnimation();
	void SetHandCollider();

	UFUNCTION()
		void OnBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnHit(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void SetupRoomscaleOutline();
};
