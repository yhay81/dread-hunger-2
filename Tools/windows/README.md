# Windows Tools

These helpers are for the Windows-only development path. Run them from a fresh Windows clone after Git LFS, Rust, Unreal Engine, and Visual Studio are installed.

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

## Single Player

To launch a visible one-player local session for manual testing:

```powershell
.\Tools\windows\run_single_player.ps1
```

This opens `L_IcebreakerWhitebox` through `UnrealEditor.exe -game`, sets `-FrostwakeSinglePlayer`, auto-starts the match with one crew player and zero saboteurs, and writes ignored telemetry under `Saved\SinglePlayer\`.

## Dedicated Server

After `FrostwakeServer.exe` exists, run the local server boot probe:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1
```

Then run the dedicated-server client join probe:

```powershell
.\Tools\windows\run_dedicated_client_join_validation.ps1
```

After the single-client join passes, run the 8-player ready-lobby dedicated probe:

```powershell
.\Tools\windows\run_dedicated_ready_validation.ps1
```

Use `-ServerExe`, `-ServerConfig`, or `-UeRoot` if your build output, config path, or Unreal install path differs from the defaults.

## Phase 2 Entry

After the Windows clone can build the server target, this wrapper runs the first validation set and all dedicated entry probes in order:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate
```

Common override shape:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 `
  -UeRoot "D:\Epic Games\UE_5.7" `
  -SkipGenerate `
  -ServerExe ".\Binaries\Win64\FrostwakeServer.exe" `
  -ServerConfig ".\Saved\Config\server_config.local.json" `
  -Port 7777 `
  -Clients 8 `
  -ExpectedPlayers 8 `
  -ExpectedSaboteurs 2 `
  -IncludeSmoke
```

Add `-IncludeSmoke` if the Phase 1 listen-server smoke set should run in the same command. Record the result in `docs\windows-phase2-entry-template.md` or a new cycle record.

## Steam Dev Config

Before enabling any Steam Lobby work, verify that the committed defaults remain Null/LAN safe:

```powershell
.\Tools\windows\check_steam_dev_config.ps1
```

If you create ignored local Steam settings under `Saved\Config\steam_dev.local.ini`, validate them explicitly:

```powershell
.\Tools\windows\check_steam_dev_config.ps1 -SteamConfig Saved\Config\steam_dev.local.ini -RequireSteamConfig
```

See `docs\steam-dev-config-gate.md`.

## Steam Lobby Preflight

Before running or extending the Steam Lobby runtime path, validate Steam dev config plus lobby metadata:

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId AbyssLock-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox
```

The wrapper writes ignored evidence under `Saved\SteamLobbyValidation\`. It validates the accepted join decision and expected `BuildMismatch` / `MapMismatch` rejection probes before any client travel. `UFrostwakeLobbySubsystem` currently provides the Null/LAN-safe metadata and join-decision foundation; `-Runtime` intentionally fails until the Steam create/find/join path is implemented.

## Output

First-run validation output is written under ignored `Saved\WindowsValidation\`. Dedicated-server probe output is written under ignored `Saved\DedicatedServerValidation\`, `Saved\DedicatedClientJoinValidation\`, or `Saved\DedicatedReadyValidation\`. Phase 2 entry wrapper output is written under ignored `Saved\Phase2EntryValidation\` with `summary.txt`, `manifest.json`, and per-step logs. Steam Lobby preflight output is written under ignored `Saved\SteamLobbyValidation\`. Copy the key pass/fail lines into `docs/windows-validation-template.md`, `docs/windows-phase2-entry-template.md`, `docs/steam-lobby-validation-template.md`, or a new cycle record. Do not commit generated logs.
