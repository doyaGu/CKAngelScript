# Validation and Troubleshooting

CKAngelScript depends on a native host, so validation is split into build checks, static script checks, exporter checks, and Player startup self-tests.

## Validation Commands

### Local Build

```powershell
tools\Validate-Local.ps1 -BuildDir build -Configuration Release
```

Useful options:

| Option | Meaning |
| --- | --- |
| `-NoSelfTests` | Configure without startup self-test sources. |
| `-ScriptRoot <path>` | Run static runtime script validation after the build. |
| `-RunBallance` | Install the DLL and run Ballance/Player validation. |
| `-BallanceRoot <path>` | Ballance root containing `BuildingBlocks` and `Bin`; optional when env vars or `%USERPROFILE%\Games\Ballance` are available. |
| `-PlayerSeconds <n>` | Maximum seconds to wait for the Player startup self-test marker, or the settle duration when `-NoStartupSelfTests` is used; defaults to 60. |
| `-IncludeLogTail` | Include current-run `AngelScript.log` and `Player.log` tails in successful output. |
| `-LogTailLines <n>` | Number of current-run log lines to include in diagnostics; defaults to 40. |
| `-SkipPlayer` | Run exporter validation without launching Player. |
| `-NoStartupSelfTests` | Launch Player without setting `CKAS_RUN_SELFTESTS`; validation passes if Player stays alive for `-PlayerSeconds`. |
| `-ExportScriptApi <path>` | Start Player and export the initialized live AngelScript API JSON. Incompatible with `-SkipPlayer`. |

### Runtime Script Static Validation

```powershell
tools\Validate-RuntimeScripts.ps1 -ScriptRoot C:\Game\Data
```

This validates manifest syntax, dependency text, source file existence, and lifecycle signatures. Add `-CompileProbe` when a host is available; it sets `CKAS_RUNTIME_VALIDATE_ONLY=1` and compiles discovered scripts without running their lifecycle callbacks.

### Ballance/Player Validation

```powershell
tools\Validate-Ballance.ps1 `
  -BuildDll build\src\Release\AngelScript.dll
```

The script backs up the installed DLL when needed, copies the new DLL, runs `VirtoolsDataExporter.exe`, verifies the `AngelScript Component` and `AngelScript Executor` BBs, starts Player, and actively waits for the startup self-test marker. It keeps the newly installed DLL in `BuildingBlocks`; backups are for manual rollback, not automatic restore.

`BallanceRoot` is resolved from the explicit parameter, `BALLANCE_ROOT`, `CKAS_BALLANCE_ROOT`, or `%USERPROFILE%\Games\Ballance`. Player validation sets `CKAS_SELFTEST_MARKER` and `CKAS_RUN_SELFTESTS=1` only for the launched process and then restores the caller environment. With `-NoStartupSelfTests`, those variables are cleared for the launched Player and the script only verifies that Player remains alive for the requested settle window. The result object includes the installed/source hashes, whether they match, backup path, export JSON path, marker path, wait mode, and Player close/kill status. Successful output omits log tails unless `-IncludeLogTail` is set; failures always include marker content and current-run log tails. Normal installs fail immediately if the copied DLL hash does not match the built DLL; `-SkipInstall` still validates the already installed DLL and reports whether it matches the build.

To export the script API surface from the live engine, build the DLL with `CKAS_ENABLE_API_EXPORT=ON` and then run:

```powershell
tools\Validate-Ballance.ps1 `
  -BallanceRoot $env:BALLANCE_ROOT `
  -BuildDll build\src\Release\AngelScript.dll `
  -ExportScriptApi build\docs\script-api.json
```

The exported JSON is schema version 1 and is intended as the source for generated Natural Docs stubs:

```powershell
python tools\generate_naturaldocs_api.py `
  --api-json build\docs\script-api.json `
  --metadata docs\api-reference.yml `
  --out-dir build\docs\api-source `
  --quality-json build\docs\api-quality.json
```

`docs\api-reference.yml` is the curated reference metadata layer for grouping, summaries, funcdef display declarations, and quality gates. The generated quality report records hidden internals, skipped declarations, group counts, metadata orphans, and required high-level type coverage.

## Environment Variables

