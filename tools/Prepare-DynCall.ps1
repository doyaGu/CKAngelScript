[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InstallRoot,
    [string]$WorkRoot = "",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$version = "1.4"
$archiveUrl = "https://dyncall.org/r1.4/dyncall-1.4.zip"
$archiveSha256 = "877D62FE1BEB41E21C9518144065074B35331AEEEE303D6452E2482A5CB6500E"

if ([string]::IsNullOrWhiteSpace($WorkRoot)) {
    $base = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
    $WorkRoot = Join-Path $base "ckas-dyncall"
}

$resolvedInstall = [System.IO.Path]::GetFullPath($InstallRoot)
$resolvedWork = [System.IO.Path]::GetFullPath($WorkRoot)
$requiredFiles = @(
    (Join-Path $resolvedInstall "include\dyncall.h"),
    (Join-Path $resolvedInstall "include\dyncall_callback.h"),
    (Join-Path $resolvedInstall "include\dynload.h"),
    (Join-Path $resolvedInstall "lib\dyncall_s.lib"),
    (Join-Path $resolvedInstall "lib\dyncallback_s.lib"),
    (Join-Path $resolvedInstall "lib\dynload_s.lib")
)
$stampPath = Join-Path $resolvedInstall ".ckas-dyncall.json"

if (Test-Path -LiteralPath $stampPath) {
    $stamp = Get-Content -LiteralPath $stampPath -Raw | ConvertFrom-Json
    $complete = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -eq 0
    if ($complete -and
        $stamp.version -eq $version -and
        $stamp.sha256 -eq $archiveSha256 -and
        $stamp.configuration -eq $Configuration) {
        Write-Host "Using verified DynCall $version at $resolvedInstall"
        return
    }
}

New-Item -ItemType Directory -Force -Path $resolvedWork, $resolvedInstall | Out-Null
$archive = Join-Path $resolvedWork "dyncall-$version.zip"
if (-not (Test-Path -LiteralPath $archive) -or
    (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $archiveSha256) {
    Invoke-WebRequest -Uri $archiveUrl -OutFile $archive -UseBasicParsing
}
$actualSha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
if ($actualSha256 -ne $archiveSha256) {
    throw "DynCall archive hash mismatch. Expected $archiveSha256, got $actualSha256."
}

$sourceParent = Join-Path $resolvedWork "source"
$sourceRoot = Join-Path $sourceParent "dyncall-$version"
if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot "CMakeLists.txt"))) {
    New-Item -ItemType Directory -Force -Path $sourceParent | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $sourceParent -Force
}

$buildRoot = Join-Path $resolvedWork "build-$Configuration"
& cmake -G Ninja -S $sourceRoot -B $buildRoot `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" `
    "-DCMAKE_INSTALL_PREFIX=$resolvedInstall"
if ($LASTEXITCODE -ne 0) { throw "DynCall configure failed with exit code $LASTEXITCODE." }
& cmake --build $buildRoot --target install
if ($LASTEXITCODE -ne 0) { throw "DynCall build failed with exit code $LASTEXITCODE." }

$missing = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
if ($missing.Count -ne 0) {
    throw "DynCall install is incomplete: $($missing -join ', ')"
}

@{
    version = $version
    sha256 = $archiveSha256
    configuration = $Configuration
} | ConvertTo-Json | Set-Content -LiteralPath $stampPath -Encoding utf8

Write-Host "Prepared verified DynCall $version at $resolvedInstall"
