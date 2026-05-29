using UnrealBuildTool;

public class Frostwake : ModuleRules
{
    public Frostwake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "NetCore",
            "HTTP",
            "Json",
            "JsonUtilities",
            "Slate",
            "SlateCore",
            "UMG",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
