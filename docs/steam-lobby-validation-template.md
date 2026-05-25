# Steam Lobby Validation Template

Use this for P2-003/P2-004 evidence on the Windows workstation. The current wrapper is a preflight until `UAbyssLobbySubsystem` is implemented.

## Header

```text
Date:
Commit:
Branch:
Windows machine:
Steam client running: yes / no
Steam dev config path:
Expected build id:
Expected map id:
```

## Preflight Command

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId AbyssLock-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox
```

Expected result:

- Steam dev config validation passes.
- Lobby metadata validation passes.
- `Saved\SteamLobbyValidation\<timestamp>\manifest.json` is written.
- Decision is `preflight_pass`.

## Future Runtime Command

After `UAbyssLobbySubsystem` exists:

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId AbyssLock-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox `
  -Runtime
```

## Runtime Evidence Matrix

| Gate | Result | Evidence Path | Notes |
| --- | --- | --- | --- |
| Steam dev config | pass / fail | `Saved\SteamLobbyValidation\...\steam_dev_config.log` |  |
| Lobby metadata contract | pass / fail | `Saved\SteamLobbyValidation\...\lobby_metadata.log` |  |
| Lobby create | pass / fail | JSONL event log | `lobby_create_requested`, `lobby_create_succeeded` |
| Lobby search | pass / fail | JSONL event log | `lobby_search_requested`, `lobby_search_completed` |
| Metadata rejection | pass / fail | JSONL event log | Build/map mismatch rejected before travel |
| Lobby join | pass / fail | JSONL event log | `lobby_join_requested`, `lobby_join_succeeded` |
| Travel | pass / fail | JSONL event log | `lobby_travel_started`; failure reason if any |
| Authoritative server path | pass / fail | JSONL event log | `client_connected`, ready state, role assignment, match start |

## Decision

Choose one:

- `preflight-pass`: P2-003 implementation can start.
- `fix-config`: Steam dev config is missing, tracked, or invalid.
- `fix-metadata`: lobby metadata contract is invalid.
- `implement-runtime`: preflight passes, but runtime subsystem is not implemented.
- `runtime-pass`: create/find/join/build-mismatch and authoritative join path are validated.

