param(
    [string]$BallanceRoot = "",
    [string]$BuildDll = "",
    [string]$ValidationDir = "",
    [int]$PlayerSeconds = 60,
    [int]$LogTailLines = 40,
    [switch]$IncludeLogTail,
    [switch]$SkipInstall,
    [switch]$SkipPlayer
)

$ErrorActionPreference = "Stop"

function Read-ValidationTextFile {
    param(
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return ""
    }

    try {
        return Get-Content -LiteralPath $Path -Raw -ErrorAction Stop
    } catch {
        return "Failed to read $Path`: $($_.Exception.Message)"
    }
}

function Get-ValidationTextTail {
    param(
        [string]$Path,
        [int]$LineCount = 80,
        [int]$StartLine = 0
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return "<missing: $Path>"
    }

    try {
        if ($StartLine -gt 0) {
            $allLines = @(Get-Content -LiteralPath $Path -ErrorAction Stop)
            $lines = @($allLines | Select-Object -Skip $StartLine | Select-Object -Last $LineCount)
            if ($lines.Count -eq 0) {
                return "<no new log lines since validation started>"
            }
        } else {
            $lines = Get-Content -LiteralPath $Path -Tail $LineCount -ErrorAction Stop
        }
        if ($null -eq $lines) {
            return ""
        }
        return ($lines -join [Environment]::NewLine)
    } catch {
        return "Failed to read $Path`: $($_.Exception.Message)"
    }
}

function Get-ValidationLineCount {
    param(
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return 0
    }

    try {
        return @(Get-Content -LiteralPath $Path -ErrorAction Stop).Count
    } catch {
        return 0
    }
}

function Get-BallanceValidationDiagnostics {
    param(
        [string]$StartupSelfTestMarker,
        [string]$AngelScriptLog,
        [string]$PlayerLog,
        [int]$LogTailLines = 40,
        [int]$AngelScriptLogStartLine = 0,
        [int]$PlayerLogStartLine = 0
    )

    $markerText = Read-ValidationTextFile -Path $StartupSelfTestMarker
    if ([string]::IsNullOrWhiteSpace($markerText)) {
        $markerText = "<missing or empty>"
    }

    return @(
        "Startup self-test marker ($StartupSelfTestMarker):",
        $markerText.TrimEnd(),
        "",
        "AngelScript.log new tail ($AngelScriptLog):",
        (Get-ValidationTextTail -Path $AngelScriptLog -LineCount $LogTailLines -StartLine $AngelScriptLogStartLine).TrimEnd(),
        "",
        "Player.log new tail ($PlayerLog):",
        (Get-ValidationTextTail -Path $PlayerLog -LineCount $LogTailLines -StartLine $PlayerLogStartLine).TrimEnd()
    ) -join [Environment]::NewLine
}

function Throw-BallanceValidationError {
    param(
        [string]$Message,
        [string]$StartupSelfTestMarker,
        [string]$AngelScriptLog,
        [string]$PlayerLog,
        [int]$LogTailLines = 40,
        [int]$AngelScriptLogStartLine = 0,
        [int]$PlayerLogStartLine = 0
    )

    $diagnostics = Get-BallanceValidationDiagnostics `
        -StartupSelfTestMarker $StartupSelfTestMarker `
        -AngelScriptLog $AngelScriptLog `
        -PlayerLog $PlayerLog `
        -LogTailLines $LogTailLines `
        -AngelScriptLogStartLine $AngelScriptLogStartLine `
        -PlayerLogStartLine $PlayerLogStartLine
    throw "$Message$([Environment]::NewLine)$([Environment]::NewLine)$diagnostics"
}

function Stop-BallanceValidationPlayer {
    param(
        [System.Diagnostics.Process]$Process
    )

    if ($null -eq $Process) {
        return [pscustomobject]@{
            HasExitedBeforeClose = $true
            ExitCode = $null
            ClosedByTest = $false
            KilledByTest = $false
        }
    }

    $Process.Refresh()
    $hasExitedBeforeClose = $Process.HasExited
    $closedByTest = $false
    $killedByTest = $false

    if (-not $Process.HasExited) {
        $closedByTest = $Process.CloseMainWindow()
        Start-Sleep -Seconds 2
        $Process.Refresh()
        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force
            $killedByTest = $true
            Start-Sleep -Milliseconds 500
            $Process.Refresh()
        }
    }

    return [pscustomobject]@{
        HasExitedBeforeClose = $hasExitedBeforeClose
        ExitCode = if ($Process.HasExited) { $Process.ExitCode } else { $null }
        ClosedByTest = $closedByTest
        KilledByTest = $killedByTest
    }
}

