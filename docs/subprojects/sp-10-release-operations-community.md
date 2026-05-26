# GP-10: Release And Community Operations

## North-Star Goal

Frostwake can run private tests, Steam Playtests, Early Access, and 1.0 releases with clear ownership, support, moderation, rollback, and public communication.

## Why This Exists

Release quality is an operating system, not a launch-day checklist. This portfolio continuously improves the ability to put builds in players' hands and recover when something goes wrong.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Readiness clarity | Each launch prerequisite has owner, artifact, status, blocker, and cancel condition |
| Build confidence | Upload, install, launch, symbols/logs, and rollback are rehearsed |
| Support coverage | Tester support route and moderation response expectations are known |
| Public claim accuracy | Store copy, patch notes, and known issues match the actual build |
| Community hosting | Dedicated server distribution and community-host runbooks are usable |
| Wave discipline | Tester cohorts scale only when logs, moderation, and stability justify it |

## Improvement Questions

- What would force us to cancel or roll back the next tester wave?
- Which launch prerequisite lacks a real owner or artifact?
- Can a tester find help without external chat dependence?
- Can a community host install and run the server from clean instructions?
- Do public notes describe the build honestly?

## Evidence Standard

Use readiness tables, build/upload dry runs, Steam checklist reviews, support incident templates, moderation rota, patch notes, rollback notes, and post-wave summaries.

Primary docs:

- `docs/steam-playtest-checklist.md`
- `docs/steam-store.md`
- `docs/store-copy-drafts.md`
- `docs/server-hosting-model.md`
- `docs/windows-dedicated-server-runbook.md`
- `Tools/build_and_upload.md`
- `Tools/ops/runbook.md`

## Boundaries

- Do not publish claims for unimplemented features.
- Do not open public signups before moderation, support, and rollback readiness are clear.
- Do not make external chat the required way to find or play matches.
- Do not scale tester waves faster than GP-02, GP-05, and GP-07 evidence supports.

## Review Cadence

Review before every tester wave, build upload, store update, and public announcement. The release question is not "can we ship?" but "can we operate what we ship?"

## Agent Charter

An agent assigned to this portfolio should improve one readiness signal. A good assignment is: "Build the next Steam Playtest readiness snapshot and identify the highest-risk missing owner, artifact, or cancel condition."
