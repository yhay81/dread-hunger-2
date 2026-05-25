# Frostwake Backend

TypeScript backend placeholder.

This service is for reports, bans, stats, admin APIs, patch notes, surveys, and optional fleet metadata. It is not the authoritative match server.

Initial API contract: [openapi.yaml](openapi.yaml)

Design notes: [Backend Contract](../../docs/backend-contract.md)

Recommended initial stack:

- TypeScript
- Fastify, NestJS, or Hono
- PostgreSQL or SQLite for local prototype
- Steam auth helper integration in Phase 2+
