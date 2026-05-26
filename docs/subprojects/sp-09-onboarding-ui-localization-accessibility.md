# GP-09: Comprehension And Accessibility

## North-Star Goal

First-time players can understand their objective, operate the interface, recover from confusion, and read critical game state without external explanation.

## Why This Exists

An 8-player betrayal game fails if new players cannot tell what to do, why they lost, or how to communicate safely. This portfolio continuously improves comprehension, UI language, localization, and accessibility.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| First-match comprehension | Players can state team goal, personal next step, and loss reason after one match |
| UI clarity | Role, objectives, system state, interactions, danger, and results are legible |
| String stability | Source strings are owned, reviewable, and ready for translation batches |
| Localization consistency | Japanese, English, and Simplified Chinese follow glossary and tone |
| Text fit | Long strings fit likely UI constraints without overlap |
| Accessibility baseline | Controls, contrast, readability, captions/subtitles policy, and audio alternatives are documented |

## Improvement Questions

- What did new players misunderstand most recently?
- Which UI state would have prevented the confusion?
- Which term is unstable or inconsistent across docs and UI?
- Which strings are too early to localize?
- Which accessibility gap blocks external testing?

## Evidence Standard

Use playtest comprehension notes, UI screenshots, source-string inventories, glossary updates, overflow checks, and accessibility checklist results.

Primary docs:

- `docs/game-design.md`
- `docs/localization-glossary.md`
- `docs/accessibility-baseline.md`
- `docs/qa-plan.md`
- `docs/commercial-quality-target.md`

## Boundaries

- Do not finalize broad translation before source strings stabilize.
- Do not change mechanics through translation.
- Do not use UI text to promise unimplemented Steam, voice, server, moderation, or localization features.
- Do not add tutorial text that masks a broken core interaction.

## Review Cadence

Review after playtests, after UI string changes, and before external tester waves. The main question is whether a new player needs less out-of-game explanation than before.

## Agent Charter

An agent assigned to this portfolio should improve one comprehension signal. A good assignment is: "Using the latest playtest notes, identify the one UI or wording change most likely to improve first-match objective comprehension."
