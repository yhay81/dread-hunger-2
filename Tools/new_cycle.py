#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CYCLE_DIR = ROOT / "docs" / "cycles"
CYCLE_PATTERN = re.compile(r"^\d{4}-\d{2}-\d{2}-cycle-(\d+)\.md$")


def next_cycle_number() -> int:
    numbers: list[int] = []
    for path in CYCLE_DIR.glob("*.md"):
        match = CYCLE_PATTERN.match(path.name)
        if match:
            numbers.append(int(match.group(1)))
    return max(numbers, default=-1) + 1


def build_cycle_doc(cycle: int, phase: str, owner: str, goal: str, gate: str) -> str:
    date = datetime.now().strftime("%Y-%m-%d")
    return f"""# Cycle {cycle}: {goal}

## Header

- Cycle: {cycle}
- Date: {date}
- Phase: {phase}
- Owner: {owner}
- Goal: {goal}
- Gate: {gate}

## Planned Scope

-

## Files Changed

-

## Verification

| Check | Result | Notes |
| --- | --- | --- |
| `python3 Tools/quality_gate.py` | not run |  |
| Unreal build | not available |  |
| Multiplayer smoke test | not run |  |
| Bot/client run | not run |  |
| Human playtest | not run |  |

## Evidence

-

## Decisions

| Label | Decision | Reason | Next Action |
| --- | --- | --- | --- |
| blocker |  |  |  |
| keep |  |  |  |
| change |  |  |  |
| cut |  |  |  |
| defer |  |  |  |

## Backlog Updates

-

## Next Cycle

- Goal:
- First action:
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Create the next Frostwake cycle record.")
    parser.add_argument("--phase", default="0", help="Current phase label.")
    parser.add_argument("--owner", default="Codex", help="Cycle owner.")
    parser.add_argument("--goal", required=True, help="Cycle goal.")
    parser.add_argument("--gate", required=True, help="Measurable gate for the cycle.")
    args = parser.parse_args()

    CYCLE_DIR.mkdir(parents=True, exist_ok=True)
    cycle = next_cycle_number()
    date = datetime.now().strftime("%Y-%m-%d")
    slug = f"{date}-cycle-{cycle}.md"
    path = CYCLE_DIR / slug
    path.write_text(build_cycle_doc(cycle, args.phase, args.owner, args.goal, args.gate), encoding="utf-8")
    print(path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
