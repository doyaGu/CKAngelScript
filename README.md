# CKAngelScript

CKAngelScript integrates the AngelScript scripting language into Virtools, providing developers with a flexible and powerful scripting environment for creating advanced and interactive 3D applications.

---

## Features

- **Full Bindings of the Virtools SDK**: Access and utilize the entire Virtools SDK through AngelScript for seamless integration of custom scripting with Virtools' extensive API.
- **AngelScript Integration**: Leverage the AngelScript scripting language to achieve greater flexibility and control in your 3D projects.
- **Foreign Function Interface (FFI)**: Call native C functions directly from AngelScript using FFI, powered by DynCall, enabling seamless interaction with external libraries and APIs.
- **Building Blocks**:
  - **AngelScript Component**: Attach AngelScript classes to Virtools behaviors with lifecycle callbacks, parameter injection, and generic script messaging.
- **Runtime Script Manager**: Discover long-lived scripts from script roots, validate manifests, run lifecycle phases, and expose structured runtime state.
- **Scene Interop API**: Use safe `ObjectRef@` handles and typed refs through `Scene::*` helpers for object lookup, creation, scene membership, selection, and guarded destruction.
- **Extensibility**: Expand Virtools' capabilities by integrating custom AngelScript modules.
- **Compatibility**: Supports Virtools 2.1 and higher.

---

## Building Blocks

### AngelScript Component

The **AngelScript Component** attaches an AngelScript class instance to a Virtools behavior. It supports lifecycle callbacks such as `OnLoad`, `Awake`, `OnEnable`, `Start`, `Update`, `OnDisable`, `OnDestroy`, and `OnReset`, plus `OnMessage` for the generic script message bus.

Component scripts can be loaded from a shared module, inline source, a file, or a component manifest. Component metadata can declare injected inputs, defaults, BB bridge bindings, and static message subscriptions.

See [docs/component-metadata-manifest.md](docs/component-metadata-manifest.md) for component manifests and metadata.

### Runtime Scripts

Runtime scripts are managed by the `ScriptManager` directly rather than by legacy script execution building blocks. They are discovered from `DATA_PATH/Scripts` and `CKAS_SCRIPT_ROOTS`, use `script.as` metadata manifests, and run through the runtime lifecycle.

See [docs/runtime-script-manager-v2.md](docs/runtime-script-manager-v2.md) for runtime script metadata, lifecycle, dependencies, validation, templates, and messaging.

### Scene Interop

Runtime scripts and AngelScript Components can use the `Scene` namespace as the preferred high-level Virtools interop layer. It wraps `CKObject` ids in revalidating `ObjectRef@` handles, returns precise typed refs such as `Entity3DRef@` and `BehaviorRef@`, creates dynamic scene objects by default, and guards persistent-object destruction.

See [docs/scene-interop.md](docs/scene-interop.md) for the API surface and examples.

---

## Getting Started

### Prerequisites

Before you begin, ensure the following:

1. **Virtools SDK (2.1 or higher)**: Required to access the APIs and resources for building this project.
2. **DynCall Library**: Required to enable Foreign Function Interface (FFI).
3. **AngelScript SDK**: Required to compile the AngelScript integration.
4. **Microsoft Visual Studio**: This project can only be compiled using Visual Studio.

---

### Preparing the Dependencies

#### 1. Preparing Virtools SDK

- Ensure the Virtools SDK is installed and accessible.
- Use the appropriate path when configuring CMake (see below).

#### 2. Preparing DynCall

