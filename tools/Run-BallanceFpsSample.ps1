[CmdletBinding(DefaultParameterSetName = "Run")]
param(
    [Parameter(ParameterSetName = "Run")]
    [switch]$Generate,

    [Parameter(ParameterSetName = "Run")]
    [switch]$Install,

    [Parameter(ParameterSetName = "Run")]
    [switch]$Run,

    [Parameter(ParameterSetName = "Restore")]
    [switch]$Restore,

    [Parameter(ParameterSetName = "Run")]
    [switch]$KeepRunning,

    [Parameter(ParameterSetName = "Run")]
    [int]$PlayerSeconds = 15,

    [Parameter(ParameterSetName = "Run")]
    [int]$StressComponents = 1,

    [Parameter(ParameterSetName = "Run")]
    [Parameter(ParameterSetName = "Restore")]
    [string]$GameRoot,

    [Parameter(ParameterSetName = "Run")]
    [string]$NmoExe,

    [Parameter(ParameterSetName = "Run")]
    [string]$MountBehaviorName = "Default Level",

    [Parameter(ParameterSetName = "Run")]
    [int]$MountInputIndex = 1,

    [Parameter(ParameterSetName = "Run")]
    [string]$SourceBaseCmo,

    [Parameter(ParameterSetName = "Run")]
    [string]$OutputCmo,

    [Parameter(ParameterSetName = "Restore")]
    [string]$BackupDir
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-DefaultGameRoot {
    if ($env:BALLANCE_ROOT) {
        return $env:BALLANCE_ROOT
    }
    if ($env:CKAS_BALLANCE_ROOT) {
        return $env:CKAS_BALLANCE_ROOT
    }
    throw "Ballance root was not provided. Pass -GameRoot or set BALLANCE_ROOT."
}

function Resolve-DefaultNmoExe {
    param([string]$RepoRoot)

    if ($env:NMO_EXE) {
        return $env:NMO_EXE
    }
    if ($env:CKAS_NMO_EXE) {
        return $env:CKAS_NMO_EXE
    }

    $sibling = Resolve-Path -Path (Join-Path $RepoRoot "..\libnmo\build\tools\nmo.exe") -ErrorAction SilentlyContinue
    if ($sibling) {
        return $sibling.Path
    }
    return "nmo.exe"
}

function Resolve-LatestBackup {
    param([string]$ValidationDir)

    $latest = Get-ChildItem -Path $ValidationDir -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "ballance-fps-backup-*" } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($null -eq $latest) {
        throw "No Ballance FPS sample backup directory was found in $ValidationDir."
    }
    return $latest.FullName
}

function Backup-GameFiles {
    param(
        [string]$GameRoot,
        [string]$ValidationDir
    )

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $backup = Join-Path $ValidationDir "ballance-fps-backup-$stamp"
    New-Item -ItemType Directory -Force -Path $backup | Out-Null

    $base = Join-Path $GameRoot "base.cmo"
    $dll = Join-Path $GameRoot "BuildingBlocks\AngelScript.dll"
    if (!(Test-Path -LiteralPath $base)) {
        throw "Game base.cmo was not found: $base"
    }

    Copy-Item -LiteralPath $base -Destination (Join-Path $backup "base.cmo") -Force
    if (Test-Path -LiteralPath $dll) {
        Copy-Item -LiteralPath $dll -Destination (Join-Path $backup "AngelScript.dll") -Force
    }
    return $backup
}

function Restore-GameFiles {
    param(
        [string]$GameRoot,
        [string]$BackupDir
    )

    $baseBackup = Join-Path $BackupDir "base.cmo"
    $dllBackup = Join-Path $BackupDir "AngelScript.dll"
    $base = Join-Path $GameRoot "base.cmo"
    $dll = Join-Path $GameRoot "BuildingBlocks\AngelScript.dll"

    if (!(Test-Path -LiteralPath $baseBackup)) {
        throw "Backup base.cmo was not found: $baseBackup"
    }

    Get-Process -Name Player -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    Copy-Item -LiteralPath $baseBackup -Destination $base -Force
    if (Test-Path -LiteralPath $dllBackup) {
        Copy-Item -LiteralPath $dllBackup -Destination $dll -Force
    }

    [pscustomobject]@{
        Restored = $true
        BackupDir = $BackupDir
        BaseCmo = $base
        AngelScriptDll = $dll
    }
}

function Invoke-Nmo {
    param([string[]]$Arguments)

    & $script:NmoExe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "nmo failed with exit code $LASTEXITCODE"
    }
}

function Resolve-MountBehaviorId {
    param(
        [string]$InputCmo,
        [string]$BehaviorName,
        [string]$ValidationDir
    )

    $behaviorJson = Join-Path $ValidationDir "base-behaviors.json"
    & $script:NmoExe "-f" "json" "behavior" "list" $InputCmo > $behaviorJson
    if ($LASTEXITCODE -ne 0) {
        throw "nmo behavior list failed with exit code $LASTEXITCODE"
    }

    $behaviorReport = Get-Content -LiteralPath $behaviorJson -Raw | ConvertFrom-Json
    $matches = @($behaviorReport.data.objects | Where-Object { $_.name -eq $BehaviorName })
    if ($matches.Count -eq 0) {
        throw "Mount behavior '$BehaviorName' was not found in $InputCmo."
    }
    if ($matches.Count -gt 1) {
        throw "Mount behavior '$BehaviorName' is ambiguous in $InputCmo."
    }
    return [int]$matches[0].id
}

