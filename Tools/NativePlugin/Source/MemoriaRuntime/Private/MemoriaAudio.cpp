#include "MemoriaAudio.h"
#include "AkAudioDevice.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "XRLoadingScreenFunctionLibrary.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, MemoriaRuntime)

static float FadesGain(float Value)
{
    return Value <= 0.f ? 0.f : FMath::Pow(10.f, FMath::Lerp(-200.f, 0.f, FMath::Clamp(Value / 100.f, 0.f, 1.f)) / 20.f);
}

TStatId UMemoriaAudioSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UMemoriaAudioSubsystem, STATGROUP_Tickables); }

int32 UMemoriaAudioSubsystem::PlayEvent(UMemoriaAudioEvent* Event, USceneComponent* Attach, float StartOffset,
    bool bStopOnOwnerDestroyed, int32 CallbackMask, const FMemoriaPostEventCallback& Callback)
{
    if (!Event || !GetWorld()) return 0;
    const int32 Id = NextPlayingId++;
    FMemoriaPlayingEvent& Record = Playing.Add(Id);
    Record.AttachComponent = Attach;
    Record.Callback = Callback;
    Record.bCallbackRequested = (CallbackMask & 1) != 0;
    Record.bStopOnOwnerDestroyed = bStopOnOwnerDestroyed;
    for (const FMemoriaAudioAction& Action : Event->NativeActions)
    {
        const float RemainingDelay = Action.Delay - StartOffset;
        if (RemainingDelay <= 0.f) ExecuteAction(Id, Action, FMath::Max(0.f, -RemainingDelay));
        else
        {
            FTimerHandle Handle;
            TWeakObjectPtr<UMemoriaAudioSubsystem> WeakThis(this);
            GetWorld()->GetTimerManager().SetTimer(Handle, [WeakThis, Id, Action]() {
                if (WeakThis.IsValid()) WeakThis->ExecuteAction(Id, Action, 0.f);
            }, RemainingDelay, false);
            if (FMemoriaPlayingEvent* Found = Playing.Find(Id)) Found->Timers.Add(Handle);
        }
    }
    return Id;
}

void UMemoriaAudioSubsystem::ExecuteAction(int32 Id, const FMemoriaAudioAction& Action, float StartOffset)
{
    FMemoriaPlayingEvent* Record = Playing.Find(Id);
    if (!Record) return;
    USceneComponent* Attach = Record->AttachComponent.Get();
    if (Action.bStop)
    {
        StopTarget(Action.TargetId, Attach, Action.bGlobalScope, Action.FadeTime);
        return;
    }
    USoundBase* Sound = UMemoriaAudioLibrary::IsSpanish() && Action.SpanishSound ? Action.SpanishSound.Get() : Action.EnglishSound.Get();
    if (!Sound) return;
    if (Sound->GetDuration() < INDEFINITELY_LOOPING_DURATION && StartOffset >= Sound->GetDuration()) return;
    UAudioComponent* Audio = NewObject<UAudioComponent>(Attach ? Attach->GetOwner() : GetWorld()->GetWorldSettings());
    Audio->bAutoActivate = false;
    Audio->bAutoDestroy = true;
    Audio->bStopWhenOwnerDestroyed = Record->bStopOnOwnerDestroyed;
    if (Attach) Audio->SetupAttachment(Attach);
    Audio->bAllowSpatialization = Action.bSpatialized;
    if (Action.bSpatialized)
    {
        Audio->bOverrideAttenuation = true;
        Audio->AttenuationOverrides.bAttenuate = true;
        Audio->AttenuationOverrides.bSpatialize = true;
        FSoundAttenuationSettings& Settings = Audio->AttenuationOverrides;
        Settings.AttenuationShapeExtents.X = 0.f;
        if (Action.DistanceCurve.Num() >= 2)
        {
            const float Radius = FMath::Max(1.f, float(Action.DistanceCurve.Last().X));
            const UMemoriaAudioComponent* Emitter = Cast<UMemoriaAudioComponent>(Attach);
            const float Scale = Emitter ? FMath::Max(0.001f, Emitter->AttenuationScalingFactor) : 1.f;
            Settings.FalloffDistance = Radius * Scale;
            Settings.DistanceAlgorithm = EAttenuationDistanceModel::Custom;
            bool bHasFalloff = false;
            FRichCurve* Curve = Settings.CustomAttenuationCurve.GetRichCurve();
            for (int32 Point = 1; Point < Action.DistanceCurve.Num(); ++Point)
            {
                const FVector2D A = Action.DistanceCurve[Point-1], B = Action.DistanceCurve[Point];
                bHasFalloff |= B.Y < -0.01f;
                for (int32 Sample = 0; Sample <= 32; ++Sample)
                {
                    const float Alpha = Sample / 32.f;
                    const float Distance = FMath::Lerp(A.X, B.X, Alpha);
                    const float Decibels = FMath::Lerp(A.Y, B.Y, Alpha);
                    Curve->AddKey(Distance / Radius, FMath::Pow(10.f, Decibels / 20.f));
                }
            }
            Settings.bAttenuate = bHasFalloff;
        }
        else Settings.bAttenuate = false;
    }
    Audio->SetSound(Sound);
    Audio->SetVolumeMultiplier(Action.bUseFadesParameter ? FadesGain(FadesValue) : 1.f);
    Audio->RegisterComponentWithWorld(GetWorld());
    Record->Voices.Add({Audio, Action.TargetId, Action.bUseFadesParameter});
    if (Action.FadeTime > 0.f && StartOffset <= 0.f) Audio->FadeIn(Action.FadeTime, 1.f, StartOffset);
    else Audio->Play(StartOffset);
}

