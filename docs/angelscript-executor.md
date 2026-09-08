# AngelScript Executor

`AngelScript Executor` is a native Building Block that executes an AngelScript
class from a Virtools behavior graph. It is designed for exact replacement work:
patched `.cmo` / `.nmo` files can shape an Executor instance to match an
original graph boundary's external interface, then route the original links
and parameters into AngelScript.

## Purpose

`AngelScript Component` remains the component-style host with metadata field
injection and continuous `Update`. `AngelScript Executor` is the graph
execution host:

- input and output shape can be generated from an interface description;
- script code receives the generic `BehaviorFrame@` host frame;
- script code can consume named inputs with `frame.Take("In")`;
- script code activates real outputs with `frame.Emit("Out")`;
- script code reads and writes real input/output parameters with
  `frame.Pin*()` and `frame.SetPout()`;
- script code can call runtime BBs through `frame.Call(...)`.

`BehaviorFrame` is shared with `AngelScript Component`; it is not an
Executor-specific call object. It does not expose a conversion back to the
native `CKBehaviorContext` ABI.

## Prototype

Base prototype:

| Kind | Entries |
| --- | --- |
| Inputs | `In` |
| Outputs | `Out` |
| Settings | `Script`, `Class`, `Interface`, `Strict Interface`, `Trace` |
| Local | state pointer |

`Script` is a stable script module id and must point at an `[executor]` entry
such as `base.loading.register_activate_init`. The Executor resolves that id through the script entry
manager and attaches the loaded module behind it. It does not
accept generated internal module names such as `__CKASRuntime_*` or arbitrary
module names outside the script entry manager.

The Executor declaration is intentionally not targetable. A replacement root
should expose target behavior only when the original graph interface has a real
target slot and the replacement host explicitly implements that slot. If the
original graph has no root target, making the Executor targetable prevents exact
dispatch.

When `Interface` is non-empty, the BB callback reshapes the instance by adding,
removing, and renaming inputs, outputs, input parameters, and output parameters.
It also updates parameter GUIDs. Patch tools should still write the final shape
into the copied asset; runtime reshaping is a validation/editor convenience, not
the primary persistence mechanism.

## Interface Format

The initial interface format is line-oriented:

```text
input 0 name="In"
input 1 name="Reset"
output 0 name="Out"
output 1 name="Fail"
pin 0 name="Message" type="Message"
pout 0 name="Handled" type="Boolean"
local 0 name="Progress" type="Float" default="0"
```

Rules:

- `input` and `output` entries define graph IO.
- `pin` entries define input parameters.
- `pout` entries define output parameters.
- `local` entries define logical locals exposed through `BehaviorFrame.Local*`.
  Unbound locals become Executor-owned `CKParameterLocal` objects; bound locals
  map directly to an original graph/source parameter and reads/writes touch that
  same Virtools parameter object.
- Parameter entries need either `guid="guid:0x...,0x..."` or a resolvable
  `type="..."`.
- If an index is present, it must match the next created slot index. Order in
  the interface text remains the created index order.
- With `Strict Interface` enabled, the Executor verifies final IO names and
  parameter GUIDs after reshaping.

## Script ABI

The required method is one of:

```angelscript
void Execute(BehaviorFrame@ frame)
int Execute(BehaviorFrame@ frame)
```

If `Execute` returns `int`, the value is used as the native BB return code. This
allows scripts to return `CKBR_ACTIVATENEXTFRAME` for graph loops. If `Execute`
returns `void`, the native BB returns `CKBR_OK` unless the script suspends.

Optional lifecycle methods:

```angelscript
void OnLoad(BehaviorFrame@ frame) {}
void Awake(BehaviorFrame@ frame) {}
void Start(BehaviorFrame@ frame) {}
void OnPause(BehaviorFrame@ frame) {}
void OnResume(BehaviorFrame@ frame) {}
void OnReset(BehaviorFrame@ frame) {}
void OnDestroy(BehaviorFrame@ frame) {}
```

