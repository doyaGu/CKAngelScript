# Public API

External plugins use the C ABI declared in `include/CKAngelScript.h`. The ABI is an opaque `CKAngelScript *` plus `CKAngelScript...` functions. It does not expose the internal `ScriptManager` C++ class or a virtual C++ interface.

For C++ callers, the same header provides `CKAngelScriptApi`, a small non-owning wrapper over the C functions, plus move-only RAII wrappers for object/function/method/execution handles.

## Getting CKAngelScript

```cpp
#include "CKAngelScript.h"

CKAngelScript *angelScript = CKGetAngelScript(context);
if (!angelScript) {
    return;
}
```

With the C++ wrapper:

```cpp
CKAngelScriptApi api = CKAngelScriptApi::Get(context);
if (!api.IsValid()) {
    return;
}
```

The handle is owned by CKAngelScript. Do not allocate, delete, or cast it to CKAngelScript internals.

Use `CKAngelScriptGetApiVersion()` and `CKAngelScriptHasFeature()` before consuming optional ABI surfaces from a soft loader. `CKAS_FEATURE` describes the binary API surface only; it does not mean the script engine is initialized or a module is loaded. Feature names are exact: module lifecycle, raw AngelScript access, function handles/execution/resume, object handles, object type namespace lookup, synchronous object method calls, typed arg/result helpers, stack traces, engine extensions, public struct initializers, status text, metadata reflection, script array access, active-context host exceptions, source-section loading, object handle argument writing, host-call filtering, module imports, module bytecode, and module replacement transactions.

## Optional Plugin Integration

Virtools plugins that should keep working without `AngelScript.dll` should soft-load the extension API instead of linking directly to the DLL. `CKAngelScriptExtensionApi` is a small header-only function table for that path:

```cpp
#include <windows.h>
#include "CKAngelScript.h"

static CKAngelScriptRawProc ResolveCkasSymbol(void *module, const char *name) {
    return reinterpret_cast<CKAngelScriptRawProc>(
        GetProcAddress(static_cast<HMODULE>(module), name));
}

HMODULE module = GetModuleHandleA("AngelScript.dll");
CKAngelScriptExtensionApi ckas;
if (!module || !CKAngelScriptLoadExtensionApi(&ckas, ResolveCkasSymbol, module)) {
    return; // CKAngelScript is optional for this plugin.
}

CKAngelScriptResult result;
CKAS_STATUS status =
    CKAngelScriptRegisterEngineExtensionWithApi(
        &ckas,
        context,
        "BML",
        RegisterBmlApi,
        bridge,
        CKAS_ENGINEEXTENSION_DEFAULT,
        &result);
```

Call `CKAngelScriptUnregisterEngineExtensionWithApi(&ckas, context, "BML", &result)` before the owning plugin state is destroyed. The v5 helper table also resolves module import and bytecode entry points for optional integrations that need script-library mechanics without taking a hard DLL link dependency.

## Handle Model

The v3 API deliberately separates borrowed AngelScript pointers from CKAngelScript handles:

- `CKAngelScriptBorrowEngine`, `CKAngelScriptBorrowActiveContext`, `CKAngelScriptBorrowModule`, and `CKAngelScriptBorrowFunctionBy*` return borrowed `asIScript*` pointers through out parameters. CKAngelScript does not `AddRef` them. Callers must not release them.
- `CKAngelScriptFunction` and `CKAngelScriptMethod` are symbol handles. They store owner, module/class identity, lookup key, and module generation. They do not retain `asIScriptFunction *` and do not block unload.
- `CKAngelScriptObject` owns a live `asIScriptObject`. It blocks unload/replace of its module until released.
- `CKAngelScriptExecution` owns runtime execution state. It blocks unload/replace of its module until released.

Borrowed raw AngelScript pointers are escape hatches. Directly mutating module import bindings through `asIScriptModule` bypasses CKAngelScript's import dependency tracking, so provider reload blocking and reload diagnostics only cover bindings made through `CKAngelScriptBindImportedFunction()` or `CKAngelScriptBindAllImportedFunctions()`.

All out pointer APIs clear the out pointer before validation and leave it null on failure.

