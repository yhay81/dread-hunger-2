# Frostwake Loop Automation

Two ways to drive the 10-minute autonomous loop (`/frostwake-loop`, which runs one
`docs/orchestration/DISPATCH.md` iteration):

## 1. In-session (active by default)

From an open Claude Code session in this repo:

```
/loop 10m /frostwake-loop
```

Runs every ~10 minutes while the session is open. Stop with `/loop stop` (or end the
session). No unattended cost. This is the recommended day-to-day mode.

## 2. Persistent across sessions (opt-in — spends tokens unattended)

Installs a Windows Scheduled Task that runs the loop every 10 minutes even when no Claude
Code session is open.

```powershell
# enable
pwsh -File Tools/automation/install_frostwake_loop_task.ps1
# disable
pwsh -File Tools/automation/uninstall_frostwake_loop_task.ps1
```

**Before enabling, understand:** each run invokes `claude -p "/frostwake-loop"` headless,
which spends API tokens and runs cargo/git on this machine unattended, committing to `main`.

### Making unattended runs actually autonomous

`frostwake_loop_runner.ps1` defaults to `-PermissionMode acceptEdits`, which auto-accepts
file edits but still prompts for shell commands — so an unattended run will stall on the
first `cargo`/`git`. To let it run end-to-end, pick one:

- **Scoped (preferred):** add an allowlist to `.claude/settings.json` permitting the loop's
  commands (`cargo run -p frostwake-tools -- *`, `cargo test *`, `git add *`, `git commit *`,
  `git status`, `git log`) and keep `acceptEdits`.
- **Broad:** install with `-PermissionMode bypassPermissions` (auto-allows everything;
  use deliberately).

### Safety built in

- Both the runner and `DISPATCH.md` honor `Saved/Automation/loop.lock` (skip if a run <25
  min old is active), so the Scheduled Task and an in-session `/loop` won't collide.
- Each run isolates `CARGO_TARGET_DIR=target/loop-gpNN` to avoid build-lock contention.
- Logs append to `Saved/Automation/loop-<date>.log` (gitignored).

## Files

| File | Purpose |
| --- | --- |
| `frostwake_loop_runner.ps1` | Locates `claude`, runs one headless `/frostwake-loop` iteration, logs it |
| `install_frostwake_loop_task.ps1` | Registers the every-10-min Scheduled Task (opt-in) |
| `uninstall_frostwake_loop_task.ps1` | Removes the Scheduled Task |
