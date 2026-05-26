# Technical Architecture

This document is the high-level architecture map for Frostwake. It explains how the Unreal gameplay runtime, Rust services, Rust tooling, Steam integration, and project content fit together.

The short version: Unreal owns the match. Rust owns surrounding software. Steam owns distribution, discovery, identity, and transport features where useful. Voice is integrated through a proven voice provider abstraction, not a custom voice transport.

## Architecture Goals

- Prove an 8-player cooperative betrayal survival match before optimizing platform layers.
- Keep all realtime gameplay authority inside Unreal so replication, possession, damage, inventory, objectives, and win/loss stay in one engine runtime.
- Keep backend services non-authoritative so outages or service bugs cannot corrupt live match truth.
- Prefer Rust for new non-Unreal software: backend APIs, validation, summaries, reporting, and repeatable operational tools.
- Keep platform integrations behind narrow boundaries so Phase 1 Null/LAN tests, Phase 2 Steam work, and later dedicated-server deployment share the same gameplay code.
- Produce structured JSONL evidence from local automation and human playtests without committing raw personal data.

## System Map

```text
Steam Client / Steam Services
  lobby invites, server discovery, ownership/auth, SDR where applicable
          |
          v
Unreal Game Client
  input, camera, UI, audio/visual presentation, local voice controls
          |
          | Unreal replication / RPCs
          v
Unreal Listen or Dedicated Server
  GameMode, GameState, PlayerState, Character, interaction, inventory,
  ship systems, sabotage, doors, life state, PvE, win/loss, JSONL telemetry
          |
          | post-match or operational HTTP, not realtime authority
          v
Rust Backend
  playtest report intake, moderation metadata, banlist metadata,
  news, fleet metadata, non-authoritative lobby directory

Rust CLI / Windows Wrappers / UE Editor Commandlets
  validation, smoke orchestration, log summaries, playtest scaffolds,
  asset checks, Steam metadata checks, whitebox generation and validation
```

## Layer Responsibilities

| Layer | Main technology | Responsibility | Must not own |
| --- | --- | --- | --- |
| Game client | Unreal Engine 5, C++, Blueprint | Player input, local presentation, UI, camera, audio/VFX, client requests | Final gameplay truth |
| Match server | Unreal Dedicated Server or listen server, C++ replication | Roles, match phase, ready flow, movement authority, interactions, inventory, damage, containment, ship state, objectives, win/loss, telemetry | External account trust or long-term services |
| Online/discovery | OnlineSubsystemNull in Phase 1; Steam Lobby, Steam Server Browser, Steam Game Servers API in Phase 2+ | Rendezvous, invites, server listing, build/map validation metadata | Role assignment, inventory, health, objectives |
| Voice | Unreal Voice Chat Interface with EOS/Vivox-style provider, Steam Voice only as fallback | Proximity voice, mute/block, reconnect/channel behavior, report hooks | Custom voice transport or committed raw audio |
| Backend | Rust, Axum/Tokio | Reports, moderation metadata, local Steam identity proof rehearsal, banlists, news, optional fleet metadata, non-authoritative lobby directory | Movement, combat, items, sabotage, match start, win/loss |
| Tools | Rust CLI, PowerShell wrappers, UE C++ commandlets | Build/validation orchestration, JSON/schema checks, smoke runs, summaries, asset ledgers, whitebox automation | Runtime gameplay authority |
| Content | Unreal assets, maps, DataAssets/DataTables, Blueprint subclasses | Whitebox layout, art, UI, animation links, data-driven balance | Hidden rule decisions or server bypasses |

## Unreal Runtime

The runtime module is `Source/AbyssLock`. The internal module name is still `AbyssLock`; the public project direction is Frostwake.

### Core Authority Classes