function Wait-StartupSelfTestMarker {
    param(
        [string]$StartupSelfTestMarker,
        [System.Diagnostics.Process]$Process,
        [int]$TimeoutSeconds,
        [string]$AngelScriptLog,
        [string]$PlayerLog,
        [int]$LogTailLines = 40,
        [int]$AngelScriptLogStartLine = 0,
        [int]$PlayerLogStartLine = 0
    )

    $timeoutMilliseconds = [Math]::Max(1, $TimeoutSeconds) * 1000
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    while ($stopwatch.ElapsedMilliseconds -lt $timeoutMilliseconds) {
        if (Test-Path -LiteralPath $StartupSelfTestMarker) {
            $selfTestText = Read-ValidationTextFile -Path $StartupSelfTestMarker
            if ($selfTestText -match "(?m)^status=ok`r?$") {
                return [pscustomobject]@{
                    MarkerText = $selfTestText
                    ElapsedSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
                }
            }
            if ($selfTestText -match "(?m)^status=failed`r?$") {
                Throw-BallanceValidationError `
                    -Message "AngelScript startup self-test failed." `
                    -StartupSelfTestMarker $StartupSelfTestMarker `
                    -AngelScriptLog $AngelScriptLog `
                    -PlayerLog $PlayerLog `
                    -LogTailLines $LogTailLines `
                    -AngelScriptLogStartLine $AngelScriptLogStartLine `
                    -PlayerLogStartLine $PlayerLogStartLine
            }
        }

        $Process.Refresh()
        if ($Process.HasExited) {
            Throw-BallanceValidationError `
                -Message "Player exited before AngelScript startup self-tests completed. ExitCode=$($Process.ExitCode)." `
                -StartupSelfTestMarker $StartupSelfTestMarker `
                -AngelScriptLog $AngelScriptLog `
                -PlayerLog $PlayerLog `
                -LogTailLines $LogTailLines `
                -AngelScriptLogStartLine $AngelScriptLogStartLine `
                -PlayerLogStartLine $PlayerLogStartLine
        }

        Start-Sleep -Milliseconds 250
    }

    Throw-BallanceValidationError `
        -Message "Timed out waiting $TimeoutSeconds second(s) for AngelScript startup self-tests." `
        -StartupSelfTestMarker $StartupSelfTestMarker `
        -AngelScriptLog $AngelScriptLog `
        -PlayerLog $PlayerLog `
        -LogTailLines $LogTailLines `
        -AngelScriptLogStartLine $AngelScriptLogStartLine `
        -PlayerLogStartLine $PlayerLogStartLine
}

function Resolve-BallanceRoot {
    param(
        [string]$RequestedRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        return $RequestedRoot
    }

    $candidates = @()
    if ($env:BALLANCE_ROOT) {
        $candidates += $env:BALLANCE_ROOT
    }
    if ($env:CKAS_BALLANCE_ROOT) {
        $candidates += $env:CKAS_BALLANCE_ROOT
    }
    $userProfile = [Environment]::GetFolderPath("UserProfile")
    if (-not [string]::IsNullOrWhiteSpace($userProfile)) {
        $candidates += (Join-Path $userProfile "Games\Ballance")
    }

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and
            (Test-Path -LiteralPath (Join-Path $candidate "BuildingBlocks")) -and
            (Test-Path -LiteralPath (Join-Path $candidate "Bin"))) {
            return $candidate
        }
    }

    throw "Ballance root was not provided or auto-detected. Pass -BallanceRoot, set BALLANCE_ROOT, or install under %USERPROFILE%\Games\Ballance."
}

$BallanceRoot = Resolve-BallanceRoot -RequestedRoot $BallanceRoot

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
$angelScriptLog = Join-Path $bin "AngelScript.log"
$playerLog = Join-Path $bin "Player.log"
$angelScriptLogStartLine = Get-ValidationLineCount -Path $angelScriptLog
$playerLogStartLine = Get-ValidationLineCount -Path $playerLog

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
$paramsJson = Join-Path $ValidationDir "params.json"
$opsJson = Join-Path $ValidationDir "ops.json"
$bbsJson = Join-Path $ValidationDir "bbs.json"
$startupSelfTestMarker = Join-Path $ValidationDir "startup-selftest.txt"
Remove-Item -LiteralPath $startupSelfTestMarker -ErrorAction SilentlyContinue

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

$installedHash = if (Test-Path -LiteralPath $targetDll) {
    (Get-FileHash -LiteralPath $targetDll -Algorithm SHA256).Hash
} else {
    $null
}
$installedMatchesSource = $installedHash -eq $sourceHash
if ([string]::IsNullOrWhiteSpace($installedHash)) {
    throw "Installed AngelScript.dll was not found: $targetDll"
}
if (-not $SkipInstall -and -not $installedMatchesSource) {
    throw "Installed AngelScript.dll hash does not match built DLL after copy. Built=$sourceHash Installed=$installedHash"
}

& $exporter -p $paramsJson -o $opsJson -b $bbsJson $targetDll
if ($LASTEXITCODE -ne 0) {
    Throw-BallanceValidationError `
        -Message "VirtoolsDataExporter failed with exit code $LASTEXITCODE." `
        -StartupSelfTestMarker $startupSelfTestMarker `
        -AngelScriptLog $angelScriptLog `
        -PlayerLog $playerLog `
        -LogTailLines $LogTailLines `
        -AngelScriptLogStartLine $angelScriptLogStartLine `
        -PlayerLogStartLine $playerLogStartLine
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
    Throw-BallanceValidationError `
        -Message "Exporter did not find expected AngelScript BBs: $($missing -join ', ')." `
        -StartupSelfTestMarker $startupSelfTestMarker `
        -AngelScriptLog $angelScriptLog `
        -PlayerLog $playerLog `
        -LogTailLines $LogTailLines `
        -AngelScriptLogStartLine $angelScriptLogStartLine `
        -PlayerLogStartLine $playerLogStartLine
}

$playerResult = $null
if (-not $SkipPlayer) {
    if (-not (Test-Path -LiteralPath $player)) {
        throw "Player.exe was not found: $player"
    }

    $oldSelfTestMarker = $env:CKAS_SELFTEST_MARKER
    $oldRunSelfTests = $env:CKAS_RUN_SELFTESTS
    $env:CKAS_SELFTEST_MARKER = $startupSelfTestMarker
    $env:CKAS_RUN_SELFTESTS = "1"
    $process = $null
    $waitResult = $null
    $closeResult = $null
    try {
        $process = Start-Process -FilePath $player -WorkingDirectory $bin -PassThru
        $waitResult = Wait-StartupSelfTestMarker `
            -StartupSelfTestMarker $startupSelfTestMarker `
            -Process $process `
            -TimeoutSeconds $PlayerSeconds `
            -AngelScriptLog $angelScriptLog `
            -PlayerLog $playerLog `
            -LogTailLines $LogTailLines `
            -AngelScriptLogStartLine $angelScriptLogStartLine `
            -PlayerLogStartLine $playerLogStartLine
    } finally {
        if ($null -eq $oldSelfTestMarker) {
            Remove-Item Env:\CKAS_SELFTEST_MARKER -ErrorAction SilentlyContinue
        } else {
            $env:CKAS_SELFTEST_MARKER = $oldSelfTestMarker
        }
        if ($null -eq $oldRunSelfTests) {
            Remove-Item Env:\CKAS_RUN_SELFTESTS -ErrorAction SilentlyContinue
        } else {
            $env:CKAS_RUN_SELFTESTS = $oldRunSelfTests
        }
        $closeResult = Stop-BallanceValidationPlayer -Process $process
    }

    $playerResult = [pscustomobject]@{
        ProcessId = $process.Id
        WaitTimeoutSeconds = $PlayerSeconds
        SelfTestElapsedSeconds = $waitResult.ElapsedSeconds
        HasExitedBeforeClose = $closeResult.HasExitedBeforeClose
        ExitCode = $closeResult.ExitCode
        ClosedByTest = $closeResult.ClosedByTest
        KilledByTest = $closeResult.KilledByTest
        StartupSelfTest = ($waitResult.MarkerText.Trim() -replace "`r?`n", "; ")
    }
}

$result = [ordered]@{
    Status = "ok"
    BallanceRoot = $BallanceRoot
    InstalledDll = $targetDll
    SourceHash = $sourceHash
    InstalledHash = $installedHash
    InstalledMatchesSource = $installedMatchesSource
    Backup = $backupPath
    ExportedBBs = ($bbs | Select-Object -ExpandProperty name)
    ExportJson = $bbsJson
    StartupSelfTestMarker = if (Test-Path -LiteralPath $startupSelfTestMarker) { $startupSelfTestMarker } else { $null }
    AngelScriptLog = if (Test-Path -LiteralPath $angelScriptLog) { $angelScriptLog } else { $null }
    PlayerLog = if (Test-Path -LiteralPath $playerLog) { $playerLog } else { $null }
    Player = $playerResult
}
if ($IncludeLogTail) {
    $result["AngelScriptLogTail"] = Get-ValidationTextTail -Path $angelScriptLog -LineCount $LogTailLines -StartLine $angelScriptLogStartLine
    $result["PlayerLogTail"] = Get-ValidationTextTail -Path $playerLog -LineCount $LogTailLines -StartLine $playerLogStartLine
}

[pscustomobject]$result
