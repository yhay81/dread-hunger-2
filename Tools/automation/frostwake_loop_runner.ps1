#requires -Version 7.0
<#
.SYNOPSIS
    Run one headless iteration of the Frostwake autonomous build loop.
.DESCRIPTION
    Locates the `claude` CLI, changes to the repository root, and invokes
    `claude -p "/frostwake-loop"` so one DISPATCH iteration runs unattended.
    Intended to be called by Windows Task Scheduler every 10 minutes
    (see install_frostwake_loop_task.ps1), or manually for a one-off run.

    Output is appended to Saved/Automation/loop-<date>.log (gitignored).
.PARAMETER PermissionMode
    Claude Code permission mode for the headless run. Default 'acceptEdits'
    auto-accepts file edits but still prompts for shell commands. For a fully
    unattended loop that runs cargo/git, either:
      - add an allowlist to .claude/settings.json (preferred, scoped), or
      - pass -PermissionMode bypassPermissions (broad; use deliberately).
.EXAMPLE
    pwsh -File Tools/automation/frostwake_loop_runner.ps1
.EXAMPLE
    pwsh -File Tools/automation/frostwake_loop_runner.ps1 -PermissionMode bypassPermissions
#>
param(
    [ValidateSet('default', 'acceptEdits', 'plan', 'bypassPermissions')]
    [string]$PermissionMode = 'acceptEdits'
)

$ErrorActionPreference = 'Stop'

# Repo root = two levels up from this script (Tools/automation/ -> repo root).
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $RepoRoot

# Resolve the claude CLI: PATH first, then common Windows install locations.
function Resolve-ClaudeCli {
    $cmd = Get-Command claude -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        (Join-Path $env:USERPROFILE '.local\bin\claude.exe'),
        (Join-Path $env:USERPROFILE '.local\bin\claude.cmd'),
        (Join-Path $env:APPDATA 'npm\claude.cmd'),
        (Join-Path $env:APPDATA 'npm\claude.ps1'),
        (Join-Path $env:LOCALAPPDATA 'Programs\claude\claude.exe')
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    return $null
}

$logDir = Join-Path $RepoRoot 'Saved\Automation'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Force -Path $logDir | Out-Null }
$stamp = Get-Date -Format 'yyyyMMdd'
$log = Join-Path $logDir "loop-$stamp.log"
$now = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'

$claude = Resolve-ClaudeCli
if (-not $claude) {
    "[$now] ERROR: could not locate the 'claude' CLI. Add it to PATH or edit Resolve-ClaudeCli." |
        Tee-Object -FilePath $log -Append
    exit 1
}

# Fresh-lock guard at the wrapper level too (DISPATCH.md enforces it inside the agent).
$lock = Join-Path $logDir 'loop.lock'
if (Test-Path $lock) {
    $age = (Get-Date) - (Get-Item $lock).LastWriteTime
    if ($age.TotalMinutes -lt 25) {
        "[$now] SKIP: fresh loop.lock ($([int]$age.TotalMinutes)m old); another run is active." |
            Tee-Object -FilePath $log -Append
        exit 0
    }
}

"[$now] START frostwake-loop via $claude (PermissionMode=$PermissionMode)" |
    Tee-Object -FilePath $log -Append

# Headless, non-interactive single iteration.
# Use a self-contained prompt (NOT the /frostwake-loop slash command) so the run never depends
# on project-command registration. DISPATCH.md holds the full algorithm.
$loopPrompt = 'Read docs/orchestration/DISPATCH.md and execute exactly one Frostwake dispatch iteration, following it literally: take the lock at Saved/Automation/loop.lock (abort if a fresh lock is held), pick the top-priority eligible lane per docs/orchestration/STATUS.md, do one small verifiable step, update the lane state file + docs/orchestration/STATUS.md, commit only the paths you changed to main with the trailer "Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>", release the lock, and end with a 3-line done/next/blocked report.'
& $claude -p $loopPrompt --permission-mode $PermissionMode *>&1 |
    Tee-Object -FilePath $log -Append

$end = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
"[$end] END frostwake-loop (exit=$LASTEXITCODE)" | Tee-Object -FilePath $log -Append
exit $LASTEXITCODE
