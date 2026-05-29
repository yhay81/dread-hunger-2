# Performance Budget

Sources: `docs/qa-plan.md`, `docs/quality-gates.json`, `docs/commercial-quality-target.md`,
`docs/production-plan.md`, `docs/technical-architecture.md`.

> **Server-side rows are blocked** until a server-capable (source-built) UE 5.7 is available.
> Launcher UE cannot build `FrostwakeServer.exe`. Evidence: `docs/cycles/2026-05-26-cycle-78.md`
> and `docs/phase1-milestone-report.md`. The blocking fact is recorded in
> `docs/orchestration/DISPATCH.md` §2.

---

## Budget Table

| Dimension | Phase 1 target | Phase 2 gate | Measurement method |
| --- | --- | --- | --- |
| **p95 server tick time** (ms) | BLOCKED — measure first once `FrostwakeServer.exe` exists | ≤ 20 ms p95 over a full 8-player match | Run `FrostwakeServer.exe` with `stat unit` logging enabled; capture `Saved/ProfilingData/` frame-stat CSV; filter to the `GameThread` row; compute p95 across all ticks from match_started to match_ended. Command (once server target unblocked): `FrostwakeServer.exe /Game/Maps/L_IcebreakerWhitebox -log -StatNamedEvents -DumpStatCsvToFile` |
| **p95 client frame time** (ms) | TBD — measure first against Editor/Game smoke on minimum-spec hardware | ≤ 16.7 ms p95 (= 60 FPS floor on min spec, per `docs/commercial-quality-target.md`) | Launch Editor/Game with `stat unit` and `-dumpstatcsv`; run 8-player listen-server smoke; capture `Saved/ProfilingData/` CSV; filter to `Frame` row; compute p95 across a 10-minute representative window. Command: `cargo run -p frostwake-tools -- run-local-smoke --profile combined8 --extra-args "-StatNamedEvents -DumpStatCsvToFile"` (Editor target; Server-dedicated target blocked). |
| **p95 ping — LAN** (ms) | TBD — measure first during listen-server smoke | ≤ 30 ms p95 on localhost / local-LAN 8-player test | Read the `ping` field from player `client_connected` and periodic heartbeat events in the match JSONL log (once a ping-telemetry event is added); until then read UE `stat net` NetDriver RTT from client output log. Reference: `docs/qa-plan.md` playtest template field "p95 ping". |
| **p95 ping — WAN** (ms) | TBD — no WAN test infra in Phase 1 | ≤ 120 ms p95 in a geographically representative 8-player test | Steam Datagram Relay or direct connect; read RTT from UE `stat net` NetDriver during a real-network 8-player match. Gated behind Phase 2 dedicated server + Steam integration (GP-02 / GP-04). |
| **Peak server memory** (MB) | BLOCKED — requires `FrostwakeServer.exe` | ≤ 1 500 MB RSS at peak 8-player match load | Attach `perfmon` or `Get-Process | Select-Object WorkingSet` to the server process during an 8-player full-match run; record peak WorkingSet. Once server target unblocked, run as part of smoke baseline: `cargo run -p frostwake-tools -- run-local-smoke --profile combined8 --collect-memory`. |
| **Peak client memory** (MB) | TBD — measure first on listen-server host | ≤ 2 000 MB RSS on minimum-spec hardware | Attach `Get-Process UnrealEditor` (Editor target) or `Frostwake` (Game target) to the listen-server smoke run; record peak WorkingSet across a full match. |
| **Bandwidth — per-player upstream** (KB/s) | TBD — measure first | ≤ 10 KB/s per-player upstream average (8-player steady-state replication) | Enable UE `stat net` and capture `OutBytes/sec` per connection during an 8-player listen-server smoke; divide by player count. Confirmed blocked for dedicated-server path until Phase 2. |
| **Bandwidth — per-player downstream** (KB/s) | TBD — measure first | ≤ 40 KB/s per-player downstream average | Same as above; read `InBytes/sec`. |
| **Minimum hardware spec** | TBD — owner decision pending Phase 1 human playtest | CPU: ≥ 4-core 3.0 GHz; GPU: ≥ GTX 1060 6 GB / RX 580 8 GB; RAM: ≥ 8 GB; OS: Windows 10 64-bit | Run a full 8-player smoke on the minimum-spec machine; confirm p95 frame time ≤ 16.7 ms and no crashes. Spec is set by the owner after the first human playtest wave (`docs/qa-plan.md` §品質ゲート Phase 1). |

---

## Notes

### Current measurement gaps (Phase 1)

- No JSONL event currently emits server tick time, frame time, ping, memory, or bandwidth.
  These require UE `stat` capture or a custom periodic telemetry event (a future backlog item
  in `docs/orchestration/lanes/GP-07.state.md`).
- Server-side measurements (tick time, server memory, per-connection bandwidth) depend on
  `FrostwakeServer.exe` existing. That binary cannot be produced from the current Launcher UE
  distribution.
- Client/Editor measurements can begin as soon as a smoke run with `-DumpStatCsvToFile` is
  added to the smoke invocation.

### Phase 2 gate rationale

- **p95 server tick ≤ 20 ms**: matches the 50 Hz tick-rate floor common in server-authoritative
  multiplayer titles at 8-player load; leaves headroom before the server becomes the FPS
  bottleneck.
- **p95 frame time ≤ 16.7 ms**: direct derivation of the 60 FPS target stated in
  `docs/commercial-quality-target.md` ("target 60 FPS client performance on minimum spec").
- **LAN ping ≤ 30 ms / WAN ping ≤ 120 ms**: practical player-experience thresholds for a
  real-time cooperative game where interaction timing matters; WAN gate deferred to Phase 2
  when a dedicated server and external network test are possible.
- **Server memory ≤ 1 500 MB**: conservative ceiling for a Windows server process running
  8 players on whitebox geometry; to be re-evaluated after first server profiling pass.
- **Client memory ≤ 2 000 MB**: standard headroom on the targeted minimum-spec 8 GB machine
  (leaves ~6 GB for OS + other processes).
- **Bandwidth ≤ 10/40 KB/s**: rough starting targets for Unreal's conventional replication of
  8 compact player states; actual measurement may reveal these are conservative and they can
  be tightened after Phase 2 profiling.

### How to proceed once unblocked

1. Add `-DumpStatCsvToFile` to the Editor/Game smoke invocation and re-run
   `cargo run -p frostwake-tools -- run-local-smoke --profile combined8`.
2. Parse the resulting CSV in `crates/frostwake-tools/src/main.rs` (add a
   `perf-summary` sub-command) to extract p95 frame time and populate the TBD rows.
3. Once `FrostwakeServer.exe` is available, run the same smoke as a dedicated server and
   add server tick p95 and server memory to the committed evidence.
4. After the first human playtest wave (P1-024+), fill "Average FPS" and "p95 ping" from the
   playtest template fields (see `docs/qa-plan.md`).
5. When numbers are first collected, update this table and add the doc to
   `required_files` in `docs/quality-gates.json` (noted as a backlog item in
   `docs/orchestration/lanes/GP-07.state.md`).
