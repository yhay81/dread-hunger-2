# Visual POC Screenshot Capture & Review Rubric

Lane: GP-08 (Distinct Presentation And Rights) · Created: 2026-05-29 (loop, cycle 80)

This rubric makes cycle-79's "Next Cycle" executable: it tells a human operator or a
GUI-capable agent exactly how to capture and judge `/Game/Maps/L_FrostwakeVisualPOC` so the
three review zones get a recorded **keep / replace / reject** decision. Capturing the images
needs an interactive Unreal Editor viewport, so a headless loop agent cannot do it — but with
this rubric the work is unblocked the moment an Editor-capable operator is available.

Sources this rubric must respect: `docs/art-bible.md`, `docs/visual-poc-rights-gate.md`,
`docs/visual-poc-import-checklist.md`, `docs/ip-boundary.md`, `docs/reference-policy.md`.

## Pre-capture setup

1. Confirm the map exists and has the required zones:
   `cargo run -p frostwake-tools -- validate-frostwake-visual-poc --platform Win64 --skip-build`
   (expected: pass — central corridor, battery/service bay, exterior ice deck present).
2. Open `/Game/Maps/L_FrostwakeVisualPOC` in the UE 5.7 Editor. Do **not** open or modify
   `/Game/Maps/L_IcebreakerWhitebox` (the automation map must stay untouched — charter boundary).
3. Build lighting if prompted, or note "preview lighting only" on each shot so reviewers do
   not judge final mood from unbuilt lighting.
4. Set viewport to 1920×1080, FOV ~90, "Lit" view mode. Take a second pass in "Unlit" only if
   you need to judge raw material/albedo readability.
5. Save screenshots to `Saved/VisualPOC/screenshots/<yyyymmdd>/` (gitignored). Commit only the
   review **notes** below, never raw PNGs, unless IP Review explicitly approves a redacted set.

## Per-zone capture viewpoints

Capture at least the listed shots per zone; add more if a defect needs to be shown.

| Zone | Shots to frame |
| --- | --- |
| **Central corridor** | (a) down-the-length eye-level establishing shot; (b) a junction / doorway showing how a player reads "where do I go"; (c) one detail of a placeholder material at arm's length |
| **Battery / service bay** | (a) wide showing the bay's function-at-a-glance; (b) the interactable/working area a crew member would stand at; (c) one material detail (e.g. metal walkway / diamond plate) |
| **Exterior ice deck** | (a) wide silhouette of the vessel exterior against the polar horizon; (b) deck-level player-eye shot; (c) the ice surface material up close |

## Review rubric (score each zone 1-5, then decide)

For each zone, rate and note evidence:

1. **Reads as a near-future icebound research vessel** — does the space communicate the
   Frostwake fiction without a caption? (1 = generic/unclear, 5 = unmistakable)
2. **Scale & proportion** — do corridors/bays feel like a real crewed ship a player can navigate?
3. **Material readability** — do the placeholder CC0 materials (Ice003, MetalWalkway014,
   DiamondPlate009) read correctly, or do they look like flat/wrong-scale placeholders?
4. **Lighting & mood legibility** — can a player tell safe vs. hazardous, interior vs. exterior?
5. **Landmark / orientation clarity** — would a confused player find this space memorable enough
   to navigate back to? (feeds GP-09 comprehension)
6. **Placeholder honesty** — is it obviously a greybox/placeholder (good) rather than something a
   viewer could mistake for shipped final art (bad — would mislead per GP-08 store-claim safety)?
7. **IP safety** — does the framing/layout/silhouette avoid resembling protected expression,
   map structure, or UI from comparable games? (per `docs/ip-boundary.md`; any doubt → flag to
   IP Review, do not promote)

### Decision per zone

Record one of:
- **keep** — direction reads; proceed to refine materials/lighting next.
- **replace** — concept is right but execution misleads or fails ≥2 criteria; list the swap.
- **reject** — does not serve the fiction or carries IP risk; remove from the POC direction.

### Overall direction decision

After the three zones: does the three-zone POC read as one coherent near-future polar response
vessel? Record keep / replace / reject for the **overall direction** with a one-paragraph reason
and the single highest-value next visual change.

## Outputs (what to commit)

1. Append a dated "Review N" section to **this file** (or a sibling `visual-poc-review-NN.md`)
   with: build/map id, lighting state, per-zone scores + decisions, overall decision, and the
   next visual change.
2. Only after a zone is **kept** and its materials judged acceptable, a follow-up (headless OK)
   step may set `reviewer_approval` for the corresponding rows in
   `docs/asset-ledger-candidates.csv` and re-verify with
   `cargo run -p frostwake-tools -- asset-ledger-check`. **Do not pre-approve assets before a
   real visual review** — that violates the GP-08 provenance boundary.

## Boundaries (from the GP-08 charter)

- Never present concept-only or placeholder shots as gameplay/marketing screenshots.
- Never import or promote assets without recorded provenance + approval state.
- Never alter `L_IcebreakerWhitebox` or default automation maps for art experiments.
- No protected names, lore, UI composition, or map layouts from comparable games.
