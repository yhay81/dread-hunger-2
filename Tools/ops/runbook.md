# Ops Runbook

## Phase 1

- Do not provision cloud servers.
- Use local listen-server tests only.
- Do not store raw voice, chat, SteamID, external chat account IDs, or IP in committed files.
- Run `Tools/ops/secret_scan.sh` before staging.

## Phase 2 Entry

- Choose one voice provider.
- Define Steam Lobby metadata and validate it with `python3 Tools/ops/lobby_metadata_check.py Tools/ops/lobby_metadata.example.json`.
- Define dedicated server config and admin token behavior.
- Define in-game admin permissions.
- Define server TTL and cleanup behavior.
- Create a sanitized cloud config without account-specific identifiers.
- Add structured JSONL log upload only after redaction is tested.

## Incident Notes

When a playtest incident happens, record:

- MatchId
- BuildId
- approximate time
- anonymized player slot ids
- event summary
- action taken

Do not paste raw logs into GitHub issues.
