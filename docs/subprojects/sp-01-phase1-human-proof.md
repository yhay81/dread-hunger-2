# GP-01: Human Playability Proof

## North-Star Goal

6-8 humans can start a match, understand what is happening, reach a meaningful ending, and want another round for reasons visible in the match rather than in the rules document.

## Why This Exists

Automated smoke tests prove that systems can run. They do not prove that players cooperate, suspect, recover, mislead, or remember the match. This portfolio continuously improves the evidence loop around real players.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Repeat-play intent | Most players want another match after the session |
| Objective comprehension | Players can state team goal and next step without prompting |
| Failure clarity | Players can explain why the match ended |
| Social incidents | Each match has at least one discussable suspicion, rescue, betrayal, or recovery moment |
| Completion reliability | Human sessions reach a rule ending or a clearly diagnosed failure |
| Confusion cost | Fewer stops caused by unclear UI, unclear role, unclear task, or unclear win/loss reason |

## Improvement Questions

- What prevented players from making meaningful decisions?
- Which moment made players want to continue?
- Which rule or UI element caused the most avoidable confusion?
- Did sabotage create evidence and conversation, or only arbitrary loss?
- Did PvE pressure force decisions, or only interrupt the round?
- What should be kept, cut, or changed before the next human test?

## Evidence Standard

Evidence should include anonymized playtest summaries, observer notes, post-round survey themes, log summaries, match duration, player count, build ID, map ID, and a keep/cut/change decision. Raw personal data and recordings stay out of commits unless explicitly approved and redacted.

Use the existing playtest packet and preflight flow:

- `docs/playtests/p1-024-human-test-1.md`
- `docs/playtests/p1-024-observer-sheet.md`
- `docs/playtests/p1-024-post-round-survey.md`
- `docs/playtests/p1-024-summary-template.md`
- `cargo run -p frostwake-tools -- playtest-preflight`

## Boundaries

- Do not treat another automated pass as a substitute for human proof.
- Do not commit to final art production from untested assumptions.
- Do not solve every complaint immediately. Improve the signal that blocks the next session.
- Do not include private identity, voice, or raw recordings in committed artifacts.

## Review Cadence

After every human session, update the evidence summary and decide the next improvement question. At the end of a testing batch, produce a Phase 2 scope recommendation.

## Agent Charter

An agent assigned to this portfolio should improve the human evidence loop. A good assignment is: "Using the latest playtest summary, identify the weakest playability signal and produce the next experiment or focused fix needed to improve it."