$repo = Resolve-RepoRoot
$GameRoot = if ($GameRoot) { $GameRoot } else { Resolve-DefaultGameRoot }
$script:NmoExe = if ($NmoExe) { $NmoExe } else { Resolve-DefaultNmoExe -RepoRoot $repo }
$validation = Join-Path $repo "build\validation\ballance"
$releaseDll = Join-Path $repo "build\src\Release\AngelScript.dll"
$bbsJson = Join-Path $validation "bbs.json"
$luaScript = Join-Path $repo "tools\Inject-BallanceFpsComponent.lua"
$output = if ($OutputCmo) { $OutputCmo } else { Join-Path $validation "base-cmo-fps.cmo" }
$gameBase = Join-Path $GameRoot "base.cmo"
$gameDll = Join-Path $GameRoot "BuildingBlocks\AngelScript.dll"
$player = Join-Path $GameRoot "Bin\Player.exe"
$bin = Join-Path $GameRoot "Bin"

if ($Restore) {
    $resolvedBackup = if ($BackupDir) { $BackupDir } else { Resolve-LatestBackup -ValidationDir $validation }
    Restore-GameFiles -GameRoot $GameRoot -BackupDir $resolvedBackup
    return
}

if (!$Generate -and !$Install -and !$Run) {
    $Generate = $true
    $Install = $true
    $Run = $true
}

if ($StressComponents -le 0) {
    throw "StressComponents must be positive."
}
if (!(Test-Path -LiteralPath $script:NmoExe)) {
    throw "nmo.exe was not found: $script:NmoExe"
}

New-Item -ItemType Directory -Force -Path $validation | Out-Null

if ($Generate) {
    if (!(Test-Path -LiteralPath $bbsJson)) {
        throw "BB manifest was not found: $bbsJson. Run tools\Validate-Ballance.ps1 -SkipPlayer first."
    }
    if (!(Test-Path -LiteralPath $luaScript)) {
        throw "FPS injection Lua script was not found: $luaScript"
    }

    $inputBase = if ($SourceBaseCmo) { $SourceBaseCmo } else { $gameBase }
    if (!(Test-Path -LiteralPath $inputBase)) {
        throw "Input base.cmo was not found: $inputBase"
    }

    $mountId = Resolve-MountBehaviorId -InputCmo $inputBase -BehaviorName $MountBehaviorName -ValidationDir $validation
    $oldParent = $env:CKAS_FPS_PARENT_BEHAVIOR_ID
    $oldInput = $env:CKAS_FPS_MOUNT_INPUT_INDEX
    $oldStress = $env:CKAS_FPS_STRESS_COMPONENTS
    try {
        $env:CKAS_FPS_PARENT_BEHAVIOR_ID = "$mountId"
        $env:CKAS_FPS_MOUNT_INPUT_INDEX = "$MountInputIndex"
        $env:CKAS_FPS_STRESS_COMPONENTS = "$StressComponents"

        Invoke-Nmo -Arguments @(
            "--plugin", $bbsJson,
            "script", "run",
            $luaScript,
            $inputBase,
            "-o", $output
        )
        Invoke-Nmo -Arguments @("validate", "all", $output)
    } finally {
        if ($null -eq $oldParent) { Remove-Item Env:\CKAS_FPS_PARENT_BEHAVIOR_ID -ErrorAction SilentlyContinue } else { $env:CKAS_FPS_PARENT_BEHAVIOR_ID = $oldParent }
        if ($null -eq $oldInput) { Remove-Item Env:\CKAS_FPS_MOUNT_INPUT_INDEX -ErrorAction SilentlyContinue } else { $env:CKAS_FPS_MOUNT_INPUT_INDEX = $oldInput }
        if ($null -eq $oldStress) { Remove-Item Env:\CKAS_FPS_STRESS_COMPONENTS -ErrorAction SilentlyContinue } else { $env:CKAS_FPS_STRESS_COMPONENTS = $oldStress }
    }
}

$backupDir = $null
if ($Install) {
    if (!(Test-Path -LiteralPath $output)) {
        throw "Generated CMO was not found: $output"
    }
    if (!(Test-Path -LiteralPath $releaseDll)) {
        throw "Release AngelScript.dll was not found: $releaseDll. Build Release first."
    }

    $backupDir = Backup-GameFiles -GameRoot $GameRoot -ValidationDir $validation
    Copy-Item -LiteralPath $output -Destination $gameBase -Force
    Copy-Item -LiteralPath $releaseDll -Destination $gameDll -Force
}

if ($Run) {
    if (!(Test-Path -LiteralPath $player)) {
        throw "Player.exe was not found: $player"
    }

    $process = Start-Process -FilePath $player -WorkingDirectory $bin -PassThru
    $hasExitedAfterWait = $false
    $closedByTool = $false
    $killedByTool = $false
    if (!$KeepRunning) {
        Start-Sleep -Seconds $PlayerSeconds
        $hasExitedAfterWait = $process.HasExited
        if (!$process.HasExited) {
            $closedByTool = $process.CloseMainWindow()
            Start-Sleep -Seconds 2
            $process.Refresh()
        }
        if (!$process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $killedByTool = $true
            Start-Sleep -Milliseconds 500
            $process.Refresh()
        }
    }

    [pscustomobject]@{
        PlayerStartedNoArgs = $true
        WorkingDirectory = $bin
        PlayerPid = $process.Id
        HasExitedAfterWait = $hasExitedAfterWait
        HasExited = $process.HasExited
        ExitCode = if ($process.HasExited) { $process.ExitCode } else { $null }
        ClosedByTool = $closedByTool
        KilledByTool = $killedByTool
        KeptRunning = [bool]$KeepRunning
        BackupDir = $backupDir
        OutputCmo = $output
        StressComponents = $StressComponents
    }
}
