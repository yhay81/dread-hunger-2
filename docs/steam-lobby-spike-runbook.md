# Steam Lobby Spike Runbook

> 🧭 **Superseded where it conflicts with [`docs/frostwake-modernization-plan.md`](frostwake-modernization-plan.md) (SSOT, 2026-05-30).** The GP-04 / GP-02 lane framing and the "update `docs/orchestration/lanes/GP-04.state.md`" step are frozen (orchestration is historical); the preflight / runtime steps and safety rules below remain valid, and the standing server-build blocker is real (plan §0/§4).

GP-04 · P2-003 + P2-004 · Status: AWAITING GP-02 UNBLOCK

This runbook contains the exact steps to run the Steam Lobby create/find/join spike with
build/map-mismatch rejection once GP-02 provides a server-capable UE build. Every step is
grounded in the contract docs and Rust tools already in the repo.

**Relevant source docs**

- `docs/steam-lobby-subsystem-design.md` — C++ boundary, validation flow, telemetry events
- `docs/steam-lobby-metadata-contract.md` — schema fields, validator rules, acceptance criteria
- `docs/steam-dev-config-gate.md` — Null/LAN safety invariant + local Steam dev config path
- `docs/steam-lobby-validation-template.md` — evidence header + runtime evidence matrix
- `docs/phase2-backlog.md` — P2-003/P2-004 done-when criteria
- `docs/windows-dedicated-server-runbook.md` — GP-02 unblock path (server build)
- `Tools/ops/lobby_metadata.schema.json` — authoritative metadata schema
- `Tools/ops/lobby_metadata.example.json` — passing example payload
- `Source/Frostwake/FrostwakeLobbySubsystem.h` + `.cpp` — Unreal C++ mirror (null-safe today)
- `Tools/windows/run_steam_lobby_validation.ps1` — preflight + runtime wrapper

---

## Safety Rules (read before every step)

1. **Null/LAN must stay safe.** `Config/DefaultEngine.ini` keeps `DefaultPlatformService=Null`.
   `Frostwake.uproject` keeps `OnlineSubsystemSteam: Enabled: false`. Do not change either.
2. **Steam dev config is local only.** `Saved\Config\steam_dev.local.ini` is gitignored.
   Never commit it, never commit AppIDs, SteamIDs, Steam server query ports, or publisher keys.
3. **Lobby metadata is rendezvous-only.** It must not contain role state, inventory, health,
   sabotage state, player identity, auth tickets, raw IP addresses, voice data, reports, or
   match authority. Gameplay authority stays in `AFrostwakeGameMode`.
4. **No raw endpoint tokens in logs.** `lobbyIdHash` is a short non-reversible diagnostic
   token. Do not log raw Steam Lobby IDs, SteamIDs, auth tickets, or IP addresses.
5. **Do not trust lobby state for gameplay.** `IsSteamLobbyRuntimeAvailable` returning `true`
   does not grant `AFrostwakeGameMode` authority — that authority path is unchanged.
6. **Do not skip the preflight.** The `-Runtime` flag must not be used before all five
   preflight steps (Steps 1-5 below) are green.

---

## Part A — Pre-Runtime Preflight (runnable today without GP-02)

These steps exercise the entire contract and rejection logic without a live Steam session.
They should be re-confirmed on the working tree before attempting Part B.

### Step 1 — Confirm Null/LAN config safety

```powershell
.\Tools\windows\check_steam_dev_config.ps1
```

Expected output line:

```text
[PASS] Default project config remains Null/LAN safe. No local Steam dev config checked.
```

This verifies `Config/DefaultEngine.ini` has `DefaultPlatformService=Null` and
`Frostwake.uproject` has `OnlineSubsystemSteam: Enabled: false`.

### Step 2 — Validate lobby metadata contract

```powershell
$env:CARGO_TARGET_DIR = 'target\loop-gp04'
cargo run -p frostwake-tools -- lobby-metadata-check `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id Frostwake-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox `
  --json
```

Expected: exit 0. JSON output includes `"valid": true`, `"buildId"` and `"mapId"` match
the expected values.

Validator rejects a payload that has:

- missing required fields (see schema at `Tools/ops/lobby_metadata.schema.json`)
- `maxPlayers` not exactly `8`
- `currentPlayers` above `maxPlayers`
- `lobbyType=standard` with `minimumCompletedMatches` below `50`
- `joinState=open` while already full, or `joinState=full` while below capacity
- `endpointToken` that looks like a raw `IP:port`
- `buildId` or `mapId` mismatch when the expected flags are supplied

### Step 3 — Run accepted join decision (compatible row)

```powershell
cargo run -p frostwake-tools -- lobby-join-decision `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id Frostwake-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox `
  --require-accepted `
  --json
