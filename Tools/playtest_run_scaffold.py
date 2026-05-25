#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLAYTEST_ID = "P1-024"
DEFAULT_MAP = "/Game/Maps/L_IcebreakerWhitebox"
DEFAULT_BUILD_ID = "AbyssLock-Win64-Development-local"
DEFAULT_PROFILE = "human-p1-024"
DEFAULT_WINDOWS_UE_ROOT = r"C:\Program Files\Epic Games\UE_5.7"
DEFAULT_MAC_UE_EDITOR = "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor"


def run_slug(run_number: int) -> str:
    if run_number < 1 or run_number > 999:
        raise SystemExit("--run-number must be between 1 and 999")
    return f"run-{run_number:02d}"


def target_players_value(value: int) -> int:
    if value not in {6, 7, 8}:
        raise SystemExit("--target-players must be 6, 7, or 8")
    return value


def write_file(path: Path, content: str, force: bool) -> None:
    if path.exists() and not force:
        raise SystemExit(f"{path} already exists; use --force to overwrite generated scaffold files")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    if path.suffix == ".sh":
        path.chmod(0o755)


def shell_prelude() -> str:
    return """#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
cd "$REPO_ROOT"
"""


def powershell_prelude(windows_ue_root: str) -> str:
    return f"""$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\\..\\..\\..")).Path
Set-Location $RepoRoot

if (-not $env:UE_ROOT -or $env:UE_ROOT.Trim() -eq "") {{
  $env:UE_ROOT = "{windows_ue_root}"
}}

$UEEditor = Join-Path $env:UE_ROOT "Engine\\Binaries\\Win64\\UnrealEditor.exe"
if (-not (Test-Path $UEEditor)) {{
  throw "UnrealEditor.exe not found at $UEEditor. Set UE_ROOT to your Unreal Engine install."
}}
"""


def preflight_ps1(windows_ue_root: str) -> str:
    return f"""{powershell_prelude(windows_ue_root)}
py -3 Tools\\quality_gate.py --require-ue
py -3 Tools\\unreal_gate.py --skip-generate --platform Win64 --include-server
py -3 Tools\\ue\\run_local_smoke.py --profile ready8 --skip-build --null-rhi --platform Win64
"""


def host_ps1(
    run_id: str,
    slug: str,
    target_players: int,
    port: int,
    build_id: str,
    map_id: str,
    profile: str,
    windows_ue_root: str,
    res_x: int,
    res_y: int,
) -> str:
    return f"""{powershell_prelude(windows_ue_root)}
& $UEEditor `
  "$RepoRoot\\AbyssLock.uproject" `
  "{map_id}?listen" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={port} `
  "-AbyssEventLog=$RepoRoot\\Saved\\Playtests\\P1-024\\{slug}\\events.jsonl" `
  -AbyssRunId={run_id} `
  "-AbyssBuildId={build_id}" `
  "-AbyssMapId={map_id}" `
  -AbyssProfile={profile} `
  -AbyssAutoReady `
  -AbyssLobbyMinPlayers={target_players}
"""


def client_local_ps1(port: int, client_port: int, windows_ue_root: str, res_x: int, res_y: int) -> str:
    return f"""{powershell_prelude(windows_ue_root)}
& $UEEditor `
  "$RepoRoot\\AbyssLock.uproject" `
  "127.0.0.1:{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={client_port}
"""


def client_lan_ps1(port: int, windows_ue_root: str, res_x: int, res_y: int) -> str:
    return f"""param(
  [Parameter(Mandatory = $true)]
  [string]$HostLanIp
)

{powershell_prelude(windows_ue_root)}
& $UEEditor `
  "$RepoRoot\\AbyssLock.uproject" `
  "$($HostLanIp):{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4
"""


def after_test_ps1(slug: str, run_id: str) -> str:
    return f"""$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\\..\\..\\..")).Path
Set-Location $RepoRoot

py -3 Tools\\log_summary.py Saved\\Playtests\\P1-024\\{slug}\\events.jsonl
py -3 Tools\\log_summary.py Saved\\Playtests\\P1-024\\{slug}\\events.jsonl --out Saved\\Playtests\\P1-024\\{slug}\\summary.json
py -3 Tools\\playtest_summary.py Saved\\Playtests\\P1-024\\{slug}\\summary.json --out docs\\playtests\\p1-024-{slug}-summary.md
py -3 Tools\\playtest_preflight.py Saved\\Playtests\\P1-024\\{slug}\\summary.json --markdown docs\\playtests\\p1-024-{slug}-summary.md
py -3 Tools\\playtest_report_upload.py Saved\\Playtests\\P1-024\\{slug}\\summary.json

Write-Host "Review and anonymize docs\\playtests\\p1-024-{slug}-summary.md before committing."
Write-Host "Review the backend payload dry-run before using playtest_report_upload.py --send."
Write-Host "RunId: {run_id}"
"""


