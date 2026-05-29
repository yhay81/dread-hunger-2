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

$ResolvedServerExe = ""
$ResolvedServerConfig = ""
$EventLog = Join-Path $OutDir "server.jsonl"
$StdoutLog = Join-Path $OutDir "server_stdout.log"
$StderrLog = Join-Path $OutDir "server_stderr.log"
$ConfigCheckLog = Join-Path $OutDir "server_config_check.log"
$SummaryJson = Join-Path $OutDir "log_summary.json"
$LogSummaryLog = Join-Path $OutDir "log_summary_command.log"

function Resolve-RepoPath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    return (Join-Path $RepoRoot $PathValue)
}

function Find-Cargo {
    $LocalCargo = Join-Path $RepoRoot "Tools\install\rust\cargo\bin\cargo.exe"
    if (Test-Path $LocalCargo) {
        return (Resolve-Path $LocalCargo).Path
    }

    $CargoCommand = Get-Command "cargo" -ErrorAction SilentlyContinue
    if ($null -ne $CargoCommand) {
        return $CargoCommand.Source
    }

    return ""
}

function Use-RustEnvironment {
    param([string]$CargoPath)

    $LocalRustRoot = Join-Path $RepoRoot "Tools\install\rust"
    if ($CargoPath.StartsWith($LocalRustRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $env:CARGO_HOME = Join-Path $LocalRustRoot "cargo"
        $env:RUSTUP_HOME = Join-Path $LocalRustRoot "rustup"
    }
}

function Write-Summary {
    param(
        [string]$Decision,
        [string]$Detail
    )

    $Commit = "$(git -C $RepoRoot rev-parse --short HEAD)"
    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Commit: $Commit",
        "RunId: $RunId",
        "Platform: Win64",
        "BuildTarget: FrostwakeServer",
        "BuildId: $BuildId",
        "Profile: $Profile",
        "ServerExe: $ResolvedServerExe",
        "ServerConfig: $ResolvedServerConfig",
        "Map: $Map",
        "Port: $Port",
        "DurationSeconds: $DurationSeconds",
        "EventLog: $EventLog",
        "StdoutLog: $StdoutLog",
        "StderrLog: $StderrLog",
        "ConfigCheckLog: $ConfigCheckLog",
        "LogSummary: $SummaryJson",
        "Decision: $Decision",
        "Detail: $Detail"
    ) | Set-Content -Path $SummaryPath -Encoding UTF8

    $ManifestPath = Join-Path $OutDir "manifest.json"
    $Payload = [ordered]@{
        decision = $Decision
        detail = $Detail
        timestamp = $Stamp
        commit = $Commit
        runId = $RunId
        platform = "Win64"
        buildTarget = "FrostwakeServer"
        buildId = $BuildId
        profile = $Profile
        serverExe = $ResolvedServerExe
        serverConfig = $ResolvedServerConfig
        map = $Map
        port = $Port
        durationSeconds = $DurationSeconds
        eventLog = $EventLog
        stdoutLog = $StdoutLog
        stderrLog = $StderrLog
        configCheckLog = $ConfigCheckLog
        logSummary = $SummaryJson
        logSummaryCommandLog = $LogSummaryLog
        output = $OutDir
    }
    $Payload | ConvertTo-Json -Depth 6 | Set-Content -Path $ManifestPath -Encoding UTF8
}

Push-Location $RepoRoot
try {
    $ResolvedServerExe = if ([string]::IsNullOrWhiteSpace($ServerExe)) {
        Join-Path $RepoRoot "Binaries\Win64\FrostwakeServer.exe"
    } else {
        Resolve-RepoPath $ServerExe
    }

    if (-not (Test-Path $ResolvedServerExe)) {
        Write-Summary "blocked" "Missing server executable: $ResolvedServerExe"
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        Write-Host "Build first with: cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server"
        Write-Host "Output: $OutDir"
        exit 2
    }

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
            Write-Summary "fail" "Missing server config: $ResolvedServerConfig"
            Write-Host "[FAIL] Missing server config: $ResolvedServerConfig"
            Write-Host "Output: $OutDir"
            exit 3
        }

        $ConfigObject = Get-Content -Raw -Path $ResolvedServerConfig | ConvertFrom-Json
        if ($ConfigObject.PSObject.Properties.Name -contains "logPath") {
            $EventLog = Resolve-RepoPath ([string]$ConfigObject.logPath)
        }
    }

    $CargoPath = Find-Cargo
    if ([string]::IsNullOrWhiteSpace($CargoPath)) {
        Write-Summary "blocked" "cargo not found"
        Write-Host "[FAIL] cargo not found. Run Tools\windows\check_prereqs.ps1."
        Write-Host "Output: $OutDir"
        exit 4
    }
    Use-RustEnvironment $CargoPath
    $ConfigCheckArgs = @("run", "-p", "frostwake-tools", "--", "server-config-check", $ResolvedServerConfig)
    & $CargoPath @ConfigCheckArgs 2>&1 | Tee-Object -FilePath $ConfigCheckLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "Invalid server config: $ResolvedServerConfig"
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
        "-FrostwakeEventLog=$EventLog",
        "-FrostwakeRunId=$RunId",
        "-FrostwakeBuildId=$BuildId",
        "-FrostwakeMapId=$Map",
        "-FrostwakeProfile=$Profile",
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

    $LogSummaryArgs = @("run", "-p", "frostwake-tools", "--", "log-summary", $EventLog, "--out", $SummaryJson)
    & $CargoPath @LogSummaryArgs 2>&1 | Tee-Object -FilePath $LogSummaryLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "frostwake-tools log-summary failed"
        Write-Host "[FAIL] frostwake-tools log-summary failed for $EventLog"
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
