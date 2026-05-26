# Steam-Native Implementation Backlog

This backlog keeps the project aligned with the intended operational flow: launch from Steam, use in-game lobby/server browser, host or join dedicated servers, and avoid external chat-service dependency.

## Phase 1: Local And LAN Only

- Keep `OnlineSubsystemSteam` disabled.
- Use `OnlineSubsystemNull`, local PIE, LAN, and packaged listen-server tests.
- Validate replicated gameplay authority before adding Steam complexity.
- Use `ServerConfig.json` only as a local dedicated-server contract scaffold.
- Do not ship or depend on any public server list yet.

## Phase 2 Entry: Steam Lobby

- Enable Steam plugin only after UE Editor/build environment is verified.
- Add Steam AppID placeholder config for local dev only.
- Create/find/join Steam Lobby as the rendezvous layer.
- Store only lightweight lobby metadata:
  - build id
  - map id
  - ruleset
  - max players
  - join state
  - host endpoint token
- Validate lobby metadata against `Tools/ops/lobby_metadata.schema.json` before client travel.
- Reject build mismatch before travel.
- Keep game state authoritative inside Unreal server, not in lobby metadata.

## Phase 2: Dedicated Server Target

- Build and run `AbyssLockServer` locally.
- Confirm the same `GameMode` works without a local player.
- Run a client-join validation against the dedicated server before starting Steam server discovery work.
- Load config from `-ServerConfig=/path/to/ServerConfig.json`.
- Write JSONL logs to the configured path.
- Add admin-token-gated console/admin commands:
  - kick
  - mute marker
  - shutdown warning
  - rotate match
- Keep community host and official host behavior in the same binary.

## Phase 2: Steam Auth And SDR Design Spike

- Decide the dedicated-server connection model before public listing:
  - Hosted dedicated server flow.
  - FakeIP flow.
  - Known data center flow.
- Document whether a coordinator/backend is needed to issue Steam Datagram Relay auth tickets.
- Add ownership/auth ticket verification before trusting reports, server metadata, or moderation identities.
- Keep direct public IP/port assumptions out of gameplay code.

## Phase 3: Steam Server Browser And Tool App

- Register dedicated servers through Steam server discovery.
- Add in-game server browser filters:
  - region
  - official/community
  - player count
  - map
  - ruleset
  - password/private flag
- Prepare a Dedicated Server Tool App:
  - separate AppID
  - Dedicated Server Redistributables enabled
  - `steam_appid.txt` containing the base game AppID
  - SteamCMD install path
  - anonymous SteamCMD download verification
  - example config
  - launch script
  - local banlist path
  - log export path
- Keep official servers minimal and avoid official-server-only dependency.

## Phase 3/4: Steam Datagram Relay

- Add SDR after dedicated-server basics are stable.
- Keep gameplay code independent from raw IP assumptions.
- Treat endpoints as opaque connection targets.
- Document community-host limitations if SDR is not available for every host path.

## Not In Scope

- External chat-service room creation.
- External voice chat as the actual gameplay voice layer.
- Custom non-Unreal authoritative match server.
- Official servers as the only playable path.
- Community server logs as automatic global-ban evidence.
