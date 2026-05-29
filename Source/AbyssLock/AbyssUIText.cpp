#include "AbyssUIText.h"

#include "Internationalization/StringTableRegistry.h"

#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"

namespace
{
// Logical ID + namespace of the shared UI string table. English is the source/native culture;
// JA and zh-Hans are produced as translations against these keys. Keys are stable contracts —
// rename a key only with a matching translation update.
const TCHAR* const GTableId = TEXT("ST_FrostwakeUI");

void EnsureRegistered()
{
    // Register once per process. The UI is built on the game thread, so a plain static guard is
    // enough; the table then persists across PIE sessions for the life of the module.
    static bool bRegistered = false;
    if (bRegistered)
    {
        return;
    }
    bRegistered = true;

    LOCTABLE_NEW("ST_FrostwakeUI", "FrostwakeUI");

    // --- HUD: header / objective / controls ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_TitlePractice", "Frostwake - Practice");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_RoleCrew", "Role: Crew");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_Objective", "Objective: advance the route, keep ship systems online");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_Controls", "[E] Interact   [Scroll] Select item   [Q] Drop");

    // --- HUD: hotbar / inventory (item tokens themselves are gameplay IDs, not localized yet) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ItemsNone", "Items: (none)");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ItemsEmpty", "Items: (empty)");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_FmtItems", "Items:{Items}");

    // --- HUD: vitals gauges ({Percent} kept as a named arg so translations can reorder) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_Vitals", "Vitals");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_VitalHealth", "Health");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_VitalFood", "Food");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_VitalWarmth", "Warmth");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_FmtGauge", "{Label}  {Percent}%");

    // --- HUD: route-to-goal ({Suffix} is the optional Final Approach tail, separately localizable) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_FmtRouteToGoal", "Route to Goal  {Percent}%{Suffix}");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_RouteFinalApproachSuffix", "  - Final Approach!");

    // --- HUD: dynamic role line (owner-only secret team; the Madman alone sees their true role) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_RoleSaboteur", "Role: Saboteur");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_RoleMadman", "Role: Madman");

    // --- HUD: end-of-match result panel (Victory/Defeat + a human-readable reason) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ResultVictory", "VICTORY");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ResultDefeat", "DEFEAT");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ResultEnded", "The match has ended.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ReasonFinalApproach", "The ship reached its destination.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ReasonTimerExpired", "Time ran out before the voyage was complete.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ReasonCrewIncapacitated", "The crew could no longer keep the ship underway.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ReasonFatalShip", "A critical ship system failed.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Hud_ReasonCrewThreshold", "Too few crew remained.");

    // --- Front-end menu: buttons (EN source; existing JP becomes the 'ja' translation) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_Play", "Play");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_SoloPractice", "Solo Practice");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_HostLobby", "Host Lobby");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_JoinLobby", "Join Lobby");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_StartMatch", "Start Match");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_StartSolo", "Start Voyage");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_ConfigPrompt", "Choose your match settings.");

    // --- Front-end menu: status lines + fields ({Required}/{Current} from AbyssLobbyRequiredPlayers) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_LobbyChoicePrompt", "Host a lobby or join an existing one.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_FmtLobbyReady", "All {Required} players are ready. The host can start.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_FmtLobbyWaiting", "Waiting for {Required} players...");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_WaitingForHost", "Waiting for the host to start.");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_FmtPlayerCount", "{Current} / {Required}");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_AddressHint", "Server address");

    // --- Front-end: host match-config selectors (mode + difficulty; {Value} is the chosen option) ---
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_FmtMode", "Mode: {Value}");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Menu_FmtDifficulty", "Difficulty: {Value}");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Mode_Standard", "Standard");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Mode_Madman", "Madman");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Difficulty_Easy", "Easy");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Difficulty_Normal", "Normal");
    LOCTABLE_SETSTRING("ST_FrostwakeUI", "Difficulty_Hard", "Hard");
}
}

namespace AbyssUIText
{
FText Text(const TCHAR* Key)
{
    EnsureRegistered();
    return FText::FromStringTable(GTableId, Key);
}

FSlateFontInfo UiFont(float Size)
{
    const int32 IntSize = FMath::Max(1, FMath::RoundToInt(Size));

    // Build the composite font once and cache it. FStandaloneCompositeFont is an FGCObject that keeps
    // the loaded font faces referenced, so a static TSharedPtr is safe against GC.
    static TSharedPtr<FStandaloneCompositeFont> Composite;
    static bool bTried = false;
    if (!bTried)
    {
        bTried = true;
        const UObject* JpFace = LoadObject<UObject>(nullptr, TEXT("/Game/ThirdParty/Quarantine/fonts/NotoSansJP/NotoSansJP-VF.NotoSansJP-VF"));
        const UObject* ScFace = LoadObject<UObject>(nullptr, TEXT("/Game/ThirdParty/Quarantine/fonts/NotoSansSC/NotoSansSC-VF.NotoSansSC-VF"));
        if (JpFace)
        {
            TSharedRef<FStandaloneCompositeFont> Built = MakeShared<FStandaloneCompositeFont>();
            // Default typeface: Noto Sans JP (covers Latin + Japanese).
            FTypefaceEntry& DefaultEntry = Built->DefaultTypeface.Fonts.AddDefaulted_GetRef();
            DefaultEntry.Name = TEXT("Regular");
            DefaultEntry.Font = FFontData(JpFace);
            // zh-Hans sub-font: Noto Sans SC (Simplified Chinese glyph forms).
            if (ScFace)
            {
                FCompositeSubFont& Sub = Built->SubTypefaces.AddDefaulted_GetRef();
                Sub.Cultures = TEXT("zh-Hans");
                FTypefaceEntry& SubEntry = Sub.Typeface.Fonts.AddDefaulted_GetRef();
                SubEntry.Name = TEXT("Regular");
                SubEntry.Font = FFontData(ScFace);
            }
            Composite = Built;
        }
    }

    if (Composite.IsValid())
    {
        return FSlateFontInfo(Composite, IntSize, TEXT("Regular"));
    }
    return FCoreStyle::GetDefaultFontStyle("Regular", IntSize);
}
}
