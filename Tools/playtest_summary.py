#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]


TELEMETRY_FIELDS = [
    "run_id",
    "build_id",
    "map_id",
    "profile",
    "duration_seconds",
    "match_duration_seconds",
    "players_connected",
    "players_disconnected",
    "match_started",
    "match_ended",
    "last_role_assignment.players",
    "last_role_assignment.saboteurs",
    "last_match_result.winner",
    "last_match_result.reason",
    "ship_task_repairs",
    "ship_task_sabotages",
    "players_damaged",
    "players_downed",
    "players_rescued",
    "players_contained",
    "players_released",
    "deaths",
    "errors",
    "invalid_lines",
    "total_events",
]

RISKY_PATTERNS = [
    re.compile(r"/Users/[^/\s|`]+"),
    re.compile(r"\b(?:\d{1,3}\.){3}\d{1,3}\b"),
    re.compile(r"\b7656119\d{10}\b"),
    re.compile(r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b"),
    re.compile(r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+"),
]


def nested_value(value: dict[str, Any], path: str) -> Any:
    current: Any = value
    for part in path.split("."):
        if not isinstance(current, dict) or part not in current:
            return None
        current = current[part]
    return current


def text_value(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, dict):
        return "`" + json.dumps(value, sort_keys=True, ensure_ascii=True) + "`"
    if isinstance(value, list):
        return "`" + json.dumps(value, sort_keys=True, ensure_ascii=True) + "`"
    return sanitize_markdown(str(value))


def sanitize_markdown(value: str) -> str:
    sanitized = value.replace("\n", " ").replace("\r", " ")
    for pattern in RISKY_PATTERNS:
        sanitized = pattern.sub("<redacted>", sanitized)
    return sanitized.replace("|", "\\|")


def bool_result(value: bool | None) -> str:
    if value is None:
        return "observer required"
    return "yes" if value else "no"


def status_result(value: bool | None, partial: bool = False) -> str:
    if value is None:
        return "observer required"
    if value:
        return "pass"
    return "partial" if partial else "fail"


def relative_label(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return "<local evidence path redacted>"


def role_counts(summary: dict[str, Any]) -> tuple[int | None, int | None, int | None]:
    assignment = summary.get("last_role_assignment")
    if not isinstance(assignment, dict):
        return None, None, None
    players = assignment.get("players")
    saboteurs = assignment.get("saboteurs")
    if not isinstance(players, int) or not isinstance(saboteurs, int):
        return None, None, None
    return players, max(players - saboteurs, 0), saboteurs


def expected_saboteurs(player_count: int | None) -> int | None:
    if player_count is None:
        return None
    if player_count >= 7:
        return 2
    if player_count >= 5:
        return 1
    return 0


def interaction_count(summary: dict[str, Any]) -> int:
    keys = [
        "ship_task_repairs",
        "ship_task_sabotages",
        "door_toggles",
        "door_locks",
        "door_unlocks",
        "item_pickups",
        "item_drops",
        "players_downed",
        "players_rescued",
        "players_contained",
        "players_released",
    ]
    return sum(int(summary.get(key) or 0) for key in keys)


def compact_json(summary: dict[str, Any]) -> str:
    excerpt = {
        "run_id": summary.get("run_id"),
        "duration_seconds": summary.get("duration_seconds"),
        "match_duration_seconds": summary.get("match_duration_seconds"),
        "players_connected": summary.get("players_connected"),
        "players_disconnected": summary.get("players_disconnected"),
        "match_started": summary.get("match_started"),
        "match_ended": summary.get("match_ended"),
        "last_role_assignment": summary.get("last_role_assignment") or {},
        "last_match_result": summary.get("last_match_result") or {},
    }
    return json.dumps(excerpt, indent=2, sort_keys=True, ensure_ascii=True)


def validate_summary_shape(summary: dict[str, Any]) -> None:
    if "event" in summary or "payload" in summary:
        raise SystemExit("input looks like a raw event record; run Tools/log_summary.py --out first")
    required_any = {"total_events", "match_started", "players_connected", "events_by_type"}
    if not required_any.intersection(summary):
        raise SystemExit("input does not look like Tools/log_summary.py summary JSON")


def telemetry_table(summary: dict[str, Any]) -> str:
    rows = ["| Field | Value |", "| --- | --- |"]
    for field in TELEMETRY_FIELDS:
        rows.append(f"| `{field}` | {text_value(nested_value(summary, field))} |")
    return "\n".join(rows)


def validity_table(summary: dict[str, Any], min_players: int) -> str:
    players, _crew, saboteurs = role_counts(summary)
    expected = expected_saboteurs(players)
    role_counts_match = None if players is None or saboteurs is None or expected is None else saboteurs == expected
    interactions = interaction_count(summary)
    repair_or_sabotage = int(summary.get("ship_task_repairs") or 0) + int(summary.get("ship_task_sabotages") or 0)
    logs_usable = int(summary.get("total_events") or 0) > 0 and int(summary.get("invalid_lines") or 0) == 0
    no_crash_from_logs = int(summary.get("errors") or 0) == 0 and int(summary.get("invalid_lines") or 0) == 0

    rows = [
        "| Check | Result | Evidence |",
        "| --- | --- | --- |",
        f"| `players_connected >= {min_players}` | {bool_result(int(summary.get('players_connected') or 0) >= min_players)} | players_connected={summary.get('players_connected') or 0} |",
        f"| `match_started > 0` | {bool_result(int(summary.get('match_started') or 0) > 0)} | match_started={summary.get('match_started') or 0} |",
        f"| `last_role_assignment` exists | {bool_result(players is not None)} | players={players if players is not None else ''}, saboteurs={saboteurs if saboteurs is not None else ''} |",
        f"| Crew/agent counts match player count | {bool_result(role_counts_match)} | expected_saboteurs={expected if expected is not None else ''} |",
        f"| At least one repair or sabotage-relevant interaction occurred | {bool_result(repair_or_sabotage > 0)} | ship_task_repairs={summary.get('ship_task_repairs') or 0}, ship_task_sabotages={summary.get('ship_task_sabotages') or 0} |",
        f"| Any interaction evidence exists | {bool_result(interactions > 0)} | interaction_count={interactions} |",
        f"| No crash or desync blocked basic participation | {bool_result(no_crash_from_logs)} | errors={summary.get('errors') or 0}, invalid_lines={summary.get('invalid_lines') or 0}; observer confirm |",
        f"| Logs are present and usable | {bool_result(logs_usable)} | total_events={summary.get('total_events') or 0} |",
        "| Keep/cut/change list was produced | observer required | Fill from player feedback |",
    ]
    return "\n".join(rows)


def technical_table(summary: dict[str, Any], min_players: int) -> str:
    players_connected = int(summary.get("players_connected") or 0)
    match_started = int(summary.get("match_started") or 0)
    match_ended = int(summary.get("match_ended") or 0)
    role_assigned = summary.get("last_role_assignment") is not None
    interactions = interaction_count(summary)
    repair_or_sabotage = int(summary.get("ship_task_repairs") or 0) + int(summary.get("ship_task_sabotages") or 0)
    logs_usable = int(summary.get("total_events") or 0) > 0 and int(summary.get("invalid_lines") or 0) == 0
    no_crash = int(summary.get("errors") or 0) == 0

    rows = [
        "| Gate | Result | Evidence |",
        "| --- | --- | --- |",
        f"| Host started | {status_result(int(summary.get('total_events') or 0) > 0)} | total_events={summary.get('total_events') or 0} |",
        f"| Clients connected | {status_result(players_connected >= min_players, partial=players_connected > 0)} | players_connected={players_connected} |",
        f"| Role assignment visible in log | {status_result(role_assigned)} | last_role_assignment={text_value(summary.get('last_role_assignment'))} |",
        "| Players could move | observer required | Fill from observer notes |",
        f"| Players could interact | {status_result(interactions > 0, partial=True)} | interaction_count={interactions}; observer confirm |",
        f"| Repair or sabotage happened | {status_result(repair_or_sabotage > 0, partial=True)} | repairs={summary.get('ship_task_repairs') or 0}, sabotages={summary.get('ship_task_sabotages') or 0} |",
        f"| Match ended or meaningful interruption captured | {status_result(match_ended > 0, partial=match_started > 0)} | match_started={match_started}, match_ended={match_ended} |",
        f"| Logs reconstruct the run | {status_result(logs_usable)} | invalid_lines={summary.get('invalid_lines') or 0} |",
        f"| No critical crash | {status_result(no_crash)} | errors={summary.get('errors') or 0}; observer confirm |",
    ]
    return "\n".join(rows)


def mechanics_table(summary: dict[str, Any]) -> str:
    role_players, crew, saboteurs = role_counts(summary)
    rows = [
        "| Area | Telemetry evidence | Observer/player evidence | Decision | Reason | Next action |",
        "| --- | --- | --- | --- | --- | --- |",
        f"| Repairs and route tasks | repairs={summary.get('ship_task_repairs') or 0}, route_final={summary.get('final_approach_started') or 0} |  | keep / cut / change |  |  |",
        f"| Sabotage actions | sabotages={summary.get('ship_task_sabotages') or 0}, winner={text_value(nested_value(summary, 'last_match_result.winner'))} |  | keep / cut / change |  |  |",
        f"| Doors and bulkheads | toggles={summary.get('door_toggles') or 0}, locks={summary.get('door_locks') or 0}, releases={summary.get('bulkhead_lock_releases') or 0} |  | keep / cut / change |  |  |",
        f"| Flooding and pump pressure | pressure_events={summary.get('flooding_pressure_events') or 0}, last_pressure={text_value(summary.get('last_flooding_pressure'))} |  | keep / cut / change |  |  |",
        f"| PvE, down, rescue, containment | damaged={summary.get('players_damaged') or 0}, downed={summary.get('players_downed') or 0}, rescued={summary.get('players_rescued') or 0}, contained={summary.get('players_contained') or 0}, released={summary.get('players_released') or 0} |  | keep / cut / change |  |  |",
        f"| Items and pickup/drop | pickups={summary.get('item_pickups') or 0}, drops={summary.get('item_drops') or 0} |  | keep / cut / change |  |  |",
        f"| Role clarity | players={role_players if role_players is not None else ''}, crew={crew if crew is not None else ''}, agents={saboteurs if saboteurs is not None else ''} |  | keep / cut / change |  |  |",
        f"| Pacing | duration={text_value(summary.get('duration_seconds'))}, match_duration={text_value(summary.get('match_duration_seconds'))} |  | keep / cut / change |  |  |",
        "| Social suspicion | telemetry cannot judge |  | keep / cut / change |  |  |",
    ]
    return "\n".join(rows)


def render_markdown(summary: dict[str, Any], args: argparse.Namespace) -> str:
    summary_path = args.summary.resolve()
    raw_evidence = sanitize_markdown(args.raw_evidence_label or relative_label(summary_path.parent))
    summary_input = sanitize_markdown(relative_label(summary_path))
    today = sanitize_markdown(args.date or dt.date.today().isoformat())
    dry_run_notice = ""
    intro = "Use this after a P1-024 run."
    validity_result = "pass / partial / fail"
    decision_result = "P1-024 result: pass / partial / fail"
    next_backlog = "Next backlog item: P1-025 / technical rehearsal / rerun P1-024"
    generated_from_commands = (
        "python3 Tools/log_summary.py Saved/Playtests/P1-024/run-01/events.jsonl --out Saved/Playtests/P1-024/run-01/summary.json\n"
        "python3 Tools/playtest_summary.py Saved/Playtests/P1-024/run-01/summary.json --out docs/playtests/p1-024-run-01-summary.md"
    )
    if args.dry_run_example:
        dry_run_notice = (
            "\n"
            "> Dry-run example only. This file was generated from smoke-test telemetry, not a human playtest. "
            "Do not count it as P1-024 evidence, social validation, balance evidence, or a pass/partial/fail result.\n"
        )
        intro = "This dry-run demonstrates the summary format with smoke telemetry."
        validity_result = "not evaluated - smoke-test dry run"
        decision_result = "P1-024 result: not evaluated (dry-run example)"
        next_backlog = "Next backlog item: none - run P1-024 with humans before using this as evidence"
        generated_from_commands = (
            f"python3 Tools/playtest_summary.py {summary_input} "
            "--out docs/playtests/p1-024-dry-run-summary-example.md --dry-run-example"
        )

    return f"""# {args.title}
{dry_run_notice}

{intro} Commit a completed human-test summary only after removing tester names, raw voice details, IP addresses, SteamIDs, local machine secrets, crash dump paths that expose usernames, and raw transcript text.

Raw evidence stays under `Saved/Playtests/...` and is not committed.

## Header

```text
RunId: {text_value(summary.get("run_id"))}
Date: {today}
Local snapshot or commit: {sanitize_markdown(args.snapshot or "")}
Build: {text_value(summary.get("build_id"))}
Map: {text_value(summary.get("map_id"))}
Profile: {text_value(summary.get("profile"))}
Host type: listen-server
Target players: {args.target_players or ""}
Actual players: {text_value(summary.get("players_connected"))}
Voice method:
Recording consent: yes / no / partial
Raw evidence path: {raw_evidence}
Summary author: {sanitize_markdown(args.author or "")}
```

## Telemetry Summary

Generated from:

```bash
{generated_from_commands}
```

{telemetry_table(summary)}

Compact anonymized excerpt:

```json
{compact_json(summary)}
```

## Technical Result

{technical_table(summary, args.min_players)}

Technical notes:

-

## Validity Gate

Resolve each item before choosing pass, partial, or fail.

{validity_table(summary, args.min_players)}

Validity result:

```text
{validity_result}
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

{mechanics_table(summary)}

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
{decision_result}
Reason:
{next_backlog}
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
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate an anonymized P1 playtest summary skeleton from Tools/log_summary.py JSON.")
    parser.add_argument("summary", type=Path, help="Path to a JSON file produced by Tools/log_summary.py --out.")
    parser.add_argument("--out", type=Path, help="Optional Markdown output path.")
    parser.add_argument("--title", default="P1-024 Anonymized Summary", help="Markdown title.")
    parser.add_argument("--min-players", type=int, default=6, help="Minimum player count for a valid P1-024 run.")
    parser.add_argument("--target-players", type=int, help="Target player count to place in the header.")
    parser.add_argument("--raw-evidence-label", help="Safe local-only raw evidence label to place in the header.")
    parser.add_argument("--snapshot", help="Commit or local snapshot label.")
    parser.add_argument("--author", help="Summary author label.")
    parser.add_argument("--date", help="Summary date in YYYY-MM-DD format. Defaults to today.")
    parser.add_argument("--dry-run-example", action="store_true", help="Mark output as a smoke-telemetry dry-run example, not human playtest evidence.")
    args = parser.parse_args()

    try:
        with args.summary.open("r", encoding="utf-8") as handle:
            summary = json.load(handle)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"input is not a single summary JSON object; run Tools/log_summary.py --out first: {exc}") from exc
    if not isinstance(summary, dict):
        raise SystemExit("summary JSON must contain an object")
    validate_summary_shape(summary)

    markdown = render_markdown(summary, args)
    if args.out:
        output_path = args.out if args.out.is_absolute() else ROOT / args.out
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(markdown, encoding="utf-8")
    else:
        print(markdown, end="")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
