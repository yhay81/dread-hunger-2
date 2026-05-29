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

function Write-Manifest {
    param([string]$Decision)

    $ManifestPath = Join-Path $OutDir "manifest.json"
    $Commit = "unknown"
    $GitOutput = & git -C $RepoRoot rev-parse --short HEAD 2>$null
    if ($LASTEXITCODE -eq 0 -and $GitOutput) {
        $Commit = [string]$GitOutput[0]
    }
    $StepArray = $StepResults.ToArray()

    $Payload = [ordered]@{
        decision = $Decision
        timestamp = $Stamp
        commit = $Commit
        runId = $RunId
        steamConfig = $SteamConfig
        lobbyMetadata = $LobbyMetadata
        expectedBuildId = $ExpectedBuildId
        expectedMapId = $ExpectedMapId
        requireSteamConfig = [bool]$RequireSteamConfig
        runtimeRequested = [bool]$Runtime
        output = $OutDir
        steps = $StepArray
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

    $StdOutPath = "$LogPath.stdout"
    $StdErrPath = "$LogPath.stderr"
    $ArgumentList = @()
    if ($Command.Count -gt 1) {
        foreach ($Argument in $Command[1..($Command.Count - 1)]) {
            if ($Argument -match '\s') {
                $ArgumentList += ('"' + $Argument.Replace('"', '\"') + '"')
            } else {
                $ArgumentList += $Argument
            }
        }
    }
    $Process = Start-Process `
        -FilePath $Command[0] `
        -ArgumentList $ArgumentList `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $StdOutPath `
        -RedirectStandardError $StdErrPath
    $ExitCode = $Process.ExitCode
    $OutputLines = @()
    if (Test-Path $StdOutPath) {
        $OutputLines += @(Get-Content -Path $StdOutPath)
    }
    if (Test-Path $StdErrPath) {
        $OutputLines += @(Get-Content -Path $StdErrPath)
    }
    $OutputLines | Set-Content -Path $LogPath -Encoding UTF8
    $OutputLines | ForEach-Object { Write-Host $_ }
    Remove-Item -Force -Path $StdOutPath, $StdErrPath -ErrorAction SilentlyContinue

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
    $CargoPath = Find-Cargo
    if ([string]::IsNullOrWhiteSpace($CargoPath)) {
        Write-Host "[FAIL] cargo not found. Run Tools\windows\check_prereqs.ps1."
        Write-Manifest -Decision "fail"
        exit 2
    }
    Use-RustEnvironment $CargoPath
    $LobbyMetadataCommand = @(
        $CargoPath,
        "run",
        "-p",
        "frostwake-tools",
        "--",
        "lobby-metadata-check",
        $ResolvedLobbyMetadata,
        "--expected-build-id",
        $ExpectedBuildId,
        "--expected-map-id",
        $ExpectedMapId,
        "--json"
    )
    Invoke-Logged "lobby_metadata" $LobbyMetadataCommand

    $LobbyJoinDecisionCommand = @(
        $CargoPath,
        "run",
        "-p",
        "frostwake-tools",
        "--",
        "lobby-join-decision",
        $ResolvedLobbyMetadata,
        "--expected-build-id",
        $ExpectedBuildId,
        "--expected-map-id",
        $ExpectedMapId,
        "--require-accepted",
        "--json"
    )
    Invoke-Logged "lobby_join_decision" $LobbyJoinDecisionCommand

    $LobbyBuildMismatchCommand = @(
        $CargoPath,
        "run",
        "-p",
        "frostwake-tools",
        "--",
        "lobby-join-decision",
        $ResolvedLobbyMetadata,
        "--expected-build-id",
        "$ExpectedBuildId-mismatch-probe",
        "--expected-map-id",
        $ExpectedMapId,
        "--expected-reject-reason",
        "BuildMismatch",
        "--json"
    )
    Invoke-Logged "lobby_join_build_mismatch" $LobbyBuildMismatchCommand

    $LobbyMapMismatchCommand = @(
        $CargoPath,
        "run",
        "-p",
        "frostwake-tools",
        "--",
        "lobby-join-decision",
        $ResolvedLobbyMetadata,
        "--expected-build-id",
        $ExpectedBuildId,
        "--expected-map-id",
        "$ExpectedMapId-mismatch-probe",
        "--expected-reject-reason",
        "MapMismatch",
        "--json"
    )
    Invoke-Logged "lobby_join_map_mismatch" $LobbyMapMismatchCommand

    if ($Runtime) {
        $Message = "Steam Lobby runtime validation is not implemented yet. Implement UFrostwakeLobbySubsystem from docs\steam-lobby-subsystem-design.md before using -Runtime."
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
        "JoinDecision: accepted metadata plus BuildMismatch and MapMismatch rejection probes passed.",
        "Output: $OutDir",
        "Manifest: $(Join-Path $OutDir "manifest.json")",
        "Next: implement UFrostwakeLobbySubsystem and rerun this wrapper with -Runtime."
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
