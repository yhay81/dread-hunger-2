using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeServerTarget : TargetRules
{
    public FrostwakeServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        // See Frostwake.Target.cs: push model can't be pinned with the installed engine (shared build
        // environment with the precompiled engine forces bWithPushModel=False). Requires a source engine.
        ExtraModuleNames.Add("Frostwake");
    }
}
