#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export a Frostwake smoke suite JSON summary as compact Markdown.")
    parser.add_argument("suite_summary", type=Path, help="Path to suite_summary.json.")
    parser.add_argument("--out", type=Path, help="Optional Markdown output path. Defaults beside the suite summary.")
    return parser.parse_args()


def nested(summary: dict[str, Any] | None, key: str) -> Any:
    if not isinstance(summary, dict):
        return ""
    value = summary.get(key, "")
    return "" if value is None else value


def render_markdown(data: dict[str, Any]) -> str:
    lines = [
        "# Smoke Suite Evidence",
        "",
        f"- Suite: `{data.get('suite_dir', '')}`",
        f"- Passed: `{str(data.get('passed', False)).lower()}`",
        f"- Profiles: `{', '.join(data.get('profiles', []))}`",
        "",
        "| Profile | Return | Match Started | Roles | Smoke Pass | Invalid Lines | Log Dir |",
        "| --- | ---: | ---: | --- | ---: | ---: | --- |",
    ]

    for result in data.get("results", []):
        summary = result.get("summary") if isinstance(result, dict) else None
        smoke_pass = (
            nested(summary, "qa_bot_smoke_passed")
            or nested(summary, "qa_player_bot_smoke_passed")
            or nested(summary, "qa_task_bot_smoke_passed")
            or nested(summary, "combined_systems_smoke_passed")
            or nested(summary, "item_drop_smoke_passed")
            or nested(summary, "pump_flooding_smoke_passed")
            or nested(summary, "pve_enemy_smoke_passed")
            or nested(summary, "bulkhead_lock_smoke_passed")
            or ""
        )
        roles = ""
        if isinstance(summary, dict) and isinstance(summary.get("last_role_assignment"), dict):
            assignment = summary["last_role_assignment"]
            roles = f"{assignment.get('players', '')}p/{assignment.get('saboteurs', '')}s"
        lines.append(
            "| {profile} | {returncode} | {match_started} | {roles} | {smoke_pass} | {invalid_lines} | `{log_dir}` |".format(
                profile=result.get("profile", ""),
                returncode=result.get("returncode", ""),
                match_started=nested(summary, "match_started"),
                roles=roles,
                smoke_pass=smoke_pass,
                invalid_lines=nested(summary, "invalid_lines"),
                log_dir=result.get("log_dir", ""),
            )
        )

    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    data = json.loads(args.suite_summary.read_text(encoding="utf-8"))
    markdown = render_markdown(data)
    out_path = args.out or args.suite_summary.with_suffix(".md")
    out_path.write_text(markdown, encoding="utf-8")
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