All public options and engine-extension structs must be zero-initialized and set `Size >= sizeof(v3_struct)`. `Size == 0` or a truncated struct returns `CKAS_INVALIDARGUMENT`; larger sizes are accepted so future struct extensions can be ignored by older binaries.

## Callback User Data Lifetime

CKAngelScript never owns callback `UserData`. The caller is always responsible for storage lifetime, but each API retains it for a different duration:

| Callback path | Lifetime contract |
| --- | --- |
| `CKAngelScriptEnumerateMetadata` | `userData`, `CKAngelScriptMetadataEntry`, and metadata strings are borrowed only for the current enumeration callback. Copy anything that must survive the callback. Module mutation from the callback returns `CKAS_INVALIDSTATE`. |
| `CKAngelScriptEnumerateImportedFunctions` | `userData`, `CKAngelScriptImportEntry`, and import strings are borrowed only for the current enumeration callback. Copy anything that must survive the callback. Module mutation from the callback returns `CKAS_INVALIDSTATE`. |
| `CKAngelScriptSaveModuleBytecode` / `CKAngelScriptLoadModuleBytecode` | `Write`, `Read`, and `UserData` are borrowed only until the bytecode call returns. Module mutation from these callbacks returns `CKAS_INVALIDSTATE`. |
| `CKAngelScriptCallObjectMethod` | `WriteArgs`, `ReadResult`, and `UserData` are borrowed only until the synchronous call returns. Do not store `CKAngelScriptArgWriter` or `CKAngelScriptResultReader` pointers. |
| `CKAngelScriptStartExecution` / `CKAngelScriptResumeExecution` | `ConfigureContext`, `ReadResult`, and `UserData` from `CKAngelScriptExecutionStepOptions` are borrowed only for that one start/resume call. Suspend does not retain them; pass fresh step options when resuming. |
| `CKAngelScriptSetHostCallFilter` | `callback` and `userData` are retained until replaced, cleared, or the CKAngelScript manager is destroyed. The callback must not execute script or unload/replace modules. |
| `CKAngelScriptRegisterEngineExtension` | `Register` and `UserData` are retained until `CKAngelScriptUnregisterEngineExtension` succeeds or the CKAngelScript manager is destroyed. If immediate registration fails, they are not retained. |

`CKAngelScriptFunctionExecutionOptions::BehaviorContext` is the exception: CKAngelScript copies it into the execution handle during `CreateFunctionExecution`, so the source pointer only needs to survive that call.

All option struct strings are call-scope. `LoadOptions`, `FunctionOptions`, `ObjectOptions`, and `MethodOptions` borrow caller strings only for the API call; public handles copy the identity they need to survive unload/reload checks.

## Initialization And Diagnostics

Use the initializer helpers instead of writing `Size` by hand:

```c
CKAngelScriptResult result;
CKAngelScriptInitResult(&result);

CKAngelScriptFunctionOptions find;
CKAngelScriptInitFunctionOptions(&find);
find.ModuleName = "example_api";
find.FunctionDecl = "int add_five(int)";

CKAngelScriptFunction *function = NULL;
CKAS_STATUS status = CKAngelScriptFindFunction(angelScript, &find, &function, &result);
if (status != CKAS_OK) {
    const char *name = CKAngelScriptGetStatusName(status);
    const char *description = CKAngelScriptGetStatusDescription(status);
}
```

Initializer helpers are null-safe no-ops. Status text helpers return static strings, never allocate, and do not update `CKAngelScriptGetLastResult()`.

## Host-Call Filtering

`CKAngelScriptSetHostCallFilter()` installs a manager-wide synchronous filter for CKAngelScript-provided host calls. It is intended for embedders that need phase gates, such as blocking world-mutating APIs while a script runtime is being hot-reloaded.

```cpp
CKAS_STATUS FilterHostCall(const char *apiName, CKDWORD flags, void *userData) {
    if ((flags & CKAS_HOSTCALL_MUTATES_HOST_STATE) != 0 && IsInRestrictedPhase(userData)) {
        return CKAS_INVALIDSTATE;
    }
    return CKAS_OK;
}

CKAngelScriptResult result;
CKAngelScriptInitResult(&result);
CKAngelScriptSetHostCallFilter(angelScript, FilterHostCall, hostState, &result);
```

