# Technical Architecture

## Core Decision

The authoritative match server is Unreal Dedicated Server, implemented in C++ with Blueprint/DataAsset support. Do not build the real-time match server in TypeScript, Python, Go, or Rust.

Phase 1 uses `OnlineSubsystemNull`, LAN, and packaged listen-server tests to prove gameplay authority first. Steam Lobby, Steam Server Browser, the Dedicated Server Tool App, and Steam Datagram Relay are Phase 2+ implementation layers, not replacements for Unreal server authority.

## Responsibilities

| Area | Technology |
| --- | --- |
| Game client | Unreal Engine 5 |
| Match server | Unreal Dedicated Server, C++, Unreal Replication |
| Server discovery | Steam Lobby, Steam Server Browser, Steam Game Servers API |
| Network transport/protection | Steam Networking Sockets, Steam Datagram Relay |
| Voice | Steam Voice or Unreal Voice Chat Interface with EOS/Vivox |
| Backend | TypeScript, Fastify/NestJS/Hono class stack |
| Admin UI | TypeScript, Next.js or lightweight web UI |
| QA/tools | Python |
| Rust | Not used initially |
| Go | Optional later for coordinator/infra only |

## C++ Scope

Write C++ for:

- MatchGameMode
- MatchGameState
- PlayerState
- RoleComponent
- InventoryComponent
- InteractionComponent
- SabotageComponent
- Health/down/death/containment system
- Door/hatch/lock system
- Objective and ship system state
- server-only authority logic
- JSONL match logs

Use Blueprint for:

- UI
- animation links
- VFX/SFX triggers
- map placement
- item visuals
- simple one-off presentation logic

Use DataAsset/DataTable for:

- item definitions
- task definitions
- sabotage definitions
- role settings
- map settings
- balance values

## Backend Scope

TypeScript backend is for surrounding services only:

- official server start/stop metadata
- server list helper API
- report intake
- banlist management
- player statistics
- match log aggregation
- admin dashboard API
- Steam auth ticket verification helper
- patch notes/news
- post-match survey

It must not own movement, combat, item sync, hatches, sabotage actions, or voice.

## Tools Scope

Python is for:

- AI generation pipeline
- asset naming and validation
- DataTable generation
- balance simulation
- log analysis
- automated test orchestration
- bot input automation
- Steam build helper scripts
- localization file generation

## Explicit Non-Goals

- Unreal client + TypeScript authoritative game server
- Unreal client + Python authoritative game server
- Unreal client + Rust/Go custom replication server
- external chat service driven room creation
- official servers only
- external voice chat as gameplay substitute
- no server browser
- no SteamCMD dedicated server path
