using UnrealBuildTool;
public class MemoriaAudioEditor : ModuleRules
{
    public MemoriaAudioEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MemoriaRuntime" });
        PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "InputBlueprintNodes", "InputCore", "Sequencer", "Slate", "SlateCore", "Niagara", "NiagaraEditor", "UnrealEd", "AudioEditor", "AssetRegistry", "Json", "JsonUtilities", "MovieScene", "MovieSceneTracks", "BlueprintGraph", "Kismet", "AkAudio", "WwiseResourceLoader" });
    }
}
