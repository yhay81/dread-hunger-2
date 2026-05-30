param(
    [string]$ServerExe = "",
    [string]$ServerConfig = "",
    [string]$UeRoot = $env:UE_ROOT,
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$Clients = 8,
    [int]$ExpectedPlayers = 8,
    [int]$ExpectedSaboteurs = 2,
    [int]$StartupDelaySeconds = 8,
    [int]$ClientLaunchSpacingSeconds = 2,
    [int]$MatchTimeoutSeconds = 90,
    [string]$RunId = "",
    [string]$BuildId = "Frostwake-Win64-Development-local",
    [string]$Profile = "dedicated-ready8"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "Frostwake.uproject"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\DedicatedReadyValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-dedicated-ready-$Stamp"
}

$ResolvedServerExe = ""
$ResolvedUeRoot = ""
$UnrealEditor = ""
$ResolvedServerConfig = ""
$EventLog = Join-Path $OutDir "server.jsonl"
$ConfigCheckLog = Join-Path $OutDir "server_config_check.log"
$ServerStdout = Join-Path $OutDir "server_stdout.log"
$ServerStderr = Join-Path $OutDir "server_stderr.log"
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
        "BuildTarget: FrostwakeServer",
        "BuildId: $BuildId",
        "Profile: $Profile",
        "ServerExe: $ResolvedServerExe",
        "UeRoot: $ResolvedUeRoot",
        "UnrealEditor: $UnrealEditor",
        "Project: $Project",
        "ServerConfig: $ResolvedServerConfig",
        "Map: $Map",
        "Port: $Port",
        "Clients: $Clients",
        "ExpectedPlayers: $ExpectedPlayers",
        "ExpectedSaboteurs: $ExpectedSaboteurs",
        "StartupDelaySeconds: $StartupDelaySeconds",
        "ClientLaunchSpacingSeconds: $ClientLaunchSpacingSeconds",
        "MatchTimeoutSeconds: $MatchTimeoutSeconds",
        "EventLog: $EventLog",
        "ServerStdout: $ServerStdout",
        "ServerStderr: $ServerStderr",
        "ConfigCheckLog: $ConfigCheckLog",
        "LogSummary: $SummaryJson",
        "Decision: $Decision",
        "Detail: $Detail"
    ) | Set-Content -Path $SummaryPath -Encoding UTF8

    $ClientLogs = @()
    for ($Index = 1; $Index -le $Clients; $Index++) {
        $ClientLogs += [ordered]@{
            index = $Index
            stdout = Join-Path $OutDir ("client_{0:D2}_stdout.log" -f $Index)
            stderr = Join-Path $OutDir ("client_{0:D2}_stderr.log" -f $Index)
        }
    }

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
        ueRoot = $ResolvedUeRoot
        unrealEditor = $UnrealEditor
        project = $Project
        serverConfig = $ResolvedServerConfig
        map = $Map
        port = $Port
        clients = $Clients
        expectedPlayers = $ExpectedPlayers
        expectedSaboteurs = $ExpectedSaboteurs
        startupDelaySeconds = $StartupDelaySeconds
        clientLaunchSpacingSeconds = $ClientLaunchSpacingSeconds
        matchTimeoutSeconds = $MatchTimeoutSeconds
        eventLog = $EventLog
        serverStdout = $ServerStdout
        serverStderr = $ServerStderr
        clientLogs = $ClientLogs
        configCheckLog = $ConfigCheckLog
        logSummary = $SummaryJson
        logSummaryCommandLog = $LogSummaryLog
        output = $OutDir
    }
    $Payload | ConvertTo-Json -Depth 8 | Set-Content -Path $ManifestPath -Encoding UTF8
}

$ServerProcess = $null
$ClientProcesses = New-Object System.Collections.Generic.List[System.Diagnostics.Process]

