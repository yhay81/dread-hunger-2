# P1-024 Dry-Run Summary Example (Non-Human Evidence)

> Dry-run example only. This file was generated from smoke-test telemetry, not a human playtest. Do not count it as P1-024 evidence, social validation, balance evidence, or a pass/partial/fail result.


This dry-run demonstrates the summary format with smoke telemetry. Commit a completed human-test summary only after removing tester names, raw voice details, IP addresses, SteamIDs, local machine secrets, crash dump paths that expose usernames, and raw transcript text.

Raw evidence stays under `Saved/Playtests/...` and is not committed.

## Header

```text
RunId: cycle29-fatal-sabotage
Date: 2026-05-25
Local snapshot or commit:
Build: cycle29-local
Map: /Game/Maps/L_IcebreakerWhitebox
Profile: custom
Host type: listen-server
Target players: 6
Actual players: 1
Voice method:
Recording consent: yes / no / partial
Raw evidence path: Saved/SmokeTests/local-20260525-063912
Summary author: Codex
```

## Telemetry Summary

Generated from:

```bash
python3 Tools/playtest_summary.py Saved/SmokeTests/local-20260525-063912/summary.json --out docs/playtests/p1-024-dry-run-summary-example.md --dry-run-example
```

| Field | Value |
| --- | --- |
| `run_id` | cycle29-fatal-sabotage |
| `build_id` | cycle29-local |
| `map_id` | /Game/Maps/L_IcebreakerWhitebox |
| `profile` | custom |
| `duration_seconds` | 6.534 |
| `match_duration_seconds` | 0.206 |
| `players_connected` | 1 |
| `players_disconnected` | 1 |
| `match_started` | 1 |
| `match_ended` | 1 |
| `last_role_assignment.players` | 1 |
| `last_role_assignment.saboteurs` | 1 |
| `last_match_result.winner` | saboteur |
| `last_match_result.reason` | fatal_ship_state |
| `ship_task_repairs` | 0 |
| `ship_task_sabotages` | 3 |
| `players_damaged` | 0 |
| `players_downed` | 0 |
| `players_rescued` | 0 |
| `players_contained` | 0 |
| `players_released` | 0 |
| `deaths` | 0 |
| `errors` | 0 |
| `invalid_lines` | 0 |
| `total_events` | 10 |

Compact anonymized excerpt:

```json
{
  "duration_seconds": 6.534,
  "last_match_result": {
    "reason": "fatal_ship_state",
    "winner": "saboteur"
  },
  "last_role_assignment": {
    "players": 1,
    "saboteurs": 1
  },
  "match_duration_seconds": 0.206,
  "match_ended": 1,
  "match_started": 1,
  "players_connected": 1,
  "players_disconnected": 1,
  "run_id": "cycle29-fatal-sabotage"
}
```

## Technical Result

| Gate | Result | Evidence |
| --- | --- | --- |
| Host started | pass | total_events=10 |
| Clients connected | partial | players_connected=1 |
| Role assignment visible in log | pass | last_role_assignment=`{"players": 1, "saboteurs": 1}` |
| Players could move | observer required | Fill from observer notes |
| Players could interact | pass | interaction_count=3; observer confirm |
| Repair or sabotage happened | pass | repairs=0, sabotages=3 |
| Match ended or meaningful interruption captured | pass | match_started=1, match_ended=1 |
| Logs reconstruct the run | pass | invalid_lines=0 |
| No critical crash | pass | errors=0; observer confirm |

Technical notes:

-

## Validity Gate

Resolve each item before choosing pass, partial, or fail.

| Check | Result | Evidence |
| --- | --- | --- |
| `players_connected >= 6` | no | players_connected=1 |
| `match_started > 0` | yes | match_started=1 |
| `last_role_assignment` exists | yes | players=1, saboteurs=1 |
| Crew/agent counts match player count | no | expected_saboteurs=0 |
| At least one repair or sabotage-relevant interaction occurred | yes | ship_task_repairs=0, ship_task_sabotages=3 |
| Any interaction evidence exists | yes | interaction_count=3 |
| No crash or desync blocked basic participation | yes | errors=0, invalid_lines=0; observer confirm |
| Logs are present and usable | yes | total_events=10 |
| Keep/cut/change list was produced | observer required | Fill from player feedback |

