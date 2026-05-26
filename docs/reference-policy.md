# Reference Policy

## Decision

Do not commit Dread Hunger server binaries, packaged content, assets, logs with player identifiers, or reverse-engineered materials into Git history.

This research repository may contain local-only ignored copies under `references/private/raw/` for private study. Those copies must never be staged, committed, pushed, used as production dependencies, or migrated into the future product repository.

The project can learn from broad operational patterns: server cost, DDoS risk, community hosting, moderation needs, and playtest logistics. The product implementation must not import or depend on protected expression, proprietary files, or game-specific implementation details from another title.

## Allowed

- Keep a local path manifest for private reference.
- Generate local-only inventories containing file paths, sizes, timestamps, and hashes.
- Keep ignored local-only copies under `references/private/raw/`.
- Review owned management scripts or public repositories only after checking their license.
- Extract high-level lessons into original documentation.
- Record decisions in this repository without copying source material.

## Not Allowed

- Staging, committing, or pushing commercial server binaries.
- Committing `.pak`, `.ucas`, `.utoc`, executable, DLL, PDB, maps, assets, audio, UI, textures, or other packaged game files.
- Copying game-specific names, UI layouts, item names, map layouts, config values, packet formats, or proprietary server behavior.
- Importing logs containing SteamID, IP address, voice/chat text, or other personal data.
- Adding symlinks that point to proprietary local folders.
- Bulk-copying existing management repositories that contain `.env`, databases, caches, virtual environments, temporary scripts, or game-specific automation.

## Local Reference Workflow

1. Put source locations in [references/external-paths.json](../references/external-paths.json).
2. Run `cargo run -p frostwake-tools -- reference-inventory` to create local metadata under `references/inventory/`.
3. Keep `references/inventory/` uncommitted.
4. If a local copy is needed, copy into `references/private/raw/` only.
5. Convert only high-level, non-expressive lessons into docs.
6. If a folder contains user-owned source code, check the license before copying or vendoring anything into tracked source folders.

## Current Local Assessment

| Path | Decision |
| --- | --- |
| `/Users/yhay81/Development/DreadHungerWorkspace/.../WindowsServer*` | Local-only ignored copy allowed. Treat as commercial server/package reference only. |
| `/Users/yhay81/Development/DreadHungerWorkspace/.../Binaries/Win64` | Local-only ignored copy allowed. Binary/server artifact risk; never commit. |
| `/Users/yhay81/ghq/github.com/yhay81/dread-hunger-server` | Local-only ignored copy allowed. Contains local environment, database, cache, and game-specific automation. Review for abstract bot/ops patterns only. |
| `/Users/yhay81/ghq/github.com/yhay81/dread-hunger-lounge-api` | Local-only ignored copy allowed. May be user-owned, but game-specific naming and domain assumptions must be removed before reuse. |
| `/Users/yhay81/ghq/github.com/Dread-Hunger-Community/dread-hunger.net` | Local-only ignored copy allowed. Do not reuse content without license review. |

## Public Release Rule

Before pushing or opening a Steam page, verify that this repository contains no third-party game files and that the repository name has been changed away from `dread-hunger-2`.
