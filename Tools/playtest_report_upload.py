#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.log_summary import summarize_events


DEFAULT_ENDPOINT = "http://localhost:8787/v1/playtest-reports"
RISKY_PATTERNS = [
    ("local user path", re.compile(r"/Users/[^/\s|`]+|[A-Z]:\\Users\\[^\\\s|`]+", re.IGNORECASE)),
    ("IPv4 address", re.compile(r"\b(?:\d{1,3}\.){3}\d{1,3}\b")),
    ("SteamID64-like value", re.compile(r"\b7656119\d{10}\b")),
    ("email address", re.compile(r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b")),
    ("secret-like assignment", re.compile(r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+")),
]


def load_summary(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            value = json.load(handle)
    except json.JSONDecodeError:
        return summarize_events(path)
    if not isinstance(value, dict) or "event" in value or "payload" in value:
        raise SystemExit("input must be a Tools/log_summary.py summary JSON object")
    return value


def as_int(value: object) -> int:
    if isinstance(value, bool):
        return 0
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    return 0


def nested_dict(value: object) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def normalize_winner(value: object) -> str:
    text = str(value or "unknown").strip().lower()
    if text in {"crew", "agents", "none", "unknown"}:
        return text
    if text in {"agent", "saboteur", "saboteurs", "traitor", "traitors"}:
        return "agents"
    return "unknown"


def player_count(summary: dict[str, Any]) -> int:
    connected = as_int(summary.get("players_connected"))
    if connected > 0:
        return max(1, min(8, connected))

    assignment = nested_dict(summary.get("last_role_assignment"))
    assigned_players = as_int(assignment.get("players"))
    if assigned_players > 0:
        return max(1, min(8, assigned_players))
    return 1


def concise_summary(summary: dict[str, Any], note: str = "") -> str:
    result = nested_dict(summary.get("last_match_result"))
    parts = [
        f"run_id={summary.get('run_id') or ''}",
        f"profile={summary.get('profile') or ''}",
        f"players_connected={as_int(summary.get('players_connected'))}",
        f"players_disconnected={as_int(summary.get('players_disconnected'))}",
        f"match_started={as_int(summary.get('match_started'))}",
        f"match_ended={as_int(summary.get('match_ended'))}",
        f"winner={normalize_winner(result.get('winner'))}",
        f"reason={result.get('reason') or ''}",
        f"repairs={as_int(summary.get('ship_task_repairs'))}",
        f"sabotages={as_int(summary.get('ship_task_sabotages'))}",
        f"downs={as_int(summary.get('players_downed'))}",
        f"rescues={as_int(summary.get('players_rescued'))}",
        f"duration_seconds={summary.get('duration_seconds') or ''}",
        f"match_duration_seconds={summary.get('match_duration_seconds') or ''}",
    ]
    if note:
        parts.append(f"note={note}")
    return "; ".join(parts)


def assert_no_risky_text(text: str) -> None:
    hits = [label for label, pattern in RISKY_PATTERNS if pattern.search(text)]
    if hits:
        raise SystemExit(f"refusing to upload summary with risky text: {', '.join(hits)}")


def build_payload(summary: dict[str, Any], note: str = "") -> dict[str, Any]:
    result = nested_dict(summary.get("last_match_result"))
    summary_text = concise_summary(summary, note)
    assert_no_risky_text(summary_text)

    payload = {
        "runId": str(summary.get("run_id") or "unknown-run"),
        "buildId": str(summary.get("build_id") or "unknown-build"),
        "mapId": str(summary.get("map_id") or "unknown-map"),
        "playerCount": player_count(summary),
        "winner": normalize_winner(result.get("winner")),
        "summary": summary_text,
    }
    return payload


def post_json(endpoint: str, payload: dict[str, Any], timeout: float) -> dict[str, Any]:
    body = json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        endpoint,
        data=body,
        headers={"content-type": "application/json", "accept": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            response_body = response.read().decode("utf-8")
            return json.loads(response_body) if response_body else {}
    except urllib.error.HTTPError as exc:
        response_body = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"upload failed: HTTP {exc.code}: {response_body}") from exc
    except urllib.error.URLError as exc:
        raise SystemExit(f"upload failed: {exc.reason}") from exc


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare or upload an anonymized playtest report to the local backend.")
    parser.add_argument("summary", type=Path, help="Path to Tools/log_summary.py summary JSON or raw JSONL events.")
    parser.add_argument("--endpoint", default=DEFAULT_ENDPOINT, help="Backend playtest report endpoint.")
    parser.add_argument("--note", default="", help="Short anonymized note to append to the report summary.")
    parser.add_argument("--timeout", type=float, default=5.0, help="HTTP timeout in seconds.")
    parser.add_argument("--send", action="store_true", help="Actually POST to the backend. Default only prints payload JSON.")
    args = parser.parse_args()

    payload = build_payload(load_summary(args.summary), args.note)
    print(json.dumps(payload, indent=2, sort_keys=True))
    if not args.send:
        print("# dry-run: add --send to POST this payload.")
        return 0

    response = post_json(args.endpoint, payload, args.timeout)
    print(json.dumps(response, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
