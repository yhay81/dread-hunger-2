# Steam Phase 2 Integration Plan

Phase 2 adds Steam-facing systems after Windows dedicated-server evidence exists. The goal is still a playable vertical slice, not a public operations stack.

## Non-Negotiables

- Unreal Dedicated Server remains the authoritative match server.
- Steam systems are discovery, identity, transport, and distribution layers; they do not own match rules.
- The game must be playable through in-game lobby and browser flows.
- External chat-service rooms are not a production gameplay path.
- Official servers are useful for tests, but community hosting must remain possible.
- Direct public IP and port details must stay out of gameplay logic.

## Sequence

1. Validate Windows dedicated server.
   Run the boot, client-join, and ready-lobby scripts from `Tools/windows/`, or use `Tools/windows/run_phase2_entry_validation.ps1` once the server target exists. Record server JSONL evidence and the filled Windows validation template.

2. Enable Steam only behind dev gates.
   Add a development-only Steam config path and validate it with `Tools/windows/check_steam_dev_config.ps1`. Phase 1 local gates must keep working with Null/LAN assumptions.

3. Add Steam Lobby rendezvous.
   Implement `docs/steam-lobby-subsystem-design.md`: create, find, join, and build mismatch rejection. Store only metadata allowed by `Tools/ops/lobby_metadata.schema.json` and needed to get clients to the authoritative server.

4. Add Steam identity.
   Promote the local mock endpoint in `docs/steam-auth-ownership-gate.md` to real Steam Web API verification before trusting reports, moderation identities, or protected server metadata.

5. Decide discovery architecture.
   Choose the staged relationship between Steam Server Browser, Steam Game Servers API, lobby metadata, and optional backend fleet metadata.

6. Prepare dedicated server distribution.
   Build the Tool App checklist, example config, launch scripts, log conventions, and SteamCMD verification flow before broad testing.

7. Decide SDR model.
   Choose hosted dedicated server, FakeIP, known data center, or a staged fallback before public server browser work.

8. Add proximity voice through a provider abstraction.
   Use Unreal Voice Chat Interface where possible and keep provider-specific code isolated.

## Definition Of Done

- A clean Windows clone can pass all Phase 1 gates and Phase 2 entry gates.
- One dedicated server accepts 8 clients, assigns roles, and starts from ready-lobby.
- A Steam Lobby spike can bring a second client to the same server path.
- Mismatched build metadata is rejected before travel.
- The server browser design and Tool App packaging checklist are written before public listing work.
- The voice provider decision includes moderation and mute/block implications.

## Deferred

- Public Playtest launch.
- Paid Early Access release.
- Full official fleet autoscaling.
- Runtime-generated AI content.
- Custom voice transport.
