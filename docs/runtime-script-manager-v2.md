# Script Entry Manager v2

Script entries are AngelScript modules discovered from Virtools data paths.
Entries have a host:

- `[runtime]` is a long-lived `ScriptContext` service with runtime lifecycle
  callbacks.
- `[executor]` is a module that provides classes for `AngelScript Executor`
  BB instances. It is compiled and reloadable, but it does not receive
  `ScriptContext` runtime lifecycle callbacks.
- `[script]` remains a compatibility alias for `[runtime]`.

The manager scans each Virtools data path `Scripts` directory shallowly for compatibility, then recursively scans each root listed in `CKAS_SCRIPT_ROOTS`. If an environment root is not already named `Scripts`, its `Scripts` child is scanned when present.

## Discovery Order

Runtime discovery is designed for game data first and explicit overrides second:

1. Virtools data path entries are inspected for a `Scripts` child and scanned one level deep.
2. Each root listed in `CKAS_SCRIPT_ROOTS` is inspected and scanned recursively.
3. If a listed root is already named `Scripts`, it is scanned directly.
4. Otherwise, the root's `Scripts` child is scanned when present.

Within an explicit `Scripts` directory, CKAngelScript recursively accepts
directory modules with `script.as` and explicit single-file `.as` modules. A
single-file module must contain a main `[runtime]`, `[executor]`, or legacy
`[script]` metadata block; plain `.as` files without a main entry block are
treated as include/source files and are not loaded independently. A directory
containing `script.as` is a module boundary and is not recursively scanned for
more entries. Avoid hidden/internal directory names beginning with `.` or `_`;
validation tools skip them.

## Layout

Directory modules use `script.as` as the manifest:

```angelscript
[runtime name="Debug Tools" version="1.0.0" class="DebugTools" entry="runtime.as"]
[runtime description="Runtime diagnostics" author="Team" category="Tools" tags="debug;runtime"]
[runtime.depends required="core>=1.0.0" optional="overlay"]
[runtime.messages topics="game.ready;ui.changed"]
```

Explicit single-file `.as` modules are still valid when they have a main entry
metadata block. For directory modules, `entry` selects the first compiled file and
`files` adds more sources relative to the manifest directory. `id` defaults to
the script's path relative to the scanned `Scripts` directory, with directory
separators converted to dots. For example,
`Scripts/base/event/load_menu_level.as` and
`Scripts/base/event/load_menu_level/script.as` both default to
`base.event.load_menu_level`. Use an explicit `id` only when the path-derived
id is not the stable public identity you want. `name`, `version`, `class`,
`target`, `enabled`, description, author, category, and tags are read as
metadata and surfaced through runtime inspection.

`#include` first resolves relative to the including file. If that file is not
found and the include path is package-style such as `Libs/foo.as`, CKAS searches
upward from the including file's directory through the package root (the parent
of its `Scripts` directory) and uses the first matching file. The search never
escapes that package root. This lets packages keep reusable source under a
root-level `Libs` directory and write stable includes such as
`#include "Libs/game/loading.as"` instead of deep `../../../` paths.

## Manifest Field Reference

| Field | Meaning |
| --- | --- |
| `id` | Stable runtime id. Other scripts construct its target with `Message::Runtime(id)`. |
| `name` | Display name for diagnostics and `RuntimeScriptInfo`. |
| `version` | Version text used for inspection and dependency checks. |
| `host` | `runtime` or `executor`. Usually implied by `[runtime]` or `[executor]`. |
| `class` | Optional script class name for runtime services; expected Executor class for executor modules. |
| `entry` | First compiled file relative to the manifest directory. |
| `files` | Additional source files relative to the manifest directory. |
| `enabled` | Initial enabled state. |
| `target` | Optional target text surfaced through `ScriptContext.Target()`. |
| `description`, `author`, `category`, `tags` | User metadata. |
| `[runtime.depends required="..."]` / `[executor.depends required="..."]` | Required dependencies. |
| `[runtime.depends optional="..."]` / `[executor.depends optional="..."]` | Optional dependencies. |
| `[runtime.messages topics="..."]` | Static runtime message subscriptions. |

