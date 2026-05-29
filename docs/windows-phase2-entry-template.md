# Windows Phase 2 Entry Template

Use this on the Windows workstation when attempting Phase 2 entry. Copy the filled result into a new cycle record or a dated local note. Keep raw logs under ignored `Saved\` folders.

## Header

```text
Date:
Commit:
Branch:
Windows machine:
Unreal Engine path:
Unreal Engine source or Launcher:
Visual Studio version:
Steam dev config enabled: no / yes
```

## Recommended Command

Run from repository root:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate
```

If Unreal is not in a default path:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 -UeRoot "D:\Epic Games\UE_5.7" -SkipGenerate
```

Use `-IncludeSmoke` only when you want the wrapper to include the Phase 1 listen-server smoke set before dedicated probes.

Full override shape:

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

## Result Matrix

| Gate | P2 Item | Result | Evidence Path | Notes |
| --- | --- | --- | --- | --- |
| Phase 2 wrapper manifest | P2-001 | pass / blocked / fail | `Saved\Phase2EntryValidation\...\manifest.json` | Should list every child step, child evidence folder, child manifest, decision, and detail |
| Phase 1 Windows validation wrapper | P2-001 | pass / blocked / fail | `Saved\Phase2EntryValidation\...` plus `Saved\WindowsValidation\...` |  |
| Win64 server target | P2-001 | pass / blocked / fail | `unreal_gate_win64.log` | If blocked, record Launcher vs source build |
| Dedicated server boot | P2-005 | pass / blocked / fail | `Saved\DedicatedServerValidation\...` |  |
| Dedicated client join | P2-005 | pass / blocked / fail | `Saved\DedicatedClientJoinValidation\...` |  |
| Dedicated ready-lobby 8-player start | P2-005 | pass / blocked / fail | `Saved\DedicatedReadyValidation\...` | Expected 8 clients, 2 saboteurs, `match_started` |

## Expected Dedicated Ready-Lobby Events

```text
session_started:
client_connected count:
player_ready_changed count:
role_assignment_complete players/saboteurs:
match_timer_started:
match_started:
```

## Manifest Check

```text
Manifest path:
phase1_windows_validation child output:
dedicated_server_boot child output:
dedicated_server_boot child manifest decision/detail:
dedicated_client_join child output:
dedicated_client_join child manifest decision/detail:
dedicated_ready_lobby child output:
dedicated_ready_lobby child manifest decision/detail:
```

## Decision

Choose one:

- `enter-phase-2`: P2-001 and P2-005 passed; start P2-002 Steam dev config.
- `fix-windows-build`: Win64 Editor/Game/Server build failed in project code.
- `install-source-ue`: Launcher UE blocks server target; install/build UE source distribution.
- `fix-dedicated-runtime`: server target builds, but boot/join/ready probes fail.

## Next Action

```text
Owner:
Next issue:
Command to run next:
```

For P2-002, the first command should be:

```powershell
.\Tools\windows\check_steam_dev_config.ps1
```
