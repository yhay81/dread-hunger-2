# Voice Chat Interface Spike

Proximity voice is part of the core game loop. Phase 2 should validate it as an in-game system, not as a social workaround.

## Goal

Choose and test a provider through Unreal's Voice Chat Interface or the closest viable abstraction. The spike should prove join, leave, mute, block, transmit, receive, and proximity behavior in an 8-player match.

## Candidate Order

1. Unreal Voice Chat Interface with a provider that supports channels, device selection, mute/block, and future platform flexibility.
2. Steam-oriented voice path only if the game remains Steam-exclusive and data transport/proximity routing are explicitly handled.
3. Custom voice transport is rejected for Phase 2.

## Gameplay Rules To Test

- Living players use proximity voice.
- Distance falloff supports quiet private speech and readable nearby speech.
- Walls, doors, and exterior exposure need an explicit attenuation decision.
- Downed, contained, and dead players need separate speaking/listening rules.
- Saboteur private communication must not leak through normal proximity channels.
- Mute and block must be accessible from player list, scoreboard, and report flow.

## Technical Acceptance

- Provider setup steps are documented for Windows.
- `docs/voice-provider-validation-template.md` is filled for the provider run under review.
- A test map/match can connect at least 5 players.
- Voice state changes are logged as metadata only, not recordings or transcripts.
- Voice-abuse reports can attach structured match metadata, local slots, voice state, immediate action, and recent server event IDs without accepting raw voice or transcript fields.
- No raw voice data, recordings, transcripts, SteamIDs, IP addresses, or personal identifiers are committed.
- The implementation can be disabled for automated smoke tests.

## Output

- Provider decision and rejected alternatives.
- Windows setup notes.
- Test command and expected result.
- Completed voice provider validation template.
- Known cost/licensing limits.
- Required moderation UX follow-up items.

