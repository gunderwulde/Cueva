#include "MemoriaQuestInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "MotionControllerComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

static AActor* ControllerProperty(APawn* Pawn, const FName Name)
{
    const FObjectProperty* Property = FindFProperty<FObjectProperty>(Pawn->GetClass(), Name);
    return Property ? Cast<AActor>(Property->GetObjectPropertyValue_InContainer(Pawn)) : nullptr;
}

UMemoriaQuestInteraction::UMemoriaQuestInteraction()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UMemoriaQuestInteraction::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
    Super::TickComponent(DeltaTime, TickType, Function);
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !Pawn->IsLocallyControlled()) return;

    // BeginPlay can run before OpenXR has established its reference space. Do not
    // retain the legacy desktop fallback's additional 180 cm above the real floor.
    if (!bFloorInitialized && UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()
        && UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName() == TEXT("OpenXR"))
    {
        UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
        TArray<USceneComponent*> Components;
        Pawn->GetComponents(Components);
        for (USceneComponent* Component : Components)
            if (Component->GetFName() == TEXT("VROrigin"))
            {
                FVector Location = Component->GetRelativeLocation();
                Location.Z = 0.f;
                Component->SetRelativeLocation(Location);
                bFloorInitialized = true;
                UE_LOG(LogTemp, Display, TEXT("MEMORIA_QUEST_FLOOR stage origin; VROrigin Z=0"));
            }
    }

    AActor* Left = ControllerProperty(Pawn, TEXT("LeftController"));
    AActor* Right = ControllerProperty(Pawn, TEXT("RightController"));
    for (AActor* Controller : {Left, Right})
        if (Controller)
            if (auto* Motion = Controller->FindComponentByClass<UMotionControllerComponent>())
            {
                const FName Aim = Controller == Left ? TEXT("LeftAim") : TEXT("RightAim");
                if (Motion->MotionSource != Aim) Motion->SetTrackingMotionSource(Aim);
            }

    AActor* Current = ControllerProperty(Pawn, TEXT("CurrentController"));
    if (!Current && Right)
    {
        if (FObjectProperty* Property = FindFProperty<FObjectProperty>(Pawn->GetClass(), TEXT("CurrentController")))
            Property->SetObjectPropertyValue_InContainer(Pawn, Right);
        if (UFunction* Switch = Right->FindFunction(TEXT("Switch")))
        {
            struct { bool On = true; } Parameters;
            Right->ProcessEvent(Switch, &Parameters);
        }
        Current = Right;
    }
    UMotionControllerComponent* Motion = Current ? Current->FindComponentByClass<UMotionControllerComponent>() : nullptr;
    bool bVisible = Motion && Motion->IsTracked();
    if (Current)
        if (const FBoolProperty* Visible = FindFProperty<FBoolProperty>(Current->GetClass(), TEXT("isVisible")))
            bVisible &= Visible->GetPropertyValue_InContainer(Current);
    if (!Motion || !PointerMesh || !PointerMaterial)
    {
        if (Beam) Beam->SetVisibility(false);
        return;
    }
    if (!Beam)
    {
        Beam = NewObject<UStaticMeshComponent>(Pawn, TEXT("QuestSelectionRay"));
        Beam->SetStaticMesh(PointerMesh);
        Beam->SetMaterial(0, PointerMaterial);
        Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Beam->SetCastShadow(false);
        Beam->bReceivesDecals = false;
        Beam->RegisterComponent();
    }
    Beam->SetVisibility(bVisible);
    if (!bVisible) return;
    Beam->AttachToComponent(Motion, FAttachmentTransformRules::KeepRelativeTransform);
    const FVector Start = Motion->GetComponentLocation();
    const FVector Direction = Motion->GetForwardVector();
    // The original selector uses GrabSphere's position/forward vector. Drive
    // that selector from the same aim pose as the visible ray, rather than gaze.
    TArray<USceneComponent*> PawnComponents;
    Pawn->GetComponents(PawnComponents);
    for (USceneComponent* Component : PawnComponents)
        if (Component->GetFName() == TEXT("GrabSphere"))
            Component->SetWorldLocationAndRotation(Start, Motion->GetComponentQuat());
    FCollisionQueryParams Query(SCENE_QUERY_STAT(MemoriaQuestPointer), false, Pawn);
    if (Left) Query.AddIgnoredActor(Left);
    if (Right) Query.AddIgnoredActor(Right);
    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * PointerLength, ECC_Visibility, Query);
    const float Distance = bHit ? FMath::Max(1.f, Hit.Distance) : PointerLength;
    Beam->SetRelativeLocation(FVector(Distance * .5f, 0.f, 0.f));
    Beam->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector, FVector::ForwardVector));
    Beam->SetRelativeScale3D(FVector(.003f, .003f, Distance / 100.f));
}
