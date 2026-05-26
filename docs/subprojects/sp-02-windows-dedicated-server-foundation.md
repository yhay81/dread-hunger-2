# GP-02: Network And Hosting Reliability

## North-Star Goal

Frostwake's 8-player sessions are stable, reproducible, and hostable through the intended Unreal authoritative server path, with enough logs to diagnose crashes, disconnects, desyncs, and join failures.

## Why This Exists

Social deduction only works when players trust the session. This portfolio continuously improves the technical foundation for hosting, joining, starting, reconnecting, and ending matches.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Dedicated-server viability | Win64 server target builds, boots, accepts clients, and starts ready-lobby matches |
| Completion stability | More matches end by rules instead of crash, timeout, or manual cleanup |
| Join reliability | Clients can join consistently with clear rejection reasons |
| Repro quality | Every failure includes build, map, profile/seed, player count, logs, and command |
| Authority safety | Match-critical state remains server authoritative |
| Offline safety | Null/LAN/listen validation remains available while Steam layers are added |

## Improvement Questions

- What is the current weakest point in the host/join/start/end chain?
- Does a failure produce enough evidence for another engineer to reproduce it?
- Are dedicated-server and listen-server paths drifting in behavior?
- Are join rejections clear before travel?
- Is any client-trusted state leaking into authoritative gameplay?

## Evidence Standard

Record validation results as dated documentation, not only terminal output. Evidence should include command, platform, UE path, build target, log path, JSONL event path, player count, and pass/fail/blocker status.

Relevant sources:

- `docs/windows-handoff.md`
- `docs/windows-validation-template.md`
- `docs/windows-phase2-entry-template.md`
- `docs/windows-dedicated-server-runbook.md`
- `Tools/windows/README.md`

## Boundaries

- Do not start Steam server browser or SDR implementation before dedicated-server evidence exists.
- Do not move match authority to the Rust backend.
- Do not remove listen-server smoke coverage while dedicated-server work is incomplete.

## Review Cadence

Review this portfolio at every phase gate and after every server-target, join, or crash blocker. The question is always whether the next public-facing system has a reliable authoritative path underneath it.

## Agent Charter

An agent assigned to this portfolio should improve one weak link in session reliability. A good assignment is: "Using the latest Windows validation note, make the next server boot or client join failure reproducible and propose the smallest corrective change."
