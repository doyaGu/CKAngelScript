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

Frame helpers:

```angelscript
CKBehavior@ Behavior()
CKContext@ Context()
CKBeObject@ Owner()
CKBeObject@ Target()
bool InputActive(const string &in name)
bool Take(const string &in name = "In")
void Emit(const string &in name = "Out")
ParamRef@ Pin(const string &in name)
ParamRef@ Pout(const string &in name)
string PinString(const string &in name, const string &in fallback = "")
int PinInt(const string &in name, int fallback = 0)
bool PinBool(const string &in name, bool fallback = false)
float PinFloat(const string &in name, float fallback = 0.0f)
CKObject@ PinObject(const string &in name)
bool SetPout(const string &in name, bool value)
bool SetPout(const string &in name, int value)
bool SetPout(const string &in name, float value)
bool SetPout(const string &in name, const string &in value)
bool SetPout(const string &in name, CKObject@ value)
BBCallBuilder@ Call(const string &in prototype)
BBCallBuilder@ Call(CKGUID guid)
void Trace(const string &in message)
```

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
