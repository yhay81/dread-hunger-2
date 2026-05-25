# Windows Handoff

Current as of 2026-05-25 JST, after cycle 52. Production implementation resumes on a Windows machine from `main`; Mac work should stay limited to docs, backend/tools, and listen-server validation until the Windows clone records fresh Win64 evidence.

## Decision

- Target development platform: Windows.
- Target Unreal build platform: `Win64`.
- Keep the existing internal module name `AbyssLock` for now.
- Keep `/Game/Maps/L_IcebreakerWhitebox` as the automation map until the Windows clone passes the same smoke gates.
- Create visual experiments in `/Game/Maps/L_FrostwakeVisualPOC` only after the candidate asset ledger is reviewed.

## Clone

```powershell
git clone git@github.com:yhay81/dread-hunger-2.git
cd dread-hunger-2
git lfs install
git lfs pull
```

If SSH is not configured on the Windows machine, use the HTTPS remote and authenticate with GitHub.

## Required Tools

- Windows 11 recommended.
- Git for Windows with Git LFS.
- Python 3.11 or newer; latest 3.14.x is preferred for tools.
- Node 24 LTS for backend/admin tooling. Current-line Node can be used locally, but CI and production should target LTS.
- Unreal Engine 5.7 installed through Epic Games Launcher or a source build. Use the latest available 5.7.x patch on the Windows machine.
- Visual Studio 2022 Current channel 17.14 with **Game development with C++**, MSVC 14.44-compatible tooling, and a Windows 10/11 SDK. Do not switch to the Visual Studio 2026 / MSVC 14.50 toolchain until Epic marks it supported for UE 5.7.

If Unreal is not installed at the default Epic path, set `UE_ROOT`:

```powershell
$env:UE_ROOT = "D:\Epic Games\UE_5.7"
```

## First Validation

Run these from the repository root:

```powershell
py -3 Tools\quality_gate.py --require-ue
py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server
```

The same first-run path is also wrapped as PowerShell helpers:

```powershell
.\Tools\windows\check_prereqs.ps1
.\Tools\windows\run_first_validation.ps1 -SkipGenerate
```

Add `-UeRoot "D:\Epic Games\UE_5.7"` if Unreal is not installed in a default Epic path. Add `-IncludeSmoke` only after the first Editor/Game/Server result is understood; it runs `qa-bot`, `match-timer`, `life-action`, `combined5`, `ready8`, and the quick smoke suite. Add `-IncludeHeavySmoke` after the quick path is stable.

Expected result:

- `quality_gate.py` should pass.
- `AbyssLockEditor` and `AbyssLock` should build.
- `AbyssLockServer` should be attempted. If the Launcher UE distribution blocks Server targets, install or build a UE source distribution and rerun with `UE_ROOT` pointing to that source build.

Record the full first-run result using [Windows Validation Template](windows-validation-template.md).

If `AbyssLockServer` builds, continue with [Windows Dedicated Server Runbook](windows-dedicated-server-runbook.md).

The first local dedicated-server boot probe is:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1
.\Tools\windows\run_dedicated_client_join_validation.ps1
.\Tools\windows\run_dedicated_ready_validation.ps1
```

## Project Files

Generate Visual Studio files with either the Unreal shell integration or:

```powershell
& "$env:UE_ROOT\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="$PWD\AbyssLock.uproject" -game -engine
```

Then open the generated solution or open `AbyssLock.uproject` in Unreal Editor and rebuild when prompted.

## Smoke Tests

After Editor/Game builds pass:

```powershell
py -3 Tools\ue\run_local_smoke.py --profile qa-bot --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile match-timer --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile life-action --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile combined5 --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile ready8 --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_smoke_suite.py --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_smoke_suite.py --include-heavy --skip-build --null-rhi --platform Win64
```

The quick suite currently covers `qa-bot`, `qa-player-bot`, and `match-timer`. The heavy suite appends `qa-task-bot`, `life-action`, `combined5`, `ready8`, and `combined8`.

The known good Mac evidence before handoff was the heavy listen-server smoke suite recorded in `docs/phase1-milestone-report.md`. The Windows clone should recreate fresh `Saved/SmokeTests` or `Saved/SmokeSuites` evidence locally; those generated folders stay ignored.

The P1-024 human-test scaffold creates Windows PowerShell scripts plus legacy POSIX shell scripts:

```powershell
py -3 Tools\playtest_run_scaffold.py --run-number 1 --target-players 6
```

## Visual POC

Do not import marketplace/Fab assets directly into production folders. Start with:

```powershell
py -3 Tools\asset_ledger_check.py
py -3 Tools\ue\scaffold_frostwake_visual_poc.py --dry-run
py -3 Tools\ue\scaffold_frostwake_visual_poc.py --write
```

The first recommended evaluation package is the free `Modular Ship Interior Environment`, followed by one paid near-future interior kit only after a fresh license/listing check and user approval. Candidate rows are in `docs/asset-ledger-candidates.csv`.

## Do Not Commit

- `Binaries/`
- `Build/Mac/`
- `Build/Windows/`
- `DerivedDataCache/`
- `Intermediate/`
- `Saved/`
- `.vs/`
- generated `.sln`, `.vcxproj`, and `.xcworkspace` files
- `Tools/install/`
- `references/private/`
- downloaded third-party game/server files
- raw playtest logs, recordings, screenshots, or personal identifiers

## Next Windows Task

1. Clone and run `py -3 Tools\quality_gate.py --require-ue`.
2. Run `py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server`.
3. Fill [Windows Validation Template](windows-validation-template.md). If server build passes, record the result in a new cycle. If it is blocked, document whether a UE source build is required on Windows.
4. Run the dedicated-server boot, client-join, and ready-lobby probes.
5. Run `qa-bot`, `match-timer`, `life-action`, `combined5`, and `ready8` smoke profiles.
6. Only after those pass, proceed to the visual POC or P1-024 human playtest.
