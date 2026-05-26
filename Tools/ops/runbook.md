# Ops Runbook

## Phase 1

- Do not provision cloud servers.
- Use local listen-server tests only.
- Do not store raw voice, chat, SteamID, external chat account IDs, or IP in committed files.
- Run `cargo run -p frostwake-tools -- secret-scan` before staging.

## Phase 2 Entry

- Choose one voice provider.
- Define Steam Lobby metadata and validate it with `cargo run -p frostwake-tools -- lobby-metadata-check Tools/ops/lobby_metadata.example.json`.
- Define dedicated server config and admin token behavior.
- Define in-game admin permissions.
- Define server TTL and cleanup behavior.
- Create a sanitized cloud config without account-specific identifiers.
- Add structured JSONL log upload only after redaction is tested.

## Playtest Wave Readiness

- Before each private-playtest wave, complete `docs/steam-playtest-checklist.md` and the "Playtest Wave Gate" section with current owner and artifacts.
- Capture the minimal wave evidence log fields from the checklist in the wave plan note:
  - Wave owner
  - Target wave number
  - Build id
  - Start criteria check results
  - Start/hold/cancel decision
  - Rollback trigger (if any)
  - Rollback decision owner
- Require moderation incident triage rules from `docs/playtest-data-and-moderation.md` before each wave.
- If cancellation or hold is selected, the wave lead updates the next wave decision note with:
  - Missing evidence
  - Corrective action
  - Owner
  - Estimated unblock date
- Do not open the next wave until the above is complete and reviewed by Engineering + Production.

## Incident Notes

When a playtest incident happens, record:

- MatchId
- BuildId
- approximate time
- anonymized player slot ids
- event summary
- action taken

Do not paste raw logs into GitHub issues.
