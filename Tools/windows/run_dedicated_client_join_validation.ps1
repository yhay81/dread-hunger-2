param(
    [string]$ServerExe = "",
    [string]$ServerConfig = "",
    [string]$UeRoot = $env:UE_ROOT,
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$StartupDelaySeconds = 8,
    [int]$ClientDurationSeconds = 15,
    [string]$RunId = "",
    [string]$BuildId = "AbyssLock-Win64-Development-local",
    [string]$Profile = "dedicated-client-join"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "AbyssLock.uproject"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\DedicatedClientJoinValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-dedicated-client-join-$Stamp"
}

$ResolvedServerExe = ""
$ResolvedUeRoot = ""
$UnrealEditor = ""
$ResolvedServerConfig = ""
$EventLog = Join-Path $OutDir "server.jsonl"
$ConfigCheckLog = Join-Path $OutDir "server_config_check.log"
$ServerStdout = Join-Path $OutDir "server_stdout.log"
$ServerStderr = Join-Path $OutDir "server_stderr.log"
$ClientStdout = Join-Path $OutDir "client_stdout.log"
$ClientStderr = Join-Path $OutDir "client_stderr.log"
$SummaryJson = Join-Path $OutDir "log_summary.json"
$LogSummaryLog = Join-Path $OutDir "log_summary_command.log"

function Resolve-RepoPath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    return (Join-Path $RepoRoot $PathValue)
}

function Find-UnrealRoot {
    if ($UeRoot -and (Test-Path $UeRoot)) {
        return (Resolve-Path $UeRoot).Path
    }
    if ($env:UE_ROOT -and (Test-Path $env:UE_ROOT)) {
        return (Resolve-Path $env:UE_ROOT).Path
    }
    if ($env:UNREAL_ENGINE_ROOT -and (Test-Path $env:UNREAL_ENGINE_ROOT)) {
        return (Resolve-Path $env:UNREAL_ENGINE_ROOT).Path
    }

    $Candidates = @(
        "C:\Program Files\Epic Games\UE_5.7",
        "D:\Epic Games\UE_5.7",
        "E:\Epic Games\UE_5.7"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            return (Resolve-Path $Candidate).Path
        }
    }

    return ""
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

function Stop-ValidationProcess {
    param([AllowNull()][System.Diagnostics.Process]$Process)

    if ($null -eq $Process) {
        return
    }
    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
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
        "BuildTarget: AbyssLockServer",
        "BuildId: $BuildId",
        "Profile: $Profile",
        "ServerExe: $ResolvedServerExe",
        "UeRoot: $ResolvedUeRoot",
        "UnrealEditor: $UnrealEditor",
        "Project: $Project",
        "ServerConfig: $ResolvedServerConfig",
        "Map: $Map",
        "Port: $Port",
        "StartupDelaySeconds: $StartupDelaySeconds",
        "ClientDurationSeconds: $ClientDurationSeconds",
        "EventLog: $EventLog",
        "ServerStdout: $ServerStdout",
        "ServerStderr: $ServerStderr",
        "ClientStdout: $ClientStdout",
        "ClientStderr: $ClientStderr",
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
        buildTarget = "AbyssLockServer"
        buildId = $BuildId
        profile = $Profile
        serverExe = $ResolvedServerExe
        ueRoot = $ResolvedUeRoot
        unrealEditor = $UnrealEditor
        project = $Project
        serverConfig = $ResolvedServerConfig
        map = $Map
        port = $Port
        startupDelaySeconds = $StartupDelaySeconds
        clientDurationSeconds = $ClientDurationSeconds
        eventLog = $EventLog
        serverStdout = $ServerStdout
        serverStderr = $ServerStderr
        clientStdout = $ClientStdout
        clientStderr = $ClientStderr
        configCheckLog = $ConfigCheckLog
        logSummary = $SummaryJson
        logSummaryCommandLog = $LogSummaryLog
        output = $OutDir
    }
    $Payload | ConvertTo-Json -Depth 6 | Set-Content -Path $ManifestPath -Encoding UTF8
}

$ServerProcess = $null
$ClientProcess = $null

