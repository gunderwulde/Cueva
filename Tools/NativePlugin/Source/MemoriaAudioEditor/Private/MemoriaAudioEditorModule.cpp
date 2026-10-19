#include "Modules/ModuleManager.h"
#include "MemoriaAudioTracks.h"
#include "ISequencerModule.h"
#include "ISequencerSection.h"
#include "MovieSceneTrackEditor.h"

class FMemoriaAudioSection : public FSequencerSection
{
public:
    explicit FMemoriaAudioSection(UMovieSceneSection& Section)
        : FSequencerSection(Section), Object(&Section) {}
    virtual FText GetSectionTitle() const override
    {
        if (const auto* Event = Cast<UMemoriaAudioEventSection>(Object.Get()))
            return FText::FromString(Event->Event ? Event->Event->GetName() : Event->EventName);
        if (const auto* Volume = Cast<UMemoriaAudioRTPCSection>(Object.Get()))
            return FText::FromString(Volume->Name);
        return FText::FromString(TEXT("Memoria audio"));
    }
private:
    TWeakObjectPtr<UMovieSceneSection> Object;
};

class FMemoriaAudioTrackEditor : public FMovieSceneTrackEditor
{
public:
    explicit FMemoriaAudioTrackEditor(TSharedRef<ISequencer> Sequencer)
        : FMovieSceneTrackEditor(Sequencer) {}
    static TSharedRef<ISequencerTrackEditor> Create(TSharedRef<ISequencer> Sequencer)
        { return MakeShared<FMemoriaAudioTrackEditor>(Sequencer); }
    virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override
        { return Type->IsChildOf(UMemoriaAudioTrack::StaticClass()); }
    virtual TSharedRef<ISequencerSection> MakeSectionInterface(
        UMovieSceneSection& Section, UMovieSceneTrack& Track, FGuid Binding) override
        { return MakeShared<FMemoriaAudioSection>(Section); }
};

class FMemoriaAudioEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (IsRunningCommandlet()) return;
        auto& Sequencer = FModuleManager::LoadModuleChecked<ISequencerModule>(TEXT("Sequencer"));
        Handle = Sequencer.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FMemoriaAudioTrackEditor::Create));
    }
    virtual void ShutdownModule() override
    {
        if (Handle.IsValid())
            if (auto* Sequencer = FModuleManager::GetModulePtr<ISequencerModule>(TEXT("Sequencer")))
                Sequencer->UnRegisterTrackEditor(Handle);
    }
private:
    FDelegateHandle Handle;
};
IMPLEMENT_MODULE(FMemoriaAudioEditorModule, MemoriaAudioEditor)
