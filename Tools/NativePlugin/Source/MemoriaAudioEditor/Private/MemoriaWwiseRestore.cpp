#include "MemoriaAudioEditorLibrary.h"
#include "AkAudioType.h"
#include "AkAudioEvent.h"
#include "AkAudioDevice.h"
#include "AkInitBank.h"
#include "AkRtpc.h"
#include "MovieSceneAkTrack.h"
#include "MovieSceneAkAudioEventSection.h"
#include "MovieSceneWwiseGameParameterSection.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

UObject* UMemoriaAudioEditorLibrary::CreateWwiseAsset(const FString& AssetPath, const FString& ClassPath)
{
    if (UObject* Existing = LoadObject<UObject>(nullptr, *AssetPath, nullptr, LOAD_NoWarn)) return Existing;
    UClass* Class = LoadClass<UAkAudioType>(nullptr, *ClassPath);
    if (!Class || !FPackageName::IsValidLongPackageName(AssetPath)) return nullptr;
    UPackage* Package = CreatePackage(*AssetPath);
    UObject* Asset = NewObject<UObject>(Package, Class, *FPackageName::GetLongPackageAssetName(AssetPath), RF_Public | RF_Standalone);
    FAssetRegistryModule::AssetCreated(Asset);
    Asset->MarkPackageDirty();
    return Asset;
}

bool UMemoriaAudioEditorLibrary::InitializeWwiseAsset(UObject* Asset, const FString& Guid, const FString& Name)
{
    UAkAudioType* Audio = Cast<UAkAudioType>(Asset);
    if (!Audio) return false;
    Audio->Modify();
    Audio->bAutoLoad = true;
    if (UAkAudioEvent* Event = Cast<UAkAudioEvent>(Audio)) Event->RequiredBank_DEPRECATED = nullptr;
    if (UAkInitBank* Init = Cast<UAkInitBank>(Audio))
    {
        Init->PrepareCookedData();
        Audio->MarkPackageDirty();
        return true;
    }
    FWwiseObjectInfo* Info = Audio->GetInfoMutable();
    if (!Info || !FGuid::Parse(Guid, Info->WwiseGuid)) return false;
    Info->WwiseName = *Name;
    Info->WwiseShortId = FAkAudioDevice::GetShortIDFromString(Name);
    Audio->FillInfo();
    Audio->MarkPackageDirty();
    const bool Found = Audio->ObjectIsInSoundBanks();
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_WWISE_ASSET %s guid=%s found=%d"), *Asset->GetPathName(), *Info->WwiseGuid.ToString(), Found);
    return Found;
}

bool UMemoriaAudioEditorLibrary::RepairWwiseTrack(UObject* Object, bool MasterTrack)
{
    UMovieSceneAkTrack* Track = Cast<UMovieSceneAkTrack>(Object);
    if (!Track) return false;
    Track->Modify();
    Track->SetIsAMasterTrack(MasterTrack);
    Track->MarkPackageDirty();
    return true;
}

bool UMemoriaAudioEditorLibrary::SetWwiseAudioSection(UObject* Object, UObject* AudioAsset, bool StopAtEnd, bool Retrigger)
{
    Object->Modify();
    if (UMovieSceneAkAudioEventSection* Section = Cast<UMovieSceneAkAudioEventSection>(Object))
    {
        UAkAudioEvent* Event = Cast<UAkAudioEvent>(AudioAsset);
        if (!Event) return false;
        Section->SetEvent(Event);
        Section->RetriggerEvent = Retrigger;
        FBoolProperty* Stop = FindFProperty<FBoolProperty>(Section->GetClass(), TEXT("StopAtSectionEnd"));
        if (!Stop) return false;
        Stop->SetPropertyValue_InContainer(Section, StopAtEnd);
        Section->MarkPackageDirty();
        return Section->GetEvent() == Event;
    }
    if (UMovieSceneWwiseGameParameterSection* Section = Cast<UMovieSceneWwiseGameParameterSection>(Object))
    {
        UAkRtpc* Parameter = Cast<UAkRtpc>(AudioAsset);
        if (!Parameter) return false;
        Section->SetGameParameter(Parameter);
        Section->MarkPackageDirty();
        return true;
    }
    return false;
}


int32 UMemoriaAudioEditorLibrary::GetWwiseCurveKeyCount(UObject* Object)
{
    const UMovieSceneWwiseGameParameterSection* Section = Cast<UMovieSceneWwiseGameParameterSection>(Object);
    if (!Section || !Section->GetGameParameter() || Section->GetGameParameter()->GetInfo().WwiseName != TEXT("FADES")) return -1;
    return Section->GetChannel().GetNumKeys();
}

FString UMemoriaAudioEditorLibrary::SmokeTestWwisePlayback()
{
    FAkAudioDevice* Device = FAkAudioDevice::Get();
    if (!Device || !Device->IsInitialized()) return TEXT("{\"initialized\":false}");
    UAkAudioEvent* Effect = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/Audio/Hotspot.Hotspot"));
    UAkAudioEvent* Voice = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/Audio/DX_B_1.DX_B_1"));
    if (!Effect || !Voice) return TEXT("{\"initialized\":true,\"missing_assets\":true}");
    auto Play = [Device](UAkAudioEvent* Event) -> int32
    {
        Event->LoadData();
        const int32 Id = Event->PostOnActor(nullptr, FOnAkPostEventCallback(), 0, true);
        if (Id) Device->StopPlayingID(Id);
        return Id;
    };
    const bool English = Device->SetCurrentAudioCulture(TEXT("English(US)"), FAkAudioDevice::EAudioCultureType::Wwise);
    const int32 EnglishEffect = Play(Effect), EnglishVoice = Play(Voice);
    const bool Spanish = Device->SetCurrentAudioCulture(TEXT("Spanish"), FAkAudioDevice::EAudioCultureType::Wwise);
    const int32 SpanishEffect = Play(Effect), SpanishVoice = Play(Voice);
    Device->SetCurrentAudioCulture(TEXT("English(US)"), FAkAudioDevice::EAudioCultureType::Wwise);
    return FString::Printf(TEXT("{\"initialized\":true,\"english_changed\":%s,\"spanish_changed\":%s,\"english_effect_id\":%d,\"english_voice_id\":%d,\"spanish_effect_id\":%d,\"spanish_voice_id\":%d}"),
        English ? TEXT("true") : TEXT("false"), Spanish ? TEXT("true") : TEXT("false"), EnglishEffect, EnglishVoice, SpanishEffect, SpanishVoice);
}