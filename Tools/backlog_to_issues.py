#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BACKLOG = ROOT / "docs" / "phase1-backlog.md"
DEFAULT_OUT_DIR = ROOT / "docs" / "issue-import"
ROW_PATTERN = re.compile(r"^\|\s*(P1-\d{3})\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|$")
WEEK_PATTERN = re.compile(r"^##\s+(Week\s+\d+|Milestone Checkpoint)$")


@dataclass(frozen=True)
class IssueRow:
    issue_id: str
    week: str
    task: str
    done_when: str

    @property
    def title(self) -> str:
        return f"{self.issue_id}: {self.task}"

    @property
    def labels(self) -> str:
        label_week = self.week.lower().replace(" ", "-")
        return f"phase-1,{label_week},prototype"

    @property
    def body(self) -> str:
        return "\n".join(
            [
                f"Source: `docs/phase1-backlog.md`",
                "",
                "Done when:",
                "",
                self.done_when,
                "",
                "Validation evidence:",
                "",
                "- [ ] Command output or cycle link added",
                "- [ ] Result recorded in a cycle file",
                "- [ ] No raw logs, recordings, or personal identifiers committed",
            ]
        )


def parse_backlog(path: Path) -> list[IssueRow]:
    rows: list[IssueRow] = []
    current_week = "Phase 1"
    for line in path.read_text(encoding="utf-8").splitlines():
        week_match = WEEK_PATTERN.match(line)
        if week_match:
            current_week = week_match.group(1)
            continue

        row_match = ROW_PATTERN.match(line)
        if not row_match:
            continue
        issue_id, task, done_when = row_match.groups()
        rows.append(IssueRow(issue_id=issue_id, week=current_week, task=task, done_when=done_when))
    return rows


def write_csv(rows: list[IssueRow], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["title", "body", "labels", "milestone"], lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow(
                {
                    "title": row.title,
                    "body": row.body,
                    "labels": row.labels,
                    "milestone": "Phase 1",
                }
            )


def write_markdown(rows: list[IssueRow], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Phase 1 Issue Import",
        "",
        "Generated from `docs/phase1-backlog.md`.",
        "",
        "| ID | Week | Title | Labels |",
        "| --- | --- | --- | --- |",
    ]
    for row in rows:
        lines.append(f"| {row.issue_id} | {row.week} | {row.task} | `{row.labels}` |")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Export Phase 1 backlog rows as GitHub issue import files.")
    parser.add_argument("--backlog", default=str(DEFAULT_BACKLOG), help="Backlog Markdown path.")
    parser.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR), help="Output directory.")
    args = parser.parse_args()

    backlog = Path(args.backlog)
    if not backlog.is_absolute():
        backlog = ROOT / backlog
    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = ROOT / out_dir

    rows = parse_backlog(backlog)
    if not rows:
        raise SystemExit(f"No P1 issue rows found in {backlog}")

    write_csv(rows, out_dir / "phase1-issues.csv")
    write_markdown(rows, out_dir / "phase1-issues.md")
    print(f"Exported {len(rows)} issues to {out_dir.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
