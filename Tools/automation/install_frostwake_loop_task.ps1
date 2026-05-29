#requires -Version 7.0
<#
.SYNOPSIS
    Install the persistent Frostwake loop as a Windows Scheduled Task (every 10 minutes).
.DESCRIPTION
    Registers a Scheduled Task named "FrostwakeLoop" that runs
    frostwake_loop_runner.ps1 every 10 minutes for the current user, so the
    autonomous loop keeps progressing across sessions/reboots.

    This is the OPT-IN persistence path. Running it ENABLES unattended runs that
    spend API tokens and run cargo/git on this machine. Review
    Tools/automation/README.md first. Remove with uninstall_frostwake_loop_task.ps1.
.PARAMETER IntervalMinutes
    Repeat interval. Default 10.
.PARAMETER PermissionMode
    Passed through to the runner. Default 'acceptEdits'. For fully unattended
    cargo/git, use a scoped .claude/settings.json allowlist or 'bypassPermissions'.
.EXAMPLE
    pwsh -File Tools/automation/install_frostwake_loop_task.ps1
#>
param(
    [int]$IntervalMinutes = 10,
    [ValidateSet('default', 'acceptEdits', 'plan', 'bypassPermissions')]
    [string]$PermissionMode = 'acceptEdits',
    [string]$TaskName = 'FrostwakeLoop'
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$runner = Join-Path $RepoRoot 'Tools\automation\frostwake_loop_runner.ps1'
$pwsh = (Get-Command pwsh -ErrorAction Stop).Source

if (-not (Test-Path $runner)) { throw "Runner not found: $runner" }

$action = New-ScheduledTaskAction -Execute $pwsh `
    -Argument "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File `"$runner`" -PermissionMode $PermissionMode" `
    -WorkingDirectory $RepoRoot

# Repeat forever, starting ~1 minute from now.
$trigger = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1) `
    -RepetitionInterval (New-TimeSpan -Minutes $IntervalMinutes)

$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
    -StartWhenAvailable -MultipleInstances IgnoreNew -ExecutionTimeLimit (New-TimeSpan -Minutes 30)

$principal = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" -LogonType Interactive -RunLevel Limited

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger `
    -Settings $settings -Principal $principal -Force | Out-Null

Write-Output "Registered Scheduled Task '$TaskName': every $IntervalMinutes min, runner=$runner, PermissionMode=$PermissionMode."
Write-Output "Disable later with: pwsh -File Tools/automation/uninstall_frostwake_loop_task.ps1"