def preflight_sh() -> str:
    return f"""{shell_prelude()}
python3 Tools/quality_gate.py --require-ue
python3 Tools/unreal_gate.py --skip-generate --include-server
python3 Tools/ue/run_local_smoke.py --profile ready8 --skip-build --null-rhi
"""


def host_sh(
    run_id: str,
    slug: str,
    target_players: int,
    port: int,
    build_id: str,
    map_id: str,
    profile: str,
    ue_editor: str,
    res_x: int,
    res_y: int,
) -> str:
    return f"""{shell_prelude()}
UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \\
  "$REPO_ROOT/AbyssLock.uproject" \\
  "{map_id}?listen" \\
  -game \\
  -windowed \\
  -ResX={res_x} \\
  -ResY={res_y} \\
  -NoLiveCoding \\
  -nop4 \\
  -port={port} \\
  -AbyssEventLog="$REPO_ROOT/Saved/Playtests/P1-024/{slug}/events.jsonl" \\
  -AbyssRunId={run_id} \\
  -AbyssBuildId="{build_id}" \\
  -AbyssMapId="{map_id}" \\
  -AbyssProfile={profile} \\
  -AbyssAutoReady \\
  -AbyssLobbyMinPlayers={target_players}
"""


def client_local_sh(port: int, client_port: int, ue_editor: str, res_x: int, res_y: int) -> str:
    return f"""{shell_prelude()}
UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \\
  "$REPO_ROOT/AbyssLock.uproject" \\
  "127.0.0.1:{port}" \\
  -game \\
  -windowed \\
  -ResX={res_x} \\
  -ResY={res_y} \\
  -NoLiveCoding \\
  -nop4 \\
  -port={client_port}
"""


def client_lan_sh(port: int, ue_editor: str, res_x: int, res_y: int) -> str:
    return f"""{shell_prelude()}
HOST_LAN_IP="${{1:-}}"
if [ -z "$HOST_LAN_IP" ]; then
  echo "Usage: $0 <host-lan-ip>" >&2
  exit 2
fi

UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \\
  "$REPO_ROOT/AbyssLock.uproject" \\
  "$HOST_LAN_IP:{port}" \\
  -game \\
  -windowed \\
  -ResX={res_x} \\
  -ResY={res_y} \\
  -NoLiveCoding \\
  -nop4
"""


def after_test_sh(slug: str) -> str:
    return f"""{shell_prelude()}
python3 Tools/log_summary.py Saved/Playtests/P1-024/{slug}/events.jsonl
python3 Tools/log_summary.py Saved/Playtests/P1-024/{slug}/events.jsonl --out Saved/Playtests/P1-024/{slug}/summary.json
python3 Tools/playtest_summary.py Saved/Playtests/P1-024/{slug}/summary.json --out docs/playtests/p1-024-{slug}-summary.md
python3 Tools/playtest_preflight.py Saved/Playtests/P1-024/{slug}/summary.json --markdown docs/playtests/p1-024-{slug}-summary.md
python3 Tools/playtest_report_upload.py Saved/Playtests/P1-024/{slug}/summary.json
"""


def commands_md(
    run_id: str,
    slug: str,
    target_players: int,
    port: int,
    build_id: str,
    map_id: str,
    profile: str,
    windows_ue_root: str,
    res_x: int,
    res_y: int,
) -> str:
    client_port = port + 1
    return f"""# {run_id} Commands

Run every command from the repository root on Windows PowerShell.

## Host Preflight

```powershell
py -3 Tools\\quality_gate.py --require-ue
py -3 Tools\\unreal_gate.py --skip-generate --platform Win64 --include-server
py -3 Tools\\ue\\run_local_smoke.py --profile ready8 --skip-build --null-rhi --platform Win64
```

## Environment

Set `UE_ROOT` if Unreal is not installed at the default path.

```powershell
$env:UE_ROOT = "{windows_ue_root}"
```

## Listen Host

```powershell
.\\Saved\\Playtests\\P1-024\\{slug}\\host.ps1
```

Manual equivalent:

```powershell
$UEEditor = Join-Path $env:UE_ROOT "Engine\\Binaries\\Win64\\UnrealEditor.exe"
& $UEEditor `
  "$PWD\\AbyssLock.uproject" `
  "{map_id}?listen" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={port} `
  "-AbyssEventLog=$PWD\\Saved\\Playtests\\P1-024\\{slug}\\events.jsonl" `
  -AbyssRunId={run_id} `
  "-AbyssBuildId={build_id}" `
  "-AbyssMapId={map_id}" `
  -AbyssProfile={profile} `
  -AbyssAutoReady `
  -AbyssLobbyMinPlayers={target_players}
```

## Same-Machine Client

```powershell
.\\Saved\\Playtests\\P1-024\\{slug}\\client-local.ps1
```

Manual equivalent:

```powershell
$UEEditor = Join-Path $env:UE_ROOT "Engine\\Binaries\\Win64\\UnrealEditor.exe"
& $UEEditor `
  "$PWD\\AbyssLock.uproject" `
  "127.0.0.1:{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={client_port}
```

For additional same-machine clients, increment the client port.

## LAN Client

```powershell
.\\Saved\\Playtests\\P1-024\\{slug}\\client-lan.ps1 -HostLanIp <host-lan-ip>
```

## After-Test Summary

```powershell
.\\Saved\\Playtests\\P1-024\\{slug}\\after-test.ps1
```

The after-test script prints a backend report payload dry-run. It does not upload unless `Tools\\playtest_report_upload.py` is rerun manually with `--send`.
"""


