param(
    [string]$SteamConfig = "",
    [string]$LobbyMetadata = "Tools\ops\lobby_metadata.example.json",
    [string]$ExpectedBuildId = "AbyssLock-Win64-Development-local",
    [string]$ExpectedMapId = "L_IcebreakerWhitebox",
    [string]$RunId = "",
    [switch]$RequireSteamConfig,
    [switch]$Runtime
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\SteamLobbyValidation\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "windows-steam-lobby-$Stamp"
}

$StepResults = New-Object System.Collections.Generic.List[object]

function Resolve-RepoPath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    return (Join-Path $RepoRoot $PathValue)
}

function Write-Manifest {
    param([string]$Decision)

    $ManifestPath = Join-Path $OutDir "manifest.json"
    $Payload = [ordered]@{
        decision = $Decision
        timestamp = $Stamp
        commit = "$(git -C $RepoRoot rev-parse --short HEAD)"
        runId = $RunId
        steamConfig = $SteamConfig
        lobbyMetadata = $LobbyMetadata
        expectedBuildId = $ExpectedBuildId
        expectedMapId = $ExpectedMapId
        requireSteamConfig = [bool]$RequireSteamConfig
        runtimeRequested = [bool]$Runtime
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
    $StepResults.Add([ordered]@{
        name = $Name
        command = ($Command -join " ")
        logPath = $LogPath
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

Push-Location $RepoRoot
try {
    $SteamConfigCommand = @(
        "powershell",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        "Tools\windows\check_steam_dev_config.ps1"
    )
    if (-not [string]::IsNullOrWhiteSpace($SteamConfig)) {
        $SteamConfigCommand += @("-SteamConfig", $SteamConfig)
    }
    if ($RequireSteamConfig) {
        $SteamConfigCommand += "-RequireSteamConfig"
    }
    Invoke-Logged "steam_dev_config" $SteamConfigCommand

    $ResolvedLobbyMetadata = Resolve-RepoPath $LobbyMetadata
    $LobbyMetadataCommand = @(
        "py",
        "-3",
        "Tools\ops\lobby_metadata_check.py",
        $ResolvedLobbyMetadata,
        "--expected-build-id",
        $ExpectedBuildId,
        "--expected-map-id",
        $ExpectedMapId,
        "--json"
    )
    Invoke-Logged "lobby_metadata" $LobbyMetadataCommand

    if ($Runtime) {
        $Message = "Steam Lobby runtime validation is not implemented yet. Implement UAbyssLobbySubsystem from docs\steam-lobby-subsystem-design.md before using -Runtime."
        $RuntimeLog = Join-Path $OutDir "runtime_not_implemented.log"
        $Message | Set-Content -Path $RuntimeLog -Encoding UTF8
        $StepResults.Add([ordered]@{
            name = "steam_lobby_runtime"
            command = "-Runtime"
            logPath = $RuntimeLog
            exitCode = 9
        }) | Out-Null
        Write-Host "[FAIL] $Message"
        Write-Manifest -Decision "not_implemented"
        exit 9
    }

    $SummaryPath = Join-Path $OutDir "summary.txt"
    @(
        "Steam Lobby validation preflight completed.",
        "Timestamp: $Stamp",
        "RunId: $RunId",
        "ExpectedBuildId: $ExpectedBuildId",
        "ExpectedMapId: $ExpectedMapId",
        "Output: $OutDir",
        "Manifest: $(Join-Path $OutDir "manifest.json")",
        "Next: implement UAbyssLobbySubsystem and rerun this wrapper with -Runtime."
    ) | Set-Content -Path $SummaryPath -Encoding UTF8

    Write-Manifest -Decision "preflight_pass"

    Write-Host ""
    Write-Host "[PASS] Steam Lobby validation preflight completed."
    Write-Host "Output: $OutDir"
    exit 0
}
finally {
    Pop-Location
}