- Download the DynCall suite from its [official website](http://www.dyncall.org/).
- Extract and install the library.

#### 3. Preparing AngelScript SDK

1. Download the AngelScript SDK from its [official website](http://www.angelcode.com/angelscript/).
2. Extract the downloaded SDK package.
3. Place the extracted contents into the `deps` directory under a folder named `angelscript`. Your directory structure should look like this:

---

### Building with CMake

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/doyaGu/CKAngelScript.git
   ```

2. **Initialize Submodules**:
   ```bash
   git submodule update --init --recursive
   ```

3. **Prepare the Dependencies**:
   Complete the preparation steps outlined in the "Preparing the Dependencies" section.

4. **Run CMake**:
   Provide paths to your Virtools SDK, DynCall, and AngelScript SDK, and then run the following command.

   ```bash
   cmake -B build -G "Visual Studio 16 2022" -A Win32 \
            -DVIRTOOLS_SDK_PATH=/path/to/virtools/sdk \
            -DDYNCALL_ROOT=/path/to/dyncall \
            -DDYNCALLBACK_ROOT=/path/to/dyncall \
            -DDYNLOAD_ROOT=/path/to/dyncall
   ```

4. **Build the Project**:
   Open the project in **Microsoft Visual Studio**, configure the build settings to match your Virtools SDK installation, and build the project. The process will generate the following file:
   ```
   AngelScript.dll
   ```

5. **Build the Project**:
   ```bash
   cmake --build .
   ```

6. **Integrate with Virtools**:
   After the build completes, copy the generated `AngelScript.dll` to the `BuildingBlocks` directory of your Virtools installation:
   ```bash
   cp AngelScript.dll /path/to/virtools/BuildingBlocks/
   ```

---

## Usage

### Runtime Script Example

Create a runtime script directory under one of the configured script roots and add a `script.as` manifest:

```cpp
[script id="example.runtime" name="Example Runtime" version="1.0.0" entry="main.as" enabled=true]
[script.meta category="Example" author="CKAngelScript"]
```

Then implement the entry file:

```cpp
void OnLoad(const ScriptContext &in ctx) {
    print("Loaded " + ctx.Id());
}

void Update(const ScriptContext &in ctx) {
    if (ctx.FrameIndex() == 1) {
        SceneRef@ scene = Scene::CurrentScene(ctx);
        dictionary payload;
        payload["source"] = ctx.Id();
        payload["scene"] = scene !is null && scene.valid ? scene.Name() : "";
        Message::Publish(ctx, "example.ready", payload);
    }
}
```

For a complete starter, use `tools/templates/runtime-script-v2`.

### Component Example

Use the **AngelScript Component** building block when a script should live on a behavior instance. The component script declares a class and lifecycle methods that receive `CKBehaviorContext`.

```cpp
class Spinner {
    void Update(const CKBehaviorContext &in ctx) {
        // Component update logic.
    }
}
```

Use the Component `Output Error Message` setting to expose script errors and stack traces through output parameters.

### Debugging and Testing

- **Console Output**: Use `print` statements in AngelScript code for logging and debugging.
- **Runtime Validation**: Run `tools\Validate-RuntimeScripts.ps1` against a script root to catch manifest, dependency, lifecycle, and compile errors before launching Virtools.

---

## Public Manager API

External Virtools plugins can retrieve the public manager with `AngelScriptManager::GetManager(context)`. The public API focuses on module loading, function lookup, task-style execution, and result diagnostics.

`LoadModule` accepts one script source per call: `Code`, `Filename`, or `Filenames` with `FileCount`. Passing more than one source returns `ANGELSCRIPT_STATUS_INVALID_ARGUMENT`. If no source is provided, the manager loads the default `<ModuleName>.as` file through the configured Virtools script path.

```cpp
#include "AngelScriptManager.h"

struct AddData {
    int Input = 0;
    int Output = 0;
};

void ConfigureAdd(asIScriptContext *ctx, void *userData) {
    AddData *data = static_cast<AddData *>(userData);
    ctx->SetArgDWord(0, static_cast<asDWORD>(data->Input));
}

void ReadAddResult(asIScriptContext *ctx, void *userData) {
    AddData *data = static_cast<AddData *>(userData);
    data->Output = static_cast<int>(ctx->GetReturnDWord());
}

void RunPublicApiExample(CKContext *context) {
    AngelScriptManager *manager = AngelScriptManager::GetManager(context);
    if (!manager) {
        return;
    }

    AngelScriptResult result = {};
    manager->CompileModule("example_api",
                           "int add_five(int value) { return value + 5; }",
                           true,
                           &result);
    if (result.Status != ANGELSCRIPT_STATUS_OK) {
        context->OutputToConsoleEx("Compile failed: %s", result.ErrorMessage ? result.ErrorMessage : "");
        return;
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
    if (!execution) {
        context->OutputToConsoleEx("CreateExecution failed: %s", result.ErrorMessage ? result.ErrorMessage : "");
        return;
    }

    AngelScriptStatus status = manager->StartExecution(execution);
    if (status == ANGELSCRIPT_STATUS_SUSPENDED) {
        // ResumeExecution should be called on a later tick after async work advances.
    } else if (status != ANGELSCRIPT_STATUS_OK) {
        const AngelScriptResult *execResult = manager->GetExecutionResult(execution);
        context->OutputToConsoleEx("Execution failed: %s",
                                   execResult && execResult->ErrorMessage ? execResult->ErrorMessage : "");
    }

    manager->ReleaseExecution(execution);
    manager->UnloadModule("example_api");
}
```

`AngelScriptResult::ErrorMessage` and `StackTrace` are borrowed strings. Results returned through API output parameters and `GetLastResult()` remain valid until the next manager API call that updates the last result. `GetExecutionResult()` strings remain valid until the execution handle is released or started, resumed, or cancelled again.

Execution handles keep their module alive from the public API perspective. `UnloadModule` and replacing an existing module fail with `ANGELSCRIPT_STATUS_EXECUTION_FAILED` while any `AngelScriptExecution` for that module is still unreleased. Call `CancelExecution` if needed, then `ReleaseExecution`, before unloading or replacing the module.

Typed async aggregate results should use the out-parameter overloads:

```angelscript
array<AsyncTask<int>@> tasks;
AsyncTask<array<int>@>@ allInts;
Async::All(tasks, @allInts);

AsyncTask<int>@ firstInt;
Async::Race(tasks, @firstInt);
Async::Any(tasks, @firstInt);
```

The direct typed-return aggregate overloads are intentionally not exposed because they can make AngelScript module compilation hang inside the Virtools Player host.

---

## Runtime Script Manager

Runtime scripts are long-lived modules discovered from `DATA_PATH/Scripts` and `CKAS_SCRIPT_ROOTS`. Directory modules use `script.as` metadata with an optional `entry` file; single-file `.as` modules remain supported.

Runtime lifecycle callbacks keep the existing naming style but must use the explicit context signature, for example `void OnLoad(const ScriptContext &in ctx)` and `void Update(const ScriptContext &in ctx)`. v2 also adds `OnPostLoad(ctx)` and `OnPostProcess(ctx)`.

See [docs/runtime-script-manager-v2.md](docs/runtime-script-manager-v2.md) for metadata, lifecycle, concise `ScriptContext` accessors, structured `RuntimeScriptInfo` / `RuntimeDependencyInfo` APIs, validation, and templates. See [docs/scene-interop.md](docs/scene-interop.md) for the high-level scene/object helper API.

Generic script messaging is exposed through the `Message` namespace for runtime scripts and AngelScript Components. Runtime targets use `runtime:<script-id>` and component targets use `component:<CK_ID>`.

---

## Contributing

Contributions to CKAngelScript are welcome. To contribute:

1. Fork this repository on GitHub.
2. Make your changes.
3. Submit a pull request with a detailed explanation of your modifications.

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- **[AngelScript](http://www.angelcode.com/angelscript/)**: The scripting language integrated into this project.
- **[DynCall](http://www.dyncall.org/)**: The dynamic call library powering FFI.
- **[Virtools](https://en.wikipedia.org/wiki/Virtools/)**: The 3D engine platform extended by CKAngelScript.