## Dependencies

`*.depends` metadata supports `required` and `optional` dependency lists. Entries are semicolon- or comma-separated ids with an optional version constraint such as `core>=1.0.0`. Required dependencies must resolve before the entry can load; optional dependencies are reported but do not block loading when absent.

Dependency status is available through `Runtime::RequiredDependencies(ctx, id)` and `Runtime::OptionalDependencies(ctx, id)`. Use these APIs for diagnostics instead of reparsing manifest text in scripts.

## Lifecycle

Only `[runtime]` entries receive runtime lifecycle callbacks. Those callbacks
must use the explicit context signature:

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

Parameterless lifecycle functions are invalid for runtime entries in v2. Async callbacks are serialized per runtime entry; a suspended phase resumes before a later phase runs. The shared lifecycle Module owns `OnLoad` → `Awake` → `Start` progression for Runtime Entry, Component, and Executor Adapters, plus pending `OnPause` → `OnDisable` position for Runtime Entry and Component Adapters. Host-specific context construction and side effects stay in each Adapter. An initial phase commits only after the Adapter reports completion; suspension or failure leaves it current. `OnEnable` remains Host Activation and may occur between `Awake` and `Start`.

If Play arrives while `OnPause` or its following `OnDisable` is suspended, Host Activation retains the resume event: the old phase finishes first, followed by `OnResume` and `OnEnable`. If `OnEnable` also suspends, the Runtime Entry remains in its resuming state until that invocation finishes. Normal update work starts on a later transition step, never in the frame that completes the retained Resume. A later Pause supersedes the pending Resume. `Runtime::Enable(ctx, id, false)` also clears retained Resume and aborts a suspended `OnEnable`; a disabled Runtime Entry still consumes the host Resume condition without scheduling lifecycle callbacks.

The shared lifecycle Module owns Reset phase position and the `Start` rewind.
Before opening a Reset Epoch, the Runtime Entry Adapter cancels work owned by
the previous epoch and aborts its prepared invocation. `OnReset` may then
suspend and resume. A repeated Reset supersedes that suspended `OnReset` and
opens another epoch; it never resumes the old context. Pause requested while
`OnReset` is pending waits behind Reset, after which the retained Pause/Resume
sequence runs before normal update work. `[executor]` entries may define
Executor lifecycle methods such as `OnLoad(BehaviorFrame@ frame)` on their
Executor classes; those are invoked by each `AngelScript Executor` BB instance,
not by the runtime manager.

`AngelScript Component` is not a script-entry host. Its `Script` parameter may
resolve to a script entry id when no loaded module has that name, but the
Component lifecycle is still owned by the Component BB instance and receives
`BehaviorFrame@`, not `ScriptContext`.

## Reload and Module Ownership

Reload compiles and validates a candidate under a generation-specific physical
module name before disturbing the active Runtime Entry. A failed candidate
therefore leaves the active generation and its consumers unchanged.

Retiring the old generation carries an explicit Runtime Entry Module Mutation
Origin. Every physical generation has a stable Script Host Scope owner that is
propagated through lifecycle and `OnMessage` invocations. Reset cancels that
owner's preceding work before `OnReset`, and retirement cancels its owned tasks
before the module registry evaluates the remaining blockers. Work owned by
another Runtime Entry is unaffected.

The registry rejects remaining unowned blockers such as public
object/execution handles, unowned async tasks, and bound import consumers. It
then tears down Component and Executor states attached to that physical module
and unloads it. A Component behavior whose state was retired resolves the
Runtime Entry again and builds against the new generation on its next
invocation. Public raw replace/unload has no such authority and continues to
return `CKAS_INUSE` while either host consumer is attached.

