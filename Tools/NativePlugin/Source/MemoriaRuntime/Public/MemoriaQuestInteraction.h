#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MemoriaQuestInteraction.generated.h"

UCLASS(ClassGroup=(Memoria), meta=(BlueprintSpawnableComponent))
class MEMORIARUNTIME_API UMemoriaQuestInteraction : public UActorComponent
{
    GENERATED_BODY()
public:
    UMemoriaQuestInteraction();
    UPROPERTY(EditAnywhere, Category="Pointer") TObjectPtr<class UMaterialInterface> PointerMaterial;
    UPROPERTY(EditAnywhere, Category="Pointer") TObjectPtr<class UStaticMesh> PointerMesh;
    UPROPERTY(EditAnywhere, Category="Pointer") float PointerLength = 800.f;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
private:
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Beam;
    bool bFloorInitialized = false;
};
