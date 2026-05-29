# Phase 1 Milestone Report

Date: 2026-05-25

## Verdict

Phase 1 has reached the local automation milestone for a greybox listen-server prototype.

This does not mean the game is commercially ready or socially proven. It means the current Unreal prototype can repeatedly start local 5-8 player matches, assign hidden teams, exercise the main interaction loops, and produce evidence logs without relying on the Dedicated Server target that is still blocked by the Launcher UE distribution.

## Current Pass Definition

| Gate | Status | Evidence |
| --- | --- | --- |
| UE project opens and builds | pass | `cargo run -p frostwake-tools -- unreal-gate --skip-generate --include-server` reports Editor/Game pass |
| Dedicated Server target | blocked | Launcher UE reports server targets are unsupported from this engine distribution |
| 5-player ready match | pass | Cycles 9, 17, 21, 26 |
| 6-player ready match | pass | Cycle 19 |
| 8-player ready match | pass | Cycles 19, 26 |
| 8-player combined systems smoke | pass | Cycles 21, 26 |
| Crew win condition | pass at 5 players | Cycle 11 |
| Saboteur win condition | pass at 5 players | Cycle 12 |
| Timer saboteur win | pass in smoke | `match-timer` profile |
| Interaction-based rescue/containment | pass in smoke | `life-action` profile |
| Human 6-8 player test | not run | P1-024 remains open |
| Social fun validation | not run | Requires human test notes and recordings |

## Validated Systems

- Project setup: UE 5.7 project files, Xcode 26.5 toolchain, Editor/Game builds, and local quality gates.
- Whitebox map: `/Game/Maps/L_IcebreakerWhitebox` with 8 player starts, required ship areas, task actors, and three lockable bulkheads.
- Lobby and match flow: replicated ready state, ready RPC, 5-8 player start condition, match phase, and role assignment.
- Team setup: 1 saboteur at 5-6 players and 2 saboteurs at 8 players, with hidden owner-only team replication.
- Ship objectives: route progress, repair tasks, sabotage tasks, crew final approach win, and fatal ship-state saboteur win.
- Interaction loops: doors, bulkhead lock/release, item pickup/drop/re-pickup/consume, down/rescue, containment/release, pump repair, and flooding pressure.
- PvE pressure: minimal replicated server-authoritative enemy damage source that can down a target and recover the match through rescue.
- QA automation: named smoke profiles, PlayerState-backed bot interaction paths, smoke suite runner, JSONL logs, and Markdown evidence export.

## Strongest Evidence

The current best milestone artifact is the heavy smoke suite:

```text
cargo run -p frostwake-tools -- run-smoke-suite --include-heavy --skip-build --null-rhi
```

Evidence files:

- `Saved/SmokeSuites/suite-20260525-050522/suite_summary.json`
- `Saved/SmokeSuites/suite-20260525-050522/suite_summary.md`
- `Saved/SmokeSuites/suite-20260525-050522/*/events.jsonl`
- `docs/cycles/2026-05-25-cycle-26.md`

The suite passed all six key profiles:

| Profile | Gate |
| --- | --- |
| `qa-bot` | automation-only pawn pickup |
| `qa-player-bot` | assigned player pawn door interaction |
| `qa-task-bot` | assigned crew/saboteur task interaction |
| `combined5` | 5-player non-terminal combined systems |
| `ready8` | 8-player ready lobby and 2-saboteur assignment |
| `combined8` | 8-player non-terminal combined systems |

## Remaining Phase 1 Gaps

- Human playtest has not started. P1-024 through P1-028 remain open.
- 8-player full match completion is not yet proven by a human or bot-driven terminal match. The current 8-player gate proves start, roles, and non-terminal system stability.
- Dedicated server runtime validation is blocked until a source-built or otherwise server-capable UE distribution produces `FrostwakeServer.exe`.
- NavMesh or route-following bot movement is pending; current bot steps are deterministic automation paths.
- Near-proximity voice, Steam lobby, Steam Server Browser, Steam Datagram Relay, moderation, and production server hosting are Phase 2 or later.

## Go / No-Go

| Area | Decision | Reason |
| --- | --- | --- |
| Continue Phase 1 | go | The prototype has enough automated proof to justify human 6-8 player tests |
| Start art production | no-go | Human fun, clarity, and repeat-match desire are still unproven |
| Start Dedicated Server implementation | gated | Requires source-built UE or a separate validation machine with server-target support |
| Start Phase 2 systems | partial go | Only low-risk prep work should start until human tests expose the top blockers |

## Next Three Cycles

1. Point `UE_ROOT` at a source-built or otherwise server-capable UE distribution and rerun the Win64 server target gate.
2. Run the dedicated boot, client-join, ready-lobby, and Phase 2 entry wrappers once `FrostwakeServer.exe` exists.
3. Run the first 6-8 player human test, or use a smaller rehearsal only if scheduling or Windows smoke readiness blocks the full run.

## Quality Bar

The next milestone is not another automated pass by itself. The next milestone is evidence that 6-8 people can understand the round, finish or meaningfully fail it, and produce at least one concrete keep/cut/change list from observed play.
