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
    foreach ($match in [regex]::Matches($text, "\[(?<kind>(?:script|runtime|executor)(?:\.[^\]\s]+)?)(?<body>(?:[^\]`"']+|`"[^`"]*`"|'[^']*')*)\]")) {
        $kind = $match.Groups["kind"].Value.ToLowerInvariant()
        if ($kind -in @("script", "runtime", "executor")) {
            $metadata["__entry"] = "true"
        }
        if ($kind -in @("runtime", "executor") -and !$metadata.ContainsKey("host")) {
            $metadata["host"] = $kind
        }
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

function ConvertTo-ScriptEntryId {
    param([string]$Text)
    $id = $Text -replace "[^A-Za-z0-9_.-]", "_"
    if ([string]::IsNullOrWhiteSpace($id)) {
        return "script"
    }
    return $id
}

function Get-RelativePathText {
    param(
        [string]$Base,
        [string]$Path
    )
    $basePath = (Resolve-Path -LiteralPath $Base).Path.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    $pathInfo = Resolve-Path -LiteralPath $Path
    $baseUri = [System.Uri]::new($basePath)
    $pathUri = [System.Uri]::new($pathInfo.Path)
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($pathUri).ToString()).Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

function Get-ScriptEntryId {
    param(
        [string]$ScriptsDir,
        [string]$Manifest,
        [hashtable]$Metadata
    )
    if ($Metadata.ContainsKey("id")) {
        return ConvertTo-ScriptEntryId $Metadata["id"]
    }

    $isDirectoryManifest = (Split-Path -Leaf $Manifest) -ieq "script.as"
    $subject = if ($isDirectoryManifest) { Split-Path -Parent $Manifest } else { $Manifest }
    $relative = Get-RelativePathText -Base $ScriptsDir -Path $subject
    if ([string]::IsNullOrWhiteSpace($relative) -or $relative -eq "." -or $relative.StartsWith("..")) {
        $relative = if ($isDirectoryManifest) {
            Split-Path -Leaf $subject
        } else {
            [System.IO.Path]::GetFileNameWithoutExtension($Manifest)
        }
    }

    $parts = @($relative -split "[\\/]" | Where-Object { $_ -and $_ -ne "." -and $_ -ne ".." })
    if (!$isDirectoryManifest -and $parts.Count -gt 0) {
        $parts[$parts.Count - 1] = [System.IO.Path]::GetFileNameWithoutExtension($parts[$parts.Count - 1])
    }
    return ConvertTo-ScriptEntryId ($parts -join ".")
}

function Get-ScriptEntryCandidates {
    param([string]$ScriptsDir)
    $selfManifest = Join-Path $ScriptsDir "script.as"
    if (Test-Path -LiteralPath $selfManifest -PathType Leaf) {
        return @($selfManifest)
    }
    $candidates = @()
    $pending = New-Object System.Collections.Generic.List[string]
    $pending.Add($ScriptsDir)
    while ($pending.Count -gt 0) {
        $current = $pending[$pending.Count - 1]
        $pending.RemoveAt($pending.Count - 1)
        Get-ChildItem -LiteralPath $current -Force | ForEach-Object {
            if ($_.PSIsContainer) {
                if ($_.Name.StartsWith(".") -or $_.Name.StartsWith("_")) {
                    return
                }
                $manifest = Join-Path $_.FullName "script.as"
                if (Test-Path -LiteralPath $manifest -PathType Leaf) {
                    $candidates += $manifest
                } else {
                    $pending.Add($_.FullName)
                }
            } elseif ($_.Extension -ieq ".as" -and $_.Name -notlike "*.inc.as" -and $_.Name -notlike "*.include.as") {
                $text = Get-Content -LiteralPath $_.FullName -Raw
                if ($text -match "\[(script|runtime|executor)(?=\s|\])") {
                    $candidates += $_.FullName
                }
            }
        }
    }
    return $candidates | Sort-Object
}

