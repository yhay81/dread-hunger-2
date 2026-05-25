# Phase 1 Backlog

This backlog is ordered for a 30-day greybox prototype. Convert each entry into GitHub issues once the repository is initialized with the project board.

## Week 1

| ID | Task | Done When |
| --- | --- | --- |
| P1-001 | Open generated UE project and commit engine-generated project metadata intentionally | UE 5.7, Xcode 26.5, project files, `AbyssLockEditor`, and `AbyssLock` builds verified; Launcher UE blocks `AbyssLockServer` target |
| P1-002 | Add replicated character movement baseline | C++ character and basic input mappings added; local 5-8 player listen-server smokes validate spawned player presence and automation movement paths; manual feel tuning pending |
| P1-003 | Add `GameState` match id, phase, timer | C++ scaffold builds; match phase, start, end, timer expiry, and role events are covered by smoke logs; UI timer polish pending |
| P1-004 | Add structured event logger | Per-run JSONL telemetry writes role assignment, match start, and ship task events in local smoke |
| P1-005 | Add whitebox map rooms and spawn markers | `/Game/Maps/L_IcebreakerWhitebox` exists, has 8 starts, required labels, 3 lockable bulkhead doors, and default map config |
| P1-006 | Add interactable base component | C++ scaffold builds; server range validation, traceable interaction bounds, and multiplayer smoke coverage exist through task, door, pickup, and bot interaction profiles |
| P1-007 | Add door interactable | Replicated open/locked state, visible blocking panel state, lock/release telemetry, and local smoke coverage exist |
| P1-008 | Add item pickup/drop skeleton | C++ pickup/drop and owner-only replicated inventory scaffold build; 5-player smoke validates add, server-spawned drop pickup, re-pickup, and consumption |
| P1-009 | Add local 8-client launch procedure | `Tools/ue/run_local_smoke.py` launches one listen host plus up to 7 localhost clients; ready-lobby and combined systems smoke pass at 8 players with 2 saboteurs |

## Week 2

| ID | Task | Done When |
| --- | --- | --- |
| P1-010 | Add lobby flow | Replicated ready state, server ready RPC, and ready-lobby start condition build and pass 5-player, 6-player, and 8-player local smoke |
| P1-011 | Add hidden role assignment | C++ owner-only `SecretTeam` and server role assignment scaffold starts from ready-lobby path; smoke validates 1 saboteur at 5-6 players and 2 saboteurs at 8 players; UI reveal/start polish pending |
| P1-012 | Add repair task | `AAbyssShipTaskActor` repair path builds, is placed in the whitebox map, and logs `ship_task_applied` in a local host+client smoke |
| P1-013 | Add ship system state | C++ replicated `ShipSystems` and route objective progress build and drive task smoke |
| P1-014 | Add crew win condition | 5-player local smoke progresses route to FinalApproach and ends with crew win |
| P1-015 | Add saboteur win condition | 5-player local smoke drives a critical ship system to zero through sabotage and ends with saboteur win; `match-timer` smoke validates timer expiry as a saboteur win |
| P1-016 | Add whitebox PvE enemy | 5-player local smoke spawns a replicated server-authoritative PvE enemy, applies damage, downs a target, and rescues the target to leave the match stable |

## Week 3

| ID | Task | Done When |
| --- | --- | --- |
| P1-017 | Add power outage sabotage | `AAbyssShipTaskActor` sabotage path builds, is placed in the whitebox map, and logs `ship_task_applied` in a local host+client smoke |
| P1-018 | Add bulkhead lock sabotage | 5-player local smoke opens a bulkhead, applies saboteur lock, blocks normal interaction while locked, releases by repair action, and reopens |
| P1-019 | Add pump sabotage | 5-player local smoke raises flooding pressure through sabotage, lowers it through pump repair, and leaves partial pressure after partial repair |
| P1-020 | Add down/rescue loop | Server-authoritative damage/down/rescue scaffold logs `player_downed` and `player_rescued`; `life-action` smoke validates interactable rescue through `AAbyssLifeActionActor` |
| P1-021 | Add containment loop | Server-authoritative contain/release scaffold logs `player_contained` and `player_released`; `life-action` smoke validates interactable contain and release through `AAbyssLifeActionActor` |
| P1-022 | Add simple QA bot | Automation-only server pawn pickup smoke, PlayerState-backed assigned-pawn door interaction smoke, and PlayerState-backed repair/sabotage task smoke pass; NavMesh movement pending |
| P1-023 | Add log summary script | `Tools/log_summary.py` counts run/build/map/profile metadata, duration, match duration, match start/end, final approach, connection lifecycle, damage, deaths, down/rescue, contain/release, life-action interactions/smoke pass, disconnects, errors, events, role assignment, repair/sabotage task counts, last match winner/reason, bulkhead lock smoke counters, flooding pressure/pump smoke counters, PvE enemy/damage smoke counters, item pickup/drop smoke counters, combined systems smoke counters, QA bot smoke counters, QA player-bot smoke counters, and QA task-bot smoke counters; `Tools/ue/run_smoke_suite.py` aggregates profile summaries; `Tools/ue/export_smoke_suite_markdown.py` exports evidence tables; heavy suite and 8-player combined evidence exist |

## Milestone Checkpoint

- Phase 1 local automation milestone is recorded in `docs/phase1-milestone-report.md`.
- Current best evidence is `Saved/SmokeSuites/suite-20260525-050522/suite_summary.md`.
- Automated 5-8 player listen-server gates are strong enough to schedule P1-024, and telemetry now records run metadata, connection lifecycle, duration, match duration, timer expiry, and interactable life actions. Human fun validation, full 8-player human completion, manual movement feel, and Dedicated Server validation remain open.

## Week 4

| ID | Task | Done When |
| --- | --- | --- |
| P1-024 | Run 6-8 player human test 1 | Packet prepared in `docs/playtests/p1-024-human-test-1.md`; `Tools/playtest_run_scaffold.py` creates ignored Windows PowerShell run folders and launch scripts; anonymized summary template prepared in `docs/playtests/p1-024-summary-template.md`; `Tools/playtest_summary.py` can generate the summary skeleton from `log_summary.py` JSON; `Tools/playtest_preflight.py` checks human-summary gates and redaction before commit; non-human dry-run example exists at `docs/playtests/p1-024-dry-run-summary-example.md`; complete when human log, notes, decision list, and preflight pass are saved |
| P1-025 | Fix top 5 blockers | Retest confirms blockers are gone |
| P1-026 | Run 6-8 player human test 2 | Players can complete a match |
| P1-027 | Run 8-player target test | At least one full 8-player match ends |
| P1-028 | Decide keep/cut/change list | Phase 2 scope is updated |
| P1-029 | Update Steam Playtest risk list | Store and infra blockers are known |
| P1-030 | Validate near-future visual POC direction | `Tools/ue/scaffold_frostwake_visual_poc.py` creates a dry-run/local manifest for a separate visual POC; a duplicate visual POC map then uses approved candidate assets or placeholders to show central corridor, engine/fuel/battery bay, and exterior ice access without changing whitebox automation; screenshots and asset-ledger candidate entries exist |
| P1-031 | Resume on Windows workstation | Windows clone passes `quality_gate.py --require-ue`, `unreal_gate.py --platform Win64 --include-server`, dedicated-server boot/client-join probes, and at least `qa-bot`, `match-timer`, `life-action`, `combined5`, and `ready8` smoke profiles; `docs/windows-validation-template.md` is filled into a new cycle or dated result note; server-target result is recorded |
