# CLAUDE.md — Frostwake (codename AbyssLock)

8-player cooperative betrayal survival game. UE 5.7 (C++ server-authoritative) + Rust tools
& non-authoritative backend + Steam/dedicated-server target. Public name: **Frostwake**.
The repo path/UE module may still say `dread-hunger-2` / `AbyssLock` internally.

## How this project is driven

Work is organized as **10 continuous parallel lanes (GP-01…GP-10)** advanced one small,
verifiable step at a time. **If you are starting work, read
[`docs/orchestration/README.md`](docs/orchestration/README.md) first**, then
[`docs/orchestration/DISPATCH.md`](docs/orchestration/DISPATCH.md) (the launch instruction),
[`STATUS.md`](docs/orchestration/STATUS.md), and the relevant
`docs/orchestration/lanes/GP-0X.state.md`.

The iteration loop and its cadence are in [`docs/iteration-loop.md`](docs/iteration-loop.md);
agent roles/handoff gates in [`docs/agent-operating-model.md`](docs/agent-operating-model.md).

## The one tool that drives everything

`frostwake-tools` (Rust CLI). Run from repo root:

```
cargo run -p frostwake-tools -- <verb> [args]
```

Key verbs: `quality-gate [--require-ue]`, `unreal-gate [--include-server] [--platform Win64]`,
`run-local-smoke`, `run-smoke-suite`, `new-cycle`, `log-summary`, `playtest-preflight`,
`playtest-run-scaffold`, `lobby-metadata-check`, `lobby-join-decision`, `asset-ledger-check`,
`secret-scan`, `create-frostwake-visual-poc`, `validate-frostwake-visual-poc`.

## Non-negotiables

- **Match authority stays in Unreal C++ / dedicated server.** Rust backend is
  non-authoritative (reports, stats, fleet metadata, lobby directory).
- **Evidence over memory.** Every claim links a command + artifact (`docs/cycles/`,
  `Saved/SmokeSuites/…`, manifests). Distinguish pass / fail / blocked / not-run.
- **Never commit** secrets, SteamIDs, IPs, raw voice/chat, PII, or private reference files.
- **No unverified public claims** (store copy, features). IP/rights gates apply before any
  third-party asset adoption. **Expression is phase-gated** (`docs/ip-boundary.md` Development
  Phase Policy): DH-near placeholders are allowed during prototyping and originalized before any
  public exposure; mechanics may match DH freely. Never commit/distribute DH files or circumvent
  DRM in any phase.
- **PowerShell is launch-wrappers only**; durable logic lives in Rust or UE C++.
- `docs/cycles/` is historical evidence — old command names there are not active instructions.

## Current hard blocker (verify, don't assume)

Only a **Launcher UE_5.7** is installed locally (no `UE_ROOT`), which **cannot build Server
targets**, so `Binaries/Win64/AbyssLockServer.exe` does not exist. This blocks the
*implementation* of GP-02 (dedicated server) and GP-04 (Steam lobby spike) and the live
GP-01 human run. All other lanes (and the design/config/blocker-evidence parts of the gated
ones) are workable now.

## Build/verify quickly

- Rust: `cargo test --workspace` (use `CARGO_TARGET_DIR=target/loop-gpNN` to avoid lock
  contention with another running build).
- Repo gate: `cargo run -p frostwake-tools -- quality-gate`.
- Commit policy for the autonomous loop: small, path-scoped commits to `main` with the
  `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>` trailer.
