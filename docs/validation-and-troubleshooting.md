# Validation and Troubleshooting

CKAngelScript depends on a native host, so core validation is split into build
checks and static script checks. Exporter and Player automation is supplied by
host integration projects.

## Validation Commands

### Local Build

```powershell
tools\Validate-Local.ps1 -BuildDir build -Configuration Release
```

Useful options:

| Option | Meaning |
| --- | --- |
| `-NoSelfTests` | Configure without startup self-test sources. |
| `-ScriptRoot <path>` | Run static script entry validation after the build. |

### Runtime Script Static Validation

```powershell
tools\Validate-ScriptEntries.ps1 -ScriptRoot C:\Game\Data
```

This validates manifest syntax, dependency text, source file existence, and
lifecycle signatures. A host integration can additionally set
`CKAS_SCRIPT_VALIDATE_ONLY=1` before launching the host to compile discovered
entries without running their lifecycle callbacks.

An exported API JSON (schema version 1) can be converted to Natural Docs stubs:

```powershell
python tools\generate_naturaldocs_api.py `
  --api-json build\docs\script-api.json `
  --metadata docs\api-reference.yml `
  --out-dir build\docs\api-source `
  --quality-json build\docs\api-quality.json
```

`docs\api-reference.yml` is the curated reference metadata layer for grouping, summaries, funcdef display declarations, and quality gates. The generated quality report records hidden internals, skipped declarations, group counts, metadata orphans, and required high-level type coverage.

### Refresh The Tracked Live API Catalog

Build the DLL with `CKAS_ENABLE_API_EXPORT=ON`, set `CKAS_EXPORT_SCRIPT_API` to
an output path, then launch a real Virtools host that loads the DLL. After the
initialized engine writes the export, refresh the tracked catalog and derived
language-server file:

```powershell
tools\Refresh-ApiCatalog.ps1 -LiveApiJson build\docs\script-api.live.json
```

For a local Player installation, the export/restore/refresh cycle can be
automated. The script refuses to proceed when Player is already running and
restores the previously installed plugin in a `finally` block:

```powershell
tools\Export-LiveApiCatalog.ps1 `
  -PlayerPath C:\Game\Bin\Player.exe `
  -GameRoot C:\Game `
  -PluginDll build-api\src\AngelScript.dll `
  -PluginDestination C:\Game\BuildingBlocks\AngelScript.dll `
  -Output build-api\docs\script-api.live.json `
  -RunSelfTests `
  -SelfTestMarker build-api\docs\script-api.live.selftest `
  -Refresh
```

The refresh normalizes the live export, removes volatile process addresses,
writes `docs/script-api.json`, regenerates `as.predefined`, and records a hash
of every source file that contributes AngelScript registrations in
`docs/script-api.lock.json`. `-RunSelfTests` also waits for the startup marker
to reach `status=ok`; the installed plugin is restored on success, failure, or
timeout.

Run the same consistency check used by CI with:

```powershell
python tools\sync_api_catalog.py --check
```

If a registration source changes without a fresh live export, or a generator
change makes `as.predefined` stale, the check fails and lists the drifting
files. The live export must come from the canonical release configuration,
including `CKAS_ENABLE_DYNCALL=ON` when the published API includes DynCall.

## Environment Variables

| Variable | Used by | Meaning |
| --- | --- | --- |
| `CKAS_RUN_SELFTESTS` | Plugin runtime | Truthy value runs startup self-tests in the host. |
| `CKAS_SELFTEST_MARKER` | Plugin runtime / validation scripts | File path where startup self-test status is written. |
| `CKAS_SCRIPT_ROOTS` | Script entry manager / validation scripts | Semicolon-separated extra script entry roots. |
| `CKAS_SCRIPT_VALIDATE_ONLY` | Script entry manager | Compile discovered script entries without running runtime lifecycle callbacks. |
| `CKAS_RUNTIME_VALIDATE_ONLY` | Script entry manager | Legacy alias for `CKAS_SCRIPT_VALIDATE_ONLY`. |
| `CKAS_EXPORT_SCRIPT_API` | Plugin runtime / validation scripts | File path where the initialized `asIScriptEngine` API JSON is written. Requires a DLL built with `CKAS_ENABLE_API_EXPORT=ON`. |
| `CKAS_DISABLE_CK2_PROTOTYPE_HOOK` | Plugin runtime | `1`, `true`, `yes`, or `on` (case-insensitive) disables CKAS' `CreateCKBehaviorPrototypeRunTime` compatibility hook. Use only when isolating loader issues. |

## Marker Files

When `CKAS_SELFTEST_MARKER` is set, startup self-tests write:

```text
status=running|ok|failed
stage=<current-stage>
message=<failure-text>
```

Expected success:

```text
status=ok
stage=complete
```

