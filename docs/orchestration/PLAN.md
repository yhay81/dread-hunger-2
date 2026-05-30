> ⛔ **FROZEN (2026-05-30) — superseded by [`docs/frostwake-modernization-plan.md`](../frostwake-modernization-plan.md).**
> This parallel-lane plan is **historical only — do NOT act on it.** Start at `CLAUDE.md` → the modernization plan.

# Frostwake Parallel Lane Plan (FROZEN — see banner)

Date re-planned: 2026-05-29 · Phase: 1 (local-automation milestone reached; human proof + Phase 2 prep)

This plan re-organizes all work into **10 parallel lanes** (the existing GP portfolios),
each with a **near-term milestone (M1)** that is concrete and verifiable, plus the
dependency graph that says which lanes can run in parallel right now and which are gated.

- **Lane charters** (stable, north-star + boundaries): `docs/subprojects/sp-0X-*.md`
- **Lane state** (mutable, current milestone + next action): `docs/orchestration/lanes/GP-0X.state.md`
- **How an agent advances a lane**: `docs/orchestration/DISPATCH.md`
- **Fast global picture**: `docs/orchestration/STATUS.md`

---

## The 10 lanes and their M1 milestones

| Lane | North-star (1 line) | M1 milestone | M1 gate (done when) |
| --- | --- | --- | --- |
| **GP-01** Human Playability | 6-8 humans finish, understand, want another round | First real 6-8p human playtest run + committed anonymized summary | `playtest-preflight --mode human` passes on a real P1-024 summary |
| **GP-02** Network/Hosting | 8p sessions stable, reproducible, hostable | `FrostwakeServer.exe` builds + boots + accepts 8 clients + ready-lobby match | `unreal-gate --include-server` pass **and** `run_phase2_entry_validation.ps1` pass |
| **GP-03** Core Match Experience | Every round = readable cooperation/suspicion/resolution — **DH-class mechanics+controls, original expression** (`docs/mechanics-parity-target.md`, `docs/control-scheme.md`) | Sabotage loss is explainable from in-match signals (not the rulebook) | Human readability note + refreshed `fatal-sabotage` smoke explanation path |
| **GP-04** Steam Online Access | Find/join/reject/host via Steam-safe paths | Lobby create/find/join spike with build/map mismatch rejection (P2-003/004) | `run_steam_lobby_validation.ps1` + `lobby-join-decision` prove join & reject |
| **GP-05** Voice & Trust | Proximity voice playable, abuse manageable | One voice provider chosen + 8-player acceptance plan written | Decision section of `voice-provider-validation-template.md` filled + reviewed |
| **GP-06** Services & Tools | Non-authoritative services/tools, safe boundaries | Backend impl ↔ `openapi.yaml` ↔ tests parity; ops tools tested | `cargo test --workspace` green + documented OpenAPI parity |
| **GP-07** Evidence/QA/Perf | Every claim reproducible; perf accountable | Performance budgets defined + first measurement method; all gates reproducible | Perf-budget doc committed + most-valuable missing telemetry field added |
| **GP-08** Presentation/Rights | Distinct, legally-safe identity & store material | Visual POC screenshots + keep/replace/reject review; asset provenance decided | POC review notes committed + ambientCG candidates promoted or rejected |
| **GP-09** Comprehension/A11y | New players understand goal, UI, loss reason | First-match comprehension checklist + stable strings + glossary consistency | Checklist committed + one highest-value comprehension change identified |
| **GP-10** Release/Community | Operate tests/releases with owners + rollback | Steam Playtest readiness snapshot (owner/artifact/status/blocker/cancel each) | Readiness table committed + highest-risk missing item named |

M2/M3 for each lane live in the lane state file's backlog; promote the next milestone when
M1's gate passes.

---

## Parallelization map (what can run at the same time)

```
NOW, in parallel (no cross-lane blocker):
  GP-06  Services/Tools .......... fully workable in Rust
  GP-07  Evidence/QA/Perf ........ Rust + Editor smoke workable
  GP-03  Core Match Experience ... Editor/Game smoke + design (human read pending)
  GP-08  Presentation/Rights ..... POC map exists; screenshots/provenance workable
  GP-05  Voice & Trust ........... provider research/decision workable
  GP-09  Comprehension/A11y ...... docs/strings/checklist workable
  GP-10  Release/Community ....... readiness docs workable
  GP-01  Human Playability ....... PREP only (dry-run, readiness) workable
  GP-04  Steam Online ............ DESIGN + config-safety + contracts workable

GATED (need a server-capable / source-built UE 5.7 so FrostwakeServer.exe can build):
  GP-02  Network/Hosting ......... IMPLEMENTATION blocked → keep blocker evidence + unblock runbook
  GP-04  Steam Online ............ IMPLEMENTATION (lobby spike) blocked behind GP-02
  GP-01  Human Playability ....... LIVE RUN also needs real humans + Windows smoke readiness
```

Dependency edges:
- `GP-02 (server boots)` → unlocks `GP-04 impl` → contributes to `GP-10 official-AWS hosting ops` (community hosting dropped — `docs/business-model.md` D8).
- `GP-01 human notes` → feed `GP-03` (readability), `GP-09` (comprehension), `GP-08` (art timing).
- `GP-07 evidence` underpins every other lane's claims and `GP-10` wave discipline.

---

## Loop priority order (used by DISPATCH §2)

> **OWNER OVERRIDE (2026-05-29): make it look good + play like a game.** Top priority is the
> *player-facing build*: dress the playable scene with beautiful **IP-safe free/CC0 assets**
> (GP-08) and make the solo/match loop *feel like a real game* (GP-03 HUD/feel), plus menu/UI
> polish incl. a JP-capable font (GP-09). Order after health: **GP-08 → GP-03 → GP-09 → GP-06 →
> GP-07 → GP-04/GP-02 upkeep → GP-01/GP-05/GP-10**. Aesthetics are owner-judged via screenshots;
> assets must be provenanced — no unattended downloads, no game rips (see DISPATCH visual rules).

When choosing the single next step, prefer the **highest** eligible (non-blocked) lane:

1. **GP-07 health** — if `quality-gate` is red, fix it (gate before features).
2. **GP-08** — capture Visual POC screenshots + review (this is the literal cycle-79 "next cycle", unblocked).
3. **GP-06** — close the largest backend/OpenAPI/test mismatch.
4. **GP-03** — improve sabotage-loss explainability (smaller, gated by human notes for tuning).
5. **GP-07** — define a performance budget / add a missing telemetry field.
6. **GP-05** — converge on the voice provider decision.
7. **GP-09** — first-match comprehension checklist + top wording fix.
8. **GP-10** — Steam Playtest readiness snapshot.
9. **GP-04** — keep lobby metadata/join-decision + config safety green; document the spike.
10. **GP-01** — keep the human-test dry-run path + readiness green.
11. **GP-02 / GP-04 (impl)** — refresh blocker evidence + sharpen the "server-capable UE" unblock runbook.

Anti-starvation: among eligible lanes, the one with the oldest `Updated:` wins ties.

---

## Definition of "phase advance" (do not let the loop forget the real bar)

Per `docs/production-plan.md` and `docs/iteration-loop.md`, Phase 1 → Phase 2 needs evidence
in **three** categories at once — Gameplay (humans want another match), Technical (8p
matches finish, logs reproduce failures), Production (rights/assets/Steam/moderation not
ignored). Automated passes alone never advance a phase. The loop's job is to keep every
lane's evidence honest and current so the human-gated milestones (GP-01, GP-02) can fire the
moment their external blockers (people, server-capable UE) are removed.
