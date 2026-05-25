#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import json
import os
import platform
import shutil
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "docs" / "quality-gates.json"
CYCLE_PATTERN = re.compile(r"cycle-(\d+)\.md$")
EXCLUDED_DIRS = {
    ".git",
    "Binaries",
    "DerivedDataCache",
    "Intermediate",
    "Saved",
    "Script",
    "Tools/install",
    "references/private",
    "references/inventory",
}
TEXT_SUFFIXES = {
    ".cs",
    ".cpp",
    ".h",
    ".ini",
    ".json",
    ".md",
    ".py",
    ".ps1",
    ".sh",
    ".txt",
    ".uproject",
}
FORBIDDEN_TRACKED_PREFIXES = (
    "references/private/",
    "references/inventory/",
    "Tools/install/",
)
FORBIDDEN_TRACKED_SUFFIXES = (
    ".7z",
    ".db",
    ".dll",
    ".dmg",
    ".exe",
    ".log",
    ".pak",
    ".pdb",
    ".sig",
    ".ucas",
    ".utoc",
    ".zip",
)


@dataclass
class CheckResult:
    name: str
    status: str
    detail: str


def relative(path: Path) -> str:
    return str(path.relative_to(ROOT))


def load_config() -> dict:
    with CONFIG_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def run_command(name: str, command: list[str]) -> CheckResult:
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
    output = "\n".join(part for part in (completed.stdout.strip(), completed.stderr.strip()) if part)
    if completed.returncode == 0:
        return CheckResult(name, "pass", output or "ok")
    return CheckResult(name, "fail", output or f"exit code {completed.returncode}")


def check_required_files(config: dict) -> CheckResult:
    missing = [item for item in config["required_files"] if not (ROOT / item).exists()]
    if missing:
        return CheckResult("required_files", "fail", ", ".join(missing))
    return CheckResult("required_files", "pass", f"{len(config['required_files'])} files present")


def check_json_files(config: dict) -> CheckResult:
    errors: list[str] = []
    for item in config["json_files"]:
        path = ROOT / item
        try:
            with path.open("r", encoding="utf-8") as handle:
                json.load(handle)
        except Exception as exc:  # noqa: BLE001 - report all parse/read failures.
            errors.append(f"{item}: {exc}")

    if errors:
        return CheckResult("json_parse", "fail", "\n".join(errors))
    return CheckResult("json_parse", "pass", f"{len(config['json_files'])} JSON files parsed")


def check_python_files(config: dict) -> CheckResult:
    errors: list[str] = []
    for item in config["python_files"]:
        try:
            ast.parse((ROOT / item).read_text(encoding="utf-8"), filename=item)
        except SyntaxError as exc:
            errors.append(f"{item}: {exc}")

    if errors:
        return CheckResult("python_compile", "fail", "\n".join(errors))
    return CheckResult("python_compile", "pass", f"{len(config['python_files'])} Python files compile")


def validate_schema_value(schema: dict, value: object, path: str) -> list[str]:
    errors: list[str] = []
    schema_type = schema.get("type")

    if schema_type == "object":
        if not isinstance(value, dict):
            return [f"{path}: expected object"]

        required = schema.get("required", [])
        for field in required:
            if field not in value:
                errors.append(f"{path}.{field}: required field missing")

        properties = schema.get("properties", {})
        if schema.get("additionalProperties") is False:
            for field in value:
                if field not in properties:
                    errors.append(f"{path}.{field}: additional property not allowed")

        for field, field_schema in properties.items():
            if field in value:
                errors.extend(validate_schema_value(field_schema, value[field], f"{path}.{field}"))
        return errors

    if schema_type == "array":
        if not isinstance(value, list):
            return [f"{path}: expected array"]
        item_schema = schema.get("items", {})
        for index, item in enumerate(value):
            errors.extend(validate_schema_value(item_schema, item, f"{path}[{index}]"))
        return errors

    if schema_type == "string":
        if not isinstance(value, str):
            return [f"{path}: expected string"]
        min_length = schema.get("minLength")
        max_length = schema.get("maxLength")
        enum = schema.get("enum")
        if min_length is not None and len(value) < min_length:
            errors.append(f"{path}: shorter than minLength {min_length}")
        if max_length is not None and len(value) > max_length:
            errors.append(f"{path}: longer than maxLength {max_length}")
        if enum is not None and value not in enum:
            errors.append(f"{path}: value {value!r} not in enum")
        return errors

    if schema_type == "integer":
        if not isinstance(value, int) or isinstance(value, bool):
            return [f"{path}: expected integer"]
        minimum = schema.get("minimum")
        maximum = schema.get("maximum")
        if minimum is not None and value < minimum:
            errors.append(f"{path}: below minimum {minimum}")
        if maximum is not None and value > maximum:
            errors.append(f"{path}: above maximum {maximum}")
        return errors

    if schema_type == "boolean":
        if not isinstance(value, bool):
            return [f"{path}: expected boolean"]
        return errors

    return errors


