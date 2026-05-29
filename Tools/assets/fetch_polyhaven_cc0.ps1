#requires -Version 7.0
<#
.SYNOPSIS
    Fetch a curated set of CC0 assets from Poly Haven into the local quarantine.
.DESCRIPTION
    Poly Haven's entire library is CC0 (public domain, no attribution required). This script
    downloads a curated overcast HDRI + industrial props (fbx + their textures) at 1k into
    Content/ThirdParty/Quarantine/polyhaven/ and writes a provenance JSON. Assets stay in
    quarantine until reviewed/imported per the GP-08 pipeline; never promote without IP Review.
.NOTES
    Reproducible: rerun to refresh. Source: https://polyhaven.com (API https://api.polyhaven.com).
#>
param(
    [string]$Resolution = "1k",
    [string[]]$Hdris = @("blaubeuren_outskirts"),
    [string[]]$Models = @("Barrel_01", "Barrel_02", "Barrel_03", "cardboard_box_01", "can_rusted", "Lantern_01")
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Base = Join-Path $Root "Content\ThirdParty\Quarantine\polyhaven"
$ApiBase = "https://api.polyhaven.com/files"
$records = @()

function Save-Url([string]$Url, [string]$OutFile) {
    $dir = Split-Path -Parent $OutFile
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing -TimeoutSec 120
    return (Get-Item $OutFile).Length
}

foreach ($slug in $Hdris) {
    try {
        $meta = Invoke-RestMethod -Uri "$ApiBase/$slug" -TimeoutSec 60
        $url = $meta.hdri.$Resolution.hdr.url
        $out = Join-Path $Base "hdri\$slug`_$Resolution.hdr"
        $len = Save-Url $url $out
        Write-Output "HDRI  $slug : $len bytes"
        $records += [pscustomobject]@{ type = "hdri"; slug = $slug; url = $url; bytes = $len }
    } catch { Write-Output "HDRI  $slug FAILED: $($_.Exception.Message)" }
}

foreach ($slug in $Models) {
    try {
        $meta = Invoke-RestMethod -Uri "$ApiBase/$slug" -TimeoutSec 60
        # Poly Haven nests the fbx file at fbx.<res>.fbx (with .url + .include textures).
        $entry = $meta.fbx.$Resolution.fbx
        if (-not $entry -or -not $entry.url) { Write-Output "MODEL $slug : no fbx/$Resolution"; continue }
        $total = 0
        $total += Save-Url $entry.url (Join-Path $Base "models\$slug\$slug`_$Resolution.fbx")
        if ($entry.include) {
            foreach ($inc in $entry.include.PSObject.Properties) {
                $rel = $inc.Name -replace "/", "\"
                $total += Save-Url $inc.Value.url (Join-Path $Base "models\$slug\$rel")
            }
        }
        Write-Output "MODEL $slug : $total bytes"
        $records += [pscustomobject]@{ type = "model"; slug = $slug; bytes = $total }
    } catch { Write-Output "MODEL $slug FAILED: $($_.Exception.Message)" }
}

$manifest = [pscustomobject]@{
    source     = "Poly Haven (https://polyhaven.com)"
    license    = "CC0 1.0 (public domain, no attribution required)"
    fetchedAt  = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    resolution = $Resolution
    assets     = $records
}
$manifestPath = Join-Path $Base "polyhaven-cc0-provenance.json"
$manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding utf8
Write-Output "Provenance written: $manifestPath"
