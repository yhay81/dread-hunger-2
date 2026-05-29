param(
    [string]$SteamConfig = "",
    [switch]$RequireSteamConfig
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ProjectPath = Join-Path $RepoRoot "Frostwake.uproject"
$DefaultEnginePath = Join-Path $RepoRoot "Config\DefaultEngine.ini"

function Resolve-RepoPath {
    param([string]$PathValue)

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    return (Join-Path $RepoRoot $PathValue)
}

function Fail {
    param([string]$Message)

    Write-Host "[FAIL] $Message"
    exit 1
}

if (-not (Test-Path $ProjectPath)) {
    Fail "Missing project file: $ProjectPath"
}
if (-not (Test-Path $DefaultEnginePath)) {
    Fail "Missing DefaultEngine.ini: $DefaultEnginePath"
}

$Project = Get-Content -Raw -Path $ProjectPath | ConvertFrom-Json
$SteamPlugin = $Project.Plugins | Where-Object { $_.Name -eq "OnlineSubsystemSteam" } | Select-Object -First 1
if ($null -eq $SteamPlugin) {
    Fail "OnlineSubsystemSteam plugin entry is missing. Keep it explicit and disabled until P2 opt-in work."
}
if ($SteamPlugin.Enabled -ne $false) {
    Fail "OnlineSubsystemSteam must remain disabled in Frostwake.uproject until Steam dev config is explicitly enabled on Windows."
}

$DefaultEngine = Get-Content -Raw -Path $DefaultEnginePath
if ($DefaultEngine -notmatch "DefaultPlatformService\s*=\s*Null") {
    Fail "Config\DefaultEngine.ini must keep DefaultPlatformService=Null for Phase 1 and local automation gates."
}
if ($DefaultEngine -match "DefaultPlatformService\s*=\s*Steam") {
    Fail "Do not commit DefaultPlatformService=Steam in Config\DefaultEngine.ini."
}
if ($DefaultEngine -match "SteamDevAppId\s*=") {
    Fail "Do not commit SteamDevAppId in Config\DefaultEngine.ini. Keep it in an ignored local Steam dev config."
}

if ([string]::IsNullOrWhiteSpace($SteamConfig)) {
    if ($RequireSteamConfig) {
        Fail "Missing -SteamConfig while -RequireSteamConfig is set."
    }

    Write-Host "[PASS] Default project config remains Null/LAN safe. No local Steam dev config checked."
    exit 0
}

$ResolvedSteamConfig = Resolve-RepoPath $SteamConfig
if (-not (Test-Path $ResolvedSteamConfig)) {
    Fail "Missing Steam dev config: $ResolvedSteamConfig"
}

$RepoFullPath = [System.IO.Path]::GetFullPath($RepoRoot)
$ConfigFullPath = [System.IO.Path]::GetFullPath($ResolvedSteamConfig)
$SavedConfigRoot = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot "Saved\Config"))
if (-not $ConfigFullPath.StartsWith($SavedConfigRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    Fail "Steam dev config must live under ignored Saved\Config\. Use Saved\Config\steam_dev.local.ini."
}

$GitCheck = & git -C $RepoFullPath check-ignore -q $ConfigFullPath
if ($LASTEXITCODE -ne 0) {
    Fail "Steam dev config is not ignored by git: $ConfigFullPath"
}

$SteamDevConfig = Get-Content -Raw -Path $ResolvedSteamConfig
$RequiredPatterns = @(
    "\[OnlineSubsystem\]",
    "DefaultPlatformService\s*=\s*Steam",
    "\[OnlineSubsystemSteam\]",
    "bEnabled\s*=\s*true",
    "SteamDevAppId\s*=\s*\d+",
    "GameServerQueryPort\s*=\s*\d+"
)

foreach ($Pattern in $RequiredPatterns) {
    if ($SteamDevConfig -notmatch $Pattern) {
        Fail "Steam dev config is missing required pattern: $Pattern"
    }
}

Write-Host "[PASS] Steam dev config is local, opt-in, and contains the required Steam development fields."
Write-Host "Config: $ResolvedSteamConfig"
