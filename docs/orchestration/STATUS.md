# Frostwake Lane Status — Global Snapshot

Updated: 2026-05-29 · Phase 1 (local-automation milestone reached) · 10 parallel lanes

> **🎯 CURRENT FOCUS (owner override 2026-05-29): make the player-facing build look good + play like a game.**
> Lead with **GP-08** (dress the playable scene with IP-safe free/CC0 assets + atmosphere) and
> **GP-03** (solo HUD / match feel), then **GP-09** (menu/UI polish incl. JP font). Aesthetics are
> owner-judged via screenshots; assets must be provenanced — no unattended downloads, no game rips.
> Working today: boot → menu → 一人モード → lit whitebox solo.

This is the fast picture the routine reads first. Drill into `lanes/GP-0X.state.md` for the
full **Next Action** (exact command, verify gate, allowed paths). Pick the lane per the
priority order below + `DISPATCH.md` §2.

| Lane | Status | M1 milestone (short) | Next Action (one-liner) | Top blocker | Updated |
| --- | --- | --- | --- | --- | --- |
| **GP-01** Human Playability | 🟡 YELLOW | First real 6-8p human run + anonymized summary (`playtest-preflight --mode human`) | Re-confirm Windows listen-server preflight is green (`playtest-run-scaffold`/`preflight`) | **8 real humans** (no code change removes it) | 2026-05-29 |
| **GP-02** Network/Hosting | 🔴 BLOCKED | `AbyssLockServer.exe` builds+boots+8 clients+ready-lobby | Keep runbook + `UE_ROOT` instructions consistent; verify `quality-gate` | **Server-capable UE 5.7** (Launcher UE can't build Server targets) | 2026-05-29 |
| **GP-03** Core Match | 🟡 YELLOW | DH-parity feel + readable round (`docs/mechanics-parity-target.md`, `docs/control-scheme.md`) | **Add minimal solo HUD** (role/objective/interaction prompt/result) so it plays like a game; then end-state readability audit | Human P1-024/025 notes for tuning | 2026-05-29 |
| **GP-04** Steam Online | 🟡 YELLOW | Lobby create/find/join + build/map-mismatch reject (P2-003/004) | Run `run_steam_lobby_validation.ps1` → `preflight_pass` on 5 steps | Runtime spike gated behind GP-02 (contracts already green) | 2026-05-29 |
| **GP-05** Voice & Trust | 🟡 YELLOW | One voice provider chosen + 8p acceptance plan | Write `docs/voice-provider-decision.md` (VCI+EOS vs Vivox vs Steam Voice) | Runtime acceptance gated by server (decision itself unblocked) | 2026-05-29 |
| **GP-06** Services & Tools | 🟢 GREEN | Backend ↔ `openapi.yaml` ↔ tests parity; `cargo test --workspace` green | ✅ 404s documented + tested (cycle 83) → add the 409 `lobby_full` test | none | 2026-05-29 |
| **GP-07** Evidence/QA/Perf | 🟡 YELLOW | Perf budgets + measurement method; gates reproducible | Draft `docs/performance-budget.md`; verify `quality-gate` | Server-side perf rows need server build (doc still writable) | 2026-05-29 |
| **GP-08** Presentation/Rights | 🟢 GREEN | **TOP**: dress the *playable* scene with IP-safe CC0 assets + atmosphere (looks like a real game) | Build CC0 material instances + apply to playable map + fog/post-process; screenshot for owner review | New meshes/props need owner to acquire CC0 packs | 2026-05-29 |
| **GP-09** Comprehension/A11y | 🟡 YELLOW | First-match comprehension checklist + stable strings + glossary · dedicated **L_MainMenu** boot + start→lobby→**solo** debug (cycle 85) | Write `docs/gp09-comprehension-checklist.md`; verify `quality-gate` | No human comprehension data yet | 2026-05-29 |
| **GP-10** Release/Community | 🟡 YELLOW | Steam Playtest readiness snapshot (owner/artifact/status/blocker/cancel) | Create `docs/steam-playtest-readiness.md`; verify `quality-gate` | No moderation-triage owner assigned anywhere | 2026-05-29 |

Legend: 🟢 GREEN = healthy & advancing · 🟡 YELLOW = workable, signal needs work · 🔴 BLOCKED = needs an external unblock.

## Loop priority order (mirror of PLAN.md §"Loop priority order")

1. **Health** — if `quality-gate` is red, fix it first (stop after green).
2. GP-08 visual POC review · 3. GP-06 backend parity · 4. GP-03 readability audit ·
5. GP-07 perf budget · 6. GP-05 voice decision · 7. GP-09 comprehension checklist ·
8. GP-10 readiness snapshot · 9. GP-04 lobby contracts/spike-doc · 10. GP-01 human-run prep ·
11. GP-02 / GP-04 impl — refresh blocker evidence + sharpen the server-capable-UE unblock runbook.

Tie-break among eligible lanes: oldest `Updated:` wins (anti-starvation).

## Two external blockers gate the human-proof finish line

1. **Server-capable UE 5.7** — only a Launcher UE is installed (no Server-target build), so
   `AbyssLockServer.exe` cannot exist. Hard-blocks GP-02 impl, GP-04 runtime spike. Until then
   those lanes only keep blocker evidence + unblock runbooks current.
2. **8 real humans** — GP-01's live run (and the human readability/comprehension signals that
   feed GP-03 / GP-09 / GP-08 art timing) need a scheduled session. All prep is ready to fire.