## Lifecycle Order

| Phase | When it is used |
| --- | --- |
| `OnLoad` | After the module is compiled and the runtime entry object is created. |
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

`ScriptContext` exposes concise accessors for the current runtime entry: `Id()`, `Name()`, `Version()`, `Target()`, `Root()`, `Manifest()`, `Entry()`, `Phase()`, `State()`, `Generation()`, `FrameIndex()`, metadata, and CK context conversion.

Use `Runtime::ListInfo(ctx)` and `Runtime::Info(ctx, id)` for structured status. `RuntimeScriptInfo` reports identity, `Host()`, first-class metadata, enabled/loaded/failed state, active phase, error text, `Root()`, `Manifest()`, `Entry()`, and generation. Failed entries remain inspectable; check `failed`, `State()`, and `Error()` before assuming an entry is active.

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

Use the generic `Message` namespace for script communication. Runtime entries can subscribe with `[runtime.messages]` or `Message::Subscribe(ctx, topic)`, publish with `Message::Publish(ctx, topic, payload)`, send directly with `Message::Send(ctx, Message::Runtime("other"), topic, payload)`, and reply to requests with `Message::Reply(ctx, msg, payload)`. AngelScript Components use the same `ScriptMessage` and `ScriptMessageTarget` values with `CKBehaviorContext`.

Use the `Scene` namespace for high-level Virtools object interop. `ObjectRef@` and precise typed refs revalidate object ids before each access, and `Scene::*` overloads accept `ScriptContext`, `CKBehaviorContext`, or `CKContext@`. See [scene-interop.md](scene-interop.md) for lookup, creation, scene membership, selection, and guarded destruction helpers.

## Validation

Run static validation with:

```powershell
tools\Validate-ScriptEntries.ps1 -ScriptRoot C:\Game\Data
```

Host integration projects can set `CKAS_SCRIPT_VALIDATE_ONLY=1` while launching
their host to compile discovered script entries without running runtime
lifecycle callbacks.

For a full local project check, use:

```powershell
tools\Validate-Local.ps1 -ScriptRoot C:\Game\Data
```

Build startup self-tests with `-DCKAS_BUILD_SELF_TESTS=ON`. At runtime, set
`CKAS_RUN_SELFTESTS=1` to run them inside the host, or set
`CKAS_SELFTEST_MARKER` to a marker file path; the marker records `status`,
`stage`, and any failure message. Host validation tools can use that marker to
verify that startup self-tests reached `status=ok`.

## Troubleshooting

If a script is not discovered, confirm that the root points to a `Scripts` directory or to a parent containing `Scripts`, and check for duplicate ids. If it is discovered but not loaded, inspect dependency status first, then the script's compile error. If lifecycle callbacks do not run, verify that every callback uses the explicit `const ScriptContext &in ctx` signature shown above.

## Validate-Only Mode

`CKAS_SCRIPT_VALIDATE_ONLY=1` makes the manager compile discovered entries and report errors without running runtime lifecycle callbacks. This is used by `Validate-ScriptEntries.ps1 -CompileProbe`, so entries can be checked inside a real host without triggering gameplay logic. `CKAS_RUNTIME_VALIDATE_ONLY` remains a compatibility alias.

## Best Practices

- Use stable ids; other scripts construct your target with `Message::Runtime(id)`.
- Keep `script.as` small and put implementation code in included source files
  or in `entry`/`files` when a manifest-driven module layout is required.
- Prefer package-root relative includes such as `Libs/foo.as` for shared source
  rather than long parent-directory paths.
- Declare static message topics in metadata.
- Prefer `Scene::*`, `Behavior`/`BB`, and `Param::*` over raw CK pointer caching.
- Use `Runtime::Info` and dependency APIs for diagnostics instead of reparsing metadata.
- Keep lifecycle functions idempotent where possible; reloads and validation flows become easier to reason about.
