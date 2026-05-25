# Playtests

This directory contains committed playtest packets and anonymized summaries.

Do not commit raw logs, crash dumps, recordings, SteamIDs, tester names, voice transcripts, or moderation reports. Raw evidence belongs under ignored local paths such as `Saved/Playtests/...` and should be summarized before entering the repository.

Current packets:

- [P1-024 Human Test 1](p1-024-human-test-1.md)
- [P1-024 Observer Sheet](p1-024-observer-sheet.md)
- [P1-024 Post-Round Survey](p1-024-post-round-survey.md)
- [P1-024 Summary Template](p1-024-summary-template.md)
- [P1-024 Dry-Run Summary Example](p1-024-dry-run-summary-example.md)

The dry-run example is generated from smoke-test telemetry. It is only a format and redaction check, not human playtest evidence and not a P1-024 pass, partial, or fail result.

Before a human run on Windows, generate the local ignored run folder:

```powershell
py -3 Tools\playtest_run_scaffold.py --run-number 1 --target-players 6
```

This writes PowerShell host/client scripts, legacy bash scripts, notes, and local evidence directories under `Saved/Playtests/P1-024/run-01/`. That path is ignored by git.

After a run, generate a local skeleton from telemetry summary JSON:

```powershell
py -3 Tools\log_summary.py Saved\Playtests\P1-024\run-01\events.jsonl --out Saved\Playtests\P1-024\run-01\summary.json
py -3 Tools\playtest_summary.py Saved\Playtests\P1-024\run-01\summary.json --out docs\playtests\p1-024-run-01-summary.md
py -3 Tools\playtest_preflight.py Saved\Playtests\P1-024\run-01\summary.json --markdown docs\playtests\p1-024-run-01-summary.md
```

Review and anonymize the generated file before committing it.

Optional backend dry-run after `summary.json` exists. The tool also accepts raw JSONL events and summarizes them locally before building the payload:

```powershell
py -3 Tools\playtest_report_upload.py Saved\Playtests\P1-024\run-01\summary.json
```

Only send to a local backend after reviewing the payload:

```powershell
py -3 Tools\playtest_report_upload.py Saved\Playtests\P1-024\run-01\summary.json --send
```
