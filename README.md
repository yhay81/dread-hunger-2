# Frostwake

Working title for a 5-8 player cooperative betrayal survival game set aboard a fictional near-future icebound research vessel.

This repository is the research and prototype workspace. The immediate goal is not a trailer or art pass. The goal is to lock the playable shape, then build a rough 5-8 player greybox prototype that proves whether people naturally want another match.

## Current Direction

- Genre: PvPvE cooperative survival social deduction
- Players: 5-8, best at 8
- Teams: crew vs. 1-2 saboteurs/agents
- Match length target: 18-25 minutes for production, up to 35 minutes in early whitebox tests
- Setting: fictional near-future icebound research vessel crossing a hostile polar route toward a rescue signal
- Core loop: maintain hull, fuel, power, radio, route, heat, and crew survival while identifying sabotage
- Voice: proximity voice is a core feature, implemented with an existing SDK rather than custom voice transport
- Match server: Unreal Dedicated Server / C++ replication
- Discovery: Steam Lobby, Steam Server Browser, Steam Game Servers API
- Network protection: Steam Networking Sockets / Steam Datagram Relay where applicable
- Backend: TypeScript for reports, stats, admin, and fleet metadata
- Tools: Python for QA, logs, asset/data generation, and automation
- Distribution path: Steam Playtest, then Demo or Early Access only after stability and moderation gates are met

## Phase 0 Outputs

- [Vision](docs/vision.md)
- [Game Design](docs/game-design.md)
- [Roles](docs/roles.md)
- [Items](docs/items.md)
- [Sabotage](docs/sabotage.md)
- [Map Flow](docs/map-flow.md)
- [Network Rules](docs/network-rules.md)
- [Server Hosting Model](docs/server-hosting-model.md)
- [Technical Architecture](docs/technical-architecture.md)
- [Best Practice Alignment](docs/best-practice-alignment.md)
- [Backend Contract](docs/backend-contract.md)
- [Steam-Native Implementation Backlog](docs/steam-native-implementation-backlog.md)
- [Commercial Quality Target](docs/commercial-quality-target.md)
- [Agent Operating Model](docs/agent-operating-model.md)
- [Iteration Loop](docs/iteration-loop.md)
- [Production Plan](docs/production-plan.md)
- [QA Plan](docs/qa-plan.md)
- [Windows Handoff](docs/windows-handoff.md)
- [Windows Validation Template](docs/windows-validation-template.md)
- [Windows Phase 2 Entry Template](docs/windows-phase2-entry-template.md)
- [Windows Dedicated Server Runbook](docs/windows-dedicated-server-runbook.md)
- [Playtest Data And Moderation](docs/playtest-data-and-moderation.md)
- [Art Bible](docs/art-bible.md)
- [Asset Acquisition Plan](docs/asset-acquisition-plan.md)
- [IP Boundary](docs/ip-boundary.md)
- [Reference Policy](docs/reference-policy.md)
- [Research Workflow](docs/research-workflow.md)
- [Steam Store Plan](docs/steam-store.md)
- [Steam Playtest Checklist](docs/steam-playtest-checklist.md)
- [Store Copy Drafts](docs/store-copy-drafts.md)
- [Localization Glossary](docs/localization-glossary.md)
- [AI Content Disclosure](docs/ai-content-disclosure.md)
- [Asset Ledger](docs/asset-ledger.md)
- [Asset Ledger Candidates](docs/asset-ledger-candidates.csv)
- [Asset Scorecard](docs/asset-scorecard.md)
- [IP Risk Register](docs/ip-risk.md)
- [Competitive Analysis](docs/competitive-analysis.md)
- [Phase 1 Backlog](docs/phase1-backlog.md)
- [Phase 2 Backlog](docs/phase2-backlog.md)
- [Steam Phase 2 Integration Plan](docs/steam-phase2-integration-plan.md)
- [Steam Dedicated Server Client Join Plan](docs/steam-dedicated-server-client-join.md)
- [Voice Chat Interface Spike](docs/voice-chat-interface-spike.md)
- [Issue Import](docs/issue-import/README.md)

## Phase 1 Current Milestone

- [Phase 1 Milestone Report](docs/phase1-milestone-report.md)
- [P1-024 Human Playtest Packet 1](docs/playtests/p1-024-human-test-1.md)
- [P1-024 Observer Sheet](docs/playtests/p1-024-observer-sheet.md)
- [P1-024 Post-Round Survey](docs/playtests/p1-024-post-round-survey.md)
- [P1-024 Anonymized Summary Template](docs/playtests/p1-024-summary-template.md)
- [P1-024 Dry-Run Summary Example](docs/playtests/p1-024-dry-run-summary-example.md)
- Current verdict: local listen-server automation milestone reached; human 6-8 player tests remain the next proof point.
- Strongest evidence: `Saved/SmokeSuites/suite-20260525-050522/suite_summary.md` from `python3 Tools/ue/run_smoke_suite.py --include-heavy --skip-build --null-rhi`.
- Server target status: Mac Launcher UE blocked `AbyssLockServer`; Windows clone must retest `Win64` server target and record the result.