def check_schema_examples(config: dict) -> CheckResult:
    errors: list[str] = []
    examples = config.get("schema_examples", [])
    for pair in examples:
        schema_path = ROOT / pair["schema"]
        example_path = ROOT / pair["example"]
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
        example = json.loads(example_path.read_text(encoding="utf-8"))
        errors.extend(f"{pair['example']}: {error}" for error in validate_schema_value(schema, example, "$"))

    if errors:
        return CheckResult("schema_examples", "fail", "\n".join(errors))
    return CheckResult("schema_examples", "pass", f"{len(examples)} examples validate")


def is_excluded(path: Path) -> bool:
    try:
        rel = path.relative_to(ROOT)
    except ValueError:
        return True

    rel_text = str(rel)
    return any(rel_text == item or rel_text.startswith(f"{item}/") for item in EXCLUDED_DIRS)


def iter_text_files() -> Iterable[Path]:
    for path in ROOT.rglob("*"):
        if not path.is_file() or is_excluded(path):
            continue
        if path.name.startswith(".") and path.suffix == "":
            continue
        if path.suffix in TEXT_SUFFIXES or path.name == "AbyssLock.uproject":
            yield path


def check_stale_terms(config: dict) -> CheckResult:
    findings: list[str] = []
    terms = [term.lower() for term in config["stale_terms"]]
    allowed_files = {
        "docs/ip-risk.md",
        "docs/ip-boundary.md",
        "docs/quality-gates.json",
        "docs/reference-policy.md",
        "references/README.md",
        "references/external-paths.json",
        "references/private-copy-manifest.md",
    }

    for path in iter_text_files():
        rel = relative(path)
        if rel in allowed_files:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore").lower()
        for term in terms:
            if term in text:
                findings.append(f"{rel}: {term}")

    if findings:
        return CheckResult("stale_terms", "fail", "\n".join(findings[:80]))
    return CheckResult("stale_terms", "pass", "no stale core terms found")


