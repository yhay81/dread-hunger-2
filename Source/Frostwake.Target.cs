using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeTarget : TargetRules
{
    public FrostwakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Frostwake");
    }
}
