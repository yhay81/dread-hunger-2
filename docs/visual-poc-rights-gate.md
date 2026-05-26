# Visual POC Rights Gate

Current snapshot: 2026-05-26 JST. Re-check every listing, price, license tier, engine compatibility, AI flag, and package detail immediately before purchase, import, screenshot capture, trailer capture, or Steam store use.

## Decision

Use an environment-only visual POC before any paid acquisition. The safest first direction is a quarantined ship-interior pass led by `Modular Ship Interior Environment`, then dressed with project-owned frost, warning labels, sensor panels, battery hardware, radio hardware, and maintenance props so it reads as Frostwake's near-future polar research vessel rather than a generic cargo ship.

Do not buy character packs, logo assets, final UI, music, voice, or hero marketing art for this POC. Do not use marketplace listing renders as store or trailer material.

## Current Listing Read

| Candidate | Current visible fit | Current visible rights/technical notes | GP-08 decision |
| --- | --- | --- | --- |
| Modular Ship Interior Environment | Ship engine, cargo, and industrial interior construction pieces | Fab page still shows Unreal Engine format, free pricing, 5.0 rating from 3 ratings, allows AI use, and not generated with AI | First quarantined candidate if the user approves acquisition; low purchase risk, but must be dressed away from present-day cargo |
| Sci-fi Rooms and Corridors Interior Kit | Modular rooms, corridors, props, and decals with strong near-future readability | Fab page still shows Unreal Engine format, 4.9 rating from 11 ratings, allows AI use, and not generated with AI; page also describes an orbital spaceship theme | Paid fallback only if the free ship POC reads too plain; strip orbital/spacestation cues and use muted ship-system dressing |
| Spaceship Interior Environment Set | Modular hard-surface rooms, corridors, pipes, props, and demo level | Fab page still shows Unreal Engine format, 4.7 rating from 80 ratings, allows AI use, and not generated with AI | Backup interior library, not the identity lead; use only hard-surface pieces after grounding review |
| Plan V: Arctic Settlement Kit | Snow, containers, rugged arctic dressing, and industrial exterior pieces | Fab page still shows Unreal Engine format, no current rating, custom collision/LODs, mostly 4096 textures, does not allow AI use, and not generated with AI | Exterior deck/ice-access candidate after the ship identity works; reject village, cabin, wooden, and settlement-leading compositions |
| Metahuman Winter Clothing Pack | Modular winter clothing for MetaHuman bodies | Fab page still shows Unreal Engine format, 3.7 rating from 3 ratings, no AI use, not generated with AI, and notes MetaHuman characters are not included | Defer until map POC and human proof justify character spend |
| Military Winter Jacket 101 | Rigged tactical winter jacket for MetaHuman and UE5 skeletons | Fab page still shows Unreal Engine/MetaHuman format, 5.0 rating from 7 ratings, 4K textures, no AI use, and not generated with AI | Defer; tactical/military tone and texture budget are wrong to solve before the map direction |

## License Interpretation

Fab's Standard License summary currently allows private/commercial use, project incorporation, modification, distribution of projects with incorporated assets, compatible-tool use, and project collaborator sharing. It does not allow standalone resale or redistribution of the asset. Fab's license documentation currently lists Standard and CC-BY license types, Personal and Professional Standard tiers, and a legacy UE Marketplace license case for some migrated products.

That is enough to keep candidates in the ledger, but not enough to approve production use. Each imported package still needs a purchase-date snapshot, proof-of-purchase path, final license state, reviewer approval, and in-engine inspection before promotion from `candidate` to `approved`.

For the first free ship-interior candidate, use `docs/visual-poc-import-checklist.md` after this rights gate passes and before any acquisition/import step.

## Acceptance Gate

Pass all of these before importing or screenshotting any third-party visual asset:

- The candidate row exists in `docs/asset-ledger-candidates.csv` with the current listing snapshot date.
- The visible listing still matches the intended use, price, publisher, engine format, AI flags, and license route.
- The user has approved any paid purchase.
- The first-candidate import checklist is completed before files enter the Unreal project.
- Imported files land only under `Content/ThirdParty/Quarantine/<Publisher>/<AssetName>/`.
- Project-owned wrappers, material instances, and placed actors live under `Content/Frostwake/` only after review.
- `L_IcebreakerWhitebox` and default maps remain untouched.
- The POC target remains `/Game/Maps/L_FrostwakeVisualPOC`.
- Screenshots are captured from the local build only, not marketplace renders or concept material.
- Store copy does not claim production art, multiple maps, final characters, voice, moderation, dedicated servers, or Steam networking unless the submitted build proves those features.

## POC Scene Brief

Build only three readable zones:

1. Central corridor / social choke point with sealed hatches, frost traces, warning LEDs, and route/radio signage.
2. Engine, fuel, pump, or battery bay with exposed maintenance panels and clear sabotage/repair affordances.
3. Exterior ice deck or airlock edge with rails, containers, winch hardware, and whiteout framing.

Reject a POC pass if it looks like a 19th-century expedition, a wooden ship, an orbital station, a generic military bunker, a fantasy horror set, or an arctic settlement where the vessel is no longer the main identity.

## Sources Checked

- https://www.fab.com/eula
- https://dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab?lang=en-US
- https://www.fab.com/listings/3209f936-17d5-4e7a-86fa-31f5adb0401f?lang=en
- https://www.fab.com/listings/f3e9c40d-a56f-4308-8a78-a1d4f0076f96
- https://www.fab.com/listings/e55bb035-8720-487f-8a36-4ce50d16f344?lang=en
- https://www.fab.com/listings/df9c51e3-61e5-4893-8dd7-b7e7bd1a6051?lang=fr
- https://www.fab.com/listings/f57c877b-7939-41fd-a789-2e18eaa53fd8
- https://www.fab.com/listings/700406e8-44fd-4fff-813c-397bda1df282
