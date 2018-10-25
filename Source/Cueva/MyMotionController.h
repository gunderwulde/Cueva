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

	FVector		thumbAxe;
	FRotator	teleportRotation;
	bool		isValidTeleporDestination;

public:
	// Sets default values for this actor's properties
	AMyMotionController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	AMyMotionController* other;

	class USteamVRChaperoneComponent* steamVRChaperoneComponent;
	class USphereComponent * sphereCollider;
	class USkeletalMeshComponent* skeletalMeshComponent;
	class UHandAnimInstance* handAnimInstance;
	class AActor* attachedActor;
	bool  wantsToGrip;
	bool  isTeleportActive2;
	bool  isRoomScale2;

	FRotator initialControllerRotation2;


	class UMotionControllerComponent* motionController;
	UPROPERTY(EditDefaultsOnly, Category = "MotionController")
		UHapticFeedbackEffect_Base* HapticEffect;

	class UStaticMeshComponent* teleportCylinder;
	class UStaticMeshComponent* roomScaleMesh;
	class UStaticMeshComponent* arcEndPoint;
	class UArrowComponent* arcDirection;

	EControllerHand Hand;
	void SetHand(EControllerHand hand);

	void GrabActor();
	void ReleaseActor();

	class AActor* GetActorNearHand();
	void RumbleController(float intensity);

	void UpdateHandAnimation();
	void SetHandCollider();

	UFUNCTION()
		void OnBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnHit(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void SetupRoomscaleOutline();

	void ActivateTeleporter();
	UFUNCTION(BlueprintCallable)
	void DisableTeleporter();
	void ExecuteTeleportation2();

	void SetThumbX(float amount);
	void SetThumbY(float amount);

	void SetTeleportRotation();
	void UpdateArc();
	void ClearArc();
	void TraceTeleportDestination();
	void UpdateArcSpline(TArray<FVector> *tracedPoints);
	void UpdateArcEndPoint(FVector *newLocation);
};