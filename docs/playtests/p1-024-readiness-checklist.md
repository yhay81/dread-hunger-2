# P1-024 Readiness Checklist — First 6-8 Player Human Test

Date prepared: 2026-05-29
Source charter: `docs/subprojects/sp-01-phase1-human-proof.md`
Run packet: `docs/playtests/p1-024-human-test-1.md`

This checklist must pass in full before the first live human session fires.
Every item is grounded in existing repo evidence.
"Verify" commands are strings to run, not marks of a completed run.

---

## Section 1 — Build Identity

Each item must have a concrete value filled in before launch.

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 1.1 | **Build ID** recorded: commit hash or `cycle-NN-local` tag, noted in the run header. | Read the run header block in `docs/playtests/p1-024-human-test-1.md`; confirm `Local snapshot or commit:` is filled. | [ ] |
| 1.2 | **Build target**: `Frostwake Win64 Development`. Launcher UE_5.7 supports Editor/Game only; no Dedicated Server (blocked separately under GP-02). | Run `cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64` and confirm `Editor/Game pass` appears. | [ ] |
| 1.3 | **Map ID**: `/Game/Maps/L_IcebreakerWhitebox`. The run host script must set this map. | Check `Saved\Playtests\P1-024\run-01\host.ps1` for the map argument, or confirm it in the generated scaffold. | [ ] |
| 1.4 | **Player count target**: exactly 8. Fewer than 8 does not count as P1-024 per `docs/playtests/p1-024-human-test-1.md`. | Confirm the `host.ps1` `-FrostwakeLobbyMinPlayers=8` flag is present. | [ ] |
| 1.5 | **Port**: 7777 (host default). All clients connect to the host LAN IP on this port. | Read `Saved\Playtests\P1-024\run-01\host.ps1` and confirm listen port. | [ ] |

---

## Section 2 — Windows Listen-Server Smoke Readiness

Evidence baseline: Cycle 76 (`docs/cycles/2026-05-26-cycle-76.md`) — `ready8` smoke passed with `players_connected=8`, `last_role_assignment.players=8`, `last_role_assignment.saboteurs=2`, `match_started=1`, `errors=0`, `invalid_lines=0`.

Reconfirm this baseline on the day of the run if any source file, map, or config changed after 2026-05-26.

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 2.1 | **Quality gate green**. Rust, schema, PowerShell, asset ledger, and Unreal metadata surfaces are all valid. | `cargo run -p frostwake-tools -- quality-gate --require-ue` — must print no failing rows. | [ ] |
| 2.2 | **`ready8` smoke passes**. Listen-server starts, 8 bots connect, role assignment fires with 6 crew + 2 agents, no errors. | `cargo run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi --platform Win64` — confirm `players_connected=8`, `last_role_assignment.saboteurs=2`, `errors=0`. | [ ] |
| 2.3 | **Smoke logs are intact**. The `events.jsonl` from the pre-run smoke can be decoded and contains the expected events. | `cargo run -p frostwake-tools -- log-summary Saved\SmokeTests\<latest>\events.jsonl` — confirm the five key fields above. | [ ] |
| 2.4 | **No fatal or crash log terms**. Neither the quality gate nor the smoke run reports `fatal`, `crash`, `assert`, or `error` in the active build. | Check `cargo run -p frostwake-tools -- quality-gate --require-ue` output and the `errors` field from the smoke `log-summary`. | [ ] |
| 2.5 | **Preflight script passes end-to-end**. The generated scaffold preflight completes without aborting. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; .\Saved\Playtests\P1-024\run-01\preflight.ps1` — each step must emit `[RESULT] pass`. | [ ] |

If item 2.1 or 2.2 fails, do not run the human test. Diagnose and fix before rescheduling.

---

## Section 3 — Run Scaffold Readiness

The scaffold is generated locally (gitignored). Regenerate if any `playtest-run-scaffold` tool code changed since Cycle 76.

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 3.1 | **Scaffold directory exists**: `Saved\Playtests\P1-024\run-01\` contains `host.ps1`, `client-local.ps1`, `client-lan.ps1`, `preflight.ps1`, `after-test.ps1`. | `ls Saved\Playtests\P1-024\run-01\` — confirm all five scripts are present. | [ ] |
| 3.2 | **Scaffold is current**. If any frostwake-tools source changed after the last scaffold generation, regenerate. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- playtest-run-scaffold --run-number 1 --target-players 8 --force` | [ ] |
| 3.3 | **`Invoke-Cargo` wrapper present** inside generated helpers. This prevents silent failures from Cargo command exits. | Read `Saved\Playtests\P1-024\run-01\preflight.ps1` and confirm `Invoke-Cargo` is used for every cargo call (established in Cycle 76). | [ ] |
| 3.4 | **Isolated Cargo target dir**. Generated helpers default to `target\playtest-tools` to avoid locking the shared `target\debug` binary. | Read `Saved\Playtests\P1-024\run-01\host.ps1`; confirm `CARGO_TARGET_DIR = 'target\playtest-tools'` is set. | [ ] |

