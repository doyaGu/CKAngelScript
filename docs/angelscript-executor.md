# AngelScript Executor

`AngelScript Executor` is a native Building Block that executes an AngelScript
class from a Virtools behavior graph. It is designed for exact replacement work:
patched `.cmo` / `.nmo` files can shape an Executor instance to match an
original root behavior's external graph interface, then route the original links
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
Executor-specific call object. The older `CKBehaviorContext` ABI remains
available for low-level compatibility and migration.

## Prototype

Base prototype:

| Kind | Entries |
| --- | --- |
| Inputs | `In` |
| Outputs | `Out` |
| Settings | `Script`, `Class`, `Interface`, `Strict Interface`, `Trace` |
| Local | state pointer |

`Script` is a stable runtime script id, either a compatibility package such as
`ballance.logic` or a standalone replacement package such as
`ballance.base.loading.register_activate_init`. The Executor resolves that id
through `ScriptRuntime` and attaches the loaded module behind it. It does not
accept generated internal module names such as `__CKASRuntime_*` or arbitrary
module names outside the runtime script manager.

The Executor declaration is intentionally not targetable. A replacement root
should expose target behavior only when the original graph interface has a real
target slot and the replacement host explicitly implements that slot. The
Ballance `base\Loading_Manager\Register & Activate Init_Script` tracer slice has
no root target, so making the Executor targetable prevents exact dispatch.

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
```

Rules:

- `input` and `output` entries define graph IO.
- `pin` entries define input parameters.
- `pout` entries define output parameters.
- Parameter entries need either `guid="guid:0x...,0x..."` or a resolvable
  `type="..."`.
- If an index is present, it must match the next created slot index. Order in
  the interface text remains the created index order.
- With `Strict Interface` enabled, the Executor verifies final IO names and
  parameter GUIDs after reshaping.

## Script ABI

The preferred required method is one of:

```angelscript
void Execute(BehaviorFrame@ frame)
int Execute(BehaviorFrame@ frame)
```

Legacy methods remain valid:

```angelscript
void Execute(const CKBehaviorContext &in ctx)
int Execute(const CKBehaviorContext &in ctx)
```

When both signatures are present on the same class, the `BehaviorFrame@`
signature is selected.

If `Execute` returns `int`, the value is used as the native BB return code. This
allows scripts to return `CKBR_ACTIVATENEXTFRAME` for graph loops. If `Execute`
returns `void`, the native BB returns `CKBR_OK` unless the script suspends.

Optional lifecycle methods:

```angelscript
void OnLoad(BehaviorFrame@ frame) {}
void Awake(BehaviorFrame@ frame) {}
void Start(BehaviorFrame@ frame) {}
void OnReset(BehaviorFrame@ frame) {}
void OnDestroy(BehaviorFrame@ frame) {}
```

The legacy `const CKBehaviorContext &in ctx` form is accepted for every
lifecycle method. If both forms exist, the `BehaviorFrame@` form is selected.

`BehaviorFrame` is the preferred current-BB facade. It covers the full current
`CKBehaviorContext` plus the current `CKBehavior` identity, layout, IO,
parameters, metadata, graph entry points, and the editable CKBehavior surface.
Use the high-level convenience methods for normal gameplay code and the direct
frame wrappers for SDK-level graph edits. `Behavior()` remains available as an
escape hatch and for interop with APIs that still expect a raw `CKBehavior@`.

```angelscript
// Context and identity.
CKBehavior@ Behavior() const
CKContext@ Context() const
CKBehaviorContext ToBehaviorContext() const
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
CKObject@ CKGetObject(CK_ID id) const
CKERROR CKDestroyObject(CKObject@ object, CKDWORD flags = 0,
                        CKDependencies &in depoptions = void)
CKERROR CKDestroyObject(CK_ID id, CKDWORD flags = 0,
                        CKDependencies &in depoptions = void)
