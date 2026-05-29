# Voice Provider Decision Memo

- Date: 2026-05-29
- Author: GP-05 lane agent (claude-sonnet-4-6)
- Reviewer role: Steam/Ops (with Game Design)
- Status: DECISION MEMO — awaiting reviewer sign-off before Build.cs wiring cycle opens

---

## 1. Context

Frostwake requires in-game proximity voice as a core gameplay feature. External out-of-game communication is explicitly rejected as the shipped gameplay path (see `docs/subprojects/sp-05-voice-and-moderation.md` Boundaries; `docs/technical-architecture.md` § Voice Architecture; `docs/voice-chat-interface-spike.md`). The architecture mandates the Unreal Voice Chat Interface (VCI) as the abstraction layer, with a proven provider below it. Custom voice transport is rejected for all phases.

The three candidates named in `docs/voice-chat-interface-spike.md` § Candidate Order are evaluated below.

---

## 2. Requirements Matrix

These requirements come from `docs/voice-chat-interface-spike.md` § Gameplay Rules To Test and § Technical Acceptance, and from `docs/subprojects/sp-05-voice-and-moderation.md` § Health Signals.

| Requirement | Abbrev |
| --- | --- |
| Distance falloff (quiet private / readable nearby speech) | DIST |
| Explicit wall/door/room attenuation decision | WALL |
| Downed player speaking/listening rules | DOWN |
| Contained player speaking/listening rules | CONT |
| Dead/spectator voice rules | DEAD |
| Saboteur private channel isolation (no proximity leak) | SAB |
| Mute/block accessible from player list, scoreboard, report flow | MUTE |
| Report attaches metadata only (no raw voice or transcript) | META |
| Reconnect with session restore (channel rejoins cleanly) | RECON |
| Windows setup steps documented + repeatable | SETUP |
| Provider can be disabled for automated smoke tests | SMOKE |
| No external chat app as the shipped gameplay path | NOCHAT |
| Cost/licensing understood (free tier, per-user risk, redistribution limits) | COST |
| VCI-compatible abstraction | VCI |

---

## 3. Candidates

### 3A. Unreal VCI + EOS Voice Chat Plugin

**Description.** Epic Online Services (EOS) ships a first-party voice chat plugin for Unreal Engine (`EOSVoiceChat` module). It integrates with the Unreal Voice Chat Interface abstraction. EOS Voice supports channels, device selection, mute/block, volume, and proximity callbacks.

Sources consulted: `docs/best-practice-alignment.md` (official EOS Voice Chat plugin doc link); `docs/technical-architecture.md` § Voice Architecture; `docs/voice-chat-interface-spike.md`.

| Requirement | Assessment | Notes |
| --- | --- | --- |
| DIST | Achievable | VCI exposes volume/proximity attenuation callbacks; implementation maps to `voiceState=proximity` events |
| WALL | Design decision, not provider-dependent | WALL attenuation is a Unreal audio geometry decision, not a voice-transport decision; can be explicit pass or defer for Phase 2 |
| DOWN | Achievable | Channel switching on `EFrostwakeLifeState::Downed` is a server-authoritative state change exposed through VCI channel assignment |
| CONT | Achievable | Same channel-switch path as DOWN; contained players can be routed to a restricted channel |
| DEAD | Achievable | Dead/spectator channel isolation is standard VCI channel management |
| SAB | Achievable | A separate named EOS Voice channel for the saboteur team satisfies isolation; must be server-assigned, not client-selectable |
| MUTE | Achievable | EOS Voice exposes per-player mute at the VCI layer; client-side mute is immediate |
| META | Achievable | VCI does not expose transcripts or recordings by design; `voiceState` metadata maps to existing backend report fields |
| RECON | Achievable with care | EOS Voice supports rejoining a channel by name; reconnect behavior must be explicitly tested (see § 5 weakest signal) |
| SETUP | Well-documented | Official EOS + Unreal integration docs exist; plugin enable path and config location are known |
| SMOKE | Achievable | VCI implementations can be no-op-swapped or config-disabled for `FrostwakeServer` null-RHI smoke |
| NOCHAT | Satisfied by design | VCI + EOS Voice is entirely in-game; no external service hosts the match voice path |
| COST | Manageable | EOS free tier covers early closed-test scale; per-DAU threshold applies at growth scale; no redistribution fee for Unreal plugin use |
| VCI | Fully compatible | `EOSVoiceChat` module is the canonical VCI implementation from Epic |

