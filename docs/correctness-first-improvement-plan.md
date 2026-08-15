# Correctness-First Runtime Improvement Plan

Status: implementation in progress (started 2026-08-14).

## Implementation Progress

The first correctness tranche is implemented:

- scheduler-wide cancellation preserves wait records until the waiter consumes
  the cancellation or its invocation explicitly forgets the context;
- Pause freezes async task advancement and queued-message/time-out frame
  advancement, while Play resumes both processors;
- Reset cancels pre-reset async/message work and suspended public execution
  handles before `OnReset`, so work created by `OnReset` is not erased by a
  later global clear;
- End and PostClearAll cancel public executions and pending work before hard
  teardown;
- Runtime Entry resume events are retained while `OnPause`/`OnDisable` is
  suspended, then run in `OnResume` → `OnEnable` order;
- public module replace/unload preflight includes live Component and Executor
  consumers and returns `CKAS_INUSE`;
- every Component runtime reset advances its message incarnation, so targets
  captured before the rebuild are stale.

The following work remains open:

- apply the deferred resume state machine to Component and Executor adapters;
- make reset cancellation owner-scoped for every Component/Executor invocation,
  including work created inside their `OnReset` callbacks;
- record attached module generations in Component/Executor state and validate
  them at direct invocation boundaries;
- move all mutation ownership/orphan retirement into the module registry and
  remove the post-commit `PrepareModuleMutation()` hook;
- consolidate lifecycle ordering behind the shared lifecycle seam;
- add the engine-backed CTest target and obtain a passing real Player callback
  trace for the new startup regressions.

Validation on 2026-08-14: the Release DLL builds and links with startup
self-tests compiled in, and `CKAngelScript.Core` passes. Local Player validation
was attempted but could not reach the startup marker: the Ballance host failed
render-engine initialization, while the separate NeMo test distribution did
not load this SDK build into a manager frame. The Player gate therefore remains
open; build success is not recorded as a substitute for it. The API catalog
tool's four unit tests pass, but `sync_api_catalog.py --check` reports an
existing `docs/script-api.lock.json` drift. None of the tracked catalog inputs
or outputs changed in this tranche, and no public AngelScript declaration was
changed, so the lock was not regenerated without a fresh live-engine export.

This plan addresses correctness risks in async suspension, host lifecycle
transitions, and module-generation ownership before attempting broad structural
refactoring. It uses the existing CKAngelScript domain language: Runtime Entry,
Behavior Frame, Script Host Scope, Object Ref, and Building Block Invocation.

The governing decision is:

> Preserve or reject state transitions explicitly. Never erase state and then
> infer success from its absence.

## Scope

This plan covers:

- async task and `Await` behavior across pause, resume, reset, clear, and
  shutdown;
- ordering of suspendable lifecycle phases for Runtime Entries, Components, and
  Executors;
- module replacement and unload while internal consumers retain objects,
  functions, contexts, or tasks from a module generation;
- test seams needed to verify those contracts outside and inside a real
  Virtools host.

This plan does not change:

- Script Host Scope context-resolution compatibility;
- Object Ref validation rules;
- Building Block Invocation behavior;
- raw CK/Vx bindings or optional DynCall support;
- the public native ABI or registered AngelScript declarations unless a later
  compatibility review explicitly approves a change.

## Evidence and Current Risks

### 1. Clearing async state can turn cancellation into apparent success

`ScriptManager::OnCKPause()` calls the Runtime Entry pause lifecycle and then
clears the async scheduler. `ScriptAsyncScheduler::Clear()` removes wait records
before cancelling tracked tasks. Later, `PrepareContextResume()` treats a
missing wait record as `Ready`, so a suspended context can continue without
observing cancellation or receiving its output value.

Relevant implementation:

- [`ScriptManager::OnCKPause`](../src/ScriptManager.cpp)
- [`ScriptAsyncScheduler::Clear`](../src/ScriptAsync.cpp)
- [`ScriptAsyncScheduler::PrepareContextResume`](../src/ScriptAsync.cpp)
- [`ScriptInvocationContext::Resume`](../src/ScriptInvocation.cpp)