```

Expected: exit 0. JSON output includes `"travelAllowed": true`, `"rejectReason": "None"`.

### Step 4 — Run build-mismatch rejection probe

```powershell
cargo run -p frostwake-tools -- lobby-join-decision `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id DifferentBuild-mismatch-probe `
  --expected-map-id L_IcebreakerWhitebox `
  --expected-reject-reason BuildMismatch `
  --json
```

Expected: exit 0. JSON output includes `"travelAllowed": false`,
`"rejectReason": "BuildMismatch"`. This proves a client with a different build is rejected
before travel, with a logged reason.

### Step 5 — Run map-mismatch rejection probe

```powershell
cargo run -p frostwake-tools -- lobby-join-decision `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id Frostwake-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox-mismatch-probe `
  --expected-reject-reason MapMismatch `
  --json
```

Expected: exit 0. JSON output includes `"travelAllowed": false`,
`"rejectReason": "MapMismatch"`. This proves a client requesting a different map is rejected
before travel.

### Step 6 — Run the full preflight wrapper

The wrapper runs Steps 1-5 in one pass and writes a dated manifest.

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId Frostwake-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox
```

If `Saved\Config\steam_dev.local.ini` does not yet exist, create it first (see
`docs/steam-dev-config-gate.md` "Local Steam Dev Config" section for the content block),
or omit the `-SteamConfig` and `-RequireSteamConfig` flags to run the Null-only preflight.

Expected manifest at `Saved\SteamLobbyValidation\<timestamp>\manifest.json`:

```json
{
  "decision": "preflight_pass",
  "steps": [
    { "name": "steam_dev_config",          "exitCode": 0 },
    { "name": "lobby_metadata",            "exitCode": 0 },
    { "name": "lobby_join_decision",       "exitCode": 0 },
    { "name": "lobby_join_build_mismatch", "exitCode": 0 },
    { "name": "lobby_join_map_mismatch",   "exitCode": 0 }
  ]
}
```

`Saved\SteamLobbyValidation\` is gitignored; do not commit its contents.

---

## Part B — GP-02 Unblock Prerequisites

These must all be true before Step 7 (runtime spike):

| Prerequisite | How to confirm |
| --- | --- |
| Source-built or server-capable UE is installed | `cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server` exits 0 |
| `UE_ROOT` env var points at that install | Same command reads `UE_ROOT` |
| `FrostwakeServer.exe` exists | `Test-Path ".\Binaries\Win64\FrostwakeServer.exe"` returns True |
| Dedicated server boots and a client connects | `.\Tools\windows\run_dedicated_client_join_validation.ps1` exits 0 (see `docs/windows-dedicated-server-runbook.md`) |
| 8-client ready-lobby match starts headlessly | `.\Tools\windows\run_dedicated_ready_validation.ps1` exits 0 with `role_assignment_complete` + `match_started` in JSONL |

Until these pass, stop at Part A. Do not enable `OnlineSubsystemSteam` in default config.

---

## Part C — Runtime Steam Lobby Spike

Do not begin Part C until all Part B prerequisites are confirmed and `IsSteamLobbyRuntimeAvailable`
has been updated to return `true` behind a new compile/runtime gate in
`Source/Frostwake/FrostwakeLobbySubsystem.cpp`.

### Step 7 — Implement the Online Subsystem Steam provider path

Implement `UFrostwakeLobbySubsystem::CreateLobby`, `FindLobbies`, `JoinLobby`, and `LeaveLobby`
in `Source/Frostwake/FrostwakeLobbySubsystem.cpp` per the boundary spec in
`docs/steam-lobby-subsystem-design.md` (C++ Boundary + Validation Flow sections).

Requirements before marking implemented:

- `IsSteamLobbyRuntimeAvailable()` returns `true` only when the Steam Online Subsystem is
  loaded and a valid SteamID session is active.
- `EvaluateJoinMetadata` is called on every join candidate before any server travel.
- Telemetry events listed in `docs/steam-lobby-subsystem-design.md` (Telemetry Events
  section) are emitted through `UFrostwakeTelemetrySubsystem::LogEvent` without personal
  identifiers or raw endpoint tokens.
- `ToKeyValueMetadata` / `FromKeyValueMetadata` are used for all provider metadata handoff.
- Blueprint can call `CreateLobby`, `FindLobbies`, `JoinLobby`, `LeaveLobby` and receive
  reject/failure reasons; Blueprint must not parse raw provider metadata or write
  role/health/inventory/sabotage data into lobby metadata fields.

### Step 8 — Create the local Steam dev config

If not already present:

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
.\Tools\windows\check_steam_dev_config.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig
```

Expected: `[PASS]`. This file is gitignored. Do not commit it.

### Step 9 — Confirm Steam client is running

The Steam client must be running on the Windows workstation before the runtime spike.
The dev AppID `480` (Spacewar) is used for local testing; it allows Steam Lobby creation
without a shipped game AppID. The runtime spike does not require a public Steam listing.

