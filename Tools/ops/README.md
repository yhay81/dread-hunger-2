# Frostwake Ops

Operational tooling for playtests and server management.

This directory is a clean-room implementation area. Do not copy code, commands, secrets, identifiers, logs, or game-specific assumptions from private references. Use only generalized operational lessons.

External chat services are not core dependencies. The target operations model is Steam-native and game-native: Steam Lobby, server browser, dedicated server Tool App, dedicated server config, in-game admin, and optional private web admin.

## Planned Components

- `config.schema.json`: sanitized config contract
- `server_config.schema.json`: dedicated server config contract
- `server_config_check.py`: CLI validation for dedicated server config files
- `lobby_metadata.schema.json`: Steam Lobby rendezvous metadata contract
- `lobby_metadata_check.py`: CLI validation for lobby metadata and build/map mismatch checks
- `.env.example`: environment variable template without real values
- `steam_registry.py`: Steam/server discovery adapter
- `server_browser.py`: server listing aggregation
- `admin_api.py`: private admin API contract
- `aws_instances.py`: cloud instance adapter
- `server_bootstrap.py`: server launch template generation
- `cleanup_instances.py`: stale instance cleanup
- `logging.py`: structured logging setup
- `redaction.py`: secret and identifier redaction helpers
- `banlist.py`: local/community/official banlist helpers
- `rulesets.py`: server rule presets
- `schema.sql`: local state database schema
- `runbook.md`: operator procedure
- `secret_scan.sh`: local safety check before commits

## Phase 1

No live cloud provisioning. Keep this as design and local safety tooling until the game can complete a listen-server match.
