#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
from dataclasses import asdict, dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "AbyssLock.uproject"
DEFAULT_UE_VERSION = "UE_5.7"


@dataclass
class GateResult:
    name: str
    status: str
    detail: str


def run(name: str, command: list[str], allow_unsupported_server: bool = False) -> GateResult:
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
    output = "\n".join(part for part in (completed.stdout.strip(), completed.stderr.strip()) if part)
    if completed.returncode == 0:
        return GateResult(name, "pass", summarize_output(output))

    if allow_unsupported_server and "Server targets are not currently supported from this engine distribution" in output:
        return GateResult(name, "blocked", "Launcher UE distribution does not support Server targets")

    return GateResult(name, "fail", summarize_output(output))


def summarize_output(output: str) -> str:
    if not output:
        return "ok"

    important: list[str] = []
    for line in output.splitlines():
        if any(token in line for token in ("Result:", "Error:", "ERROR:", "Failed", "Succeeded", "Server targets are not currently supported")):
            important.append(line)

    if important:
        return "\n".join(important[-12:])
    return "\n".join(output.splitlines()[-12:])


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


def build_script(ue_root: Path, build_platform: str) -> Path:
    if build_platform == "Win64":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Build.bat"
    if build_platform == "Mac":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "Build.sh"
    if build_platform == "Linux":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Linux" / "Build.sh"
    return ue_root / "Engine" / "Build" / "BatchFiles" / "Build.sh"


def generate_project_files_script(ue_root: Path, build_platform: str) -> Path:
    if build_platform == "Win64":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "GenerateProjectFiles.bat"
    if build_platform == "Mac":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "GenerateProjectFiles.sh"
    if build_platform == "Linux":
        return ue_root / "Engine" / "Build" / "BatchFiles" / "Linux" / "GenerateProjectFiles.sh"
    return ue_root / "Engine" / "Build" / "BatchFiles" / "GenerateProjectFiles.sh"


def check_paths(ue_root: Path, build_platform: str) -> list[GateResult]:
    build = build_script(ue_root, build_platform)
    results: list[GateResult] = []
    results.append(GateResult("ue_root", "pass" if ue_root.exists() else "fail", str(ue_root)))
    results.append(GateResult("build_script", "pass" if build.exists() else "fail", str(build)))
    results.append(GateResult("project", "pass" if PROJECT.exists() else "fail", str(PROJECT)))
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Frostwake Unreal build gates.")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON.")
    parser.add_argument("--skip-generate", action="store_true", help="Skip GenerateProjectFiles.")
    parser.add_argument("--include-server", action="store_true", help="Attempt server target build.")
    parser.add_argument("--ue-root", help="Override Unreal Engine root. Defaults to UE_ROOT/UNREAL_ENGINE_ROOT or common install paths.")
    parser.add_argument("--platform", default=host_platform(), help="Unreal build platform, usually Win64, Mac, or Linux.")
    args = parser.parse_args()

    ue_root = resolve_ue_root(args.ue_root)
    build = build_script(ue_root, args.platform)
    generate_project_files = generate_project_files_script(ue_root, args.platform)

    results = check_paths(ue_root, args.platform)
    if any(result.status == "fail" for result in results):
        emit(results, args.json)
        return 1

    if not args.skip_generate:
        results.append(run("generate_project_files", [str(generate_project_files), f"-project={PROJECT}", "-game"]))

    results.append(run("build_editor", [str(build), "AbyssLockEditor", args.platform, "Development", f"-Project={PROJECT}", "-WaitMutex"]))
    results.append(run("build_game", [str(build), "AbyssLock", args.platform, "Development", f"-Project={PROJECT}", "-WaitMutex"]))

    if args.include_server:
        results.append(run("build_server", [str(build), "AbyssLockServer", args.platform, "Development", f"-Project={PROJECT}", "-WaitMutex"], allow_unsupported_server=True))

    emit(results, args.json)
    return 1 if any(result.status == "fail" for result in results) else 0


def emit(results: list[GateResult], json_output: bool) -> None:
    if json_output:
        print(json.dumps([asdict(result) for result in results], indent=2, sort_keys=True))
        return

    for result in results:
        print(f"[{result.status.upper()}] {result.name}: {result.detail}")


if __name__ == "__main__":
    raise SystemExit(main())
