#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.quality_gate import validate_schema_value


DEFAULT_SCHEMA = ROOT / "Tools" / "ops" / "server_config.schema.json"


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit(f"{path}: expected JSON object")
    return value


def summarize(config: dict[str, Any]) -> dict[str, Any]:
    return {
        "serverName": config.get("serverName"),
        "region": config.get("region"),
        "maxPlayers": config.get("maxPlayers"),
        "map": config.get("map"),
        "ruleset": config.get("ruleset"),
        "advertise": config.get("advertise"),
        "autoShutdownAfterMatch": config.get("autoShutdownAfterMatch"),
        "hasAdminTokenEnv": bool(config.get("adminTokenEnv")),
        "banlistPath": config.get("banlistPath"),
        "logPath": config.get("logPath"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a Frostwake dedicated-server config JSON file.")
    parser.add_argument("config", help="Config JSON path to validate.")
    parser.add_argument("--schema", default=str(DEFAULT_SCHEMA), help="Schema JSON path.")
    parser.add_argument("--json", action="store_true", help="Print a machine-readable summary on success.")
    args = parser.parse_args()

    config_path = Path(args.config)
    if not config_path.is_absolute():
        config_path = ROOT / config_path
    schema_path = Path(args.schema)
    if not schema_path.is_absolute():
        schema_path = ROOT / schema_path

    schema = load_json(schema_path)
    config = load_json(config_path)
    errors = validate_schema_value(schema, config, "$")
    if errors:
        for error in errors:
            print(f"[FAIL] {error}", file=sys.stderr)
        return 1

    summary = summarize(config)
    if args.json:
        print(json.dumps(summary, indent=2, sort_keys=True))
    else:
        print(
            "[PASS] server_config "
            f"name={summary['serverName']} "
            f"region={summary['region']} "
            f"maxPlayers={summary['maxPlayers']} "
            f"map={summary['map']} "
            f"ruleset={summary['ruleset']} "
            f"advertise={summary['advertise']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
