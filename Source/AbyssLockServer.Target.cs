using UnrealBuildTool;
using System.Collections.Generic;

public class AbyssLockServerTarget : TargetRules
{
    public AbyssLockServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AbyssLock");
    }
}
