# GP-09 First-Match Comprehension Checklist

Authored: 2026-05-29 · Source: `docs/game-design.md`, `docs/accessibility-baseline.md`,
`docs/localization-glossary.md`, `Source/AbyssLock/AbyssShipTaskActor.cpp`,
`Source/AbyssLock/AbyssMainMenuWidget.cpp`

This checklist defines the three questions a first-time player should be able to answer
after completing one match, with or without external coaching. Each item maps to the
current in-game signal that is supposed to provide the answer, and notes any gap.

The Phase 1 bar (`docs/commercial-quality-target.md`) is: players can construct a useful
keep/cut/change list after one match. The Phase 4 Early Access bar is: 70% objective
comprehension after one match. This checklist operationalizes both.

---

## Part A — First-Match Comprehension Checklist

### Question 1: Team goal — "What does your team need to accomplish to win?"

**Target answer (crew):** Keep the ship systems operational and complete route progress to
break through the ice route. Survive until the final approach is reached with at least five
crew aboard.

**Target answer (agent):** Stop the crew from completing the route. Reduce the surviving
crew count to three or fewer players, or drive the ship into a fatal state before the final
approach completes.

**Source in `docs/game-design.md`:**
- チーム section (乗員 / 工作員 goals).
- 勝利条件の実装しきい値: 8 players, 2 agents; crew win requires 5 survivors, agent win
  requires ≤3 crew survivors.
- コアシステム: 航路 is the crew's primary win-progress system.

**Current in-game signal providing the answer:**
- Ship task interaction prompts: `"Advance Route Progress"` (LOCTEXT key
  `AdvanceRouteProgressInteraction` in `AbyssShipTaskActor.cpp`). This is the highest-
  clarity signal for crew objective.
- Repair prompts: `"Repair Hull"`, `"Repair Fuel"`, etc. (`ShipTaskInteractionFormat`).
- No in-match HUD currently shows team assignment, route progress bar, or win-condition
  counter. The answer is inferred from task prompts only.

**Gap:** No displayed win-condition summary or role card at match start. New players who
have not read docs have no explicit statement of the team goal. Agent role is not
communicated to agents via any current UI text. Route progress percentage is not visible
on the HUD. This is the highest-severity comprehension gap for Phase 1.

**Observer check:** After one match, ask the player: "What did your team need to do to
win?" Score as pass if they mention route completion (crew) or stopping the crew / reducing
survivors (agent).

---

### Question 2: Personal next step — "After the opening minute, what is your highest-value action?"

**Target answer (crew):** Find and start a ship-task repair or route-progress task. Avoid
the ship systems in the worst state (hull damage, flooding, power outage). Note who moves
together and who splits off.

**Target answer (agent):** Use an available sabotage option (fuel contamination, generator
stop, hatch freeze, radio frequency shift, or route marker spoof — per `docs/game-design.md`
初期妨害) without being observed, then re-join the crew group with a plausible explanation.

**Source in `docs/game-design.md`:**
- マッチ進行 steps 3-5: ship systems start unstable; crew repairs and advances route; agents
  sabotage and divide.
- 初期妨害: five specific sabotage options available from the start.
- 白箱で削る基準: tasks must create conversation, not solo-grind.

**Current in-game signal providing the answer:**
- `AbyssShipTaskActor.cpp` interaction prompts: `"Repair Hull"`, `"Sabotage Fuel"`, etc.
  These are LOCTEXT-wrapped and glossary-consistent.
- No tutorial overlay, minimap, or role card directs players to the nearest relevant task.
- `AbyssLifeActionActor.cpp` uses `"Assist"` (not `"Rescue"` or `"Contain"`) — this does
  not communicate which life action is being offered, leaving the player uncertain of the
  consequence.

**Gap:** Players learn the available tasks only by wandering into interactive actors. The
`"Assist"` label is the second-highest wording gap: the glossary defines `rescue` (救助)
and `containment` (拘束) as distinct states with different strategic meanings, but the UI
collapses both into a single generic label. A player who rescues when they meant to contain
(or vice versa) cannot attribute the outcome to their choice.

**Observer check:** Ask after round 1: "Without coaching, what did you do in the first
minute and why?" Score as pass if they can name a task category (repair, route, sabotage)
or describe a decision based on observed ship state.

---

### Question 3: Loss reason — "Why did the match end the way it did?"

**Target answer (crew loss):** The ship reached a fatal state (hull failure, flooding past
threshold, power/route failure chain) before the final approach completed, or too many crew
were downed/contained.

