from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from Tools.ops.banlist import BanEntry, load_banlist, save_banlist
from Tools.ops.redaction import redact_mapping
from Tools.ops.steam_registry import SteamRegistry
from Tools.quality_gate import validate_schema_value


ROOT = Path(__file__).resolve().parents[1]


class OpsToolTests(unittest.TestCase):
    def test_redaction_recurses_into_nested_mappings_and_lists(self) -> None:
        redacted = redact_mapping(
            {
                "server": "local",
                "token": "secret-token",
                "nested": {
                    "steam_id": "76561190000000000",
                    "notes": "keep me",
                },
                "reports": [
                    {"ip": "127.0.0.1", "summary": "keep summary"},
                    {"chat_account_id": "abc123"},
                ],
            }
        )

        self.assertEqual(redacted["token"], "[REDACTED]")
        self.assertEqual(redacted["nested"]["steam_id"], "[REDACTED]")
        self.assertEqual(redacted["nested"]["notes"], "keep me")
        self.assertEqual(redacted["reports"][0]["ip"], "[REDACTED]")
        self.assertEqual(redacted["reports"][0]["summary"], "keep summary")
        self.assertEqual(redacted["reports"][1]["chat_account_id"], "[REDACTED]")

    def test_banlist_roundtrip(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "banlist.json"
            entry = BanEntry(
                platform_user_id="hash-local-1",
                scope="private_test",
                reason="voice_abuse",
                created_at="2026-05-25T00:00:00Z",
            )

            self.assertEqual(load_banlist(path), [])
            save_banlist(path, [entry])
            loaded = load_banlist(path)

        self.assertEqual(loaded, [entry])

    def test_server_config_example_matches_schema(self) -> None:
        schema = json.loads((ROOT / "Tools" / "ops" / "server_config.schema.json").read_text(encoding="utf-8"))
        example = json.loads((ROOT / "Tools" / "ops" / "server_config.example.json").read_text(encoding="utf-8"))

        self.assertEqual(validate_schema_value(schema, example, "$"), [])

    def test_server_config_check_cli_accepts_example(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                "Tools/ops/server_config_check.py",
                "Tools/ops/server_config.example.json",
                "--json",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        summary = json.loads(completed.stdout)
        self.assertEqual(summary["maxPlayers"], 8)
        self.assertEqual(summary["map"], "L_IcebreakerWhitebox")
        self.assertTrue(summary["hasAdminTokenEnv"])

    def test_server_config_check_cli_rejects_invalid_config(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "server_config.json"
            path.write_text(
                json.dumps(
                    {
                        "serverName": "Bad",
                        "region": "jp",
                        "maxPlayers": 12,
                        "map": "L_IcebreakerWhitebox",
                        "ruleset": "private_test",
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [sys.executable, "Tools/ops/server_config_check.py", str(path)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("maxPlayers", completed.stderr)

    def test_lobby_metadata_example_matches_schema(self) -> None:
        schema = json.loads((ROOT / "Tools" / "ops" / "lobby_metadata.schema.json").read_text(encoding="utf-8"))
        example = json.loads((ROOT / "Tools" / "ops" / "lobby_metadata.example.json").read_text(encoding="utf-8"))

        self.assertEqual(validate_schema_value(schema, example, "$"), [])

    def test_lobby_metadata_check_cli_accepts_example(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                "Tools/ops/lobby_metadata_check.py",
                "Tools/ops/lobby_metadata.example.json",
                "--expected-build-id",
                "AbyssLock-Win64-Development-local",
                "--expected-map-id",
                "L_IcebreakerWhitebox",
                "--json",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        summary = json.loads(completed.stdout)
        self.assertEqual(summary["players"], "3/8")
        self.assertEqual(summary["joinState"], "open")

    def test_lobby_metadata_check_cli_rejects_build_mismatch(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                "Tools/ops/lobby_metadata_check.py",
                "Tools/ops/lobby_metadata.example.json",
                "--expected-build-id",
                "DifferentBuild",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("buildId", completed.stderr)
        self.assertIn("mismatch", completed.stderr)

    def test_lobby_metadata_check_cli_rejects_raw_ip_endpoint(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "lobby_metadata.json"
            metadata = json.loads((ROOT / "Tools" / "ops" / "lobby_metadata.example.json").read_text(encoding="utf-8"))
            metadata["endpointToken"] = "127.0.0.1:7777"
            path.write_text(json.dumps(metadata), encoding="utf-8")

            completed = subprocess.run(
                [sys.executable, "Tools/ops/lobby_metadata_check.py", str(path)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("endpointToken", completed.stderr)

    def test_lobby_metadata_check_cli_rejects_inconsistent_player_counts(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "lobby_metadata.json"
            metadata = json.loads((ROOT / "Tools" / "ops" / "lobby_metadata.example.json").read_text(encoding="utf-8"))
            metadata["currentPlayers"] = 8
            metadata["joinState"] = "open"
            path.write_text(json.dumps(metadata), encoding="utf-8")

            completed = subprocess.run(
                [sys.executable, "Tools/ops/lobby_metadata_check.py", str(path)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("open lobby cannot already be full", completed.stderr)

    def test_steam_registry_phase1_returns_empty_listing(self) -> None:
        registry = SteamRegistry()

        self.assertEqual(registry.list_servers(), [])


if __name__ == "__main__":
    unittest.main()
