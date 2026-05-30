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
  **Foundation is now LIVE, not just merged (plan §9.5 holds the per-step LIVE/SCAFFOLD truth, kept accurate):**
  the **§3.15 5-attribute consolidation is complete** (Warmth=temperature-driven §3.22-23 / Hunger / Health on
  `UFrostwakeAttributeComponent`; the character holds no hand-written vitals floats); the **Action System runs
  on both halves** — Effect (`UFrostwakeColdExposureEffect` debuff + `UFrostwakeColdResistPerkEffect` perk) AND
  Action (`UFrostwakeFogAbility`, the first ability: the base now carries cooldown/cost/duration/instigator, with
  Stamina as its first consumer; `dev_smoke_ability`); the **match spine has a
  subscriber** (`GameMode` listens to `OnPlayerDied`→`EvaluateMatchEnd`); the **data path is editor-baked**
  (JSON→`.uasset`→AssetManager, cook-safe); the **inventory container stacks** (`FFrostwakeInventoryEntry{ItemId,Count}`
  honours `ItemDefinition.MaxStack`, review#2a — held-item visibility to others is the queued #2b). **§3.17 typed damage is LIVE** (DamageMultiplier + ReservedHealth/poison
  routing + carcass→revive) and **perk damage-resistance now lands via an ActionEffect** — §8 rule: combat/perk/
  effect modifiers go through Action/ActionEffect, never raw methods. All asserted by `run-local-smoke`
  (`dev_smoke_damage_type`/`down_rescue`/`perk_resist` = cold 10×50% resist→Health−5, plus single-player/host-only green).
  **Next, serially per plan §4/§6:** remaining data-type seed (weapon/recipe/role/perk JSON + re-bake) and the systems
  that consume them → ship systems → items(55) → recipes(47) …. **Test-harness debt (plan item D, partial):** the
  5 player-acting smokes (perk_resist/eat/damage_type/survival/effect) now live in `FrostwakeDevSmoke.{h,cpp}` as free
  functions (GameMode keeps only a 1-line trigger each), all gated in `run-local-smoke`; the remaining ~13
  GameMode-coupled `RunDevSmoke*` (FindPawnForTeam/match-config/role helpers) + ≥1 `AFunctionalTest` are still TODO.
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