The filter is not a sandbox and does not cover raw Virtools SDK bindings unless those bindings explicitly call into it. A non-`CKAS_OK` return rejects the host call; callers that need a script exception should set one with `CKAngelScriptSetActiveContextException()` from the active script thread.

## Module Lifecycle

```cpp
CKAngelScriptResult result;
CKAngelScriptInitResult(&result);
CKAS_STATUS status =
    CKAngelScriptCompileModule(angelScript,
                              "example_api",
                              "int add_five(int value) { return value + 5; }",
                              CKAS_COMPILE_REPLACEEXISTING,
                              &result);
```

`CKAngelScriptLoadOptions` accepts exactly one source: `Filename`, `Filenames` plus `FileCount`, `Code`, or `Sections` plus `SectionCount`. `Sections[0]` is the entry section; the remaining sections are an in-memory include snapshot used by `#include` resolution and are not automatically compiled unless included. Unknown flags return `CKAS_INVALIDARGUMENT`. Replacing an existing module requires `CKAS_LOAD_REPLACEEXISTING` or `CKAS_COMPILE_REPLACEEXISTING`; without replace, duplicates return `CKAS_ALREADYEXISTS`.

Module replacement is atomic at the public API boundary: if the replacement source fails to compile or load, the old module remains available and its generation is unchanged. Successful replacement bumps the generation once.

`CKAngelScriptGetModuleGeneration()` returns the generation tracked by CKAngelScript. Symbol handles created before an unload/replace later fail with `CKAS_STALEHANDLE`.

## Module Imports And Bytecode

CKAngelScript exposes AngelScript function imports as explicit host-controlled bindings. Use `CKAngelScriptGetImportedFunctionCount()` or `CKAngelScriptEnumerateImportedFunctions()` to inspect a module's imports. `CKAngelScriptBindImportedFunction()` binds one import by index; if `SourceModuleName` or `FunctionDecl` is omitted, CKAngelScript uses the source module and declaration written in the script import. `CKAngelScriptBindAllImportedFunctions()` first resolves every import, then commits the bindings; missing source modules or functions return `CKAS_NOTFOUND` without changing the importing module. Incompatible bindings return `CKAS_TYPEMISMATCH`.

```angelscript
import int add_score(int value) from "score_lib";
```

```cpp
CKAngelScriptImportBindOptions bind = CKAngelScriptApi::ImportBindOptions("consumer", 0);
CKAS_STATUS status = CKAngelScriptBindImportedFunction(angelScript, &bind, &result);
```

`CKAngelScriptUnbindImportedFunction()` and `CKAngelScriptUnbindAllImportedFunctions()` remove bindings from the importing module. Bind and unbind operations change the importing module generation, so old consumer symbol/execution handles become stale. Bindings point at the provider function resolved at bind time; replacing or unloading a provider module with CKAngelScript-tracked bound consumers returns `CKAS_INUSE`. Reload coordinators should replace or unbind affected consumers before replacing the provider, then rebind consumers after provider generation changes.

Bytecode APIs use caller-provided read/write callbacks, so CKAngelScript never allocates byte buffers that the caller must free:

```cpp
CKAngelScriptBytecodeSaveOptions save =
    CKAngelScriptApi::BytecodeSaveOptions("score_lib", WriteBytes, userData);
CKAS_STATUS status = CKAngelScriptSaveModuleBytecode(angelScript, &save, &result);
```

`CKAngelScriptLoadModuleBytecode()` creates a module from bytecode. Loading over an existing module requires `CKAS_BYTECODE_REPLACEEXISTING`; without it, duplicates return `CKAS_ALREADYEXISTS`. Live `CKAngelScriptObject` or `CKAngelScriptExecution` handles block bytecode replacement with `CKAS_INUSE`. Replacement is transactional at the public API boundary: CKAngelScript first loads the bytecode into a transient module, snapshots it, then commits it under the requested module name. Failed loads or failed commits leave the previous module and generation unchanged when rollback succeeds. Successful bytecode replacement bumps module generation and stale-checks old symbol handles.

