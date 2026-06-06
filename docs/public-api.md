# Public API

External plugins use the C ABI declared in `include/CKAngelScript.h`. The ABI is based on an opaque `CKAngelScript*` plus `CKAngelScript...` functions; it does not expose the internal `ScriptManager` C++ class or a virtual C++ interface.

For C++ callers, the same header provides `CKAngelScriptApi`, a small non-owning wrapper over the C functions.

## Getting CKAngelScript

C ABI:

```cpp
#include "CKAngelScript.h"

CKAngelScript *angelScript = CKGetAngelScript(context);
if (!angelScript) {
    return;
}
```

C++ wrapper:

```cpp
#include "CKAngelScript.h"

CKAngelScriptApi api = CKAngelScriptApi::Get(context);
if (!api.IsValid()) {
    return;
}
```

The handle is owned by CKAngelScript. Do not allocate, delete, or cast it to CKAngelScript internals.

## Module Lifecycle

Use one source per load call:

```cpp
CKAngelScriptResult result = {};
CKAS_STATUS status =
    CKAngelScriptCompileModule(angelScript,
                              "example_api",
                              "int add_five(int value) { return value + 5; }",
                              CKAS_COMPILE_REPLACEEXISTING,
                              &result);
```

With the C++ wrapper:

```cpp
CKAngelScriptResult result = {};
api.CompileModule("example_api",
                  "int add_five(int value) { return value + 5; }",
                  CKAS_COMPILE_REPLACEEXISTING,
                  &result);
```

`CKAngelScriptLoadOptions` accepts:

- `ModuleName`
- `Filename`
- `Filenames` plus `FileCount`
- `Code`
- `Flags`

Set `Size = sizeof(CKAngelScriptLoadOptions)` when using the C ABI directly. The C++ wrapper helper `CKAngelScriptApi::LoadOptions()` does this for you.
Set `CKAS_LOAD_REPLACEEXISTING` in `Flags` to replace an existing module.
Passing multiple source kinds at once returns `CKAS_INVALIDARGUMENT`.

## Engine Extensions

Host plugins can register additional AngelScript APIs without adding host-specific code to CKAngelScript itself. Register an extension before runtime scripts are loaded:

```cpp
static int RegisterBmlApi(asIScriptEngine *engine,
                          CKAngelScript *angelScript,
                          void *userData,
                          const char **errorMessage) {
    int r = engine->SetDefaultNamespace("BML");
    if (r < 0) return r;

    r = engine->RegisterGlobalFunction("float GetLastDeltaTime()",
                                       asFUNCTION(BmlGetLastDeltaTime),
                                       asCALL_CDECL);
    int reset = engine->SetDefaultNamespace("");
    if (r < 0) return r;
    return reset < 0 ? reset : 0;
}

CKAngelScriptEngineExtension extension = CKAngelScriptApi::EngineExtension();
extension.Name = "BML";
extension.Register = RegisterBmlApi;
extension.UserData = bridge;

CKAngelScriptResult result = {};
CKAngelScriptRegisterEngineExtension(angelScript, &extension, &result);
```

With the C++ wrapper:

```cpp
api.RegisterEngineExtension(extension, &result);
```

Callbacks receive the same opaque `CKAngelScript*` used by the C ABI. If a callback needs CKAngelScript operations, call the `CKAngelScript...` functions directly or wrap it temporarily:

```cpp
CKAngelScriptApi api(angelScript);
asIScriptEngine *engine = api.GetScriptEngine();
```

Callbacks return `>= 0` on success and `< 0` on failure. If `errorMessage` is set, CKAngelScript copies it into registration diagnostics. The extension is retained and invoked again after the script engine is rebuilt.

A failing extension is non-fatal: CKAngelScript logs it but never tears down the engine or skips the other extensions. The extension is retained regardless, so a callback that fails immediately may still succeed on a later engine rebuild. By default registration invokes the callback immediately when the engine is ready; set `CKAS_ENGINEEXTENSION_DEFERRED` in `Flags` to defer invocation until the next engine rebuild. When the immediate attempt fails, `CKAngelScriptRegisterEngineExtension` returns `CKAS_EXECUTIONFAILED` while still keeping the extension registered.

Callbacks should be atomic. CKAngelScript only checks the final return code: if a callback registers some functions and then fails partway through, those functions remain in the current engine until the next rebuild. There is no automatic rollback.

Call `CKAngelScriptUnregisterEngineExtension(angelScript, name, userData, result)` before unloading the host DLL. This removes the retained callback; it does not remove functions already registered into the current AngelScript engine. Passing `userData = nullptr` matches by name only.

## Execution Handles

Create, start, and release an execution:

```cpp
struct AddData {
    int Input = 0;
    int Output = 0;
};

void ConfigureAdd(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<AddData *>(userData);
    ctx->SetArgDWord(0, static_cast<asDWORD>(data->Input));
}

void ReadAddResult(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<AddData *>(userData);
    data->Output = static_cast<int>(ctx->GetReturnDWord());
}

AddData data;
data.Input = 37;

CKAngelScriptExecuteOptions options = CKAngelScriptApi::ExecuteOptions();
options.ModuleName = "example_api";
options.FunctionDecl = "int add_five(int)";
options.ConfigureContext = ConfigureAdd;
options.ReadResult = ReadAddResult;
options.UserData = &data;

CKAngelScriptExecution *execution =
    CKAngelScriptCreateExecution(angelScript, &options, &result);
if (execution) {
    CKAS_STATUS status = CKAngelScriptStartExecution(angelScript, execution);
    CKAngelScriptReleaseExecution(angelScript, execution);
}
```

With the C++ wrapper:

```cpp
CKAngelScriptExecution *execution = api.CreateExecution(options, &result);
if (execution) {
    CKAS_STATUS status = api.StartExecution(execution);
    api.ReleaseExecution(execution);
}
```

If `StartExecution` returns `CKAS_SUSPENDED`, call `ResumeExecution` on a later tick after async work advances.

## Result Lifetime

`CKAngelScriptResult::ErrorMessage` and `StackTrace` are borrowed strings.

- Output-parameter results and `CKAngelScriptGetLastResult()` remain valid until the next CKAngelScript API call that updates the last result.
- `CKAngelScriptGetExecutionResult()` strings remain valid until the execution handle is released or started/resumed/cancelled again.

Copy strings if they must outlive those boundaries.

## Unload Constraints

Execution handles keep their module alive from the public API perspective. `UnloadModule` and replacing an existing module fail while any `CKAngelScriptExecution` for that module is still unreleased. Cancel and release active executions before unloading or replacing a module.

## Status Values

Common statuses:

| Status | Meaning |
| --- | --- |
| `CKAS_OK` | Operation succeeded. |
| `CKAS_INVALIDARGUMENT` | Bad options or missing required fields. |
| `CKAS_NOTINITIALIZED` | CKAngelScript or the engine is not ready. |
| `CKAS_NOTFOUND` | Module or function was not found. |
| `CKAS_COMPILEERROR` | Module compilation failed. |
| `CKAS_EXECUTIONFAILED` | Runtime execution failed. |
| `CKAS_SUSPENDED` | Execution awaits async work. |
| `CKAS_CANCELLED` | Execution was cancelled. |