Validity result:

```text
not evaluated - smoke-test dry run
```

## Observed Flow

| Moment | Timestamp or elapsed time | What happened | Decision impact |
| --- | --- | --- | --- |
| First goal comprehension |  |  |  |
| First repair or sabotage |  |  |  |
| First accusation |  |  |  |
| First down/rescue/containment |  |  |  |
| Biggest confusion |  |  |  |
| Best social moment |  |  |  |
| Worst stall |  |  |  |
| End or interruption |  |  |  |

Short narrative:

-

## Player Feedback

Aggregate only. Do not name players.

| Question | Summary |
| --- | --- |
| What did players think their goal was? |  |
| When did suspicion first appear? |  |
| What created trust? |  |
| What created distrust? |  |
| What was confusing? |  |
| What was tense, funny, or memorable? |  |
| How many wanted another round? |  |
| What should be kept? |  |
| What should be cut? |  |
| What should change before the next test? |  |

Agent-only feedback:

-

Crew-only feedback:

-

## Keep / Cut / Change

Keep:

-

Cut:

-

Change before P1-026:

-

## Mechanics Decision Table

| Area | Telemetry evidence | Observer/player evidence | Decision | Reason | Next action |
| --- | --- | --- | --- | --- | --- |
| Repairs and route tasks | repairs=0, route_final=0 |  | keep / cut / change |  |  |
| Sabotage actions | sabotages=3, winner=saboteur |  | keep / cut / change |  |  |
| Doors and bulkheads | toggles=0, locks=0, releases=0 |  | keep / cut / change |  |  |
| Flooding and pump pressure | pressure_events=0, last_pressure= |  | keep / cut / change |  |  |
| PvE, down, rescue, containment | damaged=0, downed=0, rescued=0, contained=0, released=0 |  | keep / cut / change |  |  |
| Items and pickup/drop | pickups=0, drops=0 |  | keep / cut / change |  |  |
| Role clarity | players=1, crew=0, agents=1 |  | keep / cut / change |  |  |
| Pacing | duration=6.534, match_duration=0.206 |  | keep / cut / change |  |  |
| Social suspicion | telemetry cannot judge |  | keep / cut / change |  |  |

## Top Blockers

List at most five. These become P1-025 candidates.

| Rank | Blocker | Severity | Evidence | Proposed fix | Owner | Retest gate |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 |  | critical / high / medium / low |  |  |  |  |
| 2 |  | critical / high / medium / low |  |  |  |  |
| 3 |  | critical / high / medium / low |  |  |  |  |
| 4 |  | critical / high / medium / low |  |  |  |  |
| 5 |  | critical / high / medium / low |  |  |  |  |

Severity guide:

- critical: prevents host, connection, role assignment, movement, interaction, or basic participation.
- high: match can run, but players cannot understand goals or recover from a common stall.
- medium: hurts pacing, suspicion, or clarity but does not stop the test.
- low: polish, copy, layout, or quality-of-life issue.

## P1-024 Decision

Choose one:

- pass: 6 or more players connected, roles assigned, players interacted with the loop, useful social data was generated, and a keep/cut/change list exists.
- partial: technical or clarity issues blocked completion, but logs and observations identify what to fix.
- fail: host, connection, role assignment, movement, interaction, or logging failed before useful data.

Decision:

```text
P1-024 result: not evaluated (dry-run example)
Reason:
Next backlog item: none - run P1-024 with humans before using this as evidence
```

## Data Handling Check

- [ ] Raw logs are not committed.
- [ ] Raw recordings are not committed.
- [ ] Tester names are removed.
- [ ] Voice transcripts are not included.
- [ ] IP addresses, SteamIDs, and local usernames are removed.
- [ ] Any moderation-sensitive details are summarized without identifying people.
- [ ] Summary is stored in `docs/playtests/` only after anonymization.

## Follow-Up

Immediate next action:

-

Retest command or plan:

```text

```
