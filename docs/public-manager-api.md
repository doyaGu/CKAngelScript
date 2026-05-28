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
