param(
    [string]$OutRoot = "Content/ThirdParty/Quarantine/ambientCG",
    [string]$ProofRoot = "Saved/AssetProof/ambientcg-free-materials",
    [string]$Resolution = "1K-JPG",
    [switch]$Download
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$outRootPath = Join-Path $repoRoot $OutRoot
$proofRootPath = Join-Path $repoRoot $ProofRoot
$assetIds = @(
    "DiamondPlate009",
    "PaintedMetal004",
    "MetalWalkway014",
    "Snow015",
    "Ice003"
)

New-Item -ItemType Directory -Force -Path $outRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $proofRootPath | Out-Null

$headers = @{ "User-Agent" = "FrostwakeAssetIntake/0.1" }
$snapshot = [ordered]@{
    capturedAt = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    provider = "ambientCG"
    providerLicenseUrl = "https://docs.ambientcg.com/license/"
    license = "Creative Commons CC0 1.0 Universal"
    resolution = $Resolution
    downloadMode = [bool]$Download
    assets = @()
}

foreach ($assetId in $assetIds) {
    $apiUrl = "https://ambientCG.com/api/v2/full_json?id=$assetId&include=downloadData,displayData,tagData"
    $response = Invoke-RestMethod -Uri $apiUrl -Headers $headers
    $asset = $response.foundAssets | Select-Object -First 1
    if (-not $asset) {
        throw "ambientCG asset not found: $assetId"
    }

    $assetDownload = $asset.downloadFolders.default.downloadFiletypeCategories.zip.downloads |
        Where-Object { $_.attribute -eq $Resolution } |
        Select-Object -First 1
    if (-not $assetDownload) {
        throw "ambientCG asset $assetId does not expose $Resolution"
    }

    $assetOut = Join-Path $outRootPath $assetId
    $sourceOut = Join-Path $assetOut "Source"
    New-Item -ItemType Directory -Force -Path $sourceOut | Out-Null

    $zipPath = Join-Path $sourceOut $assetDownload.fileName
    if ($Download) {
        Invoke-WebRequest -Uri $assetDownload.downloadLink -OutFile $zipPath -Headers $headers
        $extractPath = Join-Path $assetOut $Resolution
        New-Item -ItemType Directory -Force -Path $extractPath | Out-Null
        Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force
    }

    $snapshot.assets += [ordered]@{
        assetId = $asset.assetId
        displayName = $asset.displayName
        shortLink = $asset.shortLink
        dataType = $asset.dataType
        creationMethod = $asset.creationMethod
        releaseDate = $asset.releaseDate
        tags = $asset.tags
        downloadFile = $assetDownload.fileName
        downloadLink = $assetDownload.downloadLink
        size = $assetDownload.size
        localPath = (Join-Path $OutRoot $assetId)
    }
}

$snapshotPath = Join-Path $proofRootPath "ambientcg-visual-poc-snapshot.json"
$snapshot | ConvertTo-Json -Depth 8 | Set-Content -Path $snapshotPath -Encoding UTF8
Write-Output "[PASS] wrote $snapshotPath"
if (-not $Download) {
    Write-Output "Dry run only. Re-run with -Download to fetch and extract source files."
}