**Target answer (agent win reading):** The agents slowed route progress enough and reduced
crew count to ≤3 survivors; OR the ship reached a critical failure state.

**Target answer (crew win):** The final approach completed with at least 5 survivors aboard
before the agents could trigger a fatal system failure or eliminate enough crew.

**Source in `docs/game-design.md`:**
- 勝利条件の実装しきい値 table: 8p, 2 agents, crew need 5 survivors, agents need ≤3 crew.
- マッチ進行 step 7: 最終航行または船の致命状態で勝敗を決定する.
- 死亡と拘束: ダウン → 救助 → 拘束 → 完全死亡 priority chain.
- Tie-breaking: simultaneous win conditions favor ship-fatal / agent win over crew win.

**Current in-game signal providing the answer:**
- No end-of-match result screen or loss-reason text exists in the current build. Players
  observe that the match ended and the widget closes (`EnterGameInputAndClose()` in
  `AbyssMainMenuWidget.cpp`), but no message states why.
- No post-match role reveal is currently implemented.

**Gap:** This is a critical comprehension blocker for the Phase 1 bar. After one match,
players cannot distinguish between a ship-fatal loss (a system failed) and an attrition
loss (not enough survivors). Without a result screen, the QA observer must reconstruct the
reason from match logs — new players cannot do this themselves.

**Observer check:** After the match ends, ask: "Why did the match end, and who won?" Score
as pass only if the player can name the mechanism (route completed / ship failed / too few
survivors) without the observer explaining it.

---

## Summary: Gap Priority for Phase 1

| Priority | Gap | Source file | Recommended first step |
| --- | --- | --- | --- |
| 1 (critical) | No end-of-match result / loss-reason display | No current file — new UI needed | Add a minimal result overlay (win condition met + reason) before P1-024 human run |
| 2 (high) | No role card or team-goal display at match start | No current file — new UI needed | Add a 3-second role-reveal overlay showing `乗員` / `工作員` + one-sentence goal at match begin |
| 3 (high) | `"Assist"` label does not distinguish rescue from containment | `AbyssLifeActionActor.cpp` line 14 | Change label to `"Rescue"` or `"Contain"` based on target player state (server-authoritative read) |
| 4 (medium) | No route-progress indicator on HUD | No current HUD file | Add a numeric or bar display of route progress percentage |
| 5 (medium) | `AbyssMainMenuWidget.cpp` strings are hardcoded Japanese with no LOCTEXT namespace | `AbyssMainMenuWidget.cpp` lines 118-126, 148, 162, 188, 206-214 | Wrap in `LOCTEXT_NAMESPACE "AbyssMainMenuWidget"` with glossary-consistent keys |

---

## Part B — Japanese Font Plan

### Problem

`AbyssMainMenuWidget.cpp` renders all button labels and lobby status strings in Japanese
(`"ゲーム開始"`, `"一人モード"`, `"ロビーを開く"`, `"ロビーに入る"`, `"開始"`,
`"8人揃いました。ホストが開始できます。"`, etc.) using the default Slate font, which
inherits the engine default (Roboto). Roboto does not include Japanese (CJK) glyphs, so
these strings render as blank boxes or question marks in shipping builds and on machines
without system JP fonts installed.

This was confirmed by reading `AbyssMainMenuWidget.cpp` directly: `MakeText` calls
`TextBlock->GetFont()` and sets only the size, leaving typeface as the engine default.

### Recommended Font: Noto Sans JP

**Why Noto Sans JP:**
- License: SIL Open Font License 1.1 (OFL). Commercial use, modification, and bundling
  are permitted. No attribution required in the UI, but the license file must ship with
  the font and be listed in `docs/asset-ledger.md`.
- Coverage: Full Hiragana, Katakana, CJK Unified Ideographs, Latin, and basic symbols —
  covers all current JP strings plus any future EN/JP bilingual UI.
- Weight range: Thin through Black (100-900). The menu uses 24-44px sizes; Regular (400)
  and Medium (500) are clean at those sizes.
- Distribution source: Google Fonts (fonts.google.com/noto/specimen/Noto+Sans+JP) and the
  Noto project GitHub (github.com/notofonts/noto-cjk). Both are primary sources with
  stable versioning.
- IP safety: Noto fonts are designed to have no similarity to proprietary font faces.
  No IP review concern.

