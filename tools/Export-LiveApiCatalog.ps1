param(
    [Parameter(Mandatory = $true)]
    [string]$PlayerPath,
    [Parameter(Mandatory = $true)]
    [string]$GameRoot,
    [Parameter(Mandatory = $true)]
    [string]$PluginDll,
    [Parameter(Mandatory = $true)]
    [string]$PluginDestination,
    [string]$Output = "",
    [int]$TimeoutSeconds = 60,
    [switch]$RunSelfTests,
    [string]$SelfTestMarker = "",
    [switch]$Refresh
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = Join-Path $repoRoot "build\docs\script-api.live.json"
}

$resolvedPlayer = (Resolve-Path -LiteralPath $PlayerPath).Path
$resolvedGameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$resolvedPlugin = (Resolve-Path -LiteralPath $PluginDll).Path
$resolvedDestination = (Resolve-Path -LiteralPath $PluginDestination).Path
$resolvedOutput = [System.IO.Path]::GetFullPath($Output)
$outputParent = Split-Path -Parent $resolvedOutput
New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
$resolvedSelfTestMarker = ""
if ($RunSelfTests) {
    if ([string]::IsNullOrWhiteSpace($SelfTestMarker)) {
        $SelfTestMarker = $resolvedOutput + ".selftest"
    }
    $resolvedSelfTestMarker = [System.IO.Path]::GetFullPath($SelfTestMarker)
    $markerParent = Split-Path -Parent $resolvedSelfTestMarker
    New-Item -ItemType Directory -Force -Path $markerParent | Out-Null
}

$existingPlayer = Get-Process -Name Player -ErrorAction SilentlyContinue
if ($existingPlayer) {
    throw "Player.exe is already running; refusing to replace a loaded plugin."
}

$backup = Join-Path ([System.IO.Path]::GetTempPath()) ("CKAngelScript-catalog-{0}.dll" -f [guid]::NewGuid().ToString("N"))
$player = $null
Copy-Item -LiteralPath $resolvedDestination -Destination $backup
try {
    Copy-Item -LiteralPath $resolvedPlugin -Destination $resolvedDestination -Force
    Remove-Item -LiteralPath $resolvedOutput -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ($resolvedOutput + ".stage") -Force -ErrorAction SilentlyContinue
    if ($RunSelfTests) {
        Remove-Item -LiteralPath $resolvedSelfTestMarker -Force -ErrorAction SilentlyContinue
        $env:CKAS_SELFTEST_MARKER = $resolvedSelfTestMarker
        $env:CKAS_SELFTEST_PLAYER_LIFECYCLE = "1"
    }
    $env:CKAS_EXPORT_SCRIPT_API = $resolvedOutput
    $player = Start-Process -FilePath $resolvedPlayer -WorkingDirectory $resolvedGameRoot -WindowStyle Hidden -PassThru

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $selfTestStatus = ""
    do {
        $exportReady = Test-Path -LiteralPath $resolvedOutput
        if ($RunSelfTests -and (Test-Path -LiteralPath $resolvedSelfTestMarker)) {
            $markerText = Get-Content -LiteralPath $resolvedSelfTestMarker -Raw
            if ($markerText -match '(?m)^status=(ok|failed)\r?$') {
                $selfTestStatus = $Matches[1]
            }
        }
        if ($exportReady -and (-not $RunSelfTests -or $selfTestStatus -in @("ok", "failed"))) {
            break
        }
        $player.Refresh()
        if ($player.HasExited) {
            break
        }
        Start-Sleep -Milliseconds 250
    } while ([DateTime]::UtcNow -lt $deadline)

    if (-not (Test-Path -LiteralPath $resolvedOutput)) {
        $stagePath = $resolvedOutput + ".stage"
        $stage = if (Test-Path -LiteralPath $stagePath) { Get-Content -LiteralPath $stagePath -Raw } else { "<none>" }
        throw "Player did not produce the live API export. Exited=$($player.HasExited), stage=$stage"
    }
    if ($RunSelfTests -and $selfTestStatus -ne "ok") {
        $markerText = if (Test-Path -LiteralPath $resolvedSelfTestMarker) {
            Get-Content -LiteralPath $resolvedSelfTestMarker -Raw
        } else {
            "<missing>"
        }
        throw "Player startup self-tests did not pass. Exited=$($player.HasExited), marker=$markerText"
    }

    if ($Refresh) {
        & (Join-Path $PSScriptRoot "Refresh-ApiCatalog.ps1") -LiveApiJson $resolvedOutput
        if ($LASTEXITCODE -ne 0) {
            throw "Catalog refresh failed with exit code $LASTEXITCODE."
        }
    }

    Get-Item -LiteralPath $resolvedOutput
} finally {
    Remove-Item Env:CKAS_EXPORT_SCRIPT_API -ErrorAction SilentlyContinue
    Remove-Item Env:CKAS_SELFTEST_MARKER -ErrorAction SilentlyContinue
    Remove-Item Env:CKAS_SELFTEST_PLAYER_LIFECYCLE -ErrorAction SilentlyContinue
    if ($player -and -not $player.HasExited) {
        Stop-Process -Id $player.Id -Force -ErrorAction SilentlyContinue
        [void]$player.WaitForExit(10000)
    }
    $restored = $false
    for ($attempt = 0; $attempt -lt 40 -and -not $restored; $attempt++) {
        try {
            Copy-Item -LiteralPath $backup -Destination $resolvedDestination -Force
            $restored = $true
        } catch [System.IO.IOException] {
            Start-Sleep -Milliseconds 250
        }
    }
    if ($restored) {
        Remove-Item -LiteralPath $backup -Force -ErrorAction SilentlyContinue
    } else {
        throw "Could not restore the installed plugin. Backup remains at $backup"
    }
}
