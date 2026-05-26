# GP-06: Product Services And Tools

## North-Star Goal

Non-authoritative services and Rust-first tools make testing, reporting, moderation, server metadata, and operations safer and easier without owning real-time gameplay.

## Why This Exists

The game needs surrounding services, but the match remains Unreal authoritative. This portfolio continuously improves the support layer while protecting that boundary.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| API contract health | Implemented backend behavior matches OpenAPI and docs |
| Test coverage | Cargo workspace tests cover service and tool behavior |
| Data safety | Reports, banlists, logs, and summaries avoid personal/raw sensitive data in commits |
| Operational usefulness | Tools reduce manual launch, validation, summary, and moderation work |
| Boundary clarity | Backend never owns movement, combat, role, inventory, sabotage, or match result authority |
| Rust consistency | Durable non-Unreal automation moves into Rust with tests |

## Improvement Questions

- Which manual operation is most error-prone and should become a tested Rust tool?
- Which backend endpoint lacks contract parity or useful tests?
- Are report and banlist data useful without unsafe identity leakage?
- Is an ops workflow optional for Phase 1 and reliable enough for later phases?
- Are PowerShell scripts only launch wrappers, or have durable rules leaked into them?

## Evidence Standard

Evidence should include passing `cargo fmt --all --check`, `cargo test --workspace`, API tests, OpenAPI parity notes, schema validation, and data-retention notes.

Primary docs:

- `docs/backend-contract.md`
- `docs/rust-unification-plan.md`
- `docs/server-hosting-model.md`
- `docs/playtest-data-and-moderation.md`
- `apps/backend/README.md`
- `apps/backend/openapi.yaml`

## Boundaries

- Do not move authoritative match logic into the backend.
- Do not make Phase 1 human tests depend on backend uptime.
- Do not store secrets, raw voice, transcripts, private logs, IPs, or raw SteamIDs in committed artifacts.

## Review Cadence

Review with each backend/API change and before each external tester wave. The main question is whether the support layer made operations safer without making gameplay coupling worse.

## Agent Charter

An agent assigned to this portfolio should improve one service/tool health signal. A good assignment is: "Find the largest mismatch between backend implementation, OpenAPI, and tests, then close it or document the blocker."
