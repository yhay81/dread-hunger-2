# Windows Dedicated Server Runbook

Use this after `FrostwakeServer` builds on the Windows workstation. This runbook is for local/LAN and private-test server validation before Steamworks, Steam Datagram Relay, or SteamCMD distribution are integrated.

## Status

- Target platform: Windows `Win64`.
- Server executable: `FrostwakeServer` after a successful Win64 server build.
- Current map: `L_IcebreakerWhitebox`.
- Current online layer: Phase 1 still uses `OnlineSubsystemNull` and LAN/listen-server style validation.
- Current gameplay telemetry covers match timer expiry and interactable rescue, containment, and release in listen-server smokes; dedicated-server validation must confirm the same JSONL event path works when the server is headless.
- Steam Lobby, Steam Server Browser, Steam Game Servers API, Steam Datagram Relay, SteamCMD Tool App distribution, and public server discovery are Phase 2+ work.

## Build Gate

Run from the repository root:

```powershell
cargo run -p frostwake-tools -- quality-gate --require-ue
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
```

Interpret `FrostwakeServer` result:

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

Validate any edited config before launch:

```powershell
cargo run -p frostwake-tools -- server-config-check Saved\Config\server_config.local.json
```

## Launch Shape

The intended config flag is:

```powershell
-ServerConfig="C:\path\to\server_config.local.json"
```

Expected command shape after the server executable exists:

```powershell
$ServerExe = ".\Binaries\Win64\FrostwakeServer.exe"
& $ServerExe `
  "/Game/Maps/L_IcebreakerWhitebox" `
  -log `
  -unattended `
  -NoLiveCoding `
  -nop4 `
  "-ServerConfig=$PWD\Saved\Config\server_config.local.json" `
  -FrostwakeRunId=windows-dedicated-local `
  "-FrostwakeBuildId=Frostwake-Win64-Development-local" `
  "-FrostwakeMapId=/Game/Maps/L_IcebreakerWhitebox" `
  -FrostwakeProfile=dedicated-private-test
```

If the executable path differs after packaging, record the actual path in the Windows validation result.

The same boot probe is wrapped by:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1
```

Useful overrides:

```powershell
.\Tools\windows\run_dedicated_server_validation.ps1 `
  -ServerExe ".\Binaries\Win64\FrostwakeServer.exe" `
  -ServerConfig ".\Saved\Config\server_config.local.json" `
  -DurationSeconds 30 `
  -Port 7777
```

The wrapper validates the server config, writes ignored evidence under `Saved\DedicatedServerValidation\...`, stops the server after the probe duration, and runs `cargo run -p frostwake-tools -- log-summary` if the configured event log is produced. Early blockers, including a missing server executable or missing config file, still write `summary.txt` and `manifest.json` in that evidence folder.

After boot validation passes, run the dedicated client join probe:

```powershell
.\Tools\windows\run_dedicated_client_join_validation.ps1
```

Useful overrides:

```powershell
.\Tools\windows\run_dedicated_client_join_validation.ps1 `
  -ServerExe ".\Binaries\Win64\FrostwakeServer.exe" `
  -ServerConfig ".\Saved\Config\server_config.local.json" `
  -UeRoot "D:\Epic Games\UE_5.7" `
  -Port 7777
```

The wrapper starts the server, launches one `UnrealEditor.exe -game` client against `127.0.0.1:<port>`, stops both processes, summarizes the server JSONL log, and fails if `client_connected` is not recorded. Early blockers also write `summary.txt` and `manifest.json` under `Saved\DedicatedClientJoinValidation\...`.

After the single-client join passes, run the 8-player ready-lobby dedicated probe:

```powershell
.\Tools\windows\run_dedicated_ready_validation.ps1
```

Useful overrides:

```powershell
.\Tools\windows\run_dedicated_ready_validation.ps1 `
  -ServerExe ".\Binaries\Win64\FrostwakeServer.exe" `
  -ServerConfig ".\Saved\Config\server_config.local.json" `
  -UeRoot "D:\Epic Games\UE_5.7" `
  -Clients 8 `
  -ExpectedPlayers 8 `
  -ExpectedSaboteurs 2 `
  -Port 7777
```

This wrapper starts a headless dedicated server with `-FrostwakeAutoReady`, launches eight `UnrealEditor.exe -game` clients, and fails unless the server JSONL records `role_assignment_complete` with `players=8`, `saboteurs=2`, plus `match_started`. Early blockers also write `summary.txt` and `manifest.json` under `Saved\DedicatedReadyValidation\...`, including the expected player/saboteur counts and per-client log paths.

When boot, client join, and ready-lobby behavior are understood, run the Phase 2 entry orchestration wrapper:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 `
  -SkipGenerate `
  -ServerExe ".\Binaries\Win64\FrostwakeServer.exe" `
  -ServerConfig ".\Saved\Config\server_config.local.json" `
  -Port 7777 `
  -Clients 8 `
  -ExpectedPlayers 8 `
  -ExpectedSaboteurs 2
```

The wrapper writes `Saved\Phase2EntryValidation\<timestamp>\manifest.json` with links to the child validation outputs. When a child probe writes its own `manifest.json`, the Phase 2 wrapper copies the child manifest path, decision, detail, output folder, and run id into the parent step result. A child `blocked` decision, such as a missing server executable, makes the top-level Phase 2 manifest `blocked` instead of a generic `fail`.

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
- command-line override: `-FrostwakeEventLog=C:\path\to\events.jsonl`
- smoke evidence: `Saved\SmokeTests\...`
- suite evidence: `Saved\SmokeSuites\...`
- crash notes: local `Saved\...` folders only

Telemetry path precedence is:

1. `-FrostwakeEventLog=...` command-line override, used by smoke and playtest scaffolds for isolated per-run logs.
2. `logPath` from `-ServerConfig=...`, used by dedicated-server validation.
3. Default `Saved\Logs\server.jsonl` if no override or config value is present.

Do not commit raw logs. Summarize with:

```powershell
cargo run -p frostwake-tools -- log-summary Saved\Logs\server.jsonl
```

If a suite produces `suite_summary.json`, export:

```powershell
cargo run -p frostwake-tools -- export-smoke-suite-markdown Saved\SmokeSuites\<suite-id>\suite_summary.json
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

Fill this after `FrostwakeServer` launches:

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