### Step 10 — Run the full runtime wrapper

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -RequireSteamConfig `
  -ExpectedBuildId Frostwake-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox `
  -Runtime
```

The wrapper currently exits 9 (`not_implemented`) at the `-Runtime` flag. It will pass
once Step 7 is complete. Do not modify the wrapper to skip the runtime check.

### Step 11 — Verify JSONL telemetry events

After the wrapper exits 0, confirm the JSONL event log (default path:
`Saved\Logs\server.jsonl`, or the path set by `-FrostwakeEventLog` / server config `logPath`)
contains all of the following events in order:

| Event | What it proves |
| --- | --- |
| `lobby_create_requested` | Subsystem began create flow |
| `lobby_create_succeeded` | Steam returned a valid lobby handle (logged as `lobbyIdHash`) |
| `lobby_search_requested` | Second client began find flow |
| `lobby_search_completed` | Find returned at least one valid result |
| `lobby_metadata_rejected` | A build-mismatch or map-mismatch probe was rejected before travel |
| `lobby_join_requested` | Second client began join flow |
| `lobby_join_succeeded` | Second client joined the lobby |
| `lobby_travel_started` | Second client began server travel |
| `client_connected` | Second client reached the authoritative Unreal server path |
| `role_assignment_complete` | GameMode assigned roles (server authority) |
| `match_started` | GameMode started the match (server authority) |

Summarize the log with:

```powershell
cargo run -p frostwake-tools -- log-summary Saved\Logs\server.jsonl
```

### Step 12 — Fill the Runtime Evidence Matrix

Open `docs/steam-lobby-validation-template.md` and fill the Runtime Evidence Matrix table
with the result (pass/fail), evidence path, and any notes for each gate row. Set the
Decision field at the bottom to `runtime-pass` if all rows pass.

---

## Part D — Post-Spike Checks

After the runtime spike passes (manifest `decision=runtime_pass`):

1. Re-run the Null/LAN preflight without `-SteamConfig` or `-Runtime` to confirm the
   default config is still safe:

   ```powershell
   .\Tools\windows\run_steam_lobby_validation.ps1 `
     -ExpectedBuildId Frostwake-Win64-Development-local `
     -ExpectedMapId L_IcebreakerWhitebox
   ```

   Expected manifest: `"decision": "preflight_pass"`.

2. Confirm `Config/DefaultEngine.ini` still has `DefaultPlatformService=Null`.

3. Confirm `Frostwake.uproject` still has `OnlineSubsystemSteam: Enabled: false` (the
   runtime path uses the local ignored ini override, not the default config).

4. Update `docs/orchestration/lanes/GP-04.state.md`: set milestone progress to
   `RUNTIME PASS`, record the manifest path as Latest Evidence, and advance the Next Action
   to P2-006 (Steam auth ticket) or P2-007 design gate (server browser) per the backlog.

---

## Expected Manifests and Decisions

| Stage | Manifest decision | What it means |
| --- | --- | --- |
| Part A complete | `preflight_pass` | Contract, config safety, metadata, and rejection probes all pass |
| Part B incomplete | n/a — do not run `-Runtime` | GP-02 blocker still active |
| Step 7 not yet done | `not_implemented` (exit 9) | Wrapper reached `-Runtime` but subsystem not implemented |
| Part C complete | `runtime_pass` | Full create/find/join + rejection probes + authoritative server path verified |
| Any step failed | `fail` | Check the named step's `.log` file in `Saved\SteamLobbyValidation\<timestamp>\` |

---

## What Stays Out of This Spike

Per `docs/steam-lobby-subsystem-design.md` (Not In This Spike section) and
`docs/phase2-backlog.md`:

- Steam Server Browser (P2-007/P2-009) — deferred until runtime pass
- Steam Game Servers API registration (P2-008)
- Steam Datagram Relay / Steam Networking Sockets (P2-012/P2-013)
- Steam auth ticket trust for moderation (P2-006 — separate, lower priority)
- Dedicated Server Tool App packaging (P2-010/P2-011)
- Voice chat (P2-014/P2-015 — GP-05 lane)
- Any public server discovery or IP exposure

---

## Authority and Trust Boundaries

These invariants must hold throughout the spike and must not be relaxed to make the spike
pass:

- `AFrostwakeGameMode` is the only owner of role assignment and match start.
- `AFrostwakeGameState` is the replicated public match state owner.
- `AFrostwakePlayerState` is the player ready/role/life state owner.
- Steam Lobby is pre-match rendezvous only: it brings clients to the same server travel
  target. It does not assign roles, start matches, mutate inventory, or decide win
  conditions.
- A `lobby_join_succeeded` event does not mean the match has started. The authoritative
  path (`client_connected` + `role_assignment_complete` + `match_started`) is the only
  proof that a client reached the server.
