#include "MemoriaAudioEditorLibrary.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionPrecomputedAOMask.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "NiagaraSystem.h"
#include "NiagaraGraph.h"
#include "UObject/UObjectIterator.h"

int32 UMemoriaAudioEditorLibrary::RepairMobileAsset(UObject* Asset)
{
    if (UMaterial* Material = Cast<UMaterial>(Asset))
    {
        int32 Changes = 0;
        for (UMaterialExpression* Expression : Material->GetExpressions())
            if (auto* Custom = Cast<UMaterialExpressionCustom>(Expression))
            {
                const FString Old = TEXT("SceneTextureLookup(uvShifted, TexIndex, false)");
                if (!Custom->Code.Contains(Old)) continue;
                Custom->Code = Custom->Code.Replace(*Old, TEXT("SceneTextureLookup(Parameters, uvShifted, TexIndex, false).rgb"));
                ++Changes;
            }
        if (Changes) { Material->PostEditChange(); Material->MarkPackageDirty(); }
        return Changes;
    }
    if (UNiagaraSystem* System = Cast<UNiagaraSystem>(Asset))
    {
        // Legacy skeletal-surface scripts contain reads that the cook trimmer cannot trace.
        for (TObjectIterator<UNiagaraGraph> It; It; ++It)
        {
            if (!It->IsIn(System)) continue;
            It->RenameParameter(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
                TEXT("Particles.Initial.SampleSkeletalMeshSurfaceMeshPosition")),
                TEXT("Particles.Initial.SampleSkeletalMeshSurface.MeshPosition"));
            It->NotifyGraphChanged();
        }
        System->SetTrimAttributesOnCook(false);
        System->RequestCompile(true);
        System->WaitForCompilationComplete();
        System->MarkPackageDirty();
        return System->IsValid() ? 1 : 0;
    }
    UMaterialFunction* Function = Cast<UMaterialFunction>(Asset);
    if (!Function) return 0;
    TArray<UMaterialExpression*> Expressions;
    for (UMaterialExpression* Expression : Function->GetExpressions()) Expressions.Add(Expression);
    int32 Count = 0;
    for (UMaterialExpression* Expression : Expressions)
    {
        if (!Cast<UMaterialExpressionPrecomputedAOMask>(Expression)) continue;
        bool AlreadyWrapped = false;
        for (UMaterialExpression* Other : Expressions)
            if (auto* Existing = Cast<UMaterialExpressionFeatureLevelSwitch>(Other))
                AlreadyWrapped |= Existing->Default.Expression == Expression && Existing->Desc.StartsWith(TEXT("Memoria:"));
        if (AlreadyWrapped) { ++Count; continue; }
        auto* Switch = NewObject<UMaterialExpressionFeatureLevelSwitch>(Function, NAME_None, RF_Transactional);
        auto* Zero = NewObject<UMaterialExpressionConstant>(Function, NAME_None, RF_Transactional);
        Zero->R = 0.0f; // No precomputed AO is available on mobile.
        Switch->Default.Connect(0, Expression);
        Switch->Inputs[ERHIFeatureLevel::ES3_1].Connect(0, Zero);
        Switch->Desc = TEXT("Memoria: precomputed AO on desktop, unoccluded fallback on mobile");
        Switch->MaterialExpressionEditorX = Expression->MaterialExpressionEditorX + 180;
        Switch->MaterialExpressionEditorY = Expression->MaterialExpressionEditorY;
        for (UMaterialExpression* Consumer : Expressions)
            for (FExpressionInput* Input : Consumer->GetInputsView())
                if (Input && Input->Expression == Expression) Input->Expression = Switch;
        Function->GetExpressionCollection().AddExpression(Switch);
        Function->GetExpressionCollection().AddExpression(Zero);
        ++Count;
    }
    if (Count) { Function->PostEditChange(); Function->MarkPackageDirty(); }
    return Count;
}
