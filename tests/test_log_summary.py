from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from Tools.log_summary import summarize_events


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "p1_024_human_events.jsonl"


class LogSummaryTests(unittest.TestCase):
    def test_summarizes_human_playtest_fixture(self) -> None:
        summary = summarize_events(FIXTURE)

        self.assertEqual(summary["run_id"], "P1-024-run-01")
        self.assertEqual(summary["build_id"], "AbyssLock-Win64-Development-local")
        self.assertEqual(summary["map_id"], "/Game/Maps/L_IcebreakerWhitebox")
        self.assertEqual(summary["profile"], "human-p1-024")
        self.assertEqual(summary["players_connected"], 6)
        self.assertEqual(summary["players_disconnected"], 1)
        self.assertEqual(summary["match_started"], 1)
        self.assertEqual(summary["match_ended"], 1)
        self.assertEqual(summary["match_timer_started"], 0)
        self.assertEqual(summary["match_timer_expired"], 0)
        self.assertEqual(summary["ship_task_repairs"], 1)
        self.assertEqual(summary["ship_task_sabotages"], 1)
        self.assertEqual(summary["door_toggles"], 1)
        self.assertEqual(summary["players_downed"], 1)
        self.assertEqual(summary["players_rescued"], 1)
        self.assertEqual(summary["invalid_lines"], 0)
        self.assertEqual(summary["last_role_assignment"], {"players": 6, "saboteurs": 1})
        self.assertEqual(summary["last_match_result"], {"winner": "crew", "reason": "route_complete"})
        self.assertEqual(summary["duration_seconds"], 182.0)
        self.assertEqual(summary["match_duration_seconds"], 160.0)

    def test_counts_invalid_jsonl_without_dropping_valid_events(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "events.jsonl"
            path.write_text(
                "\n".join(
                    [
                        json.dumps({"event": "match_started", "elapsed_seconds": 1.0}),
                        "{not json",
                        json.dumps({"event": "match_ended", "elapsed_seconds": 3.5, "payload": {"winner": "crew"}}),
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            summary = summarize_events(path)

        self.assertEqual(summary["invalid_lines"], 1)
        self.assertEqual(summary["match_started"], 1)
        self.assertEqual(summary["match_ended"], 1)
        self.assertEqual(summary["match_duration_seconds"], 2.5)


if __name__ == "__main__":
    unittest.main()
