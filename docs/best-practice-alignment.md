# Best Practice Alignment

Last checked: 2026-05-25 JST.

This document records whether the project direction matches current official Unreal, Steamworks, Node, Python, Apple, and Microsoft guidance. Treat it as a planning gate, not marketing copy.

## Sources Checked

- Unreal Engine 5.7 release notes: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-7-release-notes
- Unreal dedicated servers: https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine
- Unreal networking and Iris: https://dev.epicgames.com/documentation/unreal-engine/networking-and-multiplayer-in-unreal-engine
- Unreal Voice Chat Interface: https://dev.epicgames.com/documentation/unreal-engine/voice-chat-interface-in-unreal-engine
- EOS Voice Chat plugin: https://dev.epicgames.com/documentation/en-us/unreal-engine/voice-chat-with-epic-online-services
- Steam Playtest: https://partner.steamgames.com/doc/features/playtest
- Steam dedicated server distribution: https://partner.steamgames.com/doc/sdk/uploading/distributing_gs
- Steam Networking / SDR: https://partner.steamgames.com/doc/features/multiplayer/networking
- Python downloads: https://www.python.org/downloads/latest/
- Node releases: https://nodejs.org/en/download/releases/
- Node current download: https://nodejs.org/en/download/current
- Apple Xcode support: https://developer.apple.com/support/xcode/
- Visual Studio 2022 release notes: https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-notes
- Visual Studio 2022 lifecycle: https://learn.microsoft.com/en-us/lifecycle/products/visual-studio-2022

## Alignment Verdict

The core approach is aligned:

- Use Unreal Dedicated Server and Unreal replication for the authoritative match server.
- Use C++ for server-authoritative gameplay and Blueprint/DataAsset for presentation and tuning.
- Keep TypeScript and Python outside the real-time match server.
- Validate Phase 1 with `OnlineSubsystemNull`, LAN/listen-server, and local automation before Steam integration.
- Use Steam-native lobby, server discovery, Playtest, Dedicated Server Tool App, SteamCMD, and SDR later.
- Avoid external chat-service dependency for core matchmaking, hosting, moderation, or proximity voice.

## Required Improvements From This Review

| Area | Current Decision | Improvement |
| --- | --- | --- |
| Dedicated server validation | Boot, client-join, and ready-lobby probes exist | Run all three on the Windows workstation and record summarized evidence before Steam server discovery work |
| Steam Tool App | Tool App is planned | Phase 3 gate must include Dedicated Server Redistributables, `steam_appid.txt`, depot upload, release checklist, and anonymous SteamCMD download verification |
| SDR | Phase 3/4 implementation | Add Phase 2 design spike to choose Hosted Dedicated Server, FakeIP, or known data center flow before public server browser work |
| Voice | Steam Voice or Unreal Voice Chat Interface | Prefer Unreal Voice Chat Interface with EOS Voice/Vivox-style provider; keep Steam Voice as Steam-only fallback |
| Iris | Not enabled | Keep generic replication for Phase 1; evaluate Iris only as an opt-in Phase 2 performance spike because Epic still labels it experimental/caution-for-shipping |
| Steam auth | Backend mentions helper | Add explicit gate for auth ticket / ownership verification before public server listing or moderation trust decisions |
| Node runtime | Local Node is Current-line v25 | Use Node 24 LTS for backend production and allow current-line only for local experimentation |

## Software Currency

| Software | Local / Repo State | Official Current Finding | Decision |
| --- | --- | --- | --- |
| Unreal Engine | `UE_5.7` installed locally | Official docs and release notes are on UE 5.7 | Current major is OK; Windows source/Launcher build should use latest available 5.7.x patch, not jump engine branches mid-Phase 1 |
| Xcode | `Xcode 26.5` | Apple lists Xcode 26.5 as latest | Current |
| Python | local `3.14.3`; docs require `3.11+` | Python latest is `3.14.5` | Accept `3.11+`; recommend updating local/Windows to latest 3.14.x when convenient |
| Node.js | local `25.8.2`; backend now declares `>=24 <27` | Node current is 26.x; Node 24 is Active LTS | Use Node 24 LTS for production/backend CI; current-line Node 25/26 is acceptable only for local smoke unless it exposes incompatibility |
| npm | local `11.11.1` | Node 26 current bundles newer npm 11.x | OK for local; CI should follow chosen Node LTS bundle |
| TypeScript | upgraded to `^6.0.3` | npm latest is `6.0.3` | Current |
| `@types/node` | upgraded to Node 24 line `^24.12.4` | Latest package line includes Node 25 types | Use Node 24 type line to match LTS runtime target |
| Visual Studio | Windows requirement says VS 2022 | Microsoft current channel is VS 2022 17.14; UE 5.7 notes MSVC 14.44 support and say MSVC 14.50 from VS 2026 is not currently supported | Require VS 2022 17.14 Current channel / MSVC 14.44-compatible toolchain; do not use VS 2026 toolchain until Epic supports it |

## Phase Gates To Preserve

1. Phase 1 stays on `OnlineSubsystemNull`, generic replication, local listen-server and Windows dedicated-server boot validation.
2. Phase 2 starts only after Windows `AbyssLockServer` boot and client join evidence exists.
3. Steam Lobby can be added before Steam Server Browser, but lobby metadata must remain lightweight and non-authoritative.
4. Server Browser and Steam Game Servers API work must wait for build id compatibility checks, ownership/auth checks, and moderation basics.
5. SDR work must be designed before any public IP/port-based hosting plan becomes product policy.
6. Voice implementation must prove proximity attenuation, mute/block, death/spectator channel behavior, and reconnect behavior before Playtest.
