# Local Client Runner Spec

Phase 1 starts with `cargo run -p frostwake-tools -- run-local-smoke`, which launches an uncooked UE `-game` listen host through `UnrealEditor` and optional localhost clients. The later bot runner should build on the same log and process-control conventions.

## Current Smoke Command

```bash
cargo run -p frostwake-tools -- run-local-smoke \
  --host-only \
  --skip-build \
  --null-rhi \
  --duration 45
```

For a localhost client smoke:

```bash
cargo run -p frostwake-tools -- run-local-smoke \
  --clients 1 \
  --skip-build \
  --smoke-interact \
  --duration 120 \
  --windowed
```

## Future Bot Command Shape

```bash
Tools/bot_client_runner.sh \
  --client ./Binaries/Mac/Frostwake \
  --map L_IcebreakerWhitebox \
  --clients 8 \
  --bots 8 \
  --seed 12345 \
  --time-limit 1800 \
  --log-dir Saved/Automation/Run_001 \
  --windowed
```

## Exit Codes

| Code | Meaning |
| --- | --- |
| 0 | Completed |
| 10 | Listen host failed to start |
| 11 | Client failed to start |
| 12 | Client connection timeout |
| 13 | Match did not start |
| 14 | Match did not end |
| 20 | Crash detected |
| 30 | Log validation failed |

## Required Outputs

- `listen_host.log`
- `client_01.log` through `client_08.log`
- `events.jsonl`
- `summary.json`
- `crashes/` if crash dumps exist

## Phase 1 Constraint

This runner targets a packaged listen server flow. Dedicated server binaries are a Phase 2 task.
