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
$CargoTargetDir = $env:CARGO_TARGET_DIR
if ([string]::IsNullOrWhiteSpace($CargoTargetDir)) {
    $CargoTargetDir = Join-Path $RepoRoot "target\windows-validation-$Stamp"
    $env:CARGO_TARGET_DIR = $CargoTargetDir
}

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

    $PreviousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        if ($Command.Count -gt 1) {
            & $Command[0] @($Command[1..($Command.Count - 1)]) 2>&1 | Tee-Object -FilePath $LogPath
        } else {
            & $Command[0] 2>&1 | Tee-Object -FilePath $LogPath
        }
    } finally {
        $ErrorActionPreference = $PreviousErrorActionPreference
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

function Find-Cargo {
    $LocalCargo = Join-Path $RepoRoot "Tools\install\rust\cargo\bin\cargo.exe"
    if (Test-Path $LocalCargo) {
        $env:CARGO_HOME = Join-Path $RepoRoot "Tools\install\rust\cargo"
        $env:RUSTUP_HOME = Join-Path $RepoRoot "Tools\install\rust\rustup"
        return $LocalCargo
    }

    $Cargo = Get-Command cargo -ErrorAction SilentlyContinue
    if ($Cargo) { return $Cargo.Source }
    throw "cargo not found. Run Tools\windows\check_prereqs.ps1 first."
}

Push-Location $RepoRoot
try {
    Invoke-Logged "check_prereqs" @("powershell", "-ExecutionPolicy", "Bypass", "-File", "Tools\windows\check_prereqs.ps1")

    $Cargo = Find-Cargo
    $ToolCommand = @($Cargo, "run", "-p", "frostwake-tools", "--")

    $QualityCommand = $ToolCommand + @("quality-gate", "--require-ue")
    Invoke-Logged "quality_gate" $QualityCommand

    $UnrealCommand = $ToolCommand + @("unreal-gate", "--platform", "Win64", "--include-server")
    if ($SkipGenerate) {
        $UnrealCommand += "--skip-generate"
    }
    if ($UeRoot -and $UeRoot.Length -gt 0) {
        $UnrealCommand += @("--ue-root", $UeRoot)
    }
    Invoke-Logged "unreal_gate_win64" $UnrealCommand

    if ($IncludeSmoke) {
        Invoke-Logged "smoke_qa_bot" (Add-UeRootArgs -Command ($ToolCommand + @("run-local-smoke", "--profile", "qa-bot", "--skip-build", "--null-rhi", "--platform", "Win64")))
        Invoke-Logged "smoke_match_timer" (Add-UeRootArgs -Command ($ToolCommand + @("run-local-smoke", "--profile", "match-timer", "--skip-build", "--null-rhi", "--platform", "Win64")))
        Invoke-Logged "smoke_life_action" (Add-UeRootArgs -Command ($ToolCommand + @("run-local-smoke", "--profile", "life-action", "--skip-build", "--null-rhi", "--platform", "Win64")))
        Invoke-Logged "smoke_combined5" (Add-UeRootArgs -Command ($ToolCommand + @("run-local-smoke", "--profile", "combined5", "--skip-build", "--null-rhi", "--platform", "Win64")))
        Invoke-Logged "smoke_ready8" (Add-UeRootArgs -Command ($ToolCommand + @("run-local-smoke", "--profile", "ready8", "--skip-build", "--null-rhi", "--platform", "Win64")))
        Invoke-Logged "smoke_suite_quick" (Add-UeRootArgs -Command ($ToolCommand + @("run-smoke-suite", "--skip-build", "--null-rhi", "--platform", "Win64")))
    }

    if ($IncludeHeavySmoke) {
        Invoke-Logged "smoke_suite_heavy" (Add-UeRootArgs -Command ($ToolCommand + @("run-smoke-suite", "--include-heavy", "--skip-build", "--null-rhi", "--platform", "Win64")))
    }

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Windows validation completed.",
        "Timestamp: $Stamp",
        "CargoTargetDir: $CargoTargetDir",
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