---

## Section 4 — Packet and Observer Doc Readiness

All committed playtest documents are confirmed present in `docs/quality-gates.json` required-files list.

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 4.1 | **Run packet present and current**: `docs/playtests/p1-024-human-test-1.md` — player brief, roles, pass/fail criteria, after-test commands. | `ls docs\playtests\p1-024-human-test-1.md` — confirm file exists and is readable. | [ ] |
| 4.2 | **Observer sheet present**: `docs/playtests/p1-024-observer-sheet.md` — timeline table, comprehension checks, GP-09 signals, social dynamics, blockers, keep/cut/change. | `ls docs\playtests\p1-024-observer-sheet.md` — confirm file exists. | [ ] |
| 4.3 | **Post-round survey present**: `docs/playtests/p1-024-post-round-survey.md` — all-player, crew-only, and agent-only question sets plus observer summary table. | `ls docs\playtests\p1-024-post-round-survey.md` — confirm file exists. | [ ] |
| 4.4 | **Summary template present**: `docs/playtests/p1-024-summary-template.md` — blank template ready to fill after the run. | `ls docs\playtests\p1-024-summary-template.md` — confirm file exists. | [ ] |
| 4.5 | **Two observers assigned** (or one if only one is available with coverage): Observer A tracks timing + logs; Observer B tracks comprehension + social dynamics. Roles defined in the run packet. | Confirm named coverage in the run header before launch. No tool gate — requires a human assignment decision. | [ ] |
| 4.6 | **Voice and recording consent rule briefed** before the session starts. Recording requires explicit consent; anyone declining means call audio is not recorded. | Confirm the host has read the `Communication Rule` and `Recording consent` sections of `docs/playtests/p1-024-human-test-1.md`. | [ ] |

---

## Section 5 — Playtest-Preflight Dry-Run

Run the preflight tooling against the existing dry-run example to confirm the tool chain is working before the live run.

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 5.1 | **`playtest-preflight` tool available** and compiles cleanly. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- playtest-preflight --help` — must print usage without error. | [ ] |
| 5.2 | **Dry-run example correctly rejects `--mode human`**. The tool must refuse the dry-run file as a valid human summary. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- playtest-preflight Saved\Playtests\P1-024\run-01\summary.json --markdown docs\playtests\p1-024-dry-run-summary-example.md` — expect rejection for dry-run markers or unresolved human-signal rows. | [ ] |
| 5.3 | **`playtest-summary` tool available** and produces a valid skeleton from a `summary.json`. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- playtest-summary --help` — must print usage without error. | [ ] |
| 5.4 | **`log-summary` tool available** and can summarize a JSONL events file. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- log-summary --help` — must print usage without error. | [ ] |
| 5.5 | **After-test script is present and readable**. It will generate the `summary.json` and Markdown skeleton immediately after the run. | `ls Saved\Playtests\P1-024\run-01\after-test.ps1` — confirm file exists. | [ ] |

---

## Section 6 — Human Signal Commitments

These items cannot be gated by tools before the run. They are commitments the host and observers carry into the session.

