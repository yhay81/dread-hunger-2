#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import platform
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "AbyssLock.uproject"
DEFAULT_UE_VERSION = "UE_5.7"
DEFAULT_MAP = "/Game/Maps/L_IcebreakerWhitebox"
SMOKE_PROFILES = (
    "ready5",
    "ready6",
    "ready8",
    "combined5",
    "combined8",
    "qa-bot",
    "qa-player-bot",
    "qa-task-bot",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch a local Frostwake listen-server smoke test.")
    parser.add_argument("--profile", choices=SMOKE_PROFILES, help="Named smoke profile that fills the common long argument set.")
    parser.add_argument("--describe-profile", action="store_true", help="Print the effective profile settings and exit before launching Unreal.")
    parser.add_argument("--clients", type=int, default=0, help="Number of localhost clients to launch after the listen host.")
    parser.add_argument("--duration", type=int, default=45, help="Seconds to keep processes running after launch.")
    parser.add_argument("--startup-timeout", type=int, default=45, help="Seconds to wait for host startup evidence.")
    parser.add_argument("--match-timeout", type=int, default=45, help="Seconds to wait for dev auto-start after clients launch.")
    parser.add_argument("--client-launch-spacing", type=float, default=3.0, help="Seconds to wait between localhost client launches.")
    parser.add_argument("--auto-start-min-players", type=int, default=None, help="Override dev auto-start player threshold.")
    parser.add_argument("--expected-players", type=int, default=None, help="Validate the last role assignment player count.")
    parser.add_argument("--expected-saboteurs", type=int, default=None, help="Validate the last role assignment saboteur count.")
    parser.add_argument("--map", default=DEFAULT_MAP, help="Map package to open on the listen host.")
    parser.add_argument("--port", type=int, default=7777, help="Listen server port.")
    parser.add_argument("--log-dir", type=Path, default=None, help="Output directory for smoke-test logs.")
    parser.add_argument("--run-id", help="Telemetry run id. Defaults to the log directory name.")
    parser.add_argument("--build-id", default="local-dev", help="Telemetry build id.")
    parser.add_argument("--skip-build", action="store_true", help="Skip the Editor build before launching.")
    parser.add_argument("--ue-root", help="Override Unreal Engine root. Defaults to UE_ROOT/UNREAL_ENGINE_ROOT or common install paths.")
    parser.add_argument("--platform", default=host_platform(), help="Unreal build platform, usually Win64, Mac, or Linux.")
    parser.add_argument("--null-rhi", action="store_true", help="Run without rendering. Useful for CI-style smoke tests.")
    parser.add_argument("--windowed", action="store_true", help="Open visible game windows.")
    parser.add_argument("--host-only", action="store_true", help="Alias for --clients 0.")
    parser.add_argument("--smoke-interact", action="store_true", help="Run a dev-only server-side task interaction smoke after auto-start.")
    parser.add_argument("--smoke-down-rescue", action="store_true", help="Run a dev-only server-side down/rescue smoke after match start.")
    parser.add_argument("--smoke-containment", action="store_true", help="Run a dev-only server-side contain/release smoke after match start.")
    parser.add_argument("--smoke-route-complete", action="store_true", help="Run a dev-only route objective completion smoke after match start.")
    parser.add_argument("--smoke-fatal-sabotage", action="store_true", help="Run a dev-only fatal sabotage smoke after match start.")
    parser.add_argument("--smoke-bulkhead-lock", action="store_true", help="Run a dev-only bulkhead lock/unlock smoke after match start.")
    parser.add_argument("--smoke-pump-flooding", action="store_true", help="Run a dev-only pump/flooding pressure smoke after match start.")
    parser.add_argument("--smoke-pve-enemy", action="store_true", help="Run a dev-only PvE enemy spawn/damage smoke after match start.")
    parser.add_argument("--smoke-pve-damage", action="store_true", help="Alias for --smoke-pve-enemy.")
    parser.add_argument("--smoke-item-drop", action="store_true", help="Run a dev-only item add/drop/re-pickup smoke after match start.")
    parser.add_argument("--smoke-combined-systems", action="store_true", help="Run a dev-only non-terminal combined systems smoke after match start.")
    parser.add_argument("--smoke-qa-bot", action="store_true", help="Run a dev-only QA bot spawn/move/interact smoke after match start.")
    parser.add_argument("--smoke-qa-player-bot", action="store_true", help="Run a dev-only PlayerState-backed QA bot door interaction smoke after match start.")
    parser.add_argument("--smoke-qa-task-bot", action="store_true", help="Run a dev-only PlayerState-backed QA bot ship-task interaction smoke after match start.")
    parser.add_argument("--no-auto-start", action="store_true", help="Do not pass -AbyssAutoStart; useful for lobby ready smoke tests.")
    parser.add_argument("--auto-ready", action="store_true", help="Pass a dev-only flag that marks players ready after login.")
    parser.add_argument("--lobby-min-players", type=int, default=None, help="Dev-only override for the ready-lobby start threshold.")
    parser.add_argument("--lobby-dev-saboteurs", type=int, default=None, help="Dev-only ready-lobby saboteur count override.")
    return parser.parse_args()


def apply_profile(args: argparse.Namespace) -> None:
    if not args.profile:
        return

    profiles = {
        "ready5": {
            "clients": 4,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 5,
            "expected_players": 5,
            "expected_saboteurs": 1,
            "duration": 5,
            "startup_timeout": 60,
            "match_timeout": 120,
            "client_launch_spacing": 1.5,
        },
        "ready6": {
            "clients": 5,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 6,
            "expected_players": 6,
            "expected_saboteurs": 1,
            "duration": 5,
            "startup_timeout": 75,
            "match_timeout": 180,
            "client_launch_spacing": 1.25,
        },
        "ready8": {
            "clients": 7,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 8,
            "expected_players": 8,
            "expected_saboteurs": 2,
            "duration": 5,
            "startup_timeout": 90,
            "match_timeout": 240,
            "client_launch_spacing": 1.25,
        },
        "combined5": {
            "clients": 4,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 5,
            "expected_players": 5,
            "expected_saboteurs": 1,
            "smoke_combined_systems": True,
            "duration": 5,
            "startup_timeout": 60,
            "match_timeout": 120,
            "client_launch_spacing": 1.5,
        },
        "combined8": {
            "clients": 7,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 8,
            "expected_players": 8,
            "expected_saboteurs": 2,
            "smoke_combined_systems": True,
            "duration": 5,
            "startup_timeout": 90,
            "match_timeout": 240,
            "client_launch_spacing": 1.25,
        },
        "qa-bot": {
            "clients": 0,
            "host_only": True,
            "smoke_qa_bot": True,
            "duration": 5,
            "startup_timeout": 60,
            "match_timeout": 120,
        },
        "qa-player-bot": {
            "clients": 4,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 5,
            "expected_players": 5,
            "expected_saboteurs": 1,
            "smoke_qa_player_bot": True,
            "duration": 5,
            "startup_timeout": 60,
            "match_timeout": 120,
            "client_launch_spacing": 1.5,
        },
        "qa-task-bot": {
            "clients": 4,
            "no_auto_start": True,
            "auto_ready": True,
            "lobby_min_players": 5,
            "expected_players": 5,
            "expected_saboteurs": 1,
            "smoke_qa_task_bot": True,
            "duration": 5,
            "startup_timeout": 60,
            "match_timeout": 120,
            "client_launch_spacing": 1.5,
        },
    }

    for key, value in profiles[args.profile].items():
        setattr(args, key, value)


def describe_effective_settings(args: argparse.Namespace) -> dict[str, object]:
    keys = (
        "profile",
        "clients",
        "duration",
        "startup_timeout",
        "match_timeout",
        "client_launch_spacing",
        "auto_start_min_players",
        "expected_players",
        "expected_saboteurs",
        "map",
        "run_id",
        "build_id",
        "host_only",
        "smoke_interact",
        "smoke_down_rescue",
        "smoke_containment",
        "smoke_route_complete",
        "smoke_fatal_sabotage",
        "smoke_bulkhead_lock",
        "smoke_pump_flooding",
        "smoke_pve_enemy",
        "smoke_pve_damage",
        "smoke_item_drop",
        "smoke_combined_systems",
        "smoke_qa_bot",
        "smoke_qa_player_bot",
        "smoke_qa_task_bot",
        "no_auto_start",
        "auto_ready",
        "lobby_min_players",
        "lobby_dev_saboteurs",
    )
    return {key: getattr(args, key) for key in keys}


def host_platform() -> str:
    system = platform.system()
    if system == "Windows":
        return "Win64"
    if system == "Darwin":
        return "Mac"
    if system == "Linux":
        return "Linux"
    return system


def candidate_ue_roots() -> list[Path]:
    candidates: list[Path] = []
    for env_name in ("UE_ROOT", "UNREAL_ENGINE_ROOT"):
        value = os.environ.get(env_name)
        if value:
            candidates.append(Path(value))

    system = platform.system()
    if system == "Windows":
        for base in (
            os.environ.get("ProgramFiles"),
            os.environ.get("ProgramW6432"),
            r"C:\Program Files",
        ):
            if base:
                candidates.append(Path(base) / "Epic Games" / DEFAULT_UE_VERSION)
    elif system == "Darwin":
        shared = Path("/Users/Shared/Epic Games")
        candidates.append(shared / DEFAULT_UE_VERSION)
        if shared.exists():
            candidates.extend(sorted(shared.glob("UE_*"), reverse=True))
    else:
        candidates.extend((Path.home() / "Epic Games" / DEFAULT_UE_VERSION, Path("/opt") / DEFAULT_UE_VERSION))

    deduped: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate)
        if key not in seen:
            deduped.append(candidate)
            seen.add(key)
    return deduped


