# Parity Observation Protocol (reference play sessions)

> **Purpose.** A repeatable checklist for observing a *reference* game's UX/mechanics during
> **owned** play, to inform Frostwake's **original** design. Record only abstracted lessons in our
> own terms. Per [ip-risk.md](ip-risk.md) / [ip-boundary.md](ip-boundary.md): do **not** copy
> names, UI/map layouts, item/role names, or verbatim wording. Capture *how it feels and what
> structures exist*, not *their expression*. Any raw screenshots/recordings stay local-only under
> `references/private/` (never committed).

## How to run

1. Launch the owned game normally via Steam (Steam must be running/logged in).
2. *(Optional)* add `-log` in Steam -> game -> Properties -> Launch Options for a live log window.
   The game also writes `.../AppData/Local/DreadHunger/Saved/Logs/` automatically. Note: verbose
   logging is compiled out of the shipped build, so logs stay at Display level.
3. Play a short session - ideally host one and join one - with this checklist open. Jot terse
   notes per row **in your own words** (no proprietary names).
4. Afterward, hand me the notes (and point me at the local log). I synthesize abstracted parity
   entries into the docs; the raw log stays local-only.

## Checklist (note in your own words)

### Onboarding & match flow
- Time from launch -> in a match. Lobby model (host / server browser / matchmaking).
- Pre-match role/loadout flow; how roles are revealed; team sizes.
- Match phases and rough timings (start / mid / climax / end); win & lose conditions structurally.

### Controls & feel
- Core control scheme (move, interact, inventory, map, voice: push-to-talk vs open mic).
- Interaction model: hold-to-use? progress bars? tool/resource requirements?
- Moment-to-moment pace; how heavy/snappy movement feels.

### HUD & UI (structure, not layout copy)
- What information is always on screen (meters, compass, objectives); how many meters.
- Menu/inventory interaction pattern; diegetic vs overlay balance.
- Glance readability: what's easy/hard to parse.

### Survival loop
- Which needs/resources you manage (heat / food / health / ... in generic terms) and pressure cadence.
- Failure states and recovery loops; downtime vs tension balance.

### Ship / systems
- Which ship systems you operate; how maintenance tasks are structured; failure/repair loops.
- How the environment (cold / ice / ...) applies pressure.

### Social deduction
- How suspicion is generated (evidence types, logs, sightlines, voice).
- Saboteur affordances at a structural level; how sabotage is detected/contested.
- Proximity voice behavior: range, occlusion, death/ghost rules.

### Multiplayer feel
- Perceived latency/desync; how actions confirm; rubber-banding.
- Reconnection behavior; what happens on host/player drop.

## Output

Notes become abstracted entries in the parity docs (e.g. [mechanics-parity-target.md](mechanics-parity-target.md),
[control-scheme.md](control-scheme.md)) expressed in our near-future-vessel terms - never their wording.
