#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.quality_gate import validate_schema_value


DEFAULT_SCHEMA = ROOT / "Tools" / "ops" / "lobby_metadata.schema.json"
RAW_IP_ENDPOINT_PATTERN = re.compile(r"\b\d{1,3}(?:\.\d{1,3}){3}:\d{2,5}\b")


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit(f"{path}: expected JSON object")
    return value


def validate_lobby_metadata(
    schema: dict[str, Any],
    metadata: dict[str, Any],
    *,
    expected_build_id: str | None = None,
    expected_map_id: str | None = None,
) -> list[str]:
    errors = validate_schema_value(schema, metadata, "$")
    if errors:
        return errors

    max_players = int(metadata["maxPlayers"])
    current_players = int(metadata["currentPlayers"])
    join_state = str(metadata["joinState"])
    endpoint_token = str(metadata["endpointToken"])

    if current_players > max_players:
        errors.append("$.currentPlayers: cannot exceed maxPlayers")
    if join_state == "open" and current_players >= max_players:
        errors.append("$.joinState: open lobby cannot already be full")
    if join_state == "full" and current_players < max_players:
        errors.append("$.joinState: full lobby must have currentPlayers >= maxPlayers")
    if RAW_IP_ENDPOINT_PATTERN.search(endpoint_token):
        errors.append("$.endpointToken: must be opaque, not a raw IP:port endpoint")
    if expected_build_id and metadata["buildId"] != expected_build_id:
        errors.append(f"$.buildId: mismatch expected {expected_build_id!r} got {metadata['buildId']!r}")
    if expected_map_id and metadata["mapId"] != expected_map_id:
        errors.append(f"$.mapId: mismatch expected {expected_map_id!r} got {metadata['mapId']!r}")

    return errors


def summarize(metadata: dict[str, Any]) -> dict[str, Any]:
    return {
        "schemaVersion": metadata["schemaVersion"],
        "buildId": metadata["buildId"],
        "mapId": metadata["mapId"],
        "ruleset": metadata["ruleset"],
        "players": f"{metadata['currentPlayers']}/{metadata['maxPlayers']}",
        "joinState": metadata["joinState"],
        "connectionMode": metadata["connectionMode"],
        "official": metadata["official"],
        "passworded": metadata["passworded"],
        "region": metadata.get("region"),
        "serverName": metadata.get("serverName"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Frostwake Steam Lobby metadata without calling Steam APIs.")
    parser.add_argument("metadata", help="Lobby metadata JSON path to validate.")
    parser.add_argument("--schema", default=str(DEFAULT_SCHEMA), help="Schema JSON path.")
    parser.add_argument("--expected-build-id", help="Reject metadata that does not match this build id.")
    parser.add_argument("--expected-map-id", help="Reject metadata that does not match this map id.")
    parser.add_argument("--json", action="store_true", help="Print a machine-readable summary on success.")
    args = parser.parse_args()

    metadata_path = Path(args.metadata)
    if not metadata_path.is_absolute():
        metadata_path = ROOT / metadata_path
    schema_path = Path(args.schema)
    if not schema_path.is_absolute():
        schema_path = ROOT / schema_path

    schema = load_json(schema_path)
    metadata = load_json(metadata_path)
    errors = validate_lobby_metadata(
        schema,
        metadata,
        expected_build_id=args.expected_build_id,
        expected_map_id=args.expected_map_id,
    )
    if errors:
        for error in errors:
            print(f"[FAIL] {error}", file=sys.stderr)
        return 1

    summary = summarize(metadata)
    if args.json:
        print(json.dumps(summary, indent=2, sort_keys=True))
    else:
        print(
            "[PASS] lobby_metadata "
            f"build={summary['buildId']} "
            f"map={summary['mapId']} "
            f"players={summary['players']} "
            f"joinState={summary['joinState']} "
            f"mode={summary['connectionMode']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
