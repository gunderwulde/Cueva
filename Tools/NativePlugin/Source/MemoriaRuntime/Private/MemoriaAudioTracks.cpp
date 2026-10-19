#include "MemoriaAudioTracks.h"
#include "EntitySystem/MovieSceneSharedPlaybackState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "GameFramework/Actor.h"

static UWorld* PlaybackWorld(TSharedRef<UE::MovieScene::FSharedPlaybackState> Playback)
{
    UObject* Context = Playback->GetPlaybackContext();
    return Context && GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
}

UMemoriaAudioEventSection::UMemoriaAudioEventSection(const FObjectInitializer& Initializer) : Super(Initializer)
{ bRequiresRangedHook = true; }

void UMemoriaAudioEventSection::Begin(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const
{
    UWorld* World = PlaybackWorld(Playback);
    if (!World || !World->IsGameWorld() || !Event || Params.Context.IsSilent()) return;
    if (Params.Context.GetStatus() != EMovieScenePlayerStatus::Playing) return;
    UMemoriaAudioSubsystem* Audio = World->GetSubsystem<UMemoriaAudioSubsystem>();
    FSectionPlayback& Record = ActivePlayback.FindOrAdd(&Playback.Get());
    Record.Audio = Audio;
    const FFrameTime Start = HasStartFrame() ? FFrameTime(GetInclusiveStartFrame()) : FFrameTime(0);
    const float Offset = FMath::Max(0., Params.Context.GetFrameRate().AsSeconds(Params.Context.GetTime() - Start));
    const auto BoundObjects = Playback->FindBoundObjects(Params.ObjectBindingID, Params.SequenceID);
    for (TWeakObjectPtr<> Weak : BoundObjects)
    {
        UObject* Object = Weak.Get();
        USceneComponent* Attach = Cast<USceneComponent>(Object);
        if (AActor* Actor = Cast<AActor>(Object)) Attach = Actor->GetRootComponent();
        Record.Ids.Add(Audio->PlayEvent(Event, Attach, Offset));
    }
    if (Record.Ids.IsEmpty()) Record.Ids.Add(Audio->PlayEvent(Event, nullptr, Offset));
}

void UMemoriaAudioEventSection::Update(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const
{
    const void* Key = &Playback.Get();
    if (Params.Context.HasJumped())
    {
        if (FSectionPlayback* Existing = ActivePlayback.Find(Key))
            if (Existing->Audio.IsValid()) for (int32 Id : Existing->Ids) Existing->Audio->StopEvent(Id);
        ActivePlayback.Remove(Key);
    }
    if (!ActivePlayback.Contains(Key)) { Begin(Playback, Params); return; }
    const FSectionPlayback& Existing = ActivePlayback.FindChecked(Key);
    if (Existing.Audio.IsValid()) for (int32 Id : Existing.Ids)
        Existing.Audio->SetPaused(Id, Params.Context.GetStatus() != EMovieScenePlayerStatus::Playing);
}

void UMemoriaAudioEventSection::End(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const
{
    FSectionPlayback Record;
    if (!ActivePlayback.RemoveAndCopyValue(&Playback.Get(), Record)) return;
    if (StopAtSectionEnd && Record.Audio.IsValid()) for (int32 Id : Record.Ids) Record.Audio->StopEvent(Id);
}

UMemoriaAudioRTPCSection::UMemoriaAudioRTPCSection(const FObjectInitializer& Initializer) : Super(Initializer)
{
    bRequiresRangedHook = true;
#if WITH_EDITOR
    FMovieSceneChannelMetaData Metadata;
    Metadata.SetIdentifiers(TEXT("Volume"), FText::FromString(TEXT("Volume")));
    ChannelProxy = MakeShared<FMovieSceneChannelProxy>(Channel, Metadata, TMovieSceneExternalValue<float>());
#else
    ChannelProxy = MakeShared<FMovieSceneChannelProxy>(Channel);
#endif
}
void UMemoriaAudioRTPCSection::Begin(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const
{ Update(Playback, Params); }
void UMemoriaAudioRTPCSection::Update(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const
{
    UWorld* World = PlaybackWorld(Playback);
    if (!World || !World->IsGameWorld()) return;
    float Value = 100.f;
    if (Channel.Evaluate(Params.Context.GetTime(), Value)) World->GetSubsystem<UMemoriaAudioSubsystem>()->SetParameter(Name, Value);
}
