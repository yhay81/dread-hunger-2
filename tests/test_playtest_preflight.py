from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
import uuid
from pathlib import Path

from Tools.log_summary import summarize_events


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "p1_024_human_events.jsonl"


def resolved_markdown(summary: dict[str, object]) -> str:
    return f"""# P1-024 Resolved Fixture Summary

## Header

```text
RunId: {summary["run_id"]}
Date: 2026-05-25
Local snapshot or commit: test-snapshot
Build: {summary["build_id"]}
Map: {summary["map_id"]}
Profile: {summary["profile"]}
Host type: listen-server
Target players: 6
Actual players: {summary["players_connected"]}
Voice method: game-native voice placeholder
Recording consent: yes
Raw evidence path: Saved/Playtests/P1-024/run-01
Summary author: Test
```

## Telemetry Summary

Fixture telemetry reviewed.

## Technical Result

All objective gates passed in fixture data.

## Validity Gate

Validity result:

```text
pass
```

## Observed Flow

Players connected, roles assigned, repairs and sabotage occurred, and the match ended.

## Player Feedback

Aggregate fixture feedback: players understood the loop and wanted another round.

## Keep / Cut / Change

Keep: repair and sabotage pressure.

Cut: no fixture cuts.

Change: improve UI labels.

## Mechanics Decision Table

| Area | Decision |
| --- | --- |
| Repairs | keep |
| Sabotage | keep |

## Top Blockers

| Rank | Blocker | Severity | Evidence | Proposed fix | Owner | Retest gate |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | None from fixture | low | fixture | continue | Codex | rerun |

## P1-024 Decision

Decision:

```text
P1-024 result: pass
Reason: fixture satisfies objective gates
Next backlog item: P1-025
```

## Data Handling Check

- [x] Raw logs are not committed.
- [x] Raw recordings are not committed.
- [x] Tester names are removed.
- [x] Voice transcripts are not included.
- [x] IP addresses, SteamIDs, and local usernames are removed.
- [x] Any moderation-sensitive details are summarized without identifying people.
- [x] Summary is stored in `docs/playtests/` only after anonymization.
"""


class PlaytestPreflightTests(unittest.TestCase):
    def test_accepts_resolved_human_summary(self) -> None:
        summary = summarize_events(FIXTURE)
        summary["source"] = "Saved/Playtests/P1-024/run-01/events.jsonl"
        with tempfile.TemporaryDirectory() as temp_dir:
            summary_path = Path(temp_dir) / "Saved" / "Playtests" / "P1-024" / "run-01" / "summary.json"
            summary_path.parent.mkdir(parents=True)
            summary_path.write_text(json.dumps(summary), encoding="utf-8")

            markdown_path = ROOT / "docs" / "playtests" / f"p1-024-test-{uuid.uuid4().hex}-summary.md"
            try:
                markdown_path.write_text(resolved_markdown(summary), encoding="utf-8")
                completed = subprocess.run(
                    [
                        sys.executable,
                        "Tools/playtest_preflight.py",
                        str(summary_path),
                        "--markdown",
                        str(markdown_path),
                        "--min-players",
                        "6",
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
            finally:
                markdown_path.unlink(missing_ok=True)

        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
        self.assertIn("[PASS] decision_resolved", completed.stdout)
        self.assertIn("[PASS] no_unresolved_placeholders", completed.stdout)

    def test_rejects_generated_placeholder_summary_as_unresolved(self) -> None:
        summary = summarize_events(FIXTURE)
        summary["source"] = "Saved/Playtests/P1-024/run-01/events.jsonl"
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            summary_path = temp / "Saved" / "Playtests" / "P1-024" / "run-01" / "summary.json"
            generated_markdown = ROOT / "docs" / "playtests" / f"p1-024-test-{uuid.uuid4().hex}-summary.md"
            summary_path.parent.mkdir(parents=True)
            summary_path.write_text(json.dumps(summary), encoding="utf-8")
            subprocess.run(
                [
                    sys.executable,
                    "Tools/playtest_summary.py",
                    str(summary_path),
                    "--out",
                    str(generated_markdown),
                    "--target-players",
                    "6",
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=True,
            )
            try:
                completed = subprocess.run(
                    [
                        sys.executable,
                        "Tools/playtest_preflight.py",
                        str(summary_path),
                        "--markdown",
                        str(generated_markdown),
                        "--min-players",
                        "6",
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
            finally:
                generated_markdown.unlink(missing_ok=True)

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("no_unresolved_placeholders", completed.stdout)


if __name__ == "__main__":
    unittest.main()