| Class | Role |
| --- | --- |
| `AAbyssLockGameMode` | Server-only owner of login/logout handling, role assignment, ready-gated match start, practice start, match timer, dev smoke hooks, and match-end evaluation. |
| `AAbyssLockGameState` | Replicated public match state: match phase, match ID, build number, match time remaining, winning team, end reason, ship system statuses, route progress, and flooding pressure. |
| `AAbyssLockPlayerState` | Replicated per-player state: life state, revealed team, secret team for the owning player path, and ready state. |
| `AAbyssLockPlayerController` | Owner-facing control boundary for player requests such as ready state and primary interaction. |
| `AAbyssLockCharacter` | Player pawn behavior, input binding, interaction component, inventory component, replicated health, down/rescue/contain/release helpers, and item drop request path. |

### Gameplay Actors And Components

| Area | Current boundary |
| --- | --- |
| Interaction | `UAbyssInteractionComponent` sends player interaction attempts through server-validated actor logic. |
| Inventory | `UAbyssInventoryComponent`, `AAbyssItemPickupActor`, and character drop/pickup paths keep item state server-owned and replicated only as needed. |
| Generic interactables | `AAbyssInteractableActor` is the base surface for server-authoritative interaction targets. |
| Ship tasks | `AAbyssShipTaskActor` applies repair or sabotage against `AAbyssLockGameState` ship systems and can advance route progress or trigger match-end evaluation. |
| Doors and bulkheads | `AAbyssDoorActor` owns open/closed/locked state and validates sabotage lock and repair release interactions. |
| Life actions | `AAbyssLifeActionActor` exposes rescue, containment, and release interactions through normal authority checks. |
| PvE | `AAbyssPveEnemyActor` is a minimal replicated server-authoritative damage source for whitebox pressure tests. |
| Telemetry | `UAbyssTelemetrySubsystem` writes structured event logs used by smoke tests and playtest summaries. |
| Server config | `UAbyssServerConfigSubsystem` is the runtime boundary for dedicated-server configuration. |
| Lobby | `UAbyssLobbySubsystem` is the intended GameInstance subsystem boundary for Steam Lobby work; it must remain a rendezvous layer only. |

### Match State Model

The shared enum model is in `AbyssLockTypes.h`.

| Enum | Values |
| --- | --- |
| `EAbyssMatchPhase` | `WaitingForPlayers`, `RoleAssignment`, `InProgress`, `FinalApproach`, `MatchEnded` |
| `EAbyssTeam` | `Unassigned`, `Crew`, `Saboteur`, `Spectator` |
| `EAbyssLifeState` | `Alive`, `Downed`, `Contained`, `Dead`, `Spectating` |
| `EAbyssShipSystem` | `Hull`, `Fuel`, `Engine`, `Power`, `Radio`, `Route`, `Heat`, `Flooding` |
| `EAbyssShipTaskMode` | `Repair`, `Sabotage` |

### Server Authority Rules

- Clients send requests. The server decides whether they are valid.
- `GameMode` is server-only and is never the source for client-visible replicated state.
- `GameState` contains public match truth visible to all clients.
- `PlayerState` contains player state; secret information must be server-only or owner-only when the implementation is hardened for public play.
- Gameplay truth uses replicated properties or server state. Multicast should stay presentation-oriented.
- Distance, role, life state, match phase, cooldowns, and target validity are checked server-side before gameplay mutation.
- Listen-server smoke tests are useful for velocity but do not provide the same secrecy guarantee as dedicated servers.
- The current Phase 1 admission policy rejects new joins after the match leaves `WaitingForPlayers`. Treat this as an explicit no-mid-match-reconnect policy until Phase 2 dedicated-server identity and rejoin behavior are designed.

## Runtime Flow

### Phase 1 Local Match Flow

