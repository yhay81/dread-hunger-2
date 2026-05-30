using UnrealBuildTool;
using System.IO;

public class FrostwakeEditor : ModuleRules
{
    public FrostwakeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "Frostwake"));

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Frostwake"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetTools",
            "AssetRegistry",
            "Json",
            "JsonUtilities",
            "LevelEditor",
            "MaterialEditor",
            "Slate",
            "StandaloneRenderer",
            "UnrealEd"
        });
    }
}
