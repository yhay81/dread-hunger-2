# Free Asset Intake

Current snapshot: 2026-05-28 JST. Re-check source pages before public screenshots, Steam store use, or promotion from `candidate` to `approved`.

## Current Direction

Use free assets to make the visual POC readable before buying anything. The first set is CC0 ambientCG materials that can dress the existing geometry as a near-future polar response vessel:

| Asset | Role | Source |
| --- | --- | --- |
| Diamond Plate 009 | ship deck and corridor floor | https://ambientcg.com/a/DiamondPlate009 |
| Painted Metal 004 | warning paint, panels, hazard bands | https://ambientcg.com/a/PaintedMetal004 |
| Metal Walkway 014 | worn racks, service bay metal, industrial props | https://ambientcg.com/a/MetalWalkway014 |
| Snow 015 | exterior deck snow and dirty ice buildup | https://ambientcg.com/a/Snow015 |
| Ice 003 | frosted windows and ice patches | https://ambientcg.com/a/Ice003 |

## Rights Position

ambientCG documents its assets under Creative Commons CC0 1.0 Universal. The candidate rows in `docs/asset-ledger-candidates.csv` record commercial use as allowed, credit as not required, and redistribution as allowed for these source assets, while keeping them at `adoption_state=candidate` until the in-engine import and visual review pass.

License source: https://docs.ambientcg.com/license/

API source: https://docs.ambientcg.com/api/v2/full_json/

## Intake Command

Dry-run and proof snapshot:

```powershell
.\Tools\assets\fetch_ambientcg_visual_poc.ps1
```

Download and extract the 1K JPG source packages into quarantine:

```powershell
.\Tools\assets\fetch_ambientcg_visual_poc.ps1 -Download
```

The script writes proof metadata under `Saved/AssetProof/ambientcg-free-materials/` and source files under `Content/ThirdParty/Quarantine/ambientCG/`.

## Visual POC Map

Create the first dressed placeholder map:

```powershell
cargo run -p frostwake-tools -- create-frostwake-visual-poc --platform Win64
cargo run -p frostwake-tools -- validate-frostwake-visual-poc --platform Win64 --skip-build
```

The commandlet creates `/Game/Maps/L_FrostwakeVisualPOC` with placeholder actors named for the CC0 material targets. It does not change `/Game/Maps/L_IcebreakerWhitebox` or default maps.

## Promotion Gate

Do not promote any of these materials to `approved` until:

- source files are in quarantine and proof metadata exists;
- the texture maps import cleanly as Unreal assets;
- material instances are connected to the visual POC placeholders;
- screenshots from `L_FrostwakeVisualPOC` show the three target zones clearly;
- a named reviewer records keep, replace, or reject decisions.
