# Windows Dedicated Server Runbook

Use this after `AbyssLockServer` builds on the Windows workstation. This runbook is for local/LAN and private-test server validation before Steamworks, Steam Datagram Relay, or SteamCMD distribution are integrated.

## Status

- Target platform: Windows `Win64`.
- Server executable: `AbyssLockServer` after a successful Win64 server build.
- Current map: `L_IcebreakerWhitebox`.
- Current online layer: Phase 1 still uses `OnlineSubsystemNull` and LAN/listen-server style validation.
- Steam Lobby, Steam Server Browser, Steam Game Servers API, Steam Datagram Relay, SteamCMD Tool App distribution, and public server discovery are Phase 2+ work.

## Build Gate

Run from the repository root:

```powershell
py -3 Tools\quality_gate.py --require-ue
py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server
```

Interpret `AbyssLockServer` result:

| Result | Meaning | Action |
| --- | --- | --- |
| pass | Windows can build server target | Continue to local dedicated-server launch |
| blocked | Launcher UE does not support Server target | Install/build UE source distribution and set `UE_ROOT` |
| fail | Server target is supported but project failed | Fix compile errors before any playtest |

## Server Config

Create a local config from the committed example. Keep secrets out of git.

```powershell
New-Item -ItemType Directory -Force Saved\Config | Out-Null
Copy-Item Tools\ops\server_config.example.json Saved\Config\server_config.local.json
```

Required fields are defined by `Tools\ops\server_config.schema.json`:

- `serverName`
- `region`
- `maxPlayers`, maximum `8`
- `map`
- `ruleset`, one of `standard`, `private_test`, or `developer`

Recommended local test values:

```json
{
  "serverName": "Frostwake Windows Test Server",
  "region": "jp",
  "maxPlayers": 8,
  "map": "L_IcebreakerWhitebox",
  "ruleset": "private_test",
  "adminTokenEnv": "FROSTWAKE_ADMIN_TOKEN",
  "banlistPath": "Saved/Config/banlist.json",
  "logPath": "Saved/Logs/server.jsonl",
  "advertise": false,
  "autoShutdownAfterMatch": false
}
```

## Launch Shape

The intended config flag is:

```powershell
-ServerConfig="C:\path\to\server_config.local.json"
```

Expected command shape after the server executable exists:

```powershell
$ServerExe = ".\Binaries\Win64\AbyssLockServer.exe"
& $ServerExe `
  "/Game/Maps/L_IcebreakerWhitebox" `
  -log `
  -unattended `
  -NoLiveCoding `
  -nop4 `
  "-ServerConfig=$PWD\Saved\Config\server_config.local.json"
```

If the executable path differs after packaging, record the actual path in the Windows validation result.

## Ports And Firewall

Concrete public-server port policy is TBD until the Windows server target is verified and Steam networking decisions are made.

For local/LAN experiments only:

- record the chosen game port in the validation note;
- allow it through Windows Firewall only for private networks;
- do not publish public IP or host details in committed docs;
- do not design around exposed public UDP ports until Steam Datagram Relay / Steam Networking Sockets is integrated.

## Logging

Use structured logs for every validation run:

- config log path: `Saved\Logs\server.jsonl`
- smoke evidence: `Saved\SmokeTests\...`
- suite evidence: `Saved\SmokeSuites\...`
- crash notes: local `Saved\...` folders only

Do not commit raw logs. Summarize with:

```powershell
py -3 Tools\log_summary.py Saved\Logs\server.jsonl
```

If a suite produces `suite_summary.json`, export:

```powershell
py -3 Tools\ue\export_smoke_suite_markdown.py Saved\SmokeSuites\<suite-id>\suite_summary.json
```

## Community Hosting Direction

The product direction remains:

- Game App for players.
- Dedicated Server Tool App for free server distribution.
- SteamCMD-compatible install path.
- CLI-startable server with config file.
- Community-hostable servers from the start of public testing.
- Official servers kept minimal.

This runbook only prepares the local/private-test version of that model.

## Deferred Until Steamworks / SDR

Do not block Phase 1 Windows validation on these:

- Steam Lobby.
- Steam Server Browser / Game Servers API registration.
- Steam auth ticket verification.
- Steam Datagram Relay / Steam Networking Sockets.
- SteamCMD anonymous dedicated-server download.
- Tool App ID setup.
- Official fleet metadata and server health dashboard.
- Global ban sharing and moderation queue.
- Quick Join backed by Steam server metadata.

## First Dedicated Server Evidence To Capture

Fill this after `AbyssLockServer` launches:

```text
Commit:
UE_ROOT:
Launcher or source UE:
Server exe path:
Config path:
Map:
Ruleset:
Max players:
Advertise:
Port/firewall setting:
Server booted: yes / no
Client joined: yes / no
Match started: yes / no
Log summary path:
Crash/ensure lines:
Decision:
```