CKERROR CKDestroyObjects(const XObjectArray &in ids, CKDWORD flags = 0,
                         CKDependencies &in depoptions = void)
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
CKParameterIn@ TargetParameter() const
BehaviorRef@ Self() const
BehaviorRef@ Parent() const
BehaviorRef@ OwnerScript() const
CKBehavior@ GetParent() const
CKBehavior@ GetOwnerScript() const
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
CKBehaviorIO@ Input(int index) const
CKBehaviorIO@ Output(int index) const
CKParameterIn@ InputParameter(int index) const
CKParameterOut@ OutputParameter(int index) const
CKParameterLocal@ LocalParameter(int index) const
int InputPosition(CKBehaviorIO@ io) const
int OutputPosition(CKBehaviorIO@ io) const
ParamRef@ Pin(const string &in name, int occurrence = 0) const
ParamRef@ Pin(int index) const
ParamRef@ Pout(const string &in name, int occurrence = 0) const
ParamRef@ Pout(int index) const
ParamRef@ Local(const string &in name, int occurrence = 0) const
ParamRef@ Local(int index) const
ParamRef@ Setting(const string &in name, int occurrence = 0) const
ParamRef@ Setting(int index) const
int PinPosition(CKParameterIn@ pin) const
int PoutPosition(CKParameterOut@ pout) const
int LocalPosition(CKParameterLocal@ local) const
NativePointer PinReadDataPtr(int index) const
NativePointer PoutWriteDataPtr(int index) const
NativePointer LocalReadDataPtr(int index) const
NativePointer LocalWriteDataPtr(int index) const
CKERROR GetInputParameterValue(int index, ?&out value)
CKERROR GetOutputParameterValue(int index, ?&out value)
CKERROR SetOutputParameterValue(int index, ?&in value)
CKERROR GetLocalParameterValue(int index, ?&out value)
CKERROR SetLocalParameterValue(int index, ?&in value)

// Typed convenience.
int PinInt(const string &in name, int fallback = 0, int occurrence = 0) const
int PinInt(int index, int fallback = 0) const
bool SetPout(const string &in name, bool value, int occurrence = 0)
bool SetPout(int index, bool value)
bool SetLocal(const string &in name, int value, int occurrence = 0)
bool SetSetting(int index, const string &in value)

