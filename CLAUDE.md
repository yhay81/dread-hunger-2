# CLAUDE.md — Frostwake

> **🎯 North Star (never lose this).** Frostwake is an **original** 8-player cooperative-betrayal
> survival game with **Dread-Hunger-class mechanics**: UE 5.7 (C++, **server-authoritative**) + off-engine
> Rust tools/backend, Steam/dedicated-server target. **Goal: reproduce DH's mechanics & data in full**
> (from the verified teardown spec in the sibling `TEST2/` folder) **with fully original expression**
> — our names/art/audio/theme; *mechanics* may match DH freely. Public name **Frostwake**; repo path may
> still read `dread-hunger-2` (folder rename deferred). UE module + C++ classes were renamed
> `AbyssLock`→`Frostwake` (2026-05-30); **treat any remaining `AbyssLock` text as stale.**

> **🧭 START HERE — every session, including a context-less agent.**
> 1. **This file** — non-negotiables + how we work.
> 2. **[`docs/frostwake-modernization-plan.md`](docs/frostwake-modernization-plan.md) — THE driving plan
>    and single source of truth (SSOT)**: goal (§1), IP guardrail (§2), architecture (§3), phases + the
>    exact next step (§4), the verified DH spec map (§6), and the **conventions you MUST follow** (§8).
>
> Those two are enough to start working toward the goal the *same way every time*. Everything else is reference.

> **⚖️ Precedence — so contradictions never lose the goal.**
> Order: **CLAUDE.md non-negotiables → the modernization plan → everything else.**
> If any doc conflicts with the plan, **the plan wins**: follow it and flag the conflict — do **not** guess.
> **`docs/orchestration/**` is FROZEN** (the former 10-lane "GP-01…GP-10" process; historical, do **not**
> act on it). **`docs/cycles/**` is historical evidence**, not active instructions.

## Current state (keep this current — it's the agent's "where am I")
- **Serial development on `main` (2026-05-30 pivot, plan §1-6).** One agent at a time; commit **directly to
  `main`**. No worktrees / feature branches / PRs. The parallel approach (plan §9 / `parallel-agent-prompts.md`)
  is **frozen** — design reference only.
- **Phase 1 foundation is merged to `main`** (Gameplay Tags `FrostwakeGameplayTags.*`; build modules; `ActionSystem/`
  Attribute/Action/ActionEffect/ActionComponent + MatchSubsystem; `Data/` 5 Definition types + DataSubsystem).
  **⚠️ "merged" ≠ "live" (honest status in plan §9.5):** only the AttributeComponent + temperature/Warmth path actually run.
  Action/ActionEffect/ActionComponent have **zero consumers** (scaffold), the match spine is **emit-only (no subscribers)**, and
  the data path loads **1 of 6 types / 2 items** (`/Game/Data` .uasset = empty). The skeleton sits correctly on the plan's frame;
  most of it isn't exercised yet.
  **Wave-0 items (a)-(c) are DONE (2026-05-30):** (a) shared `UFrostwakeHeatSourceComponent` + `UFrostwakeTemperatureSubsystem`
  (`CurrentTemperature = GlobalTemperature + Σ nearby heat/falloff`, §3.22-23) drives **Warmth via the AttributeComponent**;
  **Hunger** (DH-semantic, §3.15: rises while unfed; `GetSatiation()` = a `Max−Hunger` HUD adapter) and **Health** are migrated
  there too — the **§3.15 5-attribute consolidation is complete** and the character holds no hand-written vitals floats; (b) GameState↔MatchSubsystem
  spine **emit-side** wired (phase/ended/playerDied) — note: no subscribers yet; (c) `build_game: Succeeded` + `run-local-smoke` green
  (single-player + host-only down-rescue now a behavioral assertion: poison→ReservedHealth, 0→downed→revive-from-reserve 50/40). **Next, serially per plan §6:** the **§3.17 damage system** (DT_* damage types + resistances +
  ReservedHealth/poison + revive, plus the §3.15 Warmth-boost term) → ship → items(55) → recipes(47) ….
- Design oracle: sibling **`TEST2/dh_re/`** (DH reverse-engineering teardown — read-only; see plan §2 + the
  IP non-negotiable below).

