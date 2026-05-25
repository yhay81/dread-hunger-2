param(
    [string]$UeRoot = $env:UE_ROOT,
    [string]$ServerExe = "",
    [string]$ServerConfig = "",
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$BootDurationSeconds = 20,
    [int]$StartupDelaySeconds = 8,
    [int]$ClientDurationSeconds = 15,
    [int]$Clients = 5,
    [int]$ExpectedPlayers = 5,
    [int]$ExpectedSaboteurs = 1,
    [int]$ClientLaunchSpacingSeconds = 2,
    [int]$MatchTimeoutSeconds = 90,
    [string]$BuildId = "AbyssLock-Win64-Development-local",
    [string]$RunId = "",
    [switch]$SkipGenerate,
    [switch]$IncludeSmoke,
    [switch]$IncludeHeavySmoke
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\Phase2EntryValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-phase2-entry-$Stamp"
}

if ($UeRoot -and $UeRoot.Length -gt 0) {
    $env:UE_ROOT = $UeRoot
}

$StepResults = New-Object System.Collections.Generic.List[object]

function Write-Manifest {
    param([string]$Decision)

    $ManifestPath = Join-Path $OutDir "manifest.json"
    $Payload = [ordered]@{
        decision = $Decision
        timestamp = $Stamp
        commit = "$(git -C $RepoRoot rev-parse --short HEAD)"
        runId = $RunId
        buildId = $BuildId
        ueRoot = $UeRoot
        serverExe = $ServerExe
        serverConfig = $ServerConfig
        map = $Map
        port = $Port
        bootDurationSeconds = $BootDurationSeconds
        startupDelaySeconds = $StartupDelaySeconds
        clientDurationSeconds = $ClientDurationSeconds
        clients = $Clients
        expectedPlayers = $ExpectedPlayers
        expectedSaboteurs = $ExpectedSaboteurs
        clientLaunchSpacingSeconds = $ClientLaunchSpacingSeconds
        matchTimeoutSeconds = $MatchTimeoutSeconds
        includeSmoke = [bool]$IncludeSmoke
        includeHeavySmoke = [bool]$IncludeHeavySmoke
        output = $OutDir
        steps = @($StepResults)
    }
    $Payload | ConvertTo-Json -Depth 8 | Set-Content -Path $ManifestPath -Encoding UTF8
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
    $ChildOutputs = @()
    if (Test-Path $LogPath) {
        $ChildOutputs = @(Select-String -Path $LogPath -Pattern "^(Output|Log):\s+(.+)$" | ForEach-Object { $_.Matches[0].Groups[2].Value.Trim() })
    }

    $StepResults.Add([ordered]@{
        name = $Name
        command = ($Command -join " ")
        logPath = $LogPath
        childOutputs = $ChildOutputs
        exitCode = $ExitCode
    }) | Out-Null
    Write-Manifest -Decision "running"

    if ($ExitCode -ne 0) {
        Write-Host "[FAIL] $Name exited with $ExitCode"
        Write-Host "Log: $LogPath"
        Write-Manifest -Decision "fail"
        exit $ExitCode
    }

    Write-Host "[PASS] $Name"
}

function Add-OptionalArg {
    param(
        [string[]]$Command,
        [string]$Name,
        [string]$Value
    )

    if ($Value -and $Value.Length -gt 0) {
        return [string[]]($Command + @($Name, $Value))
    }

    return $Command
}

Push-Location $RepoRoot
try {
    $FirstValidation = @("powershell", "-ExecutionPolicy", "Bypass", "-File", "Tools\windows\run_first_validation.ps1")
    if ($SkipGenerate) {
        $FirstValidation += "-SkipGenerate"
    }
    if ($IncludeSmoke) {
        $FirstValidation += "-IncludeSmoke"
    }
    if ($IncludeHeavySmoke) {
        $FirstValidation += "-IncludeHeavySmoke"
    }
    $FirstValidation = Add-OptionalArg -Command $FirstValidation -Name "-UeRoot" -Value $UeRoot
    Invoke-Logged "phase1_windows_validation" $FirstValidation

    $DedicatedBoot = @(
        "powershell",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "Tools\windows\run_dedicated_server_validation.ps1",
        "-Map",
        $Map,
        "-Port",
        "$Port",
        "-DurationSeconds",
        "$BootDurationSeconds",
        "-RunId",
        "$RunId-boot",
        "-BuildId",
        $BuildId
    )
    $DedicatedBoot = Add-OptionalArg -Command $DedicatedBoot -Name "-ServerExe" -Value $ServerExe
    $DedicatedBoot = Add-OptionalArg -Command $DedicatedBoot -Name "-ServerConfig" -Value $ServerConfig
    Invoke-Logged "dedicated_server_boot" $DedicatedBoot

    $ClientJoin = @(
        "powershell",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "Tools\windows\run_dedicated_client_join_validation.ps1",
        "-Map",
        $Map,
        "-Port",
        "$Port",
        "-StartupDelaySeconds",
        "$StartupDelaySeconds",
        "-ClientDurationSeconds",
        "$ClientDurationSeconds",
        "-RunId",
        "$RunId-client-join",
        "-BuildId",
        $BuildId
    )
    $ClientJoin = Add-OptionalArg -Command $ClientJoin -Name "-UeRoot" -Value $UeRoot
    $ClientJoin = Add-OptionalArg -Command $ClientJoin -Name "-ServerExe" -Value $ServerExe
    $ClientJoin = Add-OptionalArg -Command $ClientJoin -Name "-ServerConfig" -Value $ServerConfig
    Invoke-Logged "dedicated_client_join" $ClientJoin

    $ReadyLobby = @(
        "powershell",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "Tools\windows\run_dedicated_ready_validation.ps1",
        "-Map",
        $Map,
        "-Port",
        "$Port",
        "-Clients",
        "$Clients",
        "-ExpectedPlayers",
        "$ExpectedPlayers",
        "-ExpectedSaboteurs",
        "$ExpectedSaboteurs",
        "-StartupDelaySeconds",
        "$StartupDelaySeconds",
        "-ClientLaunchSpacingSeconds",
        "$ClientLaunchSpacingSeconds",
        "-MatchTimeoutSeconds",
        "$MatchTimeoutSeconds",
        "-RunId",
        "$RunId-ready5",
        "-BuildId",
        $BuildId
    )
    $ReadyLobby = Add-OptionalArg -Command $ReadyLobby -Name "-UeRoot" -Value $UeRoot
    $ReadyLobby = Add-OptionalArg -Command $ReadyLobby -Name "-ServerExe" -Value $ServerExe
    $ReadyLobby = Add-OptionalArg -Command $ReadyLobby -Name "-ServerConfig" -Value $ServerConfig
    Invoke-Logged "dedicated_ready_lobby" $ReadyLobby

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Phase 2 entry validation completed.",
        "Timestamp: $Stamp",
        "RunId: $RunId",
        "BuildId: $BuildId",
        "Map: $Map",
        "Port: $Port",
        "Clients: $Clients",
        "ExpectedPlayers: $ExpectedPlayers",
        "ExpectedSaboteurs: $ExpectedSaboteurs",
        "Output: $OutDir",
        "Manifest: $(Join-Path $OutDir "manifest.json")",
        "Next: record P2-001 and P2-005 results in docs\windows-phase2-entry-template.md or a new cycle record."
    ) | Set-Content -Path $SummaryPath -Encoding UTF8

    Write-Manifest -Decision "pass"

    Write-Host ""
    Write-Host "[PASS] Phase 2 entry validation command set completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    Pop-Location
}
