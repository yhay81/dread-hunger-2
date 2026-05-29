#pragma once

#include "CoreMinimal.h"

/**
 * Shared source-of-truth for front-end / HUD UI strings (codename Frostwake).
 *
 * All localizable UI strings live in one string table ("ST_FrostwakeUI", namespace
 * "FrostwakeUI") whose source text is ENGLISH (the native/source culture). String tables are
 * gathered by the UE localization pipeline exactly like LOCTEXT, so the Japanese and Simplified
 * Chinese translations are produced against these stable keys without touching code. Sharing one
 * key set across HUD + menu also lets every label switch language at once on a culture change.
 *
 * GP-09 interim mechanism: the table is defined in C++ via the LOCTABLE_* macros so it builds with
 * no editor (no .uasset). Moving the source strings into a String Table asset or CSV later is
 * mechanical and leaves these call sites (the FrostwakeUIText::Text keys) unchanged. CJK fonts (for
 * legible JA / zh-Hans glyphs instead of tofu) are a separate, gated GP-09 step.
 */
struct FSlateFontInfo;

namespace FrostwakeUIText
{
/** Resolve a localizable UI string by key from the shared Frostwake UI string table. */
FText Text(const TCHAR* Key);

/**
 * UI font (point size) with CJK glyph coverage: Noto Sans JP as the default typeface (covers
 * Latin + Japanese) plus a Noto Sans SC sub-font for the zh-Hans culture (Simplified Chinese
 * glyph forms). Built once at runtime from the quarantined OFL font faces. Falls back to the
 * engine default font if the faces have not been imported yet (ImportNotoCjkFontAssets commandlet),
 * so callers are always safe.
 */
FSlateFontInfo UiFont(float Size);
}
