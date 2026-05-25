#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
EXPECTED_MAP = "/Game/Maps/L_IcebreakerWhitebox"
EXPECTED_PROFILE = "human-p1-024"
EXPECTED_RUN_PREFIX = "P1-024"
EXPECTED_PLAYTEST_PATH = "Saved/Playtests/P1-024/"
CORE_SUMMARY_FIELDS = [
    "run_id",
    "build_id",
    "map_id",
    "profile",
    "players_connected",
    "players_disconnected",
    "duration_seconds",
    "match_started",
    "match_ended",
    "invalid_lines",
    "total_events",
    "events_by_type",
]
REQUIRED_MARKDOWN_SECTIONS = [
    "Header",
    "Telemetry Summary",
    "Technical Result",
    "Validity Gate",
    "Observed Flow",
    "Player Feedback",
    "Keep / Cut / Change",
    "Mechanics Decision Table",
    "Top Blockers",
    "P1-024 Decision",
    "Data Handling Check",
]

RISKY_MARKDOWN_PATTERNS = [
    ("local user path", re.compile(r"/Users/[^/\s|`]+")),
    ("IPv4 address", re.compile(r"\b(?:\d{1,3}\.){3}\d{1,3}\b")),
    ("SteamID64-like value", re.compile(r"\b7656119\d{10}\b")),
    ("email address", re.compile(r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b")),
    ("secret-like assignment", re.compile(r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+")),
]

UNRESOLVED_MARKDOWN_PATTERNS = [
    ("unresolved P1-024 decision", re.compile(r"P1-024 result:\s*pass\s*/\s*partial\s*/\s*fail", re.IGNORECASE)),
    ("choose-one instruction still present", re.compile(r"Choose one:", re.IGNORECASE)),
    ("observer-required placeholder", re.compile(r"observer required", re.IGNORECASE)),
    ("fill-from placeholder", re.compile(r"Fill from", re.IGNORECASE)),
    ("keep/cut/change placeholder", re.compile(r"keep\s*/\s*cut\s*/\s*change", re.IGNORECASE)),
    ("recording-consent placeholder", re.compile(r"Recording consent:\s*yes\s*/\s*no", re.IGNORECASE)),
    ("unchecked data-handling item", re.compile(r"^- \[ \] ", re.MULTILINE)),
]

DECISION_PATTERN = re.compile(r"P1-024 result:\s*(pass|partial|fail)\b", re.IGNORECASE)
DRY_RUN_DECISION_PATTERN = re.compile(r"P1-024 result:\s*not evaluated\s*\(dry-run example\)", re.IGNORECASE)


@dataclass
class CheckResult:
    name: str
    status: str
    detail: str


def relative_label(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def load_summary(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            value = json.load(handle)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"input is not a single summary JSON object; run Tools/log_summary.py --out first: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit("summary JSON must contain an object")
    if "event" in value or "payload" in value:
        raise SystemExit("input looks like a raw event record; run Tools/log_summary.py --out first")
    if not {"total_events", "match_started", "players_connected", "events_by_type"}.intersection(value):
        raise SystemExit("input does not look like Tools/log_summary.py summary JSON")
    return value


def as_int(value: object) -> int:
    if isinstance(value, bool):
        return 0
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    return 0


def nested_value(value: dict[str, Any], path: str) -> Any:
    current: Any = value
    for part in path.split("."):
        if not isinstance(current, dict) or part not in current:
            return None
        current = current[part]
    return current


def expected_saboteurs(player_count: int | None) -> int | None:
    if player_count is None:
        return None
    if player_count >= 7:
        return 2
    if player_count >= 5:
        return 1
    return 0


def role_counts(summary: dict[str, Any]) -> tuple[int | None, int | None]:
    assignment = summary.get("last_role_assignment")
    if not isinstance(assignment, dict):
        return None, None
    players = assignment.get("players")
    saboteurs = assignment.get("saboteurs")
    if not isinstance(players, int) or not isinstance(saboteurs, int):
        return None, None
    return players, saboteurs


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
    return sum(as_int(summary.get(key)) for key in keys)


def smoke_or_qa_events(summary: dict[str, Any]) -> list[str]:
    events_by_type = summary.get("events_by_type")
    if not isinstance(events_by_type, dict):
        return []
    return sorted(key for key in events_by_type if str(key).startswith(("dev_smoke_", "qa_")))


def add(results: list[CheckResult], name: str, passed: bool, detail: str, warn: bool = False) -> None:
    if passed:
        status = "pass"
    else:
        status = "warn" if warn else "fail"
    results.append(CheckResult(name, status, detail))


def path_contains_playtest_label(path: Path, source: object) -> bool:
    labels = [relative_label(path), str(source or "")]
    return any(EXPECTED_PLAYTEST_PATH in label for label in labels)


def check_summary(summary: dict[str, Any], summary_path: Path, mode: str, min_players: int) -> list[CheckResult]:
    results: list[CheckResult] = []
    total_events = as_int(summary.get("total_events"))
    invalid_lines = as_int(summary.get("invalid_lines"))
    errors = as_int(summary.get("errors"))
    players_connected = as_int(summary.get("players_connected"))
    players_disconnected = as_int(summary.get("players_disconnected"))
    match_started = as_int(summary.get("match_started"))
    match_ended = as_int(summary.get("match_ended"))
    players, saboteurs = role_counts(summary)
    expected = expected_saboteurs(players)
    repairs_or_sabotages = as_int(summary.get("ship_task_repairs")) + as_int(summary.get("ship_task_sabotages"))
    interactions = interaction_count(summary)
    missing_core = [field for field in CORE_SUMMARY_FIELDS if field not in summary]

    add(results, "summary_shape", True, "summary JSON shape is compatible with Tools/log_summary.py")
    add(results, "events_present", total_events > 0, f"total_events={total_events}")
    add(results, "jsonl_parse_clean", invalid_lines == 0, f"invalid_lines={invalid_lines}")
    add(results, "no_logged_errors", errors == 0, f"errors={errors}")
    add(results, "map_id", summary.get("map_id") == EXPECTED_MAP, f"map_id={summary.get('map_id')!r}")

    if mode == "dry-run":
        add(results, "dry_run_player_count_not_scored", True, f"players_connected={players_connected}; human minimum is not evaluated in dry-run mode")
        add(results, "dry_run_source_allowed", True, f"summary={relative_label(summary_path)}")
        return results

    add(results, "core_summary_fields", not missing_core, ", ".join(missing_core) if missing_core else "all core fields present")
    add(results, "run_id", str(summary.get("run_id") or "").startswith(EXPECTED_RUN_PREFIX), f"run_id={summary.get('run_id')!r}")
    add(results, "profile", summary.get("profile") == EXPECTED_PROFILE, f"profile={summary.get('profile')!r}")
    human_smoke_events = smoke_or_qa_events(summary)
    add(results, "no_smoke_or_qa_basis", not human_smoke_events, ", ".join(human_smoke_events) if human_smoke_events else "no smoke or QA event types found")
    add(
        results,
        "playtest_evidence_path",
        path_contains_playtest_label(summary_path, summary.get("source")),
        f"summary={relative_label(summary_path)}, source={summary.get('source')!r}",
    )
    add(results, "minimum_players", players_connected >= min_players, f"players_connected={players_connected}, required={min_players}")
    add(results, "match_started", match_started > 0, f"match_started={match_started}")
    add(results, "role_assignment_present", players is not None and saboteurs is not None, f"players={players}, saboteurs={saboteurs}")
    add(results, "role_count_matches", players is not None and saboteurs == expected, f"players={players}, saboteurs={saboteurs}, expected={expected}")
    add(results, "repair_or_sabotage_seen", repairs_or_sabotages > 0, f"repairs+sabotages={repairs_or_sabotages}")
    add(results, "interaction_seen", interactions > 0, f"interaction_count={interactions}")
    add(results, "disconnects_recorded", players_disconnected >= 0, f"players_disconnected={players_disconnected}")
    add(results, "match_end_optional", match_ended > 0, f"match_ended={match_ended}; partial/fail summaries may still be valid", warn=True)
    return results


def read_markdown(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def header_value(text: str, label: str) -> str | None:
    match = re.search(rf"^{re.escape(label)}:\s*(.*?)\s*$", text, re.MULTILINE)
    if not match:
        return None
    return match.group(1).strip()


def check_header_match(results: list[CheckResult], text: str, summary: dict[str, Any]) -> None:
    expected_values = {
        "RunId": str(summary.get("run_id") or ""),
        "Build": str(summary.get("build_id") or ""),
        "Map": str(summary.get("map_id") or ""),
        "Profile": str(summary.get("profile") or ""),
        "Actual players": str(summary.get("players_connected") or ""),
    }
    mismatches: list[str] = []
    for label, expected in expected_values.items():
        actual = header_value(text, label)
        if actual != expected:
            mismatches.append(f"{label}: expected {expected!r}, got {actual!r}")
    add(results, "markdown_header_matches_summary", not mismatches, "; ".join(mismatches) if mismatches else "header values match summary")


def pass_decision_objective_blockers(summary: dict[str, Any], min_players: int) -> list[str]:
    blockers: list[str] = []
    players_connected = as_int(summary.get("players_connected"))
    players, saboteurs = role_counts(summary)
    expected = expected_saboteurs(players)
    if players_connected < min_players:
        blockers.append(f"players_connected={players_connected} < {min_players}")
    if as_int(summary.get("match_started")) <= 0:
        blockers.append("match_started=0")
    if players is None or saboteurs is None:
        blockers.append("role assignment missing")
    elif saboteurs != expected:
        blockers.append(f"saboteurs={saboteurs}, expected={expected}")
    if as_int(summary.get("ship_task_repairs")) + as_int(summary.get("ship_task_sabotages")) <= 0:
        blockers.append("no repair/sabotage evidence")
    if interaction_count(summary) <= 0:
        blockers.append("no interaction evidence")
    if as_int(summary.get("invalid_lines")) != 0:
        blockers.append(f"invalid_lines={as_int(summary.get('invalid_lines'))}")
    if as_int(summary.get("errors")) != 0:
        blockers.append(f"errors={as_int(summary.get('errors'))}")
    return blockers


def check_markdown(markdown_path: Path | None, mode: str, summary: dict[str, Any], min_players: int) -> list[CheckResult]:
    results: list[CheckResult] = []
    if markdown_path is None:
        add(results, "markdown_checked", False, "no --markdown file supplied; committed summary privacy/placeholders not checked", warn=True)
        return results

    if not markdown_path.exists():
        add(results, "markdown_exists", False, f"{markdown_path} does not exist")
        return results

    text = read_markdown(markdown_path)
    relative = relative_label(markdown_path)
    add(results, "markdown_exists", True, relative)
    add(results, "markdown_location", relative.startswith("docs/playtests/"), relative)
    missing_sections = [section for section in REQUIRED_MARKDOWN_SECTIONS if f"## {section}" not in text]
    add(results, "markdown_required_sections", not missing_sections, ", ".join(missing_sections) if missing_sections else "all required sections present")
    check_header_match(results, text, summary)

    risky_hits: list[str] = []
    for label, pattern in RISKY_MARKDOWN_PATTERNS:
        if pattern.search(text):
            risky_hits.append(label)
    add(results, "markdown_redaction", not risky_hits, ", ".join(risky_hits) if risky_hits else "no risky identity/path patterns found")

    has_dry_run_notice = "Dry-run example only" in text or "smoke-test telemetry, not a human playtest" in text
    if mode == "dry-run":
        add(results, "dry_run_notice", has_dry_run_notice, "dry-run warning present" if has_dry_run_notice else "missing dry-run warning")
        add(results, "dry_run_decision", DRY_RUN_DECISION_PATTERN.search(text) is not None, "expected not evaluated dry-run decision")
        add(results, "dry_run_not_marked_human_result", DECISION_PATTERN.search(text) is None, "no pass/partial/fail P1-024 result found")
        return results

    add(results, "no_dry_run_marker", not has_dry_run_notice, "human summary must not contain dry-run warning")
    decision_match = DECISION_PATTERN.search(text)
    decision = decision_match.group(1).lower() if decision_match else ""
    add(results, "decision_resolved", decision_match is not None, f"decision={decision}")
    if decision == "pass":
        pass_blockers = pass_decision_objective_blockers(summary, min_players)
        add(results, "pass_decision_matches_objective_gates", not pass_blockers, ", ".join(pass_blockers) if pass_blockers else "objective pass gates satisfied")

    unresolved_hits: list[str] = []
    for label, pattern in UNRESOLVED_MARKDOWN_PATTERNS:
        if pattern.search(text):
            unresolved_hits.append(label)
    add(results, "no_unresolved_placeholders", not unresolved_hits, ", ".join(unresolved_hits) if unresolved_hits else "key placeholders resolved")
    return results


def print_results(results: list[CheckResult]) -> None:
    for result in results:
        print(f"[{result.status.upper()}] {result.name}: {result.detail}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Preflight-check a P1-024 playtest summary before committing anonymized evidence.")
    parser.add_argument("summary", type=Path, help="Path to a JSON file produced by Tools/log_summary.py --out.")
    parser.add_argument("--markdown", type=Path, help="Generated anonymized Markdown summary to check for privacy and unresolved placeholders.")
    parser.add_argument("--mode", choices=("human", "dry-run"), default="human", help="Use dry-run for committed smoke-telemetry examples only.")
    parser.add_argument("--min-players", type=int, default=6, help="Minimum player count for a valid P1-024 human run.")
    args = parser.parse_args()

    summary = load_summary(args.summary)
    results = check_summary(summary, args.summary, args.mode, args.min_players)
    results.extend(check_markdown(args.markdown, args.mode, summary, args.min_players))
    print_results(results)

    failed = [result for result in results if result.status == "fail"]
    warned = [result for result in results if result.status == "warn"]
    if failed:
        print(f"[RESULT] fail: {len(failed)} failed, {len(warned)} warning(s)")
        return 1
    print(f"[RESULT] pass: {len(warned)} warning(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
