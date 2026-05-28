# CKAngelScript

CKAngelScript integrates AngelScript into the Virtools/CK2 runtime. It builds as `AngelScript.dll`, installs into `BuildingBlocks`, and provides:

- Full low-level CK/Vx SDK bindings for scripts that need direct engine access.
- `AngelScript Component`, a Building Block that attaches a script class to a behavior instance.
- Runtime Script Manager v2 for long-lived scripts discovered from data/script roots.
- High-level `Scene`, `Behavior`/`BB`, `Param`, `Message`, and `Async` APIs.
- DynCall-backed native FFI helpers for advanced integrations.

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
| C++ manager API | [docs/public-manager-api.md](docs/public-manager-api.md) |
| Raw SDK bindings | [docs/sdk-bindings.md](docs/sdk-bindings.md) |
| Ballance catalog snapshot | [docs/catalog-ballance.md](docs/catalog-ballance.md) |
| Examples | [docs/examples.md](docs/examples.md) |
| Troubleshooting | [docs/validation-and-troubleshooting.md](docs/validation-and-troubleshooting.md) |

## Quick Build

Prerequisites:

- Windows with Visual Studio 2022.
- Virtools SDK 2.1 or newer.
- DynCall libraries.
- AngelScript source placed under `deps/angelscript`.

```powershell
git submodule update --init --recursive

cmake -B build -G "Visual Studio 17 2022" -A Win32 `
  -DVIRTOOLS_SDK_PATH=C:\Path\To\VirtoolsSDK `
  -DDYNCALL_ROOT=C:\Path\To\dyncall `
  -DDYNCALLBACK_ROOT=C:\Path\To\dyncall `
  -DDYNLOAD_ROOT=C:\Path\To\dyncall `
  -DCKAS_BUILD_SELF_TESTS=ON

cmake --build build --config Release
Copy-Item build\src\Release\AngelScript.dll C:\Path\To\Virtools\BuildingBlocks\
```

For the repeatable local path:

```powershell
tools\Validate-Local.ps1 -BuildDir build -Configuration Release
```

With a Ballance install:

```powershell
tools\Validate-Local.ps1 `
  -BuildDir build `
  -Configuration Release `
  -RunBallance `
  -BallanceRoot C:\Users\kakut\Games\Ballance
```

## First Runtime Script

Create `Data\Scripts\hello.runtime\script.as`:

```angelscript
[script id="hello.runtime" name="Hello Runtime" version="1.0.0" entry="runtime.as"]
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
class Spinner {
    [param type="float" default="1.0"]
    float Speed;

    void Update(const CKBehaviorContext &in ctx) {
        Entity3DRef@ target = Scene::FindEntity3D(ctx, "Ball", 0, true);
        if (target !is null && target.valid) {
            CK3dEntity@ entity = target.Entity3D();
            if (entity !is null) {
                entity.Rotate(0.0f, 1.0f, 0.0f, Speed);
            }
        }
    }
}
```

Set the component `Class` parameter to `Spinner`, then provide either inline source, a script file, or a shared module. Component metadata can create editor inputs and inject them into script fields.

## License

CKAngelScript is licensed under the MIT License. See [LICENSE](LICENSE).
