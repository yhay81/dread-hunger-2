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

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Commit: $(git -C $RepoRoot rev-parse --short HEAD)",
        "RunId: $RunId",
        "ServerExe: $ResolvedServerExe",
        "UnrealEditor: $UnrealEditor",
        "ServerConfig: $ResolvedServerConfig",
        "Map: $Map",
        "Port: $Port",
        "EventLog: $EventLog",
        "Decision: $Decision",
        "Detail: $Detail"
    ) | Set-Content -Path $SummaryPath -Encoding UTF8
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
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        exit 2
    }

    $ResolvedUeRoot = Find-UnrealRoot
    if ([string]::IsNullOrWhiteSpace($ResolvedUeRoot)) {
        Write-Host "[FAIL] Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        exit 3
    }

    $UnrealEditor = Join-Path $ResolvedUeRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $UnrealEditor)) {
        Write-Host "[FAIL] Missing UnrealEditor: $UnrealEditor"
        exit 4
    }

    if (-not (Test-Path $Project)) {
        Write-Host "[FAIL] Missing project: $Project"
        exit 5
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
            exit 6
        }

        $ConfigObject = Get-Content -Raw -Path $ResolvedServerConfig | ConvertFrom-Json
        if ($ConfigObject.PSObject.Properties.Name -contains "logPath") {
            $EventLog = Resolve-RepoPath ([string]$ConfigObject.logPath)
        }
    }

    $ConfigCheckLog = Join-Path $OutDir "server_config_check.log"
    py -3 Tools\ops\server_config_check.py $ResolvedServerConfig 2>&1 | Tee-Object -FilePath $ConfigCheckLog
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] Invalid server config: $ResolvedServerConfig"
        Write-Host "Output: $OutDir"
        exit 7
    }

    $ServerStdout = Join-Path $OutDir "server_stdout.log"
    $ServerStderr = Join-Path $OutDir "server_stderr.log"
    $ServerArgs = @(
        $Map,
        "-log",
        "-stdout",
        "-FullStdOutLogOutput",
        "-unattended",
        "-NoLiveCoding",
        "-nop4",
        "-ServerConfig=$ResolvedServerConfig",
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

    $ClientStdout = Join-Path $OutDir "client_stdout.log"
    $ClientStderr = Join-Path $OutDir "client_stderr.log"
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

    $SummaryJson = Join-Path $OutDir "log_summary.json"
    py -3 Tools\log_summary.py $EventLog | Set-Content -Path $SummaryJson -Encoding UTF8
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "log_summary.py failed"
        Write-Host "[FAIL] log_summary.py failed for $EventLog"
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
