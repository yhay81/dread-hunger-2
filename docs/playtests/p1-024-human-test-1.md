# P1-024 Human Playtest Packet 1

Date prepared: 2026-05-25

## Purpose

Run the first 8-player human greybox test after the Phase 1 automation milestone.

This test is not for art, balance polish, store readiness, or production networking. It is for deciding whether players can understand the rough loop, produce suspicion and cooperation, and create a useful keep/cut/change list.

## Current Technical Baseline

- Build path: Windows `Win64` Unreal Editor/Game listen-server validation.
- Dedicated Server: retest on Windows with `--platform Win64 --include-server`; if Launcher UE blocks it, use a UE source build.
- Best automation evidence: `Saved/SmokeSuites/suite-20260525-050522/suite_summary.md`.
- Pre-test gate: `cargo run -p frostwake-tools -- quality-gate --require-ue` must pass.
- Optional pre-test gate: `cargo run -p frostwake-tools -- run-smoke-suite --include-heavy --skip-build --null-rhi --platform Win64` should pass if there was any code or map change after Cycle 26.

## Test Size

Valid run: exactly 8 players.

If fewer than 8 players are available, do not count it as P1-024. Use it only as a technical rehearsal or a one-player practice check.

## Run Header

Fill this before launch:

```text
RunId: P1-024-run-__
Build: Frostwake Win64 Development, UE 5.7
Commit or local snapshot:
Host machine:
Host LAN IP:
Port: 7777
Map: /Game/Maps/L_IcebreakerWhitebox
Target players: 8
Voice method:
Recording consent: yes / no
Observer A, timing and logs:
Observer B, comprehension and social dynamics:
```

## Roles

The game assigns roles automatically.

Expected role counts:

| Players | Crew | Agents |
| ---: | ---: | ---: |
| 8 | 6 | 2 |

Player-facing terms:

- Crew: keep the ship moving, repair systems, recover from sabotage, and identify suspicious behavior.
- Agent: quietly prevent the ship from succeeding through sabotage, misdirection, isolation, and timing.

Do not use Dread Hunger names, roles, factions, or framing during the test.

## Host Preflight

Run from the repository root:

```powershell
cargo run -p frostwake-tools -- quality-gate --require-ue
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
cargo run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi --platform Win64
```

Expected result:

- `quality-gate` passes.
- `unreal-gate` reports Editor/Game pass and records the Windows Server target result.
- `ready8` smoke passes with `8p/2s` role assignment.
- Startup evidence appears in host logs: listen port, whitebox map load, `role_assignment_complete`, no fatal/crash log terms.

If the `ready8` smoke fails, do not run the human test. Fix the startup or role-assignment failure first.

## Log Directory

Use one local directory per run:

```powershell
cargo run -p frostwake-tools -- playtest-run-scaffold --run-number 1 --target-players 8
```

This creates `Saved/Playtests/P1-024/run-01/` with `host.ps1`, `client-local.ps1`, `client-lan.ps1`, `preflight.ps1`, `after-test.ps1`, POSIX helper scripts, notes, and local evidence folders. Raw logs and recordings stay out of git. After the test, summarize the run in a committed markdown note and keep raw evidence under `Saved/Playtests/...`.

## Listen Host Launch

Replace `run-01` with the current run id. Set `UE_ROOT` first if Unreal is not installed at the default Epic path.

```powershell
$env:UE_ROOT = "C:\Program Files\Epic Games\UE_5.7"
.\Saved\Playtests\P1-024\run-01\host.ps1
```

Use `-FrostwakeLobbyMinPlayers=8` for the target 8-player run if all players are ready before launch.

## Client Launch

Same machine client:

```powershell
.\Saved\Playtests\P1-024\run-01\client-local.ps1
```

LAN client:

```powershell
.\Saved\Playtests\P1-024\run-01\client-lan.ps1 -HostLanIp <host-lan-ip>
```

If multiple clients run on the same Windows host, give each client a unique port, such as `-port=7778`, `-port=7779`, and `-port=7780`.

## Communication Rule

Phase 1 may use an external voice call only to approximate conversation volume, suspicion, and coordination. Do not score proximity-voice quality from this test.

Recording rule:

- Ask for explicit consent before recording screen, voice, or the shared call.
- If anyone declines voice recording, record only screen/video without call audio or skip recording.
- Delete raw recordings after review, target 30 days.
- Do not commit raw recordings or transcripts.

Observer rule:

- Do not coach players mid-round unless the match is blocked.
- Do not reveal roles.
- Do not explain hidden rules after the round starts.
- If two observers are available, one tracks timing/logs and one tracks comprehension/social dynamics.

## Player Brief

Read this once before the round:

```text
This is a greybox prototype. Visuals and UI are temporary.

Crew: keep the ship alive and progress the route. Repair systems, share information, and watch for suspicious behavior.

Agent: prevent the ship from succeeding. Use sabotage, timing, isolation, and false explanations. Do not simply hide forever; create situations other players can discuss.

Everyone: speak naturally, call out discoveries, and say what you think happened. If something is confusing, say it out loud rather than silently waiting.
```

Do not give a long tutorial. If players cannot infer the loop after a short brief and one round, that is evidence.

## Observer Notes

Record timestamps for:

- First moment players understand the goal.
- First repair or sabotage interaction.
- First accusation.
- First player who gets lost or stuck.
- First down, containment, or rescue.
- Any moment where all players stop knowing what to do.
- Any moment that causes laughter, tension, argument, or a request for another round.
- Any crash, disconnect, hard stall, or role assignment failure.

Track these counts:

| Metric | Value |
| --- | --- |
| Players connected |  |
| Match started |  |
| Crew players |  |
| Agent players |  |
| Match ended |  |
| Winner |  |
| Disconnects |  |
| Crashes |  |
| Critical blockers |  |
| Players asking for another round |  |

Track GP-09 comprehension and accessibility signals:

| Signal | Observation |
| --- | --- |
| Objective comprehension |  |
| Next-step clarity |  |
| Failure-state clarity |  |
| UI or control confusion |  |
| Accessibility blocker |  |
| Text or term issue |  |

## Post-Match Questions

Ask every player:

- What did you think your goal was?
- When did you first suspect another player?
- What action made you trust someone?
- What action made you distrust someone?
- What was the most confusing moment?
- Did any control, readability, audio, or screen effect issue block you?
- What was the most tense or funny moment?
- Did you want another round?
- What one thing should be cut?
- What one thing should be kept?
- What one change would most improve the next test?

Ask agents privately:

- Did you know what sabotage options existed?
- Could you create suspicion without being instantly obvious?
- Were you bored while waiting for an opportunity?

Ask crew privately:

- Did repair work feel like teamwork or busywork?
- Did you have enough information to accuse someone?
- Did ship state feel urgent enough?

## Pass / Fail

Count P1-024 as a pass only if:

- 8 players connect.
- A match starts and role assignment is visible in `events.jsonl`.
- Players can perform at least one repair or sabotage-relevant interaction.
- The run produces a keep/cut/change list.
- No crash or synchronization bug prevents basic participation.

Count it as a partial if:

- Players connect and roles assign, but the match stalls before useful social data.
- A technical issue blocks completion but the cause is captured in logs.

Count it as a fail if:

- The host cannot start.
- Clients cannot connect.
- Roles do not assign.
- Most players cannot control or interact.
- Logs are missing.

## After-Test Commands

Summarize telemetry:

```bash
cargo run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/run-01/events.jsonl
cargo run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/run-01/events.jsonl --out Saved/Playtests/P1-024/run-01/summary.json
cargo run -p frostwake-tools -- playtest-summary Saved/Playtests/P1-024/run-01/summary.json --out docs/playtests/p1-024-run-01-summary.md
cargo run -p frostwake-tools -- playtest-preflight Saved/Playtests/P1-024/run-01/summary.json --markdown docs/playtests/p1-024-run-01-summary.md
```

The summary should include `run_id`, `build_id`, `map_id`, `profile`, `players_connected`, `players_disconnected`, `duration_seconds`, and `match_duration_seconds` when the match ends.

Preserve raw evidence locally:

```text
Saved/Playtests/P1-024/run-01/events.jsonl
Saved/Playtests/P1-024/run-01/host-notes.md
Saved/Playtests/P1-024/run-01/recordings/
```

Commit only an anonymized summary. Do not commit raw logs, recordings, voice transcripts, tester names, SteamIDs, IP addresses, or moderation details.

`cargo run -p frostwake-tools -- playtest-preflight` must pass before a human P1-024 summary is committed. It checks the `log-summary --out` JSON for the expected run/profile/map/player-count gates and checks the Markdown summary for unresolved placeholders, dry-run markers, and common identity or local-path leaks.
It also requires resolved human playability evidence for objective comprehension, confusion, a memorable social incident, repeat-play intent, and the keep/cut/change decision rows.

## Summary Template

```text
MatchId:
Date:
Build:
Players:
Map:
Run directory:
Duration:
Winner:
Interrupted:

Technical result:
- Host started:
- Clients connected:
- Role assignment:
- Crashes:
- Disconnects:
- Log summary:

Observed flow:
- First understood goal:
- First repair/sabotage:
- First accusation:
- Biggest confusion:
- Best moment:

Player answers:
- Wanted another round:
- Keep:
- Cut:
- Change:

Decision:
- Count as P1-024 pass/partial/fail:
- Top 5 blockers before next test:
```

## Next Decision

If P1-024 passes, move to P1-025 and fix the top five blockers before the second human test.

If P1-024 is partial or failed for technical reasons, run a smaller technical rehearsal before asking players for another full test.
