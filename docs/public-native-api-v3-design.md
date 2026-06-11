# Public Native API v3 Design Draft

Status: draft for an unpublished breaking change.

This document defines the intended shape of the CKAngelScript native plugin API
before the public ABI is frozen. The design keeps AngelScript's host API as the
advanced integration surface and uses CKAngelScript only for the Virtools
boundary, diagnostics, module lifetime tracking, and a small set of safe helper
handles.

Decision: clean break. Because this API has not been published, v3 should not
carry deprecated aliases, compatibility shims, or ambiguous legacy names. Source
breaks are acceptable when they make ownership, lifetime, or execution semantics
clearer.

## Design Philosophy

The API should be boring at the boundary and sharp at the escape hatch. Most
callers should be able to stay on a small CKAngelScript-owned handle surface
with predictable diagnostics. Advanced callers should still be able to use
AngelScript directly, but raw access must be named as raw access and carry the
native lifetime rules visibly.

Use these principles to judge every v3 API decision:

- Mechanical sympathy with AngelScript. Do not hide AngelScript's execution
  model, context state machine, reference counting, or module ownership behind a
  fake simpler abstraction. CKAngelScript should align with the host API instead
  of fighting it.
- Boundary ownership is explicit. Names must say whether a pointer is borrowed,
  owned, or released through a specific API. A caller should not need to inspect
  implementation code to know who owns a value.
- Safe path first, raw path honest. CKAngelScript-owned handles provide stale
  checks and diagnostics for common plugin workflows. Raw AngelScript pointers
  stay available for expert integrations, but they are never presented as stable
  CKAngelScript identities.
- Make invalid combinations fail early. Option structs use exactly-one fields,
  unknown flags are invalid, and overloaded name lookup must either be unique or
  return `CKAS_AMBIGUOUS`.
- State transitions are part of the API. Execution handles have a visible state
  machine. Invalid transitions return `CKAS_INVALIDSTATE`, not a generic
  execution failure.
- Diagnostics are a contract. Every status-returning call has a consistent
  result path, out pointers are null on failure, and errors distinguish script
  execution failures from host-side API misuse.
- Pay only for the safety requested. The safe handle path owns metadata and
  performs generation checks. The raw path stays thin and does not add hidden
  lifetime tracking.
- Prefer compile-time breakage before publication. Because the API is not
  public yet, clean names and clear contracts matter more than compatibility
  aliases.
- No framework vanity. Do not introduce classes, factories, registries, or
  marshalling systems merely to look complete. Add abstraction only where it
  removes a real lifetime, state, or diagnostic hazard.

## Goals

- Make ownership and lifetime visible in function names and documentation.
- Keep the API small enough that advanced callers can still use AngelScript
  directly.
- Provide safe handles only where CKAngelScript can add real value: script
  function/method caching, script object lifetime, stale module detection,
  execution state, and diagnostics.
- Use one status/result convention for operations that can fail.
- Keep source and binary breakage acceptable while the API is unpublished.
- Prefer obvious compile failures over compatibility aliases when renaming API
  concepts.

## Non-goals

- Do not wrap every AngelScript type.
- Do not replace `asIScriptContext::Prepare`, `SetArg*`, `Execute`, and
  `GetReturn*` with a full marshalling layer.
- Do not promise a compiler-agnostic C ABI. The exported functions use C
  linkage, but the API intentionally passes AngelScript and Virtools C++ types.
  It is a native plugin ABI for the same toolchain/runtime family.
- Do not expose CKAngelScript internals or the `ScriptManager` C++ class.

## Terminology

Use these words consistently in names and docs:

| Term | Meaning |
| --- | --- |
| Borrowed | The caller receives a pointer owned by CKAngelScript, AngelScript, or Virtools. The caller must not release it. |
| Leased | The caller receives a context-like resource and must return it through the matching API that produced it. |
| Owned handle | The caller receives an opaque CKAngelScript handle and must release it through CKAngelScript. Owned handles may be symbol handles or runtime-state handles; their unload behavior is documented per handle type. |
| Generation | CKAngelScript's monotonically increasing module version used to reject stale handles after unload/rebuild. |
| Out pointer | A pointer-to-pointer output parameter. The callee writes `nullptr` before validation and leaves it null on failure. |
| Raw | Direct AngelScript/Virtools pointer access. Powerful, but lifetime-sensitive. |