`CKM_BEHAVIORPAUSE` and `CKM_BEHAVIORRESUME` are delivered to an already
loaded Executor instance as `OnPause` and `OnResume`. Both callbacks may
suspend. While `OnPause` is pending, `Execute` does not run; if Resume arrives
first, the shared lifecycle Module retains it until `OnPause` finishes, then
runs `OnResume` exactly once. The frame that completes `OnResume` remains
lifecycle-only, so normal `Execute` resumes on the following activation.
A later Pause supersedes a retained Resume. Reset and Destroy supersede both.
If ordinary `Execute` is suspended when Pause arrives, its prepared invocation
is aborted before `OnPause` starts so one Executor never has two script
contexts in flight. Executor classes still do not expose `OnDisable`.

Executor teardown uses the shared terminal lifecycle transition, with
`OnDisable` mapped to a no-op because Executor classes do not expose that
callback. `OnDestroy` runs while the owning `CKBehavior` is still valid. Since
CK object deletion cannot wait for a suspended callback, a suspended Executor
`OnDestroy` is aborted before native state is released.
When original CK2 has already marked the behavior pending deletion, the
terminal `BehaviorFrame` may still resolve that exact identity-stamped behavior
for the duration of the synchronous callback. This does not make ordinary
Object Refs valid during deletion.

`BehaviorFrame` is the preferred current-BB facade. It covers the full current
`CKBehaviorContext` plus the current `CKBehavior` identity, layout, IO,
parameters, metadata, graph entry points, and the editable CKBehavior surface.
Use the high-level convenience methods for normal gameplay code and the direct
frame wrappers for SDK-level graph edits. The frame does not expose its raw
`CKBehavior@`.