```text
Unreal process starts
  -> OnlineSubsystemNull or direct listen-server path
  -> clients connect
  -> AAbyssLockGameMode::PostLogin records connection telemetry
  -> players mark ready through AAbyssLockPlayerController
  -> AAbyssLockGameMode starts once the required ready count is met
  -> roles are assigned server-side
  -> GameState and PlayerState replicate public state
  -> interactions mutate ship, door, item, life, and PvE state on the server
  -> EvaluateMatchEnd resolves crew, saboteur, timer, or fatal-system outcomes
  -> telemetry is summarized by frostwake-tools
```

### Phase 2 Steam Rendezvous Flow

```text
UI requests create/search/join
  -> UAbyssLobbySubsystem calls OnlineSubsystem/Steam behind config gates
  -> lobby metadata is validated before travel
  -> client resolves an opaque endpoint token
  -> client travels to listen or dedicated server
  -> existing Unreal login, ready, role, and match-start flow takes over
```

Steam Lobby metadata must not contain raw SteamIDs, IP addresses, roles, inventory, voice data, or secret match state. It is a pre-match rendezvous and filtering layer only.

### Dedicated Server Flow

```text
Server executable starts
  -> reads ServerConfig.json through the server config boundary
  -> opens the configured map/ruleset
  -> advertises through Steam server discovery when enabled
  -> runs the same AAbyssLockGameMode authority path as local tests
  -> writes JSONL telemetry and optional post-match summaries
  -> may upload or expose non-authoritative metadata to the Rust backend
```

Phase 1 can use listen-server automation. Phase 2 validates the Windows dedicated server target and Steam join path. Production hosting should support both official and community dedicated servers.

## Rust Backend

The backend lives in `apps/backend` and is intentionally non-authoritative. It is a Rust Axum/Tokio service with in-memory storage in the current prototype.

Current API surface:

| Endpoint | Purpose |
| --- | --- |
| `GET /healthz` | Local/deployment health. |
| `POST /v1/playtest-reports` | Accept anonymized playtest summary payloads. |
| `POST /v1/moderation/reports` | Accept structured moderation metadata without raw voice or transcripts. |
| `POST /v1/identity/steam/session-ticket` | Verify the local mock Steam ticket contract and return only a subject hash/proof. |
| `GET /v1/matchmaking/lobbies` | List non-authoritative named lobby-directory records. |
| `POST /v1/matchmaking/lobbies` | Create a named 8-player `casual` or `standard` lobby directory entry. |
| `POST /v1/matchmaking/lobbies/{lobbyId}/join` | Join a lobby-directory entry; `standard` requires 50 completed matches. |
| `POST /v1/matchmaking/lobbies/{lobbyId}/leave` | Leave a lobby-directory entry and delete it when empty. |
| `GET /v1/banlists/{scope}` | Return banlist metadata placeholder data. |
| `GET /v1/news` | Return patch/news placeholder data. |
| `GET /v1/fleet/servers` | Return official fleet metadata placeholder data. |

All backend responses include `x-frostwake-authority: non-authoritative`. That header is part of the architecture contract, not just an implementation detail.

Backend data rules:

- Do not send raw voice, recordings, transcripts, SteamIDs, IP addresses, emails, local user paths, or secrets.
- Use run IDs, build IDs, map IDs, local pseudonymous IDs, player slots, aggregate counts, and structured evidence fields.
- Treat the Steam identity endpoint as a deterministic local mock until real Steam Web API auth and ownership checks replace it.
- Keep Steam auth ticket verification as Phase 2+ before trusting client identity for public services.
- Keep backend availability optional for Phase 1 gameplay validation.

## Rust Tools And Operator Surface

The workspace root `Cargo.toml` defines two Rust packages:

| Package | Path | Purpose |
| --- | --- | --- |
| `frostwake-backend` | `apps/backend` | Non-authoritative HTTP service prototype. |
| `frostwake-tools` | `crates/frostwake-tools` | Repository checks, local automation, smoke orchestration, playtest tooling, schemas, summaries, and migration targets. |

`frostwake-tools` is the durable home for non-Unreal automation. Current responsibilities include:

- server config validation;
- Steam Lobby metadata validation and join decision checks;
- asset ledger checks;
- reference inventory generation;
- JSON redaction;
- local banlist helpers;
- secret scanning;
- JSONL log summaries;
- playtest report upload payloads;
- anonymized playtest summary generation and preflight checks;
- playtest run scaffolding;
- repository quality gates;
- backlog and issue import helpers;
- Unreal project gate checks;
- local smoke and smoke-suite orchestration;
- smoke-suite Markdown evidence export;
- visual POC manifest scaffolding;
- launching UE C++ commandlets for whitebox generation and validation.

PowerShell under `Tools/windows` is the Windows operator surface. It should call Rust tools and Unreal commandlets instead of reimplementing business logic. `Tools/ops` should remain schemas, examples, and runbooks only.

## Unreal Editor Automation

Editor-only automation lives in the `AbyssLockEditor` module, not in runtime gameplay code.

| Boundary | Responsibility |
| --- | --- |
| `AbyssLockEditor` module | Editor-only commandlets and validation utilities. |
| `CreateIcebreakerWhitebox` commandlet | Generates or refreshes the automation whitebox map. |
| `ValidateIcebreakerWhitebox` commandlet | Validates whitebox layout, task placement, doors, and expected map structure. |
| `frostwake-tools` command wrappers | Launch the commandlets from a repeatable Rust CLI surface. |

The automation whitebox map is `/Game/Maps/L_IcebreakerWhitebox`. Visual POC work must stay separate from this map so art experiments do not break automation evidence.

## Content And Data

Blueprint and content should extend C++ authority surfaces instead of bypassing them.

Use Blueprint for:

- UI screens and widgets;
- animation links;
- VFX/SFX triggers;
- map placement;
- item visuals;
- simple one-off presentation behavior.

Use DataAssets or DataTables for:

- item definitions;
- task definitions;
- sabotage definitions;
- role settings;
- map settings;
- balance values.

Avoid putting match authority, hidden role decisions, inventory mutation, sabotage results, or win/loss logic in Blueprint-only paths.

## Networking And Replication

Phase 1 uses conventional Unreal replication. Iris remains out of scope until profiling or scale evidence shows a real need.

Replication expectations:

- Match phase, winner, timer, route progress, and ship system state replicate from `AAbyssLockGameState`.
- Ready state, life state, and role-facing player state replicate from `AAbyssLockPlayerState`.
- Character health replicates from `AAbyssLockCharacter`.
- Interactions, item drops, repairs, sabotage, doors, life actions, and PvE damage are server-validated before state changes.
- Inventory should replicate only the minimum needed for the owner and presentation.
- Event logs are server-side evidence, not a gameplay synchronization channel.

The project should add profiling counters before changing the replication system. With an 8-player fixed match target, explicit compact replicated state is the safer first architecture.

## Steam And Platform Integration

Steam is a platform layer around the Unreal authority path.

| Feature | Phase | Architecture boundary |
| --- | --- | --- |
| OnlineSubsystemNull / LAN | Phase 1 | Local proof of gameplay authority. |
| Steam Lobby | Phase 2 | Rendezvous, invites, metadata filtering, opaque endpoint resolution. |
| Steam Server Browser / Game Servers API | Phase 2+ | Dedicated-server discovery for official and community servers. |
| Dedicated Server Tool App | Phase 2+ | SteamCMD-compatible community-hosted server distribution. |
| Steam auth / ownership | Phase 2+ | Identity trust for public backend and moderation workflows. |
| Steam Datagram Relay | After dedicated-server stability | Transport protection without gameplay code depending on raw IPs. |

Steam must not become the match rules engine. Once travel succeeds, Unreal server code owns readiness, role assignment, interactions, objectives, and outcomes.

## Voice Architecture

Voice is a required gameplay feature, but custom voice transport is not part of this architecture.