The same unqualified `Clear()` operation is used for reset, end, full clear, and
shutdown even though those transitions require different behavior.

### 2. A deferred resume can be lost after a suspended pause phase

Runtime Entry callbacks are documented as serialized: a suspended lifecycle
phase must resume before a later phase runs. If `OnPause` or its following
`OnDisable` suspends and CK resumes, `ScriptRuntime::OnResume()` skips the module
while `PendingPause` is set. The pending pause later completes from
`UpdateModule()`, but the skipped `OnResume` event is not retained.

Component state has an equivalent risk. `CKM_BEHAVIORRESUME` calls
`EnableInstance()`, which returns early while `InstanceEnabled` is still true.
The previously suspended `OnDisable` can then finish and set the instance to
disabled after the host has already resumed.

Relevant implementation:

- [`ScriptRuntime::OnPause`, `OnResume`, and `UpdateModule`](../src/ScriptRuntime.cpp)
- [`EnableInstance`, `DisableInstance`, and the Component callback`](../src/AngelScriptComponent.cpp)
- [documented Runtime Entry serialization](runtime-script-manager-v2.md)

Actual Virtools callback ordering must be captured in Player validation before
changing Component transition code, but no ordering can make deletion of the
pending resume event correct.

### 3. Module mutation does not account for all internal consumers

The mutation policy checks public runtime handles, async tasks, and bound import
consumers. It does not include Component or Executor states. The post-commit
preparation hook releases Executor states only.

A Component attached to a loaded shared module stores the module name but not
the manager's module generation. `ComponentIdentityChanged()` therefore cannot
detect a same-name replacement, and the loaded fast path continues to use the
old script object and cached methods.

Relevant implementation:

- [`ScriptModuleMutationPolicy`](../src/ScriptModuleMutationPolicy.cpp)
- [`ScriptManager::PrepareModuleMutation`](../src/AngelScriptExecutor.cpp)
- [`ComponentIdentityChanged`](../src/AngelScriptComponent.cpp)
- [`ScriptComponentState`](../src/ScriptComponentState.h)
- [Component module ownership contract](component-metadata-manifest.md)

Runtime Entry reloads usually avoid this exact failure because each discovered
generation receives a different engine module name. Loaded shared modules do
not have that protection.

There is also a message-incarnation gap. `MessageGeneration` changes when a
Component state is created, but not when its script object and module binding
are rebuilt in the same state. A queued or retained typed target can therefore
cross a runtime incarnation and reach whichever object happens to be installed
when delivery runs. Direct Component message delivery also enters the cached
`OnMessage` method without first validating the attached module generation.

Relevant implementation:

- [`ScriptComponentStateStore::GetOrCreate` and `ResetComponentStateRuntime`](../src/ScriptComponentStateStore.cpp)
- [`ScriptManager::DeliverComponentMessage`](../src/ScriptComponentStateStore.cpp)
- [`ScriptMessageTarget`](../src/ScriptMessage.cpp)

### 4. The highest-risk behavior is not covered by the default CTest seam

The default `CKAngelScript.Core` executable covers the host-neutral initial
lifecycle, metadata, and dependency modules. Most engine, Component, Executor,
Message, Async, Bridge, and public native interface checks are startup self-tests
compiled into the DLL when `CKAS_BUILD_SELF_TESTS` is enabled.

This makes the real state-machine interaction harder to exercise in a normal
CTest run and makes Player callback ordering an implicit part of correctness.

## Correctness Contracts

Implementation work must begin with tests for these contracts.

### Await contract

For every suspended AngelScript context registered with the async scheduler:

1. Exactly one terminal result is observed: completed, failed, or cancelled.
2. Absence of a wait record never means that a previously registered wait
   completed successfully.
3. A typed output is written only after successful completion and type
   validation.
4. A wait record is removed only when:
   - `PrepareContextResume()` consumes its terminal result; or
   - the owning invocation is explicitly aborted or released and calls
     `ForgetContext()`.
5. Cancelling scheduler ownership of a task does not erase the result that its
   suspended waiter still needs to observe.
6. A context must not be resumed after hard teardown.
7. Cancellation is a typed terminal outcome. Internal invocation state must
   distinguish it from generic failure, and public execution handles report
   `CKAS_CANCELLED`.
8. Public execution handles participate in host transitions. Pause freezes a
   scheduler-dependent execution; reset or end cancels executions superseded by
   that transition; shutdown releases them after cancellation.

### Host lifecycle contract

Host transitions have distinct semantics:

| Transition | Async work | Suspended invocation | Required ordering |
| --- | --- | --- | --- |
| Pause | Freeze; do not consume frames or timeout | Preserve | Finish `OnPause`/`OnDisable` after processing resumes, then deliver the retained resume event |
| Resume | Resume scheduler and message processing | Resume pending phase first | `OnPause`/`OnDisable` completion precedes `OnResume`/`OnEnable`, which precedes `Update`/`Execute` |
| Reset | Cancel prior work for the affected Script Host Scope before `OnReset` starts | Abort superseded phase | Work created by `OnReset` belongs to the new reset epoch and may suspend normally |
| End / clear / shutdown | Cancel all work, including public execution handles | Abort before forgetting wait records | No script context resumes after teardown begins |

Pause therefore freezes pending message requests as well as async tasks. A
frame-based timeout does not advance while manager processing is paused.

Reset must be owner-scoped rather than implemented as a late global `Clear()`.
Runtime Entry, Component, and Executor adapters must cancel their prior work
before invoking `OnReset`; tasks created during `OnReset` must survive. Work
without a live Script Host Scope is cancelled as orphaned work after owners have
transitioned.

### Lifecycle serialization contract

Each Runtime Entry or Behavior Frame script has one serialized lifecycle stream:

- at most one lifecycle invocation is active;
- host events that arrive while a phase is suspended are retained, coalesced
  only when the documented semantics are identical, and processed in order;
- `OnResume` is never lost because `OnPause` is pending;
- `OnEnable` never runs before a prior `OnDisable` completes;
- reset supersedes normal update work but does not silently convert a cancelled
  wait into success;
- destroy supersedes every other phase and cannot return to normal update work.

### Module mutation contract

1. A failed mutation leaves the engine module, source cache, include/import
   edges, generation, and all consumers unchanged.
2. Generation increments exactly once after a successful commit.
3. Unload cannot return `CKAS_OK` while an internal consumer can still execute
   code from that module under the unloaded name.
4. Same-name replacement cannot leave a Component executing the old generation
   without an explicit compatibility policy.
5. Public raw replacement or unload returns `CKAS_INUSE` when a live Component,
   Executor, invocation, async task, object, or import consumer depends on the
   generation.
6. Owner-coordinated transitions may retire their own consumers, but the origin
   and allowed consumers must be explicit:
   - Runtime Entry retirement may retire consumers attached to that Runtime
     Entry generation;
   - private Component module retirement may retire only its owning Component;
   - shutdown may retire all consumers without resumption.
7. A replacement candidate is compiled and validated before consumers of the
   old generation are disturbed.
8. Rebuilding a Component runtime increments its message incarnation. Queued
   messages and typed targets from an older incarnation are rejected as stale.
9. Direct Component message delivery validates both message incarnation and the
   attached module generation before invoking `OnMessage`.

The default policy is rejection, not implicit live migration. Re-instantiating
script objects can execute user lifecycle code and fail; it is not an atomic
side effect and must not be hidden inside a public raw module mutation.

## Target Architecture

### Deepen `ScriptAsyncScheduler`

The scheduler interface currently exposes a shallow `Clear()` operation whose
callers must know how waits, tasks, invocations, and lifecycle transitions
interact. Move that knowledge into the scheduler module.

Replace lifecycle callers of `Clear()` with explicit internal operations such
as:

```cpp
PauseProcessing();
ResumeProcessing();
CancelScope(scopeIdentity, reason);
CancelOrphans(reason);
Shutdown(reason);
```

These names are illustrative, not a frozen C++ interface. The implementation
must distinguish tracked-task ownership from waiter-result ownership. A
cancelled task may be removed from the runnable collection while its terminal
result remains reachable by the suspended context.

The deletion test supports this deepening: deleting the scheduler transition
logic would force every lifecycle caller to reconstruct the same cancellation,
wait-retention, and resume rules.

### Deepen `ScriptModuleRegistry`

Module mutation is currently spread across the registry, replacer, mutation
policy, manager, state stores, handle registry, async scheduler, and import
binder. Keep those as implementation details behind one mutation transaction in
`ScriptModuleRegistry`.

The synchronous transaction has four conceptual stages:

1. **Preflight**: validate flags, callback state, dependencies, and origin.
2. **Stage**: compile or load a candidate without disturbing the current
   generation.
3. **Quiesce**: prove there are no live consumers for public raw mutation, or
   stop only consumers authorized by an owner-coordinated origin while the old
   generation is still current.
4. **Commit and publish**: replace engine/cache state, increment generation
   once, and expose the new generation.

Public raw mutation stops at preflight with `CKAS_INUSE` when any live internal
consumer exists. It never runs user lifecycle code as an implicit side effect.
An owner-coordinated live migration that can suspend or fail is not forced into
this synchronous transaction. It must use generation-specific physical module
names, stage the new generation independently, switch the logical owner only
after validation, and retire the old generation after the switch. Runtime Entry
reload already follows this general physical-name strategy.

Public callers continue to use the existing load, compile, and unload interface.
They should not learn the internal consumer graph. The deeper module increases
leverage by enforcing one transaction contract for source, sections, bytecode,
and unload paths, and increases locality by concentrating rollback and
generation rules.

`ScriptComponentStateStore` and `ScriptExecutorStateStore` need module-aware
queries so the registry can preflight internal consumers. Component state must
also record the attached module generation for diagnostics and defensive stale
checks even when the default mutation policy rejects live replacement. A
Component runtime rebuild must also issue a new message incarnation so old
targets cannot cross the rebuild seam.

### Deepen `ScriptInitialLifecycle` after behavior is fixed

Do not start by mechanically merging the three large entry implementations.
First lock down observable behavior with tests and apply the minimal correctness
fixes. Then deepen `ScriptInitialLifecycle` from an initial-phase bitset into the
module that owns lifecycle transition ordering, deferred host events, and
suspend/resume outcomes.

The seam has two real production adapters:

- a Runtime Entry adapter that supplies `ScriptContext` and runtime methods;
- a Behavior Frame adapter used by Component and Executor code.

Host-specific lookup, parameter injection, dynamic Executor IO, and Component
automation remain in those adapters. The lifecycle module owns only ordering,
state transitions, and terminal outcomes. This gives tests one interface while
preserving locality for host-specific implementation.

### Keep `ScriptManager` as the orchestration root for now

`ScriptManager` is broad, but splitting it before transition contracts stabilize
would move complexity without removing it. The first changes should make it a
thin lifecycle orchestrator over deeper scheduler, module-registry, and
lifecycle interfaces. Reassess a larger split only after the new tests pass and
callers no longer reach through it for mutation details.

## Delivery Plan

### Phase 0 — Characterize and make failures reproducible

No production behavior changes.

1. Add table-driven tests for lifecycle event ordering and all invocation
   outcomes: finished, suspended, failed, and cancelled.
2. Add an engine-backed test for a wait record that is cancelled while its
   context is suspended.
3. Add Player trace coverage for manager and behavior callback order during
   pause, resume, reset, end, and clear.
4. Add a Component test that attaches to a loaded shared module and attempts
   same-name replace and unload.
5. Add public execution-handle tests across pause, reset, end, and shutdown.
6. Add a queued-message test across Component runtime rebuild and verify stale
   message incarnation handling.
7. Record the chosen pause/reset semantics in `async.md`,
   `runtime-script-manager-v2.md`, and `component-metadata-manifest.md` alongside
   the implementation change, not before.

Exit gate:

- the two known failure paths reproduce in tests or the plan is corrected with
  contrary host evidence;
- Player callback ordering is captured as test output rather than assumed;
- no public registered declaration changes.

### Phase 1 — Fix async and host-transition correctness

1. Split scheduler processing state from destructive teardown.
2. Preserve terminal wait results until consumed or explicitly forgotten.
3. Replace pause-time queue clearing with freeze/resume behavior.
4. Replace reset-time global clearing with Script Host Scope cancellation before
   each owner's `OnReset`.
5. Retain deferred resume events until pending pause/disable phases complete.
6. Make shutdown ordering explicit: abort invocations, forget contexts, cancel
   tasks, release task references, then release the engine.
7. Update Message request behavior to follow the same frame-freeze contract.
8. Cancel or freeze public execution handles according to the host transition;
   do not leave a suspended public handle able to resume after CK End.

Primary files:

- `src/ScriptAsync.h`, `src/ScriptAsync.cpp`
- `src/ScriptInvocation.cpp`
- `src/ScriptManager.cpp`
- `src/ScriptRuntime.cpp`
- `src/AngelScriptComponent.cpp`
- `src/AngelScriptExecutor.cpp`
- `src/ScriptMessage.cpp`

Exit gate:

- every Await contract test passes;
- pause/resume and reset sequences pass in Player validation;
- AngelScript garbage collection reports no retained wait/task cycles after
  hard teardown;
- no lifecycle event is silently dropped.

### Phase 2 — Make module mutation generation-safe

1. Add module-consumer queries to Component and Executor state stores.
2. Record attached generation in Component and Executor state for diagnostics.
3. Move preflight and post-commit consumer handling into the registry mutation
   transaction.
4. Reject public raw mutation with `CKAS_INUSE` for every live internal
   consumer.
5. Give Runtime Entry retirement and private Component retirement explicit
   owner-coordinated mutation origins.
6. Keep replacement staging transactional. Public raw mutation never detaches
   consumers; an owner-coordinated migration stages a generation-specific
   physical module before switching consumers.
7. Increment Component message incarnation on every runtime rebuild and validate
   module generation before direct message delivery.
8. Remove `ScriptManager::PrepareModuleMutation()` after all mutation paths use
   the deeper registry interface.

Primary files:

- `src/ScriptModuleRegistry.*`
- `src/ScriptModuleReplacer.*`
- `src/ScriptModuleMutationPolicy.*`
- `src/ScriptComponentState.*`
- `src/ScriptComponentStateStore.*`
- `src/ScriptExecutorState.*`
- `src/ScriptExecutorStateStore.*`
- `src/ScriptManagerModules.cpp`

Exit gate:

- failed candidate build and failed commit preserve the old generation and all
  consumers;
- public unload/replace with an attached Component or Executor returns
  `CKAS_INUSE` without changing generation;
- owner-coordinated Runtime Entry reload never executes an old generation after
  attachment to the new one;
- private Component rebuild does not block on its own ownership record.
- queued messages and retained typed targets cannot cross a Component runtime
  incarnation.

### Phase 3 — Consolidate lifecycle ordering

1. Extend `ScriptInitialLifecycle` to represent active phase, pending host
   events, desired enabled/paused state, and terminal invocation outcome.
2. Move ordering rules out of Runtime Entry, Component, and Executor callers.
3. Keep context construction, parameter injection, Behavior Frame refresh, and
   host-specific side effects in the adapters.
4. Delete duplicated pending flags only after parity tests pass for each entry
   kind.

Exit gate:

- the lifecycle transition matrix runs against both production adapters;
- entry-specific tests show no change outside the contracts fixed in Phases 1
  and 2;
- deletion of caller-side transition code reduces, rather than redistributes,
  lifecycle complexity.

### Phase 4 — Promote the test seam into normal CI

1. Add an engine-backed CTest target for scheduler, invocation, and module
   mutation tests that do not require Player.
2. Keep CK callback-order and real Building Block tests in the Player workflow.
3. Make Player validation required for changes touching lifecycle, async,
   Component, Executor, Message, or module mutation sources.
4. Keep API catalog validation as a separate compatibility gate.

Exit gate:

- ordinary CTest catches state-machine and module-generation regressions;
- Player validation is limited to behavior that genuinely requires the host;
- the interface used by tests is the same seam used by production adapters.

## Required Regression Matrix

### Async and lifecycle

| Scenario | Required result |
| --- | --- |
| Delay completes normally | Await resumes once with success |
| Pause while Delay is pending | No frames consumed while paused; wait resumes after remaining frames |
| `OnPause` suspends and host resumes | `OnPause` completes, then `OnResume` runs exactly once |
| Component `OnDisable` suspends and host resumes | Disable completes before re-enable; Component ends enabled |
| Reset while `Update` awaits | Old invocation is aborted; cancellation is not observed as success |
| `OnReset` creates and awaits a task | New task survives reset and completes before later lifecycle phases |
| Message request spans pause | Request and timeout freeze, then continue after resume |
| Shutdown while a context awaits | Context never resumes; no wait/task references leak |
| Public execution is suspended during pause | It remains suspended until processing resumes; no false completion |
| Public execution is suspended during reset/end | State becomes cancelled and later resume is rejected |

### Module mutation

| Scenario | Required result |
| --- | --- |
| Shared loaded module has an attached Component | Public replace/unload returns `CKAS_INUSE` |
| Mutation rejected as in-use | Generation, cache, imports, and consumer remain unchanged |
| Component is released, then module replaced | Replacement succeeds and generation increments once |
| Runtime Entry candidate fails to compile | Old Runtime Entry and consumers continue unchanged |
| Runtime Entry reload succeeds | Consumers attach to the new engine module name; old code no longer executes |
| Private Component source changes | Only the owning private module is retired and rebuilt |
| Module has a suspended async consumer | Mutation returns `CKAS_INUSE` |
| Component runtime is rebuilt with queued messages | Old message target is stale; old message is not delivered to the new object |
| Shutdown retires all modules | No stale consumer executes during or after engine release |

### Lifecycle transition matrix

Run each applicable phase with `Finished`, `Suspended`, and `Failed` outcomes
against Runtime Entry and Behavior Frame adapters. Cover at least:

- load → awake → enable → start → update;
- update → pause → disable → resume → enable → update;
- update → reset → start → update;
- update → disable → destroy;
- message → reset;
- execute → reset;
- pause immediately followed by end.

## Validation Commands and Gates

Each phase must pass, in order:

```powershell
ctest --test-dir build-current-release -C Release --output-on-failure
python tools\test_sync_api_catalog.py
python tools\sync_api_catalog.py --check
tools\Validate-Local.ps1 -BuildDir build-current-release -Configuration Release
```

Changes touching host transitions must also pass the Player Validation workflow
with startup self-tests enabled. A successful Core CTest alone is insufficient.

## Rollback Strategy

- Land Phase 0 tests separately when practical.
- Keep each production phase independently revertible.
- Do not combine public interface additions with lifecycle fixes.
- Preserve old behavior behind no compatibility flag when it violates the
  contracts above; a flag would make incorrect state erasure a supported mode.
- If Player traces contradict assumed callback order, revise the adapter
  sequencing while preserving the Await and serialization contracts.

## Definition of Done

The improvement is complete when:

- pause, reset, and shutdown no longer call one ambiguous scheduler `Clear()`;
- every suspended wait has an exactly-once terminal outcome or an explicit
  invocation abort;
- deferred resume/enable events cannot be lost behind pending pause/disable;
- public mutation accounts for public and internal consumers of a module
  generation;
- Component state records and checks its attached generation;
- Component runtime rebuild advances message incarnation and rejects stale
  delivery;
- public execution handles cannot survive reset/end in a resumable suspended
  state;
- lifecycle ordering is implemented once behind a tested interface and used by
  Runtime Entry and Behavior Frame adapters;
- normal CTest covers engine-backed scheduler and mutation behavior;
- Player validation covers only real CK callback and Building Block behavior;
- documentation and registered interfaces remain synchronized.