Push-Location $RepoRoot
try {
    $ResolvedServerExe = if ([string]::IsNullOrWhiteSpace($ServerExe)) {
        Join-Path $RepoRoot "Binaries\Win64\AbyssLockServer.exe"
    } else {
        Resolve-RepoPath $ServerExe
    }

    if (-not (Test-Path $ResolvedServerExe)) {
        Write-Summary "blocked" "Missing server executable: $ResolvedServerExe"
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        Write-Host "Output: $OutDir"
        exit 2
    }

    $ResolvedUeRoot = Find-UnrealRoot
    if ([string]::IsNullOrWhiteSpace($ResolvedUeRoot)) {
        Write-Summary "blocked" "Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        Write-Host "[FAIL] Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        Write-Host "Output: $OutDir"
        exit 3
    }

    $UnrealEditor = Join-Path $ResolvedUeRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $UnrealEditor)) {
        Write-Summary "blocked" "Missing UnrealEditor: $UnrealEditor"
        Write-Host "[FAIL] Missing UnrealEditor: $UnrealEditor"
        Write-Host "Output: $OutDir"
        exit 4
    }

    if (-not (Test-Path $Project)) {
        Write-Summary "fail" "Missing project: $Project"
        Write-Host "[FAIL] Missing project: $Project"
        Write-Host "Output: $OutDir"
        exit 5
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
            exit 6
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
        exit 7
    }
    Use-RustEnvironment $CargoPath
    $ConfigCheckArgs = @("run", "-p", "frostwake-tools", "--", "server-config-check", $ResolvedServerConfig)
    & $CargoPath @ConfigCheckArgs 2>&1 | Tee-Object -FilePath $ConfigCheckLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "Invalid server config: $ResolvedServerConfig"
        Write-Host "[FAIL] Invalid server config: $ResolvedServerConfig"
        Write-Host "Output: $OutDir"
        exit 7
    }

    $ServerArgs = @(
        $Map,
        "-log",
        "-stdout",
        "-FullStdOutLogOutput",
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

    Write-Host "[START] server: $ResolvedServerExe"
    Write-Host ($ServerArgs -join " ")
    $ServerProcess = Start-Process `
        -FilePath $ResolvedServerExe `
        -ArgumentList $ServerArgs `
        -PassThru `
        -NoNewWindow `
        -RedirectStandardOutput $ServerStdout `
        -RedirectStandardError $ServerStderr

    Start-Sleep -Seconds $StartupDelaySeconds
    if ($ServerProcess.HasExited) {
        Write-Summary "fail" "Server exited before client launch with code $($ServerProcess.ExitCode)"
        Write-Host "[FAIL] Server exited before client launch."
        Write-Host "Output: $OutDir"
        exit 8
    }

    $ClientArgs = @(
        $Project,
        "127.0.0.1:$Port",
        "-game",
        "-stdout",
        "-FullStdOutLogOutput",
        "-Unattended",
        "-NoSound",
        "-NoSplash",
        "-NoLiveCoding",
        "-nop4",
        "-nullrhi",
        "-port=$($Port + 1)"
    )

    Write-Host "[START] client: $UnrealEditor"
    Write-Host ($ClientArgs -join " ")
    $ClientProcess = Start-Process `
        -FilePath $UnrealEditor `
        -ArgumentList $ClientArgs `
        -PassThru `
        -NoNewWindow `
        -RedirectStandardOutput $ClientStdout `
        -RedirectStandardError $ClientStderr

    Start-Sleep -Seconds $ClientDurationSeconds
    if ($ServerProcess.HasExited) {
        Write-Summary "fail" "Server exited during client join with code $($ServerProcess.ExitCode)"
        Write-Host "[FAIL] Server exited during client join."
        Write-Host "Output: $OutDir"
        exit 9
    }

    Stop-ValidationProcess $ClientProcess
    Stop-ValidationProcess $ServerProcess

    if (-not (Test-Path $EventLog)) {
        Write-Summary "fail" "Server did not write expected event log"
        Write-Host "[FAIL] Missing event log: $EventLog"
        Write-Host "Output: $OutDir"
        exit 10
    }

    $LogSummaryArgs = @("run", "-p", "frostwake-tools", "--", "log-summary", $EventLog, "--out", $SummaryJson)
    & $CargoPath @LogSummaryArgs 2>&1 | Tee-Object -FilePath $LogSummaryLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "frostwake-tools log-summary failed"
        Write-Host "[FAIL] frostwake-tools log-summary failed for $EventLog"
        Write-Host "Output: $OutDir"
        exit 11
    }

    $Summary = Get-Content -Raw -Path $SummaryJson | ConvertFrom-Json
    if ($Summary.players_connected -lt 1) {
        Write-Summary "fail" "Expected at least one client_connected event, got $($Summary.players_connected)"
        Write-Host "[FAIL] Dedicated client did not connect according to server telemetry."
        Write-Host "Output: $OutDir"
        exit 12
    }

    Write-Summary "pass" "Server telemetry recorded $($Summary.players_connected) client connection(s)"
    Write-Host "[PASS] Dedicated client join validation completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    Stop-ValidationProcess $ClientProcess
    Stop-ValidationProcess $ServerProcess
    Pop-Location
}
