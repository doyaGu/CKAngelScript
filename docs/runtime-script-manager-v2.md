# Runtime Script Manager v2

Runtime scripts are long-lived AngelScript modules discovered from Virtools data paths. The runtime scans each `DATA_PATH/Scripts` directory and every root listed in `CKAS_SCRIPT_ROOTS` plus `Scripts`.

## Layout

Directory modules use `script.as` as the manifest:

```angelscript
[script id="tools.debug" name="Debug Tools" version="1.0.0" class="DebugTools" entry="runtime.as"]
[script description="Runtime diagnostics" author="Team" category="Tools" tags="debug;runtime"]
[script.depends required="core>=1.0.0" optional="overlay"]
[script.messages topics="game.ready;ui.changed"]
```

Single-file `.as` modules are still valid. For directory modules, `entry` selects the first compiled file and `files` adds more sources relative to the manifest directory.

## Lifecycle

Runtime lifecycle callbacks must use the explicit context signature:

```angelscript
void OnLoad(const ScriptRuntimeContext &in ctx) {}
void Awake(const ScriptRuntimeContext &in ctx) {}
void OnEnable(const ScriptRuntimeContext &in ctx) {}
void Start(const ScriptRuntimeContext &in ctx) {}
void Update(const ScriptRuntimeContext &in ctx) {}
void OnPostLoad(const ScriptRuntimeContext &in ctx) {}
void OnPostProcess(const ScriptRuntimeContext &in ctx) {}
void OnDisable(const ScriptRuntimeContext &in ctx) {}
void OnDestroy(const ScriptRuntimeContext &in ctx) {}
void OnReset(const ScriptRuntimeContext &in ctx) {}
void OnPause(const ScriptRuntimeContext &in ctx) {}
void OnResume(const ScriptRuntimeContext &in ctx) {}
void OnMessage(const ScriptMessage &in msg, const ScriptRuntimeContext &in ctx) {}
```

Parameterless lifecycle functions are invalid in v2. Async callbacks are serialized per script; a suspended phase resumes before a later phase runs.

## Runtime Inspection

`ScriptRuntimeContext` exposes script identity, root/manifest/entry paths, current phase/state, generation, frame index, metadata, and CK context conversion.

Use `Runtime::ListInfo(ctx)` and `Runtime::Info(ctx, id)` for structured status. `RuntimeScriptInfo` reports identity, first-class metadata, enabled/loaded/failed state, active phase, error text, paths, and generation. `Runtime::RequiredDependencies(ctx, id)` and `Runtime::OptionalDependencies(ctx, id)` return structured dependency status.

Use the generic `Message` namespace for script communication. Runtime scripts can subscribe with `[script.messages]` or `Message::Subscribe(ctx, topic)`, publish with `Message::Publish(ctx, topic, payload)`, send directly with `Message::Send(ctx, "runtime:other", topic, payload)`, and reply to requests with `Message::Reply(ctx, msg, payload)`. AngelScript Components use the same `ScriptMessage` type with `CKBehaviorContext`.

## Validation

Run static validation with:

```powershell
tools\Validate-RuntimeScripts.ps1 -ScriptRoot C:\Game\Data
```

When a Ballance/Player environment is available, add `-CompileProbe` to install the built DLL, set `CKAS_RUNTIME_VALIDATE_ONLY=1`, and compile discovered runtime scripts without running their lifecycle callbacks.
