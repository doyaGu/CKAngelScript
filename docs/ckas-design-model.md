# CKAngelScript Design Model

This document is a working architecture map for using CKAngelScript as a
Ballance logic replacement runtime. It describes how CKAngelScript is shaped
inside Virtools, which execution surfaces it provides, and what that means for
pre-execution replacement of original Ballance behavior graphs.

## Core Mental Model

CKAngelScript is not a single script runner. It is a Virtools plugin made of:

- a manager plugin that owns the AngelScript engine and frame lifecycle;
- Building Blocks that let behavior graphs enter AngelScript;
- shared bridge services for scene objects, behavior graphs, parameters,
  messages, async tasks, and raw CK/Vx SDK access;
- a public C ABI for external native plugins.

For Ballance replacement work, the important split is:

| Surface | Context | Best use | Not best use |
| --- | --- | --- | --- |
| Runtime entry | `ScriptContext` | global services, diagnostics, coordination, preload/validation, message subscribers | exact graph-local root replacement by itself |
| `AngelScript Component` BB | `CKBehaviorContext` / `BehaviorFrame` | component-style behavior logic, owner/target-aware helpers, metadata field injection | drop-in replacement for arbitrary multi-input/multi-output roots |
| `AngelScript Executor` BB | `CKBehaviorContext` / `BehaviorFrame` | graph execution replacement with generated IO and parameter shape | component metadata injection |
| Behavior Bridge | `ScriptContext` or `CKBehaviorContext` | find existing graphs, read/write params, spawn/step native BBs, apply explicit graph edits | saving patched files; persistence remains `nmo`/host responsibility |
| Public native API | C ABI / `CKAngelScriptApi` | optional integration from other native plugins | direct access to internal `ScriptManager` state |

## Native Plugin Layer

`src/Plugin.cpp` exposes two plugin infos:

- `AngelScript Building Blocks`: registers `AngelScript Component` and
  `AngelScript/Event Hook` behavior declarations.
- `AngelScript Manager`: creates and destroys `ScriptManager`.

`ScriptManager` is the central owner. It derives from `CKBaseManager` and owns:

- `ScriptEngineHost`
- `ScriptModuleRegistry`
- `ScriptRuntime`
- `ScriptBehaviorBridge`
- `ScriptParameterRegistry`
- `ScriptAsyncScheduler`
- `ScriptMessageBus`
- component state and CK object handle registries
- public module/object/execution handle stores

The manager receives CK lifecycle callbacks through
`GetValidFunctionsMask()`. The frame order in `ScriptManager::PreProcess()` is:

1. async scheduler tick;
2. message bus tick;
3. runtime entry `PreProcess`;
4. behavior bridge `PreProcess`.

`PostProcess()` then runs runtime post-process callbacks before bridge
post-process work. Reset, pause, play, end, post-load, and clear callbacks are
also manager-owned and fan out to runtime, bridge, component state, message
queues, async queues, and retained CK object data.

Consequence: all CKAngelScript execution is coordinated through the manager
lifecycle. A Ballance replacement plan must respect this ordering instead of
assuming that a script call is isolated from queued messages, async resumes, or
bridge cleanup.

## CKBehavior Host Model

A `CKBehavior` is the live Virtools behavior instance that owns runtime IO,
parameter slots, local/settings data, owner/target relation, and graph position.
It is not the same thing as an AngelScript object or a script entry module.

Keep these layers separate:

- `CKObjectDeclaration` identifies a BB type to Virtools and points to the
  prototype creation function.
- `CKBehaviorPrototype` declares the initial IO, parameter, local/setting,
  callback, flags, and native execution function.
- `CKBehavior` is the instantiated graph node. Its live layout may differ from
  the prototype after callbacks, settings edits, target assignment, or generated
  Executor interface changes.
- `CKBehaviorContext` is a borrowed per-execution frame. It gives the current
  behavior, context, scene/level, managers, delta time, and callback message.
  It should not be stored as durable gameplay state.

Runtime semantics follow Virtools graph rules:

