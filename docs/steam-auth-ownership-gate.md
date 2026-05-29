# Steam Auth And Ownership Gate

Current as of 2026-05-26 JST. Checked against official Steamworks documentation for User Authentication and Ownership, ISteamUserAuth/AuthenticateUserTicket, and ISteamUser/CheckAppOwnership.

## Sources

- Steamworks User Authentication and Ownership: https://partner.steamgames.com/doc/features/auth
- Steamworks `ISteamUserAuth/AuthenticateUserTicket`: https://partner.steamgames.com/doc/webapi/ISteamUserAuth#AuthenticateUserTicket
- Steamworks `ISteamUser/CheckAppOwnership`: https://partner.steamgames.com/doc/webapi/ISteamUser#CheckAppOwnership

## Goal

P2-006 verifies Steam identity and app ownership before the backend trusts moderation identities, protected fleet/server metadata, or public support workflows. This gate does not move gameplay authority out of Unreal Dedicated Server.

## Boundary

- Unreal owns real-time match state, roles, inventory, damage, sabotage, voice state, and win/loss.
- The backend is the secure server for Steam Web API calls and must keep publisher keys outside the client, Unreal config, committed files, and logs.
- Steam Lobby metadata must never contain raw SteamIDs, auth tickets, player names, IP addresses, moderation reports, or ticket-derived secrets.
- Phase 1 and local Phase 2 rehearsals may keep using pseudonymous local IDs. Public trust decisions require this gate.

## Planned Flow

1. Unreal client asks Steam for a Web API auth ticket with a Frostwake-specific identity string.
2. Client waits for Steam's ticket callback, then sends the hex ticket to the backend over HTTPS.
3. Backend calls `ISteamUserAuth/AuthenticateUserTicket` with the configured AppID, publisher key, ticket, and matching identity string.
4. Backend verifies app ownership with `ISteamUser/CheckAppOwnership` before issuing any trusted identity result.
5. Backend returns only an opaque identity proof, a non-reversible subject hash, ownership status, and expiry. It must not echo raw SteamID or ticket material.
6. Moderation/report/fleet APIs may reference the identity proof or subject hash, while committed evidence records only hashes and pass/fail reasons.

## Backend Contract

Implemented local mock endpoint contract:

```text
POST /v1/identity/steam/session-ticket
```

The current Rust backend implementation is a deterministic local mock. It does not call Steam Web API, does not require a publisher key, and exists to lock the API shape and privacy guarantees before real credentials are introduced.

Request fields:

| Field | Rule |
| --- | --- |
| `ticketHex` | Required, hex string, never logged verbatim |
| `purpose` | Enum such as `moderation`, `server_metadata`, or `support` |
| `localRunId` | Optional local diagnostic ID, scanned for risky identity patterns |

Response fields:

| Field | Rule |
| --- | --- |
| `verified` | True only after Steam ticket authentication succeeds |
| `ownsApp` | True only after ownership check succeeds for the configured AppID |
| `subjectHash` | Server-side HMAC or equivalent non-reversible hash of SteamID and environment salt |
| `proofId` | Short-lived backend proof ID for later report/fleet calls |
| `expiresAt` | Short TTL; clients refresh instead of reusing old proof |

Failure responses should distinguish invalid ticket, ownership failure, missing backend Steam config, and upstream Steam error without exposing secrets or raw identifiers.

Current mock sentinel outcomes:

| `ticketHex` | Result |
| --- | --- |
| `feedfacecafebeef` | Success with `verified=true`, `ownsApp=true`, subject hash, proof ID, and 15-minute expiry |
| `deadbeef` | `401 invalid_steam_ticket` |
| `0badc0de` | `403 app_ownership_required` |
| Any other valid hex | `401 invalid_steam_ticket` |

## Unreal Client Helper

`UFrostwakeSteamIdentitySubsystem` is the Unreal-side contract stub for this endpoint. It can build the request JSON, submit it to a configured backend base URL, parse the response, and return `FFrostwakeSteamIdentityVerificationResult` to Blueprint or C++ callers.

Current helper boundaries:

- It does not request real Steam tickets yet; callers must provide the ticket hex from a future Steam SDK integration or the local mock sentinel values.
- It posts only to `/v1/identity/steam/session-ticket`.
- It accepts only `moderation`, `server_metadata`, or `support` as purpose values.
- It validates that `ticketHex` is even-length hex before sending.
- It logs `ticketHex=[REDACTED]`, ticket byte count, purpose, endpoint path, and pass/fail result metadata only.
- It never logs or emits raw SteamIDs, auth tickets, player names, IP addresses, publisher keys, or backend secrets.
- Success telemetry records only booleans plus whether `proofId` and `subjectHash` were present.

## Evidence Requirements

- `cargo test -p frostwake-backend` covers success, invalid ticket, ownership failure, missing config, and no raw SteamID/ticket echo in responses.
- `cargo run -p frostwake-tools -- secret-scan` or the quality gate confirms no publisher key, ticket sample, raw SteamID, or local config leak is committed.
- Windows runtime evidence records only subject hashes, endpoint names, configured AppID, and pass/fail reasons.
- OpenAPI and `docs/backend-contract.md` stay in parity with the implemented endpoint before public moderation or protected fleet metadata uses it.

## Open Questions

| Question | Decision Needed Before Runtime |
| --- | --- |
| Hashing | Choose HMAC key storage and rotation plan for subject hashes |
| Persistence | Decide whether identity proofs are in-memory, SQLite, or hosted database records |
| Scope | Decide if private playtests require ownership verification or only public Steam Playtest waves |
| Failure UX | Define client-facing copy for ownership failure, Steam outage, and retryable ticket failure |
