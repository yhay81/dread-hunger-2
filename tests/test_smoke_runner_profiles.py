from __future__ import annotations

import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class SmokeRunnerProfileTests(unittest.TestCase):
    def test_life_action_profile_describes_expected_launch_shape(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                "Tools/ue/run_local_smoke.py",
                "--profile",
                "life-action",
                "--describe-profile",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        profile = json.loads(completed.stdout)
        self.assertEqual(profile["clients"], 4)
        self.assertEqual(profile["expected_players"], 5)
        self.assertEqual(profile["expected_saboteurs"], 1)
        self.assertTrue(profile["auto_ready"])
        self.assertTrue(profile["smoke_life_action"])

    def test_smoke_runner_has_no_legacy_editor_constant_reference(self) -> None:
        source = (ROOT / "Tools" / "ue" / "run_local_smoke.py").read_text(encoding="utf-8")

        self.assertNotIn("UNREAL_EDITOR", source)


if __name__ == "__main__":
    unittest.main()