def resolve_ue_root(override: str | None) -> Path:
    if override:
        return Path(override)
    for candidate in candidate_ue_roots():
        if candidate.exists():
            return candidate
    return candidate_ue_roots()[0]


def unreal_editor_path(ue_root: Path, build_platform: str) -> Path:
    if build_platform == "Win64":
        return ue_root / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"
    if build_platform == "Mac":
        return ue_root / "Engine" / "Binaries" / "Mac" / "UnrealEditor"
    if build_platform == "Linux":
        return ue_root / "Engine" / "Binaries" / "Linux" / "UnrealEditor"
    return ue_root / "Engine" / "Binaries" / build_platform / "UnrealEditor"


def build_script(ue_root: Path, build_platform: str) -> Path:
    if build_platform == "Win64":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Build.bat"
    if build_platform == "Mac":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "Build.sh"
    if build_platform == "Linux":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Linux" / "Build.sh"
    return ue_root / "Engine" / "Build" / "BatchFiles" / "Build.sh"


def build_editor(ue_root: Path, build_platform: str) -> None:
    build = build_script(ue_root, build_platform)
    command = [
        str(build),
        "AbyssLockEditor",
        build_platform,
        "Development",
        f"-Project={PROJECT}",
        "-WaitMutex",
    ]
    completed = subprocess.run(command, cwd=ROOT, text=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError(f"Editor build failed with exit code {completed.returncode}.")


def make_log_dir(value: Path | None) -> Path:
    if value:
        path = value if value.is_absolute() else ROOT / value
    else:
        stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
        path = ROOT / "Saved" / "SmokeTests" / f"local-{stamp}"

    path.mkdir(parents=True, exist_ok=True)
    return path


def common_args(args: argparse.Namespace) -> list[str]:
    values = [
        "-game",
        "-stdout",
        "-FullStdOutLogOutput",
        "-Unattended",
        "-NoSound",
        "-NoSplash",
        "-NoLiveCoding",
        "-nop4",
    ]
    if args.null_rhi:
        values.append("-nullrhi")
    if args.windowed:
        values.extend(["-windowed", "-ResX=960", "-ResY=540"])
    return values


def open_process(name: str, command: list[str], log_path: Path) -> subprocess.Popen:
    handle = log_path.open("w", encoding="utf-8")
    handle.write("$ " + " ".join(command) + "\n\n")
    handle.flush()
    process = subprocess.Popen(
        command,
        cwd=ROOT,
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
    )
    process._abyss_log_handle = handle  # type: ignore[attr-defined]
    print(f"[START] {name}: pid={process.pid} log={log_path}")
    return process


def close_process(process: subprocess.Popen, timeout: float = 10.0) -> None:
    if process.poll() is not None:
        close_log_handle(process)
        return

    process.terminate()
    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=timeout)
    finally:
        close_log_handle(process)


