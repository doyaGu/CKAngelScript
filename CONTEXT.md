# CKAngelScript Script Host

This context describes how AngelScript code enters the Virtools host and reaches high-level scene, behavior, parameter, messaging, and asynchronous capabilities.

## Language

**Script Host Scope**:
The resolved host identity for one high-level script call: its `CKContext`, `ScriptManager`, behavior bridge, parameter registry, scheduler/message facilities, owner/target identity, and runtime-or-behavior message source. A `ScriptContext`, `CKBehaviorContext`, or raw `CKContext` adapts into one Script Host Scope.
_Avoid_: Generic context, API context, environment

**Runtime Entry**:
A long-lived discovered AngelScript module identified by a stable runtime id and managed by the Script Entry Manager.
_Avoid_: Global script, runtime component

**Behavior Frame**:
The revalidating execution-frame facade for the currently executing Component or Executor behavior. It is transient execution state, not persistent gameplay state.
_Avoid_: Behavior context wrapper, component context

**Object Ref**:
A revalidating script handle that stores CK object identity and reacquires the live CK object near use.
_Avoid_: Cached CK pointer, object wrapper

**Building Block Invocation**:
A configured one-shot or task-based execution of a Virtools Building Block prototype through the behavior bridge.
_Avoid_: Native callback, direct BB pointer call

## Flagged ambiguities

- `Context` alone is ambiguous between AngelScript execution context, `CKContext`, `ScriptContext`, and `CKBehaviorContext`. Use the exact type, or Script Host Scope for the resolved high-level host identity.
- Script Host Scope context resolution is part of each high-level call's compatibility contract: Scene calls may fall back from a Behavior Frame to its owning `CKContext`, while Message, Param, BB, and Behavior calls require the Behavior Frame's explicit `CKContext`. Param calls do not substitute the frame's `CKParameterManager` snapshot for a missing `CKContext`. Missing BB or Behavior bridges remain script exceptions for Behavior adapters and quiet null/zero results for Runtime adapters. Runtime Behavior lookup remains an exact global name lookup; Behavior Frame lookup remains graph/owner-local first, then global. Runtime named graphs use the requested global behavior as their root, while Behavior Frame graphs derive their containing root from the current behavior.
- `Local` means logical Component/Executor state in the high-level interface; physical `CKBehavior` locals are called Behavior Locals.

## Example dialogue

> Developer: This Scene call can start from a Runtime Entry or a Behavior Frame. Which context should its implementation accept?
>
> Domain expert: Adapt either entry point into a Script Host Scope, then run the shared Scene implementation. Keep the original AngelScript overloads as compatibility adapters.
>
> Developer: Can I retain the `CKObject@` returned during the call?
>
> Domain expert: Retain an Object Ref instead, and reacquire the CK object near the final engine operation.
