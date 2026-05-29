# GP-08 Exterior Arctic Field — Quixel/Fab (Megascans) plan

Owner decision (2026-05-29): make the **exterior field real first**, using **Quixel/Fab (Megascans)**
(free, requires Epic login). This is the owner-facing acquisition guide + the integration plan.
Parallel split: **owner downloads Megascans assets** (login-gated, can't be automated) while the
**loop prepares the ground/integration** (done: `WB_Field_FrozenSea` plane added cycle 96).

## Why this split
- Megascans **surfaces** are *materials you apply to your own geometry* → we need the ground plane
  first (added). Megascans **3D assets** (ice/rock) are *meshes we scatter* on it. So the plane is
  the permanent ground; only its material changes — not throwaway.
- Fab/Megascans needs an **Epic account login in-editor** (OAuth + GUI). The loop can't do that
  headless, so acquisition is an owner step; integration/placement is the loop's step.

## Step 1 — Owner: enable Fab + sign in (UE 5.7)
1. Open the editor (it's already running): `UnrealEditor.exe Frostwake.uproject`.
2. The **Fab** plugin ships enabled in UE 5.5+. Open the Fab window: editor toolbar **Fab** button,
   or **Window → Fab**. (If hidden: Edit → Plugins → search "Fab" → enable → restart.)
3. **Sign in** with your Epic Games account in the Fab panel.

## Step 2 — Owner: download a small arctic starter set (free)
In the Fab panel, filter to **Free** + **Megascans** and grab a minimal set (search terms):
- **Ground surfaces (materials):** `snow` → 1–2 (e.g. fresh/packed snow); `ice` → 1 frozen surface.
- **Hero meshes (scatter):** `iceberg`, `ice rock`, `glacier`, `snowy rock`, `arctic rock` → 3–5 meshes.
- **Small detail (optional):** snow drift / ice chunk small meshes → 2–3.
Use the lowest practical resolution first (1K–2K) to keep import fast. Click **Add to Library** →
**Add to Project** (imports to `/Game/Megascans/...` or `/Game/Fab/...`).
- **License/IP:** Megascans is free for use within Unreal Engine projects (Epic license), commercial
  UE games included. Record each adopted asset in `docs/asset-ledger-candidates.csv` (source=Fab/
  Quixel Megascans, license=Epic/Megascans-for-UE, commercial=yes). No game-ripped assets.

## Step 3 — Owner: tell the loop the import paths
After download, share the `/Game/...` paths (or just the asset names) of: the chosen **snow surface
material**, the **ice surface material**, and the **ice/rock meshes**. (Or say "done" and the loop
will discover them under `/Game/Megascans` / `/Game/Fab`.)

## Step 4 — Loop: dress the field (integration plan)
Once assets are in `/Game`:
1. Extend `SpawnCube`/`FCubeSpec` with an optional `MaterialPath` + `SetMaterial(0, LoadObject<...>)`
   (null-guarded → engine default if absent), and assign the Megascans **snow** material to
   `WB_Field_FrozenSea` + the bow `WB_Ice_DeckPlate`; **ice** material to `WB_IcePressureGate`/frost.
2. Add a **scatter pass** (like `SpawnProp`): place the Megascans ice/rock meshes across the field
   at varied positions/rotations/scales (a hand-authored list first; a simple jittered grid later),
   Whitebox-tagged so regenerate clears them. Keep the count modest for the first pass.
3. Tune the existing `WB_Atmo_Fog` + HDRI so the plane fades into a believable horizon; verify the
   open bow reads as a frozen sea.
4. `create-icebreaker-whitebox` + `validate-icebreaker-whitebox` + `quality-gate`; screenshot for owner.

## Status
- [x] Field ground canvas: `WB_Field_FrozenSea` 200m×160m plane the ship sits on (cycle 96; engine
      material placeholder — Megascans snow material replaces it in Step 4).
- [ ] Owner: enable Fab + download the arctic starter set (Steps 1–2).
- [ ] Loop: apply Megascans snow/ice material + scatter ice rocks (Step 4) after assets land.
- [ ] Later: ship **exterior hull** mesh; walkable gangway from deck to ice; distant ice ridges.

## Notes
- Interior (the walled ship rooms, cycle 95) stays as-is for now; this lane is the exterior field.
  Interior real-asset dressing (modular kit) is a later step once the field reads right.
- Aesthetics owner-judged via screenshots; keep IP/provenance discipline (ledger row per asset).
