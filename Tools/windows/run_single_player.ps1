param(
    [string]$UeRoot = $env:UE_ROOT,
    [string]$Map = "/Game/Maps/L_IcebreakerWhitebox",
    [int]$Port = 7777,
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [string]$RunId = "",
    [string]$BuildId = "AbyssLock-Win64-Development-local",
    [string]$Profile = "single-player"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "Frostwake.uproject"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$OutDir = Join-Path $RepoRoot "Saved\SinglePlayer\$Stamp"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = "single-player-$Stamp"
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

$ResolvedUeRoot = Find-UnrealRoot
if ([string]::IsNullOrWhiteSpace($ResolvedUeRoot)) {
    Write-Host "[FAIL] Missing Unreal Engine root. Pass -UeRoot or set UE_ROOT."
    Write-Host "Output: $OutDir"
    exit 2
}

$UnrealEditor = Join-Path $ResolvedUeRoot "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $UnrealEditor)) {
    Write-Host "[FAIL] Missing UnrealEditor: $UnrealEditor"
    Write-Host "Output: $OutDir"
    exit 3
}

if (-not (Test-Path $Project)) {
    Write-Host "[FAIL] Missing project: $Project"
    Write-Host "Output: $OutDir"
    exit 4
}

$EventLog = Join-Path $OutDir "events.jsonl"
$LaunchCommandPath = Join-Path $OutDir "launch_command.txt"
$ManifestPath = Join-Path $OutDir "manifest.json"

$Args = @(
    $Project,
    "${Map}?listen",
    "-game",
    "-log",
    "-windowed",
    "-ResX=$ResX",
    "-ResY=$ResY",
    "-NoSplash",
    "-NoLiveCoding",
    "-nop4",
    "-FrostwakeSinglePlayer",
    "-FrostwakeEventLog=$EventLog",
    "-FrostwakeRunId=$RunId",
    "-FrostwakeBuildId=$BuildId",
    "-FrostwakeMapId=$Map",
    "-FrostwakeProfile=$Profile",
    "-port=$Port"
)

@(
    $UnrealEditor,
    ($Args -join " ")
) | Set-Content -Path $LaunchCommandPath -Encoding UTF8

Write-Host "[START] single-player: $UnrealEditor"
Write-Host ($Args -join " ")
$Process = Start-Process `
    -FilePath $UnrealEditor `
    -ArgumentList $Args `
    -WorkingDirectory $RepoRoot `
    -PassThru

Start-Sleep -Seconds 2
if ($Process.HasExited) {
    Write-Host "[FAIL] UnrealEditor exited early with code $($Process.ExitCode)."
    Write-Host "Output: $OutDir"
    exit 5
}

$Payload = [ordered]@{
    decision = "started"
    timestamp = $Stamp
    runId = $RunId
    buildId = $BuildId
    profile = $Profile
    ueRoot = $ResolvedUeRoot
    unrealEditor = $UnrealEditor
    project = $Project
    map = $Map
    port = $Port
    resX = $ResX
    resY = $ResY
    eventLog = $EventLog
    launchCommand = $LaunchCommandPath
    processId = $Process.Id
    output = $OutDir
}
$Payload | ConvertTo-Json -Depth 6 | Set-Content -Path $ManifestPath -Encoding UTF8

Write-Host "[PASS] Single-player session started."
Write-Host "ProcessId: $($Process.Id)"
Write-Host "Output: $OutDir"
