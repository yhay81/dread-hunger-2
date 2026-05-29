# Server Hosting Model

> ⚠️ **SUPERSEDED IN PART (2026-05-29) — `docs/business-model.md` が正典。** 現方針は **公式AWSのみ・プレミアム作成ゲートのマルチプレイ**。本書の **Community Host Flow / 「official minimal + community」/ Phase Plan の community 行は "いったん廃止"（D8）**。Steam rendezvous・ServerConfig・moderation・SDR の記述は公式fleetに有効だが、**community 経路は現行計画外**。dedicated は **Linux** バイナリ＋opaque endpoint で将来の再導入を安く保つ。

## Direction

Use a Steam-native and game-native hosting model. Do not depend on external chat services for core play, matchmaking, server operations, or moderation.

## App Layout

```text
Game App ID
  - paid game client
  - Steam Lobby join/invite
  - in-game server browser

Dedicated Server Tool App ID
  - free server program
  - SteamCMD-compatible distribution
  - CLI start
  - community host ready
```

## Player Flow

1. Player opens the game from Steam.
2. Player chooses Quick Join, Friends Lobby, Host Game, or Server Browser.
3. Quick Join uses Steam Lobby or server metadata to place the player in a valid match.
4. Friends Lobby uses Steam invite flow.
5. Server Browser lists official and community dedicated servers.
6. In-game UI handles mute, report, kick prompts, and connection errors.

## Host Game Flow

1. Player selects lobby name, lobby type, map, public/private, region, password, and ruleset. Match size is fixed at 8 players.
2. Client starts the dedicated server executable when available.
3. Dedicated server reads `ServerConfig.json`.
4. Server registers with Steam server discovery.
5. Other players join through the in-game browser or Steam invite flow.
6. After match end, server continues or auto-shuts down depending on config.

Phase 1 can use listen server for speed. Phase 2 begins dedicated server validation.

## Community Host Flow

1. Downloads dedicated server Tool App when available.
2. Edits `ServerConfig.json`.
3. Starts the server with `-ServerConfig=/path/to/ServerConfig.json`.
4. Server registers itself with Steam server discovery.
5. Host manages local banlist and logs.

## Official Host Flow

1. Uses the same dedicated server binary.
2. Adds fleet metadata and log upload.
3. Uses official ban scope and moderation queue.
4. Can run behind Steam Datagram Relay when available.

## Required Config

Schema: [server_config.schema.json](../Tools/ops/server_config.schema.json)

```json
{
  "serverName": "Frostwake Test Server",
  "region": "jp",
  "maxPlayers": 8,
  "map": "L_IcebreakerWhitebox",
  "ruleset": "standard",
  "adminTokenEnv": "FROSTWAKE_ADMIN_TOKEN",
  "banlistPath": "Saved/Config/banlist.json",
  "logPath": "Saved/Logs/server.jsonl",
  "advertise": true,
  "autoShutdownAfterMatch": false
}
```

## Moderation

- Local mute is immediate and client-side.
- Kick is server-authoritative.
- Community bans stay local to that community server.
- Official bans require official backend review.
- Reports attach recent structured gameplay events, not raw voice.

## Phase Plan

| Phase | Hosting Scope |
| --- | --- |
| Phase 1 | Local listen server only |
| Phase 2 | Steam Lobby, first dedicated server target, local server browser prototype |
| Phase 3 | Steam Playtest, community host trial, dedicated server Tool App plan |
| Phase 4 | Early Access with official minimal servers plus community hosting |

## DDoS Posture

- Avoid official-server-only dependency.
- Prefer Steam Networking Sockets / Steam Datagram Relay for public servers when available.
- Keep community-hosted servers possible from the start of Early Access.
- Do not expose IP assumptions to gameplay code; keep connection endpoints abstract.

## Phase 2 Steam Gate

Before any public server browser work, record decisions for:

- Steam auth ticket and ownership verification.
- Dedicated-server client join validation on Windows.
- Dedicated Server Tool App setup, including Dedicated Server Redistributables and `steam_appid.txt`.
- SteamCMD anonymous download verification.
- SDR connection model: hosted dedicated server, FakeIP, or known data center.
- Whether a coordinator/backend is required to issue relay auth tickets.
