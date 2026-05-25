param(
    [string]$ServerExe = "",
    [string]$ServerConfig = "",
    [string]$UeRoot = $env:UE_ROOT,
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$Clients = 5,
    [int]$ExpectedPlayers = 5,
    [int]$ExpectedSaboteurs = 1,
    [int]$StartupDelaySeconds = 8,
    [int]$ClientLaunchSpacingSeconds = 2,
    [int]$MatchTimeoutSeconds = 90,
    [string]$RunId = "",
    [string]$BuildId = "AbyssLock-Win64-Development-local",
    [string]$Profile = "dedicated-ready5"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "AbyssLock.uproject"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\DedicatedReadyValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-dedicated-ready-$Stamp"
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
        "Clients: $Clients",
        "ExpectedPlayers: $ExpectedPlayers",
        "ExpectedSaboteurs: $ExpectedSaboteurs",
        "EventLog: $EventLog",
        "Decision: $Decision",
        "Detail: $Detail"
    ) | Set-Content -Path $SummaryPath -Encoding UTF8
}

$ServerProcess = $null
$ClientProcesses = New-Object System.Collections.Generic.List[System.Diagnostics.Process]

Push-Location $RepoRoot
try {
    if ($Clients -lt 1) {
        Write-Host "[FAIL] -Clients must be >= 1"
        exit 2
    }

    $ResolvedServerExe = if ([string]::IsNullOrWhiteSpace($ServerExe)) {
        Join-Path $RepoRoot "Binaries\Win64\AbyssLockServer.exe"
    } else {
        Resolve-RepoPath $ServerExe
    }

    if (-not (Test-Path $ResolvedServerExe)) {
        Write-Host "[FAIL] Missing server executable: $ResolvedServerExe"
        exit 3
    }

    $ResolvedUeRoot = Find-UnrealRoot
    if ([string]::IsNullOrWhiteSpace($ResolvedUeRoot)) {
        Write-Host "[FAIL] Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
        exit 4
    }

    $UnrealEditor = Join-Path $ResolvedUeRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $UnrealEditor)) {
        Write-Host "[FAIL] Missing UnrealEditor: $UnrealEditor"
        exit 5
    }

    if (-not (Test-Path $Project)) {
        Write-Host "[FAIL] Missing project: $Project"
        exit 6
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
            exit 7
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
        exit 8
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
        "-AbyssEventLog=$EventLog",
        "-AbyssRunId=$RunId",
        "-AbyssBuildId=$BuildId",
        "-AbyssMapId=$Map",
        "-AbyssProfile=$Profile",
        "-AbyssAutoReady",
        "-AbyssLobbyMinPlayers=$ExpectedPlayers",
        "-AbyssLobbyDevSaboteurs=$ExpectedSaboteurs",
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

    $SummaryJson = Join-Path $OutDir "log_summary.json"
    py -3 Tools\log_summary.py $EventLog | Set-Content -Path $SummaryJson -Encoding UTF8
    if ($LASTEXITCODE -ne 0) {
        Write-Summary "fail" "log_summary.py failed"
        Write-Host "[FAIL] log_summary.py failed for $EventLog"
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
