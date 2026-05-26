# Accessibility Baseline

Current as of 2026-05-26 JST.

This baseline defines the minimum evidence to collect before Frostwake asks new players to judge comprehension, comfort, or readability. It is not a final compliance claim.

## Scope

Use this for greybox and vertical-slice playtests. The goal is to find blockers that prevent first-time players from understanding objectives, operating controls, reading critical state, or recovering from confusion.

## Baseline Checks

| Area | Baseline | Evidence |
| --- | --- | --- |
| Controls | Required movement, interaction, repair, sabotage-relevant, menu, and leave-session actions are reachable without hidden knowledge | Observer notes plus player reports |
| Readability | Critical role, objective, interaction, danger, and result text remains readable at the planned test resolution | Screenshot or observer check |
| Color and contrast | Danger, success, team, and interactable states are not communicated by color alone | Screenshot or UI review |
| Audio alternatives | Critical audio-only information has a visual, text, or observer-verifiable alternative before it is counted as a gameplay requirement | UI review and playtest notes |
| Captions and voice policy | Voice, captions, subtitles, mute, and report expectations are stated as test scope, not assumed as shipped features | Test packet and run notes |
| Motion comfort | Camera movement, screen effects, and whiteout-style states do not block basic orientation without a note and mitigation plan | Player report |
| Failure recovery | Players can identify why an interaction failed, why the ship is improving or failing, and why a match ended | Post-round survey |
| Text fit | Long English source strings and likely Japanese/Simplified Chinese drafts have a known fit risk before UI implementation is called done | String review |
| Glossary consistency | Public-facing and UI-facing terms follow `docs/localization-glossary.md` | Glossary review |

## Playtest Capture

Every committed human-test summary should include:

- objective comprehension: whether players could state the crew goal after one round;
- next-step clarity: whether players could name a useful next action without coaching;
- failure-state clarity: whether players could explain improvement, failure, or match end state;
- UI or control confusion: the specific label, prompt, control, or missing state that caused avoidable confusion;
- accessibility blocker: any readability, input, audio, caption, contrast, or motion issue that blocked participation;
- text or term issue: any unstable wording that should change before localization.

Use `none observed` only when an observer explicitly checked the signal. Blank fields mean the evidence was not collected.

## Non-Goals

- Do not claim full localization, captions, voice accessibility, controller support, or platform accessibility certification until the feature exists and has evidence.
- Do not hide unclear mechanics behind tutorial text. If players cannot infer the loop after the short test brief and one round, record that as product evidence.
- Do not commit raw transcripts, tester names, account identifiers, IP addresses, local machine paths, or moderation-sensitive details.