If validation closes or kills Player after marker success, the self-tests still passed; the script closes Player as soon as `status=ok` is observed and may force-kill if the window does not exit quickly. If the marker reports `failed`, Player exits early, or the timeout expires, the thrown error includes marker content plus the log tails written during the current validation run.

## Registration Failures

AngelScript registration failures are checked in Release and Debug builds. If a binding fails during startup, CKAngelScript writes a summary to the Virtools console and `AngelScript.log`, then releases the partially initialized engine.

Typical causes:

- Virtools SDK version mismatch.
- Changed native method signature or calling convention.
- Type registered in the wrong order.
- Duplicate or invalid AngelScript declaration.

Start with the first failure in the summary; later failures often cascade from it.

## Common Problems

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `AngelScript.dll` is not listed by a host exporter | DLL not copied to `BuildingBlocks`, dependency DLL missing, or wrong architecture | Rebuild Win32, install native dependencies, and use the host integration's validation command. |
| Player starts but marker is missing | Self-tests not built or Player did not load the plugin | Configure with `-DCKAS_BUILD_SELF_TESTS=ON`, confirm plugin location. |
| Marker says `failed` | A startup self-test failed | Read `stage` and `message`, then inspect `AngelScript.log`. |
| Script entry is not discovered | Wrong root or missing `Scripts` directory | Pass `-ScriptRoot`, set `CKAS_SCRIPT_ROOTS`, or place modules under `Data\Scripts`. |
| A shared source file is compiled as its own script entry | `CKAS_SCRIPT_ROOTS` points at an implementation directory instead of a root containing `Scripts`, or the file has an accidental main `[runtime]`, `[executor]`, or `[script]` block | Point the root at the package root. Shared libraries should live outside `Scripts`, for example under `Libs`, and should not contain a main entry block. |
| Script entry is discovered but not loaded | Compile error or missing required dependency | Use `Runtime::Info(ctx, id)` and `Runtime::RequiredDependencies(ctx, id)`. |
| Executor reports `Failed to attach script module` | The `Script` setting did not resolve to a loaded script module id | Check the full Executor diagnostic: it reports the script id value, module existence/load/failure state, host, resolved module, cache state, and engine module state. |
| Component reports `Failed to attach component script` | The `Script` input did not resolve to a loaded module name or script entry id, or the resolved entry failed to load | Check the Component diagnostic: it reports whether an entry was found, the resolved module name when applicable, and entry host/load/failure state. |
| Component fields are not injected | Metadata field name/type mismatch | Check [component-metadata-manifest.md](component-metadata-manifest.md). |
| Scene ref becomes invalid | CK object was deleted, reset, or id was reused | Reacquire through `Scene::*` and avoid storing raw CK pointers long-term. |
| `ParamValue.AsInt()` returns `1` or a bool-like value for an integer pin | Older conversion code treated `CKPGUID_INT` as bool storage before checking integer storage | Rebuild with the `ScriptParameterConversion` fix and run startup self-tests; `CKPGUID_INT` must read/write as integer storage, while `CKPGUID_BOOL` remains bool storage. |
| Runtime BB call reports `Building Block 'Activate Script' is not targetable` | `BB::Call(ctx, ...)` inherited the current `CKBehaviorContext` target | Add `Target(null)` for non-targetable BBs, and pass script-name parameters with the original CK parameter GUID, for example `CKPGUID_SCRIPT`. |
| Dynamic pins such as `Add Row` row values are missing or have no names | The BB declaration has no static input parameter list; pins are synthesized after a target and settings are applied | Prefer `BehaviorFrame` one-shot calls: set `Target(RegisterNMO)`, set `Setting("Unique Key Column", true)`, then use `Set("All_Group", group)` and `Set("Load ID", loadId)`. The builder resolves names against the final runtime layout, so do not add `pIn 0` / `pIn 1` fallback slot names in gameplay code. |
| Runtime BB call throws `BBSlot 'Out' layout changed; bind the slot again` | A target-dependent BB callback rebuilt dynamic outputs after a cached `BBSlot@` was captured | Reacquire the live slot from `BBInstance@` after setting changes, target assignment, or execution before calling `OutputActive`, `Set`, or `Source`. |
| Runtime-created BB prototypes have missing or invalid layouts | Virtools' `CreateCKBehaviorPrototypeRunTime` path diverges from the normal prototype constructor in this host | CKAS installs a process-local compatibility hook that redirects the runtime export to `CreateCKBehaviorPrototype`. Leave it enabled for script-created BB prototypes. The hook is sticky by default and tolerates later same-target hooks such as BML+. |
| Player closes with `__CKASRuntime_*` external-reference warnings after an Executor replacement ran | Executor state still holds script object or method references while script modules are unloading | Rebuild with the Executor state-release hook; `ScriptManager` must release Executor states before module clear/end. |

## Build Hygiene

Generated build outputs should stay under `build/`. `src/Version.h` is generated into the build tree and should not be committed.
