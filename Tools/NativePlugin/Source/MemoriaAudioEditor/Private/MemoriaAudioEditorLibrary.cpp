#include "MemoriaAudioEditorLibrary.h"
#include "MemoriaAudioTracks.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundNodeMixer.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeConcatenator.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeDelay.h"
#include "Sound/SoundNodeModulator.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "K2Node_SwitchName.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"



static TSharedPtr<FJsonObject> ReadJson(const FString& Text)
{
    TSharedPtr<FJsonObject> Object;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object);
    return Object;
}
static FString Prop(const TSharedPtr<FJsonObject>& Node, const FString& Name, const FString& Default=TEXT("0"))
{
    const TSharedPtr<FJsonObject>* Props;
    FString Value;
    return Node->TryGetObjectField(TEXT("properties"), Props) && (*Props)->TryGetStringField(Name, Value) ? Value : Default;
}
static float Number(const TSharedPtr<FJsonObject>& Node, const FString& Name)
{ return FCString::Atof(*Prop(Node, Name)); }

template<class T> static T* MakeNode(USoundCue* Cue)
{ return Cue->ConstructSoundNode<T>(T::StaticClass(), false); }

static void LinkSingle(USoundNode* Parent, USoundNode* Child)
{ TArray<USoundNode*> Children{Child}; Parent->SetChildNodes(Children); }

static USoundNode* BuildNode(USoundCue* Cue, const TSharedPtr<FJsonObject>& Node, const FString& Language)
{
    if (Prop(Node, TEXT("Inclusion"), TEXT("True")) == TEXT("False")) return nullptr;
    TArray<USoundNode*> Children;
    const TArray<TSharedPtr<FJsonValue>>* Values;
    if (Node->TryGetArrayField(TEXT("children"), Values)) for (const auto& Value : *Values)
        if (USoundNode* Child = BuildNode(Cue, Value->AsObject(), Language)) Children.Add(Child);
    if (Node->TryGetArrayField(TEXT("sources"), Values))
    {
        TSharedPtr<FJsonObject> Selected;
        for (const auto& Value : *Values)
        {
            const auto Source = Value->AsObject();
            const FString Lang = Source->GetStringField(TEXT("language"));
            if (Lang == Language || Lang == TEXT("SFX")) { Selected = Source; break; }
            if (!Selected) Selected = Source;
        }
        FString Asset;
        if (Selected && Selected->TryGetStringField(TEXT("wave_asset"), Asset))
        {
            USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Asset);
            if (!Wave) { UE_LOG(LogTemp, Error, TEXT("MEMORIA missing native wave %s"), *Asset); return nullptr; }
            USoundNodeWavePlayer* Player = MakeNode<USoundNodeWavePlayer>(Cue);
            Player->SetSoundWave(Wave);
            Player->bLooping = Prop(Node, TEXT("IsLoopingEnabled")) == TEXT("True");
            Children.Add(Player);
        }
    }
    if (Children.IsEmpty()) return nullptr;
    USoundNode* Result = nullptr;
    const FString Kind = Node->GetStringField(TEXT("kind"));
    if (Children.Num() == 1) Result = Children[0];
    else if (Kind == TEXT("RandomSequenceContainer") && Prop(Node, TEXT("RandomOrSequence")) != TEXT("1"))
    {
        USoundNodeRandom* Random = MakeNode<USoundNodeRandom>(Cue);
        Random->SetChildNodes(Children);
        Random->bRandomizeWithoutReplacement = true;
        Random->PreselectAtLevelLoad = 0;
        Result = Random;
    }
    else if (Kind == TEXT("MusicPlaylistContainer") || Kind == TEXT("RandomSequenceContainer"))
    {
        USoundNodeConcatenator* Sequence = MakeNode<USoundNodeConcatenator>(Cue);
        Sequence->SetChildNodes(Children);
        Sequence->InputVolume.Init(1.f, Children.Num());
        Result = Sequence;
    }
    else
    {
        USoundNodeMixer* Mixer = MakeNode<USoundNodeMixer>(Cue);
        Mixer->SetChildNodes(Children);
        Mixer->InputVolume.Init(1.f, Children.Num());
        Result = Mixer;
    }
    if (Kind == TEXT("MusicPlaylistContainer") || Prop(Node, TEXT("IsContinuousPlayback")) == TEXT("True"))
    {
        USoundNodeLooping* Loop = MakeNode<USoundNodeLooping>(Cue);
        Loop->bLoopIndefinitely = true;
        LinkSingle(Loop, Result);
        Result = Loop;
    }
    const float Volume = Number(Node, TEXT("Volume"));
    const float Pitch = Number(Node, TEXT("Pitch"));
    if (Volume != 0.f || Pitch != 0.f)
    {
        USoundNodeModulator* Gain = MakeNode<USoundNodeModulator>(Cue);
        Gain->VolumeMin = Gain->VolumeMax = FMath::Pow(10.f, Volume / 20.f);
        Gain->PitchMin = Gain->PitchMax = FMath::Pow(2.f, Pitch / 1200.f);
        LinkSingle(Gain, Result);
        Result = Gain;
    }
    const float Delay = Number(Node, TEXT("InitialDelay"));
    if (Delay > 0.f)
    {
        USoundNodeDelay* DelayNode = MakeNode<USoundNodeDelay>(Cue);
        DelayNode->DelayMin = DelayNode->DelayMax = Delay;
        LinkSingle(DelayNode, Result);
        Result = DelayNode;
    }
    return Result;
}