**Alternative considered: M PLUS 1p (OFL, lighter file size).** Valid fallback if the
Noto file size (the full JP set is ~4 MB for the variable font, ~1 MB per weight for
static) is a concern. Both are OFL. Noto Sans JP is recommended because it is better
tested at small sizes and more familiar to Japanese players.

### Unreal Engine Wiring Approach

#### Step 1 — Add the font file to the project

Place the `.ttf` or `.otf` files in `Content/Fonts/NotoSansJP/` (create the directory).
Suggested static weights: `NotoSansJP-Regular.ttf` and `NotoSansJP-Medium.ttf`.
Record both in `docs/asset-ledger.md` with license (OFL 1.1), source URL, and version.
The font files are binary and should be committed via Git LFS if LFS is enabled.

#### Step 2 — Create a UE Font asset

In the Unreal Editor, create a new Font asset at
`Content/Fonts/NotoSansJP/F_NotoSansJP.uasset`:
- Asset type: `Font` (not a Font Face — a composite Font that holds the font face
  reference).
- Add the Regular font face as the Default typeface subface.
- Add the Medium font face as an additional subface named `Medium`.

Alternatively, create two `Font Face` assets (one per `.ttf`) and reference them in the
composite Font. The composite Font is what `FSlateFontInfo` references at runtime.

#### Step 3 — Reference the font in Slate / UMG C++

In `AbyssMainMenuWidget.cpp`, the `MakeText` helper sets `FSlateFontInfo` via
`TextBlock->GetFont()`. To apply Noto Sans JP, load the Font asset and assign its
typeface:

```cpp
// At the top of AbyssMainMenuWidget.cpp (or in a shared helper header):
static const FSlateFontInfo GetNotoSansJPFont(float InSize)
{
    // Load the composite Font asset created in Step 2.
    const UFont* NotoFont = LoadObject<UFont>(
        nullptr,
        TEXT("/Game/Fonts/NotoSansJP/F_NotoSansJP"));
    FSlateFontInfo FontInfo;
    if (NotoFont)
    {
        FontInfo = FSlateFontInfo(NotoFont, InSize);
    }
    else
    {
        // Fallback: engine default so the widget does not crash if the asset is missing.
        FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", InSize);
    }
    return FontInfo;
}
```

Then in `MakeText`, replace:

```cpp
FSlateFontInfo Font = TextBlock->GetFont();
Font.Size = FontSize;
TextBlock->SetFont(Font);
```

with:

```cpp
TextBlock->SetFont(GetNotoSansJPFont(FontSize));
```

This is the minimum wiring. No Blueprint change is required because the widget is code-only.

#### Step 4 — Verify rendering

After wiring, open the Editor and use `Play In Editor` → start screen. Confirm that
`"ゲーム開始"` and `"一人モード"` render with visible Japanese glyphs at the expected
weight and size. Take a screenshot into `Saved/Screenshots/` for owner review.

If glyphs still show as squares, confirm: (a) the Font asset path matches the
`LoadObject` path exactly, (b) the `.ttf` file was imported (not just copied to disk),
and (c) the Font Face asset inside the composite Font has the correct `.uasset` reference.

#### Step 5 — LOCTEXT wrapping prerequisite

The JP-font wiring makes the existing hardcoded strings render correctly. Wrapping those
strings in `LOCTEXT_NAMESPACE "AbyssMainMenuWidget"` (backlog item in GP-09.state.md)
is a separate subsequent step — it does not affect font rendering but is required before
localization batches can be sent to a translator.

### Asset Ledger Entry (to add when font is imported)

```
Name: Noto Sans JP (Regular, Medium)
Type: Font
License: SIL Open Font License 1.1
Source: https://fonts.google.com/noto/specimen/Noto+Sans+JP
Version: (record version downloaded)
Commercial use: Yes
Redistribution: Yes (with license file)
Attribution required in UI: No
UE path: Content/Fonts/NotoSansJP/
Ledger status: Quarantine → Approved after IP Review confirms OFL compliance
```

---

## Verification

This document is a docs-only deliverable.
Verification command: `cargo run -p frostwake-tools -- quality-gate`
The quality gate checks that the file exists and passes doc hygiene; it does not run the
UE build or test font rendering (those require Editor). The JP-font wiring steps are
written as implementation instructions for the Unreal C++ owner, not executed here.

Reviewer required: Localization (string gaps) + Game Design (comprehension gap priorities).
Do not integrate the wording or UI changes in Part A into C++ until reviewed.