## API Layers

The public header should make the intended layer obvious.

1. Runtime discovery and metadata.
2. Raw AngelScript access, explicitly named `Borrow...`.
3. Managed CKAngelScript handles for common safe calls.
4. Execution handles for functions that may suspend/resume.
5. Engine extension registration.
6. Thin C++ convenience wrappers and move-only RAII for CKAngelScript-owned
   handles only.

## API Shape Rules

Apply these rules before adding any v3 entry point:

- Any operation that can fail returns `CKAS_STATUS`.
- Any operation that creates, finds, or borrows a pointer writes it through an
  out pointer. It does not return the pointer directly.
- Pointer-returning functions are reserved for pure getters where failure has no
  useful diagnostic, such as `CKGetAngelScript()`.
- Option structs must have `Size` as their first field. Callers zero-initialize
  the struct and set `Size >= sizeof(v3_struct)`. `Size == 0` or a truncated
  struct is invalid; larger sizes are accepted and unknown future fields are
  ignored.
- Unknown flags are invalid and return `CKAS_INVALIDARGUMENT`.
- Fields that represent alternatives must use exactly-one semantics. Passing
  both alternatives or neither is `CKAS_INVALIDARGUMENT`.
- Name-based function or method lookup is valid only when the name resolves to
  exactly one overload. Ambiguous name lookup returns `CKAS_AMBIGUOUS`; callers
  should use declarations for overloaded APIs.
- Do not add `Optional` flags to lookup options. A missing symbol is
  `CKAS_NOTFOUND`; callers that treat absence as non-fatal should handle that
  status explicitly.
- Release functions return `CKAS_STATUS`. Releasing `nullptr` returns
  `CKAS_OK`. A non-null handle must belong to the same `CKAngelScript` manager
  that releases it.
- Public API calls should be made from the Virtools main thread unless a
  function explicitly documents a broader threading contract.

## Runtime And Features

Keep the opaque root handle:

```cpp
typedef struct CKAngelScript CKAngelScript;

#define CKAS_API_VERSION 3

CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context);
CKAS_API CKDWORD CKAngelScriptGetApiVersion(void);
CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript);
CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript);
CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript);
```

The v2 names are removed instead of kept as deprecated aliases. Callers should
fail at compile time and update to the v3 naming/lifetime model deliberately.

Replace the current capability names with feature names that describe the exact
surface area:

```cpp
typedef enum CKAS_FEATURE {
    CKAS_FEATURE_MODULE_LIFECYCLE = 1,
    CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS = 2,
    CKAS_FEATURE_FUNCTION_HANDLE = 3,
    CKAS_FEATURE_FUNCTION_EXECUTION = 4,
    CKAS_FEATURE_FUNCTION_EXECUTION_RESUME = 5,
    CKAS_FEATURE_OBJECT_HANDLE = 6,
    CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL = 7,
    CKAS_FEATURE_TYPED_ARG_READER_WRITER = 8,
    CKAS_FEATURE_STACK_TRACE = 9,
    CKAS_FEATURE_ENGINE_EXTENSION = 10
} CKAS_FEATURE;

CKAS_API CKBOOL CKAngelScriptHasFeature(CKAS_FEATURE feature);
```

Avoid broad names such as `ASYNC_RESUME`. Function execution resume and object
method execution resume are different promises. Do not advertise a feature until
that exact API path supports it.

`CKAngelScriptHasFeature()` reports whether this binary implements an API
surface. It is not a readiness check and should not depend on whether the
AngelScript engine is currently initialized. Runtime readiness failures are
reported by the specific API call, usually as `CKAS_NOTINITIALIZED`.

## Result Convention

Operations that can fail should return `CKAS_STATUS`, accept an optional
`CKAngelScriptResult *result`, and update the manager's last result. Functions
that are pure getters and have no meaningful diagnostics may return the value
directly and must document that they do not update the last result.

Invariants:

- `status == result->Status` whenever a result output is provided.
- Any API with a non-null out pointer must set `*out` to `nullptr` before
  validation and leave it null on failure.
- `CKAngelScriptGetLastResult()` is initialized to `CKAS_OK` after manager
  creation.
- Error strings in output results and `GetLastResult()` remain valid until the
  next API call that updates the last result.
- Error strings in `BorrowExecutionResult()` remain valid until the execution is
  released or started/resumed/cancelled again.
- Public entry points must not leave stale `ErrorMessage` or `StackTrace`
  pointers in a success result.

Keep existing status values unless there is a behavioral reason to break them.
Cosmetic enum renames are not worth the migration cost.

Add v3 statuses where they remove ambiguity:

| Status | Use |
| --- | --- |
| `CKAS_INVALIDSTATE` | The handle is valid but the requested state transition is invalid, such as starting an already-started execution. |
| `CKAS_INUSE` | A resource cannot be unloaded or replaced because live execution state depends on it. |
| `CKAS_ALREADYEXISTS` | A unique registration name is already present. |
| `CKAS_AMBIGUOUS` | Name lookup matched more than one overload. |
| `CKAS_FOREIGNHANDLE` | A handle belongs to a different CKAngelScript manager than the one used for the call. |

Keep `CKAS_EXECUTIONFAILED` for script execution failures and AngelScript
runtime failures, not host-side state machine errors.

## Raw AngelScript Access

The raw path is important and should stay first-class, but its names must be
honest:

```cpp
CKAS_API CKAS_STATUS CKAngelScriptBorrowEngine(CKAngelScript *angelScript,
                                               asIScriptEngine **outEngine,
                                               CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptBorrowActiveContext(CKAngelScript *angelScript,
                                                      asIScriptContext **outContext,
                                                      CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptBorrowModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               asIScriptModule **outModule,
                                               CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByName(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionName,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByDecl(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionDecl,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);
```

Rules:

- Borrowed `asIScriptEngine *`, `asIScriptModule *`, and `asIScriptFunction *`
  must not be released by the caller.
- If a caller wants to keep a borrowed function pointer, it must call
  `AddRef()` and later `Release()`. This only manages AngelScript reference
  counts. It does not provide CKAngelScript generation checks, unload blocking,
  or stale-handle diagnostics.
- Borrowed module/function pointers must not be treated as durable identities
  across CKAngelScript module unload, replace, or engine rebuild.
- Callers that need CKAngelScript lifetime protection for a module-level
  function must use `CKAngelScriptFunction`, not a borrowed
  `asIScriptFunction *`.
- Callers that need a context should prefer `engine->RequestContext()` and
  `engine->ReturnContext(ctx)`, because CKAngelScript installs engine context
  callbacks for pooling and exception setup.

`CKAngelScriptBorrowActiveContext()` is a special borrowed view over
AngelScript's current thread-local active context:

- It returns `CKAS_NOTFOUND` with a null out pointer when there is no active
  context for this CKAngelScript manager.
- If another AngelScript engine has an active context on the same thread, this
  API still returns `CKAS_NOTFOUND`; it never returns a context that belongs to a
  different manager.
- It is valid only while the current script call stack is active.
- The caller must not `Release()` it.
- The caller must not pass it to `ReturnContext()`.
- The caller must not store it for later use or use it from another thread.

Do not expose separate CKAngelScript context lease helpers in v3. Raw callers
who need a fresh context should borrow the engine and use AngelScript's
`RequestContext()`/`ReturnContext()` pair directly.

## Module Lifecycle

Keep load, compile, unload, and generation tracking. These APIs are where
CKAngelScript adds real value over raw AngelScript:

```cpp
CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                             const CKAngelScriptLoadOptions *options,
                                             CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptCompileModule(CKAngelScript *angelScript,
                                                const char *moduleName,
                                                const char *scriptCode,
                                                CKDWORD flags,
                                                CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnloadModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               CKAngelScriptResult *result);
CKAS_API CKBOOL CKAngelScriptHasModule(CKAngelScript *angelScript,
                                       const char *moduleName);
CKAS_API CKDWORD CKAngelScriptGetModuleGeneration(CKAngelScript *angelScript,
                                                  const char *moduleName);
```

