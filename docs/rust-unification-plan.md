# Rust Unification Plan

## Policy

Rust is the default language for new Frostwake software outside Unreal's required engine integration files. Backend services, admin tooling, validation tools, log analysis, data generation, and local automation should move to Rust crates with `cargo test` coverage.

## Current Migration State

- `apps/backend` has been rewritten as the `frostwake-backend` Rust crate.
- `frostwake-tools` now owns server config checks, lobby metadata checks, asset ledger validation, reference inventory generation, redaction helpers, banlist helpers, the Phase 1 Steam registry placeholder, secret scanning, log summaries, playtest run scaffolding, playtest summary skeletons, playtest preflight checks, and playtest report upload payloads.
- The repository root is now a Cargo workspace.
- The quality gate runs `cargo fmt --all --check` and `cargo test --workspace` when Cargo is available.
- The quality gate rejects active references to retired non-Rust tool files. `docs/cycles` remains historical evidence and is intentionally excluded from this retired-reference check.
- The local Rust toolchain can live under ignored `Tools/install/rust` without modifying the user's global PATH.

## Unreal Boundary

Unreal Engine still requires C++ headers/source for reflected `UCLASS`, `USTRUCT`, RPC, replication, module, and target declarations. C# target/build files are also Unreal Build Tool inputs. Those files cannot be deleted while this remains an Unreal project.

The migration target is:

- keep the smallest possible Unreal C++/C# boundary;
- move pure rules, validation, serialization, balancing, and deterministic simulation into Rust crates where Unreal can call them through an FFI layer;
- keep server authority in Unreal Dedicated Server unless the game engine itself changes.

## Remaining Migration Work

- Keep `Tools/ops` as schemas, examples, and runbooks. New ops behavior should be implemented in Rust, either as `frostwake-tools` subcommands or backend endpoints.
- Playtest log summary, run scaffolding, summary generation, preflight, and report upload are now covered by `frostwake-tools`.
- Port smoke-suite orchestration to Rust while preserving Windows/UE process behavior.
- Replace shell and PowerShell wrappers with Rust executables where they do not need shell-native integration. Secret scanning and reference inventory generation have moved into `frostwake-tools`.
- Add Rust integration tests for each port before removing the legacy script.