- active inputs are scheduling signals and must be cleared only when consumed;
- active outputs are graph continuation signals and should be activated after
  output parameters are written;
- input parameters are connection endpoints, not just values. A
  `CKParameterIn` reads through its real source parameter;
- output parameters are writable behavior-owned slots;
- `CKBR_ACTIVATENEXTFRAME` schedules the same behavior again through Virtools;
- owner and target are graph semantics, not cosmetic metadata. Target-dependent
  BBs can rebuild dynamic pins from the assigned target.

AngelScript suspension is layered on top of this model. When a Component or
Executor method awaits, CKAngelScript must keep the prepared AngelScript
context, return `CKBR_ACTIVATENEXTFRAME`, and refresh the `BehaviorFrame`
against the current `CKBehaviorContext` before resuming. A resumed script is
continuing the same AngelScript call, not starting a fresh BB execution.

## Engine And Module Layer

`ScriptEngineHost` creates the AngelScript engine, configures engine
properties, registers add-ons, and registers CKAngelScript APIs. The repository
vendors AngelScript 2.38.0 under `deps/angelscript`.

Module identity is explicit:

- script entries are discovered modules with stable script ids;
- components may use shared modules by script name;
- components with `Source` or `File` use private per-behavior modules;
- public API handles track module generations so stale module references can be
  detected after reload/replace.

Consequence: Ballance replacement code should use stable module and class names
for generated replacements. Private component modules are useful for isolated
experiments, but shared modules are easier to validate and diff across many
patched roots.

## Script Entries

Script entries are discovered from Virtools data path `Scripts` directories
and explicit `CKAS_SCRIPT_ROOTS`. Data path roots are scanned one level deep
for compatibility with existing game folders; explicit roots are scanned
recursively. Directory modules use `script.as` as the manifest and form module
boundaries. Single-file modules must contain a main `[runtime]`, `[executor]`,
or legacy `[script]` metadata block. Plain `.as` files without a main entry
block are available to `#include` but are not loaded independently. When `id`
is omitted, the entry id is
derived from the path relative to the scanned `Scripts` directory, so
`Scripts/base/event/load_menu_level.as` becomes `base.event.load_menu_level`.
Only `[runtime]` entries define class-backed `ScriptContext` lifecycle
callbacks:

- `OnLoad`
- `Awake`
- `OnEnable`
- `Start`
- `Update`
- `OnPostLoad`
- `OnPostProcess`
- `OnDisable`
- `OnDestroy`
- `OnReset`
- `OnPause`
- `OnResume`
- `OnMessage`

The required context signature is `const ScriptContext &in ctx`.

Runtime entries are long-lived and global. They are good for:

- shared Ballance state tables and diagnostics;
- topic subscriptions;
- validation/probe orchestration;
- module reload and runtime inspection;
- services used by multiple components.

They are not sufficient as the primary replacement mechanism for an original
root behavior graph, because they do not naturally occupy the original graph
node's owner, target, input, output, and link position.

## AngelScript Component

`AngelScript Component` is a real Virtools Building Block. Its prototype has a
fixed shape:

| Kind | Entries |
| --- | --- |
| Inputs | `Enable`, `Disable` |
| Outputs | `Enabled`, `Disabled`, `Error` |
| Input parameters | `Script`, `Class`, `Source`, `File`, `Manifest` |
| Local parameter | component state pointer |
| Setting | `Output Error Message` |

The component instantiates a script class and calls lifecycle methods with
`BehaviorFrame@ frame` by default. The older `const CKBehaviorContext &in ctx`
form remains compatible. `Update` is required. Optional methods include
`OnLoad`, `Awake`, `OnEnable`, `Start`, `OnDisable`, `OnDestroy`, `OnReset`,
and `OnMessage`.

The Component is a BB host, not a script-entry host. Its module source is
explicit: `Source`/`File` compile a private per-behavior module owned by the
Component; otherwise `Script` first names an already loaded module, then falls
back to a script entry id when no loaded module has that name. This lets
Component reuse code managed by `[runtime]` or `[executor]` entries without
adding a `[component]` entry type or giving the Component its own global
lifecycle.