| Variable | Used by | Meaning |
| --- | --- | --- |
| `CKAS_RUN_SELFTESTS` | Plugin runtime | Truthy value runs startup self-tests in the host. |
| `CKAS_SELFTEST_MARKER` | Plugin runtime / validation scripts | File path where startup self-test status is written. |
| `CKAS_SCRIPT_ROOTS` | Runtime manager / validation scripts | Semicolon-separated extra runtime script roots. |
| `CKAS_RUNTIME_VALIDATE_ONLY` | Runtime manager | Compile discovered runtime scripts without running lifecycle callbacks. |
| `CKAS_EXPORT_SCRIPT_API` | Plugin runtime / validation scripts | File path where the initialized `asIScriptEngine` API JSON is written. Requires a DLL built with `CKAS_ENABLE_API_EXPORT=ON`. |
| `BALLANCE_ROOT` / `CKAS_BALLANCE_ROOT` | Tools | Default Ballance root. |
| `CKAS_NMO_EXE` | FPS sample tool | Path to `nmo` when injecting sample components. |

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
| `AngelScript.dll` is not listed by exporter | DLL not copied to `BuildingBlocks`, dependency DLL missing, or wrong architecture | Rebuild Win32, install DynCall dependencies, rerun `Validate-Ballance.ps1`. |
| Player starts but marker is missing | Self-tests not built or Player did not load the plugin | Configure with `-DCKAS_BUILD_SELF_TESTS=ON`, confirm plugin location. |
| Marker says `failed` | A startup self-test failed | Read `stage` and `message`, then inspect `AngelScript.log`. |
| Runtime script is not discovered | Wrong root or missing `Scripts` directory | Pass `-ScriptRoot`, set `CKAS_SCRIPT_ROOTS`, or place modules under `Data\Scripts`. |
| A package implementation file such as `base_loading.as` is compiled as its own runtime script | `CKAS_SCRIPT_ROOTS` points at a package directory instead of a root containing `Scripts` | Point the root at the project package root, for example `examples\ballance-logic-port`, or at `examples\ballance-logic-port\Packages` for standalone manifests; do not point at `Scripts\ballance.logic`. |
| Runtime script is discovered but not loaded | Compile error or missing required dependency | Use `Runtime::Info(ctx, id)` and `Runtime::RequiredDependencies(ctx, id)`. |
| Executor reports `Failed to attach runtime script module` | The `Script` setting did not resolve to a loaded runtime script id | Check the full Executor diagnostic: it reports the runtime id value, runtime existence/load/failure state, resolved runtime module, cache state, and engine module state. |
| Component fields are not injected | Metadata field name/type mismatch | Check [component-metadata-manifest.md](component-metadata-manifest.md). |
| Scene ref becomes invalid | CK object was deleted, reset, or id was reused | Reacquire through `Scene::*` and avoid storing raw CK pointers long-term. |
| Patched Ballance file loads to a black screen and the Executor trace never appears | Replacement `CKBehaviorIO.old_flags` do not match authored input/output IO metadata, or the replacement was made targetable without a real target slot | For `nmo`-generated Executor nodes, set input IO `old_flags=1`, output IO `old_flags=2`, and keep the Executor declaration non-targetable unless the original root target behavior is explicitly implemented. |
| Executor trace appears but loading stops at `Get Row=>Error(Execution)` | Rewiring input parameters with a bidirectional `param connect` changed original source-output backrefs | Preserve the original source output object and set only the new `CKParameterIn.source_id`; do not add `destination_count` entries that the pristine graph did not have. |
| `ParamValue.AsInt()` returns `1` or a bool-like value for an integer pin | Older conversion code treated `CKPGUID_INT` as bool storage before checking integer storage | Rebuild with the `ScriptParameterConversion` fix and run startup self-tests; `CKPGUID_INT` must read/write as integer storage, while `CKPGUID_BOOL` remains bool storage. |
| Runtime BB call reports `Building Block 'Activate Script' is not targetable` | `BB::Call(ctx, ...)` inherited the current `CKBehaviorContext` target | Add `Target(null)` for non-targetable BBs, and pass script-name parameters with the original CK parameter GUID, for example `CKPGUID_SCRIPT`. |
| Dynamic pins such as `Add Row` row values are missing or have no names | The BB declaration has no static input parameter list; pins are synthesized after a target and settings are applied | Prefer `BehaviorFrame` one-shot calls: set `Target(RegisterNMO)`, set `Setting("Unique Key Column", true)`, then use `Set("All_Group", group)` and `Set("Load ID", loadId)`. The builder resolves names against the final runtime layout, so do not add `pIn 0` / `pIn 1` fallback slot names in gameplay code. |
| Runtime BB call throws `BBSlot 'Out' layout changed; bind the slot again` | A target-dependent BB callback rebuilt dynamic outputs after a cached `BBSlot@` was captured | Reacquire the live slot from `BBInstance@` after setting changes, target assignment, or execution before calling `OutputActive`, `Set`, or `Source`. |
| Patched loader reaches `MenuLevel.nmo` but active camera stays null or menu camera does not move | The replacement treated `Activate Script` as an immediate BB success output instead of scheduling the script through the current scene | Follow Ballanced `ActivateScript.cpp`: null scripts are successful no-ops, real scripts must call `CKScene::Activate(script, reset)` and wake the owner object. Verify with a camera probe that `MenuLevel_Init` is active and `Cam_MenuLevel` is attached and moving. |
| Menu loads correctly, but selecting Level 1 leads to a black screen | `Load_Object` did not preserve native `Iterator` cursor semantics for the runtime `Loading_Parameter` row, especially `Ending Row=-1` as the inclusive last row | Confirm `unload Menu_Level` receives `Deleted_LoadID`, then inspect `Load_Object` batches. The AS implementation must keep `firstRow`, `nextRow`, `stopRow`, row-count wrap-around, and immediate completion for `Starting Row >= rowCount`; Level 1 should load `Levelinit_Loading` through row 10 and reach `InGameCam`. |
| Loading progress bar enters a level just before reaching the right edge | The loaded-array completion broadcast was sent without preserving original `#358 Load_Object` `Broadcast Message.In` link delay `2`, or CKAS async resume checks advanced delay tasks in addition to scheduler ticks | Use `frame.LinkDelay(2)` for the original two-frame behavior-link delay before AS `<table>_Loaded` broadcasts, and verify `PrepareContextResume` only checks readiness. The regression log should show one idle frame between the last loaded row and the loaded-array broadcast. This affects only batch completion messages, not per-NMO `Loaded` output to registration. |
| Player closes with `__CKASRuntime_*` external-reference warnings after an Executor replacement ran | Executor state still holds script object or method references while runtime modules are unloading | Rebuild with the Executor state-release hook; `ScriptManager` must release Executor runtime states before `Runtime::Clear` / `Runtime::OnEnd`. |

## Build Hygiene

Generated build outputs should stay under `build/`. `src/Version.h` is generated into the build tree and should not be committed.
