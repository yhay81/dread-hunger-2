#requires -Version 7.0
<#
.SYNOPSIS
    Remove the persistent Frostwake loop Scheduled Task.
.EXAMPLE
    pwsh -File Tools/automation/uninstall_frostwake_loop_task.ps1
#>
param([string]$TaskName = 'FrostwakeLoop')

$ErrorActionPreference = 'Stop'
$existing = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if ($existing) {
    Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
    Write-Output "Removed Scheduled Task '$TaskName'."
} else {
    Write-Output "No Scheduled Task named '$TaskName' found."
}
