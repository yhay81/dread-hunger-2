using UnrealBuildTool;
using System.Collections.Generic;

public class FrostwakeTarget : TargetRules
{
    public FrostwakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        // NOTE (verified 2026-05-30): push model can NOT be pinned here. The installed/launcher engine's
        // precompiled UnrealGame has bWithPushModel=False, and this Game target shares that build
        // environment — UBT forbids overriding it ("bWithPushModel: True != False") without a SOURCE
        // engine (unique build env). So the AttributeComponent's MARK_PROPERTY_DIRTY annotations fall
        // back to legacy comparison replication (correct, just unoptimized). Activating push model is
        // gated on a source-engine build — same class of limit as the Server-target blocker. See plan §3.4.
        ExtraModuleNames.Add("Frostwake");
    }
}