**Risk.** EOS account linkage: EOS Voice in product mode requires EOS product credentials. Config (product ID, sandbox ID, deployment ID) must remain in ignored `Saved\Config\` paths — never committed. A null/dev mode can skip EOS auth for smoke tests.

**Licensing note.** EOS SDK is royalty-free for Unreal projects; redistribution through the plugin is covered under Unreal Engine EULA. Per-user voice minutes are currently free at closed-test scale under EOS free tier (as recorded in `docs/best-practice-alignment.md`).

---

### 3B. Unreal VCI + Vivox-style Provider

**Description.** A managed third-party voice service with a VCI adapter. The VCI abstraction means the gameplay code does not change if the provider is later swapped.

| Requirement | Assessment | Notes |
| --- | --- | --- |
| DIST | Achievable | Provider SDK handles transport; proximity attenuation is client-side via VCI callbacks |
| WALL | Design decision | Same as 3A |
| DOWN / CONT / DEAD | Achievable | Channel management same pattern as 3A |
| SAB | Achievable | Named private channel; same pattern as 3A |
| MUTE | Achievable | Standard VCI mute path |
| META | Achievable | Same metadata-only evidence path as 3A |
| RECON | Achievable | Provider SDKs typically support channel rejoin |
| SETUP | More complex | Requires a separate SDK download, account, and credential set; adds a third-party dependency outside the Epic ecosystem |
| SMOKE | Achievable | Config-disable pattern same as 3A |
| NOCHAT | Satisfied by design | In-game only |
| COST | Higher risk | Per-minute/per-user billing model at scale; redistribution terms and SDK licensing vary by vendor; requires a separate vendor contract before public scale |
| VCI | Compatible (via adapter) | VCI adapter must be implemented or sourced; more integration surface than 3A |

**Rejection reason.** Deferred over 3A. Both paths are VCI-compatible, but 3B adds a third-party vendor contract, independent SDK distribution and update dependency, and per-minute cost exposure that must be renegotiated at each scale step. 3A (EOS Voice) is first-party to the Unreal ecosystem, reduces vendor surface, and has no redistribution barrier within the Unreal EULA. 3B should be revisited only if EOS Voice fails reconnect or platform-flexibility acceptance tests.

---

### 3C. Steam Voice Fallback

**Description.** Steam's built-in voice capture and transmission path. Available without a separate vendor account on Steam-shipped builds.

| Requirement | Assessment | Notes |
| --- | --- | --- |
| DIST | Not natively supported | Steam Voice does not expose proximity attenuation callbacks; custom proximity logic would require the rejected custom-transport path |
| WALL | Not applicable | No attenuation layer |
| DOWN / CONT / DEAD | Limited | Steam Voice has no built-in channel switching model; per-state routing requires workarounds |
| SAB | High risk | Saboteur private channel isolation is not natively expressible in Steam Voice without custom routing logic |
| MUTE | Partial | Steam overlay mute exists but is not per-player within a match context |
| META | Achievable | No transcripts by default |
| RECON | Steam session-dependent | Voice reconnect ties to Steam networking session, not an independent channel join |
| SETUP | Simplest | No extra SDK; available in any Steam build |
| SMOKE | Conditional | Smoke must run with `OnlineSubsystemNull` where Steam Voice is unavailable; smoke disable path is required |
| NOCHAT | Satisfied | In-game |
| COST | Zero additional | Included in Steamworks |
| VCI | Not compatible | Steam Voice does not implement the Unreal Voice Chat Interface; using it requires bypassing the VCI abstraction or a custom wrapper |

**Rejection reason.** Steam Voice does not satisfy the VCI compatibility requirement (from `docs/technical-architecture.md` § Voice Architecture and `docs/voice-chat-interface-spike.md`). Proximity attenuation, per-state channel switching, and saboteur private channel isolation are not natively achievable without custom transport logic, which is explicitly rejected by the architecture. It is retained only as a last-resort fallback if the game becomes permanently Steam-exclusive and all VCI-based providers become unavailable — a scenario that does not apply to the current phase.

---

## 4. Decision

**Selected candidate: Unreal VCI + EOS Voice (Candidate 3A).**

Rationale:
- Only candidate that is fully VCI-compatible without a custom wrapper.
- First-party to the Unreal ecosystem; no third-party vendor contract or separate redistribution agreement required.
- Supports all required channel behaviors (proximity, per-life-state switching, saboteur isolation) through standard VCI channel management.
- Mute/block path is at the VCI layer, keeping implementation portable.
- Cost at closed-test scale (8 players) is zero under the EOS free tier.
- Config must remain in ignored `Saved\Config\` paths; no credentials in the repository.

Rejected candidates:
- **3B (Vivox-style)**: deferred; re-evaluate if EOS Voice fails the reconnect acceptance test or if a platform (non-Steam) requires a different provider.
- **3C (Steam Voice fallback)**: rejected; does not satisfy VCI compatibility or proximity/channel requirements without custom transport.

No external out-of-game communication service may be used as the shipped gameplay voice path. This rule is repeated here because it is an explicit architecture boundary, not a preference.

---

## 5. Cost and Licensing Summary

| Factor | EOS Voice (chosen) |
| --- | --- |
| SDK redistribution | Covered under Unreal Engine EULA; no separate fee |
| EOS product credentials | Required in product mode; must stay in ignored local config |
| Free tier (closed test) | Covers early closed-test scale; see current EOS pricing docs before any public announcement |
| Per-DAU growth risk | Applies at scale; review EOS pricing before Steam Playtest wave if player count is growing |
| Smoke/CI use | Requires a null/dev EOS mode or config-disable path; must not require live EOS auth for automated tests |
| Platform future | EOS Voice is multi-platform; VCI abstraction keeps platform options open |

No provider credentials, tokens, account IDs, or deployment IDs may be committed to the repository. Keep all EOS Voice config under `Saved\Config\WindowsEditor\` or an equivalent ignored path and record the config key names only in this memo.

Required config keys (key names only; values stay ignored and local):
- `DefaultEngine.ini` key group: `[OnlineSubsystem]` and `[EpicOnlineServices]` sections
- Plugin enable: `EOSVoiceChat` in `DefaultGame.ini` or `Frostwake.uproject` plugins array (added only when the Build.cs wiring cycle is opened)

---

## 6. 8-Player Acceptance Plan

This section covers the written acceptance plan for P2-015. It does not require a running dedicated server; it is a design plan that will be tested once GP-02 server-target gating is resolved.

### 6.1 Distance Falloff

Living players use proximity voice. Distance falloff should allow quiet private speech (approximately 2–4 meters, with volume near inaudible at sender's low speech level) and readable nearby speech (approximately 5–12 meters at normal speech). Volume should approach inaudible past roughly 15–20 meters, calibrated during 8-player testing. VCI attenuation parameters will be configured in Unreal audio settings, not in provider config. Acceptance: two observers at different ranges report voice is distinguishable at near range and inaudible at far range.

### 6.2 Wall and Room Attenuation

The Phase 2 decision for walls and exterior exposure is: **explicit pass (not defer)**. Voice attenuation through walls will be handled at the Unreal audio spatialization layer, not the voice transport layer. This allows the transport (EOS Voice) to carry the signal and lets the Unreal audio system apply occlusion geometry. The test map (`L_IcebreakerWhitebox`) has door actors and bulkhead geometry suitable for basic attenuation probing. Acceptance signal: observer in an adjacent room reports voice is noticeably attenuated versus line-of-sight range at the same distance.

### 6.3 Downed, Contained, and Dead Voice Rules

These rules map to `EFrostwakeLifeState` values in `FrostwakeTypes.h`:

| Life state | Speaking | Listening | Channel |
| --- | --- | --- | --- |
| Alive | Yes | All nearby alive | Proximity |
| Downed (`Downed`) | Proximity-limited (can cry out) | Can hear nearby alive | Same proximity channel; volume reduced |
| Contained (`Contained`) | Restricted (crew only, close range) | Restricted | Separate contained channel or suppressed proximity |
| Dead / Spectating | No | Spectator channel only (if enabled) or silent | Dead channel, not proximity |

Server authority: channel assignment must be triggered by a server-authoritative `EFrostwakeLifeState` change, not a client request. The VCI channel switch is a client action initiated on receiving the server-replicated life state update. Acceptance: observer logs confirm `voiceState=downed`, `voiceState=contained`, `voiceState=dead_spectator` events align with server-side life state transitions.

### 6.4 Saboteur Private Channel

The saboteur team uses a named EOS Voice channel separate from the proximity channel. Assignment is server-authoritative (triggered on role assignment, not client request). The saboteur channel must not be audible through the proximity path to crew members. Acceptance: crew observer confirms no saboteur voice is audible during a saboteur private exchange at any range.

### 6.5 Reconnect Behavior

When a client disconnects and reconnects mid-match, the VCI channel rejoin must restore the player's correct voice state (proximity for alive, correct alternative for other life states). The current Phase 1 policy in `docs/technical-architecture.md` § Server Authority Rules explicitly states no-mid-match-reconnect until Phase 2 dedicated-server identity and rejoin behavior are designed. The reconnect voice path is therefore a Phase 2 acceptance item. Acceptance plan: on reconnect, the server re-sends the current `EFrostwakeLifeState` to the reconnected client; the client VCI layer re-joins the correct channel on receipt. A log event `voiceState=<channel>` on reconnect confirms restore.

### 6.6 Mute, Block, and Report

- **Mute** is immediate, local, and accessible from player list and scoreboard. The VCI `MutePlayer` call takes effect before the next audio frame.
- **Block** persists for the session. A persistent-block list stored in local session state (not the backend) prevents the blocked player from being heard for the remainder of the match.
- **Report** attaches structured metadata only: `matchId`, `buildId`, `mapId`, local player slots, `voiceState`, `actionTaken`, `matchTimeSeconds`, `recentEventIds`, and `category`. No transcript, recording URL, SteamID, IP address, or device identifier field is accepted by the backend report endpoint (see `apps/backend/openapi.yaml`).
- **Kick/ban** path is scoped to moderation owner review; not self-service during closed test.

Acceptance: observer confirms mute takes effect immediately (no audible output after mute action); backend dry-run report payload passes the field validation gate in `docs/voice-provider-validation-template.md` § Moderation Payload Probe.

---

## 7. Weakest 8-Player Acceptance Signal

**Weakest signal: reconnect with session restore.**

Reasoning:
- Reconnect is explicitly deferred to Phase 2 by the current no-mid-match-reconnect policy (`docs/technical-architecture.md` § Server Authority Rules).
- It requires both server-side identity re-association (GP-02 dedicated-server target, which is currently blocked by the Launcher UE limitation) and a VCI channel-rejoin sequence that has not been tested at any scale.
- The risk is that a reconnected player either (a) silently lands in the wrong channel (e.g., hears dead players while alive), or (b) is unable to rejoin voice at all, requiring a full match restart.
- All other signals (distance falloff, life-state channels, saboteur isolation, mute) have a local test path that can run on a listen-server with the provider plugin integrated. Reconnect specifically requires a dedicated-server target and a real disconnect event, both of which require GP-02 gating.

The second weakest signal is saboteur private channel isolation, because it requires server-authoritative channel assignment to be correctly wired to role assignment before the 8-player test can validate that crew members cannot hear saboteur voice.

---

## 8. Provider Decision Table (for voice-provider-validation-template.md)

This table maps directly to the Provider Decision rows in `docs/voice-provider-validation-template.md`:

| Check | Result | Evidence | Notes |
| --- | --- | --- | --- |
| Provider selected | pass | `docs/voice-provider-decision.md` § 4 | Unreal VCI + EOS Voice; rejected alternatives documented in § 3B and § 3C |
| Rejected alternatives documented | pass | `docs/voice-provider-decision.md` § 3B, § 3C | Vivox-style deferred (third-party cost/contract risk); Steam Voice rejected (no VCI, no proximity attenuation) |
| Cost/licensing understood | pass | `docs/voice-provider-decision.md` § 5 | EOS free tier covers closed-test; per-DAU risk noted; no redistribution fee; credentials must stay ignored |

Remaining Provider Decision rows (Windows setup repeatable, automated smoke disable switch exists, config is local/ignored) are gated on the Build.cs wiring cycle, which opens only after reviewer sign-off on this memo.

---

## 9. Follow-Up Items

- Reviewer (Steam/Ops, with Game Design) must review this memo and sign off before the Build.cs wiring cycle is opened.
- Add `EOSVoiceChat` plugin to `Frostwake.uproject` and `VoiceChat` / `EOSVoiceChat` modules to `Source/Frostwake/Frostwake.Build.cs` only in that wiring cycle, with a QA acceptance gate.
- Write EOS Voice Windows setup notes (plugin enable, config key names, null-mode for smoke) as a separate `docs/eos-voice-setup.md` during the wiring cycle.
- Runtime reconnect acceptance test is gated on GP-02 server-target resolution; record this as a P2-015 known gap.
- Review EOS pricing against current docs before any Steam Playtest wave that mentions voice.
