# Public Manager API

External C++ plugins can use `AngelScriptManager` from `include/AngelScriptManager.h`.

## Getting the Manager

```cpp
#include "AngelScriptManager.h"

AngelScriptManager *manager = AngelScriptManager::GetManager(context);
if (!manager) {
    return;
}
```

The manager exposes the AngelScript engine, module loading, function lookup, execution handles, and last-result diagnostics.

## Module Lifecycle

Use one source per load call:

```cpp
AngelScriptResult result = {};
manager->CompileModule("example_api",
                       "int add_five(int value) { return value + 5; }",
                       true,
                       &result);
```

`AngelScriptLoadOptions` accepts:

- `ModuleName`
- `Filename`
- `Filenames` plus `FileCount`
- `Code`
- `ReplaceExisting`

Passing multiple source kinds at once returns `ANGELSCRIPT_STATUS_INVALID_ARGUMENT`.

## Engine Extensions

Host plugins can register additional AngelScript APIs without adding host-specific code to CKAngelScript itself. Register an extension before runtime scripts are loaded:

```cpp
static int RegisterBmlApi(asIScriptEngine *engine,
                          AngelScriptManager *manager,
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

AngelScriptEngineExtension extension = {};
extension.Name = "BML";
extension.Register = RegisterBmlApi;
extension.UserData = bridge;

AngelScriptResult result = {};
manager->RegisterEngineExtension(extension, &result);
```

Callbacks return `>= 0` on success and `< 0` on failure. If `errorMessage` is set, CKAngelScript copies it into registration diagnostics. The extension is retained and invoked again after the script engine is rebuilt.

A failing extension is **non-fatal**: CKAngelScript logs it but never tears down the engine or skips the other extensions, so a bug in one host's extension cannot break core scripting or another host. The extension is retained regardless, so a callback that fails immediately may still succeed on a later engine rebuild. When `InvokeImmediately` is true and the immediate attempt fails, `RegisterEngineExtension` returns `ANGELSCRIPT_STATUS_EXECUTION_FAILED` while still keeping the extension registered.

Callbacks should be **atomic**. CKAngelScript only checks the final return code: if a callback registers some functions and then fails partway through, those functions remain in the current engine until the next rebuild — there is no automatic rollback. Reset the default namespace and undo partial work yourself before returning a failure code.

If the engine is already initialized and `InvokeImmediately` is true, registration is attempted immediately. This is useful for host plugins that discover CKAngelScript after startup, but the safest path is still to register before user modules compile.

Call `UnregisterEngineExtension(name, userData)` before unloading the host DLL. This removes the retained callback; it does not remove functions already registered into the current AngelScript engine. Passing `userData = nullptr` (the default) matches by name only; pass the same `UserData` you registered with to scope the removal to a specific instance.

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

AngelScriptExecuteOptions options = {};
options.ModuleName = "example_api";
options.FunctionDecl = "int add_five(int)";
options.ConfigureContext = ConfigureAdd;
options.ReadResult = ReadAddResult;
options.UserData = &data;

AngelScriptExecution *execution = manager->CreateExecution(options, &result);
if (execution) {
    AngelScriptStatus status = manager->StartExecution(execution);
    manager->ReleaseExecution(execution);
}
```

If `StartExecution` returns `ANGELSCRIPT_STATUS_SUSPENDED`, call `ResumeExecution` on a later tick after async work advances.

## Result Lifetime

`AngelScriptResult::ErrorMessage` and `StackTrace` are borrowed strings.

- Output-parameter results and `GetLastResult()` remain valid until the next manager API call that updates the last result.
- `GetExecutionResult()` strings remain valid until the execution handle is released or started/resumed/cancelled again.

Copy strings if they must outlive those boundaries.

## Unload Constraints

Execution handles keep their module alive from the public API perspective. `UnloadModule` and replacing an existing module fail while any `AngelScriptExecution` for that module is still unreleased. Cancel and release active executions before unloading or replacing a module.

## Status Values

Common statuses:

| Status | Meaning |
| --- | --- |
| `ANGELSCRIPT_STATUS_OK` | Operation succeeded. |
| `ANGELSCRIPT_STATUS_INVALID_ARGUMENT` | Bad options or missing required fields. |
| `ANGELSCRIPT_STATUS_NOT_INITIALIZED` | Manager/engine is not ready. |
| `ANGELSCRIPT_STATUS_NOT_FOUND` | Module or function was not found. |
| `ANGELSCRIPT_STATUS_COMPILE_ERROR` | Module compilation failed. |
| `ANGELSCRIPT_STATUS_EXECUTION_FAILED` | Runtime execution failed. |
| `ANGELSCRIPT_STATUS_SUSPENDED` | Execution awaits async work. |
| `ANGELSCRIPT_STATUS_CANCELLED` | Execution was cancelled. |
