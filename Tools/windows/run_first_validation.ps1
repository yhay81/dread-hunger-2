param(
    [string]$UeRoot = $env:UE_ROOT,
    [switch]$SkipGenerate,
    [switch]$IncludeSmoke,
    [switch]$IncludeHeavySmoke
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\WindowsValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ($UeRoot -and $UeRoot.Length -gt 0) {
    $env:UE_ROOT = $UeRoot
}

function Invoke-Logged {
    param(
        [string]$Name,
        [string[]]$Command
    )

    $LogPath = Join-Path $OutDir "$Name.log"
    Write-Host ""
    Write-Host "=== $Name ==="
    Write-Host ($Command -join " ")

    if ($Command.Count -gt 1) {
        & $Command[0] @($Command[1..($Command.Count - 1)]) 2>&1 | Tee-Object -FilePath $LogPath
    } else {
        & $Command[0] 2>&1 | Tee-Object -FilePath $LogPath
    }
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -ne 0) {
        Write-Host "[FAIL] $Name exited with $ExitCode"
        Write-Host "Log: $LogPath"
        exit $ExitCode
    }

    Write-Host "[PASS] $Name"
}

function Add-UeRootArgs {
    param(
        [string[]]$Command
    )

    if ($UeRoot -and $UeRoot.Length -gt 0) {
        return [string[]]($Command + @("--ue-root", $UeRoot))
    }

    return $Command
}

Push-Location $RepoRoot
try {
    Invoke-Logged "check_prereqs" @("powershell", "-ExecutionPolicy", "Bypass", "-File", "Tools\windows\check_prereqs.ps1")

    $QualityCommand = @("py", "-3", "Tools\quality_gate.py", "--require-ue")
    Invoke-Logged "quality_gate" $QualityCommand

    $UnrealCommand = @("py", "-3", "Tools\unreal_gate.py", "--platform", "Win64", "--include-server")
    if ($SkipGenerate) {
        $UnrealCommand += "--skip-generate"
    }
    if ($UeRoot -and $UeRoot.Length -gt 0) {
        $UnrealCommand += @("--ue-root", $UeRoot)
    }
    Invoke-Logged "unreal_gate_win64" $UnrealCommand

    if ($IncludeSmoke) {
        Invoke-Logged "smoke_qa_bot" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_local_smoke.py", "--profile", "qa-bot", "--skip-build", "--null-rhi", "--platform", "Win64"))
        Invoke-Logged "smoke_match_timer" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_local_smoke.py", "--profile", "match-timer", "--skip-build", "--null-rhi", "--platform", "Win64"))
        Invoke-Logged "smoke_life_action" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_local_smoke.py", "--profile", "life-action", "--skip-build", "--null-rhi", "--platform", "Win64"))
        Invoke-Logged "smoke_combined5" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_local_smoke.py", "--profile", "combined5", "--skip-build", "--null-rhi", "--platform", "Win64"))
        Invoke-Logged "smoke_ready8" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_local_smoke.py", "--profile", "ready8", "--skip-build", "--null-rhi", "--platform", "Win64"))
        Invoke-Logged "smoke_suite_quick" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_smoke_suite.py", "--skip-build", "--null-rhi", "--platform", "Win64"))
    }

    if ($IncludeHeavySmoke) {
        Invoke-Logged "smoke_suite_heavy" (Add-UeRootArgs -Command @("py", "-3", "Tools\ue\run_smoke_suite.py", "--include-heavy", "--skip-build", "--null-rhi", "--platform", "Win64"))
    }

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Windows validation completed.",
        "Timestamp: $Stamp",
        "Output: $OutDir",
        "Next: copy key results into docs\windows-validation-template.md or a new cycle record."
    ) | Set-Content -Path $SummaryPath -Encoding UTF8

    Write-Host ""
    Write-Host "[PASS] Windows validation command set completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    Pop-Location
}