Once enabled, the component is continuous: after `Start`, it calls `Update`,
steps automated `BBConfig` instances, and returns `CKBR_ACTIVATENEXTFRAME`.

The component supports metadata and manifest-driven field injection. Script
fields can receive scalar values, CK objects, `BehaviorRef@`, `BBDecl@`,
`BBConfig@`, `BBSlot@`, `ParamTypeInfo@`, and related bridge objects.

Consequence for Ballance:

- The component is useful for component-style replacement logic because it has
  `CKBehaviorContext` and graph-local ownership.
- It is not the default host for arbitrary original roots. The fixed Component
  IO cannot directly express original root inputs, outputs, delayed links, and
  downstream activations.
- One-shot original roots must be represented deliberately. Either the adapter
  pulses and disables the component, or the script guards its own one-shot
  state.

## Event Hook

`AngelScript/Event Hook` is a small graph-to-message bridge. It has one input,
`Out` and `Error` outputs, and parameters for topic, target, and payload. When
triggered, it publishes through `ScriptMessageBus`.

Use it for observation, instrumentation, and simple persistent graph event
injection. Do not use it to preserve exact same-frame graph output semantics
unless the timing has been verified, because message delivery is manager-ticked
and should not be treated as immediate graph continuation.

## Shared Interop Services

### Scene

The `Scene` namespace returns checked `ObjectRef@` wrappers such as
`Entity3DRef@`, `MaterialRef@`, `MeshRef@`, `SceneRef@`, and `BehaviorRef@`.
These refs store CK identity and revalidate before access. Scripts should keep
refs as durable identity and reacquire raw CK pointers only near the operation
that needs them.

### Behavior And BB Bridge

The Behavior Bridge exposes:

- graph lookup and traversal through `BehaviorGraph`, `BehaviorQuery`, and
  `BehaviorNode`;
- durable graph handles such as `BehaviorRef@` and `BehaviorLinkRef@`;
- parameter handles and SDK-driven value conversion through `ParamRef@` and
  `Param::*`;
- runtime native BB calls through `BBDecl@`, `BBConfig@`, `BBSlot@`,
  `BBInstance@`, `BBTask@`, and `GraphTask@`;
- explicit persistent graph edit transactions through `BehaviorGraphEdit`.

Live graph edits happen only when `Apply(ctx)` succeeds. Saving modified
`.cmo` or `.nmo` files is outside CKAngelScript and remains a host or `nmo`
responsibility.

The bridge has to distinguish static declaration layout from live behavior
layout:

- `BBDecl` and graph summaries describe the declaration or cached graph shape.
- A live `CKBehavior` can have dynamic pins after a target is assigned, after an
  Executor interface reshape, or after a Virtools BB updates itself.
- Replacement scripts should use `BehaviorRef.Pin(index)` for fixed slots and
  `BehaviorRef.Pin(name, occurrence)` when the live instance is the authority.
  `Pin(name)` scans the actual `CKBehavior::GetInputParameter(i)` list; it is
  the correct lookup path for dynamic or generated replacement instances.
- Runtime BB calls inherit the current `CKBehaviorContext` target unless the
  call overrides it. Use `Target(object)` before calling target-dependent BBs
  that synthesize dynamic pins, and use `Target(null)` before non-targetable BBs
  such as `Activate Script`.
- Dynamic BB callbacks can change layout generations after settings, target
  assignment, or execution. Do not keep `BBSlot@` handles across those phases
  when the BB is known to rebuild pins or outputs; rebind live slots from the
  `BBInstance@` before reading or writing them.
- Some Virtools BBs represent scheduler or scene semantics, not just immediate
  function calls. `Activate Script`, as implemented in Ballanced, schedules a
  `CKBehavior` through the current `CKScene::Activate(script, reset)` and wakes
  the owner object when needed. Exact replacements must preserve that scene
  activation side effect; a transient runtime BB result with `Out` active is not
  sufficient by itself.

