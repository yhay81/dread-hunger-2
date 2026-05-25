param(
    [string]$ServerExe = "",
    [string]$ServerConfig = "",
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$DurationSeconds = 20,
    [string]$RunId = "",
    [string]$BuildId = "AbyssLock-Win64-Development-local",
    [string]$Profile = "dedicated-private-test"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\DedicatedServerValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-dedicated-$Stamp"
}

function Resolve-RepoPath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    return (Join-Path $RepoRoot $PathValue)
}

function Write-Summary {
    param(
        [string]$Decision,
        [string]$Detail
    )

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Commit: $(git -C $RepoRoot rev-parse --short HEAD)",
        "RunId: $RunId",
        "ServerExe: $ResolvedServerExe",
        "ServerConfig: $ResolvedServerConfig",
        "Map: $Map",
        "Port: $Port",
        "DurationSeconds: $DurationSeconds",
        "EventLog: $EventLog",
        "Decision: $Decision",
        "Detail: $Detail"
    ) | Set-Content -Path $SummaryPath -Encoding UTF8
}

Push-Location $RepoRoot
try {
    $ResolvedServerExe = if ([string]::IsNullOrWhiteSpace($ServerExe)) {
        Join-Path $RepoRoot "Binaries\Win64\AbyssLockServer.exe"
    } else {
        Resolve-RepoPath $ServerExe
    }

    if (-not (Test-Path $ResolvedServerExe)) {
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        Write-Host "Build first with: py -3 Tools\unreal_gate.py --skip-generate --platform Win64 --include-server"
        exit 2
    }

    $EventLog = Join-Path $OutDir "server.jsonl"
    if ([string]::IsNullOrWhiteSpace($ServerConfig)) {
        $ResolvedServerConfig = Join-Path $OutDir "server_config.validation.json"
        $ExampleConfig = Join-Path $RepoRoot "Tools\ops\server_config.example.json"
        $ConfigObject = Get-Content -Raw -Path $ExampleConfig | ConvertFrom-Json
        $ConfigObject.ruleset = "private_test"
        $ConfigObject.advertise = $false
        $ConfigObject.logPath = $EventLog
        $ConfigObject | ConvertTo-Json -Depth 8 | Set-Content -Path $ResolvedServerConfig -Encoding UTF8
    } else {
        $ResolvedServerConfig = Resolve-RepoPath $ServerConfig
        if (-not (Test-Path $ResolvedServerConfig)) {
            Write-Host "[FAIL] Missing server config: $ResolvedServerConfig"
            exit 3
        }

        $ConfigObject = Get-Content -Raw -Path $ResolvedServerConfig | ConvertFrom-Json
        if ($ConfigObject.PSObject.Properties.Name -contains "logPath") {
            $EventLog = Resolve-RepoPath ([string]$ConfigObject.logPath)
        }
    }

    $StdoutLog = Join-Path $OutDir "server_stdout.log"
    $StderrLog = Join-Path $OutDir "server_stderr.log"
    $ConfigCheckLog = Join-Path $OutDir "server_config_check.log"
    py -3 Tools\ops\server_config_check.py $ResolvedServerConfig 2>&1 | Tee-Object -FilePath $ConfigCheckLog
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] Invalid server config: $ResolvedServerConfig"
        Write-Host "Output: $OutDir"
        exit 4
    }

    $ArgList = @(
        $Map,
        "-log",
        "-unattended",
        "-NoLiveCoding",
        "-nop4",
        "-ServerConfig=$ResolvedServerConfig",
        "-AbyssEventLog=$EventLog",
        "-AbyssRunId=$RunId",
        "-AbyssBuildId=$BuildId",
        "-AbyssMapId=$Map",
        "-AbyssProfile=$Profile",
        "-port=$Port"
    )

    Write-Host "[START] $ResolvedServerExe"
    Write-Host ($ArgList -join " ")
    $Process = Start-Process `
        -FilePath $ResolvedServerExe `
        -ArgumentList $ArgList `
        -PassThru `
        -NoNewWindow `
        -RedirectStandardOutput $StdoutLog `
        -RedirectStandardError $StderrLog

    Start-Sleep -Seconds $DurationSeconds
    $ExitedEarly = $Process.HasExited
    if (-not $ExitedEarly) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }

    if ($ExitedEarly) {
        $ExitCode = $Process.ExitCode
        Write-Summary "fail" "Server exited before ${DurationSeconds}s with code $ExitCode"
        Write-Host "[FAIL] Server exited early with code $ExitCode"
        Write-Host "Output: $OutDir"
        exit 5
    }

    if (-not (Test-Path $EventLog)) {
        Write-Summary "fail" "Server stayed alive but did not write expected event log"
        Write-Host "[FAIL] Missing event log: $EventLog"
        Write-Host "Output: $OutDir"
        exit 6
    }

    $SummaryJson = Join-Path $OutDir "log_summary.json"
    py -3 Tools\log_summary.py $EventLog | Set-Content -Path $SummaryJson -Encoding UTF8
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "log_summary.py failed"
        Write-Host "[FAIL] log_summary.py failed for $EventLog"
        Write-Host "Output: $OutDir"
        exit 7
    }

    Write-Summary "pass" "Server stayed alive for ${DurationSeconds}s and wrote telemetry"
    Write-Host "[PASS] Dedicated server validation completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    Pop-Location
}
