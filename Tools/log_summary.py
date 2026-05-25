#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import json
from collections import Counter
from pathlib import Path
from typing import Any


def event_name(event: dict[str, Any]) -> str:
    value = event.get("event") or event.get("event_type") or "unknown"
    return str(value)


def parse_timestamp(value: object) -> dt.datetime | None:
    if value is None:
        return None
    text = str(value)
    if text.endswith("Z"):
        text = text[:-1] + "+00:00"
    try:
        parsed = dt.datetime.fromisoformat(text)
    except ValueError:
        return None
    if parsed.tzinfo is None:
        return parsed.replace(tzinfo=dt.UTC)
    return parsed.astimezone(dt.UTC)


def as_float(value: object) -> float | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, (int, float)):
        return float(value)
    return None


def summarize_events(path: Path) -> dict[str, Any]:
    counts: Counter[str] = Counter()
    invalid_lines = 0
    first_timestamp: str | None = None
    last_timestamp: str | None = None
    first_timestamp_value: dt.datetime | None = None
    last_timestamp_value: dt.datetime | None = None
    first_elapsed_seconds: float | None = None
    last_elapsed_seconds: float | None = None
    match_started_elapsed_seconds: float | None = None
    match_ended_elapsed_seconds: float | None = None
    match_started_timestamp: dt.datetime | None = None
    match_ended_timestamp: dt.datetime | None = None
    session_metadata: dict[str, Any] | None = None
    run_id: str | None = None
    build_id: str | None = None
    map_id: str | None = None
    profile: str | None = None
    task_repairs = 0
    task_sabotages = 0
    door_locks = 0
    door_unlocks = 0
    door_toggles = 0
    bulkhead_lock_sabotages = 0
    bulkhead_locked_interaction_blocks = 0
    bulkhead_lock_releases = 0
    bulkhead_lock_smoke_passed = 0
    last_bulkhead_lock_smoke: dict[str, Any] | None = None
    flooding_pressure_changes = 0
    flooding_pressure_sabotages = 0
    flooding_pressure_repairs = 0
    last_flooding_pressure: float | None = None
    pump_flooding_smoke_passed = 0
    last_pump_flooding_smoke: dict[str, Any] | None = None
    pve_enemy_spawns = 0
    pve_enemy_attacks = 0
    pve_enemy_smoke_passed = 0
    last_pve_enemy_smoke: dict[str, Any] | None = None
    players_damaged = 0
    pve_spawned_events = 0
    pve_damage_applied = 0
    pve_damage_smoke_passed = 0
    last_pve_damage_smoke: dict[str, Any] | None = None
    item_pickups = 0
    item_drops = 0
    item_drop_smoke_passed = 0
    last_item_drop_smoke: dict[str, Any] | None = None
    combined_systems_smoke_passed = 0
    last_combined_systems_smoke: dict[str, Any] | None = None
    qa_bot_spawns = 0
    qa_bot_moves = 0
    qa_bot_interactions = 0
    qa_bot_smoke_passed = 0
    last_qa_bot_smoke: dict[str, Any] | None = None
    qa_player_bot_moves = 0
    qa_player_bot_interactions = 0
    qa_player_bot_smoke_passed = 0
    last_qa_player_bot_smoke: dict[str, Any] | None = None
    qa_task_bot_moves = 0
    qa_task_bot_interactions = 0
    qa_task_bot_smoke_passed = 0
    last_qa_task_bot_smoke: dict[str, Any] | None = None
    last_role_assignment: dict[str, Any] | None = None
    last_match_result: dict[str, Any] | None = None

    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            stripped = line.strip()
            if not stripped:
                continue

            try:
                event = json.loads(stripped)
            except json.JSONDecodeError:
                invalid_lines += 1
                continue

            name = event_name(event)
            counts[name] += 1
            run_id = run_id or event.get("run_id")
            build_id = build_id or event.get("build_id")
            map_id = map_id or event.get("map_id")
            profile = profile or event.get("profile")
            elapsed_seconds = as_float(event.get("elapsed_seconds"))
            if elapsed_seconds is not None:
                first_elapsed_seconds = elapsed_seconds if first_elapsed_seconds is None else first_elapsed_seconds
                last_elapsed_seconds = elapsed_seconds
            payload = event.get("payload")
            if isinstance(payload, str):
                try:
                    payload = json.loads(payload)
                except json.JSONDecodeError:
                    payload = None

            if name == "session_started" and isinstance(payload, dict):
                session_metadata = payload
                run_id = run_id or payload.get("runId")
                build_id = build_id or payload.get("buildId")
                map_id = map_id or payload.get("mapId")
                profile = profile or payload.get("profile")
            elif name == "match_started":
                match_started_elapsed_seconds = elapsed_seconds
                timestamp = parse_timestamp(event.get("timestamp_utc") or event.get("timestamp"))
                if timestamp is not None:
                    match_started_timestamp = timestamp
            elif name == "match_ended" and isinstance(payload, dict):
                last_match_result = payload
                match_ended_elapsed_seconds = elapsed_seconds
                timestamp = parse_timestamp(event.get("timestamp_utc") or event.get("timestamp"))
                if timestamp is not None:
                    match_ended_timestamp = timestamp
            elif name == "ship_task_applied" and isinstance(payload, dict):
                mode = payload.get("mode")
                if mode == 0:
                    task_repairs += 1
                elif mode == 1:
                    task_sabotages += 1
            elif name == "role_assignment_complete" and isinstance(payload, dict):
                last_role_assignment = payload
            elif name == "door_lock_changed" and isinstance(payload, dict):
                if payload.get("locked") is True:
                    door_locks += 1
                elif payload.get("locked") is False:
                    door_unlocks += 1
            elif name == "door_toggled":
                door_toggles += 1
            elif name == "bulkhead_lock_sabotaged":
                bulkhead_lock_sabotages += 1
            elif name == "bulkhead_locked_interaction_blocked":
                bulkhead_locked_interaction_blocks += 1
            elif name == "bulkhead_lock_released":
                bulkhead_lock_releases += 1
            elif name == "dev_smoke_bulkhead_lock" and isinstance(payload, dict):
                last_bulkhead_lock_smoke = payload
                if payload.get("result") == "pass":
                    bulkhead_lock_smoke_passed += 1
            elif name == "flooding_pressure_changed" and isinstance(payload, dict):
                flooding_pressure_changes += 1
                delta_pressure = payload.get("deltaPressure", payload.get("delta"))
                mode = payload.get("mode")
                if isinstance(delta_pressure, (int, float)) and delta_pressure < 0:
                    flooding_pressure_repairs += 1
                elif isinstance(delta_pressure, (int, float)) and delta_pressure > 0:
                    flooding_pressure_sabotages += 1
                elif mode == 0:
                    flooding_pressure_repairs += 1
                elif mode == 1:
                    flooding_pressure_sabotages += 1
                pressure = payload.get("pressure")
                if isinstance(pressure, (int, float)):
                    last_flooding_pressure = float(pressure)
            elif name == "dev_smoke_pump_flooding" and isinstance(payload, dict):
                last_pump_flooding_smoke = payload
                if payload.get("result") == "pass":
                    pump_flooding_smoke_passed += 1
            elif name == "pve_enemy_spawned":
                pve_enemy_spawns += 1
            elif name == "pve_spawned":
                pve_spawned_events += 1
            elif name == "pve_enemy_attack":
                pve_enemy_attacks += 1
            elif name == "player_damaged":
                players_damaged += 1
            elif name == "pve_damage_applied":
                pve_damage_applied += 1
            elif name == "dev_smoke_pve_enemy" and isinstance(payload, dict):
                last_pve_enemy_smoke = payload
                if payload.get("result") == "pass":
                    pve_enemy_smoke_passed += 1
            elif name == "dev_smoke_pve_damage" and isinstance(payload, dict):
                last_pve_damage_smoke = payload
                if payload.get("result") == "pass":
                    pve_damage_smoke_passed += 1
            elif name == "item_pickup":
                item_pickups += 1
            elif name == "item_dropped":
                item_drops += 1
            elif name == "dev_smoke_item_drop" and isinstance(payload, dict):
                last_item_drop_smoke = payload
                if payload.get("result") == "pass":
                    item_drop_smoke_passed += 1
            elif name == "dev_smoke_combined_systems" and isinstance(payload, dict):
                last_combined_systems_smoke = payload
                if payload.get("result") == "pass":
                    combined_systems_smoke_passed += 1
            elif name == "qa_bot_spawned":
                qa_bot_spawns += 1
            elif name == "qa_bot_moved":
                qa_bot_moves += 1
            elif name == "qa_bot_interacted":
                qa_bot_interactions += 1
            elif name == "dev_smoke_qa_bot" and isinstance(payload, dict):
                last_qa_bot_smoke = payload
                if payload.get("result") == "pass":
                    qa_bot_smoke_passed += 1
            elif name == "qa_player_bot_moved":
                qa_player_bot_moves += 1
            elif name == "qa_player_bot_interacted":
                qa_player_bot_interactions += 1
            elif name == "dev_smoke_qa_player_bot" and isinstance(payload, dict):
                last_qa_player_bot_smoke = payload
                if payload.get("result") == "pass":
                    qa_player_bot_smoke_passed += 1
            elif name == "qa_task_bot_moved":
                qa_task_bot_moves += 1
            elif name == "qa_task_bot_interacted":
                qa_task_bot_interactions += 1
            elif name == "dev_smoke_qa_task_bot" and isinstance(payload, dict):
                last_qa_task_bot_smoke = payload
                if payload.get("result") == "pass":
                    qa_task_bot_smoke_passed += 1

            timestamp = event.get("timestamp_utc") or event.get("timestamp")
            if timestamp is not None:
                timestamp_text = str(timestamp)
                first_timestamp = first_timestamp or timestamp_text
                last_timestamp = timestamp_text
                timestamp_value = parse_timestamp(timestamp)
                if timestamp_value is not None:
                    first_timestamp_value = timestamp_value if first_timestamp_value is None else first_timestamp_value
                    last_timestamp_value = timestamp_value

    duration_seconds: float | None = None
    if first_elapsed_seconds is not None and last_elapsed_seconds is not None:
        duration_seconds = round(max(0.0, last_elapsed_seconds - first_elapsed_seconds), 3)
    elif first_timestamp_value is not None and last_timestamp_value is not None:
        duration_seconds = round(max(0.0, (last_timestamp_value - first_timestamp_value).total_seconds()), 3)

    match_duration_seconds: float | None = None
    if match_started_elapsed_seconds is not None and match_ended_elapsed_seconds is not None:
        match_duration_seconds = round(max(0.0, match_ended_elapsed_seconds - match_started_elapsed_seconds), 3)
    elif match_started_timestamp is not None and match_ended_timestamp is not None:
        match_duration_seconds = round(max(0.0, (match_ended_timestamp - match_started_timestamp).total_seconds()), 3)

    return {
        "source": str(path),
        "run_id": run_id,
        "build_id": build_id,
        "map_id": map_id,
        "profile": profile,
        "session_metadata": session_metadata,
        "total_events": sum(counts.values()),
        "invalid_lines": invalid_lines,
        "first_timestamp": first_timestamp,
        "last_timestamp": last_timestamp,
        "duration_seconds": duration_seconds,
        "match_duration_seconds": match_duration_seconds,
        "first_elapsed_seconds": first_elapsed_seconds,
        "last_elapsed_seconds": last_elapsed_seconds,
        "match_started": counts.get("match_started", 0) + counts.get("round_start", 0),
        "match_ended": counts.get("match_ended", 0) + counts.get("round_end", 0),
        "final_approach_started": counts.get("final_approach_started", 0),
        "players_connected": counts.get("player_connect", 0) + counts.get("client_connected", 0),
        "players_disconnected": counts.get("player_disconnect", 0) + counts.get("client_disconnected", 0),
        "deaths": counts.get("death", 0) + counts.get("player_killed", 0),
        "players_downed": counts.get("player_downed", 0),
        "players_rescued": counts.get("player_rescued", 0),
        "players_contained": counts.get("player_contained", 0),
        "players_released": counts.get("player_released", 0),
        "ship_task_repairs": task_repairs,
        "ship_task_sabotages": task_sabotages,
        "door_locks": door_locks,
        "door_unlocks": door_unlocks,
        "door_toggles": door_toggles,
        "bulkhead_lock_sabotages": bulkhead_lock_sabotages,
        "bulkhead_locked_interaction_blocks": bulkhead_locked_interaction_blocks,
        "bulkhead_lock_releases": bulkhead_lock_releases,
        "bulkhead_lock_smoke_passed": bulkhead_lock_smoke_passed,
        "last_bulkhead_lock_smoke": last_bulkhead_lock_smoke,
        "flooding_pressure_changes": flooding_pressure_changes,
        "flooding_pressure_events": flooding_pressure_changes,
        "flooding_pressure_sabotages": flooding_pressure_sabotages,
        "flooding_pressure_repairs": flooding_pressure_repairs,
        "pump_flooding_sabotages": flooding_pressure_sabotages,
        "pump_flooding_repairs": flooding_pressure_repairs,
        "last_flooding_pressure": last_flooding_pressure,
        "pump_flooding_smoke_passed": pump_flooding_smoke_passed,
        "last_pump_flooding_smoke": last_pump_flooding_smoke,
        "pve_enemy_spawns": pve_enemy_spawns,
        "pve_enemy_attacks": pve_enemy_attacks,
        "pve_enemy_smoke_passed": pve_enemy_smoke_passed,
        "last_pve_enemy_smoke": last_pve_enemy_smoke,
        "players_damaged": players_damaged,
        "pve_spawned": pve_spawned_events,
        "pve_damage_applied": pve_damage_applied,
        "pve_damage_smoke_passed": pve_damage_smoke_passed,
        "last_pve_damage_smoke": last_pve_damage_smoke,
        "item_pickups": item_pickups,
        "item_drops": item_drops,
        "item_drop_smoke_passed": item_drop_smoke_passed,
        "last_item_drop_smoke": last_item_drop_smoke,
        "combined_systems_smoke_passed": combined_systems_smoke_passed,
        "last_combined_systems_smoke": last_combined_systems_smoke,
        "qa_bot_spawns": qa_bot_spawns,
        "qa_bot_moves": qa_bot_moves,
        "qa_bot_interactions": qa_bot_interactions,
        "qa_bot_smoke_passed": qa_bot_smoke_passed,
        "last_qa_bot_smoke": last_qa_bot_smoke,
        "qa_player_bot_moves": qa_player_bot_moves,
        "qa_player_bot_interactions": qa_player_bot_interactions,
        "qa_player_bot_smoke_passed": qa_player_bot_smoke_passed,
        "last_qa_player_bot_smoke": last_qa_player_bot_smoke,
        "qa_task_bot_moves": qa_task_bot_moves,
        "qa_task_bot_interactions": qa_task_bot_interactions,
        "qa_task_bot_smoke_passed": qa_task_bot_smoke_passed,
        "last_qa_task_bot_smoke": last_qa_task_bot_smoke,
        "last_role_assignment": last_role_assignment,
        "last_match_result": last_match_result,
        "errors": counts.get("exception", 0) + counts.get("error", 0),
        "events_by_type": dict(sorted(counts.items())),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize Frostwake JSONL match logs.")
    parser.add_argument("events", type=Path, help="Path to events JSONL file.")
    parser.add_argument("--out", type=Path, help="Optional output JSON path.")
    args = parser.parse_args()

    summary = summarize_events(args.events)
    serialized = json.dumps(summary, indent=2, sort_keys=True)

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(serialized + "\n", encoding="utf-8")
    else:
        print(serialized)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