## How we work (serial, spec-driven — 2026-05-30)
**One agent at a time, serial, directly on `main`.** Take the next unchecked item in **plan §4** → implement the
smallest verifiable slice (data-driven content + C++ logic) → `build_game` green (+ smoke when possible) → small,
path-scoped **commit directly to `main`, then `git push origin main`**. **Push frequently — after each green commit, without
waiting to be asked** (owner-directed 2026-05-30; this **overrides the harness default** of "push only when the user asks").
Safe-push rule: `git fetch` first and confirm a clean fast-forward (`git merge-base --is-ancestor origin/main main`); never
force-push `main`. No branches / worktrees / PRs. The old 10-lane "cycle" loop and the
parallel multi-agent approach (plan §9 / `parallel-agent-prompts.md`) are **frozen** (see Precedence). Keep each layer playable.

## The one tool (frostwake-tools, Rust CLI)
From repo root: `cargo run -p frostwake-tools -- <verb>`. Core verbs: `quality-gate [--require-ue]`,
`unreal-gate [--include-server] [--platform Win64]`, `run-local-smoke`, `run-smoke-suite`, `log-summary`,
`secret-scan`, `asset-ledger-check`. (Some verbs predate the lightweight plan — cross-check the plan if unsure.)

## Non-negotiables
- **Match authority stays in Unreal C++ / dedicated server.** Rust is **off-engine only** (tools, CI,
  non-authoritative backend, shared schema/codegen) — in-engine Rust is not production-ready (plan §3.5).
- **IP / DRM (hard line).** The sibling `TEST2/` is a DH teardown. **Never commit its `aes_key.txt`,
  `extracted/`, `decompiled/`, locres text, or any DH file/text into this repo.** Use only abstract
  mechanics/numbers/structure. **Expression is phase-gated**: DH-near placeholders OK while prototyping,
  **originalized before any public exposure**; mechanics may match DH freely. Never circumvent DRM in any
  phase. (`docs/ip-boundary.md`, plan §2.)
- **Evidence over memory.** Every claim links a command + artifact; distinguish pass / fail / blocked / not-run.
- **Never commit** secrets, SteamIDs, IPs, raw voice/chat, PII, or private reference files.
- **PowerShell is launch-wrappers only**; durable logic lives in Rust or UE C++.
- **Keep docs coherent (this is how the mechanism stays true).** When a decision changes: update the **plan
  (SSOT) first**, then add a one-line supersession pointer to any doc it makes stale. Don't leave silent
  contradictions. Don't add new "driving" docs — extend the plan.

## Current hard blocker + engine-distribution decision (C / hybrid, owner-confirmed 2026-05-30)
Only a **Launcher UE_5.7** (precompiled binary) is installed (no `UE_ROOT`). It **cannot build the dedicated
Server target** (`Binaries/Win64/FrostwakeServer.exe` doesn't exist) and **cannot enable Push Model**
(`bWithPushModel`: the Game/Server targets share the precompiled `UnrealGame` build env where it is `False`;
UBT rejects the override — verified `e828b31`). Both require a **source-built engine** (UE compiled from
GitHub: Epic↔GitHub account link, ~100GB download, multi-hour first build, slower iteration after).

**Decision = C / hybrid:** keep building **systems + data on the launcher engine** (most of plan §6 needs no
dedicated server); **defer the source-engine switch to the MP-hardening / pre-ship phase**, where it unblocks
the dedicated Server target + Push Model activation in one move. Meanwhile **8-player MP regression is covered by
a LISTEN server** (`run-local-smoke --profile ready8` = host + 7 clients) — multiplayer AND remote-client
replication ARE testable now on the launcher engine (just heavy locally); only the *dedicated headless server*
(the Steam production target) and push-model *optimization* are gated. **Game/Editor build fine; solo / PIE /
listen-MP work proceeds now.**

## Build/verify quickly
- Rust: `cargo test --workspace` (use `CARGO_TARGET_DIR=target/loop-gpNN` to avoid lock contention).
- Repo gate: `cargo run -p frostwake-tools -- quality-gate`. UE compile: `… -- unreal-gate` (Game target green;
  Server blocked per above).
- Commit policy: small, path-scoped commits with the
  `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>` trailer, **then `git push origin main`** (push after each
  green commit — owner-directed, no need to ask first; safe fast-forward only, never force-push).