Preferred direction:

- use Unreal's Voice Chat Interface as the abstraction layer;
- validate EOS Voice, Vivox-style providers, or another proven provider against proximity requirements;
- keep Steam Voice as a Steam-only fallback or prototype option;
- integrate mute, block, report, downed/contained/dead channel behavior, and reconnect behavior;
- never commit raw voice, transcripts, or recordings as test artifacts.

External voice calls may help early fun testing, but they do not satisfy shipped voice acceptance.

## Telemetry, QA, And Evidence

The architecture depends on structured local evidence because the first goal is proving the match loop.

Evidence path:

```text
Unreal server emits JSONL events
  -> frostwake-tools log-summary
  -> smoke-suite summary or playtest summary
  -> anonymized Markdown evidence in docs/playtests when appropriate
  -> optional non-authoritative backend report upload
```

Important telemetry categories:

- client connect/disconnect;
- ready state changes;
- role assignment;
- match start/end and match duration;
- repair and sabotage applications;
- route progress and fatal ship states;
- door/bulkhead lock and repair outcomes;
- flooding pressure and pump repair;
- down/rescue/contain/release;
- PvE damage;
- item drop, pickup, and use;
- lobby metadata validation and travel failures in Phase 2.

Human playtest summaries must be anonymized before they are committed. Raw logs, recordings, local user paths, SteamIDs, IP addresses, and voice transcripts must stay out of committed artifacts.

## Repository Layout

```text
Config/              Unreal project settings
Content/             Unreal maps and assets
Source/              Unreal C++ runtime, editor module, and target definitions
apps/backend/        Rust non-authoritative backend prototype
apps/admin/          Future Rust-first admin UI shell
crates/              Rust CLI and future reusable non-Unreal crates
Tools/windows/       Windows operator wrappers
Tools/ops/           JSON schemas, examples, and runbooks
docs/                Current design, production, QA, legal, and platform plans
docs/cycles/         Historical cycle notes, not active command guidance
references/          Local reference manifests and ignored private inventories
```

Generated Unreal outputs, local smoke logs, playtest raw evidence, and private reference inventories should not become architecture dependencies.

## Current Maturity

Implemented or scaffolded:

- Unreal C++ runtime module with ready lobby, role assignment, match state, life state, inventory, tasks, sabotage, route progress, doors, flooding pressure, PvE, and JSONL telemetry smoke coverage.
- Whitebox map at `/Game/Maps/L_IcebreakerWhitebox`.
- Rust backend prototype with health, report intake, moderation metadata, local Steam identity proof rehearsal, lobby directory, banlist/news/fleet placeholders, and redaction guardrails.
- Rust CLI migration for active automation and validation behavior.
- Windows wrapper scripts for operator entry points.
- Editor commandlet boundary for whitebox generation and validation.
- Steam Lobby metadata contract and subsystem design, with runtime Steam integration still gated.

Open architecture risks:

- Dedicated server target must be validated on Windows after the Mac Launcher UE server-target limitation.
- Secret role replication and listen-host information exposure need hardening before public play.
- `frostwake-tools` is large and should be split into modules after the current migration stabilizes.
- Backend persistence, real Steam auth, and access control are placeholders until Phase 2+.
- Voice provider selection and acceptance testing remain future work.
- Steam Lobby, server browser, Game Servers API registration, and SDR are planned platform layers, not proven runtime paths yet.

## Explicit Non-Goals

- Replacing Unreal networking with a custom authoritative Rust server.
- Moving realtime movement, combat, inventory, hatches, sabotage, or win/loss into the Rust backend.
- Using an external chat service as room creation, matchmaking, or gameplay voice authority.
- Building custom voice transport.
- Shipping official-server-only architecture.
- Depending on raw IP addresses in gameplay code.
- Enabling Iris or large-scale replication systems before profiling evidence.
- Treating `docs/cycles` as active implementation guidance.
