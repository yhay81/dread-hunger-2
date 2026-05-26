Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$RequiredFailures = New-Object System.Collections.Generic.List[string]

function Write-Check {
    param(
        [string]$Name,
        [bool]$Ok,
        [string]$Detail = ""
    )

    $Status = if ($Ok) { "PASS" } else { "FAIL" }
    if ($Detail.Length -gt 0) {
        Write-Host "[$Status] ${Name}: $Detail"
    } else {
        Write-Host "[$Status] $Name"
    }
}

function Test-CommandAvailable {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Find-UnrealRoot {
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

Push-Location $RepoRoot
try {
    $GitOk = Test-CommandAvailable "git"
    $GitDetail = if ($GitOk) { (git --version | Out-String).Trim() } else { "not found" }
    Write-Check "git" $GitOk $GitDetail
    if (-not $GitOk) { $RequiredFailures.Add("git") }

    $LfsOk = $false
    $LfsDetail = "not found"
    if ($GitOk) {
        $LfsOutput = git lfs version 2>&1
        $LfsOk = $LASTEXITCODE -eq 0
        $LfsDetail = ($LfsOutput | Out-String).Trim()
    }
    Write-Check "git-lfs" $LfsOk $LfsDetail
    if (-not $LfsOk) { $RequiredFailures.Add("git-lfs") }

    $CargoPath = Find-Cargo
    $CargoOk = $CargoPath.Length -gt 0
    $CargoDetail = if ($CargoOk) { ((& $CargoPath --version) | Out-String).Trim() } else { "cargo not found" }
    Write-Check "rust cargo" $CargoOk $CargoDetail
    if (-not $CargoOk) { $RequiredFailures.Add("Rust cargo") }

    $ProjectOk = Test-Path (Join-Path $RepoRoot "AbyssLock.uproject")
    Write-Check "uproject" $ProjectOk (Join-Path $RepoRoot "AbyssLock.uproject")
    if (-not $ProjectOk) { $RequiredFailures.Add("AbyssLock.uproject") }

    $UeRoot = Find-UnrealRoot
    $UeRootOk = $UeRoot.Length -gt 0
    $UeRootDetail = if ($UeRootOk) { $UeRoot } else { "set UE_ROOT to your UE_5.7 install" }
    Write-Check "unreal root" $UeRootOk $UeRootDetail
    if (-not $UeRootOk) { $RequiredFailures.Add("UE_ROOT") }

    $BuildBat = if ($UeRootOk) { Join-Path $UeRoot "Engine\Build\BatchFiles\Build.bat" } else { "" }
    $BuildBatOk = $UeRootOk -and (Test-Path $BuildBat)
    Write-Check "unreal build script" $BuildBatOk $BuildBat
    if (-not $BuildBatOk) { $RequiredFailures.Add("Build.bat") }

    $VsBase = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    if ([string]::IsNullOrWhiteSpace($VsBase)) {
        $VsBase = "C:\Program Files (x86)"
    }
    $VsWhere = Join-Path $VsBase "Microsoft Visual Studio\Installer\vswhere.exe"
    $VsOk = Test-Path $VsWhere
    $VsDetail = ""
    if ($VsOk) {
        $VsDetail = ((& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath) | Select-Object -First 1 | Out-String).Trim()
    } else {
        $VsDetail = "vswhere not found"
    }
    $VsInstallOk = $VsOk -and -not [string]::IsNullOrWhiteSpace($VsDetail)
    Write-Check "visual studio native game workload" $VsInstallOk $VsDetail
    if (-not $VsInstallOk) { $RequiredFailures.Add("Visual Studio Game development with C++") }

    if ($RequiredFailures.Count -gt 0) {
        Write-Host ""
        Write-Host "Missing required prerequisites:"
        foreach ($Failure in $RequiredFailures) {
            Write-Host "- $Failure"
        }
        exit 1
    }

    Write-Host ""
    Write-Host "Windows prerequisites look ready for repository validation."
    exit 0
}
finally {
    Pop-Location
}
