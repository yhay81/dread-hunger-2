# GP-07: Evidence, QA, And Performance

## North-Star Goal

Every product claim, phase gate, and regression decision has reproducible evidence linked to build, map, profile, player count, logs, and summary output.

## Why This Exists

Multiplayer projects decay when passes are remembered instead of recorded. This portfolio continuously improves observability, test repeatability, and performance accountability.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Gate reproducibility | Another agent can rerun or understand a pass/fail from committed evidence |
| Telemetry completeness | Logs reconstruct match start/end, roles, objectives, sabotage, downs/deaths, disconnects, and errors |
| Failure classification | Results distinguish pass, fail, blocked, flaky, and inconclusive |
| Performance visibility | p95 server tick, frame time, ping, memory, bandwidth, and minimum spec are measured |
| Bug quality | Repros include build, map, seed/profile, player count, expected/actual, and log path |
| Privacy safety | Raw personal data and secrets are excluded or redacted |

## Improvement Questions

- Which phase gate currently has the weakest evidence?
- What failure cannot be reproduced from the current logs?
- Which telemetry field would most reduce manual investigation?
- Which smoke profile is slow, flaky, or no longer representative?
- What performance budget is missing before public scale?

## Evidence Standard

Use the smallest useful validation command for the changed surface. Record command, platform, build, map, profile/seed, player count, log path, result, and interpretation.

Primary docs:

- `docs/qa-plan.md`
- `docs/iteration-loop.md`
- `docs/quality-gates.json`
- `docs/phase1-milestone-report.md`
- `docs/windows-validation-template.md`

## Boundaries

- Do not decide feature direction alone from QA data.
- Do not accept unrecorded terminal output as lasting evidence.
- Do not commit private logs, raw recordings, personal data, or secrets.
- Do not let a broad suite replace a smaller focused repro when diagnosing a bug.

## Review Cadence

Review this portfolio after every runtime change, before phase gates, and before external tester waves. Evidence quality should improve continuously, even when no feature changes.

## Agent Charter

An agent assigned to this portfolio should improve one evidence signal. A good assignment is: "Audit the latest smoke suite and playtest flow, then add or document the missing telemetry field that most improves reproducibility."
