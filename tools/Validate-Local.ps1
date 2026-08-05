param(
    [string]$BuildDir = "",
    [string]$Configuration = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "Win32",
    [string]$VirtoolsSdkPath = "",
    [string]$DynCallRoot = "",
    [string[]]$ScriptRoot = @(),
    [switch]$NoSelfTests
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build"
}

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $BuildDir,
    "-G", $Generator,
    "-A", $Platform,
    "-DCKAS_BUILD_SELF_TESTS=$([int](-not $NoSelfTests))"
)

if (-not [string]::IsNullOrWhiteSpace($VirtoolsSdkPath)) {
    $configureArgs += "-DVIRTOOLS_SDK_PATH=$VirtoolsSdkPath"
}
if (-not [string]::IsNullOrWhiteSpace($DynCallRoot)) {
    $configureArgs += "-DDYNCALL_ROOT=$DynCallRoot"
    $configureArgs += "-DDYNCALLBACK_ROOT=$DynCallRoot"
    $configureArgs += "-DDYNLOAD_ROOT=$DynCallRoot"
}

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

& cmake --build $BuildDir --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE."
}

$buildDll = Join-Path $BuildDir "src\$Configuration\AngelScript.dll"

if ($ScriptRoot.Count -gt 0) {
    & (Join-Path $PSScriptRoot "Validate-ScriptEntries.ps1") -ScriptRoot $ScriptRoot
}

[pscustomobject]@{
    BuildDir = $BuildDir
    Configuration = $Configuration
    SelfTestsBuilt = -not $NoSelfTests
    Dll = $buildDll
    ScriptEntryRoots = ($ScriptRoot -join ";")
    Status = "ok"
}
