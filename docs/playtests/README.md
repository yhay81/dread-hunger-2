# Playtests

This directory contains committed playtest packets and anonymized summaries.

Do not commit raw logs, crash dumps, recordings, SteamIDs, tester names, voice transcripts, or moderation reports. Raw evidence belongs under ignored local paths such as `Saved/Playtests/...` and should be summarized before entering the repository.

Before each private or public wave starts, complete:

- `docs/steam-playtest-checklist.md` (Playtest Wave Gate)
- `docs/playtest-data-and-moderation.md` (incident intake and escalation)

Record the wave decision in that checklist and keep the decision summary link in the committed playtest packet.

Current packets:

- [P1-024 Human Test 1](p1-024-human-test-1.md)
- [P1-024 Observer Sheet](p1-024-observer-sheet.md)
- [P1-024 Post-Round Survey](p1-024-post-round-survey.md)
- [P1-024 Summary Template](p1-024-summary-template.md)
- [P1-024 Dry-Run Summary Example](p1-024-dry-run-summary-example.md)

The dry-run example is generated from smoke-test telemetry. It is only a format and redaction check, not human playtest evidence and not a P1-024 pass, partial, or fail result.

Before a human run on Windows, generate the local ignored run folder:

```powershell
cargo run -p frostwake-tools -- playtest-run-scaffold --run-number 1 --target-players 8
```

This writes PowerShell host/client scripts, POSIX helper scripts, notes, and local evidence directories under `Saved/Playtests/P1-024/run-01/`. That path is ignored by git.

After a run, generate a local skeleton from telemetry summary JSON:

```powershell
cargo run -p frostwake-tools -- log-summary Saved\Playtests\P1-024\run-01\events.jsonl --out Saved\Playtests\P1-024\run-01\summary.json
cargo run -p frostwake-tools -- playtest-summary Saved\Playtests\P1-024\run-01\summary.json --out docs\playtests\p1-024-run-01-summary.md
cargo run -p frostwake-tools -- playtest-preflight Saved\Playtests\P1-024\run-01\summary.json --markdown docs\playtests\p1-024-run-01-summary.md
```

Review and anonymize the generated file before committing it. The preflight checks that human summaries include GP-01 playability evidence for objective comprehension, confusion cost, a memorable social incident, repeat-play intent, and keep/cut/change decisions. It also checks GP-09 comprehension/accessibility evidence for objective comprehension, next-step clarity, failure-state clarity, UI/control confusion, accessibility blockers, and text/term issues.

Optional backend dry-run after `summary.json` exists. The tool also accepts raw JSONL events and summarizes them locally before building the payload:

```powershell
cargo run -p frostwake-tools -- playtest-report-upload Saved\Playtests\P1-024\run-01\summary.json
```

Only send to a local backend after reviewing the payload:

```powershell
cargo run -p frostwake-tools -- playtest-report-upload Saved\Playtests\P1-024\run-01\summary.json --send
```
