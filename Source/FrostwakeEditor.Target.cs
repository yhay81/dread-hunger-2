using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeEditorTarget : TargetRules
{
    public FrostwakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Frostwake");
        ExtraModuleNames.Add("FrostwakeEditor");
    }
}