## Repository Layout

```text
Config/              Unreal project config
Content/             Unreal assets, maps, UI, audio
Source/              Unreal C++ gameplay modules
apps/backend/        TypeScript non-authoritative operations backend prototype
apps/admin/          TypeScript admin UI placeholder
Tools/               Build, QA, asset, ops, and simulation utilities
docs/                Design, production, legal, and Steam planning
references/          Local reference manifests and ignored private research copies
```

## Windows Handoff Status

Development moves to Windows after the current Mac handoff. Clone `main`, install Git LFS, Unreal Engine 5.7, Visual Studio 2022 C++ tools, and run the validation sequence in [Windows Handoff](docs/windows-handoff.md).

The repository now avoids committing Mac generated build outputs. `Tools/quality_gate.py`, `Tools/unreal_gate.py`, and `Tools/ue/run_local_smoke.py` accept Windows paths and `--platform Win64`; set `UE_ROOT` if Unreal is not installed in the default Epic path.

## Historical Mac Status

Git LFS is installed. Epic Games Launcher is installed at `/Applications/Epic Games Launcher.app`. Unreal Engine 5.7 is installed at `/Users/Shared/Epic Games/UE_5.7`.

UE project files were generated with `GenerateProjectFiles.sh`, producing `AbyssLock (Mac).xcworkspace` and generated `Build/Mac/Resources` metadata. Xcode 26.5 is selected, `AbyssLockEditor` and `AbyssLock` Mac Development builds succeed, `python3 Tools/quality_gate.py --require-ue` passes, and `python3 Tools/unreal_gate.py --skip-generate --include-server` reports Editor/Game pass with Server blocked.

`AbyssLockServer` did not build on the Mac Launcher UE distribution because UnrealBuildTool reported: `Server targets are not currently supported from this engine distribution.` Dedicated Server verification should be retried on Windows with `--platform Win64`; if it is still blocked, use a UE source build.

The UE module is still named `AbyssLock` internally because it was created before the Frostwake direction pivot. Public/display naming is now Frostwake. Rename the UE module after the first successful UE Editor build, or keep it as an internal codename if renaming would slow the greybox.

The first generated whitebox map asset exists at `/Game/Maps/L_IcebreakerWhitebox` and is configured as both `GameDefaultMap` and `EditorStartupMap`. Regenerate it with `Tools/ue/create_icebreaker_whitebox.py` and validate it with `Tools/ue/validate_icebreaker_whitebox.py` after layout changes.

Near-future visual work must stay separate from the automation whitebox. Use `python3 Tools/ue/scaffold_frostwake_visual_poc.py --dry-run` or `--write` to create a local ignored manifest for `/Game/Maps/L_FrostwakeVisualPOC`; do not rename the whitebox or change default maps for art experiments.

The first ship task implementation is in C++ as `AAbyssShipTaskActor`. It supports server-authoritative repair and sabotage interactions, updates replicated ship system state, logs `ship_task_applied`, and calls match-end evaluation. The generated whitebox map now contains task actors for radio, power, fuel, flooding, and route loops. The next step is validating a 1-2 client interaction smoke test.

Local listen-server smoke tests use `Tools/ue/run_local_smoke.py`. The current uncooked-map path runs through `UnrealEditor -game`; it can start a host-only smoke or launch localhost clients while collecting logs under `Saved/SmokeTests`.

The current automated smoke path has validated one listen host plus one localhost client, dev role assignment with one saboteur, and server-side task application for both sabotage and repair through `-AbyssSmokeInteract`.

Smoke runs now write per-run JSONL telemetry through `-AbyssEventLog=...`. Each event includes session/run/build/map/profile metadata, sequence, and elapsed seconds. `Tools/log_summary.py` can summarize match start/end, duration, connection lifecycle, role assignment, and repair/sabotage task counts from those files.

The first ready-lobby path is implemented in C++: PlayerState replicates ready state, PlayerController exposes a server ready RPC, and GameMode starts a 5-8 player match when every connected player is ready. Non-shipping smoke flags allow 2-player local validation.

The first life-state loop is also in place: character health replicates, server damage can move a player to `Downed`, and rescue returns them to `Alive` with partial health. The smoke runner can validate this through `--smoke-down-rescue`.

The local smoke runner has also reached the first supported match size: one listen host plus four localhost clients can all become ready and start a 5-player match with 1 saboteur.

Containment state is scaffolded as a server-authoritative life-state transition as well. `--smoke-containment` validates contain/release events and JSONL summary counts.

The first complete crew-side match path is working in smoke: 5 players ready, 1 saboteur is assigned, route repair tasks advance replicated route progress to FinalApproach, and the crew win condition ends the match.

The first saboteur-side match path is also working in smoke: 5 players ready, 1 saboteur is assigned, repeated critical-system sabotage drives a ship system to zero, and the fatal ship state ends the match with a saboteur win.

Bulkhead lock sabotage now has its first network smoke path: the whitebox map contains three `AAbyssDoorActor` bulkhead doors, and 5-player ready-lobby smoke validates saboteur lock, blocked interaction while locked, repair release, and reopening. `Tools/log_summary.py` reports dedicated `bulkhead_*` counters for this path.

