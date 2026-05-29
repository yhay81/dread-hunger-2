# Commercial Quality Target

## Purpose

Target a commercial multiplayer survival deduction game at the quality level players expect from the best shipbound proximity-voice titles, while building a distinct product identity. The benchmark is player-facing craft, reliability, tension, and replayable social stories, not copied characters, lore, roles, items, UI, map layouts, audio, store copy, or signature mechanics.

## Quality Pillars

1. **Social tension every match**
   Players must need each other, doubt each other, and leave each match with at least one explainable incident. Suspicion should emerge from observed behavior, missing information, voice distance, resource pressure, and competing priorities.

2. **Readable survival pressure**
   Ship, weather, heat, route, power, radio, fuel, and injury systems must be legible enough for new players to understand why they are losing. Failure states may be harsh, but they must not feel arbitrary.

3. **Fair hidden-role counterplay**
   Sabotage must create uncertainty without erasing evidence. Every major hostile action needs at least one trace: timing, location, resource delta, sound cue, witness opportunity, repair clue, or loggable game-state change.

4. **Authoritative multiplayer stability**
   Server authority, deterministic match state, reconnect-safe sessions, and debuggable logs are core product quality, not polish. A fun match that desyncs is a failed match.

5. **Distinct expression**
   The game must stand on its own: near-future fictional icebound research vessel, diesel-electric/battery failure chains, agents, radio/route deception, industrial audio, and original UI language. Do not ship placeholder text, names, silhouettes, layouts, or mechanics that invite confusion with another title.

6. **Operational trust**
   Players need confidence that sessions are joinable, moderation works, and abuse can be acted on. Reporting, muting, bans, server visibility, and crash diagnostics are part of the core feature set. **(Updated 2026-05-29, `docs/business-model.md` D4/D8: multiplayer runs on official AWS; community hosting is dropped for now, so operational trust depends on the official fleet's availability rather than community hosts.)**

## Measurable Gates By Phase

| Phase | Target | Required gate |
| --- | --- | --- |
| 0: Definition | Scope, IP boundary, and test method are fixed. | GDD, IP boundary, QA plan, asset ledger, logging schema, and first playable success metrics exist. No unresolved reference dependence on another game's names, UI, roles, map layouts, or protected expression. |
| 1: Whitebox | Prove the social loop before production art. | 8-player matches complete end to end. At least 5 human matches are recorded. Median match length is 10-35 minutes. At least 60% of testers answer that they would play another match. Logs can reconstruct role assignment, tasks, sabotage, deaths, disconnects, and match end. |
| 2: Vertical Slice | Prove one shippable match slice. | 8-player Steam lobby or dedicated-server flow works. Proximity voice, mute, kick, report, reconnect, results screen, tutorial prompts, and one polished map route are playable. Crash-free completion is at least 90% across 30 external test matches. p95 server tick time and p95 ping are tracked. |
| 3: Closed Playtest | Prove retention and supportability. | 100-500 external players. Day-1 return among invited testers is at least 25%. Median time-to-first-match is under 4 minutes. Critical bug repro rate is above 80% from logs and crash bundles. Balance stays within 40-60% win rate by team after excluding known-abuse matches. |
| 4: Early Access | Prove commercial baseline. | Public build supports stable matchmaking/match list (official AWS), premium-gated match hosting, moderation workflow, crash reporting, patch notes, and onboarding (community hosting dropped — `docs/business-model.md` D8). 95% of completed matches end by rules rather than crash, timeout, or forced restart. New-player objective comprehension is at least 70% after one match. |
| 5: 1.0 Candidate | Prove durable product quality. | No P0/P1 known issues. All release content has ownership recorded. Localization, accessibility baseline, server operations, support queue, store assets, trailer capture, and legal review are complete. Two release-candidate weekends pass without rollback. |

## Non-Negotiable Release Bars

- **Original identity:** no names, roles, item labels, UI compositions, map room layouts, lore beats, audio cues, or marketing claims that imply sequel, remake, affiliation, or substitute branding.
- **Match completion:** at least 95% crash-free and softlock-free completion in release-candidate multiplayer tests with 8 players.
- **Network correctness:** no known reproducible desync that changes win/loss, role visibility, inventory ownership, damage, task state, sabotage state, or player position in a way players can exploit.
- **Voice and safety:** proximity voice, mute, report, kick/ban, death/spectator channel rules, and abuse evidence capture work before public sale.
- **Onboarding:** a first-time player can identify the team goal, personal next step, failure reason, and how to report abuse without external documentation.
- **Fair evidence:** every sabotage and lethal failure path leaves enough observable evidence for deduction, counterplay, or post-match explanation.
- **Performance:** target 60 FPS client performance on minimum spec in representative scenes, with documented p95 frame time, server tick time, memory, bandwidth, and load limits.
- **Operations:** crash IDs, build IDs, session IDs, server logs, moderation records, and patch versions are linked well enough to investigate live issues within one business day.
- **Content rights:** every shipped asset, sound, font, animation, text block, generated asset, and marketplace dependency has a recorded source, license, and approval status.
- **Rollback readiness:** release builds can be rolled back, hotfixed, or server-disabled without corrupting player progress or active official-fleet sessions (community-hosted sessions dropped — `docs/business-model.md` D8).

## Quality Review Cadence

At the end of each phase, review three evidence sets before advancing: playtest recordings, telemetry/log summaries, and IP/source-control audit notes. Advance only when the phase gate is met, explicitly waived with an owner and expiry date, or cut from scope. A feature that increases novelty but weakens match completion, evidence readability, or operational trust does not pass the commercial quality target.