AngelScript bytecode is not a stable interchange format. A persistent cache key must include at least the CKAngelScript API version, AngelScript version, engine options, build flags, and a hash of the registered host API surface. Do not reuse bytecode across CKAngelScript or registration changes unless the cache key proves compatibility.

## Function Lookup And Execution

Create a symbol handle, then create execution handles from it:

```cpp
CKAngelScriptFunctionOptions find;
CKAngelScriptInitFunctionOptions(&find);
find.ModuleName = "example_api";
find.FunctionDecl = "int add_five(int)";

CKAngelScriptFunction *function = nullptr;
CKAS_STATUS status = CKAngelScriptFindFunction(angelScript, &find, &function, &result);
```

`FunctionName` and `FunctionDecl` are exactly-one options. Name lookup returns `CKAS_AMBIGUOUS` when multiple overloads match. `FunctionDecl` lookup is preferred for overloads.

The C++ wrapper keeps the same status-returning model but removes the manual release path for common use:

```cpp
CKAngelScriptResult result = CKAngelScriptApi::Result();
CKAngelScriptFunctionHandle function;
CKAS_STATUS status =
    api.FindFunctionByDecl("example_api", "int add_five(int)", function, &result);
if (!function) {
    const char *name = CKAngelScriptApi::StatusName(status);
}
```

```cpp
struct AddData {
    int Input = 0;
    int Output = 0;
};

CKAS_STATUS ConfigureAdd(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<AddData *>(userData);
    return ctx->SetArgDWord(0, static_cast<asDWORD>(data->Input)) >= 0
        ? CKAS_OK
        : CKAS_EXECUTIONFAILED;
}

CKAS_STATUS ReadAddResult(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<AddData *>(userData);
    data->Output = static_cast<int>(ctx->GetReturnDWord());
    return CKAS_OK;
}

AddData data;
data.Input = 37;

CKAngelScriptFunctionExecutionOptions exec =
    CKAngelScriptApi::FunctionExecutionOptions(function);
CKAngelScriptExecutionStepOptions step =
    CKAngelScriptApi::ExecutionStepOptions(ConfigureAdd, ReadAddResult, &data);

CKAngelScriptExecution *execution = nullptr;
status = CKAngelScriptCreateFunctionExecution(angelScript, &exec, &execution, &result);
if (status == CKAS_OK) {
    status = CKAngelScriptStartExecution(angelScript, execution, &step, &result);
    CKAngelScriptReleaseExecution(angelScript, execution, &result);
}
CKAngelScriptReleaseFunction(angelScript, function, &result);
```

`StartExecution` only accepts `CKAS_EXECUTION_READY`. `ResumeExecution` only accepts `CKAS_EXECUTION_SUSPENDED`. Invalid transitions return `CKAS_INVALIDSTATE`. On resume, `ConfigureContext` is an optional raw context hook called before the suspended context continues; it must not rewrite initial arguments that have already been consumed. `ReadResult` is called only when the script reaches a finished state, not when it suspends. `CancelExecution` returns `CKAS_OK` when accepted and stores `CKAS_CANCELLED` in the execution result. If `CKAS_CALL_NO_SUSPEND` is set and a script suspends, CKAngelScript aborts the context and returns `CKAS_UNSUPPORTED`.

## Object And Method Calls

Object/method APIs are for synchronous class method calls without exposing `asIScriptObject`, `asIScriptFunction`, or `asIScriptContext` directly:

```cpp
CKAngelScriptObjectOptions objectOptions =
    CKAngelScriptApi::ObjectOptions("example_api", "ExampleMod");

CKAngelScriptObject *object = nullptr;
CKAngelScriptCreateObject(angelScript, &objectOptions, &object, &result);

CKAngelScriptMethodOptions methodOptions =
    CKAngelScriptApi::MethodByDeclOptions(object, "int Add(int)");

CKAngelScriptMethod *method = nullptr;
CKAngelScriptFindObjectMethod(angelScript, &methodOptions, &method, &result);
```

