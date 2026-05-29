# Dread Hunger — Dedicated Server Reference Observations

> **Scope & IP boundary.** This is a *competitive reference* built only from **runtime
> observation** (booting the owned game's dedicated server and reading its own logs) and
> **file metadata** (paths/sizes). No packaged content was decrypted, no binary was
> decompiled, and **no protected expression was copied**. Specific class names, map names,
> item names, packet formats, and config files are deliberately omitted. Frostwake's
> implementation decisions are our own — this doc informs *what questions to answer*, not
> *what to copy*. See [reference-policy.md](reference-policy.md), [ip-risk.md](ip-risk.md),
> [ip-boundary.md](ip-boundary.md).

## Provenance (evidence)

- Date: 2026-05-29 JST.
- Subject: owned Steam copy, App `1418630`, `WindowsServer` dedicated-server build, buildid `13179402`.
- Method:
  - Booted the Shipping dedicated-server exe with `-log -stdout -unattended -nopause -port=7777`
    for ~22 s, captured stdout + the engine log, then stopped the process.
  - `cargo run -p frostwake-tools -- reference-inventory --manifest references/private/external-paths.windows.json`
    for file metadata.
  - Read the Linux server launch wrapper (Epic-generated `.sh`).
  - Booted the Shipping client exe windowed for ~28 s and captured its boot log (it calls
    `RequestExit` at boot when no Steam client is running, so only online/UI init was observed).
- Local-only artifacts (gitignored, never committed): `references/private/dh-server-obs/` and
  `references/private/dh-client-obs/` (logs), `references/inventory/inventory_20260529_133254.tsv`
  (+ summary).
- Engine identified in log: **UE 4.26.1** (Shipping, compiled 2023-12-20).

## Observed architecture (abstracted)

| Dimension | Observation (competitor, for context) | What it tells us |
| --- | --- | --- |
| Engine | UE 4.26.1, Shipping, PhysX, binned2 allocator | Mature UE4-era netcode; we target UE 5.7 |
| Launch model | Stock Epic-generated server wrapper exec'ing the Shipping server; map + options passed as args; no custom bootstrapper | A vanilla UE dedicated-server launch model is sufficient; operators pass `Map?listen -port -log` |
| Net transport | Stock `IpNetDriver`, UDP, default port 7777, ~128 KB socket buffers | No exotic transport; standard UE replication over UDP |
| Replication scaling | No Replication Graph (stock property replication) | 8-player shipped *without* RepGraph; we can start stock and add RepGraph only if profiling demands it |
| Server tick | 60 Hz (observed) | A 60 Hz authoritative tick is viable at this player count; our value remains our own choice |
| Online subsystem | **`EOSPlus`** composite — EOS layered over Steam as the platform OSS (both binaries ship the EOS SDK); client boot **hard-requires** a working OSS | They commit to an EOS+Steam composite and mandatory-online. We decide our own Steam-first stance (GP-04), whether EOS/EOSPlus earns its keep, and whether to keep an offline/dev path not gated behind online |
| Packet security | PacketHandler with an AES-GCM component referenced | They budget for encrypted game traffic; we should decide our own packet-encryption posture |
| Match flow | Native C++ GameMode base + Blueprint subclass; standard UE MatchState progression (entering-map → waiting-to-start → in-progress); GameMode owns spawning the central ship actor at a spawn point | Hybrid C++/BP authority split is the norm — keep authoritative logic in C++ base, tune in BP |
| Content protection | Paks **encrypted and signed** (`.sig`); server **PDB shipped** (debug symbols) | Their shipping hygiene; for us, keep our own provenance/asset ledger and strip PDBs from ship builds |
| Localization | UE String Tables | Consistent with the GP-09 localization approach |
| Build split | Device Profiles layer server-vs-client CVars; server disables texture streaming | Use Device Profiles for our own server/client divergence |

## Implications for GP-02 (our original decisions)

- The intended Frostwake path — an **Unreal-authoritative dedicated server** — is the same
  *shape* DH ships, which de-risks the architecture (proven, vanilla-UE). It does **not**
  unblock GP-02's actual blocker: we still need a **server-capable UE 5.7** to *build our own*
  server target (see [GP-02.state.md](orchestration/lanes/GP-02.state.md),
  [server-capable-ue-unblock-runbook.md](server-capable-ue-unblock-runbook.md)).
- Start with **stock `IpNetDriver` + standard replication at a 60 Hz tick**; defer Replication
  Graph until profiling 8 players proves a need.
- **Steam-first** for sessions/browser (GP-04). Treat EOS as an *open question* to decide and
  record, not a default to mirror.
- Decide our **packet-encryption** posture deliberately (their AES-GCM presence is context, not a spec).
- Keep the **C++-authoritative GameMode, Blueprint-tunable** split.

## Client-side notes (boot observation)

- Online subsystem instance created is **`EOSPlus`** (EOS composited over Steam); client boot
  **hard-requires** a working OSS — with no Steam client running it logs the failure and calls
  `RequestExit` rather than falling back to an offline path.
- **GeForce NOW SDK** is initialized at startup (cloud-gaming integration).
- Standard **Slate** UI on **D3D11**, with CEF (embedded Chromium) web views available.
- Same engine build as the server (UE 4.26.1, compiled 2023-12-20).

## Deliberately NOT done (boundary)

- **No decryption / no circumvention.** Paks are AES-encrypted and signed; the AES key was not
  sought or used (`UnrealPak -List` stops at the key check).
- **No decompilation for content**, despite a shipped server PDB. The outputs of reverse
  engineering (proprietary server behavior, packet formats, config values, names) are on
  reference-policy.md's *Not Allowed* list and would contaminate clean-room implementation.
- **No protected expression** (class / map / item names, UI, config values) copied into this
  doc or into any Frostwake source.

## Open questions runtime observation can't answer (need our own design)

- Replication relevancy/priority tuning for 8 players → our own profiling.
- Anti-cheat / EOS rationale → our own decision (GP-04).
- Reconnection / seamless-travel behavior → observe later in a populated session, or design fresh.
