# Frostwake Autonomous Dispatch — Launch Instruction

You are **one iteration** of the Frostwake build loop. You may have **no memory** of
previous iterations. Everything you need is in the repository. Read this file, then do
**exactly one** small, verifiable step toward a lane milestone, record it, and stop.

This file is the single source of truth for "what should I do next?". The 10-minute
routine (`/frostwake-loop`) and any manually launched agent both start here.

---

## 0. Orient (always do this first, every run)

Read, in order:

1. `docs/orchestration/STATUS.md` — global snapshot of all 10 lanes (the fast picture).
2. `docs/orchestration/PLAN.md` — lane map, milestones, priority order, sequencing.
3. The most recent file in `docs/cycles/` (highest cycle number) — what the last
   substantive step did and what it said to do next.
4. `git status --short` and `git log --oneline -5` — real working-tree / history state.

Then pick the lane to advance (Section 2) and open its state file
`docs/orchestration/lanes/GP-0X.state.md`. Its **"Next Action"** block tells you the
concrete next step, how to verify it, what you may change, and what you must not do.

If `STATUS.md` or a lane state file is missing or stale, your job for this iteration is to
**rebuild it** from the lane charter (`docs/subprojects/sp-0X-*.md`) + recent evidence,
then stop. A correct map is worth more than a rushed step.

---

## 1. Hard rules (never violate)

- **One small step per iteration.** Finish with a diff, a doc update, a gate result, or a
  recorded blocker. Do not start multi-hour work. If a step is large, split it and do the
  first slice, then write the remainder into the lane's backlog.
- **Concurrency lock.** At start, check `Saved/Automation/loop.lock`.
  - If it exists and its timestamp is **younger than 25 minutes**, another agent (or the
    user) is working — **abort this iteration immediately** without changes.
  - Otherwise write `Saved/Automation/loop.lock` with the current UTC timestamp + a short
    tag, do your work, and **delete it before you finish** (also delete on any early exit).
  - `Saved/` is gitignored; the lock is never committed.
- **Isolated Cargo target.** Any `cargo` command MUST set a unique target dir, e.g.
  `CARGO_TARGET_DIR=target/loop-gpNN`. The default `target/` may be locked by the user's
  own build. (PowerShell: `$env:CARGO_TARGET_DIR='target/loop-gp06'; cargo ...`)
- **Respect the lane charter.** Each lane's `docs/subprojects/sp-0X-*.md` lists Boundaries
  and an Evidence Standard. Honor them. Also honor `docs/agent-operating-model.md`
  (authority stays server-side, no unverified public claims, no secrets/PII committed).
- **Evidence or it didn't happen.** Record the exact command, result, and artifact path.
  Do not write "pass" without a reproducible gate.
- **Git scope.** When committing, stage **only the paths you changed this iteration**
  (`git add <path> ...`). Never `git add -A` / `git add .` — there may be unrelated
  work-in-progress in the tree that is not yours to commit.
- **Do not fake progress.** If the only honest outcome is "still blocked", update the
  blocker evidence and say so. Keeping the map accurate is real progress.

---

## 2. Choose the lane (deterministic)

1. **Health first (cheap — do NOT re-run the full gate every iteration).** Read the gate
   result recorded in the latest `docs/cycles/` file. If it was **red**, fixing it is the
   only valid step this iteration; stop after green. If it was green, trust it and move on —
   the full `cargo run -p frostwake-tools -- quality-gate` takes minutes and must not eat
   every 10-minute slot. You only run the gate yourself when **your own change this
   iteration touches a gated surface** (Rust, schemas, PowerShell, asset ledger, Unreal
   metadata) — then it doubles as your verify step (§4).
2. Otherwise walk the **priority order in `PLAN.md`** top to bottom and pick the first lane
   whose state is **not `BLOCKED`** and whose "Next Action" is not yet done.
