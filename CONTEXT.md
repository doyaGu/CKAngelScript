# CKAngelScript Script Host

This context describes how AngelScript code enters the Virtools host and reaches high-level scene, behavior, parameter, messaging, and asynchronous capabilities.

## Language

**Script Host Scope**:
The resolved host identity for one high-level script call: its `CKContext`, `ScriptManager`, behavior bridge, parameter registry, scheduler/message facilities, owner/target identity, and runtime-or-behavior message source. A `ScriptContext`, `CKBehaviorContext`, or raw `CKContext` adapts into one Script Host Scope.
_Avoid_: Generic context, API context, environment

**Runtime Entry**:
A long-lived discovered AngelScript module identified by a stable runtime id and managed by the Script Entry Manager.
_Avoid_: Global script, runtime component

**Initial Lifecycle**:
The once-per-incarnation `OnLoad` → `Awake` → `Start` progression shared by Runtime Entry and Behavior Frame adapters. An initial phase advances only after its invocation finishes; suspension or failure retains the current phase. The shared Module also owns Reset and Destroy phase progression. `OnEnable` is Host Activation that may occur between `Awake` and `Start`.
_Avoid_: Startup callbacks, initialization list

**Host Activation**:
The desired enabled-or-disabled and paused-or-active conditions of a Runtime Entry, Component, or Executor. The shared lifecycle Module owns desired enabled state, pending `OnPause` → `OnDisable` phase position, and Resume eligibility; each Adapter owns its actual enabled side effects. The Component Adapter treats its unsupported `OnPause` phase as a no-op, while the Executor Adapter exposes `OnPause`/`OnResume` and treats unsupported `OnDisable` as a no-op. Resume remains pending while an earlier pause, disable, or enable phase is suspended, is fulfilled only after that phase completes, and is superseded by a later Pause or explicit Disable.
_Avoid_: Pending resume flag, callback boolean

**Reset Epoch**:
The lifecycle interval for one Script Host Scope opened by a host Reset. Opening an epoch cancels work owned by the preceding epoch, aborts its prepared invocation, clears older Pause/Resume transitions, rewinds `Start`, and begins a new `OnReset` phase. A repeated Reset supersedes a suspended `OnReset`; work created by the replacement `OnReset` belongs to the new epoch and may suspend normally. A Pause arriving after the Reset is retained until that Reset phase finishes.
After host-owned transitions finish, the manager performs one `OnCKPostReset` orphan sweep: work with no owner or with an owner that no longer maps to a live Runtime Entry, Component, or Executor is cancelled, while work created by a live owner's new Reset Epoch is preserved. `OnCKPlay` and `PreProcess` retain idempotent fallbacks for failed resets, nonstandard hosts, or direct lifecycle callers that omit the post-reset callback.
_Avoid_: Pending reset flag, reset callback state

**Destroy Transition**:
The terminal per-incarnation transition owned by the shared lifecycle Module. It supersedes Pause, Resume, and Reset, then serializes optional `OnDisable` before `OnDestroy`. Runtime Entry and Component adapters may retain a suspended phase during soft retirement; hard teardown aborts a suspension and advances terminal cleanup. The Executor adapter supplies a no-op `OnDisable` phase because Executor classes do not expose that callback.
For CK object deletion, the manager enters this transition from `SequenceToBeDeleted`; for a full object clear it enters from `PreClearAll`, while behaviors and their frame context are still resolvable. `PostClearAll` is an idempotent cleanup fallback, not the primary callback seam.
_Avoid_: Pending destroy flag, ad-hoc delete callback

**Behavior Frame**:
The revalidating execution-frame facade for the currently executing Component or Executor behavior. It is transient execution state, not persistent gameplay state. A frame created inside a synchronous hard-destroy callback may resolve its exact identity-stamped behavior while CK2 marks it pending deletion; normal frames and Object Refs remain live-only, and the terminal frame becomes invalid as soon as CK2 removes the object.
_Avoid_: Behavior context wrapper, component context

**Object Ref**:
A revalidating script handle that stores CK object identity and reacquires the live CK object near use.
_Avoid_: Cached CK pointer, object wrapper

**Building Block Invocation**:
A configured one-shot or task-based execution of a Virtools Building Block prototype through the behavior bridge.
_Avoid_: Native callback, direct BB pointer call

**Module Mutation Origin**:
The explicit authority carried into module load, replace, or unload policy. Public mutation is rejection-only; Runtime Entry retirement may quiesce Component and Executor consumers attached to its physical module generation; private Component retirement may unload only the module owned by that Component.
_Avoid_: Caller type, reload flag, internal unload

## Flagged ambiguities

- `Context` alone is ambiguous between AngelScript execution context, `CKContext`, `ScriptContext`, and `CKBehaviorContext`. Use the exact type, or Script Host Scope for the resolved high-level host identity.
- Script Host Scope context resolution is part of each high-level call's Interface: Scene calls may fall back from a Behavior Frame to its owning `CKContext`, while frame-owned calls require the Behavior Frame's explicit `CKContext`. Param calls do not substitute the frame's `CKParameterManager` snapshot for a missing `CKContext`. Missing BB or Behavior bridges remain script exceptions for Behavior Frame calls and quiet null/zero results for Runtime Entry calls. Runtime Behavior lookup remains an exact global name lookup. Behavior Frame graph access derives the containing root from the current behavior; Runtime named graphs use the requested global behavior as their root.
- `Local` means logical Component/Executor state in the high-level interface; physical `CKBehavior` locals are called Behavior Locals.

## Example dialogue

> Developer: This Scene call can start from a Runtime Entry or a Behavior Frame. Which context should its implementation accept?
>
> Domain expert: Adapt either entry point into a Script Host Scope, then run the shared Scene implementation. Runtime Entries expose `ScriptContext`; Component and Executor code exposes `BehaviorFrame`. Keep raw native context values inside the implementation unless a low-level SDK call explicitly needs them.
>
> Developer: Can I retain the `CKObject@` returned during the call?
>
> Domain expert: Retain an Object Ref instead, and reacquire the CK object near the final engine operation.
