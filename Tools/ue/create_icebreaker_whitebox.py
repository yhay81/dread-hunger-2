#!/usr/bin/env python3
from __future__ import annotations

import unreal


MAP_PATH = "/Game/Maps/L_IcebreakerWhitebox"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"


def log(message: str) -> None:
    unreal.log(f"[FrostwakeWhitebox] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[FrostwakeWhitebox] {message}")
    raise RuntimeError(message)


def vector(x: float, y: float, z: float) -> unreal.Vector:
    return unreal.Vector(float(x), float(y), float(z))


def rotator(pitch: float = 0.0, yaw: float = 0.0, roll: float = 0.0) -> unreal.Rotator:
    return unreal.Rotator(float(pitch), float(yaw), float(roll))


def get_actor_subsystem() -> unreal.EditorActorSubsystem:
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not subsystem:
        fail("EditorActorSubsystem is unavailable.")
    return subsystem


def spawn_actor(actor_class, label: str, location: unreal.Vector, rotation: unreal.Rotator | None = None):
    rotation = rotation or rotator()
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
    if not actor:
        fail(f"Could not spawn actor {label}.")
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [unreal.Name("FrostwakeWhitebox")])
    return actor


def spawn_cube(label: str, location: unreal.Vector, size: unreal.Vector):
    cube_mesh = unreal.load_asset(CUBE_PATH)
    if not cube_mesh:
        fail(f"Could not load {CUBE_PATH}.")

    actor = spawn_actor(unreal.StaticMeshActor, label, location)
    actor.set_actor_scale3d(vector(size.x / 100.0, size.y / 100.0, size.z / 100.0))
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        fail(f"StaticMeshComponent missing on {label}.")
    component.set_static_mesh(cube_mesh)
    return actor


def spawn_player_start(index: int, location: unreal.Vector, yaw: float) -> None:
    actor = spawn_actor(unreal.PlayerStart, f"PS_Crew_{index:02d}", location, rotator(0.0, yaw, 0.0))
    actor.set_editor_property("player_start_tag", unreal.Name(f"Crew{index:02d}"))


def spawn_target(label: str, location: unreal.Vector) -> None:
    spawn_actor(unreal.TargetPoint, label, location)


def spawn_ship_task(
    label: str,
    system: str,
    is_sabotage: bool,
    location: unreal.Vector,
    delta: float,
    single_use: bool,
    saboteur_only: bool,
    set_offline: bool,
) -> None:
    task_class = getattr(unreal, "AbyssShipTaskActor", None)
    if not task_class:
        fail("AbyssShipTaskActor is unavailable. Build AbyssLockEditor after adding the C++ class.")

    actor = spawn_actor(task_class, label, location)
    configure = getattr(actor, "configure_task_by_name", None)
    if not configure:
        fail("AbyssShipTaskActor.configure_task_by_name is unavailable.")

    if not configure(unreal.Name(system), is_sabotage, float(delta), single_use, saboteur_only, set_offline):
        fail(f"Could not configure ship task {label} for system {system}.")


def spawn_bulkhead_door(label: str, door_id: str, location: unreal.Vector, yaw: float) -> None:
    door_class = getattr(unreal, "AbyssDoorActor", None)
    if not door_class:
        fail("AbyssDoorActor is unavailable. Build AbyssLockEditor after adding the C++ class.")

    actor = spawn_actor(door_class, label, location, rotator(0.0, yaw, 0.0))
    configure = getattr(actor, "configure_door_by_name", None)
    if configure:
        configure(unreal.Name(door_id))


def is_whitebox_actor(actor) -> bool:
    label = actor.get_actor_label()
    if label.startswith(("WB_", "PS_Crew_", "TP_", "TASK_", "DOOR_", "L_Key_", "L_Sky_")):
        return True
    tags = [str(tag) for tag in actor.get_editor_property("tags")]
    return "FrostwakeWhitebox" in tags


