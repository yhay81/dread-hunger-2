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
| `buildId` | Build compatibility gate before travel |
| `mapId` | Map compatibility gate before travel |
| `ruleset` | `standard`, `private_test`, or `developer` |
| `maxPlayers` | Server-advertised max, capped at 8 |
| `currentPlayers` | Lightweight browser display only |
| `joinState` | `open`, `locked`, `in_match`, or `full` |
| `connectionMode` | `listen` or `dedicated` |
| `official` | Official/community display flag |
| `passworded` | Private lobby display flag |
| `endpointToken` | Opaque connection target, not a raw public IP:port |

Optional display fields:

- `region`
- `serverName`

## Validation

Validate the committed example:

```powershell
py -3 Tools\ops\lobby_metadata_check.py Tools\ops\lobby_metadata.example.json --json
```

Validate expected build and map before travel:

```powershell
py -3 Tools\ops\lobby_metadata_check.py `
  Tools\ops\lobby_metadata.example.json `
  --expected-build-id AbyssLock-Win64-Development-local `
  --expected-map-id L_IcebreakerWhitebox
```

The validator fails if:

- schema fields are missing or invalid;
- `currentPlayers` exceeds `maxPlayers`;
- `joinState=open` while already full;
- `joinState=full` while below capacity;
- `endpointToken` looks like a raw `IP:port`;
- expected build or map id does not match.

## P2-003/P2-004 Acceptance

- Create/find/join uses this metadata shape.
- Build mismatch is rejected before travel and logged as a clear local reason.
- Lobby metadata remains non-authoritative.
- Dedicated server client join evidence remains the baseline for the real match path.

