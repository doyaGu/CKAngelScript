param(
    [string]$BallanceRoot = "",
    [string]$BuildDll = "",
    [string]$ValidationDir = "",
    [int]$PlayerSeconds = 10,
    [switch]$SkipInstall,
    [switch]$SkipPlayer
)

$ErrorActionPreference = "Stop"

$BallanceRoot = if (-not [string]::IsNullOrWhiteSpace($BallanceRoot)) {
    $BallanceRoot
} elseif ($env:BALLANCE_ROOT) {
    $env:BALLANCE_ROOT
} elseif ($env:CKAS_BALLANCE_ROOT) {
    $env:CKAS_BALLANCE_ROOT
} else {
    throw "Ballance root was not provided. Pass -BallanceRoot or set BALLANCE_ROOT."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDll)) {
    $BuildDll = Join-Path $repoRoot "build\src\Release\AngelScript.dll"
}
if ([string]::IsNullOrWhiteSpace($ValidationDir)) {
    $ValidationDir = Join-Path $repoRoot "build\validation\ballance"
}

$buildingBlocks = Join-Path $BallanceRoot "BuildingBlocks"
$bin = Join-Path $BallanceRoot "Bin"
$player = Join-Path $bin "Player.exe"
$exporter = Join-Path $bin "VirtoolsDataExporter.exe"
$targetDll = Join-Path $buildingBlocks "AngelScript.dll"
$crashDir = Join-Path $BallanceRoot "CrashDumps"

if (-not (Test-Path -LiteralPath $BuildDll)) {
    throw "Built AngelScript.dll was not found: $BuildDll"
}
if (-not (Test-Path -LiteralPath $buildingBlocks)) {
    throw "Ballance BuildingBlocks directory was not found: $buildingBlocks"
}
if (-not (Test-Path -LiteralPath $exporter)) {
    throw "VirtoolsDataExporter.exe was not found: $exporter"
}

New-Item -ItemType Directory -Force -Path $ValidationDir | Out-Null

$sourceHash = (Get-FileHash -LiteralPath $BuildDll -Algorithm SHA256).Hash
$backupPath = $null
if (-not $SkipInstall) {
    if (Test-Path -LiteralPath $targetDll) {
        $targetHash = (Get-FileHash -LiteralPath $targetDll -Algorithm SHA256).Hash
        if ($targetHash -ne $sourceHash) {
            $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
            $backupPath = "$targetDll.bak-$stamp"
            Copy-Item -LiteralPath $targetDll -Destination $backupPath -Force
        }
    }
    Copy-Item -LiteralPath $BuildDll -Destination $targetDll -Force
}

$paramsJson = Join-Path $ValidationDir "params.json"
$opsJson = Join-Path $ValidationDir "ops.json"
$bbsJson = Join-Path $ValidationDir "bbs.json"
$selfTestMarker = Join-Path $ValidationDir "behavior-bridge-selftest.txt"

& $exporter -p $paramsJson -o $opsJson -b $bbsJson $targetDll
if ($LASTEXITCODE -ne 0) {
    throw "VirtoolsDataExporter failed with exit code $LASTEXITCODE"
}

$bbs = Get-Content -LiteralPath $bbsJson -Raw | ConvertFrom-Json
$expected = @("AngelScript Component")
$missing = @()
foreach ($name in $expected) {
    if (-not ($bbs | Where-Object { $_.name -eq $name })) {
        $missing += $name
    }
}
if ($missing.Count -gt 0) {
    throw "Exporter did not find expected AngelScript BBs: $($missing -join ', ')"
}

$playerResult = $null
if (-not $SkipPlayer) {
    if (-not (Test-Path -LiteralPath $player)) {
        throw "Player.exe was not found: $player"
    }

    $start = Get-Date
    Remove-Item -LiteralPath $selfTestMarker -ErrorAction SilentlyContinue
    $oldSelfTestMarker = $env:CKAS_SELFTEST_MARKER
    $env:CKAS_SELFTEST_MARKER = $selfTestMarker
    try {
        $process = Start-Process -FilePath $player -WorkingDirectory $bin -PassThru
    } finally {
        if ($null -eq $oldSelfTestMarker) {
            Remove-Item Env:\CKAS_SELFTEST_MARKER -ErrorAction SilentlyContinue
        } else {
            $env:CKAS_SELFTEST_MARKER = $oldSelfTestMarker
        }
    }
    Start-Sleep -Seconds $PlayerSeconds

    $closedByTest = $false
    $killedByTest = $false
    $hasExitedAfterWait = $process.HasExited
    if (-not $process.HasExited) {
        $closedByTest = $process.CloseMainWindow()
        Start-Sleep -Seconds 2
        $process.Refresh()
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $killedByTest = $true
            Start-Sleep -Milliseconds 500
            $process.Refresh()
        }
    }

    if (-not (Test-Path -LiteralPath $selfTestMarker)) {
        throw "AngelScript behavior bridge script self-test marker was not written by Player."
    }
    $selfTestText = Get-Content -LiteralPath $selfTestMarker -Raw
    if ($selfTestText -notmatch "(?m)^status=ok`r?$") {
        throw "AngelScript behavior bridge script self-test failed: $selfTestText"
    }

    $newDumps = @()
    if (Test-Path -LiteralPath $crashDir) {
        $newDumps = @(Get-ChildItem -LiteralPath $crashDir |
            Where-Object { $_.LastWriteTime -ge $start.AddSeconds(-2) } |
            Select-Object -ExpandProperty Name)
    }

    $playerResult = [pscustomobject]@{
        ProcessId = $process.Id
        HasExitedAfterWait = $hasExitedAfterWait
        ExitCode = if ($process.HasExited) { $process.ExitCode } else { $null }
        ClosedByTest = $closedByTest
        KilledByTest = $killedByTest
        NewCrashDumps = if ($newDumps.Count -gt 0) { $newDumps -join "; " } else { "" }
        ScriptSelfTest = ($selfTestText.Trim() -replace "`r?`n", "; ")
    }
}

[pscustomobject]@{
    InstalledDll = $targetDll
    SourceHash = $sourceHash
    Backup = $backupPath
    ExportedBBs = ($bbs | Select-Object -ExpandProperty name)
    ExportJson = $bbsJson
    ScriptSelfTestMarker = if (Test-Path -LiteralPath $selfTestMarker) { $selfTestMarker } else { $null }
    Player = $playerResult
}