For namespaced types, use `ObjectOptionsByNamespace("module", "MyNamespace", "ExampleMod")`
or `ObjectOptionsByDecl("module", "MyNamespace::ExampleMod")`. `ClassNamespace`
cannot be combined with `TypeDecl`. Soft loaders that set `ClassNamespace` must require
`CKAS_FEATURE_OBJECT_TYPE_NAMESPACE`; older CKAngelScript builds may accept the smaller
v3 object options struct but ignore the namespace field.

This is the v3 baseline object identity model. CKAngelScript does not expose
create-by-metadata-entry or create-by-AngelScript-type-id APIs; metadata
reflection is for discovery and diagnostics, while object creation uses
`ModuleName + ClassName + optional ClassNamespace` or `ModuleName + TypeDecl`.
Object method lookup stays scoped to a live `CKAngelScriptObject` handle, so
method handles inherit the object's module, type namespace, and generation.

`MethodName` and `MethodDecl` are exactly-one options. Missing symbols return `CKAS_NOTFOUND`; overloaded name lookup returns `CKAS_AMBIGUOUS`.

Arguments and return values are written through callbacks:

```cpp
struct CallData {
    int Input = 0;
    int Output = 0;
};

CKAS_STATUS WriteAddArgs(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<CallData *>(userData);
    return CKAngelScriptArgSetInt(writer, 0, data->Input);
}

CKAS_STATUS ReadAddResult(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<CallData *>(userData);
    return CKAngelScriptResultGetInt(reader, &data->Output);
}

CKAngelScriptObjectMethodExecuteOptions call =
    CKAngelScriptApi::ObjectMethodExecuteOptions(
        object,
        method,
        WriteAddArgs,
        ReadAddResult,
        &data,
        CKAS_CALL_NO_SUSPEND);

CKAS_STATUS status = CKAngelScriptCallObjectMethod(angelScript, &call, &result);
```

Use `CKAngelScriptArgSetBorrowedObject` only for borrowed object reference
parameters such as `const T &in`. Use `CKAngelScriptArgSetObjectHandle` for
handle parameters such as `T@`; it accepts null handles and relies on
AngelScript's normal handle AddRef/Release behavior for the duration of the
call.

Typed helpers support `bool`, `int`, `float`, `string`, and explicitly borrowed host objects. Borrowed object arguments are for callback-local views already registered with the engine and must target read-only input references.

Release handles when the host no longer needs them:

```cpp
CKAngelScriptReleaseMethod(angelScript, method, &result);
CKAngelScriptReleaseObject(angelScript, object, &result);
```

`CKAngelScriptCreateObjectMethodExecution` is intentionally not exported in v3. Object-method resume is not part of the public ABI until it is implemented honestly.

## Raw Borrowing

Raw access is available for plugins that intentionally want AngelScript primitives:

```cpp
asIScriptModule *module = nullptr;
CKAS_STATUS status =
    CKAngelScriptBorrowModule(angelScript, "example_api", &module, &result);
```

Borrowed pointers are only valid under normal AngelScript/module lifetime rules. CKAngelScript does not extend their lifetime. `BorrowActiveContext` only returns a context whose engine belongs to the same CKAngelScript manager; otherwise it returns `CKAS_NOTFOUND`.

## Metadata Reflection

CKAngelScript preserves AngelScript `CScriptBuilder` metadata for compiled/loaded modules and exposes it through enumeration:

```cpp
CKAS_STATUS OnMetadata(const CKAngelScriptMetadataEntry *entry,
                       CKDWORD index,
                       const char *metadata,
                       void *userData) {
    // Copy data you need to keep after this callback returns.
    return CKAS_OK;
}

CKAS_STATUS status =
    CKAngelScriptEnumerateMetadata(angelScript, "example_api", OnMetadata, userData, &result);
```

`CKAngelScriptMetadataEntry::Target` identifies whether the metadata belongs to a type, type method, global function, global variable, or type property. `Name`, `Namespace`, `Declaration`, `ParentTypeName`, and `ParentTypeNamespace` are borrowed strings valid only for the callback duration. Returning any status other than `CKAS_OK` stops enumeration and becomes the API return status.

This API is intentionally a reflection primitive. CKAngelScript does not define host-specific script mod manifests, namespace rules, or discovery policy; applications such as BML should consume standard AngelScript metadata and apply their own policy above this boundary.