def check_git_ignored(config: dict) -> CheckResult:
    failures: list[str] = []
    for item in config["required_directories_ignored_by_git"]:
        completed = subprocess.run(
            ["git", "check-ignore", "-q", item],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            failures.append(item)

    if failures:
        return CheckResult("git_ignored_private_paths", "fail", ", ".join(failures))
    return CheckResult("git_ignored_private_paths", "pass", "private/generated paths ignored")


def check_source_control_safety() -> CheckResult:
    completed = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        return CheckResult("source_control_safety", "fail", completed.stderr.strip() or "git ls-files failed")

    failures: list[str] = []
    for rel in completed.stdout.splitlines():
        path = ROOT / rel
        if rel.startswith(FORBIDDEN_TRACKED_PREFIXES):
            failures.append(f"{rel}: forbidden path")
        if rel.lower().endswith(FORBIDDEN_TRACKED_SUFFIXES):
            failures.append(f"{rel}: forbidden binary/package/log suffix")
        if path.is_symlink():
            target = path.resolve(strict=False)
            if "references/private" in str(target):
                failures.append(f"{rel}: symlink points into private references")

    if failures:
        return CheckResult("source_control_safety", "fail", "\n".join(failures[:80]))
    return CheckResult("source_control_safety", "pass", "no forbidden tracked-safe paths")


def check_shell_files(config: dict) -> CheckResult:
    errors: list[str] = []
    for item in config.get("shell_files", []):
        completed = subprocess.run(
            ["bash", "-n", item],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            errors.append(f"{item}: {completed.stderr.strip() or completed.stdout.strip()}")

    if errors:
        return CheckResult("shell_syntax", "fail", "\n".join(errors))
    return CheckResult("shell_syntax", "pass", f"{len(config.get('shell_files', []))} shell files parsed")


def powershell_static_errors(path: Path) -> list[str]:
    errors: list[str] = []
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines:
        return ["empty file"]

    for index, line in enumerate(lines, start=1):
        stripped = line.rstrip()
        if stripped.endswith("`"):
            if index == len(lines):
                errors.append(f"line {index}: trailing line-continuation backtick at EOF")
            elif not lines[index].strip():
                errors.append(f"line {index}: line-continuation backtick followed by a blank line")

    text = "\n".join(lines)
    if "Set-StrictMode" not in text:
        errors.append("missing Set-StrictMode")
    if "$ErrorActionPreference" not in text:
        errors.append("missing $ErrorActionPreference")
    return errors


def check_powershell_files(config: dict) -> CheckResult:
    files = config.get("powershell_files", [])
    parser = shutil.which("pwsh") or shutil.which("powershell")
    errors: list[str] = []

    for item in files:
        path = ROOT / item
        errors.extend(f"{item}: {error}" for error in powershell_static_errors(path))
        if parser:
            command = [
                parser,
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                (
                    "$errors = $null; "
                    f"$null = [System.Management.Automation.PSParser]::Tokenize((Get-Content -Raw -LiteralPath '{path}'), [ref]$errors); "
                    "if ($errors) { $errors | ForEach-Object { Write-Error $_ }; exit 1 }"
                ),
            ]
            completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
            if completed.returncode != 0:
                errors.append(f"{item}: {completed.stderr.strip() or completed.stdout.strip()}")

    if errors:
        return CheckResult("powershell_syntax", "fail", "\n".join(errors))
    if parser:
        return CheckResult("powershell_syntax", "pass", f"{len(files)} PowerShell files parsed with {Path(parser).name}")
    return CheckResult("powershell_syntax", "pass", f"{len(files)} PowerShell files passed static checks")


def check_unreal_metadata() -> CheckResult:
    errors: list[str] = []
    project = json.loads((ROOT / "AbyssLock.uproject").read_text(encoding="utf-8"))
    module_names = {module["Name"] for module in project.get("Modules", [])}
    for module_name in module_names:
        if not (ROOT / "Source" / module_name / f"{module_name}.Build.cs").exists():
            errors.append(f"missing Build.cs for module {module_name}")

    target_files = list((ROOT / "Source").glob("*.Target.cs"))
    for target_file in target_files:
        text = target_file.read_text(encoding="utf-8")
        if not any(f'ExtraModuleNames.Add("{module}")' in text for module in module_names):
            errors.append(f"{relative(target_file)}: no project module in ExtraModuleNames")

    plugins = {plugin["Name"]: plugin.get("Enabled") for plugin in project.get("Plugins", [])}
    if plugins.get("OnlineSubsystemSteam") is not False:
        errors.append("OnlineSubsystemSteam must remain disabled during Phase 1")

    default_engine = (ROOT / "Config" / "DefaultEngine.ini").read_text(encoding="utf-8")
    if "DefaultPlatformService=Null" not in default_engine:
        errors.append("DefaultPlatformService=Null missing")
    if "MaxPlayers=8" not in default_engine:
        errors.append("MaxPlayers=8 missing")

    if errors:
        return CheckResult("unreal_metadata", "fail", "\n".join(errors))
    return CheckResult("unreal_metadata", "pass", f"{len(module_names)} module(s), {len(target_files)} target(s)")


def check_unreal_cpp_heuristics() -> CheckResult:
    errors: list[str] = []
    source_root = ROOT / "Source"
    for header in source_root.rglob("*.h"):
        text = header.read_text(encoding="utf-8")
        if any(marker in text for marker in ("UCLASS", "USTRUCT", "UENUM")):
            expected = f'#include "{header.stem}.generated.h"'
            if expected not in text:
                errors.append(f"{relative(header)}: missing {expected}")

    secret_team_declared = any("SecretTeam" in header.read_text(encoding="utf-8") for header in source_root.rglob("*.h"))
    if secret_team_declared:
        replication_text = "\n".join(cpp.read_text(encoding="utf-8") for cpp in source_root.rglob("*.cpp"))
        if "DOREPLIFETIME_CONDITION" not in replication_text or "SecretTeam" not in replication_text or "COND_OwnerOnly" not in replication_text:
            errors.append("SecretTeam must replicate with COND_OwnerOnly")

    if errors:
        return CheckResult("unreal_cpp_heuristics", "fail", "\n".join(errors[:80]))
    return CheckResult("unreal_cpp_heuristics", "pass", "basic Unreal C++ heuristics passed")


def check_cycle_records() -> CheckResult:
    cycle_dir = ROOT / "docs" / "cycles"
    cycle_files = sorted(
        (path for path in cycle_dir.glob("*.md") if path.name != "template.md"),
        key=lambda path: int(CYCLE_PATTERN.search(path.name).group(1)) if CYCLE_PATTERN.search(path.name) else -1,
    )
    if not cycle_files:
        return CheckResult("cycle_records", "fail", "no cycle records found")

    latest = cycle_files[-1]
    text = latest.read_text(encoding="utf-8")
    required_terms = ["Goal:", "Gate:", "## Evidence", "## Decisions", "## Next Cycle"]
    missing = [term for term in required_terms if term not in text]
    if missing:
        return CheckResult("cycle_records", "fail", f"{relative(latest)} missing {', '.join(missing)}")
    return CheckResult("cycle_records", "pass", f"latest cycle record {relative(latest)}")


def check_unreal_installed(require_ue: bool) -> CheckResult:
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
                candidates.extend(Path(base, "Epic Games").glob("UE_*"))
    elif system == "Darwin":
        shared = Path("/Users/Shared/Epic Games")
        if shared.exists():
            candidates.extend(shared.glob("UE_*"))
    else:
        candidates.extend((Path("/opt").glob("UE_*") if Path("/opt").exists() else []))

    launcher = Path("/Applications/Epic Games Launcher.app") if system == "Darwin" else Path(r"C:\Program Files (x86)\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe")
    existing_candidates = [path for path in candidates if path.exists()]
    if existing_candidates:
        return CheckResult("unreal_installed", "pass", ", ".join(str(path) for path in existing_candidates))
    if require_ue:
        return CheckResult("unreal_installed", "fail", "UE install not found. Set UE_ROOT or UNREAL_ENGINE_ROOT if installed elsewhere.")
    if launcher.exists():
        return CheckResult("unreal_installed", "warn", "Epic Games Launcher installed, UE install not found")
    return CheckResult("unreal_installed", "warn", "Epic Games Launcher and UE install not found")


def check_cpp_toolchain(require_ue: bool) -> CheckResult:
    if platform.system() == "Windows":
        cl_path = shutil.which("cl.exe") or shutil.which("cl")
        if cl_path:
            return CheckResult("cpp_toolchain", "pass", f"MSVC compiler on PATH: {cl_path}")

        vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        if vswhere.exists():
            completed = subprocess.run(
                [str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Workload.NativeGame", "-property", "installationPath"],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            install_path = completed.stdout.strip()
            if completed.returncode == 0 and install_path:
                return CheckResult("cpp_toolchain", "pass", f"Visual Studio native game workload: {install_path}")

        detail = "Visual Studio C++ toolchain not found. Install Visual Studio 2022 with Game development with C++."
        if require_ue:
            return CheckResult("cpp_toolchain", "fail", detail)
        return CheckResult("cpp_toolchain", "warn", detail)

    if platform.system() != "Darwin":
        cxx = shutil.which("clang++") or shutil.which("g++")
        if cxx:
            return CheckResult("cpp_toolchain", "pass", cxx)
        detail = "C++ compiler not found on PATH"
        if require_ue:
            return CheckResult("cpp_toolchain", "fail", detail)
        return CheckResult("cpp_toolchain", "warn", detail)

    completed = subprocess.run(
        ["xcodebuild", "-version"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode == 0:
        return CheckResult("cpp_toolchain", "pass", completed.stdout.strip().splitlines()[0])

    xcode_select = subprocess.run(
        ["xcode-select", "-p"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    detail = completed.stderr.strip() or completed.stdout.strip() or "xcodebuild unavailable"
    if xcode_select.stdout.strip():
        detail = f"{detail}; active developer dir: {xcode_select.stdout.strip()}"

    if require_ue:
        return CheckResult("cpp_toolchain", "fail", detail)
    return CheckResult("cpp_toolchain", "warn", detail)


def emit(results: list[CheckResult], json_output: bool) -> None:
    if json_output:
        print(json.dumps([result.__dict__ for result in results], indent=2, sort_keys=True))
        return

    for result in results:
        print(f"[{result.status.upper()}] {result.name}: {result.detail}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Frostwake local quality gates.")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON.")
    parser.add_argument("--require-ue", action="store_true", help="Fail if Unreal Engine is not installed.")
    args = parser.parse_args()

    config = load_config()
    results = [
        check_required_files(config),
        check_json_files(config),
        check_schema_examples(config),
        check_python_files(config),
        run_command("unit_tests", ["python3", "-m", "unittest", "discover", "-s", "tests"]),
        check_git_ignored(config),
        check_source_control_safety(),
        check_shell_files(config),
        check_powershell_files(config),
        check_stale_terms(config),
        check_unreal_metadata(),
        check_unreal_cpp_heuristics(),
        check_cycle_records(),
        run_command("asset_ledger_check", ["python3", "Tools/asset_ledger_check.py"]),
        run_command("secret_scan", ["Tools/ops/secret_scan.sh"]),
        run_command("git_diff_check", ["git", "diff", "--check"]),
        run_command("git_cached_diff_check", ["git", "diff", "--cached", "--check"]),
        check_unreal_installed(args.require_ue),
        check_cpp_toolchain(args.require_ue),
    ]

    emit(results, args.json)
    return 1 if any(result.status == "fail" for result in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
