using UnrealBuildTool;
using System.Collections.Generic;

public class AbyssLockTarget : TargetRules
{
    public AbyssLockTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AbyssLock");
    }
}
