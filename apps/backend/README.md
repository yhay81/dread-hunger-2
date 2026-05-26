# Frostwake Backend

Rust backend prototype.

This service is for reports, bans, stats, admin APIs, patch notes, surveys, and optional fleet metadata. It is not the authoritative match server.

Initial API contract: [openapi.yaml](openapi.yaml)

Design notes: [Backend Contract](../../docs/backend-contract.md)

Current local implementation:

- Rust with Axum/Tokio
- In-memory storage only
- JSON validation for anonymized playtest reports and moderation metadata
- Structured moderation evidence fields for voice/trust reports without raw audio or transcript intake
- Mocked Steam identity proof endpoint for local Phase 2 contract tests without real Steam credentials
- In-memory matchmaking lobby directory for named 8-player `casual` and `standard` lobbies
- Redaction guardrails for IP addresses, SteamID-like values, emails, local user paths, and secret-like assignments
- Non-authoritative response header: `x-frostwake-authority: non-authoritative`

Local commands:

```bash
cargo test -p frostwake-backend
cargo run -p frostwake-backend
```

Default local URL:

```text
http://localhost:8787
```

Playtest report payload generation and local upload are handled by `cargo run -p frostwake-tools -- playtest-report-upload`; the backend accepts the same `/v1/playtest-reports` payload shape documented in `openapi.yaml`.

Phase 2+:

- PostgreSQL or SQLite persistence
- Real Steam Web API auth/ownership integration after the local mock contract is promoted