USoundCue* UMemoriaAudioEditorLibrary::BuildNativeSoundCue(const FString& AssetPath, const FString& TreeJson, const FString& Language)
{
    if (FPackageName::DoesPackageExist(AssetPath)) if (USoundCue* Existing = LoadObject<USoundCue>(nullptr, *AssetPath)) return Existing;
    const auto Tree = ReadJson(TreeJson);
    if (!Tree) return nullptr;
    UPackage* Package = CreatePackage(*AssetPath);
    const FString Name = FPackageName::GetLongPackageAssetName(AssetPath);
    USoundCue* Cue = NewObject<USoundCue>(Package, *Name, RF_Public|RF_Standalone|RF_Transactional);
    Cue->CreateGraph();
    Cue->VolumeMultiplier = 1.f;
    Cue->PitchMultiplier = 1.f;
    const TArray<TSharedPtr<FJsonValue>>* Ancestors;
    if (Tree->TryGetArrayField(TEXT("ancestors"), Ancestors)) for (const auto& Ancestor : *Ancestors)
    {
        Cue->VolumeMultiplier *= FMath::Pow(10.f, Number(Ancestor->AsObject(), TEXT("Volume"))/20.f);
        Cue->PitchMultiplier *= FMath::Pow(2.f, Number(Ancestor->AsObject(), TEXT("Pitch"))/1200.f);
    }
    Cue->FirstNode = BuildNode(Cue, Tree, Language);
    if (!Cue->FirstNode) { UE_LOG(LogTemp, Error, TEXT("MEMORIA empty native sound tree %s"), *AssetPath); return nullptr; }
    for (USoundNode* Node : Cue->AllNodes) if (Node && Node->GetGraphNode()) Node->GetGraphNode()->ReconstructNode();
    Cue->LinkGraphNodesFromSoundNodes();
    Cue->CacheAggregateValues();
    Cue->PostEditChange();
    FAssetRegistryModule::AssetCreated(Cue);
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public|RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
    if (!UPackage::SavePackage(Package, Cue, *File, Args)) return nullptr;
    return Cue;
}

bool UMemoriaAudioEditorLibrary::RestoreVolumeCurve(UMovieSceneSection* Section, const FString& CurveJson)
{
    FMovieSceneFloatChannel* Volume = Section ? Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0) : nullptr;
    const auto Curve = ReadJson(CurveJson);
    if (!Volume || !Curve) return false;
    Volume->Reset();
    double Default;
    if (Curve->TryGetNumberField(TEXT("default"), Default)) Volume->SetDefault(Default);
    for (const auto& Value : Curve->GetArrayField(TEXT("keys")))
    {
        const auto Key = Value->AsObject();
        const FFrameNumber Time(Key->GetIntegerField(TEXT("frame")));
        FMovieSceneFloatValue Float(Key->GetNumberField(TEXT("value")));
        const FString Interpolation = Key->GetStringField(TEXT("interpolation"));
        Float.InterpMode = Interpolation.Contains(TEXT("CUBIC")) ? RCIM_Cubic : Interpolation.Contains(TEXT("CONSTANT")) ? RCIM_Constant : RCIM_Linear;
        Float.TangentMode = RCTM_User;
        Float.Tangent.ArriveTangent = Key->GetNumberField(TEXT("arrive_tangent"));
        Float.Tangent.LeaveTangent = Key->GetNumberField(TEXT("leave_tangent"));
        Volume->GetData().AddKey(Time, Float);
    }
    Section->MarkPackageDirty();
    return true;
}

bool UMemoriaAudioEditorLibrary::CompileNativeBlueprint(UBlueprint* Blueprint)
{
    if (!Blueprint) return false;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_SwitchName* Switch = Cast<UK2Node_SwitchName>(Node);
        if (!Switch) continue;
        const int32 Index = Switch->PinNames.IndexOfByKey(FName(TEXT("SteamVR")));
        if (Index == INDEX_NONE) continue;
        Switch->PinNames[Index] = TEXT("OpenXR");
        if (UEdGraphPin* Pin = Switch->FindPin(TEXT("SteamVR"), EGPD_Output)) Pin->PinName = TEXT("OpenXR");
        Switch->ReconstructNode();
    }
    FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
    FString Export;
    for (UEdGraph* Graph : Graphs)
    {
        TSet<UObject*> Nodes;
        for (UEdGraphNode* Node : Graph->Nodes) if (Node) Nodes.Add(Node);
        FString Text;
        FEdGraphUtilities::ExportNodesToText(Nodes, Text);
        Export += Text;
    }
    const FString Folder = FPaths::Combine(FPaths::ProjectDir(), TEXT("../Diagnostics/BlueprintGraphs58"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    FFileHelper::SaveStringToFile(Export, *FPaths::Combine(Folder, Blueprint->GetName()+TEXT(".txt")));
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_NATIVE_BLUEPRINT %s errors=%d warnings=%d"), *Blueprint->GetPathName(), Results.NumErrors, Results.NumWarnings);
    return Results.NumErrors == 0;
}
UBlueprint* UMemoriaAudioEditorLibrary::GetLevelBlueprint(UWorld* World)
{ return World && World->PersistentLevel ? World->PersistentLevel->GetLevelScriptBlueprint(true) : nullptr; }