### Messaging

The `Message` namespace connects runtime entries and components with
`ScriptMessage`. Targets are `runtime:<id>` and `component:<CK_ID>`. Topic
publish and directed send are shared between `ScriptContext` and
`CKBehaviorContext`.

Messages are queued/ticked by the manager. Use messages for service-level
communication, not as an unverified replacement for exact Virtools graph link
ordering.

### Async

`AsyncTask<T>` is cooperative and scheduler-backed. It is not an OS-threading
model. Async resumes are part of the manager tick order and can interact with
message and runtime callback ordering.

## Replacement Model For Ballance

The preferred Ballance replacement model is offline pre-execution patching:

1. inspect the pristine `.cmo` or `.nmo` with `nmo`;
2. produce a patch manifest for one original root behavior;
3. write a copied patched file that inserts an `AngelScript Executor` shaped
   from the original root interface;
4. disconnect inbound activation paths to the original root before first
   execution;
5. preserve downstream activations, delays, parameter side effects, messages,
   and object ownership;
6. validate and launch Player against the copied patched file.

A startup bootstrap is acceptable only when file-level patching is impractical,
and it must still apply before any replaced game root can execute.

Runtime hot swapping is not part of the target architecture.

The persistent graph edit must preserve Virtools execution metadata, not only
the visible links:

- keep the replacement Executor non-targetable unless the original root had a
  target slot and the host implements it;
- persist generated input/output `CKBehaviorIO.old_flags` with values that match
  authored behavior nodes;
- preserve one-way parameter `source_id` links without inventing source
  destination back-references;
- keep the original root present but unreachable until the replacement has
  passed Player smoke testing and rollback has been recorded.

## Adapter Requirement

For each original root behavior, classify the external interface:

- owner object and target;
- parent graph position;
- input names, indices, and activation assumptions;
- output names, indices, downstream links, and delays;
- input/output/local parameters and source links;
- data-array reads/writes;
- messages sent or waited;
- side effects on objects, scenes, physics, sound, UI, and loaded NMOs.

Then choose one host shape:

| Host shape | Use when |
| --- | --- |
| `AngelScript Executor` | Default path for root replacement; the replacement should expose generated inputs, outputs, pins, and pouts |
| Component-only | Non-root helper behavior or a root whose interface is already compatible with fixed Component IO |
| Component plus generated graph adapter | Temporary path when reusing a Component implementation during migration |

The adapter is part of the replacement, not an optional convenience. If the
adapter cannot prove output and timing equivalence, the port unit is not ready
for replacement testing.

The dedicated graph execution path is specified in
[`angelscript-executor.md`](angelscript-executor.md). The intended shape is an
`AngelScript Executor` instance whose inputs, outputs, pins, and pouts are
generated from the original root interface.

## Practical Rules

- Use runtime entries for shared services and probes.
- Use `AngelScript Executor` for graph-local root replacement classes.
- Use `AngelScript Component` for component-style helpers and metadata-driven BB composition.
- Use `Scene` refs for durable object identity.
- Use the Behavior Bridge to delegate native BB behavior during early migration.
- Use messages for decoupled services, not for exact graph-link emulation unless
  timing is verified.
- Use `nmo` to produce reproducible patched files; do not depend on CKAS live
  graph edits for persistent Ballance assets.
- Add C++ bindings only after a port unit proves that `Scene`, `Behavior`,
  `BB`, `Param`, raw SDK bindings, and runtime BB delegation are insufficient.

## Immediate Consequence For The First Port Unit

For `base\Event_handler`, the first exact replacement slice should produce:

- a static interface description from `base.behavior-4692.dump.json`;
- a list of inbound links that must be disconnected pre-execution;
- a list of outputs/downstream activations that the Executor must preserve;
- an `AngelScript Executor` class for the logic body;
- a generated Executor interface and insertion plan;
- a patch manifest and copied output file path;
- a reference probe when static evidence cannot prove message or activation
  ordering.

Only after that should the original `Event_handler` branch be made unreachable
in a replacement build.
