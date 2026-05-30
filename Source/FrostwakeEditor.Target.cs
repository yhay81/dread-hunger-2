using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeEditorTarget : TargetRules
{
    public FrostwakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        // Push model intentionally NOT pinned here: the editor target *can* take bWithPushModel=true, but
        // the shipping Game/Server targets cannot (installed-engine shared env, see Frostwake.Target.cs).
        // Keeping editor/PIE consistent with the game build avoids "works in PIE, off when shipped" drift.
        ExtraModuleNames.Add("Frostwake");
        ExtraModuleNames.Add("FrostwakeEditor");
    }
}
