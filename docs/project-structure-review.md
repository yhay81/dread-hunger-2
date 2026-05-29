# Project Structure Review

Date: 2026-05-25

## Findings

- `apps/backend` is now a Rust service and should not contain Node build output or package metadata. Ignored `dist/` and `node_modules/` are stale local artifacts and should be removed when found.
- `Tools/ops` is now schema and runbook territory. Executable ops helpers live in `crates/frostwake-tools`.
- Playtest telemetry, summary, preflight, report upload, reference inventory, asset ledger, and ops helper behavior now lives in `crates/frostwake-tools`. Keeping duplicate non-Rust implementations for the same commands creates a split source of truth and should be treated as stale.
- Active docs and README links were checked on 2026-05-25; no broken Markdown links were found outside immutable `docs/cycles` records.
- `Tools/ops/README.md` no longer lists non-existent helper component files. Future server listing, admin, cleanup, logging, ruleset, and persistence work should land as Rust backend endpoints or `frostwake-tools` commands.
- `cargo run -p frostwake-tools -- quality-gate` now rejects active script-language source files and retired tool path references so deleted migration targets do not reappear as project tooling.
- Unreal asset generation and validation now lives in the `FrostwakeEditor` C++ commandlets. Rust CLI commands launch those commandlets instead of carrying editor automation logic.
- `Tools/windows` remains PowerShell because it is the Windows operator surface. It should call Rust tools for validation and summarization instead of reimplementing logic.
- `docs/cycles` is historical evidence. Old command names inside cycle records are not active instructions and should not be rewritten unless a record is factually wrong.

## Target Layout

| Path | Role |
| --- | --- |
| `Source/` | Unreal-required C++/C# boundary, reflected gameplay classes, replication, and build targets |
| `Content/` | Unreal maps and assets only |
| `apps/backend/` | Rust non-authoritative operations backend |
| `apps/admin/` | Future Rust admin UI shell |
| `crates/frostwake-tools/` | Rust CLI for validation, reports, summaries, ledgers, inventories, and future reusable tool logic |
| `Tools/ue/` | Retired tool directory; do not add new executables here |
| `Tools/windows/` | Windows operator wrappers that call Rust and UE commandlets |
| `Tools/ops/` | JSON schemas, examples, and runbooks only |
| `docs/` | Current design, production, QA, legal, and platform plans |
| `docs/cycles/` | Immutable historical cycle notes |
| `references/` | Manifests and ignored local-only research inventory |

## Plan Check

The current Rust migration plan is directionally sound because it keeps the gameplay authority in Unreal while moving repeatable, testable non-Unreal software into Cargo crates.

The main risk is duplicating the same behavior across tool runtimes during migration. The rule from here should be:

1. Port behavior to `frostwake-tools`.
2. Move tests to Rust, or for UE asset behavior, validate through UE C++ commandlets.
3. Update active docs and wrappers.
4. Delete the old script.
5. Keep `docs/cycles` as historical evidence only.

## Better Alternative Considered

A custom Rust authoritative server would make the "all Rust" goal cleaner, but it would throw away Unreal replication, UHT/UBT integration, and the current smoke-test evidence. For this project, the better near-term architecture is a thin Unreal authority boundary plus Rust for surrounding services and tooling.

The next useful structural improvement is splitting `crates/frostwake-tools/src/main.rs` into modules once the current migration batch stabilizes. Suggested modules:

- `asset_ledger`
- `backend_payloads`
- `log_summary`
- `playtest`
- `ops`
- `reference_inventory`
- `schema`
- `secret_scan`

Do this as a mechanical refactor after the current behavior is stable, not while porting more features.
