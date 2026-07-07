param(
    [string[]]$ScriptRoot = @(),
    [switch]$CompileProbe,
    [string]$BallanceRoot = "",
    [string]$BuildDll = "",
    [int]$PlayerSeconds = 8
)

$ErrorActionPreference = "Stop"

$scriptEntryArgs = @{
    CompileProbe = [bool]$CompileProbe
    PlayerSeconds = $PlayerSeconds
}
if ($ScriptRoot.Count -gt 0) {
    $scriptEntryArgs.ScriptRoot = $ScriptRoot
}
if (-not [string]::IsNullOrWhiteSpace($BallanceRoot)) {
    $scriptEntryArgs.BallanceRoot = $BallanceRoot
}
if (-not [string]::IsNullOrWhiteSpace($BuildDll)) {
    $scriptEntryArgs.BuildDll = $BuildDll
}

$result = & (Join-Path $PSScriptRoot "Validate-ScriptEntries.ps1") @scriptEntryArgs
foreach ($item in @($result)) {
    if ($null -ne $item -and
        $null -ne $item.PSObject.Properties["ScriptEntries"] -and
        $null -eq $item.PSObject.Properties["RuntimeScripts"]) {
        $item | Add-Member -NotePropertyName RuntimeScripts -NotePropertyValue $item.ScriptEntries
    }
    $item
}