function Test-LifecycleSignatures {
    param(
        [string[]]$Files,
        [string]$ScriptId,
        [string]$EntryHost
    )
    $errors = @()
    if ($EntryHost -ne "runtime") {
        return $errors
    }
    foreach ($file in $Files) {
        $text = Get-Content -LiteralPath $file -Raw
        foreach ($name in $lifecycleNames) {
            foreach ($match in [regex]::Matches($text, "void\s+$name\s*\((?<args>[^)]*)\)")) {
                $args = $match.Groups["args"].Value.Trim()
                if ($args -ne "const ScriptContext &in ctx") {
                    $errors += "[$ScriptId] $file has invalid $name signature. Expected: void $name(const ScriptContext &in ctx)"
                }
            }
        }
        foreach ($match in [regex]::Matches($text, "void\s+OnMessage\s*\((?<args>[^)]*)\)")) {
            $args = $match.Groups["args"].Value.Trim()
            if ($args -ne "const ScriptMessage &in msg, const ScriptContext &in ctx") {
                $errors += "[$ScriptId] $file has invalid OnMessage signature. Expected: void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx)"
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
    throw "No script entry root was provided. Pass -ScriptRoot or set CKAS_SCRIPT_ROOTS."
}

$errors = @()
$seenIds = @{}
$scriptCount = 0
$scriptEntryRoots = @()

foreach ($root in $roots) {
    $scriptsDir = Resolve-ScriptsDir $root
    $scriptEntryRoots += if ((Split-Path -Leaf $scriptsDir) -ieq "Scripts") { Split-Path -Parent $scriptsDir } else { $root }
    if (!(Test-Path -LiteralPath $scriptsDir -PathType Container)) {
        $errors += "Scripts directory not found: $scriptsDir"
        continue
    }
    foreach ($manifest in Get-ScriptEntryCandidates $scriptsDir) {
        $meta = Read-Metadata $manifest
        if (!$meta.ContainsKey("__entry")) {
            $errors += "[$manifest] missing main [runtime], [executor], or legacy [script] metadata"
            continue
        }
        $scriptCount++
        $entryHost = if ($meta.ContainsKey("host")) { $meta["host"].ToLowerInvariant() } else { "runtime" }
        if ($entryHost -notin @("runtime", "executor")) {
            $errors += "[$manifest] unsupported script host '$entryHost'"
        }
        $id = Get-ScriptEntryId -ScriptsDir $scriptsDir -Manifest $manifest -Metadata $meta
        if ($seenIds.ContainsKey($id)) {
            $errors += "Duplicate script entry id '$id': $manifest and $($seenIds[$id])"
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
        if ($meta.ContainsKey("topics")) {
            foreach ($topic in Split-ListText $meta["topics"]) {
                if ($topic -notmatch "^[A-Za-z0-9_.:-]+$") {
                    $errors += "[$id] invalid message topic '$topic'"
                }
            }
        }
        $existingFiles = @($files | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -Unique)
        $errors += Test-LifecycleSignatures -Files $existingFiles -ScriptId $id -EntryHost $entryHost
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ -ErrorAction Continue }
    throw "Script entry validation failed with $($errors.Count) error(s)."
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
    $oldValidate = $env:CKAS_SCRIPT_VALIDATE_ONLY
    $env:CKAS_SCRIPT_ROOTS = ($scriptEntryRoots | Select-Object -Unique) -join ";"
    $env:CKAS_SCRIPT_VALIDATE_ONLY = "1"
    try {
        & (Join-Path $PSScriptRoot "Validate-Ballance.ps1") -BallanceRoot $BallanceRoot -BuildDll $BuildDll -PlayerSeconds $PlayerSeconds
    } finally {
        if ($null -eq $oldRoots) { Remove-Item Env:\CKAS_SCRIPT_ROOTS -ErrorAction SilentlyContinue } else { $env:CKAS_SCRIPT_ROOTS = $oldRoots }
        if ($null -eq $oldValidate) { Remove-Item Env:\CKAS_SCRIPT_VALIDATE_ONLY -ErrorAction SilentlyContinue } else { $env:CKAS_SCRIPT_VALIDATE_ONLY = $oldValidate }
    }
}

[pscustomobject]@{
    ScriptEntries = $scriptCount
    Roots = ($roots -join ";")
    CompileProbe = [bool]$CompileProbe
    Status = "ok"
}
