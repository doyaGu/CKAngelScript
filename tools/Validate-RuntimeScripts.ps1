param(
    [string[]]$ScriptRoot = @()
)

$ErrorActionPreference = "Stop"

$scriptEntryArgs = @{}
if ($ScriptRoot.Count -gt 0) {
    $scriptEntryArgs.ScriptRoot = $ScriptRoot
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
