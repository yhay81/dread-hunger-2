from __future__ import annotations

import unittest

from Tools.playtest_run_scaffold import after_test_ps1, commands_md, host_ps1, preflight_ps1


class PlaytestRunScaffoldTests(unittest.TestCase):
    def test_preflight_script_is_windows_first(self) -> None:
        script = preflight_ps1(r"C:\Program Files\Epic Games\UE_5.7")

        self.assertIn("--platform Win64", script)
        self.assertIn("--include-server", script)
        self.assertIn("--profile ready8", script)
        self.assertIn("Tools\\quality_gate.py --require-ue", script)

    def test_host_script_includes_telemetry_identity(self) -> None:
        script = host_ps1(
            run_id="P1-024-run-01",
            slug="run-01",
            target_players=6,
            port=7777,
            build_id="AbyssLock-Win64-Development-local",
            map_id="/Game/Maps/L_IcebreakerWhitebox",
            profile="human-p1-024",
            windows_ue_root=r"C:\Program Files\Epic Games\UE_5.7",
            res_x=1280,
            res_y=720,
        )

        self.assertIn("-AbyssRunId=P1-024-run-01", script)
        self.assertIn("-AbyssBuildId=AbyssLock-Win64-Development-local", script)
        self.assertIn("-AbyssMapId=/Game/Maps/L_IcebreakerWhitebox", script)
        self.assertIn("-AbyssProfile=human-p1-024", script)
        self.assertIn("-AbyssEventLog=$RepoRoot\\Saved\\Playtests\\P1-024\\run-01\\events.jsonl", script)
        self.assertIn("-AbyssLobbyMinPlayers=6", script)

    def test_after_test_script_runs_summary_and_preflight(self) -> None:
        script = after_test_ps1(slug="run-01", run_id="P1-024-run-01")

        self.assertIn("Tools\\log_summary.py", script)
        self.assertIn("Tools\\playtest_summary.py", script)
        self.assertIn("Tools\\playtest_preflight.py", script)
        self.assertIn("docs\\playtests\\p1-024-run-01-summary.md", script)

    def test_command_card_documents_windows_manual_equivalent(self) -> None:
        text = commands_md(
            run_id="P1-024-run-01",
            slug="run-01",
            target_players=6,
            port=7777,
            build_id="AbyssLock-Win64-Development-local",
            map_id="/Game/Maps/L_IcebreakerWhitebox",
            profile="human-p1-024",
            windows_ue_root=r"C:\Program Files\Epic Games\UE_5.7",
            res_x=1280,
            res_y=720,
        )

        self.assertIn("Host Preflight", text)
        self.assertIn("--platform Win64", text)
        self.assertIn(".\\Saved\\Playtests\\P1-024\\run-01\\host.ps1", text)
        self.assertIn("-AbyssRunId=P1-024-run-01", text)


if __name__ == "__main__":
    unittest.main()
