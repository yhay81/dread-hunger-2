# Asset Scorecard

Current snapshot: 2026-05-26 JST. Re-check listing price, license, compatibility, AI flags, and package contents immediately before purchase or import.

## Scoring Criteria

| Criterion | Weight | Question |
| --- | ---: | --- |
| POC fit | 25 | Does it help the near-future icebound research-vessel POC immediately? |
| Modularity | 20 | Can it build corridors, rooms, ship systems, and reusable layouts? |
| IP distance | 20 | Does it avoid period expedition, occult, cannibalism, wooden ship, and lookalike risks? |
| Technical risk | 15 | Are UE format, collisions, materials, LOD/Nanite, and texture budgets likely manageable? |
| Cost timing | 10 | Is it free/low-risk enough before human fun validation? |
| Replacement cost | 10 | If replaced later, did it still provide useful layout/prototype value? |

Maximum score: 100.

## GP-08 Decision

The current first POC direction is an environment-only, quarantined ship-interior pass. `Modular Ship Interior Environment` stays first because it is the lowest-cost way to test Frostwake's vessel identity without buying into character, military, or orbital-space expression too early.

Paid interior, exterior, and character assets remain conditional. Use `Sci-fi Rooms and Corridors Interior Kit` only if the free ship-interior POC fails to read near-future after project-owned dressing. Use `Plan V: Arctic Settlement Kit` only for exterior deck/ice-access dressing after the vessel reads clearly. Defer character clothing until the map POC and human playability evidence justify that spend.

Before acquiring or importing the first candidate, complete `docs/visual-poc-import-checklist.md`.

## Candidate Ranking

| Rank | Candidate | Score | Decision | Reason |
| ---: | --- | ---: | --- | --- |
| 1 | Modular Ship Interior Environment | 87 | Use first if current listing still matches | Free UE ship/interior pack, directly useful for engine/cargo/industrial rooms, low purchase risk |
| 2 | Sci-fi Rooms and Corridors Interior Kit | 78 | Paid fallback after free ship POC | Strong modular near-future corridors and rooms; must be grounded away from orbital spaceship visuals |
| 3 | Plan V: Arctic Settlement Kit | 72 | Exterior/deck candidate after ship identity works | Useful snow/industrial exterior dressing; keep ship as the main identity |
| 4 | Spaceship Interior Environment Set | 68 | Backup interior candidate | Large hard-surface kit; higher risk of looking like a generic space station |
| 5 | Military Winter Jacket 101 | 54 | Defer | Useful silhouette but character/performance/military-fantasy risk is high before map POC |
| 6 | Metahuman Winter Clothing Pack | 48 | Defer | Character clothing is not the bottleneck until the map POC proves the direction |

## Recommended Purchase Order

1. Add/import the free `Modular Ship Interior Environment` as the first quarantined candidate if the current listing still says it is free and Unreal Engine format.
2. Build a three-zone visual POC using only a quarantined import and project-owned wrapper materials.
3. If the POC reads too present-day cargo ship, buy one near-future modular interior pack, with `Sci-fi Rooms and Corridors Interior Kit` as the first paid candidate.
4. Add an arctic exterior kit only after the central corridor and engine/fuel/battery bay read correctly.
5. Do not buy character packs until the Windows smoke gates and first human test are complete.

## Hard Gates Before Import

- `docs/asset-ledger-candidates.csv` has a candidate row.
- Listing URL, publisher, license snapshot date, AI flags, price, UE version support, and package details are freshly checked.
- Asset imports to `Content/ThirdParty/Quarantine/...`, not production folders.
- `L_IcebreakerWhitebox` remains untouched.
- `L_FrostwakeVisualPOC` remains separate from default maps and smoke automation.

## Source Checks

- Fab Standard License summary currently allows commercial/private use, modification into projects, and distributing projects that incorporate Fab assets, while forbidding standalone resale/redistribution: https://www.fab.com/eula
- Fab license docs currently list Standard and CC-BY license types, Personal/Professional Standard tiers, and note legacy UE Marketplace licenses can appear on migrated products: https://dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab?lang=en-US
- Modular Ship Interior Environment listing currently describes ship engine/cargo/interior structural elements, Unreal Engine format, 5.0 rating from 3 ratings, and free pricing: https://www.fab.com/listings/3209f936-17d5-4e7a-86fa-31f5adb0401f?lang=en
- Sci-fi Rooms and Corridors Interior Kit listing currently describes orbital spaceship-themed rooms/corridors/props/decals, Unreal Engine format, and 4.9 rating from 11 ratings: https://www.fab.com/listings/f3e9c40d-a56f-4308-8a78-a1d4f0076f96
- The 2026-05-26 GP-08 listing review is recorded in `docs/visual-poc-rights-gate.md`.