Pump/flooding sabotage now has its first pressure loop as well. Flooding pressure is derived from the replicated flooding system condition, two flooding sabotages raise pressure to 0.70, one pump repair lowers it to 0.35, and the 5-player smoke validates the partial-pressure state without ending the match.

The first PvE enemy gate is also in place. `AAbyssPveEnemyActor` is a minimal replicated server-authoritative damage source, and the 5-player smoke spawns it, applies damage, confirms the target enters `Downed`, then rescues the target so future combined smoke runs start from a stable life state.

The first item drop loop is now covered too. Inventory contents replicate owner-only, `DropItem` is bound to `Q`, dropped pickups are server-spawned replicated `AAbyssItemPickupActor` instances, and the 5-player smoke validates add, drop, pickup interaction, consumption, and inventory restoration.

The first combined non-terminal systems smoke also passes. In one 5-player ready-lobby match, it exercises item drop/re-pickup, down/rescue, containment/release, bulkhead lock/release, pump/flooding pressure, and PvE enemy damage, then confirms the match remains `InProgress`.

The first QA bot automation smoke is in place as an automation-only pawn. It spawns on the server, moves through scripted steps without NavMesh, interacts with a server-spawned pickup, and verifies the bot inventory received the item. PlayerState-backed door and task bot profiles are now covered by `qa-player-bot` and `qa-task-bot`; NavMesh movement remains later work.

Local scale smoke has reached the target player count. The ready-lobby path now validates 6 players with 1 saboteur and 8 players with 2 saboteurs by launching one listen host plus localhost clients through `Tools/ue/run_local_smoke.py`.

Common smoke runs now have named profiles. Use `python3 Tools/ue/run_local_smoke.py --profile qa-bot --skip-build --null-rhi` for the light bot gate, `--profile match-timer` for the match timer expiry gate, `--profile life-action` for interactable rescue/contain/release, `--profile combined5` for the 5-player non-terminal integration gate, `--profile ready8` for the 8-player ready-lobby scale gate, and `--profile combined8` for the 8-player non-terminal integration gate. `--describe-profile` prints the effective settings without launching Unreal.

The 8-player combined systems gate now passes. In one 8-player ready-lobby match with 2 saboteurs, it exercises item drop/re-pickup, down/rescue on a third crew target, containment/release, bulkhead lock/release, pump/flooding pressure, and PvE enemy damage while keeping the match in progress.

The QA bot path now has a PlayerState-backed variant. `--profile qa-player-bot` starts a 5-player ready lobby, moves an assigned player pawn to a bulkhead, opens and closes it through the normal role/life-gated door interaction, and confirms the match remains in progress.

`--profile qa-task-bot` extends the PlayerState-backed QA path to ship tasks. It uses assigned crew and saboteur pawns, applies sabotage to a non-terminal ship system, repairs it back to full condition, and verifies the match remains in progress.

Smoke profiles can also be run as a suite. `python3 Tools/ue/run_smoke_suite.py --skip-build --null-rhi` runs the quick suite and writes per-profile logs plus `suite_summary.json` under `Saved/SmokeSuites`; add `--include-heavy` to include task, combined, and 8-player profiles.

Suite summaries can be exported to Markdown with `python3 Tools/ue/export_smoke_suite_markdown.py <suite_summary.json>`, producing a compact evidence table beside the JSON. The first heavy suite has passed across `qa-bot`, `qa-player-bot`, `qa-task-bot`, `combined5`, `ready8`, and `combined8`.

Playtest telemetry is now ready for the first P1-024 human run. `client_connected` and `client_disconnected` events are counted by `Tools/log_summary.py`, and terminal smoke logs report `match_duration_seconds` when `match_ended` is emitted.

An anonymized summary template is available at `docs/playtests/p1-024-summary-template.md`; use it to convert raw local evidence into pass/partial/fail, keep/cut/change, and the P1-025 top-blocker list.

`Tools/playtest_summary.py` can generate that Markdown skeleton from a `Tools/log_summary.py --out` JSON file. It reads summary JSON only, not raw event logs or recordings.

`docs/playtests/p1-024-dry-run-summary-example.md` is a committed non-human example generated from smoke telemetry. Use it to verify summary format and redaction expectations; do not count it as P1-024 human evidence.

`Tools/playtest_preflight.py` checks a generated P1-024 summary before it is committed:

```bash
python3 Tools/playtest_preflight.py Saved/Playtests/P1-024/run-01/summary.json --markdown docs/playtests/p1-024-run-01-summary.md
```

`Tools/playtest_run_scaffold.py` creates the ignored local folder and run scripts for a P1-024 human session:

```bash
python3 Tools/playtest_run_scaffold.py --run-number 1 --target-players 6
```

## Next Execution Target

Phase 1 begins with a greybox match loop:

- 5-8 local or LAN clients
- lobby and match start
- crew/saboteur assignment
- interactable hatches, items, repair tasks, and sabotage tasks
- down, rescue, containment, or death
- win/loss resolution
- replayable JSONL logs sufficient for playtest analysis
