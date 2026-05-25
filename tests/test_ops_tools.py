from __future__ import annotations

import json
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

    def test_steam_registry_phase1_returns_empty_listing(self) -> None:
        registry = SteamRegistry()

        self.assertEqual(registry.list_servers(), [])


if __name__ == "__main__":
    unittest.main()
