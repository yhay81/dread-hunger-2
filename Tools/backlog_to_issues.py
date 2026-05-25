#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT_DIR = ROOT / "docs" / "issue-import"
ROW_PATTERN = re.compile(r"^\|\s*([A-Z]+\d+-\d{3})\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|$")
SECTION_PATTERN = re.compile(r"^##\s+(.+?)\s*$")


def default_backlog_path(phase: int) -> Path:
    return ROOT / "docs" / f"phase{phase}-backlog.md"


def slug_label(value: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")
    return slug or "unsorted"


@dataclass(frozen=True)
class IssueRow:
    issue_id: str
    section: str
    task: str
    done_when: str
    phase: int
    source_path: Path
    milestone: str
    base_label: str

    @property
    def title(self) -> str:
        return f"{self.issue_id}: {self.task}"

    @property
    def labels(self) -> str:
        return f"phase-{self.phase},{slug_label(self.section)},{self.base_label}"

    @property
    def body(self) -> str:
        source = self.source_path.relative_to(ROOT).as_posix()
        return "\n".join(
            [
                f"Source: `{source}`",
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


def parse_backlog(
    path: Path,
    *,
    phase: int,
    id_prefix: str,
    milestone: str,
    base_label: str,
) -> list[IssueRow]:
    rows: list[IssueRow] = []
    current_section = milestone
    for line in path.read_text(encoding="utf-8").splitlines():
        section_match = SECTION_PATTERN.match(line)
        if section_match:
            current_section = section_match.group(1)
            continue

        row_match = ROW_PATTERN.match(line)
        if not row_match:
            continue
        issue_id, task, done_when = row_match.groups()
        if not issue_id.startswith(id_prefix):
            continue
        rows.append(
            IssueRow(
                issue_id=issue_id,
                section=current_section,
                task=task,
                done_when=done_when,
                phase=phase,
                source_path=path,
                milestone=milestone,
                base_label=base_label,
            )
        )
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
                    "milestone": row.milestone,
                }
            )


def write_markdown(rows: list[IssueRow], path: Path, *, phase: int, source_path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    source = source_path.relative_to(ROOT).as_posix()
    lines = [
        f"# Phase {phase} Issue Import",
        "",
        f"Generated from `{source}`.",
        "",
        "| ID | Section | Title | Labels |",
        "| --- | --- | --- | --- |",
    ]
    for row in rows:
        lines.append(f"| {row.issue_id} | {row.section} | {row.task} | `{row.labels}` |")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Export phase backlog rows as GitHub issue import files.")
    parser.add_argument("--phase", type=int, default=1, help="Phase number used for default paths, labels, and milestone.")
    parser.add_argument("--backlog", help="Backlog Markdown path. Defaults to docs/phaseN-backlog.md.")
    parser.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR), help="Output directory.")
    parser.add_argument("--id-prefix", help="Issue id prefix. Defaults to PN- for the selected phase.")
    parser.add_argument("--milestone", help="GitHub milestone name. Defaults to Phase N.")
    parser.add_argument("--base-label", default="prototype", help="Additional base label added to every exported issue.")
    args = parser.parse_args()

    backlog = Path(args.backlog) if args.backlog else default_backlog_path(args.phase)
    if not backlog.is_absolute():
        backlog = ROOT / backlog
    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = ROOT / out_dir

    id_prefix = args.id_prefix or f"P{args.phase}-"
    milestone = args.milestone or f"Phase {args.phase}"
    rows = parse_backlog(
        backlog,
        phase=args.phase,
        id_prefix=id_prefix,
        milestone=milestone,
        base_label=args.base_label,
    )
    if not rows:
        raise SystemExit(f"No {id_prefix} issue rows found in {backlog}")

    write_csv(rows, out_dir / f"phase{args.phase}-issues.csv")
    write_markdown(rows, out_dir / f"phase{args.phase}-issues.md", phase=args.phase, source_path=backlog)
    print(f"Exported {len(rows)} issues to {out_dir.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
