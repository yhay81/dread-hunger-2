#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import unreal


MAP_PACKAGE = "/Game/Maps/L_IcebreakerWhitebox"
MAP_ASSET = "/Game/Maps/L_IcebreakerWhitebox.L_IcebreakerWhitebox"
ROOT = Path(__file__).resolve().parents[2]

REQUIRED_LABELS = {
    "WB_Hull_MainDeck",
    "WB_Bridge",
    "WB_RadioRoom",
    "WB_Infirmary",
    "WB_QuartermasterStore",
    "WB_EngineRoom",
    "WB_FuelBay",
    "WB_BatteryRoom",
    "WB_IcePressureGate",
    "TP_RouteControl",
    "TP_RadioRepair",
    "TP_FuelContamination",
    "TP_PowerRepair",
    "TP_EngineRepair",
    "TP_HullFlooding",
    "TASK_Repair_Radio",
    "TASK_Sabotage_Radio",
    "TASK_Repair_Power",
    "TASK_Sabotage_Power",
    "TASK_Repair_Engine",
    "TASK_Sabotage_Fuel",
    "TASK_Repair_HullFlooding",
    "TASK_Sabotage_HullFlooding",
    "TASK_Repair_Route",
    "DOOR_RadioBulkhead",
    "DOOR_EngineBulkhead",
    "DOOR_HoldFloodBulkhead",
}

TASK_EXPECTATIONS = {
    "TASK_Repair_Radio": ("radio", "repair"),
    "TASK_Sabotage_Radio": ("radio", "sabotage"),
    "TASK_Repair_Power": ("power", "repair"),
    "TASK_Sabotage_Power": ("power", "sabotage"),
    "TASK_Repair_Engine": ("engine", "repair"),
    "TASK_Sabotage_Fuel": ("fuel", "sabotage"),
    "TASK_Repair_HullFlooding": ("flooding", "repair"),
    "TASK_Sabotage_HullFlooding": ("flooding", "sabotage"),
    "TASK_Repair_Route": ("route", "repair"),
}


def log(message: str) -> None:
    unreal.log(f"[FrostwakeWhiteboxValidation] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[FrostwakeWhiteboxValidation] {message}")
    raise RuntimeError(message)


def load_map() -> None:
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE):
        fail(f"Missing map asset: {MAP_PACKAGE}")

    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PACKAGE)
    if not world:
        fail(f"Could not load map: {MAP_PACKAGE}")


def actor_labels() -> set[str]:
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor_subsystem:
        fail("EditorActorSubsystem is unavailable.")

    return {actor.get_actor_label() for actor in actor_subsystem.get_all_level_actors()}


def all_level_actors() -> list:
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor_subsystem:
        fail("EditorActorSubsystem is unavailable.")

    return list(actor_subsystem.get_all_level_actors())


def validate_player_starts(labels: set[str]) -> None:
    starts = sorted(label for label in labels if label.startswith("PS_Crew_"))
    if len(starts) != 8:
        fail(f"Expected 8 crew player starts, found {len(starts)}: {starts}")

    expected = {f"PS_Crew_{index:02d}" for index in range(1, 9)}
    missing = sorted(expected - set(starts))
    if missing:
        fail(f"Missing crew player starts: {missing}")


def validate_required_labels(labels: set[str]) -> None:
    missing = sorted(REQUIRED_LABELS - labels)
    if missing:
        fail(f"Missing required whitebox labels: {missing}")


def validate_default_maps() -> None:
    config_path = ROOT / "Config" / "DefaultEngine.ini"
    if not config_path.exists():
        fail(f"Missing DefaultEngine.ini: {config_path}")

    values: dict[str, str] = {}
    for raw_line in config_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("[") or "=" not in line:
            continue

        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()

    if values.get("EditorStartupMap") != MAP_ASSET:
        fail(f"EditorStartupMap is not {MAP_ASSET}: {values.get('EditorStartupMap')}")
    if values.get("GameDefaultMap") != MAP_ASSET:
        fail(f"GameDefaultMap is not {MAP_ASSET}: {values.get('GameDefaultMap')}")


def validate_task_config(actors: list) -> None:
    actors_by_label = {actor.get_actor_label(): actor for actor in actors}
    for label, (expected_system, expected_mode) in TASK_EXPECTATIONS.items():
        actor = actors_by_label.get(label)
        if not actor:
            fail(f"Missing task actor: {label}")

        get_system = getattr(actor, "get_target_system", None)
        get_mode = getattr(actor, "get_task_mode", None)
        if not get_system or not get_mode:
            fail(f"{label} is not an AbyssShipTaskActor with expected accessors.")

        system_value = str(get_system()).lower()
        mode_value = str(get_mode()).lower()
        if expected_system not in system_value:
            fail(f"{label} target system is not {expected_system}: {system_value}")
        if expected_mode not in mode_value:
            fail(f"{label} task mode is not {expected_mode}: {mode_value}")


def validate_door_config(actors: list) -> None:
    actors_by_label = {actor.get_actor_label(): actor for actor in actors}
    for label in ("DOOR_RadioBulkhead", "DOOR_EngineBulkhead", "DOOR_HoldFloodBulkhead"):
        actor = actors_by_label.get(label)
        if not actor:
            fail(f"Missing door actor: {label}")

        class_name = actor.get_class().get_name()
        if "AbyssDoorActor" not in class_name:
            fail(f"{label} is not an AbyssDoorActor: {class_name}")


def main() -> int:
    load_map()
    actors = all_level_actors()
    labels = {actor.get_actor_label() for actor in actors}
    validate_player_starts(labels)
    validate_required_labels(labels)
    validate_default_maps()
    validate_task_config(actors)
    validate_door_config(actors)
    log(f"{MAP_PACKAGE} passed validation with {len(labels)} actors.")
    return 0


if __name__ == "__main__":
    main()
