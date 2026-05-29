# GP-09 JP Localization + Font Plan (HUD + menu)

Goal: make the HUD/menu render natural Japanese (the menu already has hardcoded JP that currently
shows as tofu because the engine default font has no CJK glyphs). Produced by a read-only design
agent against the current widgets. Two orthogonal pieces: (1) own the UI strings, (2) import an
OFL Japanese font and apply it. IP-safe: OFL/CC0 font only, provenance recorded, quarantine until IP
Review; **no unattended download — needs owner go-ahead to fetch.**

## 1. String mapping

### HUD (`Source/AbyssLock/AbyssHudWidget.cpp`)
| English (current) | Japanese |
| --- | --- |
| Frostwake - Practice | Frostwake - 練習 |
| Role: Crew | 役割：乗員 |
| Objective: advance the route, keep ship systems online | 目標：航路を進め、船の設備を稼働させ続ける |
| [E] Interact   [Scroll] Select item   [Q] Drop | [E] 調べる   [スクロール] アイテム選択   [Q] 捨てる |
| (add: [F] Use / eat) | [F] 使う／食べる |
| Items: / Items: (empty) / Items: (none) | 所持品： / 所持品：（空） / 所持品：（なし） |
| Vitals | 状態 |
| Health | 体力 |
| Food | 食料 |
| Warmth | 暖かさ |
| Route to Goal  N% | ゴールまでの航路  N% |
| - Final Approach! | - 最終接近！ |

Notes: gauges use `Printf("%s  %d%%", Name, …)` — pass the JP label as `Name`. Route line: localize
prefix + suffix, keep the numeric `%d%%`. "Route to Goal": chose `ゴールまでの航路`; glossary
canonical is `航路進行度` — pick one and record it in `docs/localization-glossary.md`. Glossary uses
**乗員** for crew (not 乗組員). No forbidden/stale terms appear.

### Menu (`Source/AbyssLock/AbyssMainMenuWidget.cpp`)
Already Japanese (ゲーム開始 / 一人モード / ロビーを開く / ロビーに入る / 開始 / 接続先アドレス /
status lines) — these just need the font. Keep `Frostwake` (brand) and `127.0.0.1` Latin.
Consistency nit: menu `一人モード` vs HUD `練習` name the same mode — unify the wording (flag to
Localization reviewer; don't silently change). Menu LOCTEXT-wrap is a separate GP-09 commit.

## 2. Localization mechanism
- (a) `FText::FromString` JP literals — today's approach; not gathered by the loc pipeline; dead end
  for EN/zh-CN. Keep only for non-localizable tokens (`Frostwake`, `127.0.0.1`, `%d / %d`).
- (b) `LOCTEXT`/`NSLOCTEXT` — idiomatic UE, gathered; already in flight for `AbyssShipTaskActor.cpp`.
- (c) **String Table asset + `FText::FromStringTable` — RECOMMENDED target.** Also gathered; source
  strings live in data (editable without recompile), shared between HUD + menu, multi-locale friendly.
- **Path:** do (b) LOCTEXT now if minimizing churn (one-file change, same call shape), migrate to (c)
  when a 2nd locale is scheduled — (b)→(c) is mechanical. `MakeLine`/`MakeText` already take
  `const FText&`, so only call sites change. For the two runtime `Printf` lines, switch to
  `FText::Format` with named args so JP/zh-CN can reorder.

Sketch (String Table):
```cpp
namespace AbyssUIText { inline FText T(const TCHAR* Key)
  { return FText::FromStringTable(TEXT("/Game/Localization/ST_FrostwakeUI"), Key); } }
// BuildHud(): MakeLine(WidgetTree, AbyssUIText::T(TEXT("Hud_RoleCrew")), 18.0f);
```

## 3. Font plan (OFL, reproducible, provenanced)
- **Font: Noto Sans JP, license SIL Open Font License 1.1 (OFL-1.1).** Sources:
  `https://fonts.google.com/noto/specimen/Noto+Sans+JP` or `https://github.com/notofonts/noto-cjk`.
- **OFL facts:** commercial use, modification, and redistribution/bundling permitted **provided the
  `OFL.txt` ships alongside the font** and the Reserved Font Name rule is respected (don't ship a
  modified font as "Noto"). No in-UI attribution required, but ledger + provenance must record it.
- Alternative (smaller, also OFL): M PLUS 1p / Rounded 1c.

Steps:
1. **Fetch** `Tools/assets/fetch_noto_jp_ofl.ps1` (mirror `fetch_polyhaven_cc0.ps1`): download Regular
   + Medium static OTFs **and `OFL.txt`** into `Content/ThirdParty/Quarantine/fonts/NotoSansJP/`;
   write `noto-jp-ofl-provenance.json` (source, license, fetchedAt, per-file bytes, `licenseFile=OFL.txt`).
2. **Import** new editor commandlet `ImportNotoJpFont` (mirror `ImportPolyhavenCc0Assets`): Slate-init
   guard, `IAssetTools.ImportAssetsAutomated` the OTFs to `/Game/ThirdParty/Quarantine/fonts/NotoSansJP`,
   `SavePackage` each; build a composite `UFont` `F_NotoSansJP` (Regular default + Medium subface).
   Add `ValidateNotoJpFont` (LoadObject the UFont, fail if missing) for evidence. Optionally surface
   `import-noto-jp-font` / `validate-noto-jp-font` verbs in frostwake-tools (Editor commandlet, not
   Server — not blocked).
3. **Apply** via `FSlateFontInfo` in the shared helper, swapping the one line in `MakeLine`/`MakeText`:
```cpp
FSlateFontInfo AbyssUIText::JpFont(float Size) {
  if (const UFont* F = LoadObject<UFont>(nullptr, TEXT("/Game/ThirdParty/Quarantine/fonts/NotoSansJP/F_NotoSansJP")))
    return FSlateFontInfo(F, FMath::RoundToInt(Size));
  return FCoreStyle::GetDefaultFontStyle("Regular", Size); // safe fallback
}
// MakeLine: TextBlock->SetFont(AbyssUIText::JpFont(FontSize));  // fixes every label at once
```
4. **Verify:** editor+game build; quality-gate; validate commandlet; PIE the menu + solo match,
   confirm real glyphs (not boxes), screenshot to `Saved/Screenshots/` for owner.
5. **Ledger row** (`docs/asset-ledger-candidates.csv`, schema-validated; `adoption_state=candidate`,
   `commercial_use_allowed=yes`, `credit_required=no`, `redistribution_allowed=yes`, `rights_risk=low`,
   human license OFL-1.1 in notes + provenance JSON, `output_path` → the quarantine provenance JSON).
   Promote to `approved` only after IP Review confirms `OFL.txt` ships with the font.

## Sequencing
Font swap and LOCTEXT/String-Table wrap are separate commits. The font fix alone makes the
already-JP menu legible; the HUD needs both JP strings AND the font. Keep everything in quarantine
until IP Review.
