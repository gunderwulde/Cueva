using UnrealBuildTool;
public class MemoriaRuntime : ModuleRules
{
    public MemoriaRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MovieScene", "MovieSceneTracks" });
        PrivateDependencyModuleNames.AddRange(new string[] { "XRBase", "HeadMountedDisplay", "AkAudio" });
    }
}
