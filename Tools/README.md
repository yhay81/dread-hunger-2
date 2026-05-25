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
- `unreal_gate.py`: runs UE project generation and local Editor/Game build gates.
- `new_cycle.py`: creates the next cycle record under `docs/cycles/`.
- `log_summary.py`: summarizes JSONL match telemetry for Phase 1 QA.
- `ue/run_local_smoke.py`: launches a local UE listen-server smoke test and optional localhost clients.
- `reference_inventory.sh`: creates ignored metadata inventories for local research copies.
- `ops/`: safe local ops contracts, redaction helpers, banlist helpers, and config schemas.