```angelscript
// Context and identity.
CKContext@ Context() const
bool get_valid() const
CK_ID get_id() const
string get_name() const
float DeltaTime() const
CKLevel@ CurrentLevel() const
CKScene@ CurrentScene() const
CKRenderContext@ CurrentRenderContext() const
CKParameterManager@ ParameterManager() const
CKMessageManager@ MessageManager() const
CKAttributeManager@ AttributeManager() const
CKTimeManager@ TimeManager() const

// CKObject / CKSceneObject inherited behavior state.
void SetName(const string &in name, bool shared = false)
CK_CLASSID ClassId() const
CKDWORD ObjectFlags() const
void ModifyObjectFlags(CKDWORD add, CKDWORD remove)
bool IsDynamic() const
bool IsToBeDeleted() const
bool IsNotToBeSaved() const
bool IsPrivate() const
bool IsInterfaceObj() const
bool IsUpToDate() const
int MemoryOccupation() const
NativePointer AppData() const
void SetAppData(NativePointer data)
CKScene@ SceneIn(int index) const
int SceneInCount() const
bool IsInScene(CKScene@ scene) const
bool IsActiveInCurrentScene() const
bool IsActiveInScene(CKScene@ scene) const
void Show(CK_OBJECT_SHOWOPTION show = CKSHOW)
bool IsVisible() const

// Current behavior object and graph.
CKBeObject@ Owner() const
CKBeObject@ Target() const
BehaviorRef@ Self() const
BehaviorRef@ Parent() const
BehaviorRef@ OwnerScript() const
BehaviorLayout@ Layout() const
BehaviorGraph@ Graph() const       // owner-script/container graph
BehaviorGraph@ AsGraph() const     // current behavior as subgraph

// CKBehavior metadata/state.
CK_BEHAVIOR_TYPE Type() const
void SetType(CK_BEHAVIOR_TYPE type)
CK_BEHAVIOR_FLAGS Flags() const
void SetFlags(CK_BEHAVIOR_FLAGS flags)
CK_BEHAVIOR_FLAGS ModifyFlags(CKDWORD add, CKDWORD remove)
void UseGraph()
void UseFunction()
bool IsActive() const
void Activate(bool active = true, bool reset = false)
bool IsTargetable() const
void SetAsTargetable(bool target = true)
CKERROR UseTarget(bool use = true)
bool IsUsingTarget() const
CK_CLASSID CompatibleClassId() const
void SetCompatibleClassId(CK_CLASSID cid)
NativePointer Function() const
void SetFunction(NativePointer fct)
CKGUID PrototypeGuid() const
CKBehaviorPrototype@ Prototype() const
string PrototypeName() const
CKDWORD Version() const
void SetVersion(CKDWORD version)
int Priority() const
void SetPriority(int priority)
float LastExecutionTime() const

// IO and parameters, by name or index. Index overloads use CKBehavior slot
// indexes directly.
int InputCount() const
int OutputCount() const
int PinCount() const
int PoutCount() const
int LocalCount() const
int SettingCount() const
int FindInput(const string &in name, int occurrence = 0) const
int FindPin(const string &in name, int occurrence = 0) const
bool InputActive(const string &in name = "In", int occurrence = 0) const
bool InputActive(int index) const
bool Take(const string &in name = "In", int occurrence = 0)
bool Take(int index)
void Emit(const string &in name = "Out", int occurrence = 0)
void Emit(int index)
int BehaviorLocalCount() const
int FindBehaviorLocal(const string &in name, int occurrence = 0) const
ParamRef@ BehaviorLocal(const string &in name, int occurrence = 0) const
ParamRef@ BehaviorLocal(int index) const
ParamRef@ Pin(const string &in name, int occurrence = 0) const
ParamRef@ Pin(int index) const
ParamRef@ Pout(const string &in name, int occurrence = 0) const
ParamRef@ Pout(int index) const
ParamRef@ Local(const string &in name, int occurrence = 0) const
ParamRef@ Local(int index) const
ParamRef@ Setting(const string &in name, int occurrence = 0) const
ParamRef@ Setting(int index) const
NativeBuffer@ PinReadData(int index) const
NativeBuffer@ PoutWriteData(int index) const
NativeBuffer@ LocalReadData(int index) const
NativeBuffer@ LocalWriteData(int index) const
NativeBuffer@ BehaviorLocalReadData(int index) const
NativeBuffer@ BehaviorLocalWriteData(int index) const
// Typed convenience.
int PinInt(const string &in name, int fallback = 0, int occurrence = 0) const
int PinInt(int index, int fallback = 0) const
int LocalInt(const string &in name, int fallback = 0, int occurrence = 0) const
int LocalInt(int index, int fallback = 0) const
bool SetPout(const string &in name, bool value, int occurrence = 0)
bool SetPout(int index, bool value)
bool SetLocal(const string &in name, int value, int occurrence = 0)
bool SetSetting(int index, const string &in value)

// Safe child inspection. Structural graph edits use frame.Graph().Edit().
BehaviorRef@ SubBehavior(int index) const
void NotifyEdition()
void NotifySettingsEdition()
BBCallBuilder@ Call(const string &in prototype) const
BBCallBuilder@ Call(CKGUID guid) const
BBBridge@ BB() const
AsyncTask<void>@ LinkDelay(int frames) const
AsyncTask<void>@ Wait(BBTask@ task, int inputIndex = -1) const
AsyncTask<void>@ Wait(GraphTask@ task) const
AsyncTask<void>@ WaitMessage(const string &in message, CKBeObject@ object = null) const
AsyncTask<void>@ WaitMessage(CKMessageType messageType, CKBeObject@ object = null) const
void Trace(const string &in message) const
```

`Pin()`/`Pout()`/`Local()` are strict reference APIs. Use `PinInt`,
`PinString`, `PinBool`, `PinFloat`, and the matching `LocalInt`,
`LocalString`, `LocalBool`, `LocalFloat`, `LocalObject` helpers only for
optional reads where a default value is part of the script's intended behavior.
Do not use typed fallback readers to hide a missing required original pin,
pout, local binding, or source link in a replacement script; make the patch or
Executor interface fail instead.

