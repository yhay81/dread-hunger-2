# Research Workflow

This repository is allowed to be a research workspace, but Git history must not become a container for third-party commercial game binaries, packaged assets, logs with personal data, or reverse-engineered implementation details.

## Working Rule

Use local source folders as read-only references. Bring only these into tracked Git files:

- path manifests
- local-only file inventories under ignored directories
- original notes
- generalized operational lessons
- self-owned code after license/secret review and game-specific naming removal

Local raw copies are permitted only under ignored `references/private/raw/`.

Do not bring in:

- binaries
- packaged assets
- extracted assets
- save data
- logs with identifiers
- `.env`, DBs, caches, virtual environments
- temporary mod or patch scripts targeting a specific commercial binary

## Research Loop

1. Register a local folder in `references/external-paths.json`.
2. Run `cargo run -p frostwake-tools -- reference-inventory`.
3. Read the generated ignored `references/inventory/*_summary.tsv`.
4. Write original conclusions into docs, for example:
   - server launch assumptions
   - directory layout lessons
   - logging categories to emulate
   - moderation workflows
   - deployment risks
5. If code is self-owned and useful, create a new Frostwake implementation from scratch using the concept, not copied identifiers or proprietary assumptions.

## Why Not Copy Everything

Research status does not remove distribution, license, privacy, or future contamination risk. Once proprietary files enter Git history, it becomes hard to prove the new game was independently implemented. Keeping raw copies ignored and outside tracked source preserves a cleaner boundary while still allowing structured research.

## Current Private Copy

Local-only copies were placed under `references/private/raw/`. Git reports this folder as ignored. Do not stage it.
