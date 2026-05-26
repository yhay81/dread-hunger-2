# Frostwake Ops

Operational tooling for playtests and server management.

This directory is a clean-room implementation area. Do not copy code, commands, secrets, identifiers, logs, or game-specific assumptions from private references. Use only generalized operational lessons.

External chat services are not core dependencies. The target operations model is Steam-native and game-native: Steam Lobby, server browser, dedicated server Tool App, dedicated server config, in-game admin, and optional private web admin.

## Current Files

- `config.schema.json`: sanitized config contract
- `server_config.schema.json`: dedicated server config contract
- `cargo run -p frostwake-tools -- server-config-check`: CLI validation for dedicated server config files
- `lobby_metadata.schema.json`: Steam Lobby rendezvous metadata contract
- `cargo run -p frostwake-tools -- lobby-metadata-check`: CLI validation for lobby metadata and build/map mismatch checks
- `cargo run -p frostwake-tools -- lobby-join-decision`: pre-travel join decision and reject-reason evidence for lobby rows
- `.env.example`: environment variable template without real values
- `cargo run -p frostwake-tools -- steam-registry-list`: Phase 1 empty Steam/server discovery adapter
- `cargo run -p frostwake-tools -- redact-json`: secret and identifier redaction helper
- `cargo run -p frostwake-tools -- banlist-list` / `banlist-add`: local/community/official banlist helpers
- `runbook.md`: operator procedure
- `cargo run -p frostwake-tools -- secret-scan`: local safety check before commits

## Deferred Components

Future server listing aggregation, admin APIs, instance cleanup, launch template generation, structured logging, rule presets, and local persistence should be added as Rust backend endpoints or `frostwake-tools` subcommands. Do not add new executable helpers under `Tools/ops`; this directory should remain schemas, examples, and operator runbooks.

## Phase 1

No live cloud provisioning. Keep this as design and local safety tooling until the game can complete a listen-server match.