Use `LinkDelay(frames)` when translating an original graph `CKBehaviorLink`
activation delay into AS replacement code. It returns an `AsyncTask<void>@`
driven by the CKAS scheduler and keeps that Virtools graph semantics distinct
from ordinary `Async::Delay()` waits.

Use `frame.Wait(...)` and `frame.WaitMessage(...)` for bridge-task and CK
message waits. The `Async` namespace does not accept a reconstructed
`CKBehaviorContext`.

`LocalCount`, `FindLocal`, `Local(index/name)`, typed `Local*`, and typed
`SetLocal` use the host's logical local interface. For an Executor with
`interface local` declarations, `Local(0)` means declaration slot 0, not the
physical `CKBehavior` local index. If a logical local is mapped by the runtime
patch facade, `Local*` reads and writes the original source parameter directly.

`BehaviorLocal*` uses the physical CKBehavior local-parameter order.
`Setting(index)` and
`FindSetting(name)` also use that physical local index. `SetSetting()` writes
through the bridge setting path, sends `CKM_BEHAVIORSETTINGSEDITED`, and
refreshes runtime layout resolution.

Lifecycle methods may suspend through CKAS async support. Initial lifecycle
suspension keeps the Executor active until the lifecycle resumes. `OnReset`
suspension resumes on a later Executor activation and does not run `Execute` as
part of the reset-only resume. Reset first aborts the preceding suspended
Executor invocation and cancels async tasks owned by that Executor. Tasks owned
by other behaviors are not affected, while tasks created by the new `OnReset`
invocation remain eligible to complete. The shared lifecycle Module retains
the Reset phase and rewinds `Start`. A repeated Reset cancels work from the
first Reset Epoch, aborts its suspended `OnReset`, and invokes a replacement
`OnReset`; the old context is never resumed.

Example:

```angelscript
class EventHandlerExecutor {
    void Execute(BehaviorFrame@ frame) {
        if (!frame.Take("In")) {
            return;
        }

        int value = frame.PinInt("Value", 0);
        frame.SetPout("Handled", value > 0);
        frame.Emit("Out");
    }
}
```

Executor retirement uses the shared terminal transition and delivers `OnDestroy`
while its `CKBehavior` is still live. Ordinary deletion is intercepted by manager
`SequenceToBeDeleted`; full CK object clearing is intercepted by `PreClearAll`.
Hard teardown aborts a suspended destroy callback before releasing the instance.

The native Executor snapshots active inputs before calling `Execute` and clears
those inputs after the call either finishes or suspends. During the initial
`Execute` call, the script sees the real active input state. After an `await`
resume, consumed inputs are no longer active. If the same graph input is
delivered again while that call is suspended, the Executor consumes its local
input activation and checks the existing suspended context without deactivating
the upstream output, which may also feed other graph nodes.

Executor instances can hold script objects and method references into script
entry modules. Each instance records the non-zero manager generation captured
when it attaches. Lifecycle, `Execute`, and suspended-context resume boundaries
compare that equality token before entering cached script state; a mismatch
aborts the context and fails the Executor. Public unload/replace returns
`CKAS_INUSE` while an Executor remains attached. Runtime Entry retirement and
manager teardown explicitly release their owned Executor states before module
unload, so a loaded replacement root does not keep `__CKASRuntime_*` modules
alive during Player close.

## Current Implementation Boundaries

The first implementation intentionally keeps the surface small:

- script module id plus class loading only;
- no inline `Source` or `File` private module path;
- only `[executor]` entries can be attached;
- no custom Executor-only call object; use the shared `BehaviorFrame`;
- interface reshaping covers counts and created parameter types, with strict
  name/GUID validation;
- interface hash validation and default/source restoration are left to the patch
  tool and later validation work.

This is enough to start testing the replacement host inside Player without
committing to a host-specific patch-manifest format. Persistent asset editing,
game object names, and verified graph slices belong in the host integration
project.
