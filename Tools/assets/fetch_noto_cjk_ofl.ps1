#requires -Version 7.0
<#
.SYNOPSIS
    Fetch OFL Noto Sans CJK UI fonts (Japanese + Simplified Chinese) into the local quarantine.
.DESCRIPTION
    Downloads the variable Noto Sans JP and Noto Sans SC fonts AND their OFL.txt license files from
    the google/fonts repository into Content/ThirdParty/Quarantine/fonts/, then writes a provenance
    JSON. These give the CJK glyph coverage so Japanese / Simplified Chinese UI renders real glyphs
    instead of tofu (Latin + Cyrillic are already covered by the engine default font). OFL-1.1 allows
    commercial use, modification, and bundling PROVIDED the OFL.txt ships alongside the font and the
    Reserved Font Name rule is respected (do not ship a modified font under the name "Noto"). Assets
    stay in quarantine until IP Review; never promote/import without it.
.NOTES
    Reproducible: rerun to refresh. The raw font binaries are gitignored (large, IP-pending); this
    script + the provenance JSON + the asset-ledger row are the tracked evidence.
    Source: https://github.com/google/fonts (OFL families notosansjp / notosanssc).
#>
param(
    [string]$Ref = "main"
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Base = Join-Path $Root "Content\ThirdParty\Quarantine\fonts"
$RawBase = "https://raw.githubusercontent.com/google/fonts/$Ref"
$records = @()

function Save-Url([string]$Url, [string]$OutFile) {
    $dir = Split-Path -Parent $OutFile
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing -TimeoutSec 180
    return (Get-Item $OutFile).Length
}

# google/fonts OFL family slug -> quarantine subdir + clean output filename ([wght] = URL-encoded %5Bwght%5D).
$families = @(
    [pscustomobject]@{ culture = "ja";      name = "Noto Sans JP"; slug = "notosansjp"; file = "NotoSansJP%5Bwght%5D.ttf"; out = "NotoSansJP-VF.ttf" }
    [pscustomobject]@{ culture = "zh-Hans"; name = "Noto Sans SC"; slug = "notosanssc"; file = "NotoSansSC%5Bwght%5D.ttf"; out = "NotoSansSC-VF.ttf" }
)

foreach ($f in $families) {
    try {
        $dir = Join-Path $Base $f.slug
        $fontUrl = "$RawBase/ofl/$($f.slug)/$($f.file)"
        $oflUrl = "$RawBase/ofl/$($f.slug)/OFL.txt"
        $fontBytes = Save-Url $fontUrl (Join-Path $dir $f.out)
        $oflBytes = Save-Url $oflUrl (Join-Path $dir "OFL.txt")
        Write-Output "FONT  $($f.name) [$($f.culture)] : font=$fontBytes bytes, OFL.txt=$oflBytes bytes"
        $records += [pscustomobject]@{
            culture     = $f.culture
            family      = $f.name
            fontFile    = $f.out
            fontBytes   = $fontBytes
            licenseFile = "OFL.txt"
            licenseBytes = $oflBytes
            fontUrl     = $fontUrl
            licenseUrl  = $oflUrl
        }
    }
    catch { Write-Output "FONT  $($f.name) FAILED: $($_.Exception.Message)" }
}

$manifest = [pscustomobject]@{
    source      = "google/fonts (https://github.com/google/fonts)"
    ref         = $Ref
    license     = "SIL Open Font License 1.1 (OFL-1.1)"
    licenseNote = "OFL.txt must ship alongside the font; Reserved Font Name 'Noto' — do not ship a modified font under that name; no in-UI attribution required."
    fetchedAt   = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    purpose     = "CJK glyph coverage for JA + Simplified Chinese UI (Latin/Cyrillic already covered by engine default font)"
    fonts       = $records
}
$manifestPath = Join-Path $Base "noto-cjk-ofl-provenance.json"
$manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding utf8
Write-Output "Provenance written: $manifestPath"
