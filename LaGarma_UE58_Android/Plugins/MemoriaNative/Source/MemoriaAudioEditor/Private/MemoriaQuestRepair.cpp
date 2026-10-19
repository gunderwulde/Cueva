#include "MemoriaAudioEditorLibrary.h"
#include "MemoriaQuestInteraction.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_InputAction.h"
#include "K2Node_EnhancedInputAction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "InputAction.h"
#include "EnhancedInputDeveloperSettings.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "InputCoreTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

template<class T> static T* QuestAsset(const FString& Path)
{
    if (FPackageName::DoesPackageExist(Path)) return LoadObject<T>(nullptr, *Path);
    UPackage* Package = CreatePackage(*Path);
    T* Asset = NewObject<T>(Package, *FPackageName::GetLongPackageAssetName(Path), RF_Public|RF_Standalone|RF_Transactional);
    FAssetRegistryModule::AssetCreated(Asset);
    return Asset;
}
static bool SaveQuestAsset(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public|RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

bool UMemoriaAudioEditorLibrary::RepairQuestPawn(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->SimpleConstructionScript) return false;
    UInputAction* Left = QuestAsset<UInputAction>(TEXT("/Game/NativeVR/Input/IA_SelectLeft"));
    UInputAction* Right = QuestAsset<UInputAction>(TEXT("/Game/NativeVR/Input/IA_SelectRight"));
    for (UInputAction* Action : {Left, Right})
    {
        Action->ValueType = EInputActionValueType::Axis1D;
        Action->Triggers.Reset();
        auto* Down = NewObject<UInputTriggerDown>(Action);
        Down->ActuationThreshold = .5f;
        Action->Triggers.Add(Down);
        if (!SaveQuestAsset(Action)) return false;
    }
    UInputMappingContext* Context = QuestAsset<UInputMappingContext>(TEXT("/Game/NativeVR/Input/IMC_Quest"));
    Context->UnmapAll();
    Context->MapKey(Left, EKeys::OculusTouch_Left_Trigger_Axis);
    Context->MapKey(Right, EKeys::OculusTouch_Right_Trigger_Axis);
    if (!SaveQuestAsset(Context)) return false;

    UMaterial* Material = QuestAsset<UMaterial>(TEXT("/Game/NativeVR/M_SelectionRay"));
    Material->SetShadingModel(MSM_Unlit);
    if (Material->GetExpressions().IsEmpty())
    {
        auto* Color = NewObject<UMaterialExpressionConstant3Vector>(Material);
        Color->Constant = FLinearColor(.08f, .65f, 1.f);
        Material->GetExpressionCollection().AddExpression(Color);
        Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Color);
    }
    Material->PostEditChange();
    if (!SaveQuestAsset(Material)) return false;

    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    int32 Replaced = 0;
    for (UEdGraph* Graph : Graphs)
    {
        const auto Nodes = Graph->Nodes;
        for (UEdGraphNode* Node : Nodes)
        {
            auto* Legacy = Cast<UK2Node_InputAction>(Node);
            if (!Legacy || (Legacy->InputActionName != TEXT("GrabLeft") && Legacy->InputActionName != TEXT("GrabRight"))) continue;
            auto* Enhanced = NewObject<UK2Node_EnhancedInputAction>(Graph);
            Enhanced->InputAction = Legacy->InputActionName == TEXT("GrabLeft") ? Left : Right;
            Enhanced->NodePosX = Legacy->NodePosX;
            Enhanced->NodePosY = Legacy->NodePosY;
            Graph->AddNode(Enhanced, false, false);
            Enhanced->CreateNewGuid();
            Enhanced->PostPlacedNewNode();
            Enhanced->AllocateDefaultPins();
            UEdGraphPin* Pressed = Legacy->FindPin(TEXT("Pressed"));
            UEdGraphPin* Started = Enhanced->FindPin(TEXT("Started"));
            if (!Pressed || !Started || Pressed->LinkedTo.IsEmpty()) return false;
            const auto Links = Pressed->LinkedTo;
            Pressed->BreakAllPinLinks();
            for (UEdGraphPin* Target : Links) Started->MakeLinkTo(Target);
            Legacy->DestroyNode();
            ++Replaced;
        }
        // Quest uses a floor reference space. Keep the original desktop fallback.
        for (UEdGraphNode* Node : Graph->Nodes)
            for (UEdGraphPin* Pin : Node->Pins)
                if (Pin->PinName == TEXT("Origin") && Pin->DefaultValue == TEXT("LocalFloor"))
                    Pin->DefaultValue = TEXT("Stage");
    }
    USCS_Node* InteractionNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        if (Node->ComponentClass == UMemoriaQuestInteraction::StaticClass()) InteractionNode = Node;
    if (!InteractionNode)
    {
        InteractionNode = Blueprint->SimpleConstructionScript->CreateNode(UMemoriaQuestInteraction::StaticClass(), TEXT("QuestInteraction"));
        Blueprint->SimpleConstructionScript->AddNode(InteractionNode);
    }
    auto* Interaction = CastChecked<UMemoriaQuestInteraction>(InteractionNode->ComponentTemplate);
    Interaction->PointerMaterial = Material;
    Interaction->PointerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    const bool bCompiled = CompileNativeBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_QUEST_REPAIRED input events=%d compiled=%d"), Replaced, bCompiled);
    return bCompiled && Interaction->PointerMesh;
}

bool UMemoriaAudioEditorLibrary::ValidateQuestConfiguration(UBlueprint* Blueprint)
{
    const auto* Settings = GetDefault<UEnhancedInputDeveloperSettings>();
    bool bContext = false;
    for (const FDefaultContextSetting& Context : Settings->DefaultMappingContexts)
        bContext |= Context.InputMappingContext.ToSoftObjectPath().ToString() == TEXT("/Game/NativeVR/Input/IMC_Quest.IMC_Quest") && Context.bAddImmediately;
    bool bProfiles = true;
    for (const TCHAR* Name : { TEXT("Meta_Quest_3"), TEXT("Meta_Quest_3S") })
    {
        UDeviceProfile* Profile = UDeviceProfileManager::Get().FindProfile(Name);
        bProfiles &= Profile && Profile->GetCVarValue(TEXT("r.Mobile.AntiAliasing")) == TEXT("3")
            && Profile->GetCVarValue(TEXT("r.MSAACount")) == TEXT("4");
    }
    int32 Events = 0;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Input = Cast<UK2Node_EnhancedInputAction>(Node))
            if (Input->InputAction && Input->InputAction->GetName().StartsWith(TEXT("IA_Select")))
                if (UEdGraphPin* Started = Input->FindPin(TEXT("Started")))
                    Events += Started->LinkedTo.Num() == 1;
    const bool bOK = Settings->bEnableDefaultMappingContexts && bContext && bProfiles && Events == 2;
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_QUEST_CONFIG defaultContext=%d profilesMSAA4=%d selectionEvents=%d ok=%d"), bContext, bProfiles, Events, bOK);
    return bOK;
}