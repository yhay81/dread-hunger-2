# GP-05: Voice And Player Trust

## North-Star Goal

Players can communicate through in-game proximity voice and trust that abuse can be muted, reported, reviewed, and acted on.

## Why This Exists

Voice is core gameplay for this genre, and moderation is part of shipping quality. This portfolio continuously improves communication quality and player safety together.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Proximity behavior | Distance, rooms, death/downed/contained states, and reconnect rules behave as designed |
| Comprehension | Players understand who can hear them and why |
| Safety controls | Mute/block/report/kick/ban are available where needed |
| Moderation evidence | Reports have enough metadata to review without unsafe raw voice storage |
| Provider risk | Cost, licensing, platform support, and operations are known |
| Public readiness | Closed-test exceptions shrink before Steam Playtest scale |

## Improvement Questions

- Did voice create useful suspicion and coordination in the last test?
- Did anyone avoid play because of voice or safety concerns?
- Can a player stop harmful audio quickly during a match?
- Can moderators act on reports without committing private identifiers or raw voice?
- Do death, downed, contained, and spectator voice rules improve the match?

## Evidence Standard

Evidence should include provider decision notes, 8-player validation results, voice-state behavior, mute/block/report test cases, moderation workflow notes, and privacy boundaries.

Primary docs:

- `docs/voice-chat-interface-spike.md`
- `docs/voice-provider-validation-template.md`
- `docs/technical-architecture.md`
- `docs/playtest-data-and-moderation.md`
- `docs/backend-contract.md`
- `docs/steam-playtest-checklist.md`

## Boundaries

- Do not use external voice chat as the shipped gameplay path.
- Do not implement custom voice transport unless a separate architecture decision approves it.
- Do not commit raw voice, transcripts, SteamIDs, IPs, or personal identifiers.
- Do not launch public scale without a moderation owner and minimum response path.

## Review Cadence

Review after provider research, after each voice test, and before every external tester wave. Player trust issues should be treated as product blockers, not polish.

## Agent Charter

An agent assigned to this portfolio should improve communication trust. A good assignment is: "Compare the current voice provider option against mute/report/reconnect requirements and identify the weakest acceptance signal."
