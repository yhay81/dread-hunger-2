# Tools

Utility area for:

- multi-client launch scripts
- log collection and crash bundling
- bot command profiles
- balance simulation
- asset import checks
- local reference inventory without copying third-party files
- Steam build/upload notes

Implemented:

- `cargo run -p frostwake-tools -- quality-gate`: Rust quality-gate orchestrator for repository, Rust, PowerShell, Unreal metadata, asset-ledger, and secret checks.
- `cargo run -p frostwake-tools -- backlog-to-issues`: exports phase backlog rows to issue-import CSV and Markdown.
- `cargo run -p frostwake-tools -- github-issue-sync`: dry-runs or creates GitHub issues from the generated issue-import CSV. It defaults to Phase 1; pass `--csv docs/issue-import/phase2-issues.csv` for Phase 2.
- `cargo run -p frostwake-tools -- unreal-gate`: runs UE project generation and local Editor/Game build gates.
- `cargo run -p frostwake-tools -- new-cycle`: creates the next cycle record under `docs/cycles/`.
- `cargo run -p frostwake-tools -- log-summary`: Rust replacement for JSONL match telemetry summaries.
- `cargo run -p frostwake-tools -- run-local-smoke`: launches a local UE listen-server smoke test and optional localhost clients.
- `cargo run -p frostwake-tools -- run-smoke-suite`: runs named smoke profiles; pass `--platform Win64` on Windows.
- `windows/run_single_player.ps1`: launches a visible one-player `UnrealEditor.exe -game` session for manual testing.
- `cargo run -p frostwake-tools -- playtest-run-scaffold`: creates ignored P1-024 local run folders with Windows PowerShell and POSIX helper scripts.
- `cargo run -p frostwake-tools -- playtest-summary`: generates the anonymized human-test summary skeleton.
- `cargo run -p frostwake-tools -- playtest-preflight`: checks summary privacy, placeholders, and objective gates.
- `cargo run -p frostwake-tools -- playtest-report-upload`: builds or optionally uploads an anonymized playtest report payload to the local backend.
- `cargo run -p frostwake-tools -- asset-ledger-check`: Rust validation for candidate asset provenance and approval evidence.
- `cargo run -p frostwake-tools -- reference-inventory`: creates ignored metadata inventories for local research copies.
- `cargo run -p frostwake-tools -- redact-json`, `banlist-list`, `banlist-add`, and `steam-registry-list`: Rust ops helpers.
- `windows/`: Windows-only first-run, dedicated-server, Phase 2 entry, Steam dev config, and Steam Lobby validation helpers.
- `ops/`: safe local ops contracts and config schemas. The redaction, banlist, Steam registry, and config validators live in `crates/frostwake-tools`.

Tests:

- `cargo test --workspace`: Rust service and tool tests, including the telemetry and playtest evidence pipeline.
- `cargo run -p frostwake-tools -- quality-gate`: runs the full local gate. Script-language source files are no longer accepted in tracked project tooling.
