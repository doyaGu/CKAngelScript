# Getting Started

This guide takes a clean checkout to a validated `AngelScript.dll`.

## Prerequisites

- Windows.
- Visual Studio 2022 with C++ build tools.
- Virtools SDK 2.1 or newer.
- DynCall, DynCallback, and DynLoad libraries, only when configuring with `-DCKAS_ENABLE_DYNCALL=ON`.
- AngelScript source under `deps/angelscript`; if it is missing, CMake downloads the SDK from AngelCode.

For offline builds, pass `-DCKAS_ANGELSCRIPT_ARCHIVE=C:\Path\To\angelscript_2.38.0.zip`. Use `CKAS_ANGELSCRIPT_URL` and `CKAS_ANGELSCRIPT_SHA256` when pointing CMake at a mirror or a different SDK archive.

The plugin is built as 32-bit because Virtools/Ballance hosts are 32-bit.

## Configure and Build

```powershell
git submodule update --init --recursive

cmake -B build -G "Visual Studio 17 2022" -A Win32 `
  -DVIRTOOLS_SDK_PATH=C:\Path\To\VirtoolsSDK `
  -DCKAS_ENABLE_DYNCALL=ON `
  -DDYNCALL_ROOT=C:\Path\To\dyncall `
  -DDYNCALLBACK_ROOT=C:\Path\To\dyncall `
  -DDYNLOAD_ROOT=C:\Path\To\dyncall `
  -DCKAS_BUILD_SELF_TESTS=ON

cmake --build build --config Release
```

The DLL is written to:

```text
build\src\Release\AngelScript.dll
```

Install it into the host:

```powershell
Copy-Item build\src\Release\AngelScript.dll C:\Path\To\Virtools\BuildingBlocks\
```

## Validate Locally

Run the repeatable local workflow:

```powershell
tools\Validate-Local.ps1 -BuildDir build -Configuration Release
```

This configures and builds the plugin. Add `-RunBallance` when a Ballance install is available:

```powershell
$env:BALLANCE_ROOT = "C:\Path\To\Ballance"
tools\Validate-Local.ps1 `
  -BuildDir build `
  -Configuration Release `
  -RunBallance `
  -BallanceRoot $env:BALLANCE_ROOT
```

The Ballance validation path installs the DLL, runs `VirtoolsDataExporter.exe`, starts Player, waits for the CKAngelScript startup self-test marker, and expects `status=ok`.

## First Runtime Script

Runtime scripts live under a `Scripts` directory. A directory module uses `script.as` as the manifest.

```text
Data\Scripts\hello.runtime\script.as
Data\Scripts\hello.runtime\runtime.as
```

`script.as`:

```angelscript
[script id="hello.runtime" name="Hello Runtime" version="1.0.0" entry="runtime.as"]
[script.messages topics="hello.ping"]
```

`runtime.as`:

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

void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx) {
    print("message " + msg.Topic());
}
```

Validate before launching the host:

```powershell
tools\Validate-RuntimeScripts.ps1 -ScriptRoot C:\Path\To\Game\Data
```

## First Component

Add the `AngelScript Component` Building Block to a behavior graph. Give it a class name and script source:

```angelscript
class Spinner {
    [param type="float" default="0.02"]
    float Speed;

    void Update(const CKBehaviorContext &in ctx) {
        Entity3DRef@ target = Scene::FindEntity3D(ctx, "Ball", 0, true);
        if (target is null || !target.valid) {
            return;
        }

        CK3dEntity@ entity = target.Entity3D();
        if (entity !is null) {
            entity.Rotate(0.0f, 1.0f, 0.0f, Speed);
        }
    }
}
```

The `[param]` metadata creates a Virtools input parameter and injects its value into `Speed`.

## Next Steps

- Runtime scripts: [runtime-script-manager-v2.md](runtime-script-manager-v2.md)
- Components: [component-metadata-manifest.md](component-metadata-manifest.md)
- Scene objects: [scene-interop.md](scene-interop.md)
- Behavior graphs: [behavior-bridge-design.md](behavior-bridge-design.md)
- Troubleshooting: [validation-and-troubleshooting.md](validation-and-troubleshooting.md)
