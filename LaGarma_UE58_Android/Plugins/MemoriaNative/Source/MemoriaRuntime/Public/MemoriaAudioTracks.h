#pragma once
#include "CoreMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "Sections/MovieSceneHookSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "MemoriaAudio.h"
#include "MemoriaAudioTracks.generated.h"

UCLASS(Abstract)
class MEMORIARUNTIME_API UMemoriaAudioTrack : public UMovieSceneNameableTrack
{
    GENERATED_BODY()
public:
    virtual void RemoveAllAnimationData() override { Sections.Empty(); }
    virtual bool HasSection(const UMovieSceneSection& Section) const override { return Sections.Contains(&Section); }
    virtual void AddSection(UMovieSceneSection& Section) override { Sections.Add(&Section); }
    virtual void RemoveSection(UMovieSceneSection& Section) override { Sections.Remove(&Section); }
    virtual void RemoveSectionAt(int32 Index) override { Sections.RemoveAt(Index); }
    virtual bool IsEmpty() const override { return Sections.IsEmpty(); }
    virtual const TArray<UMovieSceneSection*>& GetAllSections() const override { return Sections; }
    virtual bool SupportsMultipleRows() const override { return true; }
protected:
    // Matches the serialized array in the legacy track.
    UPROPERTY() TArray<UMovieSceneSection*> Sections;
    UPROPERTY() uint32 bIsAMasterTrack : 1;
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioEventSection : public UMovieSceneHookSection
{
    GENERATED_BODY()
public:
    UMemoriaAudioEventSection(const FObjectInitializer& Initializer);
    UPROPERTY(EditAnywhere, Category="Memoria|Audio") TObjectPtr<UMemoriaAudioEvent> Event;
    UPROPERTY(EditAnywhere, Category="Memoria|Audio") FString EventName;
    UPROPERTY(EditAnywhere, Category="Memoria|Audio") bool StopAtSectionEnd = true;
    UPROPERTY(EditAnywhere, Category="Memoria|Audio") bool RetriggerEvent = false;
    virtual void Begin(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const override;
    virtual void Update(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const override;
    virtual void End(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const override;
private:
    struct FSectionPlayback { TWeakObjectPtr<UMemoriaAudioSubsystem> Audio; TArray<int32> Ids; };
    mutable TMap<const void*, FSectionPlayback> ActivePlayback;
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioEventTrack : public UMemoriaAudioTrack
{
    GENERATED_BODY()
public:
    virtual bool SupportsType(TSubclassOf<UMovieSceneSection> Type) const override { return Type == UMemoriaAudioEventSection::StaticClass(); }
    virtual UMovieSceneSection* CreateNewSection() override { return NewObject<UMemoriaAudioEventSection>(this, NAME_None, RF_Transactional); }
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioRTPCSection : public UMovieSceneHookSection
{
    GENERATED_BODY()
public:
    UMemoriaAudioRTPCSection(const FObjectInitializer& Initializer);
    UPROPERTY(EditAnywhere, Category="Memoria|Audio") FString Name;
    UPROPERTY() FMovieSceneFloatChannel Channel;
    virtual void Begin(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const override;
    virtual void Update(TSharedRef<FSharedPlaybackState> Playback, const UE::MovieScene::FEvaluationHookParams& Params) const override;
};

UCLASS()
class MEMORIARUNTIME_API UMemoriaAudioRTPCTrack : public UMemoriaAudioTrack
{
    GENERATED_BODY()
public:
    virtual bool SupportsType(TSubclassOf<UMovieSceneSection> Type) const override { return Type == UMemoriaAudioRTPCSection::StaticClass(); }
    virtual UMovieSceneSection* CreateNewSection() override { return NewObject<UMemoriaAudioRTPCSection>(this, NAME_None, RF_Transactional); }
};