## Engine Extensions

Host plugins can register additional AngelScript APIs without adding host-specific code to CKAngelScript itself:

```cpp
CKAngelScriptEngineExtension extension;
CKAngelScriptInitEngineExtension(&extension);
extension.Name = "BML";
extension.Register = RegisterBmlApi;
extension.UserData = bridge;

CKAngelScriptRegisterEngineExtension(angelScript, &extension, &result);
```

CKAngelScript copies the extension name internally and retains the callback for future engine rebuilds. Duplicate names return `CKAS_ALREADYEXISTS`. Successful extension registration is wrapped in an AngelScript config group named from the extension; callbacks must not call `BeginConfigGroup` or `EndConfigGroup`. `CKAngelScriptUnregisterEngineExtension(angelScript, name, result)` removes that config group from the current engine and deletes the retained callback. If loaded modules or live script objects still reference the extension surface, unregister returns `CKAS_INUSE` and keeps the retained callback.

Callbacks return `>= 0` on success and `< 0` on failure. If `errorMessage` is set, CKAngelScript copies it into registration diagnostics. A failing immediate callback returns `CKAS_EXECUTIONFAILED`, rolls back its config group, and is not retained.

## Result Lifetime

`CKAngelScriptResult::ErrorMessage` and `StackTrace` are borrowed strings.

- Output-parameter results and `CKAngelScriptGetLastResult()` remain valid until the next CKAngelScript API call that updates the last result.
- `CKAngelScriptBorrowExecutionResult()` strings remain valid until the execution handle is released or started/resumed/cancelled again.
- If a status-returning C entry point is called with an invalid `CKAngelScript *`, the output result is filled from static diagnostics when a result pointer is supplied; no manager exists, so `CKAngelScriptGetLastResult()` is not updated.

Copy strings if they must outlive those boundaries.

Typed result helpers clear their writable out parameters before validation. `CKAngelScriptResultGetString()` reports the required size for `CKAS_BUFFERTOOSMALL`, but clears the buffer before copying.

`CKAngelScriptGetStatusName()` and `CKAngelScriptGetStatusDescription()` return static text for every `CKAS_STATUS`; unknown numeric values return stable fallback strings.

## Unload Constraints

`CKAngelScriptObject` and `CKAngelScriptExecution` handles block unload/replace of their module and cause `CKAS_INUSE`. Release them before unloading or replacing a module.

`CKAngelScriptFunction` and `CKAngelScriptMethod` symbol handles do not block unload. They become stale across unload/replace and later use returns `CKAS_STALEHANDLE`.

## Status Values

| Status | Meaning |
| --- | --- |
| `CKAS_OK` | Operation succeeded. |
| `CKAS_INVALIDARGUMENT` | Bad options, unknown flags, null out pointer, or missing required fields. |
| `CKAS_NOTINITIALIZED` | CKAngelScript or the engine is not ready. |
| `CKAS_NOTFOUND` | Module or symbol was not found. |
| `CKAS_COMPILEERROR` | Module compilation failed. |
| `CKAS_EXECUTIONFAILED` | Runtime execution failed. |
| `CKAS_SUSPENDED` | Execution awaits async work. |
| `CKAS_CANCELLED` | Execution result was cancelled. |
| `CKAS_STALEHANDLE` | Symbol/execution handle belongs to an old module generation. |
| `CKAS_UNSUPPORTED` | Requested mode is not implemented, such as no-suspend on a suspending script. |
| `CKAS_TYPEMISMATCH` | Argument writer or result reader type does not match the target declaration. |
| `CKAS_BUFFERTOOSMALL` | String result buffer is too small; inspect required size. |
| `CKAS_INVALIDSTATE` | The handle state does not allow this transition. |
| `CKAS_INUSE` | Module unload/replace or extension unregister is blocked by live references. |
| `CKAS_ALREADYEXISTS` | Module or extension already exists. |
| `CKAS_AMBIGUOUS` | Name lookup matched multiple overloads. |
| `CKAS_FOREIGNHANDLE` | Handle belongs to another CKAngelScript manager. |
