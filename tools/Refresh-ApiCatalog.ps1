param(
    [Parameter(Mandatory = $true)]
    [string]$LiveApiJson
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$syncScript = Join-Path $PSScriptRoot "sync_api_catalog.py"

$resolvedExport = (Resolve-Path -LiteralPath $LiveApiJson).Path
& python $syncScript --api-json $resolvedExport
if ($LASTEXITCODE -ne 0) {
    throw "API catalog refresh failed with exit code $LASTEXITCODE."
}

Push-Location $repoRoot
try {
    & python $syncScript --check
    if ($LASTEXITCODE -ne 0) {
        throw "Refreshed API catalog did not pass its consistency check."
    }
} finally {
    Pop-Location
}
