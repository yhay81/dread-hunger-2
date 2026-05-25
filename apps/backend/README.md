# Frostwake Backend

TypeScript backend prototype.

This service is for reports, bans, stats, admin APIs, patch notes, surveys, and optional fleet metadata. It is not the authoritative match server.

Initial API contract: [openapi.yaml](openapi.yaml)

Design notes: [Backend Contract](../../docs/backend-contract.md)

Current local implementation:

- TypeScript
- Node HTTP server with no runtime dependencies
- In-memory storage only
- JSON validation for anonymized playtest reports and moderation metadata
- Redaction guardrails for IP addresses, SteamID-like values, emails, local user paths, and secret-like assignments
- Non-authoritative response header: `x-frostwake-authority: non-authoritative`

Local commands:

```bash
npm ci
npm test
npm start
```

Default local URL:

```text
http://localhost:8787
```

Optional playtest report upload dry-run from repository root. The tool accepts a summary JSON or raw JSONL events:

```bash
python3 Tools/playtest_report_upload.py Saved/Playtests/P1-024/run-01/summary.json
```

Phase 2+:

- Fastify, Hono, or NestJS if the plain Node server becomes limiting
- PostgreSQL or SQLite persistence
- Steam auth helper integration in Phase 2+