// Structural CKBehavior wrappers for migration and graph-edit code.
int AddInput(const string &in name)
int AddOutput(const string &in name)
CKBehaviorIO@ CreateInput(const string &in name)
CKBehaviorIO@ CreateOutput(const string &in name)
CKBehaviorIO@ RemoveInput(int index)
CKBehaviorIO@ RemoveOutput(int index)
CKERROR DeleteInput(int index)
CKERROR DeleteOutput(int index)
CKParameterIn@ CreatePin(const string &in name, CKParameterType type)
CKParameterOut@ CreatePout(const string &in name, CKParameterType type)
CKParameterLocal@ CreateLocal(const string &in name, CKParameterType type)
CKParameterIn@ RemovePin(int index)
CKParameterOut@ RemovePout(int index)
CKParameterLocal@ RemoveLocal(int index)
BehaviorRef@ SubBehavior(int index) const
CKBehavior@ GetSubBehavior(int index) const
CKERROR AddSubBehavior(CKBehavior@ behavior)
BehaviorRef@ RemoveSubBehavior(int index)
CKBehaviorLink@ SubBehaviorLink(int index) const
CKERROR AddSubBehaviorLink(CKBehaviorLink@ link)
CKParameterOperation@ ParameterOperation(int index) const
void NotifyEdition()
void NotifySettingsEdition()
BBCallBuilder@ Call(const string &in prototype) const
BBCallBuilder@ Call(CKGUID guid) const
void Trace(const string &in message) const
```

`Pin()`/`Pout()` are strict reference APIs. Use `PinInt`, `PinString`,
`PinBool`, and `PinFloat` for fallback reads that should tolerate absent slots,
absent sources, or incompatible value kinds.

`Local(index)`, `Setting(index)`, and `FindSetting(name)` all use the real
CKBehavior local-parameter index. `SetSetting()` writes through the bridge
setting path, sends `CKM_BEHAVIORSETTINGSEDITED`, and refreshes runtime layout
resolution.

Lifecycle methods may suspend through CKAS async support. Initial lifecycle
suspension keeps the Executor active until the lifecycle resumes. `OnReset`
suspension resumes on a later Executor activation and does not run `Execute` as
part of the reset-only resume.

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

The native Executor snapshots active inputs before calling `Execute` and clears
those inputs after the call either finishes or suspends. During the initial
`Execute` call, the script sees the real active input state. After an `await`
resume, consumed inputs are no longer active.

Executor instances can hold script objects and method references into runtime
script modules. `ScriptManager` releases those Executor runtime states before
clearing runtime modules on `PostClearAll`, `OnCKEnd`, and shutdown, so a loaded
replacement root does not keep `__CKASRuntime_*` modules alive during Player
close.

## Current Implementation Boundaries

The first implementation intentionally keeps the surface small:

- runtime script id plus class loading only;
- no inline `Source` or `File` private module path;
- runtime scripts are the only accepted module source;
- no custom Executor-only call object; use the shared `BehaviorFrame`;
- interface reshaping covers counts and created parameter types, with strict
  name/GUID validation;
- interface hash validation and default/source restoration are left to the patch
  tool and later validation work.

This is enough to start testing the replacement host inside Player without
committing to the final Ballance patch-manifest format.

## Ballance Use

For an exact replacement slice:

1. `nmo` inspects the original root behavior.
2. `nmo` inserts `AngelScript Executor` in the same parent graph.
3. `nmo` writes the Executor shape and settings.
4. inbound links are rewired from the original root to the Executor;
5. outbound links are rewired from the Executor to the original downstream
   targets;
6. parameter source links and output parameter consumers are rewired by index;
7. the original root remains present but unreachable until verification passes.

The Executor is the runtime host. The patch manifest remains the source of truth
for persistent graph edits and rollback.

## Ballance Persistence Rules

These rules were verified against Ballance Player while replacing
`base.cmo` behavior `#237`, `Register & Activate Init_Script`, under
`#363 Loading_Manager`.

- Generated `CKBehaviorIO` objects must use the same persistent input/output
  flag shape as Virtools-authored nodes. For the current `nmo` output, set the
  replacement input IO `old_flags` to `1` and output IO `old_flags` to `2`.
  The Player can load a visually valid graph but never dispatch the Executor if
  these flags remain at the tool-generated defaults.
- Preserve one-way parameter source links. For original Ballance source outputs
  such as `#186`, `#206`, and `#171`, set the new `CKParameterIn.source_id`
  directly. Do not add a `destination_count` back-reference unless the original
  graph already has one; doing so changed `Get Row` execution and caused an
  immediate loading failure.
- Dynamic BB input parameters are target-dependent. For example, `Add Row` has
  no static input-parameter list in its declaration; its row-value pins appear
  after the target `CKDataArray` is set. Replacement scripts should set the
  target first, then use the live behavior instance pin order. For the verified
  loader slice, `BehaviorRef.Pin(int)` is the available stable access path.
- Preserve original settings on delegated BBs. In the verified loader slice,
  `Add Row` must run with `Unique Key Column=true`. Because `Add Row` can update
  its dynamic layout after settings, target assignment, and execution, rebind
  live slots from the `BBInstance@` after those phases instead of reusing stale
  `BBSlot@` handles.
- Treat Ballanced BB source as the execution-semantic reference when static
  graph data is ambiguous. For `Activate Script`, the reference implementation
  is
  `C:\Users\kakut\Works\Ballanced\Source\BuildingBlocks\Narratives\Behaviors\ActivateScript.cpp`.
  Its new-version path activates `Out`, skips null script parameters as a
  successful no-op, schedules real scripts through the current
  `CKScene::Activate(script, reset)`, and wakes inactive owner objects. The
  verified loader replacement implements that scene activation directly instead
  of treating transient `BB::Call("Activate Script")` output as sufficient.
- A copied patched asset must pass `nmo validate all`, launch in Player, execute
  the replacement input, activate the expected output, and restore the original
  `base.cmo` after smoke testing.
