# GP-03 Fatal Sabotage Smoke Summary

> Automation evidence only. This is not a human playtest and must not be counted as P1-024 social, comprehension, or repeat-play proof.

## Purpose

GP-03 needs sabotage losses to be explainable after the round. This focused smoke verifies that the named `fatal-sabotage` profile can drive a critical ship system to failure and leave enough structured evidence to explain why the match ended.

## Latest Run

| Field | Value |
| --- | --- |
| Run ID | `cycle77-gp03-fatal-sabotage` |
| Build ID | `cycle77-gp03-fatal-sabotage` |
| Profile | `fatal-sabotage` |
| Map | `/Game/Maps/L_IcebreakerWhitebox` |
| Local evidence | `Saved/SmokeTests/local-20260526-183356/` |
| Event log | `Saved/SmokeTests/local-20260526-183356/events.jsonl` |
| Summary JSON | `Saved/SmokeTests/local-20260526-183356/summary.json` |

## Result Snapshot

| Signal | Result |
| --- | --- |
| `match_started` | `1` |
| `match_ended` | `1` |
| `players_connected` | `1` |
| `last_role_assignment.players` | `1` |
| `last_role_assignment.saboteurs` | `1` |
| `ship_task_sabotages` | `3` |
| `last_match_result.winner` | `saboteur` |
| `last_match_result.reason` | `fatal_ship_state` |
| `last_match_result.criticalSystems` | `["radio"]` |
| `last_match_result.systemConditions.radio` | `0` |
| `errors` | `0` |
| `invalid_lines` | `0` |

## Interpretation

The smoke demonstrates the technical explanation path for a sabotage-driven loss: a saboteur applies three sabotage interactions to the radio task, the radio condition reaches zero, and the match ends with `winner=saboteur` and `reason=fatal_ship_state`.

This improves GP-03 evidence quality because the end state includes both the failed system list and final system-condition map. It still does not prove that humans notice the clues, trust the UI, or can retell the cause under social pressure. Those questions remain for P1-024/P1-025 observer notes and post-round survey answers.

## Keep / Change / Defer

| Decision | Rationale | Next Check |
| --- | --- | --- |
| Keep the named `fatal-sabotage` profile as the focused GP-03 terminal-path check | It now produces a compact run with winner, loss reason, critical system, and system condition proof | Reuse it after gameplay tuning that touches ship tasks, sabotage deltas, or match-end rules |
| Keep terminal sabotage outside the default smoke suite for now | It intentionally ends the match and can mask non-terminal interaction regressions | Promote only if GP-03 starts changing fatal-state logic frequently |
| Defer human readability claims | This run proves telemetry and mechanics, not social comprehension | Use P1-024 human notes to decide the next wording, UI, or clue-surface change |
