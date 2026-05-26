# Visual POC Import Checklist

Current scope: first quarantined evaluation of `Modular Ship Interior Environment` only. This checklist does not approve a purchase, does not approve production use, and does not permit public screenshots by itself.

Use this after `docs/visual-poc-rights-gate.md` passes and immediately before acquiring or importing the package.

## Required Inputs

| Field | Required Value |
| --- | --- |
| Candidate asset id | `fab-modular-ship-interior` |
| Candidate name | `Modular Ship Interior Environment` |
| Listing URL | `https://www.fab.com/listings/3209f936-17d5-4e7a-86fa-31f5adb0401f` |
| Publisher/seller | `andi475` |
| Intended Unreal target | UE 5.7 project import |
| Quarantine path | `Content/ThirdParty/Quarantine/andi475/ModularShipInteriorEnvironment/` |
| Project wrapper path | `Content/Frostwake/VisualPOC/ShipInterior/` |
| POC map | `/Game/Maps/L_FrostwakeVisualPOC` |
| Automation map | `/Game/Maps/L_IcebreakerWhitebox` remains untouched |
| Proof path | Local ignored path under `Saved/AssetProof/fab-modular-ship-interior/` |

## Pre-Acquisition Checks

Complete every row before downloading or adding the asset to the project.

| Check | Pass Evidence |
| --- | --- |
| Listing still matches the candidate | Current page still identifies the same asset, publisher, Unreal format, and free price |
| License route is recorded | Current Fab license route, tier, and any legacy license note are captured in the proof path |
| AI flags are recorded | Current generated-with-AI and allows-AI-use flags are captured in the proof path |
| Commercial and project-use assumptions are recorded | Notes distinguish Fab Standard summary from final approval |
| User approval exists | The user explicitly approved acquiring this asset in the current context |
| Ledger row is current | `docs/asset-ledger-candidates.csv` has a fresh `license_snapshot_date` and `adoption_state=candidate` |
| Store risk is contained | No marketplace renders, demo-map screenshots, or listing copy will be reused as Frostwake marketing |

## Import Quarantine

Import only into the quarantine path. Do not move third-party source assets directly under `Content/Frostwake/`.

| Check | Pass Evidence |
| --- | --- |
| Quarantine folder is isolated | Imported files are under `Content/ThirdParty/Quarantine/andi475/ModularShipInteriorEnvironment/` |
| Default maps are unchanged | `GameDefaultMap` and `EditorStartupMap` still point at the validated whitebox path |
| Whitebox is untouched | No changes to `/Game/Maps/L_IcebreakerWhitebox` or its smoke-test actors |
| Demo maps are review-only | Any vendor demo map is not used as Frostwake composition or public capture |
| Project wrappers are separate | Frostwake material instances, Blueprint wrappers, and placed POC actors live under `Content/Frostwake/VisualPOC/ShipInterior/` |

## Technical Inspection

Record keep/replace notes before placing the asset into the POC map.

| Area | Inspect For |
| --- | --- |
| Scale | Doors, corridors, stairs, panels, and props match first-person player scale |
| Collision | Walkable pieces, doors, rails, and props have usable collision or a clear collision fix plan |
| Materials | Materials can be instanced or overridden for frost, warning paint, utility labels, and muted ship colors |
| Texture budget | Texture sizes are acceptable for a prototype POC and not mistaken for production optimization |
| Lighting | Pieces work under Frostwake emergency lighting and whiteout/ice contrast |
| Modularity | Corridor, machinery, and cargo pieces can form the three-zone POC without using vendor composition as-is |
| Performance risk | Nanite/LOD/collision setup is noted, even if full optimization is deferred |

## Identity Review

Reject or heavily reskin any part that makes the POC read as:

- 19th-century expedition or wooden vessel.
- Generic present-day cargo ship with no near-future research identity.
- Orbital station or spaceship interior.
- Military bunker as the main fantasy.
- Fantasy horror, occult, cannibalism, or protected lookalike expression.

The POC passes identity review only if a fresh viewer can read the scene as a near-future polar research/response vessel within a few seconds.

## POC Placement Gate

Use only three zones:

1. Central corridor / social choke point.
2. Engine, fuel, pump, or battery bay.
3. Exterior ice deck or airlock edge.

Do not place enough finished art to imply a production art pass. The goal is direction proof, not final polish.

## Promotion Rule

The asset remains `candidate` until a named reviewer records:

- current license snapshot;
- proof-of-acquisition path;
- commercial-use decision;
- IP fit decision;
- technical inspection notes;
- screenshot evidence from `L_FrostwakeVisualPOC`;
- keep, replace, or reject recommendation.

Only then may a later cycle consider `adoption_state=approved`.
