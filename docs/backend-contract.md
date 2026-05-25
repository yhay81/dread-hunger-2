# Backend Contract

The backend is not the match server. Unreal Dedicated Server remains authoritative for movement, roles, inventory, damage, ship systems, sabotage, and win/loss.

The TypeScript backend is allowed to handle:

- health checks;
- playtest report uploads after a match;
- moderation reports;
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
| `GET /v1/banlists/{scope}` | Fetch ban metadata for official/private scopes | Phase 2 |
| `GET /v1/news` | Patch notes and announcements | Phase 2 |
| `GET /v1/fleet/servers` | Official fleet metadata, not Steam server discovery replacement | Phase 2+ |

## Data Rules

- Raw voice, recordings, transcripts, SteamIDs, IP addresses, and personal identifiers must not be sent from Phase 1 local tests.
- Playtest report payloads should use run IDs, build IDs, aggregate counts, and anonymized notes only.
- Moderation report storage needs retention, access control, and abuse-review workflow before public testing.
- Steam auth ticket verification is Phase 2+ and must be added before public services trust client identity.

## Local Development Direction

Current local prototype:

- TypeScript under `apps/backend`.
- Node HTTP server with no runtime dependencies.
- In-memory storage only.
- `npm test` builds the service and runs Node's built-in test runner.
- Responses include `x-frostwake-authority: non-authoritative`.
- Playtest and moderation summaries reject obvious raw identity data patterns.

Future stack options:

- Fastify, Hono, or NestJS.
- SQLite for local prototype or PostgreSQL when hosted.
- Zod or JSON Schema validation.
- Structured JSON logs.

Keep the backend optional for Phase 1. Windows Unreal validation and P1-024 human testing must not depend on the backend.
