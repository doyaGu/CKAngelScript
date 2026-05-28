# Runtime Script Manager v2

Runtime scripts are long-lived AngelScript modules discovered from Virtools data paths. The runtime scans each Virtools data path `Scripts` directory, then each root listed in `CKAS_SCRIPT_ROOTS`. If an environment root is not already named `Scripts`, its `Scripts` child is scanned when present.

## Discovery Order

Runtime discovery is designed for game data first and explicit overrides second:

1. Virtools data path entries are inspected for a `Scripts` child.
2. Each root listed in `CKAS_SCRIPT_ROOTS` is inspected.
3. If a listed root is already named `Scripts`, it is scanned directly.
4. Otherwise, the root's `Scripts` child is scanned when present.

Within a `Scripts` directory, CKAngelScript accepts directory modules with `script.as` and single-file `.as` modules. Avoid hidden/internal directory names beginning with `.` or `_`; validation tools skip them.

## Layout

Directory modules use `script.as` as the manifest:

```angelscript
[script id="tools.debug" name="Debug Tools" version="1.0.0" class="DebugTools" entry="runtime.as"]
[script description="Runtime diagnostics" author="Team" category="Tools" tags="debug;runtime"]
[script.depends required="core>=1.0.0" optional="overlay"]
[script.messages topics="game.ready;ui.changed"]
```

Single-file `.as` modules are still valid. For directory modules, `entry` selects the first compiled file and `files` adds more sources relative to the manifest directory. `id` defaults to the directory name for `script.as` manifests and to the file stem for single-file modules. `name`, `version`, `class`, `target`, `enabled`, description, author, category, and tags are read as metadata and surfaced through runtime inspection.

## Manifest Field Reference

| Field | Meaning |
| --- | --- |
| `id` | Stable runtime id. Other scripts address it as `runtime:<id>`. |
| `name` | Display name for diagnostics and `RuntimeScriptInfo`. |
| `version` | Version text used for inspection and dependency checks. |
| `class` | Optional script class name when the runtime script is class-backed. |
| `entry` | First compiled file relative to the manifest directory. |
| `files` | Additional source files relative to the manifest directory. |
| `enabled` | Initial enabled state. |
| `target` | Optional target text surfaced through `ScriptContext.Target()`. |
| `description`, `author`, `category`, `tags` | User metadata. |
| `[script.depends required="..."]` | Required dependencies. |
| `[script.depends optional="..."]` | Optional dependencies. |
| `[script.messages topics="..."]` | Static topic subscriptions. |

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

## Lifecycle Order

| Phase | When it is used |
| --- | --- |
| `OnLoad` | After the module is compiled and the runtime script is created. |
| `Awake` | Early initialization before normal update work. |
| `OnEnable` | When a script becomes enabled. |
| `Start` | First active startup phase. |
| `Update` | Main pre-process tick. |
| `OnPostLoad` | Host post-load callback. |
| `OnPostProcess` | Post-process tick. |
| `OnDisable` | When a script becomes disabled. |
| `OnDestroy` | Runtime clear or script destruction. |
| `OnReset` | Host reset callback. |
| `OnPause` / `OnResume` | Host pause/play callbacks. |
| `OnMessage` | Message bus delivery. |

## Runtime Inspection

`ScriptContext` exposes concise accessors for the current script: `Id()`, `Name()`, `Version()`, `Target()`, `Root()`, `Manifest()`, `Entry()`, `Phase()`, `State()`, `Generation()`, `FrameIndex()`, metadata, and CK context conversion.

Use `Runtime::ListInfo(ctx)` and `Runtime::Info(ctx, id)` for structured status. `RuntimeScriptInfo` reports identity, first-class metadata, enabled/loaded/failed state, active phase, error text, `Root()`, `Manifest()`, `Entry()`, and generation. Failed scripts remain inspectable; check `failed`, `State()`, and `Error()` before assuming a script is active.

Common runtime helpers:

```angelscript
array<string>@ Runtime::List(const ScriptContext &in ctx)
array<RuntimeScriptInfo>@ Runtime::ListInfo(const ScriptContext &in ctx)
RuntimeScriptInfo Runtime::Info(const ScriptContext &in ctx, const string &in id)
bool Runtime::Reload(const ScriptContext &in ctx, const string &in id)
bool Runtime::ReloadAll(const ScriptContext &in ctx)
bool Runtime::Enable(const ScriptContext &in ctx, const string &in id, bool enabled)
array<RuntimeDependencyInfo>@ Runtime::RequiredDependencies(const ScriptContext &in ctx, const string &in id)
array<RuntimeDependencyInfo>@ Runtime::OptionalDependencies(const ScriptContext &in ctx, const string &in id)
```

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

## Validate-Only Mode

`CKAS_RUNTIME_VALIDATE_ONLY=1` makes the runtime compile discovered scripts and report errors without running lifecycle callbacks. This is used by `Validate-RuntimeScripts.ps1 -CompileProbe`, so scripts can be checked inside a real host without triggering gameplay logic.

## Best Practices

- Use stable ids; other scripts address you as `runtime:<id>`.
- Keep `script.as` small and put code in `entry`/`files`.
- Declare static message topics in metadata.
- Prefer `Scene::*`, `Behavior`/`BB`, and `Param::*` over raw CK pointer caching.
- Use `Runtime::Info` and dependency APIs for diagnostics instead of reparsing metadata.
- Keep lifecycle functions idempotent where possible; reloads and validation flows become easier to reason about.