3. **Anti-starvation tie-break.** Among lanes of equal priority that are eligible, pick the
   one with the **oldest `Updated:` date** in its state file.
4. A `BLOCKED` lane is still worth a visit **only** to refresh its blocker evidence or its
   "what unblocks this" note if that evidence is stale (older than ~7 days) — then stop.

The current blocking fact (verify it each run, do not assume): the only local engine is a
**Launcher UE_5.7** that cannot build Server targets, so `AbyssLockServer.exe` does not
exist. This hard-blocks the **implementation** steps of GP-02 and GP-04 (and the live
human run of GP-01) until `UE_ROOT` points at a server-capable / source-built UE. Their
**design, config-safety, and blocker-evidence** steps are NOT blocked.

---

## 3. Do the step

- Make the **smallest** change that moves the chosen lane's milestone signal.
- Stay inside the files listed in the lane's "Allowed to change".
- If the step needs a tool you cannot run here (e.g. a server-target build), do the
  reachable substitute (refresh the blocker manifest, tighten the runbook that will unblock
  it, add the missing telemetry/doc) and record why the gated step could not run.

---

## 4. Verify

Run the smallest gate that proves the step, choosing from:

| Surface changed | Verify with |
| --- | --- |
| Rust tools / backend | `cargo test --workspace` (isolated target) or the focused test |
| Repo hygiene / docs / schemas | `cargo run -p frostwake-tools -- quality-gate` |
| Steam lobby metadata / join | `lobby-metadata-check` / `lobby-join-decision` |
| Asset ledger | `asset-ledger-check` |
| Match log / smoke | `log-summary`, or `run-local-smoke` (Editor/Game only; Server is blocked) |
| Playtest summary | `playtest-preflight` |
| Secrets/PII risk | `secret-scan` |

Record command + result (pass / fail / blocked / not-run-and-why). Use the **smallest** gate
that proves *your* change — do not run the full `quality-gate` unless you changed a gated
surface (Rust, schemas, PowerShell, asset ledger, Unreal metadata). A docs-only change is
verified by a file check, not a multi-minute build.

---

## 5. Record (this is mandatory — it is how the next cold agent continues)

1. **Update the lane state file** `docs/orchestration/lanes/GP-0X.state.md`:
   - bump `Updated:` to today's date,
   - move completed items, write the **new** single "Next Action" with its verify gate,
   - update Progress / Blockers / Latest Evidence.
2. **Update `docs/orchestration/STATUS.md`** — the lane's row (status, milestone progress,
   one-line next action, updated date) and the "Last loop iteration" footer.
3. **If the step is a meaningful cycle of work** (not just a state refresh), create a cycle
   record so history stays continuous:
   `cargo run -p frostwake-tools -- new-cycle --phase GP-0X --owner loop --goal "<goal>" --gate "<measurable gate>"`
   then fill the generated `docs/cycles/<date>-cycle-<n>.md` (scope, files, verification,
   evidence, decisions, next cycle).

---

## 6. Commit (policy: main, auto)

```
git add <only the paths you changed>
git commit -m "loop: GP-0X <short summary> (cycle <n>)"
```

Commit message ends with the co-author trailer:

```
Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
```

Do not push unless the user has asked. Do not amend prior commits.

---

## 7. Finish

Delete `Saved/Automation/loop.lock`. Output a 3-line report: **done / next / blocked**.
Stop. The routine will fire again in ~10 minutes and re-read this file from the top.

---

## Abort conditions (stop and leave a note instead of forcing progress)

- The lock is held by a fresh run (Section 1).
- `quality-gate` is red for a reason you cannot safely fix in one small step → record the
  failing rows in GP-07 state and stop.
- A step would require importing third-party/protected assets, moving match authority out
  of UE C++, committing secrets/PII, or making an unverified public claim → stop, record
  the conflict per `docs/agent-operating-model.md`.
- You cannot determine a safe next action → rebuild the relevant state file and stop.