Everything else advances now, headless, in parallel.

## Last loop iteration

- 2026-05-29 — **Loop RESUMED (owner refocus).** Top priority = player-facing build: GP-08 IP-safe
  CC0 visual dressing of the playable scene + GP-03 solo HUD/feel + GP-09 menu/UI polish (JP font).
  Front-end works (menu → 一人モード → lit whitebox). Aesthetics owner-judged via screenshots;
  assets provenanced only (no unattended downloads, no game rips).
- 2026-05-29 — front-end fixes (interactive): black-menu WidgetTree fix; whitebox Movable lights
  (no more "LIGHTING NEEDS TO BE REBUILT"); dedicated `L_MainMenu` boot map (cycle 85).
- 2026-05-29 **cycle 85** (GP-09, interactive) — dedicated front-end map: boot now loads
  `L_MainMenu` (own `AbyssMenuGameMode`, no whitebox); **一人モード** travels to `whitebox?solo`
  and the GameMode auto-starts the 1-player practice match. Verified headlessly (boot→L_MainMenu;
  solo→`match_started source=single_player`). Editor build + quality-gate green.
- 2026-05-29 **cycle 84** (GP-09, interactive) — front-end: added a full-screen dark backdrop to
  the menu so boot shows a clean start screen (Frostwake + ゲーム開始) instead of the whitebox;
  confirmed Start → Lobby-choice → **一人モード** → controllable solo debug match. Editor/Game
  build + quality-gate green. Autonomous cron paused during this focused work.
- 2026-05-29 **cycle 83** (GP-06) — documented backend not-found responses (404 join/leave/banlist,
  409 join) in `openapi.yaml` + 3 new 404 integration tests (backend 19→22). quality-gate green.
  Next GP-06: the 409 `lobby_full` test.
- 2026-05-29 **cycle 82** (GP-06) — closed the `/healthz` `startedAt` OpenAPI parity gap (schema
  + test; 48 backend tests pass under quality-gate). Next GP-06: document 404/409 error responses.
- 2026-05-29 **cycle 81** (baseline) — committed the pending working-tree baseline; quality-gate
  green; Editor/Game UE build passed (gameplay C++ compiles). Loop switched to a self-contained
  prompt (the `/frostwake-loop` slash command was unknown to the running session).
- 2026-05-29 **cycle 80** (GP-08) — wrote `docs/visual-poc-screenshot-review.md` (capture +
  keep/replace/reject rubric for the three POC zones). GP-08's remaining step is Editor-gated;
  next headless step rotates to **GP-06** (add `startedAt` to the backend `HealthResponse`
  schema + test). See `docs/cycles/2026-05-29-cycle-80.md`.
- 2026-05-29 — Initial setup. Orchestration system created; all 10 lane state files authored by
  parallel lane agents.