def clear_existing_whitebox() -> None:
    actor_subsystem = get_actor_subsystem()
    existing = [actor for actor in actor_subsystem.get_all_level_actors() if is_whitebox_actor(actor)]
    if not existing:
        return

    log(f"Removing {len(existing)} existing whitebox actors before regeneration.")
    if not actor_subsystem.destroy_actors(existing):
        fail("Could not remove existing whitebox actors.")


def create_map() -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log(f"{MAP_PATH} already exists; loading before regeneration.")
        world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
        if not world:
            fail(f"Could not load existing map {MAP_PATH}.")
    else:
        level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if not level_editor:
            fail("LevelEditorSubsystem is unavailable.")

        try:
            created = level_editor.new_level(MAP_PATH, False)
        except TypeError:
            created = level_editor.new_level(MAP_PATH)
        if not created:
            fail(f"Could not create map {MAP_PATH}.")

    clear_existing_whitebox()

    # Overall coordinate convention: X runs stern to bow, Y port to starboard, Z up.
    spawn_cube("WB_Hull_MainDeck", vector(0, 0, -20), vector(4200, 1350, 40))
    spawn_cube("WB_Hull_LowerServiceDeck", vector(-250, 0, -260), vector(2900, 950, 40))
    spawn_cube("WB_Foredeck_IceWorkArea", vector(1650, 0, 20), vector(900, 1200, 40))
    spawn_cube("WB_AftDeck_MusterArea", vector(-1750, 0, 20), vector(700, 1050, 40))

    spawn_cube("WB_PortHullWall", vector(0, -700, 150), vector(4200, 60, 340))
    spawn_cube("WB_StarboardHullWall", vector(0, 700, 150), vector(4200, 60, 340))
    spawn_cube("WB_BowBulkhead", vector(2120, 0, 150), vector(60, 1350, 340))
    spawn_cube("WB_SternBulkhead", vector(-2120, 0, 150), vector(60, 1350, 340))

    rooms = [
        ("WB_Bridge", vector(1100, 0, 320), vector(620, 520, 260)),
        ("WB_RadioRoom", vector(450, -380, 170), vector(520, 360, 260)),
        ("WB_Infirmary", vector(450, 380, 170), vector(520, 360, 260)),
        ("WB_Wardroom", vector(-250, 380, 170), vector(560, 360, 260)),
        ("WB_QuartermasterStore", vector(-250, -380, 170), vector(560, 360, 260)),
        ("WB_EngineRoom", vector(-1050, 0, 170), vector(720, 640, 300)),
        ("WB_FuelBay", vector(-1550, -360, 170), vector(520, 380, 260)),
        ("WB_BatteryRoom", vector(-1550, 360, 170), vector(520, 380, 260)),
        ("WB_Hold", vector(850, 0, -120), vector(700, 760, 220)),
    ]
    for label, location, size in rooms:
        spawn_cube(label, location, size)

    corridors = [
        ("WB_CentralCompanionway", vector(-150, 0, 40), vector(2100, 280, 80)),
        ("WB_PortSidePassage", vector(-400, -520, 40), vector(2600, 180, 80)),
        ("WB_StarboardSidePassage", vector(-400, 520, 40), vector(2600, 180, 80)),
        ("WB_BridgeLadder", vector(900, 0, 160), vector(220, 220, 280)),
        ("WB_LowerDeckLadder", vector(100, 0, -120), vector(220, 220, 260)),
    ]
    for label, location, size in corridors:
        spawn_cube(label, location, size)

    obstacles = [
        ("WB_IcePressureGate", vector(1980, 0, 110), vector(90, 1100, 180)),
        ("WB_FuelLine", vector(-1120, -560, 90), vector(1120, 80, 100)),
        ("WB_PumpManifold", vector(-960, 560, 90), vector(780, 80, 100)),
        ("WB_RadioMastBase", vector(650, -520, 240), vector(160, 160, 360)),
        ("WB_DeckWinch", vector(1570, 420, 120), vector(360, 220, 180)),
        ("WB_CargoCrates", vector(1350, -420, 120), vector(420, 260, 180)),
    ]
    for label, location, size in obstacles:
        spawn_cube(label, location, size)

    player_starts = [
        (1, vector(-1800, -320, 120), 15),
        (2, vector(-1800, 0, 120), 0),
        (3, vector(-1800, 320, 120), -15),
        (4, vector(-1350, -510, 120), 35),
        (5, vector(-1350, 510, 120), -35),
        (6, vector(-650, -520, 120), 60),
        (7, vector(-650, 520, 120), -60),
        (8, vector(-80, 0, 120), 0),
    ]
    for index, location, yaw in player_starts:
        spawn_player_start(index, location, yaw)

    targets = [
        ("TP_RouteControl", vector(1140, 0, 500)),
        ("TP_RadioRepair", vector(450, -380, 330)),
        ("TP_MedicalRescue", vector(450, 380, 330)),
        ("TP_FuelContamination", vector(-1550, -360, 330)),
        ("TP_PowerRepair", vector(-1550, 360, 330)),
        ("TP_EngineRepair", vector(-1050, 0, 360)),
        ("TP_IcePressureObjective", vector(1980, 0, 260)),
        ("TP_HullFlooding", vector(850, 0, 40)),
    ]
    for label, location in targets:
        spawn_target(label, location)

    tasks = [
        ("TASK_Repair_Radio", "Radio", False, vector(450, -220, 150), 0.40, False, False, False),
        ("TASK_Sabotage_Radio", "Radio", True, vector(450, -540, 150), 0.35, False, True, True),
        ("TASK_Repair_Power", "Power", False, vector(-1450, 420, 150), 0.40, False, False, False),
        ("TASK_Sabotage_Power", "Power", True, vector(-1450, 560, 150), 0.35, False, True, True),
        ("TASK_Repair_Engine", "Engine", False, vector(-1050, -160, 150), 0.30, False, False, False),
        ("TASK_Sabotage_Fuel", "Fuel", True, vector(-1550, -520, 150), 0.35, False, True, False),
        ("TASK_Repair_HullFlooding", "Flooding", False, vector(820, 180, 70), 0.35, False, False, False),
        ("TASK_Sabotage_HullFlooding", "Flooding", True, vector(820, -180, 70), 0.35, False, True, False),
        ("TASK_Repair_Route", "Route", False, vector(1140, 180, 440), 0.35, False, False, False),
    ]
    for label, system, is_sabotage, location, delta, single_use, saboteur_only, set_offline in tasks:
        spawn_ship_task(label, system, is_sabotage, location, delta, single_use, saboteur_only, set_offline)

    bulkhead_doors = [
        ("DOOR_RadioBulkhead", "RadioBulkhead", vector(170, -380, 100), 0),
        ("DOOR_EngineBulkhead", "EngineBulkhead", vector(-650, 0, 100), 90),
        ("DOOR_HoldFloodBulkhead", "HoldFloodBulkhead", vector(520, 0, -80), 90),
    ]
    for label, door_id, location, yaw in bulkhead_doors:
        spawn_bulkhead_door(label, door_id, location, yaw)

    spawn_actor(unreal.DirectionalLight, "L_Key_ArcticSun", vector(-900, -1200, 2200), rotator(-38, -35, 0))
    spawn_actor(unreal.SkyLight, "L_Sky_Overcast", vector(0, 0, 1200))

    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if not editor_subsystem:
        fail("UnrealEditorSubsystem is unavailable.")
    world = editor_subsystem.get_editor_world()
    if not world:
        fail("Could not resolve editor world after whitebox creation.")
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        fail(f"Could not save map {MAP_PATH}.")
    log(f"Created and saved {MAP_PATH}.")


def main() -> int:
    try:
        create_map()
        return 0
    finally:
        try:
            unreal.SystemLibrary.quit_editor()
        except Exception as exc:  # noqa: BLE001 - UE shutdown API availability varies.
            unreal.log_warning(f"[FrostwakeWhitebox] quit_editor failed: {exc}")


if __name__ == "__main__":
    main()
