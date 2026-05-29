# Frostwake Orchestration — Start Here

This directory turns Frostwake's work into **10 parallel lanes** that autonomous agents can
advance one small, verifiable step at a time, **even with no memory of previous runs**.

## If you are an agent picking up this project

Read these, in order, then act:

1. **`DISPATCH.md`** — the launch instruction. Tells you how to pick a lane and do exactly
   one safe, verifiable step. **This is the brain. Follow it literally.**
2. **`STATUS.md`** — one-screen snapshot of all 10 lanes (status, milestone, next action).
3. **`PLAN.md`** — the lane map, milestones, priority order, and what is parallel vs gated.
4. **`lanes/GP-0X.state.md`** — the chosen lane's mutable state. Its **"Next Action"** block
   is the concrete thing to do, with how to verify it and what you may/may not change.

## The recovery contract

Nothing important lives only in an agent's head. After every step, the agent updates the
lane state file and `STATUS.md`, and (for real work) writes a `docs/cycles/` record. So the
answer to *"I lost context — what do I do next?"* is always:
**read `STATUS.md` → open the top-priority eligible lane → do its "Next Action".**

## Layout

```
docs/orchestration/
  README.md            ← you are here
  DISPATCH.md          ← launch instruction / algorithm (cold-start safe)
  PLAN.md              ← 10 lanes, M1 milestones, priority, sequencing
  STATUS.md            ← global snapshot (updated every iteration)
  lanes/
    _TEMPLATE.state.md ← shape of a lane state file
    GP-01.state.md … GP-10.state.md
```

Stable lane charters (north-star, boundaries, evidence standard) stay in
`docs/subprojects/sp-0X-*.md` — read them for *why* and *what not to do*; read the state
files for *what to do next*.

## The routine

`/frostwake-loop` (`.claude/commands/frostwake-loop.md`) runs one DISPATCH iteration. It is
driven every 10 minutes by `/loop 10m /frostwake-loop`. A persistent, cross-session version
can be installed with `Tools/automation/install_frostwake_loop_task.ps1` (Windows Task
Scheduler) — see `Tools/automation/README.md`.

## Conventions agents must keep

- Lock with `Saved/Automation/loop.lock`; isolate `CARGO_TARGET_DIR=target/loop-gpNN`.
- Verify with the smallest `frostwake-tools` gate for the surface touched.
- Commit only the paths you changed, to `main`, with the Claude co-author trailer.
- Never commit secrets/PII; keep match authority in Unreal C++; no unverified public claims.
