# Windows Validation Template

Use this after cloning on the Windows workstation. Copy this file to a new cycle record or a dated local note, fill only anonymized results, and keep raw logs under ignored `Saved/` paths.

## Header

```text
Date:
Windows machine:
CPU/GPU/RAM:
Windows version:
Unreal Engine path:
Unreal Engine source or Launcher:
Visual Studio version:
Commit:
Branch:
Git LFS pull completed: yes / no
```

## Required Commands

Run from the repository root in PowerShell.

```powershell
git status --short --branch
git lfs status
py -3 Tools\quality_gate.py --require-ue
py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server
py -3 Tools\ue\run_local_smoke.py --profile qa-bot --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile match-timer --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile life-action --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile combined5 --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_local_smoke.py --profile ready8 --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_smoke_suite.py --skip-build --null-rhi --platform Win64
py -3 Tools\ue\run_smoke_suite.py --include-heavy --skip-build --null-rhi --platform Win64
```

## Result Matrix

| Gate | Result | Evidence Path | Notes |
| --- | --- | --- | --- |
| Git clean after clone | pass / fail | command output |  |
| Git LFS assets present | pass / fail | command output |  |
| Quality gate | pass / fail | command output |  |
| Editor Win64 build | pass / fail | `Tools\unreal_gate.py` output |  |
| Game Win64 build | pass / fail | `Tools\unreal_gate.py` output |  |
| Server Win64 build | pass / blocked / fail | `Tools\unreal_gate.py` output | If blocked, record Launcher vs source build |
| `qa-bot` | pass / fail | `Saved\SmokeTests\...` |  |
| `match-timer` | pass / fail | `Saved\SmokeTests\...` | Expected saboteur win by `timer_expired` |
| `life-action` | pass / fail | `Saved\SmokeTests\...` | Expected 3 life-action interactions |
| `combined5` | pass / fail | `Saved\SmokeTests\...` |  |
| `ready8` | pass / fail | `Saved\SmokeTests\...` | Expected `8p/2s` |
| Quick suite | pass / fail | `Saved\SmokeSuites\...` | Covers `qa-bot`, `qa-player-bot`, and `match-timer` |
| Heavy suite | pass / fail | `Saved\SmokeSuites\...` | Appends `qa-task-bot`, `life-action`, `combined5`, `ready8`, and `combined8` |

## Server Target Finding

```text
AbyssLockServer result:
Launcher UE or source UE:
Exact error if blocked/failed:
Dedicated server validation wrapper:
Dedicated client join wrapper:
Dedicated ready-lobby wrapper:
Decision:
```

Decision values:

- `pass`: Windows can build server target; start Dedicated Server validation next.
- `blocked`: Launcher UE cannot build server target; install/build UE source distribution.
- `fail`: Server target is supported but the project fails to compile; fix project code first.

If `AbyssLockServer` exists, run:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1
.\Tools\windows\run_dedicated_client_join_validation.ps1
.\Tools\windows\run_dedicated_ready_validation.ps1
```

Record the output folder and `log_summary.json` result while keeping raw logs under ignored `Saved\DedicatedServerValidation\`, `Saved\DedicatedClientJoinValidation\`, or `Saved\DedicatedReadyValidation\`.

## Smoke Suite Summary

After a suite run, export Markdown:

```powershell
py -3 Tools\ue\export_smoke_suite_markdown.py Saved\SmokeSuites\<suite-id>\suite_summary.json
```

Paste the compact table here, or link the local path while keeping raw logs out of git.

## Keep / Fix / Defer

Keep:

-

Fix before human test:

-

Defer until after P1-024:

-

## Next Action

Choose exactly one:

- Run P1-024 human test with `py -3 Tools\playtest_run_scaffold.py --run-number 1 --target-players 6`.
- Fix Windows build/smoke blocker.
- Install/build UE source distribution for Dedicated Server.
- Start visual POC only after Windows smoke evidence passes.
