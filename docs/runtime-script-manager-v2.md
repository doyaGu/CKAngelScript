# Runtime Script Manager v2

Runtime scripts are long-lived AngelScript modules discovered from Virtools data paths. The runtime scans each Virtools data path `Scripts` directory, then each root listed in `CKAS_SCRIPT_ROOTS`. If an environment root is not already named `Scripts`, its `Scripts` child is scanned when present.

## Layout

Directory modules use `script.as` as the manifest:

```angelscript
[script id="tools.debug" name="Debug Tools" version="1.0.0" class="DebugTools" entry="runtime.as"]
[script description="Runtime diagnostics" author="Team" category="Tools" tags="debug;runtime"]
[script.depends required="core>=1.0.0" optional="overlay"]
[script.messages topics="game.ready;ui.changed"]
```

Single-file `.as` modules are still valid. For directory modules, `entry` selects the first compiled file and `files` adds more sources relative to the manifest directory. `id` defaults to the directory name for `script.as` manifests and to the file stem for single-file modules. `name`, `version`, `class`, `target`, `enabled`, description, author, category, and tags are read as metadata and surfaced through runtime inspection.

## Dependencies

`[script.depends]` supports `required` and `optional` dependency lists. Entries are semicolon- or comma-separated ids with an optional version constraint such as `core>=1.0.0`. Required dependencies must resolve before the script can load; optional dependencies are reported but do not block loading when absent.

Dependency status is available through `Runtime::RequiredDependencies(ctx, id)` and `Runtime::OptionalDependencies(ctx, id)`. Use these APIs for diagnostics instead of reparsing manifest text in scripts.

## Lifecycle

Runtime lifecycle callbacks must use the explicit context signature:

```angelscript
void OnLoad(const ScriptContext &in ctx) {}
void Awake(const ScriptContext &in ctx) {}
void OnEnable(const ScriptContext &in ctx) {}
void Start(const ScriptContext &in ctx) {}
void Update(const ScriptContext &in ctx) {}
void OnPostLoad(const ScriptContext &in ctx) {}
void OnPostProcess(const ScriptContext &in ctx) {}
void OnDisable(const ScriptContext &in ctx) {}
void OnDestroy(const ScriptContext &in ctx) {}
void OnReset(const ScriptContext &in ctx) {}
void OnPause(const ScriptContext &in ctx) {}
void OnResume(const ScriptContext &in ctx) {}
void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx) {}
```

Parameterless lifecycle functions are invalid in v2. Async callbacks are serialized per script; a suspended phase resumes before a later phase runs.

## Runtime Inspection

`ScriptContext` exposes concise accessors for the current script: `Id()`, `Name()`, `Version()`, `Target()`, `Root()`, `Manifest()`, `Entry()`, `Phase()`, `State()`, `Generation()`, `FrameIndex()`, metadata, and CK context conversion.

Use `Runtime::ListInfo(ctx)` and `Runtime::Info(ctx, id)` for structured status. `RuntimeScriptInfo` reports identity, first-class metadata, enabled/loaded/failed state, active phase, error text, `Root()`, `Manifest()`, `Entry()`, and generation. Failed scripts remain inspectable; check `failed`, `State()`, and `Error()` before assuming a script is active.

Use the generic `Message` namespace for script communication. Runtime scripts can subscribe with `[script.messages]` or `Message::Subscribe(ctx, topic)`, publish with `Message::Publish(ctx, topic, payload)`, send directly with `Message::Send(ctx, "runtime:other", topic, payload)`, and reply to requests with `Message::Reply(ctx, msg, payload)`. AngelScript Components use the same `ScriptMessage` type with `CKBehaviorContext`.

Use the `Scene` namespace for high-level Virtools object interop. `ObjectRef@` and precise typed refs revalidate object ids before each access, and `Scene::*` overloads accept `ScriptContext`, `CKBehaviorContext`, or `CKContext@`. See [scene-interop.md](scene-interop.md) for lookup, creation, scene membership, selection, and guarded destruction helpers.

## Validation

Run static validation with:

```powershell
tools\Validate-RuntimeScripts.ps1 -ScriptRoot C:\Game\Data
```

When a Ballance/Player environment is available, add `-CompileProbe` to install the built DLL, set `CKAS_RUNTIME_VALIDATE_ONLY=1`, and compile discovered runtime scripts without running their lifecycle callbacks.

For a full local project check, use:

```powershell
tools\Validate-Local.ps1 -ScriptRoot C:\Game\Data
```

Build startup self-tests with `-DCKAS_BUILD_SELF_TESTS=ON`. At runtime, set `CKAS_RUN_SELFTESTS=1` to run them inside the host, or set `CKAS_SELFTEST_MARKER` to a marker file path; the marker records `status`, `stage`, and any failure message. The Ballance validation script uses the marker path to verify that startup self-tests reached `status=ok`.

## Troubleshooting

If a script is not discovered, confirm that the root points to a `Scripts` directory or to a parent containing `Scripts`, and check for duplicate ids. If it is discovered but not loaded, inspect dependency status first, then the script's compile error. If lifecycle callbacks do not run, verify that every callback uses the explicit `const ScriptContext &in ctx` signature shown above.
