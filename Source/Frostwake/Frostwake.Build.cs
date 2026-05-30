using UnrealBuildTool;

public class Frostwake : ModuleRules
{
    public Frostwake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Make the module root an include root so subfolder headers resolve as
        // "Data/FrostwakeItemDefinition.h" / "ActionSystem/FrostwakeAttributeComponent.h"
        // (foundation scaffold, plan §9.1). Existing flat headers keep working unchanged.
        PublicIncludePaths.Add(ModuleDirectory);

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
            "OnlineSubsystemUtils",
            // Modern foundation (Phase 1, plan §9.1-0). GameplayTags + DataRegistry are public
            // because definition headers expose FGameplayTag / registry types. EnhancedInput is
            // wired in here (plan §3.2 Tier 1); the input *migration* in the character is a
            // separate Phase 2 step. StructUtils (FInstancedStruct) is in CoreUObject since UE5.5.
            "GameplayTags",
            "EnhancedInput",
            "DataRegistry"
        });

        // Json / JsonUtilities (already public above) back the data-ingestion path: JSON text
        // source -> typed UFrostwake*Definition at runtime (plan §9.2).
        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
