from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from Tools.log_summary import summarize_events


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "p1_024_human_events.jsonl"


class PlaytestSummaryTests(unittest.TestCase):
    def test_generates_markdown_skeleton_from_summary_json(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            summary_path = temp / "summary.json"
            markdown_path = temp / "summary.md"
            summary_path.write_text(json.dumps(summarize_events(FIXTURE)), encoding="utf-8")

            completed = subprocess.run(
                [
                    sys.executable,
                    "Tools/playtest_summary.py",
                    str(summary_path),
                    "--out",
                    str(markdown_path),
                    "--title",
                    "P1-024 Fixture Summary",
                    "--target-players",
                    "6",
                    "--snapshot",
                    "test-snapshot",
                    "--author",
                    "Test",
                    "--date",
                    "2026-05-25",
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

            self.assertEqual(completed.returncode, 0, completed.stderr)
            markdown = markdown_path.read_text(encoding="utf-8")

        self.assertIn("# P1-024 Fixture Summary", markdown)
        self.assertIn("RunId: P1-024-run-01", markdown)
        self.assertIn("Actual players: 6", markdown)
        self.assertIn("| `players_connected` | 6 |", markdown)
        self.assertIn("| `ship_task_repairs` | 1 |", markdown)
        self.assertIn("## P1-024 Decision", markdown)

    def test_rejects_raw_event_json_as_summary_input(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            raw_path = Path(temp_dir) / "raw.json"
            raw_path.write_text(json.dumps({"event": "match_started", "payload": {}}), encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, "Tools/playtest_summary.py", str(raw_path)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("raw event record", completed.stderr + completed.stdout)


if __name__ == "__main__":
    unittest.main()