Push-Location $RepoRoot
try {
    if ($Clients -lt 1) {
        Write-Summary "fail" "-Clients must be >= 1"
        Write-Host "[FAIL] -Clients must be >= 1"
        Write-Host "Output: $OutDir"
        exit 2
    }

    $ResolvedServerExe = if ([string]::IsNullOrWhiteSpace($ServerExe)) {
        Join-Path $RepoRoot "Binaries\Win64\FrostwakeServer.exe"
    } else {
        Resolve-RepoPath $ServerExe
    }

    if (-not (Test-Path $ResolvedServerExe)) {
        Write-Summary "blocked" "Missing server executable: $ResolvedServerExe"
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        Write-Host "Output: $OutDir"
        exit 3
    }

    $ResolvedUeRoot = Find-UnrealRoot
    if ([string]::IsNullOrWhiteSpace($ResolvedUeRoot)) {
        Write-Summary "blocked" "Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        Write-Host "[FAIL] Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        Write-Host "Output: $OutDir"
        exit 4
    }

    $UnrealEditor = Join-Path $ResolvedUeRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $UnrealEditor)) {
        Write-Summary "blocked" "Missing UnrealEditor: $UnrealEditor"
        Write-Host "[FAIL] Missing UnrealEditor: $UnrealEditor"
        Write-Host "Output: $OutDir"
        exit 5
    }

    if (-not (Test-Path $Project)) {
        Write-Summary "fail" "Missing project: $Project"
        Write-Host "[FAIL] Missing project: $Project"
        Write-Host "Output: $OutDir"
        exit 6
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
            exit 7
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
        exit 8
    }
    Use-RustEnvironment $CargoPath
    $ConfigCheckArgs = @("run", "-p", "frostwake-tools", "--", "server-config-check", $ResolvedServerConfig)
    & $CargoPath @ConfigCheckArgs 2>&1 | Tee-Object -FilePath $ConfigCheckLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "Invalid server config: $ResolvedServerConfig"
        Write-Host "[FAIL] Invalid server config: $ResolvedServerConfig"
        Write-Host "Output: $OutDir"
        exit 8
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
        "-FrostwakeEventLog=$EventLog",
        "-FrostwakeRunId=$RunId",
        "-FrostwakeBuildId=$BuildId",
        "-FrostwakeMapId=$Map",
        "-FrostwakeProfile=$Profile",
        "-FrostwakeAutoReady",
        "-FrostwakeLobbyMinPlayers=$ExpectedPlayers",
        "-FrostwakeLobbyDevSaboteurs=$ExpectedSaboteurs",
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
        Write-Summary "fail" "Server exited before clients launched with code $($ServerProcess.ExitCode)"
        Write-Host "[FAIL] Server exited before clients launched."
        Write-Host "Output: $OutDir"
        exit 9
    }

    for ($Index = 1; $Index -le $Clients; $Index++) {
        $ClientStdout = Join-Path $OutDir ("client_{0:D2}_stdout.log" -f $Index)
        $ClientStderr = Join-Path $OutDir ("client_{0:D2}_stderr.log" -f $Index)
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
            "-port=$($Port + $Index)"
        )

        Write-Host "[START] client_$('{0:D2}' -f $Index): $UnrealEditor"
        Write-Host ($ClientArgs -join " ")
        $ClientProcess = Start-Process `
            -FilePath $UnrealEditor `
            -ArgumentList $ClientArgs `
            -PassThru `
            -NoNewWindow `
            -RedirectStandardOutput $ClientStdout `
            -RedirectStandardError $ClientStderr
        $ClientProcesses.Add($ClientProcess)

        Start-Sleep -Seconds $ClientLaunchSpacingSeconds
        if ($ServerProcess.HasExited) {
            Write-Summary "fail" "Server exited while launching clients with code $($ServerProcess.ExitCode)"
            Write-Host "[FAIL] Server exited while launching clients."
            Write-Host "Output: $OutDir"
            exit 10
        }
    }

    $Deadline = (Get-Date).AddSeconds($MatchTimeoutSeconds)
    $RoleMatched = $false
    $MatchStarted = $false
    while ((Get-Date) -lt $Deadline) {
        if ($ServerProcess.HasExited) {
            Write-Summary "fail" "Server exited before role assignment with code $($ServerProcess.ExitCode)"
            Write-Host "[FAIL] Server exited before role assignment."
            Write-Host "Output: $OutDir"
            exit 11
        }

        if (Test-Path $EventLog) {
            $RoleEvents = Select-String -Path $EventLog -Pattern '"event":"role_assignment_complete"' -ErrorAction SilentlyContinue
            if ($RoleEvents) {
                $RoleMatched = $true
            }
            $MatchEvents = Select-String -Path $EventLog -Pattern '"event":"match_started"' -ErrorAction SilentlyContinue
            if ($MatchEvents) {
                $MatchStarted = $true
            }
            if ($RoleMatched -and $MatchStarted) {
                break
            }
        }
        Start-Sleep -Seconds 1
    }

    foreach ($ClientProcess in $ClientProcesses) {
        Stop-ValidationProcess $ClientProcess
    }
    Stop-ValidationProcess $ServerProcess

    if (-not $RoleMatched) {
        Write-Summary "fail" "No role_assignment_complete event within ${MatchTimeoutSeconds}s"
        Write-Host "[FAIL] Dedicated ready match did not start."
        Write-Host "Output: $OutDir"
        exit 12
    }
    if (-not $MatchStarted) {
        Write-Summary "fail" "No match_started event within ${MatchTimeoutSeconds}s"
        Write-Host "[FAIL] Dedicated ready match assigned roles but did not emit match_started."
        Write-Host "Output: $OutDir"
        exit 13
    }

    $LogSummaryArgs = @("run", "-p", "frostwake-tools", "--", "log-summary", $EventLog, "--out", $SummaryJson)
    & $CargoPath @LogSummaryArgs 2>&1 | Tee-Object -FilePath $LogSummaryLog
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "frostwake-tools log-summary failed"
        Write-Host "[FAIL] frostwake-tools log-summary failed for $EventLog"
        Write-Host "Output: $OutDir"
        exit 14
    }

    $Summary = Get-Content -Raw -Path $SummaryJson | ConvertFrom-Json
    if ($Summary.players_connected -lt $ExpectedPlayers) {
        Write-Summary "fail" "Expected $ExpectedPlayers connected players, got $($Summary.players_connected)"
        Write-Host "[FAIL] Not enough dedicated clients connected."
        Write-Host "Output: $OutDir"
        exit 15
    }
    if ($Summary.last_role_assignment.players -ne $ExpectedPlayers) {
        Write-Summary "fail" "Expected role players=$ExpectedPlayers, got $($Summary.last_role_assignment.players)"
        Write-Host "[FAIL] Unexpected role assignment player count."
        Write-Host "Output: $OutDir"
        exit 16
    }
    if ($Summary.last_role_assignment.saboteurs -ne $ExpectedSaboteurs) {
        Write-Summary "fail" "Expected saboteurs=$ExpectedSaboteurs, got $($Summary.last_role_assignment.saboteurs)"
        Write-Host "[FAIL] Unexpected role assignment saboteur count."
        Write-Host "Output: $OutDir"
        exit 17
    }
    if ($Summary.match_started -lt 1) {
        Write-Summary "fail" "Expected match_started event"
        Write-Host "[FAIL] Dedicated match did not emit match_started."
        Write-Host "Output: $OutDir"
        exit 18
    }

    Write-Summary "pass" "Dedicated ready match started with $ExpectedPlayers players and $ExpectedSaboteurs saboteur(s)"
    Write-Host "[PASS] Dedicated ready validation completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    foreach ($ClientProcess in $ClientProcesses) {
        Stop-ValidationProcess $ClientProcess
    }
    Stop-ValidationProcess $ServerProcess
    Pop-Location
}
