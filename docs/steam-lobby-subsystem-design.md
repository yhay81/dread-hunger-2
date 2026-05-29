# Steam Lobby Subsystem Design

This is the implementation boundary for P2-003. It prepares the Windows Steam Lobby spike without enabling Steam by default on Mac or changing Phase 1 Null/LAN validation.

## Goal

Create/find/join a Steam Lobby as a rendezvous layer, validate lobby metadata before travel, and enter the existing Unreal authoritative server path. Matches are 8 players fixed. The lobby must never own match rules or secret state.

## C++ Boundary

`UFrostwakeLobbySubsystem`

- Type: `UGameInstanceSubsystem`.
- Owns lobby lifecycle calls.
- Talks to Unreal Online Subsystem / Steam implementation behind compile/runtime gates.
- Emits telemetry through `UFrostwakeTelemetrySubsystem`.
- Exposes Blueprint-callable functions for UI.
- Does not assign roles, start matches, mutate inventory, or decide win conditions.

`FFrostwakeLobbyMetadata`

- C++ mirror of `Tools/ops/lobby_metadata.schema.json`.
- Fields: schema version, lobby name, lobby type, build id, map id, ruleset, max/current players, minimum completed matches, join state, connection mode, official flag, passworded flag, endpoint token, optional region, optional server name.
- Converts to/from key-value metadata strings used by the Online Subsystem provider.

`EFrostwakeLobbyRejectReason`

- `None`
- `SteamUnavailable`
- `InvalidMetadata`
- `BuildMismatch`
- `MapMismatch`
- `LobbyFull`
- `LobbyLocked`
- `AlreadyInMatch`
- `EndpointUnavailable`
- `TravelFailed`

## Current C++ Foundation

`Source/Frostwake/FrostwakeLobbySubsystem.*` now contains the Null/LAN-safe C++ mirror for the metadata and rejection contract:

- `FFrostwakeLobbyMetadata` mirrors the schema fields.
- `FFrostwakeLobbyJoinDecision` exposes `travelAllowed`, `rejectReason`, `rejectReasonCode`, message, and endpoint availability without logging endpoint tokens.
- `UFrostwakeLobbySubsystem::EvaluateJoinMetadata` mirrors `cargo run -p frostwake-tools -- lobby-join-decision` for `InvalidMetadata`, `BuildMismatch`, `MapMismatch`, `LobbyFull`, `LobbyLocked`, `AlreadyInMatch`, and `EndpointUnavailable`.
- `ToKeyValueMetadata` / `FromKeyValueMetadata` prepare the future Online Subsystem provider handoff.
- `IsSteamLobbyRuntimeAvailable` intentionally returns false until the Steam runtime create/find/join path is implemented.

## Blueprint Boundary

Blueprint may:

- call `CreateLobby`, `FindLobbies`, `JoinLobby`, `LeaveLobby`;
- display lobby rows;
- display reject/failure reasons;
- trigger travel only through subsystem success callbacks.

Blueprint must not:

- parse arbitrary provider metadata directly;
- bypass build/map validation;
- write role, health, inventory, sabotage, report, voice, SteamID, or IP data into lobby metadata;
- start the match directly from lobby UI.

## Server Authority Boundary

Steam Lobby is pre-match rendezvous only.

- `AFrostwakeGameMode` remains the only owner of role assignment and match start.
- `AFrostwakeGameState` remains the replicated public match state.
- `AFrostwakePlayerState` remains the player ready/role/life state owner.
- Dedicated server ready-lobby validation remains the baseline for true match flow.

## Telemetry Events

Use `UFrostwakeTelemetrySubsystem::LogEvent` with JSON payloads and no personal identifiers.

| Event | Required Payload |
| --- | --- |
| `lobby_create_requested` | `lobbyType`, `buildId`, `mapId`, `ruleset`, `maxPlayers`, `minimumCompletedMatches`, `connectionMode` |
| `lobby_create_succeeded` | `lobbyIdHash`, `metadataSchemaVersion`, `joinState` |
| `lobby_create_failed` | `reason` |
| `lobby_search_requested` | `lobbyType`, `buildId`, `mapId`, `ruleset`, `maxResults` |
| `lobby_search_completed` | `resultCount`, `validCount`, `rejectedCount` |
| `lobby_metadata_rejected` | `reason`, `buildId`, `mapId`, `joinState` |
| `lobby_join_requested` | `lobbyIdHash`, `buildId`, `mapId` |
| `lobby_join_rejected` | `lobbyIdHash`, `reason` |
| `lobby_join_succeeded` | `lobbyIdHash`, `connectionMode` |
| `lobby_travel_started` | `connectionMode`, `mapId` |
| `lobby_travel_failed` | `reason` |

`lobbyIdHash` should be a short non-reversible diagnostic token. Do not log raw Steam Lobby IDs, SteamIDs, auth tickets, IP addresses, or player names.

## Validation Flow

1. UI requests create/find/join through `UFrostwakeLobbySubsystem`.
2. Subsystem builds or reads `FFrostwakeLobbyMetadata`.
3. Subsystem validates metadata against the same rules as `cargo run -p frostwake-tools -- lobby-metadata-check`.
4. Subsystem mirrors `cargo run -p frostwake-tools -- lobby-join-decision` reason codes before travel.
5. Subsystem rejects mismatched build or map before travel.
6. Subsystem resolves the opaque endpoint token through the active provider.
7. Client travels to the server path.
8. Existing `client_connected`, `player_ready_changed`, `role_assignment_complete`, and `match_started` telemetry proves the authoritative path.

## Windows Acceptance Commands

Before runtime Steam Lobby work:

```powershell
.\Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate
.\Tools\windows\check_steam_dev_config.ps1
.\Tools\windows\check_steam_dev_config.ps1 -SteamConfig Saved\Config\steam_dev.local.ini -RequireSteamConfig
cargo run -p frostwake-tools -- lobby-metadata-check Tools\ops\lobby_metadata.example.json --expected-build-id AbyssLock-Win64-Development-local --expected-map-id L_IcebreakerWhitebox
cargo run -p frostwake-tools -- lobby-join-decision Tools\ops\lobby_metadata.example.json --expected-build-id AbyssLock-Win64-Development-local --expected-map-id L_IcebreakerWhitebox --require-accepted --json
```

P2-003 preflight command:

```powershell
.\Tools\windows\run_steam_lobby_validation.ps1 `
  -SteamConfig Saved\Config\steam_dev.local.ini `
  -ExpectedBuildId AbyssLock-Win64-Development-local `
  -ExpectedMapId L_IcebreakerWhitebox
```

Add `-Runtime` only after the Steam create/find/join provider path is implemented. The runtime path should fail unless telemetry includes create, search, join, metadata validation, travel, `client_connected`, and ready-lobby match-start evidence.

## Not In This Spike

- Steam Server Browser.
- Steam Game Servers API registration.
- Steam Datagram Relay.
- Ownership/auth ticket trust for moderation.
- Dedicated Server Tool App packaging.
- Voice chat.
