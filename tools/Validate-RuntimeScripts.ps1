param(
    [string[]]$ScriptRoot = @(),
    [switch]$CompileProbe,
    [string]$BallanceRoot = "",
    [string]$BuildDll = "",
    [int]$PlayerSeconds = 8
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$lifecycleNames = @(
    "OnLoad",
    "Awake",
    "OnEnable",
    "Start",
    "Update",
    "OnPostLoad",
    "OnPostProcess",
    "OnDisable",
    "OnDestroy",
    "OnReset",
    "OnPause",
    "OnResume"
)

function Split-ListText {
    param([string]$Text)
    if ([string]::IsNullOrWhiteSpace($Text)) {
        return @()
    }
    return @($Text -split "[;,]" | ForEach-Object { $_.Trim(" `t`r`n'`"") } | Where-Object { $_ })
}

function Read-Metadata {
    param([string]$Path)
    $text = Get-Content -LiteralPath $Path -Raw
    $metadata = @{}
    foreach ($match in [regex]::Matches($text, "\[script(?:\.[^\]\s]+)?(?<body>(?:[^\]`"']+|`"[^`"]*`"|'[^']*')*)\]")) {
        foreach ($pair in [regex]::Matches($match.Groups["body"].Value, "(?<key>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*(?:(?<dq>`"[^`"]*`")|(?<sq>'[^']*')|(?<bare>\S+))")) {
            $key = $pair.Groups["key"].Value.ToLowerInvariant()
            $value = if ($pair.Groups["dq"].Success) {
                $pair.Groups["dq"].Value.Trim("`"")
            } elseif ($pair.Groups["sq"].Success) {
                $pair.Groups["sq"].Value.Trim("'")
            } else {
                $pair.Groups["bare"].Value
            }
            $metadata[$key] = $value
        }
    }
    return $metadata
}

function Resolve-ScriptsDir {
    param([string]$Root)
    $resolved = (Resolve-Path -LiteralPath $Root).Path
    if ((Split-Path -Leaf $resolved) -ieq "Scripts") {
        return $resolved
    }
    $scripts = Join-Path $resolved "Scripts"
    if (Test-Path -LiteralPath $scripts -PathType Container) {
        return (Resolve-Path -LiteralPath $scripts).Path
    }
    return $resolved
}

function Get-RuntimeCandidates {
    param([string]$ScriptsDir)
    $selfManifest = Join-Path $ScriptsDir "script.as"
    if (Test-Path -LiteralPath $selfManifest -PathType Leaf) {
        return @($selfManifest)
    }
    $candidates = @()
    Get-ChildItem -LiteralPath $ScriptsDir -Force | ForEach-Object {
        if ($_.PSIsContainer) {
            if ($_.Name.StartsWith(".") -or $_.Name.StartsWith("_")) {
                return
            }
            $manifest = Join-Path $_.FullName "script.as"
            if (Test-Path -LiteralPath $manifest -PathType Leaf) {
                $candidates += $manifest
            }
        } elseif ($_.Extension -ieq ".as" -and $_.Name -notlike "*.inc.as" -and $_.Name -notlike "*.include.as") {
            $candidates += $_.FullName
        }
    }
    return $candidates | Sort-Object
}

function Test-LifecycleSignatures {
    param(
        [string[]]$Files,
        [string]$ScriptId
    )
    $errors = @()
    foreach ($file in $Files) {
        $text = Get-Content -LiteralPath $file -Raw
        foreach ($name in $lifecycleNames) {
            foreach ($match in [regex]::Matches($text, "void\s+$name\s*\((?<args>[^)]*)\)")) {
                $args = $match.Groups["args"].Value.Trim()
                if ($args -ne "const ScriptRuntimeContext &in ctx") {
                    $errors += "[$ScriptId] $file has invalid $name signature. Expected: void $name(const ScriptRuntimeContext &in ctx)"
                }
            }
        }
    }
    return $errors
}

$roots = @()
if ($ScriptRoot.Count -gt 0) {
    $roots = $ScriptRoot
} elseif ($env:CKAS_SCRIPT_ROOTS) {
    $roots = Split-ListText $env:CKAS_SCRIPT_ROOTS
}
if ($roots.Count -eq 0) {
    throw "No runtime script root was provided. Pass -ScriptRoot or set CKAS_SCRIPT_ROOTS."
}

$errors = @()
$seenIds = @{}
$scriptCount = 0
$runtimeRoots = @()

foreach ($root in $roots) {
    $scriptsDir = Resolve-ScriptsDir $root
    $runtimeRoots += if ((Split-Path -Leaf $scriptsDir) -ieq "Scripts") { Split-Path -Parent $scriptsDir } else { $root }
    if (!(Test-Path -LiteralPath $scriptsDir -PathType Container)) {
        $errors += "Scripts directory not found: $scriptsDir"
        continue
    }
    foreach ($manifest in Get-RuntimeCandidates $scriptsDir) {
        $scriptCount++
        $meta = Read-Metadata $manifest
        $isDirectoryManifest = (Split-Path -Leaf $manifest) -ieq "script.as"
        $id = if ($meta.ContainsKey("id")) {
            $meta["id"]
        } elseif ($isDirectoryManifest) {
            Split-Path -Leaf (Split-Path -Parent $manifest)
        } else {
            [System.IO.Path]::GetFileNameWithoutExtension($manifest)
        }
        if ($seenIds.ContainsKey($id)) {
            $errors += "Duplicate runtime script id '$id': $manifest and $($seenIds[$id])"
        } else {
            $seenIds[$id] = $manifest
        }

        $rootDir = Split-Path -Parent $manifest
        $entry = if ($meta.ContainsKey("entry")) { Join-Path $rootDir $meta["entry"] } else { $manifest }
        $files = @($entry)
        if ($meta.ContainsKey("files")) {
            foreach ($file in Split-ListText $meta["files"]) {
                $files += Join-Path $rootDir $file
            }
        }
        foreach ($file in $files | Select-Object -Unique) {
            if (!(Test-Path -LiteralPath $file -PathType Leaf)) {
                $errors += "[$id] source file missing: $file"
            }
        }
        foreach ($key in @("depends", "required", "optional")) {
            if ($meta.ContainsKey($key)) {
                foreach ($dep in Split-ListText $meta[$key]) {
                    if ($dep -notmatch "^[A-Za-z0-9_.-]+(\s*(==|=|>=|<=|>|<)\s*[0-9][A-Za-z0-9_.-]*)?$") {
                        $errors += "[$id] invalid dependency spec '$dep'"
                    }
                }
            }
        }
        $existingFiles = @($files | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -Unique)
        $errors += Test-LifecycleSignatures -Files $existingFiles -ScriptId $id
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ -ErrorAction Continue }
    throw "Runtime script validation failed with $($errors.Count) error(s)."
}

if ($CompileProbe) {
    if ([string]::IsNullOrWhiteSpace($BallanceRoot)) {
        $BallanceRoot = if ($env:BALLANCE_ROOT) { $env:BALLANCE_ROOT } else { $env:CKAS_BALLANCE_ROOT }
    }
    if ([string]::IsNullOrWhiteSpace($BuildDll)) {
        $BuildDll = Join-Path $repoRoot "build\src\Release\AngelScript.dll"
    }
    if ([string]::IsNullOrWhiteSpace($BallanceRoot)) {
        throw "Compile probe requires -BallanceRoot or BALLANCE_ROOT/CKAS_BALLANCE_ROOT."
    }
    $oldRoots = $env:CKAS_SCRIPT_ROOTS
    $oldValidate = $env:CKAS_RUNTIME_VALIDATE_ONLY
    $env:CKAS_SCRIPT_ROOTS = ($runtimeRoots | Select-Object -Unique) -join ";"
    $env:CKAS_RUNTIME_VALIDATE_ONLY = "1"
    try {
        & (Join-Path $PSScriptRoot "Validate-Ballance.ps1") -BallanceRoot $BallanceRoot -BuildDll $BuildDll -PlayerSeconds $PlayerSeconds
    } finally {
        if ($null -eq $oldRoots) { Remove-Item Env:\CKAS_SCRIPT_ROOTS -ErrorAction SilentlyContinue } else { $env:CKAS_SCRIPT_ROOTS = $oldRoots }
        if ($null -eq $oldValidate) { Remove-Item Env:\CKAS_RUNTIME_VALIDATE_ONLY -ErrorAction SilentlyContinue } else { $env:CKAS_RUNTIME_VALIDATE_ONLY = $oldValidate }
    }
}

[pscustomobject]@{
    RuntimeScripts = $scriptCount
    Roots = ($roots -join ";")
    CompileProbe = [bool]$CompileProbe
    Status = "ok"
}
