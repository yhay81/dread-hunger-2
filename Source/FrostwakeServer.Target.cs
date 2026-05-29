using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeServerTarget : TargetRules
{
    public FrostwakeServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Frostwake");
    }
}
