# Steam Dedicated Server Client Join Plan

This plan bridges the current Windows dedicated validation scripts and the later Steam server discovery work. It should be completed before public browser or relay work starts.

## Current Entry Point

Use these Windows helpers first:

```powershell
py -3 Tools\quality_gate.py --require-ue
py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server
pwsh -File Tools\windows\run_dedicated_server_validation.ps1
pwsh -File Tools\windows\run_dedicated_client_join_validation.ps1
pwsh -File Tools\windows\run_dedicated_ready_validation.ps1
```

After the individual probes are understood, prefer the Phase 2 entry wrapper for repeatable evidence:

```powershell
pwsh -File Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate -Clients 5 -ExpectedPlayers 5 -ExpectedSaboteurs 1
```

The wrapper writes `Saved\Phase2EntryValidation\<timestamp>\manifest.json` and references the child `Saved\WindowsValidation`, `Saved\DedicatedServerValidation`, `Saved\DedicatedClientJoinValidation`, and `Saved\DedicatedReadyValidation` outputs.

Expected dedicated ready-lobby evidence:

- `session_started`
- `client_connected` for 5 clients
- `player_ready_changed` for 5 players
- `role_assignment_complete` with 5 players and 1 saboteur
- `match_timer_started`
- `match_started` from ready-lobby

## Steam Layer After Local Join

The first Steam join spike should preserve the same Unreal server path:

1. Start the dedicated server with a validated `ServerConfig.json`.
2. Register or advertise a development join target.
3. Create a Steam Lobby containing only rendezvous metadata.
4. Join from a second client through the lobby.
5. Travel into the dedicated server.
6. Confirm the same JSONL events appear as the local client-join path.

## Metadata

Minimum join metadata:

- build id
- map id
- ruleset
- max players
- join state
- official/community flag
- password/private flag
- opaque endpoint token or connection target

Do not store role state, inventory, health, sabotage state, or match authority in lobby metadata.

## Failure Cases

- Build mismatch rejected before travel.
- Full server rejected before travel.
- Password/private mismatch rejected before travel.
- Server unreachable with clear UI and JSONL reason.
- Steam unavailable fallback returns to local/offline validation path in development builds.

## Exit Criteria

- Dedicated server local client join passes on Windows.
- Steam Lobby join reaches the same server-authoritative match flow.
- Generated logs are summarized by `Tools/log_summary.py`.
- The result is recorded in a cycle file and Windows validation note.
