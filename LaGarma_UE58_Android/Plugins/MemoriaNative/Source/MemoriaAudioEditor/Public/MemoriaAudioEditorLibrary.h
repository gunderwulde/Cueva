#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MemoriaAudioEditorLibrary.generated.h"

UCLASS()
class MEMORIAAUDIOEDITOR_API UMemoriaAudioEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static int32 GetWwiseCurveKeyCount(UObject* Section);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static FString SmokeTestWwisePlayback();
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static UObject* CreateWwiseAsset(const FString& AssetPath, const FString& ClassPath);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool InitializeWwiseAsset(UObject* Asset, const FString& Guid, const FString& Name);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool RepairWwiseTrack(UObject* Track, bool MasterTrack);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool SetWwiseAudioSection(UObject* Section, UObject* AudioAsset, bool StopAtEnd, bool Retrigger);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool ValidateQuestConfiguration(class UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool RepairQuestPawn(class UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static int32 RepairMobileAsset(UObject* Asset);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static class USoundCue* BuildNativeSoundCue(const FString& AssetPath, const FString& TreeJson, const FString& Language);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool RestoreVolumeCurve(class UMovieSceneSection* Section, const FString& CurveJson);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static bool CompileNativeBlueprint(class UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Memoria|Migration")
    static class UBlueprint* GetLevelBlueprint(class UWorld* World);
};