void UMemoriaAudioSubsystem::StopTarget(const FString& TargetId, USceneComponent* Attach, bool bGlobal, float FadeTime)
{
    for (auto& Pair : Playing)
    {
        if (!bGlobal && Pair.Value.AttachComponent.Get() != Attach) continue;
        for (FMemoriaVoice& Voice : Pair.Value.Voices)
        {
            if ((TargetId.IsEmpty() || Voice.TargetId == TargetId) && Voice.Component.IsValid())
            {
                if (FadeTime > 0.f) Voice.Component->FadeOut(FadeTime, 0.f);
                else Voice.Component->Stop();
            }
        }
    }
}

void UMemoriaAudioSubsystem::StopEvent(int32 Id, float FadeTime)
{
    FMemoriaPlayingEvent Record;
    if (!Playing.RemoveAndCopyValue(Id, Record)) return;
    for (FTimerHandle Handle : Record.Timers) GetWorld()->GetTimerManager().ClearTimer(Handle);
    for (const FMemoriaVoice& Voice : Record.Voices)
    {
        if (!Voice.Component.IsValid()) continue;
        if (FadeTime > 0.f) Voice.Component->FadeOut(FadeTime, 0.f);
        else Voice.Component->Stop();
    }
}

void UMemoriaAudioSubsystem::SetPaused(int32 Id, bool bPaused)
{
    if (FMemoriaPlayingEvent* Record = Playing.Find(Id))
    {
        for (const FMemoriaVoice& Voice : Record->Voices) if (Voice.Component.IsValid()) Voice.Component->SetPaused(bPaused);
        for (FTimerHandle Handle : Record->Timers)
        {
            if (bPaused) GetWorld()->GetTimerManager().PauseTimer(Handle);
            else GetWorld()->GetTimerManager().UnPauseTimer(Handle);
        }
    }
}

void UMemoriaAudioSubsystem::SetParameter(const FString& Name, float Value)
{
    if (!Name.Equals(TEXT("FADES"), ESearchCase::IgnoreCase)) return;
    FadesValue = Value;
    for (auto& Pair : Playing) for (FMemoriaVoice& Voice : Pair.Value.Voices)
        if (Voice.bUseFades && Voice.Component.IsValid()) Voice.Component->SetVolumeMultiplier(FadesGain(Value));
}

void UMemoriaAudioSubsystem::Tick(float DeltaTime)
{
    TArray<int32> Finished;
    for (const auto& Pair : Playing)
    {
        bool bActive = false;
        for (const FMemoriaVoice& Voice : Pair.Value.Voices) bActive |= Voice.Component.IsValid() && (Voice.Component->IsPlaying() || Voice.Component->bIsPaused);
        for (FTimerHandle Handle : Pair.Value.Timers) bActive |= GetWorld()->GetTimerManager().TimerExists(Handle);
        if (!bActive) Finished.Add(Pair.Key);
    }
    for (int32 Id : Finished)
    {
        FMemoriaPlayingEvent Record;
        if (!Playing.RemoveAndCopyValue(Id, Record)) continue;
        if (Record.bCallbackRequested && Record.Callback.IsBound())
        {
            UMemoriaAudioCallbackInfo* Info = NewObject<UMemoriaAudioCallbackInfo>(this);
            Info->PlayingID = Id;
            Info->AkComponent = Cast<UMemoriaAudioComponent>(Record.AttachComponent.Get());
            Record.Callback.ExecuteIfBound(EMemoriaCallbackType::EndOfEvent, Info);
        }
    }
}

void UMemoriaAudioSubsystem::Deinitialize()
{
    TArray<int32> Ids;
    Playing.GetKeys(Ids);
    for (int32 Id : Ids) StopEvent(Id);
    Super::Deinitialize();
}

UMemoriaAudioComponent::UMemoriaAudioComponent() { PrimaryComponentTick.bCanEverTick = false; }

