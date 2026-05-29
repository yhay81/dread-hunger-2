# Backend Contract

The backend is not the match server. Unreal Dedicated Server remains authoritative for movement, roles, inventory, damage, ship systems, sabotage, and win/loss.

The Rust backend is allowed to handle:

- health checks;
- playtest report uploads after a match;
- moderation reports;
- mocked Steam identity proof contract for local Phase 2 integration rehearsal;
- non-authoritative matchmaking lobby directory for named 8-player lobbies;
- ban list metadata;
- public news/patch notes;
- optional official fleet metadata after Steam integration.

Do not put realtime match authority, player movement, combat, inventory, role visibility, or voice transport in this service.

## Initial API Surface

Source contract: `apps/backend/openapi.yaml`.

| Endpoint | Purpose | Phase |
| --- | --- | --- |
| `GET /healthz` | Deployment and local process health | Phase 1+ |
| `POST /v1/playtest-reports` | Upload anonymized summary metadata after a test | Phase 1+ |
| `POST /v1/moderation/reports` | Submit abuse/report metadata | Phase 2 |
| `POST /v1/identity/steam/session-ticket` | Verify the local mock Steam ticket contract and return only a subject hash/proof | Phase 2+ |
| `GET /v1/matchmaking/lobbies` | List named lobby-directory entries for matchmaking | Phase 2 |
| `POST /v1/matchmaking/lobbies` | Open a named 8-player casual or standard lobby | Phase 2 |
| `POST /v1/matchmaking/lobbies/{lobbyId}/join` | Join a lobby; standard requires at least 50 completed matches | Phase 2 |
| `POST /v1/matchmaking/lobbies/{lobbyId}/leave` | Leave a lobby and delete it when empty | Phase 2 |
| `GET /v1/banlists/{scope}` | Fetch ban metadata for official/private scopes | Phase 2 |
| `GET /v1/news` | Patch notes and announcements | Phase 2 |
| `GET /v1/fleet/servers` | Official fleet metadata, not Steam server discovery replacement | Phase 2+ |

## Data Rules

- Raw voice, recordings, transcripts, SteamIDs, IP addresses, and personal identifiers must not be sent from Phase 1 local tests.
- Playtest report payloads should use run IDs, build IDs, aggregate counts, and anonymized notes only.
- Moderation report payloads may include structured review metadata such as match/build/map IDs, local player slots, match time, voice state, immediate action taken, and recent server event IDs. They must not include raw voice, transcripts, recording URLs, SteamIDs, IP addresses, or free-form private identifiers.
- Matchmaking lobby records may store local pseudonymous IDs for slot counting only; Unreal still owns connection, readiness, role assignment, and match start.
- Lobby listings must not expose connection `endpointToken`; the token is returned only after a successful create/join response.
- Moderation report storage needs retention, access control, and abuse-review workflow before public testing.
- Steam auth ticket verification is Phase 2+ and must be added before public services trust client identity.

## Local Development Direction

Current local prototype:

- Rust under `apps/backend`.
- Axum/Tokio HTTP server.
- In-memory storage only.
- `cargo test -p frostwake-backend` builds the service and runs API tests.
- Responses include `x-frostwake-authority: non-authoritative`.
- Playtest summaries and moderation payload string fields reject obvious raw identity data patterns.
- Moderation reports accept structured voice/trust evidence fields while rejecting unknown raw voice/transcript fields through the OpenAPI-backed allowlist.
- The Steam identity endpoint is currently a deterministic local mock; it requires explicit backend mock config and never echoes raw ticket or SteamID material.
- Unreal uses `UFrostwakeSteamIdentitySubsystem` as the current helper contract for the Steam identity endpoint. Its request and telemetry helpers redact ticket material and expose only purpose, endpoint, ticket byte count, proof/hash presence, HTTP status, and pass/fail metadata.
- Matchmaking lobbies are fixed at 8 players, have a player-facing name, and split into `casual` and `standard`; `standard` requires 50 completed matches.

Future stack options:

- SQLite for local prototype or PostgreSQL when hosted.
- Structured JSON logs.

Keep the backend optional for Phase 1. Windows Unreal validation and P1-024 human testing must not depend on the backend.