Rules:

- Successful replace or unload bumps the module generation once.
- Failed replacement keeps the old module available and leaves generation
  unchanged.
- Owned CKAngelScript handles created from an older generation become stale.
- Unload/replace returns `CKAS_INUSE` while an object handle or execution handle
  still depends on that module.
- Function and method handles do not block unload because they are symbol
  handles, not retained AngelScript function references. Later use returns
  `CKAS_STALEHANDLE`.

## Managed Function Handles

Expose a narrow owned symbol handle for module-level functions. This is not a
wrapper around `asIScriptFunction` and it does not retain an AngelScript
function reference; it stores the lookup identity, owner manager, and module
generation. The implementation resolves the raw AngelScript function only when
creating execution state.

```cpp
typedef struct CKAngelScriptFunction CKAngelScriptFunction;

typedef struct CKAngelScriptFunctionOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *FunctionName;
    const char *FunctionDecl;
} CKAngelScriptFunctionOptions;

CKAS_API CKAS_STATUS CKAngelScriptFindFunction(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionOptions *options,
    CKAngelScriptFunction **outFunction,
    CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptReleaseFunction(CKAngelScript *angelScript,
                                                  CKAngelScriptFunction *function);
```

Rules:

- Exactly one of `FunctionName` or `FunctionDecl` should be set.
- `FunctionName` must resolve to exactly one overload.
- The handle captures the module generation at lookup time.
- Calls through a stale handle return `CKAS_STALEHANDLE`.
- Function handles do not block unload because they do not retain
  `asIScriptFunction`.
- `CKAngelScriptReleaseFunction()` must be called before unloading the host DLL
  that owns the handle wrapper code, but the handle itself owns only
  CKAngelScript metadata.

## Managed Object And Method Calls

Keep object and method handles. They are not a generic AngelScript wrapper; they
are a narrow convenience for plugin code that wants to call script class methods
without managing `asIScriptObject`, `asIScriptFunction`, and `asIScriptContext`
directly.

```cpp
typedef struct CKAngelScriptObject CKAngelScriptObject;
typedef struct CKAngelScriptMethod CKAngelScriptMethod;
typedef struct CKAngelScriptArgWriter CKAngelScriptArgWriter;
typedef struct CKAngelScriptResultReader CKAngelScriptResultReader;

CKAS_API CKAS_STATUS CKAngelScriptCreateObject(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectOptions *options,
    CKAngelScriptObject **outObject,
    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                CKAngelScriptObject *object);

CKAS_API CKAS_STATUS CKAngelScriptFindObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptMethodOptions *options,
    CKAngelScriptMethod **outMethod,
    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                CKAngelScriptMethod *method);

CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result);
```

`CKAngelScriptCallObjectMethod` is synchronous. It should return
`CKAS_UNSUPPORTED` if the method suspends, and the context must be aborted and
returned to the pool. Do not expose an object-method execution handle until that
path can preserve the prepared context and resume it correctly.

Object/method lookup rules:

- Object creation returns the new object through `outObject`.
- Method lookup returns the method through `outMethod`.
- Method options use exactly one of `MethodName` or `MethodDecl`.
- `MethodName` must resolve to exactly one overload.
- Method handles are symbol handles. They capture the object's class identity
  and module generation at lookup time, but do not retain
  `asIScriptFunction`.
- Object handles own live `asIScriptObject` state and block module unload or
  replace until released.

The typed writer/reader helpers stay intentionally small:

- `bool`
- `int`
- `float`
- `string`
- explicitly borrowed registered objects

Anything more complex should use the raw context path.

## Function Execution Handles

Keep an execution handle for module-level functions, because this path has
meaningful state: it can keep the AngelScript context alive across suspension
and resume it through the async scheduler.

Function execution is created from a managed function handle:

