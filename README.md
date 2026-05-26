# Frostwake

Working title for an 8-player cooperative betrayal survival game set aboard a fictional near-future icebound research vessel.

This repository is the research and prototype workspace. The immediate goal is not a trailer or art pass. The goal is to lock the playable shape, then build a rough 8-player greybox prototype that proves whether people naturally want another match.

## Current Direction

- Genre: PvPvE cooperative survival social deduction
- Players: 8 fixed
- Teams: crew vs. 2 saboteurs/agents
- Match length target: 18-25 minutes for production, up to 35 minutes in early whitebox tests
- Setting: fictional near-future icebound research vessel crossing a hostile polar route toward a rescue signal
- Core loop: maintain hull, fuel, power, radio, route, heat, and crew survival while identifying sabotage
- Voice: proximity voice is a planned core feature; validate a proven SDK/provider before any public claim, and do not build custom voice transport
- Match server: Unreal Dedicated Server / C++ replication
- Discovery: Steam Lobby, Steam Server Browser, Steam Game Servers API
- Network protection: Steam Networking Sockets / Steam Datagram Relay where applicable
- Backend: Rust for reports, stats, admin APIs, fleet metadata, and non-authoritative matchmaking lobby directory
- Tools: Rust CLI for operations/playtest automation; PowerShell remains only as Windows launch wrappers
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
- [Project Structure Review](docs/project-structure-review.md)
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
- [Accessibility Baseline](docs/accessibility-baseline.md)
- [AI Content Disclosure](docs/ai-content-disclosure.md)
- [Asset Ledger](docs/asset-ledger.md)
- [Asset Ledger Candidates](docs/asset-ledger-candidates.csv)
- [Asset Scorecard](docs/asset-scorecard.md)
- [Visual POC Rights Gate](docs/visual-poc-rights-gate.md)
- [Visual POC Import Checklist](docs/visual-poc-import-checklist.md)
- [IP Risk Register](docs/ip-risk.md)
- [Competitive Analysis](docs/competitive-analysis.md)
- [Phase 1 Backlog](docs/phase1-backlog.md)
- [Phase 2 Backlog](docs/phase2-backlog.md)
- [Steam Phase 2 Integration Plan](docs/steam-phase2-integration-plan.md)
- [Steam Dev Config Gate](docs/steam-dev-config-gate.md)
- [Steam Lobby Metadata Contract](docs/steam-lobby-metadata-contract.md)
- [Steam Lobby Subsystem Design](docs/steam-lobby-subsystem-design.md)
- [Steam Lobby Validation Template](docs/steam-lobby-validation-template.md)
- [Steam Dedicated Server Client Join Plan](docs/steam-dedicated-server-client-join.md)
- [Voice Chat Interface Spike](docs/voice-chat-interface-spike.md)
- [Voice Provider Validation Template](docs/voice-provider-validation-template.md)
- [Issue Import](docs/issue-import/README.md)

## Phase 1 Current Milestone

- [Phase 1 Milestone Report](docs/phase1-milestone-report.md)
- [P1-024 Human Playtest Packet 1](docs/playtests/p1-024-human-test-1.md)
- [P1-024 Observer Sheet](docs/playtests/p1-024-observer-sheet.md)
- [P1-024 Post-Round Survey](docs/playtests/p1-024-post-round-survey.md)
- [P1-024 Anonymized Summary Template](docs/playtests/p1-024-summary-template.md)
- [P1-024 Dry-Run Summary Example](docs/playtests/p1-024-dry-run-summary-example.md)
- Current verdict: local listen-server automation milestone reached; human 6-8 player tests remain the next proof point.
- Strongest evidence: `Saved/SmokeSuites/suite-20260525-050522/suite_summary.md` from `cargo run -p frostwake-tools -- run-smoke-suite --include-heavy --skip-build --null-rhi`.
- Server target status: Launcher UE blocks `AbyssLockServer` on current validation machines; use a source-built or otherwise server-capable UE distribution before dedicated runtime probes can pass.

## Repository Layout

```text
Config/              Unreal project config
Content/             Unreal assets, maps, UI, audio
Source/              Unreal C++ gameplay modules
apps/backend/        Rust non-authoritative operations backend prototype
apps/admin/          Rust-first admin UI placeholder
crates/              Shared Rust tools and future reusable non-Unreal crates
Tools/               PowerShell Windows wrappers plus non-executable schemas/runbooks
docs/                Design, production, legal, and Steam planning
references/          Local reference manifests and ignored private research copies
```

Current consistency guardrails:

- `docs/cycles/` is historical evidence; old command names there are not active instructions.
- Active docs must not reference retired non-Rust tool files after their behavior has moved to Rust or UE C++ commandlets.
- `Tools/ops/` is schemas, examples, and runbooks only; executable ops behavior belongs in `crates/frostwake-tools` or the Rust backend.
- The repository path/remote may still contain the old `dread-hunger-2` name during development. Public naming, store copy, and player-facing docs must use Frostwake.

## Windows And Dedicated Server Status

Windows is now the active validation platform. Use [Windows Handoff](docs/windows-handoff.md), [Windows Validation Template](docs/windows-validation-template.md), and [Windows Dedicated Server Runbook](docs/windows-dedicated-server-runbook.md) for current operator steps.

`AbyssLockServer` is still blocked on Launcher UE distributions with `Server targets are not currently supported from this engine distribution.` Dedicated boot, client-join, and 8-player ready-lobby wrappers now write early-blocker `summary.txt` and `manifest.json` evidence, but runtime assertions cannot pass until `UE_ROOT` points at a source-built or otherwise server-capable UE distribution.

## Current Prototype Status

- The UE module is still named `AbyssLock` internally; public/player-facing naming is Frostwake.
- `/Game/Maps/L_IcebreakerWhitebox` remains the automation map and default map. Visual POC work must stay in `/Game/Maps/L_FrostwakeVisualPOC`.
- Local listen-server smoke uses `cargo run -p frostwake-tools -- run-local-smoke`; common profiles include `qa-bot`, `qa-player-bot`, `qa-task-bot`, `match-timer`, `life-action`, `combined5`, `ready8`, and `combined8`.
- Current smoke evidence covers 5-8 player ready flow, 8-player role assignment with 2 saboteurs, ship tasks, route win, fatal-system/timer saboteur wins, doors, bulkhead lock/release, item pickup/drop, down/rescue, containment/release, flooding pressure, PvE damage, and JSONL summaries.
- P1-024 human playtest materials are ready: packet, observer sheet, survey, summary template, dry-run example, `playtest-run-scaffold`, `playtest-summary`, and `playtest-preflight`.
- Steam Lobby metadata and join-decision contracts have a Null/LAN-safe C++ foundation in `UAbyssLobbySubsystem`; Steam create/find/join runtime integration remains gated.

## Next Execution Target

1. Point `UE_ROOT` at a source-built or otherwise server-capable UE 5.7 distribution.
2. Run `cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server`.
3. After `AbyssLockServer.exe` exists, run `.\Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate` and record the child manifests.
4. Run the P1-024 human 6-8 player test from `cargo run -p frostwake-tools -- playtest-run-scaffold --run-number 1 --target-players 8` when Windows smoke readiness is confirmed.
