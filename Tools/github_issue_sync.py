#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import shlex
import subprocess
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CSV = ROOT / "docs" / "issue-import" / "phase1-issues.csv"


@dataclass(frozen=True)
class IssueSpec:
    title: str
    body: str
    labels: tuple[str, ...]
    milestone: str


def load_issue_specs(path: Path) -> list[IssueSpec]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        specs: list[IssueSpec] = []
        for row in reader:
            title = (row.get("title") or "").strip()
            body = (row.get("body") or "").strip()
            labels = tuple(label.strip() for label in (row.get("labels") or "").split(",") if label.strip())
            milestone = (row.get("milestone") or "").strip()
            if not title:
                continue
            specs.append(IssueSpec(title=title, body=body, labels=labels, milestone=milestone))
    return specs


def build_gh_issue_create_command(spec: IssueSpec) -> list[str]:
    command = ["gh", "issue", "create", "--title", spec.title, "--body", spec.body]
    for label in spec.labels:
        command.extend(["--label", label])
    if spec.milestone:
        command.extend(["--milestone", spec.milestone])
    return command


def shell_command(command: list[str]) -> str:
    return " ".join(shlex.quote(part) for part in command)


def existing_issue_titles() -> set[str]:
    completed = subprocess.run(
        ["gh", "issue", "list", "--state", "all", "--limit", "1000", "--json", "title"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(completed.stderr.strip() or completed.stdout.strip() or "gh issue list failed")
    payload = json.loads(completed.stdout)
    return {str(item.get("title", "")).strip() for item in payload if item.get("title")}


def filter_new_specs(specs: list[IssueSpec], existing_titles: set[str]) -> list[IssueSpec]:
    return [spec for spec in specs if spec.title not in existing_titles]


def main() -> int:
    parser = argparse.ArgumentParser(description="Dry-run or create GitHub issues from docs/issue-import CSV.")
    parser.add_argument("--csv", type=Path, default=DEFAULT_CSV, help="Issue import CSV path.")
    parser.add_argument("--check-existing", action="store_true", help="Skip titles that already exist in GitHub issues.")
    parser.add_argument("--create", action="store_true", help="Actually run gh issue create. Default only prints commands.")
    parser.add_argument("--json", action="store_true", help="Print planned commands as JSON instead of shell commands.")
    parser.add_argument("--limit", type=int, help="Limit number of issues processed after duplicate filtering.")
    args = parser.parse_args()

    specs = load_issue_specs(args.csv if args.csv.is_absolute() else ROOT / args.csv)
    if args.check_existing:
        specs = filter_new_specs(specs, existing_issue_titles())
    if args.limit is not None:
        specs = specs[: max(args.limit, 0)]

    commands = [build_gh_issue_create_command(spec) for spec in specs]
    if args.json:
        print(json.dumps(commands, indent=2, ensure_ascii=False))
    else:
        for command in commands:
            print(shell_command(command))

    if not args.create:
        print(f"# dry-run: {len(commands)} issue command(s). Add --create to run them.")
        return 0

    for command in commands:
        completed = subprocess.run(command, cwd=ROOT, text=True, check=False)
        if completed.returncode != 0:
            return completed.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
