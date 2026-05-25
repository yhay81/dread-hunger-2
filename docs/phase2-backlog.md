# Phase 2 Backlog

This backlog turns the validated greybox prototype into a Steam-facing vertical slice foundation. It assumes Phase 1 local gameplay authority remains in Unreal C++ and that Windows is the production development platform.

Do not start public server browser work until the Windows dedicated server can boot, accept clients, assign roles, and start a ready-lobby match with evidence.

## Entry Gates

| ID | Task | Done When |
| --- | --- | --- |
| P2-001 | Confirm Phase 2 entry gates | Windows clone records `Tools/windows/run_phase2_entry_validation.ps1` output, `quality_gate.py --require-ue`, `unreal_gate.py --platform Win64 --include-server`, dedicated-server boot, client-join, ready-lobby, and key smoke profile evidence in a cycle file |
| P2-005 | Dedicated server client-join validation | `Tools/windows/run_dedicated_client_join_validation.ps1`, `Tools/windows/run_dedicated_ready_validation.ps1`, or the phase entry wrapper pass on Windows with server JSONL evidence, 5 client connections, role assignment, and ready-lobby match start |

## Steam Lobby

| ID | Task | Done When |
| --- | --- | --- |
| P2-002 | Enable Steam dev config safely | `Tools/windows/check_steam_dev_config.ps1` verifies committed defaults stay Null/LAN safe; optional ignored `Saved/Config/steam_dev.local.ini` passes validation before Steam Lobby work |
| P2-003 | Steam Lobby create/find/join spike | `Tools/windows/run_steam_lobby_validation.ps1` preflight passes; `docs/steam-lobby-subsystem-design.md` is implemented in a Windows-only spike that can create a lobby, list/find it, join from a second client, validate metadata with `Tools/ops/lobby_metadata_check.py`, and travel into the same Unreal authoritative server path without trusting lobby state for gameplay |
| P2-004 | Lobby build/version rejection | Lobby metadata follows `Tools/ops/lobby_metadata.schema.json`; mismatched build or map id is rejected before travel with a logged reason |

## Auth And Moderation Identity

| ID | Task | Done When |
| --- | --- | --- |
| P2-006 | Steam auth ticket and ownership gate | Backend and Unreal integration plan verifies Steam identity before trusted reports, moderation identities, or protected server metadata are accepted |

## Server Discovery And Tool App

| ID | Task | Done When |
| --- | --- | --- |
| P2-007 | Steam Server Browser design gate | Design note records whether discovery uses Steam Game Servers API, Steam Lobby metadata, backend fleet metadata, or a staged combination, with gameplay code kept endpoint-agnostic |
| P2-008 | Steam Game Servers API registration prototype | Dedicated server registers a development server entry with required metadata and logs registration success/failure without blocking local offline validation |
| P2-009 | In-game Server Browser prototype | Game UI lists official/community, region, player count, map, ruleset, password/private state, and join rejection reasons from a local or Steam-backed provider |
| P2-010 | Dedicated Server Tool App checklist | Steam Tool App package checklist exists for AppID split, Dedicated Server Redistributables, `steam_appid.txt`, example config, launch scripts, log path, and local banlist path |
| P2-011 | SteamCMD install verification plan | A Windows runbook verifies anonymous or permitted SteamCMD install of the dedicated server tool into a clean folder and launches it with example config |

## Steam Datagram Relay

| ID | Task | Done When |
| --- | --- | --- |
| P2-012 | SDR connection model decision | Decision record chooses hosted dedicated server, FakeIP, known data center, or staged fallback; it states whether a backend coordinator must issue relay auth tickets |
| P2-013 | SDR prototype gate | A development server/client path uses the chosen relay model or records the blocker and fallback; gameplay code does not depend on raw public IP assumptions |

## Voice

| ID | Task | Done When |
| --- | --- | --- |
| P2-014 | Voice Chat Interface provider spike | Unreal Voice Chat Interface provider choice is recorded with setup steps, cost/licensing constraints, mute/block behavior, and Windows test command |
| P2-015 | Proximity voice gameplay acceptance | 5-8 player test confirms distance falloff, wall/room attenuation decision, dead/downed/contained voice rules, mute/block UX, and no external chat-service dependency in the play loop |