int32 UMemoriaAudioComponent::PostAkEvent(UMemoriaAudioEvent* AkEvent, int32 CallbackMask, const FMemoriaPostEventCallback& PostEventCallback, const FString& in_EventName)
{
    if (!AkEvent) AkEvent = AkAudioEvent;
    if (!GetWorld() || !AkEvent) return 0;
    const int32 Id = GetWorld()->GetSubsystem<UMemoriaAudioSubsystem>()->PlayEvent(AkEvent, this, 0.f, StopWhenOwnerDestroyed, CallbackMask, PostEventCallback);
    PlayingIds.Add(Id);
    return Id;
}
int32 UMemoriaAudioComponent::PostAssociatedAkEvent(int32 CallbackMask, const FMemoriaPostEventCallback& Callback)
{ return PostAkEvent(AkAudioEvent, CallbackMask, Callback, EventName); }
void UMemoriaAudioComponent::Stop()
{
    if (GetWorld()) if (UMemoriaAudioSubsystem* Audio = GetWorld()->GetSubsystem<UMemoriaAudioSubsystem>())
        for (int32 Id : PlayingIds) Audio->StopEvent(Id);
    PlayingIds.Reset();
}
void UMemoriaAudioComponent::SetRTPCValue(FString RTPC, float Value, int32 InterpolationTimeMs)
{ if (GetWorld()) GetWorld()->GetSubsystem<UMemoriaAudioSubsystem>()->SetParameter(RTPC, Value); }
void UMemoriaAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{ if (StopWhenOwnerDestroyed) Stop(); Super::EndPlay(Reason); }

AMemoriaAmbientSound::AMemoriaAmbientSound()
{
    AkComponent = CreateDefaultSubobject<UMemoriaAudioComponent>(TEXT("AkAudioComponent0"));
    RootComponent = AkComponent;
}
void AMemoriaAmbientSound::BeginPlay()
{ Super::BeginPlay(); AkComponent->StopWhenOwnerDestroyed = StopWhenOwnerIsDestroyed; if (AutoPost) StartAmbientSound(); }
void AMemoriaAmbientSound::StartAmbientSound() { AkComponent->PostAssociatedAkEvent(0, FMemoriaPostEventCallback()); }
void AMemoriaAmbientSound::StopAmbientSound() { AkComponent->Stop(); }
bool UMemoriaAudioLibrary::SetCurrentLanguageEnglish() { FInternationalization::Get().SetCurrentCulture(TEXT("en")); FAkAudioDevice* Audio = FAkAudioDevice::Get(); return Audio && Audio->SetCurrentAudioCulture(TEXT("English(US)"), FAkAudioDevice::EAudioCultureType::Wwise); }
bool UMemoriaAudioLibrary::SetCurrentLanguageSpanish() { FInternationalization::Get().SetCurrentCulture(TEXT("es")); FAkAudioDevice* Audio = FAkAudioDevice::Get(); return Audio && Audio->SetCurrentAudioCulture(TEXT("Spanish"), FAkAudioDevice::EAudioCultureType::Wwise); }
bool UMemoriaAudioLibrary::IsSpanish() { return FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName() == TEXT("es"); }
bool UMemoriaXRFunctionLibrary::GetUserPresent()
{
    if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()) return false;
    bool bUseFocus = false, bHasFocus = false;
    UHeadMountedDisplayFunctionLibrary::GetVRFocusState(bUseFocus, bHasFocus);
    if (bUseFocus) return bHasFocus;
    return UHeadMountedDisplayFunctionLibrary::GetHMDWornState() != EHMDWornState::NotWorn;
}
void UMemoriaXRFunctionLibrary::FadeToColor(float FadeTime, FLinearColor FadeColor, bool background)
{
    if (!GEngine) return;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        UWorld* World = Context.World();
        if (!World || !World->IsGameWorld()) continue;
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (PC && PC->PlayerCameraManager)
            PC->PlayerCameraManager->StartCameraFade(PC->PlayerCameraManager->FadeAmount, FadeColor.A,
                FMath::Max(FadeTime, 0.001f), FadeColor, false, FadeColor.A > 0.f);
    }
}

void UMemoriaXRFunctionLibrary::SetSplashScreen(UTexture* Texture, FVector2D Scale, FVector Offset, bool bShowLoadingMovie, bool bShowOnSet)
{ UXRLoadingScreenFunctionLibrary::SetLoadingScreen(Texture, Scale, Offset, bShowLoadingMovie, bShowOnSet); }
void UMemoriaXRFunctionLibrary::ShowSplashScreen() { UXRLoadingScreenFunctionLibrary::ShowLoadingScreen(); }
void UMemoriaXRFunctionLibrary::HideSplashScreen() { UXRLoadingScreenFunctionLibrary::HideLoadingScreen(); }
void UMemoriaXRFunctionLibrary::EnableAutoLoadingSplashScreen(bool InAutoShowEnabled)
{
    static FDelegateHandle PreLoadHandle, PostLoadHandle;
    if (PreLoadHandle.IsValid()) FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadHandle);
    if (PostLoadHandle.IsValid()) FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
    PreLoadHandle.Reset(); PostLoadHandle.Reset();
    if (InAutoShowEnabled)
    {
        PreLoadHandle = FCoreUObjectDelegates::PreLoadMap.AddLambda([](const FString&) { ShowSplashScreen(); });
        PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddLambda([](UWorld*) { HideSplashScreen(); });
    }
}