```cpp
typedef struct CKAngelScriptExecution CKAngelScriptExecution;

typedef CKAS_STATUS (*CKAngelScriptContextCallback)(asIScriptContext *context,
                                                   void *userData);

typedef struct CKAngelScriptFunctionExecutionOptions {
    CKDWORD Size;
    CKAngelScriptFunction *Function;
    const CKBehaviorContext *BehaviorContext;
    CKAngelScriptContextCallback ConfigureContext;
    CKAngelScriptContextCallback ReadResult;
    void *UserData;
    CKDWORD Flags;
} CKAngelScriptFunctionExecutionOptions;

CKAS_API CKAS_STATUS CKAngelScriptCreateFunctionExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionExecutionOptions *options,
    CKAngelScriptExecution **outExecution,
    CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                 CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                   CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptGetExecutionState(
    CKAngelScript *angelScript,
    const CKAngelScriptExecution *execution,
    CKAS_EXECUTIONSTATE *outState,
    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowExecutionResult(
    CKAngelScript *angelScript,
    const CKAngelScriptExecution *execution,
    const CKAngelScriptResult **outExecutionResult,
    CKAngelScriptResult *result);
```

Rules:

- `StartExecution` is valid only from `READY`.
- `ResumeExecution` is valid only from `SUSPENDED`.
- `CancelExecution` aborts any live context and transitions to `CANCELLED`;
  the API call returns `CKAS_OK` when the transition is accepted, while
  `BorrowExecutionResult()` reports `CKAS_CANCELLED`.
- `ReleaseExecution` returns any retained context to the engine pool.
- The execution captures the function handle's module generation at creation
  time and revalidates it before start/resume.
- `ConfigureContext` runs before first execution and can fail the execution by
  returning a non-OK status.
- `ReadResult` runs only after `asEXECUTION_FINISHED`, matching AngelScript's
  return value contract. A non-OK return from `ReadResult` becomes the public
  call status.
- `BorrowExecutionResult()` follows the out pointer rule. Invalid or foreign
  handles return a non-OK status and a null result pointer.

Function executions and object method calls share the same call flag vocabulary.
The only v3 call flag is `CKAS_CALL_NO_SUSPEND`; it should abort and return
`CKAS_UNSUPPORTED` if a script suspends. Do not expose hint flags until they
have enforced behavior.

## Engine Extensions

Keep the extension callback API, but do not retain caller-owned option memory.
Internally store extensions in a private C++ structure:

```cpp
struct EngineExtensionRegistration {
    std::string Name;
    CKAngelScriptEngineExtensionCallback Register = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_ENGINEEXTENSION_DEFAULT;
};

CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(
    CKAngelScript *angelScript,
    const CKAngelScriptEngineExtension *extension,
    CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(
    CKAngelScript *angelScript,
    const char *name,
    CKAngelScriptResult *result);
```

Rules:

- `Name` is copied.
- The callback pointer and `UserData` are retained until explicit unregister or
  manager shutdown.
- The host DLL that owns the callback code must unregister the extension before
  unloading.
- Extension identity is `Name`; duplicate names are rejected. Multi-instance
  integrations must choose distinct names. Duplicate registration returns
  `CKAS_ALREADYEXISTS`.
- Unregistering by `Name` removes the retained callback and `UserData`. It does
  not unregister AngelScript symbols already registered into the current engine.
- Immediate registration failure is reported to the caller but the extension
  stays registered for the next engine rebuild, unless a later design adds an
  explicit `REGISTER_ONCE` flag.
- No rollback is promised if a callback partially registers AngelScript symbols
  and then fails.

## C++ Convenience Layer

Keep `CKAngelScriptApi` as a small non-owning wrapper over the exported
functions. Add move-only RAII wrappers only for CKAngelScript-owned handles:

```cpp
class CKAngelScriptObjectHandle;
class CKAngelScriptFunctionHandle;
class CKAngelScriptMethodHandle;
class CKAngelScriptExecutionHandle;
```

Do not add RAII wrappers for borrowed `asIScriptEngine *`, `asIScriptModule *`,
`asIScriptFunction *`, or `asIScriptContext *`. Raw AngelScript already has its
own reference and context-return rules, and hiding those rules would make
advanced integration less predictable.

RAII wrapper rules:

- A wrapper stores the `CKAngelScript *` needed to call the matching release
  function.
