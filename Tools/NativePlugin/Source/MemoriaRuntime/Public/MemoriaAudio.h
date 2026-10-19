#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "MemoriaAudio.generated.h"

class UAudioComponent;
class USoundBase;
class UMemoriaAudioComponent;

// Identifiers retained for importing the existing Blueprint pins.
UENUM(BlueprintType, meta=(Bitflags))
enum class EMemoriaCallbackType : uint8 { EndOfEvent=0, Marker=2, Duration=3 };

UCLASS(BlueprintType)
class MEMORIARUNTIME_API UMemoriaAudioCallbackInfo : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Memoria|Audio") TObjectPtr<UMemoriaAudioComponent> AkComponent;
    UPROPERTY(BlueprintReadOnly, Category="Memoria|Audio") int32 PlayingID = 0;
};
DECLARE_DYNAMIC_DELEGATE_TwoParams(FMemoriaPostEventCallback, EMemoriaCallbackType, CallbackType, UMemoriaAudioCallbackInfo*, CallbackInfo);

USTRUCT(BlueprintType)
struct FMemoriaAudioAction
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") FString TargetId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> EnglishSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> SpanishSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") float Delay = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") float FadeTime = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") bool bStop = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") bool bGlobalScope = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") bool bUseFadesParameter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") bool bSpatialized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TArray<FVector2D> DistanceCurve;
};

UCLASS(BlueprintType)
class MEMORIARUNTIME_API UMemoriaAudioBank : public UDataAsset
{
    GENERATED_BODY()
public:
    // Import metadata only. Native Unreal sounds do not need Wwise banks.
    UPROPERTY(EditAnywhere, Category="Legacy import") bool AutoLoad = true;
};

UCLASS(BlueprintType)
class MEMORIARUNTIME_API UMemoriaAudioEvent : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") TArray<FMemoriaAudioAction> NativeActions;
    UPROPERTY(EditAnywhere, Category="Legacy import") TObjectPtr<UMemoriaAudioBank> RequiredBank;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Memoria|Audio") float MaxAttenuationRadius = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Memoria|Audio") bool IsInfinite = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Memoria|Audio") float MinimumDuration = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Memoria|Audio") float MaximumDuration = 0.f;
};

struct FMemoriaVoice
{
    TWeakObjectPtr<UAudioComponent> Component;
    FString TargetId;
    bool bUseFades = false;
};
struct FMemoriaPlayingEvent
{
    TWeakObjectPtr<USceneComponent> AttachComponent;
    TArray<FMemoriaVoice> Voices;
    TArray<FTimerHandle> Timers;
    FMemoriaPostEventCallback Callback;
    bool bCallbackRequested = false;
    bool bStopOnOwnerDestroyed = true;
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual TStatId GetStatId() const override;
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    int32 PlayEvent(UMemoriaAudioEvent* Event, USceneComponent* Attach, float StartOffset=0.f,
        bool bStopOnOwnerDestroyed=true, int32 CallbackMask=0, const FMemoriaPostEventCallback& Callback=FMemoriaPostEventCallback());
    void StopEvent(int32 Id, float FadeTime=0.f);
    void SetPaused(int32 Id, bool bPaused);
    void StopTarget(const FString& TargetId, USceneComponent* Attach, bool bGlobal, float FadeTime);
    void SetParameter(const FString& Name, float Value);
private:
    void ExecuteAction(int32 Id, const FMemoriaAudioAction& Action, float StartOffset);
    TMap<int32, FMemoriaPlayingEvent> Playing;
    int32 NextPlayingId = 1;
    float FadesValue = 100.f;
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class MEMORIARUNTIME_API UMemoriaAudioComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UMemoriaAudioComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") TObjectPtr<UMemoriaAudioEvent> AkAudioEvent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") FString EventName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") float AttenuationScalingFactor = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") bool StopWhenOwnerDestroyed = true;
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio", meta=(AutoCreateRefTerm="PostEventCallback"))
    int32 PostAkEvent(UMemoriaAudioEvent* AkEvent, int32 CallbackMask, const FMemoriaPostEventCallback& PostEventCallback, const FString& in_EventName);
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio", meta=(AutoCreateRefTerm="PostEventCallback"))
    int32 PostAssociatedAkEvent(int32 CallbackMask, const FMemoriaPostEventCallback& PostEventCallback);
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") void Stop();
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") void SetRTPCValue(FString RTPC, float Value, int32 InterpolationTimeMs);
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    TArray<int32> PlayingIds;
};

UCLASS(BlueprintType)
class MEMORIARUNTIME_API AMemoriaAmbientSound : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaAmbientSound();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Memoria|Audio") TObjectPtr<UMemoriaAudioComponent> AkComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") bool StopWhenOwnerIsDestroyed = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Memoria|Audio") bool AutoPost = false;
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") void StartAmbientSound();
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") void StopAmbientSound();
protected:
    virtual void BeginPlay() override;
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") static bool SetCurrentLanguageEnglish();
    UFUNCTION(BlueprintCallable, Category="Memoria|Audio") static bool SetCurrentLanguageSpanish();
    static bool IsSpanish();
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaXRFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Memoria|XR") static bool GetUserPresent();
    UFUNCTION(BlueprintCallable, Category="Memoria|XR") static void FadeToColor(float FadeTime, FLinearColor FadeColor, bool background);
    UFUNCTION(BlueprintCallable, Category="Memoria|XR") static void SetSplashScreen(class UTexture* Texture, FVector2D Scale, FVector Offset, bool bShowLoadingMovie, bool bShowOnSet);
    UFUNCTION(BlueprintCallable, Category="Memoria|XR") static void ShowSplashScreen();
    UFUNCTION(BlueprintCallable, Category="Memoria|XR") static void HideSplashScreen();
    UFUNCTION(BlueprintCallable, Category="Memoria|XR") static void EnableAutoLoadingSplashScreen(bool InAutoShowEnabled);
};

