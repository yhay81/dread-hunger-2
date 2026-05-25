#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
RUN_LOCAL_SMOKE = ROOT / "Tools" / "ue" / "run_local_smoke.py"
LOG_SUMMARY = ROOT / "Tools" / "log_summary.py"
DEFAULT_QUICK_PROFILES = ("qa-bot", "qa-player-bot")
HEAVY_PROFILES = ("qa-task-bot", "combined5", "ready8", "combined8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run named Frostwake smoke profiles in sequence and collect summaries.")
    parser.add_argument(
        "--profiles",
        nargs="+",
        default=None,
        help="Profiles to run. Defaults to a quick suite unless --include-heavy is set.",
    )
    parser.add_argument("--include-heavy", action="store_true", help="Append heavier 5-8 player profiles to the default quick suite.")
    parser.add_argument("--skip-build", action="store_true", help="Pass --skip-build to each smoke profile.")
    parser.add_argument("--null-rhi", action="store_true", help="Pass --null-rhi to each smoke profile.")
    parser.add_argument("--suite-dir", type=Path, default=None, help="Output directory for suite logs and suite_summary.json.")
    return parser.parse_args()


def make_suite_dir(value: Path | None) -> Path:
    if value:
        path = value if value.is_absolute() else ROOT / value
    else:
        stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
        path = ROOT / "Saved" / "SmokeSuites" / f"suite-{stamp}"

    path.mkdir(parents=True, exist_ok=True)
    return path


def resolve_profiles(args: argparse.Namespace) -> list[str]:
    if args.profiles:
        return args.profiles

    profiles = list(DEFAULT_QUICK_PROFILES)
    if args.include_heavy:
        profiles.extend(HEAVY_PROFILES)
    return profiles


def run_command(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)


def summarize_events(events_path: Path) -> dict[str, Any] | None:
    if not events_path.exists():
        return None

    completed = run_command(["python3", str(LOG_SUMMARY), str(events_path)])
    if completed.returncode != 0:
        return {
            "summary_error": completed.stderr.strip() or completed.stdout.strip() or f"exit code {completed.returncode}",
        }

    return json.loads(completed.stdout)


def main() -> int:
    args = parse_args()
    profiles = resolve_profiles(args)
    suite_dir = make_suite_dir(args.suite_dir)

    results: list[dict[str, Any]] = []
    exit_code = 0
    for index, profile in enumerate(profiles, start=1):
        profile_dir = suite_dir / f"{index:02d}-{profile}"
        command = [
            "python3",
            str(RUN_LOCAL_SMOKE),
            "--profile",
            profile,
            "--log-dir",
            str(profile_dir),
        ]
        if args.skip_build:
            command.append("--skip-build")
        if args.null_rhi:
            command.append("--null-rhi")

        print(f"[SUITE] running profile={profile} log_dir={profile_dir}")
        completed = run_command(command)
        if completed.stdout:
            print(completed.stdout, end="")
        if completed.stderr:
            print(completed.stderr, end="")

        events_path = profile_dir / "events.jsonl"
        summary = summarize_events(events_path)
        result = {
            "profile": profile,
            "returncode": completed.returncode,
            "log_dir": str(profile_dir),
            "events": str(events_path),
            "summary": summary,
        }
        results.append(result)

        if completed.returncode != 0:
            exit_code = completed.returncode
            break

    suite_summary = {
        "suite_dir": str(suite_dir),
        "profiles": profiles,
        "passed": exit_code == 0,
        "results": results,
    }
    summary_path = suite_dir / "suite_summary.json"
    summary_path.write_text(json.dumps(suite_summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[SUITE] summary={summary_path}")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