- Move construction/assignment transfers the handle and clears the source.
- Destruction calls the status-returning release function and ignores the
  result.
- Wrappers must not outlive the owning CKAngelScript manager.

## Gaps And Pitfalls

This section lists the places where the design is intentionally strict because
the implementation is likely to drift into unsafe convenience otherwise.

| Area | Pitfall | Guardrail |
| --- | --- | --- |
| Native ABI wording | Calling the surface a generic C ABI can imply compiler independence even though AngelScript and Virtools C++ types cross the boundary. | Describe it as a C-linkage native plugin ABI for the same toolchain/runtime family. |
| Pointer-returning helpers | A convenient pointer return has no place to report `NOTFOUND`, `AMBIGUOUS`, `STALEHANDLE`, or `NOTINITIALIZED`. | Only pure getters may return pointers directly. Create/find/borrow operations return `CKAS_STATUS` and write through out pointers. |
| Out pointers | If failure leaves an old caller-owned value untouched, stale raw pointers are easy to reuse accidentally. | Null every non-null out pointer before validation and leave it null on failure. |
| Option struct sizing | `Size` fields are useful only if every entry point validates them consistently. | `Size` must be at least `sizeof(v3_struct)`. `Size == 0` and truncated structs are invalid. Larger future structs are accepted and only known fields are read. |
| Exactly-one inputs | Accepting both name and declaration invites accidental mismatch and hidden precedence rules. | Alternative fields are exactly-one. Both set or neither set returns `CKAS_INVALIDARGUMENT`. |
| Overload lookup | Name lookup can silently pick the wrong overload if AngelScript lookup rules change or new overloads are added. | Name lookup must resolve to exactly one function/method. Overloaded APIs should use declaration lookup. |
| Borrowed functions | `asIScriptFunction::AddRef()` can make a raw pointer look durable even though CKAngelScript generation checks and unload policy do not apply. | Borrowed function retention remains a raw AngelScript responsibility. CKAngelScript lifetime protection requires `CKAngelScriptFunction`. |
| Active context | The active context is thread-local and call-stack scoped, not a resource the caller owns. A thread can theoretically have an active context from another engine. | `BorrowActiveContext()` returns only contexts owned by the supplied CKAngelScript manager; otherwise it returns `CKAS_NOTFOUND` with a null out pointer. |
| Context pool | Adding CKAngelScript lease helpers would create a second ownership protocol beside AngelScript's `RequestContext()`/`ReturnContext()`. | Do not expose separate context lease helpers in v3. Raw callers borrow the engine and use AngelScript's protocol. |
| Module generation | Generations can be misread as ordered versions or persistent module IDs. | Treat generations as equality tokens only. `0` means unknown/no generation. Rebuild, unload, or replace invalidates function/method symbol handles. Object/execution handles block unload while live. |
| Module replacement | Destroying the old module before compiling the replacement can leave callers with no working module after a compile error. | Stage replacement first. If staging fails, keep the old module and generation. On success, commit the replacement and bump generation once. |
| Engine rebuild | Raw engine/module/function pointers can survive in caller code after a rebuild, but they no longer describe the current CKAngelScript world. | Raw pointers are never stable identities. Managed handles revalidate generation and engine ownership before use. |
| Execution state | Returning `CKAS_EXECUTIONFAILED` for invalid state transitions hides host-side API misuse as script failure. | Invalid transitions return `CKAS_INVALIDSTATE`; script exceptions/runtime failures use `CKAS_EXECUTIONFAILED`. |
| Suspension in sync calls | A synchronous call that suspends can leak a live context or accidentally become a half-async API. | `CKAS_CALL_NO_SUSPEND` aborts and returns `CKAS_UNSUPPORTED`. Synchronous object-method calls abort and return the context when suspension happens. |
| Cancel semantics | `CancelExecution()` can be confused with the execution result. | Accepted cancellation returns `CKAS_OK`; `BorrowExecutionResult()` records `CKAS_CANCELLED`. |
| Callback failures | `ConfigureContext`, `ReadResult`, argument writers, result readers, and engine extensions can fail after partially mutating state. | Callbacks return `CKAS_STATUS` or AngelScript error codes. Do not throw exceptions across the API boundary. No automatic rollback is promised unless documented. |
| Result lifetime | Borrowed diagnostic strings can dangle if callers store them past the next API call. | Result strings are borrowed and have documented invalidation points. Callers copy them for longer storage. |
| Manager identity | Releasing a handle through the wrong `CKAngelScript` manager can otherwise become a silent leak or corruption. | Handles record their owner. Status-returning APIs, including release functions, return `CKAS_FOREIGNHANDLE` when the owner does not match. |
| Host DLL unload | Engine extension callbacks and C++ RAII wrapper code may live in a DLL that unloads before CKAngelScript releases retained state. | Host plugins must unregister engine extensions and release CKAngelScript handles before unloading. |
| Threading | AngelScript can be prepared for multithreading, but Virtools object access generally is not thread-safe. | Public CKAngelScript API is main-thread-only unless an entry point explicitly documents otherwise. |
| Feature probing | Feature checks can become readiness checks and produce unstable answers depending on engine initialization. | `HasFeature()` describes binary API support only. Runtime readiness is reported by the called API. |
| Raw borrowed object args | Passing host objects as borrowed script arguments can outlive callback-local data if used on persistent execution paths. | Borrowed object args are synchronous-only unless a future API defines ownership transfer or retention. |
| C++ RAII | RAII wrappers can make manager lifetime look safer than it is. | RAII wrappers own only CKAngelScript handles, not the manager. They must not outlive `CKAngelScript`. |
| Tests | The API can look correct in header form while state and lifetime invariants regress in implementation. | Add self-tests for out pointer nulling, stale generation, invalid transitions, ambiguous lookup, unload `CKAS_INUSE`, and result lifetime. |

