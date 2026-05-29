# Steam Dev Config Gate

P2-002 enables a safe path for Steam development without changing the default Phase 1 automation path.

## Policy

- `Frostwake.uproject` keeps `OnlineSubsystemSteam` explicit and disabled until the Windows Steam spike begins.
- `Config/DefaultEngine.ini` keeps `DefaultPlatformService=Null`.
- Steam AppID values, Steam server query ports, and Steam-only local settings stay in ignored local files under `Saved\Config\`.
- Steam work starts only after P2-001 and P2-005 pass on Windows.

## Default Gate

Run from the Windows repository root:

```powershell
.\Tools\windows\check_steam_dev_config.ps1
```

Expected result:

```text
[PASS] Default project config remains Null/LAN safe. No local Steam dev config checked.
```

## Local Steam Dev Config

Create a local ignored file:

```powershell
New-Item -ItemType Directory -Force Saved\Config | Out-Null
@'
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
GameServerQueryPort=27015
bInitServerOnClient=true
'@ | Set-Content -Path Saved\Config\steam_dev.local.ini -Encoding UTF8
```

Validate it:

```powershell
.\Tools\windows\check_steam_dev_config.ps1 -SteamConfig Saved\Config\steam_dev.local.ini -RequireSteamConfig
```

This file is a local development input only. Do not commit it, and do not change `Config\DefaultEngine.ini` to Steam until the Steam Lobby spike has its own branch/gate and Phase 1 Null/LAN automation remains callable.

## Acceptance

- Default config gate passes.
- Local Steam config gate passes when the ignored file exists.
- Phase 1 smoke profiles still run with Null/LAN settings.
- P2-003 can use the local config as the next Steam Lobby create/find/join spike input.

