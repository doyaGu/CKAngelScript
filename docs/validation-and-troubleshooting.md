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
| `-PlayerSeconds <n>` | Maximum seconds to wait for the Player startup self-test marker; defaults to 60 and returns early on success. |
| `-IncludeLogTail` | Include current-run `AngelScript.log` and `Player.log` tails in successful output. |
| `-LogTailLines <n>` | Number of current-run log lines to include in diagnostics; defaults to 40. |
| `-SkipPlayer` | Run exporter validation without launching Player. |

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

The script backs up the installed DLL when needed, copies the new DLL, runs `VirtoolsDataExporter.exe`, verifies the `AngelScript Component` BB, starts Player, and actively waits for the startup self-test marker. It keeps the newly installed DLL in `BuildingBlocks`; backups are for manual rollback, not automatic restore.

`BallanceRoot` is resolved from the explicit parameter, `BALLANCE_ROOT`, `CKAS_BALLANCE_ROOT`, or `%USERPROFILE%\Games\Ballance`. Player validation sets `CKAS_SELFTEST_MARKER` and `CKAS_RUN_SELFTESTS=1` only for the launched process and then restores the caller environment. The result object includes the installed/source hashes, whether they match, backup path, export JSON path, marker path, and Player close/kill status. Successful output omits log tails unless `-IncludeLogTail` is set; failures always include marker content and current-run log tails. Normal installs fail immediately if the copied DLL hash does not match the built DLL; `-SkipInstall` still validates the already installed DLL and reports whether it matches the build.

## Environment Variables

| Variable | Used by | Meaning |
| --- | --- | --- |
| `CKAS_RUN_SELFTESTS` | Plugin runtime | Truthy value runs startup self-tests in the host. |
| `CKAS_SELFTEST_MARKER` | Plugin runtime / validation scripts | File path where startup self-test status is written. |
| `CKAS_SCRIPT_ROOTS` | Runtime manager / validation scripts | Semicolon-separated extra runtime script roots. |
| `CKAS_RUNTIME_VALIDATE_ONLY` | Runtime manager | Compile discovered runtime scripts without running lifecycle callbacks. |
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
| Runtime script is discovered but not loaded | Compile error or missing required dependency | Use `Runtime::Info(ctx, id)` and `Runtime::RequiredDependencies(ctx, id)`. |
| Component fields are not injected | Metadata field name/type mismatch | Check [component-metadata-manifest.md](component-metadata-manifest.md). |
| Scene ref becomes invalid | CK object was deleted, reset, or id was reused | Reacquire through `Scene::*` and avoid storing raw CK pointers long-term. |

## Build Hygiene

Generated build outputs should stay under `build/`. `src/Version.h` is generated into the build tree and should not be committed.