## Implementation Plan

1. Bump `CKAS_API_VERSION` to 3 and remove old public names that conflict with
   the v3 model.
2. Rename and document raw accessors as borrowed/status-returning APIs.
3. Remove pointer-returning raw lookup functions:
   `CKAngelScriptGetScriptEngine`, `CKAngelScriptGetActiveContext`,
   `CKAngelScriptGetModule`, `CKAngelScriptFindFunctionByName`, and
   `CKAngelScriptFindFunctionByDecl`.
4. Replace `CKAngelScriptHasCapability`/`CKAS_APICAPABILITY` with
   `CKAngelScriptHasFeature`/`CKAS_FEATURE`.
5. Add `CKAngelScriptFunction` and route function execution creation through
   that managed symbol handle.
6. Remove `Optional` from function and method lookup options.
7. Rename `CKAngelScriptCreateExecution` to
   `CKAngelScriptCreateFunctionExecution` and replace
   `CKAngelScriptExecuteOptions` with `CKAngelScriptFunctionExecutionOptions`.
8. Remove or hide `CKAngelScriptCreateObjectMethodExecution` until
   object-method resume is genuinely implemented.
9. Initialize and normalize `m_LastResult`; make every status-returning public
   entry point update it consistently.
10. Make every out pointer API null the output before validation and keep it null
   on failure.
11. Replace retained `CKAngelScriptEngineExtension` values with a private
   registration structure that copies `Name`.
12. Enforce `NO_SUSPEND` in function executions and synchronous object-method
    calls.
13. Remove unimplemented public call hint flags until they have real behavior.
14. Make function and method handles symbol handles that do not retain
    AngelScript function references and do not block unload.
15. Make object and execution handles block module unload/replace while live.
16. Save module generation in function/method symbol handles and execution
    handles; revalidate on function execution start/resume and object method
    call.
17. Make release functions status-returning and report `CKAS_FOREIGNHANDLE` for
    owner mismatches.
18. Stage module replacements so compile/load failure preserves the old module
    and generation.
19. Add C++ move-only RAII wrappers for CKAngelScript object, function, method,
    and execution handles.
20. Update `docs/public-api.md`, `docs/api-inventory.md`, and self-tests after
    the header settles.

## Open Decisions

None before the v3 header pass. Implementation details should be converted into
tracked tasks instead of left as API-level ambiguity.
