using UnrealBuildTool;
using System.IO;

public class AbyssLockEditor : ModuleRules
{
    public AbyssLockEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "AbyssLock"));

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AbyssLock"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetTools",
            "LevelEditor",
            "MaterialEditor",
            "Slate",
            "StandaloneRenderer",
            "UnrealEd"
        });
    }
}
