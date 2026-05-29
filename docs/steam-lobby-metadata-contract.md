# Steam Lobby Metadata Contract

This contract supports P2-003 and P2-004 without requiring Steam runtime on Mac. It defines what the first Steam Lobby spike may publish before clients travel to an authoritative Unreal server.

## Boundary

Lobby metadata is rendezvous data only. It must not contain role state, inventory, health, sabotage state, player identity, auth tickets, raw IP addresses, voice data, reports, or match authority.

The authoritative state remains in Unreal Dedicated Server or the listen-server path used for a development spike.

## Required Fields

The schema lives at `Tools/ops/lobby_metadata.schema.json`.

| Field | Purpose |
| --- | --- |
| `schemaVersion` | Metadata contract version, currently `1` |
| `lobbyName` | Player-facing lobby name |
| `lobbyType` | `casual` for enjoy-oriented lobbies or `standard` for experienced lobbies |
| `buildId` | Build compatibility gate before travel |
| `mapId` | Map compatibility gate before travel |
| `ruleset` | Gameplay ruleset: `standard`, `private_test`, or `developer` |
| `maxPlayers` | Fixed match size, always `8` |
| `currentPlayers` | Lightweight browser display only |
| `minimumCompletedMatches` | `0` for casual, at least `50` for standard |
| `joinState` | `open`, `locked`, `in_match`, or `full` |
| `connectionMode` | `listen` or `dedicated` |
| `official` | Official/community display flag |
| `passworded` | Private lobby display flag |
| `endpointToken` | Opaque connection target, not a raw public IP:port |

Optional display fields:

- `region`
- `serverName`
- `mode` — match mode advertised to joiners: `standard` or `madman` (mirrors `EAbyssMatchMode`). Absent ⇒ treated as `standard`. Distinct from `ruleset`: `ruleset` is the lobby category (`standard`/`private_test`/`developer`), `mode` is the in-match game mode.
- `difficulty` — difficulty preset: `easy`, `normal`, or `hard` (mirrors `EAbyssDifficulty`). Absent ⇒ treated as `normal`.

`mode`/`difficulty` are informational rendezvous fields (so a browser/joiner can see the host's chosen mode + difficulty). They are **not** join-gates — only `buildId`/`mapId` mismatch (and the join-state/capacity rules) reject before travel. See `docs/madman-mode-and-host-config-spec.md`.

## Validation

Validate the committed example:

```powershell
cargo run -p frostwake-tools -- lobby-metadata-check Tools\ops\lobby_metadata.example.json --json
```

Validate expected build and map before travel:

```powershell
cargo run -p frostwake-tools -- lobby-metadata-check `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id AbyssLock-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox
```

Generate the client-side pre-travel join decision:

```powershell
cargo run -p frostwake-tools -- lobby-join-decision `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id AbyssLock-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox `
  --require-accepted `
  --json
```

Generate expected mismatch rejection evidence without travelling:

```powershell
cargo run -p frostwake-tools -- lobby-join-decision `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id DifferentBuild `
  --expected-map-id L_IcebreakerWhitebox `
  --expected-reject-reason BuildMismatch `
  --json
```

The validator fails if:

- schema fields are missing or invalid;
- `maxPlayers` is not exactly `8`;
- `currentPlayers` exceeds `maxPlayers`;
- `lobbyType=casual` requires completed matches;
- `lobbyType=standard` has a requirement below 50 completed matches;
- `joinState=open` while already full;
- `joinState=full` while below capacity;
- `endpointToken` looks like a raw `IP:port`;
- expected build or map id does not match.

## P2-003/P2-004 Acceptance

- Create/find/join uses this metadata shape.
- Build mismatch is rejected before travel and logged as a clear local reason.
- Lobby metadata remains non-authoritative.
- `lobby-join-decision` emits `BuildMismatch` or `MapMismatch` before travel for incompatible rows.
- Dedicated server client join evidence remains the baseline for the real match path.

