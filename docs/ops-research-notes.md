# Ops Research Notes

These notes summarize high-level operations patterns observed from local private references. They intentionally avoid copying code, exact commands, provider identifiers, server binary assumptions, or game-specific protocol details.

## Do Not Reuse Directly

- Commercial server binaries, packaged assets, signatures, archives, and patch scripts.
- Existing temp scripts that target a specific commercial game binary.
- AWS account-specific AMIs, keys, security groups, subnet IDs, profile names, and region choices.
- External chat command names, text, economy/ticket rules, or game-specific moderation assumptions.
- Databases, logs, saved configs, player identifiers, external chat identifiers, message contents, or icon URLs.

## Generalized Patterns Worth Rebuilding

- Short-lived community test servers with TTL and cleanup.
- Region-aware cloud instance configuration.
- Server bootstrap templates generated from sanitized config.
- Structured logs with request context, actor id, server id, and match id.
- Quota/ticket tracking for expensive test infrastructure.
- Steam-native lobby, server browser, and dedicated server discovery.
- In-game admin and moderation controls instead of external chat commands.
- Automated stale instance cleanup.
- Deployment runbooks with explicit secret handling.
- Pre-commit, lint, secret scanning, and dependency update scripts.

## Frostwake Ops Scope

Phase 1 does not need live cloud provisioning. Build only local abstractions and documentation. Do not make an external chat service a core dependency.

Phase 2 should add:

- Steam Lobby integration
- in-game server browser prototype
- dedicated server config format
- local server admin commands
- cloud server prototype
- log upload
- moderation report handling

Phase 3 should harden:

- Steam Playtest support
- dedicated server distribution
- community host documentation
- public-facing moderation policy

## Target Operations Model

Frostwake should use a game-native operations model:

- Steam Lobby for friend invites and quick join.
- Steam Game Servers style discovery for public/community servers.
- Dedicated server config files for map, region, max players, rules, admin token, and banlist.
- In-game admin UI or console commands for kick, mute, report review, and server shutdown.
- Optional lightweight web admin for private fleet monitoring, not player-facing chat ops.
- Structured logs written by the game server, then uploaded or exported by server operators.
