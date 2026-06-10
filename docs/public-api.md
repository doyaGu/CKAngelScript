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

Use `CKAngelScriptGetApiVersion()` and `CKAngelScriptHasFeature()` before consuming optional ABI surfaces from a soft loader. `CKAS_FEATURE` describes the binary API surface only; it does not mean the script engine is initialized or a module is loaded.

## Handle Model

The v3 API deliberately separates borrowed AngelScript pointers from CKAngelScript handles:

- `CKAngelScriptBorrowEngine`, `CKAngelScriptBorrowActiveContext`, `CKAngelScriptBorrowModule`, and `CKAngelScriptBorrowFunctionBy*` return borrowed `asIScript*` pointers through out parameters. CKAngelScript does not `AddRef` them. Callers must not release them.
- `CKAngelScriptFunction` and `CKAngelScriptMethod` are symbol handles. They store owner, module/class identity, lookup key, and module generation. They do not retain `asIScriptFunction *` and do not block unload.
- `CKAngelScriptObject` owns a live `asIScriptObject`. It blocks unload/replace of its module until released.
- `CKAngelScriptExecution` owns runtime execution state. It blocks unload/replace of its module until released.

All out pointer APIs clear the out pointer before validation and leave it null on failure.

## Module Lifecycle

```cpp
CKAngelScriptResult result = {};
CKAS_STATUS status =
    CKAngelScriptCompileModule(angelScript,
                              "example_api",
                              "int add_five(int value) { return value + 5; }",
                              CKAS_COMPILE_REPLACEEXISTING,
                              &result);
```

`CKAngelScriptLoadOptions` accepts exactly one source: `Filename`, `Filenames` plus `FileCount`, or `Code`. Unknown flags return `CKAS_INVALIDARGUMENT`. Replacing an existing module requires `CKAS_LOAD_REPLACEEXISTING` or `CKAS_COMPILE_REPLACEEXISTING`; without replace, duplicates return `CKAS_ALREADYEXISTS`.

`CKAngelScriptGetModuleGeneration()` returns the generation tracked by CKAngelScript. Symbol handles created before an unload/replace later fail with `CKAS_STALEHANDLE`.

## Function Lookup And Execution

Create a symbol handle, then create execution handles from it:

```cpp
CKAngelScriptFunctionOptions find = CKAngelScriptApi::FunctionOptions();
find.ModuleName = "example_api";
find.FunctionDecl = "int add_five(int)";

CKAngelScriptFunction *function = nullptr;
CKAS_STATUS status = CKAngelScriptFindFunction(angelScript, &find, &function, &result);
```

`FunctionName` and `FunctionDecl` are exactly-one options. Name lookup returns `CKAS_AMBIGUOUS` when multiple overloads match. `FunctionDecl` lookup is preferred for overloads.

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
    CKAngelScriptApi::FunctionExecutionOptions();
exec.Function = function;
exec.ConfigureContext = ConfigureAdd;
exec.ReadResult = ReadAddResult;
exec.UserData = &data;

CKAngelScriptExecution *execution = nullptr;
status = CKAngelScriptCreateFunctionExecution(angelScript, &exec, &execution, &result);
if (status == CKAS_OK) {
    status = CKAngelScriptStartExecution(angelScript, execution);
    CKAngelScriptReleaseExecution(angelScript, execution);
}
CKAngelScriptReleaseFunction(angelScript, function);
```

`StartExecution` only accepts `CKAS_EXECUTION_READY`. `ResumeExecution` only accepts `CKAS_EXECUTION_SUSPENDED`. Invalid transitions return `CKAS_INVALIDSTATE`. `CancelExecution` returns `CKAS_OK` when accepted and stores `CKAS_CANCELLED` in the execution result. If `CKAS_CALL_NO_SUSPEND` is set and a script suspends, CKAngelScript aborts the context and returns `CKAS_UNSUPPORTED`.

## Object And Method Calls

Object/method APIs are for synchronous class method calls without exposing `asIScriptObject`, `asIScriptFunction`, or `asIScriptContext` directly:

```cpp
CKAngelScriptObjectOptions objectOptions = CKAngelScriptApi::ObjectOptions();
objectOptions.ModuleName = "example_api";
objectOptions.ClassName = "ExampleMod";

CKAngelScriptObject *object = nullptr;
CKAngelScriptCreateObject(angelScript, &objectOptions, &object, &result);

CKAngelScriptMethodOptions methodOptions = CKAngelScriptApi::MethodOptions();
methodOptions.Object = object;
methodOptions.MethodDecl = "int Add(int)";

CKAngelScriptMethod *method = nullptr;
CKAngelScriptFindObjectMethod(angelScript, &methodOptions, &method, &result);
```

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
    CKAngelScriptApi::ObjectMethodExecuteOptions();
call.Object = object;
call.Method = method;
call.WriteArgs = WriteAddArgs;
call.ReadResult = ReadAddResult;
call.UserData = &data;
call.Flags = CKAS_CALL_NO_SUSPEND | CKAS_CALL_HOT_PATH;

CKAS_STATUS status = CKAngelScriptCallObjectMethod(angelScript, &call, &result);
```

Typed helpers support `bool`, `int`, `float`, `string`, and explicitly borrowed host objects. Borrowed object arguments are for callback-local views already registered with the engine and must target read-only input references.

Release handles when the host no longer needs them:

```cpp
CKAngelScriptReleaseMethod(angelScript, method);
CKAngelScriptReleaseObject(angelScript, object);
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

## Engine Extensions

Host plugins can register additional AngelScript APIs without adding host-specific code to CKAngelScript itself:

```cpp
CKAngelScriptEngineExtension extension = CKAngelScriptApi::EngineExtension();
extension.Name = "BML";
extension.Register = RegisterBmlApi;
extension.UserData = bridge;

CKAngelScriptRegisterEngineExtension(angelScript, &extension, &result);
```

CKAngelScript copies the extension name internally and retains the callback for future engine rebuilds. Duplicate names return `CKAS_ALREADYEXISTS`. `CKAngelScriptUnregisterEngineExtension(angelScript, name, result)` removes by name only; it does not remove functions already registered into the current AngelScript engine.

Callbacks return `>= 0` on success and `< 0` on failure. If `errorMessage` is set, CKAngelScript copies it into registration diagnostics. A failing immediate callback returns `CKAS_EXECUTIONFAILED` but remains registered for a later engine rebuild.

## Result Lifetime

`CKAngelScriptResult::ErrorMessage` and `StackTrace` are borrowed strings.

- Output-parameter results and `CKAngelScriptGetLastResult()` remain valid until the next CKAngelScript API call that updates the last result.
- `CKAngelScriptBorrowExecutionResult()` strings remain valid until the execution handle is released or started/resumed/cancelled again.

Copy strings if they must outlive those boundaries.

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
| `CKAS_INUSE` | Module unload/replace is blocked by live runtime handles. |
| `CKAS_ALREADYEXISTS` | Module or extension already exists. |
| `CKAS_AMBIGUOUS` | Name lookup matched multiple overloads. |
| `CKAS_FOREIGNHANDLE` | Handle belongs to another CKAngelScript manager. |