def close_log_handle(process: subprocess.Popen) -> None:
    handle = getattr(process, "_abyss_log_handle", None)
    if handle:
        handle.close()


def log_contains(path: Path, needles: tuple[str, ...]) -> bool:
    if not path.exists():
        return False
    text = path.read_text(encoding="utf-8", errors="ignore")
    return any(needle in text for needle in needles)


def log_has_failure(path: Path) -> bool:
    if not path.exists():
        return False
    text = path.read_text(encoding="utf-8", errors="ignore")
    failure_terms = ("Fatal error", "Unhandled Exception", "Critical error", "appError called")
    return any(term in text for term in failure_terms)


def wait_for_log(path: Path, needles: tuple[str, ...], timeout_seconds: int) -> bool:
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        if log_contains(path, needles):
            return True
        time.sleep(1.0)
    return False


def last_role_assignment(events_log: Path) -> dict | None:
    if not events_log.exists():
        return None

    latest: dict | None = None
    with events_log.open("r", encoding="utf-8") as handle:
        for line in handle:
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            if event.get("event") == "role_assignment_complete" and isinstance(event.get("payload"), dict):
                latest = event["payload"]
    return latest


def main() -> int:
    args = parse_args()
    apply_profile(args)
    if args.host_only:
        args.clients = 0
    if args.describe_profile:
        print(json.dumps(describe_effective_settings(args), indent=2, sort_keys=True))
        return 0

    if args.clients < 0:
        raise SystemExit("--clients must be >= 0")
    if not PROJECT.exists():
        raise SystemExit(f"Missing project: {PROJECT}")
    ue_root = resolve_ue_root(args.ue_root)
    unreal_editor = unreal_editor_path(ue_root, args.platform)
    if not unreal_editor.exists():
        raise SystemExit(f"Missing UnrealEditor: {unreal_editor}")

    if not args.skip_build:
        build_editor(ue_root, args.platform)

    log_dir = make_log_dir(args.log_dir)
    run_id = args.run_id or log_dir.name
    profile = args.profile or "custom"
    common = common_args(args)
    auto_start_min_players = args.auto_start_min_players
    if auto_start_min_players is None:
        auto_start_min_players = args.clients + 1 if args.clients > 0 else 1

    host_log = log_dir / "listen_host.log"
    events_log = log_dir / "events.jsonl"
    host_map = f"{args.map}?listen"
    host_command = [
        str(unreal_editor),
        str(PROJECT),
        host_map,
        *common,
        f"-port={args.port}",
        f"-AbyssEventLog={events_log}",
        f"-AbyssRunId={run_id}",
        f"-AbyssBuildId={args.build_id}",
        f"-AbyssMapId={args.map}",
        f"-AbyssProfile={profile}",
    ]
    if not args.no_auto_start:
        host_command.extend(
            [
                "-AbyssAutoStart",
                f"-AbyssAutoStartMinPlayers={auto_start_min_players}",
                "-AbyssDevSaboteurs=1",
            ]
        )
    if args.smoke_interact:
        host_command.append("-AbyssSmokeInteract")
    if args.smoke_down_rescue:
        host_command.append("-AbyssSmokeDownRescue")
    if args.smoke_containment:
        host_command.append("-AbyssSmokeContainment")
    if args.smoke_route_complete:
        host_command.append("-AbyssSmokeRouteComplete")
    if args.smoke_fatal_sabotage:
        host_command.append("-AbyssSmokeFatalSabotage")
    if args.smoke_bulkhead_lock:
        host_command.append("-AbyssSmokeBulkheadLock")
    if args.smoke_pump_flooding:
        host_command.append("-AbyssSmokePumpFlooding")
    if args.smoke_pve_enemy or args.smoke_pve_damage:
        host_command.append("-AbyssSmokePveEnemy")
    if args.smoke_pve_damage:
        host_command.append("-AbyssSmokePvEDamage")
    if args.smoke_item_drop:
        host_command.append("-AbyssSmokeItemDrop")
    if args.smoke_combined_systems:
        host_command.append("-AbyssSmokeCombinedSystems")
    if args.smoke_qa_bot:
        host_command.append("-AbyssSmokeQaBot")
    if args.smoke_qa_player_bot:
        host_command.append("-AbyssSmokeQaPlayerBot")
    if args.smoke_qa_task_bot:
        host_command.append("-AbyssSmokeQaTaskBot")
    if args.auto_ready:
        host_command.append("-AbyssAutoReady")
    if args.lobby_min_players is not None:
        host_command.append(f"-AbyssLobbyMinPlayers={args.lobby_min_players}")
    if args.lobby_dev_saboteurs is not None:
        host_command.append(f"-AbyssLobbyDevSaboteurs={args.lobby_dev_saboteurs}")

    processes: list[subprocess.Popen] = []
    try:
        host = open_process("listen_host", host_command, host_log)
        processes.append(host)

        host_ready = wait_for_log(
            host_log,
            (
                "IpNetDriver listening on port",
                f"listening on port {args.port}",
                "Load map complete /Game/Maps/L_IcebreakerWhitebox",
            ),
            args.startup_timeout,
        )
        if not host_ready or host.poll() is not None:
            print(f"[FAIL] listen host did not reach startup evidence within {args.startup_timeout}s")
            return 10

        for index in range(1, args.clients + 1):
            client_log = log_dir / f"client_{index:02d}.log"
            client_command = [
                str(UNREAL_EDITOR),
                str(PROJECT),
                f"127.0.0.1:{args.port}",
                *common,
                f"-port={args.port + index}",
            ]
            client = open_process(f"client_{index:02d}", client_command, client_log)
            processes.append(client)
            time.sleep(args.client_launch_spacing)

        match_started = wait_for_log(host_log, ("dev_auto_start", "role_assignment_complete"), args.match_timeout)
        if not match_started:
            print(f"[FAIL] match did not auto-start within {args.match_timeout}s")
            return 13

        if args.smoke_interact:
            task_interacted = wait_for_log(host_log, ("dev_smoke_task_interaction", "ship_task_applied"), args.match_timeout)
            if not task_interacted:
                print(f"[FAIL] task smoke interaction did not complete within {args.match_timeout}s")
                return 14
        if args.smoke_down_rescue:
            down_rescued = wait_for_log(host_log, ("dev_smoke_down_rescue", "player_rescued"), args.match_timeout)
            if not down_rescued:
                print(f"[FAIL] down/rescue smoke did not complete within {args.match_timeout}s")
                return 15
        if args.smoke_containment:
            contained = wait_for_log(host_log, ("dev_smoke_containment", "player_released"), args.match_timeout)
            if not contained:
                print(f"[FAIL] containment smoke did not complete within {args.match_timeout}s")
                return 17
        if args.smoke_route_complete:
            route_completed = wait_for_log(host_log, ("dev_smoke_route_complete", "final_approach_started"), args.match_timeout)
            if not route_completed:
                print(f"[FAIL] route completion smoke did not complete within {args.match_timeout}s")
                return 18
        if args.smoke_fatal_sabotage:
            fatal_sabotage = wait_for_log(host_log, ("dev_smoke_fatal_sabotage", "fatal_ship_state"), args.match_timeout)
            if not fatal_sabotage:
                print(f"[FAIL] fatal sabotage smoke did not complete within {args.match_timeout}s")
                return 19
        if args.smoke_bulkhead_lock:
            bulkhead_lock = wait_for_log(host_log, ("dev_smoke_bulkhead_lock result=pass",), args.match_timeout)
            if not bulkhead_lock:
                print(f"[FAIL] bulkhead lock smoke did not complete within {args.match_timeout}s")
                return 21
        if args.smoke_pump_flooding:
            pump_flooding = wait_for_log(host_log, ("dev_smoke_pump_flooding result=pass",), args.match_timeout)
            if not pump_flooding:
                print(f"[FAIL] pump/flooding smoke did not complete within {args.match_timeout}s")
                return 22
        if args.smoke_pve_enemy or args.smoke_pve_damage:
            pve_enemy = wait_for_log(host_log, ("dev_smoke_pve_enemy result=pass",), args.match_timeout)
            if not pve_enemy:
                print(f"[FAIL] PvE enemy smoke did not complete within {args.match_timeout}s")
                return 23
        if args.smoke_item_drop:
            item_drop = wait_for_log(host_log, ("dev_smoke_item_drop result=pass",), args.match_timeout)
            if not item_drop:
                print(f"[FAIL] item drop smoke did not complete within {args.match_timeout}s")
                return 24
        if args.smoke_combined_systems:
            combined_systems = wait_for_log(host_log, ("dev_smoke_combined_systems result=pass",), args.match_timeout)
            if not combined_systems:
                print(f"[FAIL] combined systems smoke did not complete within {args.match_timeout}s")
                return 25
        if args.smoke_qa_bot:
            qa_bot = wait_for_log(host_log, ("dev_smoke_qa_bot result=pass",), args.match_timeout)
            if not qa_bot:
                print(f"[FAIL] QA bot smoke did not complete within {args.match_timeout}s")
                return 26
        if args.smoke_qa_player_bot:
            qa_player_bot = wait_for_log(host_log, ("dev_smoke_qa_player_bot result=pass",), args.match_timeout)
            if not qa_player_bot:
                print(f"[FAIL] QA player bot smoke did not complete within {args.match_timeout}s")
                return 27
        if args.smoke_qa_task_bot:
            qa_task_bot = wait_for_log(host_log, ("dev_smoke_qa_task_bot result=pass",), args.match_timeout)
            if not qa_task_bot:
                print(f"[FAIL] QA task bot smoke did not complete within {args.match_timeout}s")
                return 28

        if args.expected_players is not None or args.expected_saboteurs is not None:
            assignment = last_role_assignment(events_log)
            if not assignment:
                print("[FAIL] no role_assignment_complete event found in events.jsonl")
                return 16
            if args.expected_players is not None and assignment.get("players") != args.expected_players:
                print(f"[FAIL] expected players={args.expected_players}, got {assignment.get('players')}")
                return 16
            if args.expected_saboteurs is not None and assignment.get("saboteurs") != args.expected_saboteurs:
                print(f"[FAIL] expected saboteurs={args.expected_saboteurs}, got {assignment.get('saboteurs')}")
                return 16

        print(f"[RUN] holding smoke processes for {args.duration}s")
        time.sleep(args.duration)

        failed_logs = [path for path in log_dir.glob("*.log") if log_has_failure(path)]
        if failed_logs:
            print("[FAIL] fatal/crash terms found:")
            for path in failed_logs:
                print(f"  - {path}")
            return 20

        print(f"[PASS] local smoke launch completed. logs={log_dir}")
        return 0
    finally:
        for process in reversed(processes):
            close_process(process)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
    except RuntimeError as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        raise SystemExit(1)
