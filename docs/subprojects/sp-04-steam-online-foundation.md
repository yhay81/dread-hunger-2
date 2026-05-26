# GP-04: Steam Online Access

## North-Star Goal

Players can find, join, reject incompatible sessions, and host through Steam-safe paths while Unreal Dedicated Server remains the authority for match state.

## Why This Exists

Steam integration is not a feature pile. It is the access layer that determines whether players can reliably get into the same authoritative match and whether public systems can be operated safely.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Config safety | Default repo stays Null/LAN safe, Steam dev config stays ignored/local |
| Lobby rendezvous | Create/find/join brings clients to the same authoritative path |
| Compatibility rejection | Build, map, ruleset, and capacity mismatches fail before travel with clear logs |
| Identity trust | Reports, moderation, and protected metadata use verified auth boundaries |
| Discovery clarity | Server browser, lobby metadata, backend fleet metadata, and Steam APIs have explicit roles |
| Hosting path | Dedicated Server Tool App and SteamCMD path are documented and verified |
| SDR readiness | Relay model is chosen before public server assumptions harden |

## Improvement Questions

- What is the smallest Steam layer that improves player access without weakening authority?
- Are local validation paths still working without Steam?
- Can a client know why a session cannot be joined?
- Which data is safe in lobby metadata, and which must stay server/backend-side?
- What would break if a community host ran the server outside the developer machine?

## Evidence Standard

Steam-facing evidence should include config state, validation command, account/environment assumptions, lobby metadata payload, join result, rejection reason, server log path, and whether Null/LAN validation still passes.

Primary docs:

- `docs/phase2-backlog.md`
- `docs/steam-phase2-integration-plan.md`
- `docs/steam-dev-config-gate.md`
- `docs/steam-lobby-metadata-contract.md`
- `docs/steam-lobby-subsystem-design.md`
- `docs/server-hosting-model.md`

## Boundaries

- Do not trust Steam Lobby metadata for gameplay authority.
- Do not commit private AppIDs, secrets, SteamIDs, IPs, or local dev config.
- Do not start public server browser work before GP-02 proves the server path.
- Do not remove Null/LAN development support.

## Review Cadence

Review this portfolio weekly during Phase 2 and before any public-facing Steam promise. Each review should ask whether the join path is more reliable, safer, or clearer than the previous version.

## Agent Charter

An agent assigned to this portfolio should improve one access-layer signal. A good assignment is: "Improve build-mismatch rejection evidence for Steam Lobby joins without changing authoritative gameplay state."