| # | Item | How to verify (post-run, before committing summary) | Status |
| --: | --- | --- | --- |
| 6.1 | **Objective comprehension evidence** filled in the summary: what players thought their goal was (crew and agent separately). | `playtest-preflight --mode human` checks the `Player Feedback` table row and the `Comprehension And Accessibility` `Objective comprehension` row in `docs/playtests/p1-024-run-01-summary.md`. | [ ] |
| 6.2 | **Confusion evidence** filled: which UI, control, or rule caused the most avoidable stops. | `playtest-preflight --mode human` checks the confusion row in the Player Feedback table. | [ ] |
| 6.3 | **Memorable social incident** filled: at least one suspicion, rescue, containment, or accusation moment that players could retell. | `playtest-preflight --mode human` checks the `memorable / social moment` row. | [ ] |
| 6.4 | **Repeat-play intent** filled: how many of 8 players wanted another round. | `playtest-preflight --mode human` checks the repeat-play row. | [ ] |
| 6.5 | **Keep/cut/change list** filled: at least one entry in each of the three decision columns. | `playtest-preflight --mode human` rejects summaries with empty keep/cut/change rows. | [ ] |
| 6.6 | **GP-09 comprehension and accessibility rows** filled in the `Comprehension And Accessibility` table (six signals: objective comprehension, next-step clarity, failure-state clarity, UI/control confusion, accessibility blocker, text/term issue). | `playtest-preflight --mode human` checks all six GP-09 signal rows. | [ ] |

---

## Section 7 — Data Handling

| # | Item | How to verify | Status |
| --: | --- | --- | --- |
| 7.1 | Raw `events.jsonl` and `host-notes.md` stay under `Saved\Playtests\P1-024\run-01\` (gitignored). | Confirm path is under `Saved/` before any commit. | [ ] |
| 7.2 | No tester names, SteamIDs, IP addresses, voice transcripts, or crash dump paths that expose usernames are in the committed summary. | Read the completed `docs/playtests/p1-024-run-01-summary.md` and check the `Data Handling Check` section before committing. | [ ] |
| 7.3 | `playtest-preflight --mode human` passes on the summary Markdown + JSON before the summary is committed. | `$env:CARGO_TARGET_DIR = 'target\playtest-tools'; cargo run -p frostwake-tools -- playtest-preflight Saved\Playtests\P1-024\run-01\summary.json --markdown docs\playtests\p1-024-run-01-summary.md --mode human` | [ ] |
| 7.4 | Reviewer (QA + Game Design) has approved the committed summary before the GP-01 M1 gate is closed. | Confirm reviewer sign-off per `docs/agent-operating-model.md` (QA owns gate pass/fail records). | [ ] |

---

## Pass / No-Go Decision

All items in Sections 1–5 must be checked before the session starts.
Items in Section 6 are commitments carried into the run (checked after the run, before committing the summary).
Items in Section 7 are verified after the run, before committing.

| When | Condition | Decision |
| --- | --- | --- |
| Before launch | Sections 1–5 all checked | **Go** |
| Before launch | Any item in Section 2 fails | **No-go** — fix smoke/preflight first |
| Before launch | Any Section 1 item blank | **No-go** — fill run header first |
| After run | Sections 6–7 all checked, `playtest-preflight --mode human` passes | Commit the anonymized summary; advance GP-01-M1 |
| After run | Any Section 6 row unresolved | Do not commit as a P1-024 pass; file as partial with identified gap |

Dedicated Server (`FrostwakeServer.exe`) is NOT a listen-server run blocker.
That blocker is tracked separately under GP-02 and does not affect this checklist.

---

## Related Artifacts

- `docs/playtests/p1-024-human-test-1.md` — full run packet
- `docs/playtests/p1-024-observer-sheet.md` — observer timeline and comprehension/social tables
- `docs/playtests/p1-024-post-round-survey.md` — post-round survey questions
- `docs/playtests/p1-024-summary-template.md` — anonymized summary to fill after the run
- `docs/playtests/p1-024-dry-run-summary-example.md` — format/redaction reference (not human evidence)
- `docs/cycles/2026-05-26-cycle-76.md` — Cycle 76 preflight baseline evidence
- `docs/phase1-milestone-report.md` — Phase 1 gate table; P1-024 listed as "not run"
- `docs/subprojects/sp-01-phase1-human-proof.md` — GP-01 charter
- `docs/agent-operating-model.md` — reviewer role definitions
