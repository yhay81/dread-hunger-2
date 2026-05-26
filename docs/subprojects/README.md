# Continuous Goal Portfolios

Date: 2026-05-25

## Purpose

This directory defines Frostwake subprojects as continuous goal portfolios. Each portfolio owns a product outcome, the signals that prove whether the outcome is improving, and the guardrails that keep agents from optimizing the wrong thing.

Use these files when assigning another agent. Give the agent one portfolio, the latest evidence, and one improvement question. The agent should return evidence, a decision, or a focused diff that improves a named signal.

## Current Baseline

Phase 0 is complete and Phase 1 has reached the local automation milestone. Local listen-server smoke coverage validates 5-8 player starts, hidden role assignment, core interaction loops, combined systems, and JSONL evidence.

The product is still unproven where it matters most:

- Human 6-8 player fun, clarity, and replay desire are not yet proven.
- Windows dedicated-server validation is still pending.
- Steam Lobby, identity, server discovery, SDR, voice, moderation, onboarding, production art, localization, store readiness, external playtest, and release operations are still future work.

## Portfolio Map

| ID | Portfolio | North-star goal | Primary signal |
| --- | --- | --- | --- |
| GP-01 | [Human Playability Proof](sp-01-phase1-human-proof.md) | Players finish, understand, and want another match | Repeat-play intent and observed comprehension |
| GP-02 | [Network And Hosting Reliability](sp-02-windows-dedicated-server-foundation.md) | 8-player sessions are stable, reproducible, and hostable | Crash-free dedicated-server completion evidence |
| GP-03 | [Core Match Experience](sp-03-gameplay-vertical-slice.md) | Every round creates readable cooperation, suspicion, pressure, and resolution | Memorable incidents with explainable causes |
| GP-04 | [Steam Online Access](sp-04-steam-online-foundation.md) | Players can find, join, reject mismatches, and host through Steam-safe paths | Successful join flow without trusting lobby state |
| GP-05 | [Voice And Player Trust](sp-05-voice-and-moderation.md) | Communication is playable and abuse is manageable | Proximity voice works with mute/report/kick/ban paths |
| GP-06 | [Product Services And Tools](sp-06-backend-admin-ops-tools.md) | Non-authoritative services support tests and operations without owning gameplay | Tested backend/tool contracts and safe data boundaries |
| GP-07 | [Evidence, QA, And Performance](sp-07-qa-telemetry-performance.md) | Every claim has reproducible evidence | Gate results linked to logs, builds, maps, profiles, and summaries |
| GP-08 | [Distinct Presentation And Rights](sp-08-art-audio-ip-store-readiness.md) | The game looks, sounds, and sells as its own legally safe product | Approved asset provenance and non-misleading store material |
| GP-09 | [Comprehension And Accessibility](sp-09-onboarding-ui-localization-accessibility.md) | First-time players know what to do and can operate the game | Objective comprehension after one match |
| GP-10 | [Release And Community Operations](sp-10-release-operations-community.md) | Public tests and releases can be operated, supported, and rolled back | Launch checklist readiness with owners and cancel conditions |

## Continuous Improvement Loop

Each portfolio runs the same loop:

1. Observe the latest evidence: playtest notes, smoke logs, build reports, screenshots, support incidents, or review findings.
2. Pick one weak signal that matters to the portfolio's north-star goal.
3. Change the smallest thing likely to improve that signal.
4. Verify with the portfolio's evidence standard.
5. Record the result, including what got better, what got worse, and the next question.

Agents should not receive broad prompts like "finish Steam" or "improve gameplay." They should receive a portfolio plus a concrete improvement question, for example: "In GP-03, improve whether players can explain why the ship failed after a sabotage-heavy round."

## Sequencing

GP-01, GP-02, GP-06, and GP-07 can improve immediately and in parallel.

GP-03 should follow human evidence from GP-01, otherwise it risks polishing unproven assumptions.

GP-04 implementation depends on GP-02 dedicated-server evidence, though design and config safety can improve earlier.

GP-05 provider research can improve now, but acceptance needs an 8-player path.

GP-08 can improve asset provenance and visual direction now, but final art production should wait for GP-01 and GP-03 signals.

GP-09 can improve glossary, UI questions, and comprehension checks now, then implement against stable gameplay terms.

GP-10 can improve readiness checklists now, then execute only after GP-04, GP-05, GP-07, and GP-08 support a public test.

## Agent Handoff Format

For another agent, include:

- Portfolio file.
- Latest evidence file or current blocker.
- One improvement question.
- Files or systems allowed to change.
- Evidence expected before the agent stops.
- Reviewer role.

The output should be one of: evidence summary, decision memo, focused diff, test report, or updated portfolio note. Avoid isolated task completion that does not improve a named signal.
