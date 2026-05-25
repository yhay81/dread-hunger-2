# Tools

Utility area for:

- multi-client launch scripts
- log collection and crash bundling
- bot command profiles
- balance simulation
- asset import checks
- local reference inventory without copying third-party files
- Steam build/upload notes

Implemented:

- `quality_gate.py`: runs static repository gates for each improvement cycle.
- `backlog_to_issues.py`: exports phase backlog rows to issue-import CSV and Markdown.
- `github_issue_sync.py`: dry-runs or creates GitHub issues from the generated issue-import CSV. It defaults to Phase 1; pass `--csv docs/issue-import/phase2-issues.csv` for Phase 2.
- `unreal_gate.py`: runs UE project generation and local Editor/Game build gates.
- `new_cycle.py`: creates the next cycle record under `docs/cycles/`.
- `log_summary.py`: summarizes JSONL match telemetry for Phase 1 QA.
- `ue/run_local_smoke.py`: launches a local UE listen-server smoke test and optional localhost clients.
- `ue/run_smoke_suite.py`: runs named smoke profiles; pass `--platform Win64` on Windows.
- `playtest_run_scaffold.py`: creates ignored P1-024 local run folders with Windows PowerShell launch scripts and legacy bash scripts.
- `playtest_report_upload.py`: builds or optionally uploads an anonymized playtest report payload to the local backend.
- `windows/`: Windows-only first-run, dedicated-server, Phase 2 entry, Steam dev config, and Steam Lobby validation helpers.
- `reference_inventory.sh`: creates ignored metadata inventories for local research copies.
- `ops/`: safe local ops contracts, redaction helpers, banlist helpers, and config schemas.

Tests:

- `python3 -m unittest discover -s tests`: fixture-based checks for the P1-024 telemetry and playtest evidence pipeline. This is also run by `quality_gate.py`.
