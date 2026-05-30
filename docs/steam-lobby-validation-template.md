# Steam Lobby Validation Template

Use this for P2-003/P2-004 evidence on the Windows workstation. The current wrapper is a preflight until the Steam create/find/join runtime path is implemented.

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
  -ExpectedBuildId Frostwake-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox
```

Expected result:

- Steam dev config validation passes.
- Lobby metadata validation passes.
- Accepted join decision passes with `travelAllowed=true`.
- Build and map mismatch probes pass with `BuildMismatch` and `MapMismatch` before travel.
- `Saved\SteamLobbyValidation\<timestamp>\manifest.json` is written.
- Decision is `preflight_pass`.

## Runtime Command

Use this only after the Steam create/find/join provider path is implemented:

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId Frostwake-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox `
  -Runtime
```

## Runtime Evidence Matrix

| Gate | Result | Evidence Path | Notes |
| --- | --- | --- | --- |
| Steam dev config | pass / fail | `Saved\SteamLobbyValidation\...\steam_dev_config.log` |  |
| Lobby metadata contract | pass / fail | `Saved\SteamLobbyValidation\...\lobby_metadata.log` |  |
| Join decision | pass / fail | `Saved\SteamLobbyValidation\...\lobby_join_decision.log` | Compatible row returns `travelAllowed=true` |
| Build mismatch rejection | pass / fail | `Saved\SteamLobbyValidation\...\lobby_join_build_mismatch.log` | `BuildMismatch` before travel |
| Map mismatch rejection | pass / fail | `Saved\SteamLobbyValidation\...\lobby_join_map_mismatch.log` | `MapMismatch` before travel |
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
- `implement-runtime`: preflight passes, but Steam create/find/join runtime is not implemented.
- `runtime-pass`: create/find/join/build-mismatch and authoritative join path are validated.

