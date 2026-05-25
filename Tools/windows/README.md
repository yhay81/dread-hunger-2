# Windows Tools

These helpers are for the Windows-only development path. Run them from a fresh Windows clone after Git LFS, Python, Unreal Engine, and Visual Studio are installed.

## First Run

From the repository root:

```powershell
.\Tools\windows\check_prereqs.ps1
.\Tools\windows\run_first_validation.ps1 -SkipGenerate
```

If Unreal is not installed in a default Epic path, set `UE_ROOT` first:

```powershell
$env:UE_ROOT = "D:\Epic Games\UE_5.7"
.\Tools\windows\run_first_validation.ps1 -UeRoot $env:UE_ROOT -SkipGenerate
```

## Optional Smoke

After Editor, Game, and Server validation are understood:

```powershell
.\Tools\windows\run_first_validation.ps1 -SkipGenerate -IncludeSmoke
```

`-IncludeSmoke` runs `qa-bot`, `match-timer`, `life-action`, `combined5`, `ready8`, and the quick smoke suite. Use `-IncludeHeavySmoke` only after that path is stable.

## Dedicated Server

After `AbyssLockServer.exe` exists, run the local server boot probe:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1
```

Then run the dedicated-server client join probe:

```powershell
.\Tools\windows\run_dedicated_client_join_validation.ps1
```

After the single-client join passes, run the 5-player ready-lobby dedicated probe:

```powershell
.\Tools\windows\run_dedicated_ready_validation.ps1
```

Use `-ServerExe`, `-ServerConfig`, or `-UeRoot` if your build output, config path, or Unreal install path differs from the defaults.

## Output

First-run validation output is written under ignored `Saved\WindowsValidation\`. Dedicated-server probe output is written under ignored `Saved\DedicatedServerValidation\`, `Saved\DedicatedClientJoinValidation\`, or `Saved\DedicatedReadyValidation\`. Copy the key pass/fail lines into `docs/windows-validation-template.md` or a new cycle record. Do not commit generated logs.
