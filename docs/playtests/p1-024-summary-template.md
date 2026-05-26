# P1-024 Anonymized Summary Template

Use this after a P1-024 run. Commit the completed summary only after removing tester names, raw voice details, IP addresses, SteamIDs, local machine secrets, crash dump paths that expose usernames, and any raw transcript text.

Raw evidence stays under `Saved/Playtests/...` and is not committed.

## Header

```text
RunId:
Date:
Local snapshot or commit:
Build:
Map:
Profile:
Host type: listen-server
Target players:
Actual players:
Voice method:
Recording consent: yes / no / partial
Raw evidence path: Saved/Playtests/P1-024/run-__
Summary author:
```

## Telemetry Summary

Generate telemetry:

```bash
cargo run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/run-01/events.jsonl --out Saved/Playtests/P1-024/run-01/summary.json
```

Copy the relevant values into this table:

| Field | Value |
| --- | --- |
| `run_id` |  |
| `build_id` |  |
| `map_id` |  |
| `profile` |  |
| `duration_seconds` |  |
| `match_duration_seconds` |  |
| `players_connected` |  |
| `players_disconnected` |  |
| `match_started` |  |
| `match_ended` |  |
| `last_role_assignment.players` |  |
| `last_role_assignment.saboteurs` |  |
| `last_match_result.winner` |  |
| `last_match_result.reason` |  |
| `last_match_result.criticalSystems` |  |
| `ship_task_repairs` |  |
| `ship_task_sabotages` |  |
| `players_downed` |  |
| `players_rescued` |  |
| `players_contained` |  |
| `players_released` |  |
| `errors` |  |
| `invalid_lines` |  |

Paste only a compact, anonymized excerpt if needed:

```json
{
  "run_id": "",
  "duration_seconds": 0,
  "match_duration_seconds": 0,
  "players_connected": 0,
  "players_disconnected": 0,
  "match_started": 0,
  "match_ended": 0,
  "last_role_assignment": {},
  "last_match_result": {}
}
```

## Technical Result

| Gate | Result | Evidence |
| --- | --- | --- |
| Host started | pass / partial / fail |  |
| Clients connected | pass / partial / fail |  |
| Role assignment visible in log | pass / partial / fail |  |
| Players could move | pass / partial / fail |  |
| Players could interact | pass / partial / fail |  |
| Repair or sabotage happened | pass / partial / fail |  |
| Match ended or meaningful interruption captured | pass / partial / fail |  |
| Logs reconstruct the run | pass / partial / fail |  |
| No critical crash | pass / partial / fail |  |

Technical notes:

-

## Validity Gate

Resolve each item before choosing pass, partial, or fail.

| Check | Result | Evidence |
| --- | --- | --- |
| `players_connected == 8` | yes / no |  |
| `match_started > 0` | yes / no |  |
| `last_role_assignment` exists | yes / no |  |
| Crew/agent counts match the player count | yes / no |  |
| At least one repair or sabotage-relevant interaction occurred | yes / no |  |
| No crash or desync blocked basic participation | yes / no |  |
| Logs are present and usable | yes / no |  |
| Keep/cut/change list was produced | yes / no |  |

Validity result:

```text
pass / partial / fail
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
`playtest-preflight` requires the goal-comprehension, confusion, memorable/social moment, repeat-play intent, and keep/cut/change rows below to contain resolved evidence before a human summary is committed.

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

## Comprehension And Accessibility

Summarize GP-09 evidence from observer notes and post-round answers. Use `none observed` only when an observer explicitly checked the signal.

| Signal | Evidence | Follow-up |
| --- | --- | --- |
| Objective comprehension |  |  |
| Next-step clarity |  |  |
| Failure-state clarity |  |  |
| UI or control confusion |  |  |
| Accessibility blocker |  |  |
| Text or term issue |  |  |

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
| Repairs and route tasks |  |  | keep / cut / change |  |  |
| Sabotage actions | sabotages=, winner=, critical_systems= |  | keep / cut / change |  |  |
| Doors and bulkheads |  |  | keep / cut / change |  |  |
| Flooding and pump pressure |  |  | keep / cut / change |  |  |
| PvE, down, rescue, containment |  |  | keep / cut / change |  |  |
| Items and pickup/drop |  |  | keep / cut / change |  |  |
| Role clarity |  |  | keep / cut / change |  |  |
| Pacing |  |  | keep / cut / change |  |  |
| Social suspicion |  |  | keep / cut / change |  |  |

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

- pass: 8 players connected, roles assigned, players interacted with the loop, useful social data was generated, and a keep/cut/change list exists.
- partial: technical or clarity issues blocked completion, but logs and observations identify what to fix.
- fail: host, connection, role assignment, movement, interaction, or logging failed before useful data.

Decision:

```text
P1-024 result: pass / partial / fail
Reason:
Next backlog item: P1-025 / technical rehearsal / rerun P1-024
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