def run_card(run_id: str, slug: str, target_players: int, port: int, date: str, build_id: str, map_id: str, profile: str) -> str:
    expected_agents = 2 if target_players >= 7 else 1
    expected_crew = max(target_players - expected_agents, 0)
    return f"""# {run_id} Run Card

Date prepared: {date}

This directory is local-only raw playtest evidence. It is under `Saved/Playtests/...`, which is ignored by git. Do not copy raw logs, recordings, tester names, call links, host IPs, or transcripts into committed files.

## Header

```text
RunId: {run_id}
Build: {build_id}
Commit or local snapshot:
Host machine:
Host LAN IP:
Port: {port}
Map: {map_id}
Profile: {profile}
Target players: {target_players}
Expected crew: {expected_crew}
Expected agents: {expected_agents}
Voice method:
Recording consent: yes / no
Observer A, timing and logs:
Observer B, comprehension and social dynamics:
```

## Before Launch

Run from the repository root on Windows:

```powershell
py -3 Tools\\quality_gate.py --require-ue
py -3 Tools\\unreal_gate.py --skip-generate --platform Win64 --include-server
py -3 Tools\\ue\\run_local_smoke.py --profile ready8 --skip-build --null-rhi --platform Win64
```

Expected: quality gate passes, Editor/Game pass, Server target result is recorded, and `ready8` passes.

## During Run

- Keep observer notes in `host-notes.md`.
- Keep raw logs and recordings in this local directory only.
- Do not coach players mid-round unless the match is blocked.
- Do not reveal roles.
- Do not score proximity-voice quality from this Phase 1 run.

## After Run

```powershell
.\\Saved\\Playtests\\P1-024\\{slug}\\after-test.ps1
```

This also prints a backend report payload dry-run for review. It does not upload data.
"""


def host_notes(run_id: str, target_players: int) -> str:
    return f"""# {run_id} Host Notes

Keep this file local. Commit only the anonymized summary after preflight passes.

## Run Header

```text
RunId: {run_id}
Target players: {target_players}
Actual players:
Voice method:
Recording consent:
Observer A:
Observer B:
```

## Timeline

| Time | Event | Notes |
| --- | --- | --- |
|  | First goal comprehension |  |
|  | First repair or sabotage |  |
|  | First accusation |  |
|  | First down, containment, or rescue |  |
|  | Biggest confusion |  |
|  | Best social moment |  |
|  | Worst stall |  |
|  | End or interruption |  |

## Metrics

| Metric | Value |
| --- | --- |
| Players connected |  |
| Match started |  |
| Crew players |  |
| Agent players |  |
| Match ended |  |
| Winner |  |
| Disconnects |  |
| Crashes |  |
| Critical blockers |  |
| Players asking for another round |  |

## Aggregate Player Feedback

Do not write tester names.

| Question | Aggregate answer |
| --- | --- |
| What did players think their goal was? |  |
| When did suspicion first appear? |  |
| What created trust? |  |
| What created distrust? |  |
| What was confusing? |  |
| What was tense, funny, or memorable? |  |
| How many wanted another round? |  |
| What should be kept? |  |
| What should be cut? |  |
| What should change before the next test? |  |

## Private Role Feedback

Agent-only:

-

Crew-only:

-

## Keep / Cut / Change

Keep:

-

Cut:

-

Change:

-

## Blockers

| Rank | Blocker | Severity | Evidence | Proposed fix | Retest gate |
| ---: | --- | --- | --- | --- | --- |
| 1 |  |  |  |  |  |
| 2 |  |  |  |  |  |
| 3 |  |  |  |  |  |
| 4 |  |  |  |  |  |
| 5 |  |  |  |  |  |
"""


