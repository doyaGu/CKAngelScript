# CKAngelScript

CKAngelScript integrates AngelScript into CK2. This branch builds the SDL plugin
for NeMo2 and installs it into `BuildingBlocks`. It provides:

- Full low-level CK/Vx SDK bindings for scripts that need direct engine access.
- `AngelScript Component`, a Building Block that attaches a script class to a behavior instance.
- Runtime Script Manager v2 for long-lived scripts discovered from data/script roots.
- High-level `Scene`, `Behavior`/`BB`, `Param`, `Message`, and `Async` APIs.
- Optional DynCall-backed native FFI helpers for advanced integrations.

The `main` branch builds the plugin for original Virtools. The SDL plugin uses
the same CK2, VxMath, and SDL libraries as its host. BML+ can build it as a
CMake subproject or run with a matching prebuilt plugin.

## Documentation Map

| Topic | Start here |
| --- | --- |
| First setup and validation | [docs/getting-started.md](docs/getting-started.md) |
| Choose the right guide | [docs/index.md](docs/index.md) |
| Runtime scripts | [docs/runtime-script-manager-v2.md](docs/runtime-script-manager-v2.md) |
| AngelScript Component metadata | [docs/component-metadata-manifest.md](docs/component-metadata-manifest.md) |
| Scene lookup and safe object refs | [docs/scene-interop.md](docs/scene-interop.md) |
| Behavior Bridge and runtime BBs | [docs/behavior-bridge-design.md](docs/behavior-bridge-design.md) |
| Script messaging | [docs/messaging.md](docs/messaging.md) |
| Async tasks | [docs/async.md](docs/async.md) |
| Native buffers and DynCall | [docs/native-ffi.md](docs/native-ffi.md) |
| C API and C++ wrapper | [docs/public-api.md](docs/public-api.md) |
| API inventory | [docs/api-inventory.md](docs/api-inventory.md) |
| Raw SDK bindings | [docs/sdk-bindings.md](docs/sdk-bindings.md) |
| Examples | [docs/examples.md](docs/examples.md) |
| Troubleshooting | [docs/validation-and-troubleshooting.md](docs/validation-and-troubleshooting.md) |

## Quick Build

Install CMake 3.25+, a C++20 compiler, and the CK2/VxMath packages exported by the
NeMo2 revision pinned by your Player. Use the matching SDL3 development package.

```sh
git submodule update --init --recursive
cmake -S . -B build/sdl -DCMAKE_BUILD_TYPE=Release -DCKAS_RUNTIME=sdl \
  -DCMAKE_PREFIX_PATH="/path/to/engine-sdk;/path/to/SDL-prefix"
cmake --build build/sdl --config Release
ctest --test-dir build/sdl -C Release --output-on-failure
cmake --install build/sdl --config Release --prefix /path/to/stage
```

For a Visual Studio generator, select the Player architecture with
`-A Win32`, `-A x64`, or `-A ARM64`. Use a separate build tree per target.
For an offline build, pass `-DCKAS_ANGELSCRIPT_ARCHIVE=/path/to/angelscript_2.39.0.zip`.

The stage contains the plugin under `BuildingBlocks` and the development
headers under `include`. Copy the plugin into the matching Player installation.
Build a ZIP from the standalone stage with
`cpack --config build/sdl/CPackConfig.cmake -C Release`.

See [Getting Started](docs/getting-started.md) for installed paths, tests and
optional features.

## First Runtime Script

Create `Data\Scripts\hello.runtime\script.as`:

```angelscript
[runtime id="hello.runtime" name="Hello Runtime" version="1.0.0" entry="runtime.as"]
```

Create `Data\Scripts\hello.runtime\runtime.as`:

```angelscript
void OnLoad(const ScriptContext &in ctx) {
    print("Loaded " + ctx.Id());
}

void Update(const ScriptContext &in ctx) {
    if (ctx.FrameIndex() == 1) {
        dictionary payload;
        payload["source"] = ctx.Id();
        Message::Publish(ctx, "hello.ready", payload);
    }
}
```

Validate the root before launching the host:

```powershell
tools\Validate-RuntimeScripts.ps1 -ScriptRoot C:\Path\To\Game\Data
```

## First Component

Use the `AngelScript Component` Building Block when a script should live on a behavior instance:

```angelscript
class ObjectProbe {
    [param type="string" default="Ball"]
    string ObjectName;

    void Start(BehaviorFrame@ frame) {
        Entity3DRef@ target = Scene::FindEntity3D(frame.Context(), ObjectName, 0, true);
        frame.Trace(target !is null && target.valid ? target.Describe() : "Object not found");
    }
}
```

Set the component `Class` parameter to `ObjectProbe`, then provide either inline source, a script file, or a shared module. Component metadata creates the `ObjectName` editor input and injects it into the script field.

## License

CKAngelScript is licensed under the MIT License. See [LICENSE](LICENSE).
