# Agent Operating Model

## Purpose

Use subagents to create narrow, reviewable outputs that can be integrated into Frostwake without losing product direction, network authority, QA reproducibility, Steam compliance, localization consistency, or IP safety.

Subagents do not own final product decisions. They own prepared work products. Integration happens only after the relevant lead reviews the output against the gates in this document and the source docs it touches.

## Operating Principles

- Assign one directly responsible agent for each task.
- Give every task a written scope, expected output, source docs to respect, and explicit non-goals.
- Prefer small tasks that finish with a diff, checklist, test report, or decision memo.
- Do not let exploratory research land directly in implementation files.
- Separate invention, implementation, verification, release operations, localization, and IP review.
- Keep unresolved disagreement visible instead of blending conflicting answers into one plan.
- Record assumptions, source links, build versions, seeds, and test data needed to reproduce decisions.

## Agent Ownership

| Agent | Owns | Does Not Own | Primary Outputs |
| --- | --- | --- | --- |
| Game Design | Core loop, role rules, tasks, sabotage, PvE pressure, pacing, balance targets, playtest interpretation | Network authority, Steam API design, final legal judgment | GDD updates, tuning tables, playtest hypotheses, keep/cut/change recommendations |
| Unreal C++ | Server-authoritative gameplay, replication, GameMode/GameState/PlayerState, components, DataAsset/DataTable integration, JSONL match logs | Final balance decisions, store copy, legal/IP approval | C++/Blueprint integration plan, code diffs, replication notes, perf risks, implementation test notes |
| QA | 8-client orchestration, bot tests, crash/sync repros, log analysis, playtest records, quality gates | Deciding feature direction alone, rewriting gameplay systems | Repro steps, bug reports, automated run summaries, match metrics, gate pass/fail reports |
| Steam/Ops | Steamworks readiness, lobby/server browser/dedicated server rollout, Playtest logistics, build/release checklist, moderation ops, public comms drafts | Real-time gameplay authority, final IP approval, localization signoff | Steam task list, release checklist, server hosting notes, patch notes, store/FAQ drafts |
| Localization | Japanese, English, and Simplified Chinese strings; glossary; tone consistency; string length checks | Changing mechanics through translation, legal claims, unreviewed marketing promises | String tables, glossary updates, translation notes, ambiguity reports, UI overflow risks |
| IP Review | Reference boundary, naming safety, asset/source provenance, AI disclosure implications, public claim risk | Tuning, engineering implementation, marketing creativity after approval | Risk register updates, approve/block notes, required rewrites, asset ledger review findings |

## Task Routing

Start each task with this routing rule:

1. If the task changes player-facing rules or match experience, Game Design owns it.
2. If the task changes authoritative runtime behavior, Unreal C++ owns it.
3. If the task asks whether something works, regressed, scales, or can be reproduced, QA owns it.
4. If the task touches Steamworks, release process, server operations, moderation operations, or public Steam copy, Steam/Ops owns it.
5. If the task changes player-facing text in more than one language, Localization owns it.
6. If the task uses references, names, store claims, AI-assisted assets, third-party code/assets, or competitor comparisons, IP Review owns it.

When a task crosses areas, assign one owner and list required reviewers. Example: a new sabotage feature is owned by Game Design until rules are stable, Unreal C++ during implementation, QA for verification, Localization for strings, and IP Review if the name or fiction resembles protected references.

## Handoff Contract

Every subagent handoff must include:

- Objective: what decision or artifact is needed.
- Scope: files, systems, docs, or build being touched.
- Inputs: source docs, issue links, logs, screenshots, seeds, and constraints.
- Non-goals: what the agent must not solve.
- Output format: diff, table, checklist, repro, test report, memo, or localized string table.
- Acceptance gate: how the output will be judged before integration.
- Reviewer: named role that can approve, request changes, or block.

Do not integrate handoffs that omit assumptions, verification status, or unresolved risks.

## Conflict Rules

| Conflict | Tie-Breaker |
| --- | --- |
| Fun vs. technical complexity | Phase goal wins. Phase 1 favors playable proof and logs over systemic depth. |
| Design intent vs. replication safety | Server authority and reproducibility win. Redesign the feature if it cannot be made authoritative. |
| Speed vs. QA evidence | No integration without at least the minimum agreed check for the touched surface. |
| Steam/Ops readiness vs. feature ambition | Do not promise or expose features that are not implemented and verified. |
| Localization clarity vs. literal wording | Player comprehension wins, then glossary consistency. |
| Marketing strength vs. IP safety | IP safety wins. Rewrite names, copy, visuals, or references. |
| AI-generated convenience vs. rights uncertainty | Rights certainty wins. Keep uncertain assets as placeholders only or remove them. |
| Two agents disagree with plausible evidence | Owner writes a decision note, reviewers add objections, and the producer makes the call for the current phase. |

Block integration immediately when:

- A change imports protected expression or third-party assets without approval.
- Runtime gameplay authority moves out of Unreal Dedicated Server/C++ without an explicit architecture decision.
- QA cannot reproduce a reported pass/fail condition.
- Store, trailer, capsule, or public copy claims unimplemented features.
- Raw logs, SteamIDs, IP addresses, voice/chat text, secrets, or private reference files would be committed.