def player_brief() -> str:
    return """# Player Brief

Read once before the round.

```text
This is a greybox prototype. Visuals and UI are temporary.

Crew: keep the ship alive and progress the route. Repair systems, share information, and watch for suspicious behavior.

Agent: prevent the ship from succeeding. Use sabotage, timing, isolation, and false explanations. Do not simply hide forever; create situations other players can discuss.

Everyone: speak naturally, call out discoveries, and say what you think happened. If something is confusing, say it out loud rather than silently waiting.
```
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a local ignored P1-024 playtest run scaffold under Saved/Playtests.")
    parser.add_argument("--run-number", type=int, default=1, help="Run number used for run-NN and P1-024-run-NN.")
    parser.add_argument("--target-players", type=int, default=6, help="Target player count: 6, 7, or 8.")
    parser.add_argument("--port", type=int, default=7777, help="Listen server port.")
    parser.add_argument("--res-x", type=int, default=1280, help="Window width for generated launch commands.")
    parser.add_argument("--res-y", type=int, default=720, help="Window height for generated launch commands.")
    parser.add_argument("--build-id", default=DEFAULT_BUILD_ID, help="Abyss build id for telemetry.")
    parser.add_argument("--map-id", default=DEFAULT_MAP, help="Map path for telemetry and launch.")
    parser.add_argument("--profile", default=DEFAULT_PROFILE, help="Telemetry profile.")
    parser.add_argument("--ue-root", default=DEFAULT_WINDOWS_UE_ROOT, help="Default Windows UE_ROOT used inside generated PowerShell scripts.")
    parser.add_argument("--ue-editor", default=DEFAULT_MAC_UE_EDITOR, help="Legacy UnrealEditor path used inside generated bash scripts.")
    parser.add_argument("--force", action="store_true", help="Overwrite existing generated scaffold files.")
    parser.add_argument("--dry-run", action="store_true", help="Print planned files without writing them.")
    args = parser.parse_args()

    target_players = target_players_value(args.target_players)
    slug = run_slug(args.run_number)
    run_id = f"{PLAYTEST_ID}-{slug}"
    run_dir = ROOT / "Saved" / "Playtests" / PLAYTEST_ID / slug
    today = dt.date.today().isoformat()
    client_port = args.port + 1

    files = {
        run_dir / "README.md": run_card(run_id, slug, target_players, args.port, today, args.build_id, args.map_id, args.profile),
        run_dir / "commands.md": commands_md(
            run_id,
            slug,
            target_players,
            args.port,
            args.build_id,
            args.map_id,
            args.profile,
            args.ue_root,
            args.res_x,
            args.res_y,
        ),
        run_dir / "host-notes.md": host_notes(run_id, target_players),
        run_dir / "player-brief.md": player_brief(),
        run_dir / "preflight.ps1": preflight_ps1(args.ue_root),
        run_dir / "host.ps1": host_ps1(
            run_id,
            slug,
            target_players,
            args.port,
            args.build_id,
            args.map_id,
            args.profile,
            args.ue_root,
            args.res_x,
            args.res_y,
        ),
        run_dir / "client-local.ps1": client_local_ps1(args.port, client_port, args.ue_root, args.res_x, args.res_y),
        run_dir / "client-lan.ps1": client_lan_ps1(args.port, args.ue_root, args.res_x, args.res_y),
        run_dir / "after-test.ps1": after_test_ps1(slug, run_id),
        run_dir / "preflight.sh": preflight_sh(),
        run_dir / "host.sh": host_sh(
            run_id,
            slug,
            target_players,
            args.port,
            args.build_id,
            args.map_id,
            args.profile,
            args.ue_editor,
            args.res_x,
            args.res_y,
        ),
        run_dir / "client-local.sh": client_local_sh(args.port, client_port, args.ue_editor, args.res_x, args.res_y),
        run_dir / "client-lan.sh": client_lan_sh(args.port, args.ue_editor, args.res_x, args.res_y),
        run_dir / "after-test.sh": after_test_sh(slug),
    }
    directories = [run_dir / "recordings", run_dir / "screenshots", run_dir / "crash-notes"]

    print(f"Run directory: {run_dir.relative_to(ROOT)}")
    for directory in directories:
        print(f"Directory: {directory.relative_to(ROOT)}")
    for path in files:
        print(f"File: {path.relative_to(ROOT)}")

    if args.dry_run:
        return 0

    for directory in directories:
        directory.mkdir(parents=True, exist_ok=True)
    for path, content in files.items():
        write_file(path, content, args.force)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
