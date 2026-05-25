# Asset Acquisition Plan

Current snapshot: 2026-05-25 JST. Re-check Fab listings, prices, compatibility, and license terms immediately before purchase or import.

## Visual Decision

Frostwake should move from a strict 1930s look to an asset-led near-future icebound research vessel.

The gameplay target remains closed-environment cooperative betrayal survival: crew members must keep a vessel moving through a hostile ice route while one or two agents quietly break systems, isolate players, and manufacture doubt. The public expression should be a fictional near-future polar response ship with modular metal interiors, sealed work zones, damaged power systems, frost, warning LEDs, snow/ice exterior work areas, and utilitarian cold-weather crew gear.

This pivot is useful because it:

- gives access to stronger modular sci-fi, industrial, and ship interior asset packs;
- makes the product more visually distinct from 19th-century expedition material;
- supports readable systems gameplay through panels, power rooms, server/radio racks, decon bays, battery rooms, sensors, drones, and maintenance props;
- lets Phase 2 reach a convincing vertical slice faster without locking us into final custom art.

## Asset Strategy

| Tier | Purpose | Buy / Import Rule |
| --- | --- | --- |
| 0 | Free or already-owned prototype packs | Use only for a duplicate visual POC map after ledger entry |
| 1 | First paid visual slice | Buy one modular interior kit, one arctic exterior kit, and one industrial prop kit after license/IP/technical review |
| 2 | Defer until playtest proof | Characters, hero vessel exterior, monster/AI enemy, final UI, voice, music, and logo assets |

The first POC should art-pass only three zones:

1. Central corridor / social choke point.
2. Engine, fuel, or battery bay.
3. Exterior ice deck or ice access.

Do not overwrite `/Game/Maps/L_IcebreakerWhitebox`. Create a duplicate POC map such as `/Game/Maps/L_FrostwakeVisualPOC`, preserve the whitebox for automation, and keep third-party assets quarantined until approved.

## Candidate Shortlist

| Priority | Candidate | Role | Link | Fit | Main Risk | Gate Before Use |
| --- | --- | --- | --- | --- | --- | --- |
| First import | Modular Ship Interior Environment | Ship-specific engine/cargo/industrial interiors; good free baseline | https://www.fab.com/listings/3209f936-17d5-4e7a-86fa-31f5adb0401f?lang=en | Ship, mechanical, industrial, UE format, free | May look present-day cargo rather than near-future unless dressed with sensors/LEDs | Ledger candidate, import quarantine, scale/collision check |
| First paid evaluation | Sci-fi Rooms and Corridors Interior Kit | Modular rooms, corridors, doors, decals for near-future research vessel interiors | https://www.fab.com/listings/f3e9c40d-a56f-4308-8a78-a1d4f0076f96 | Strong near-future readability and modular layout support | Can drift into orbital spaceship if used unchanged | Re-skin with frost, hazard markings, ship systems, muted palette |
| Backup interior | Spaceship Interior Environment Set | Mature modular sci-fi interior set with walls, floors, ceilings, pipes, rooms, cargo props | https://www.fab.com/listings/e55bb035-8720-487f-8a36-4ce50d16f344?lang=en | Large review count, UE format, useful for cargo and corridors | Too space-station if not grounded | Use only for hard-surface parts, remove space branding |
| Exterior / ice access | Plan V: Arctic Settlement Kit | Snow, containers, industrial arctic dressing, exterior staging | https://www.fab.com/listings/df9c51e3-61e5-4893-8dd7-b7e7bd1a6051?lang=fr | Arctic industrial pieces and snow dressing | Settlement visuals can pull away from ship focus | Use only for exterior work deck / ice field POC |
| Character defer | Metahuman Winter Clothing Pack | Modern winter clothes for temporary crew silhouettes | https://www.fab.com/listings/f57c877b-7939-41fd-a789-2e18eaa53fd8 | Modular winter clothing, UE format | Rating is mixed; character assets carry higher IP/performance risk | Defer until visual POC proves map direction |
| Character alternative | Military Winter Jacket 101 | Near-future/tactical winter jacket compatible with MetaHuman and UE5 skeletons | https://www.fab.com/listings/700406e8-44fd-4fff-813c-397bda1df282 | Strong modern cold-weather read, rigged | Military style may imply a different fantasy; 4K textures may affect FPS | Defer; require material downscale/performance pass |

## Purchase Gates

Before any purchase:

- Confirm the listing is current and supports the installed Unreal version or imports cleanly into UE 5.7.
- Record Fab listing URL, seller/publisher, license type, purchase date, price tier, and proof-of-purchase location.
- Confirm commercial use, modification, redistribution limits, credit requirements, AI usage tag, and whether a legacy UE Marketplace license applies.
- Reject assets that rely on wooden expedition ships, occult horror, cannibalism, direct lookalike silhouettes, copied UI, or protected branding.
- Prefer modular pieces with collisions, LODs or Nanite suitability, material instances, reasonable texture budgets, and clean demo maps.
- Treat characters, logos, UI, music, voice, distinctive hero props, and branded decals as high-risk categories.

Before production use:

- Add a ledger entry with `adoption_state=candidate`.
- Import into `Content/ThirdParty/Quarantine/<Publisher>/<AssetName>/`.
- Create project-owned wrappers, material instances, and placed test actors under `Content/Frostwake/`.
- Inspect in a duplicate POC map, capture screenshots, and record keep/replace notes.
- Promote to `approved` only after named review of license, IP fit, scale, collisions, performance, and visual fit.

## Source Notes

- Fab Standard License summary says assets may be used commercially or privately, modified for projects, and distributed as part of a project, while standalone resale or redistribution is not allowed: https://www.fab.com/eula
- Fab documentation describes Standard license Personal and Professional tiers and notes that some migrated products can temporarily use the legacy UE Marketplace license: https://dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab?lang=en-US
- Fab listing pages must still be checked individually for current price, license tier, UE version support, AI flags, seller, and package details before purchase.

## Next Implementation Step

Use the dry-run scaffold before any import:

```bash
python3 Tools/ue/scaffold_frostwake_visual_poc.py --dry-run
python3 Tools/ue/scaffold_frostwake_visual_poc.py --write
```

The scaffold writes a local ignored manifest under `Saved/VisualPOC/` and intentionally does not create or modify `.umap` assets. Cycle 36 should use that manifest to create `L_FrostwakeVisualPOC` only after a specific asset package is selected and its candidate ledger entry is reviewed. Do not import purchased assets until the user confirms the first package to acquire.