## Review And Check-In Cadence

| Cadence | Participants | Purpose | Output |
| --- | --- | --- | --- |
| Daily async check-in during active implementation | Current owner plus required reviewers | Surface blockers, changed assumptions, integration risk | Three bullets: done, next, blocked |
| Twice-weekly design/engineering sync | Game Design, Unreal C++, QA | Keep rules, implementation, and observability aligned | Decision log and updated acceptance checks |
| Weekly QA gate | QA, Game Design, Unreal C++ | Review automated runs, repro quality, playtest findings, and crash/sync risks | Gate pass/fail, bug priorities, next test matrix |
| Weekly Steam/Ops readiness review from Phase 2 onward | Steam/Ops, QA, Unreal C++, IP Review, Localization as needed | Track lobby/server, Playtest, moderation, build, and public-page readiness | Release risk list and owner/date for each blocker |
| Localization batch review when strings change | Localization, Game Design, UI owner, IP Review for public text | Preserve meaning, glossary, length, and store claim safety | Approved string table or change requests |
| IP review before public exposure or asset adoption | IP Review, owner of the artifact | Prevent protected expression, rights gaps, or unsafe claims | Approve/block note with required edits |
| Phase gate review | All owners | Decide whether to proceed, cut, or extend the phase | Phase gate decision with evidence links |

Check-ins should be short. Long debate moves into a decision note with evidence, options, recommendation, and owner.

## Output Evaluation Before Integration

### Game Design

Accept only when:

- The rule or tuning change supports the current phase goal.
- Player objective, counterplay, failure states, and expected match impact are written down.
- QA can observe the result through logs, playtest prompts, or clear in-game behavior.
- The change does not add unexplained complexity that blocks 8-player match completion.

### Unreal C++

Accept only when:

- Authority is server-side for movement-sensitive, combat, role, inventory, task, sabotage, match result, and critical state changes.
- Replicated state has clear ownership and does not depend on client trust.
- Logs include enough fields for QA to reconstruct the changed behavior.
- The implementation has a focused test path: unit test, PIE/manual checklist, bot run, or 8-client run depending on risk.
- Blueprint use stays presentation-focused unless a local architecture note approves otherwise.

### QA

Accept only when:

- Repro steps include build, map, seed, player count, bot/human mix, and expected result.
- Automated or manual results separate pass, fail, blocked, and not tested.
- Critical bugs include logs or a concrete reason logs are missing.
- Metrics match the current quality gate in `docs/qa-plan.md`.
- Flaky results are rerun or labeled as inconclusive.

### Steam/Ops

Accept only when:

- Steamworks assumptions are rechecked against current official docs before submission or payment decisions.
- Release notes, store copy, FAQ, and Playtest announcements claim only implemented or explicitly scheduled features.
- Dedicated server, lobby, moderation, reporting, and data retention impacts are called out.
- Build/release checklists include owner, date, artifact location, rollback or cancel condition, and review state.

### Localization

Accept only when:

- Source strings are stable enough for translation or marked as draft.
- Glossary terms are applied consistently across Japanese, English, and Simplified Chinese.
- Text fits likely UI constraints or reports overflow risk.
- Ambiguous source text is returned to Game Design or Steam/Ops instead of guessed.
- Public-facing translated claims receive IP Review when they describe product identity or competitor positioning.

### IP Review

Accept only when:

- Names, mechanics descriptions, art directions, and store copy respect `docs/ip-boundary.md`.
- Reference usage follows `docs/reference-policy.md`.
- AI-assisted content that may ship is recorded according to `docs/ai-content-disclosure.md` and `docs/asset-ledger.md`.
- Third-party code/assets have license, commercial-use, redistribution, and attribution status recorded.
- Public release artifacts have no unresolved high-risk findings.

## Integration Workflow

1. Owner opens or updates a task with the handoff contract.
2. Subagent produces the smallest useful output and marks verification status.
3. Required reviewers evaluate against this document and the relevant source docs.
4. Owner resolves conflicts or records a decision note.
5. QA verifies behavior when runtime, build, release, or player-facing risk exists.
6. IP Review signs off before public assets, store text, AI-assisted shippable assets, or reference-derived work are adopted.
7. Integrator lands the change only with evidence links, known risks, and follow-up tasks.

## Minimum Evidence By Artifact

| Artifact | Required Evidence |
| --- | --- |
| Rule/spec change | Design rationale, expected player behavior, QA observation method |
| Unreal implementation | Diff, authority/replication note, test result, log fields touched |
| QA report | Repro details, build/seed/log links, pass/fail state, severity |
| Steam/Ops plan | Current-doc check date, risk list, owner/date, public-claim review |
| Localized text | Source string version, target languages, glossary notes, UI length risk |
| IP decision | Source/reference considered, risk level, approve/block outcome, required edits |

## Practical Limits

- Do not run more than three active subagent threads against the same feature unless there is a written integration owner.
- Do not ask a subagent to both implement and approve its own high-risk output.
- Do not assign broad prompts such as "improve the game"; split into design, implementation, QA, ops, localization, or IP tasks.
- Do not keep stale agent output alive after the source docs, architecture, or phase goal changes.
- Archive rejected outputs with the reason so the same unsafe idea is not reintroduced later.
