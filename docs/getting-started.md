# Getting Started

The `sdl` branch builds CKAngelScript as a dynamic NeMo2 plugin. Use the
`main` branch for the original Virtools SDK and Player.

## Configure and Build

Install CMake 3.25+, a C++20 compiler, and the CK2, VxMath and SDL3 development
packages matching your Player. A full install of the matching NeMo2 build
provides the CK2 and VxMath CMake packages. Use the SDL3 package selected by
that build. CMake reuses those targets if a parent project supplies them.

```sh
git submodule update --init --recursive
cmake -S . -B build/sdl -DCMAKE_BUILD_TYPE=Release -DCKAS_RUNTIME=sdl \
  -DCMAKE_PREFIX_PATH="/path/to/engine-sdk;/path/to/SDL-prefix"
cmake --build build/sdl --config Release
ctest --test-dir build/sdl -C Release --output-on-failure
```

Use a separate build directory per architecture. With Visual Studio, choose
`-A Win32`, `-A x64`, or `-A ARM64` to match Player. CMake fetches the pinned
AngelScript SDK if it is missing. For offline builds, pass
`-DCKAS_ANGELSCRIPT_ARCHIVE=/path/to/angelscript_2.38.0.zip`.

`CKAS_BUILD_CORE_TESTS` is enabled by default. CTest runs both the host-independent
core tests and the engine integration test, which loads the actual plugin into
a NeMo2 CK context without launching Player. `CKAS_BUILD_SELF_TESTS` adds tests
that run inside a Player when enabled through the environment. DynCall is
optional and requires matching DynCall, DynCallback, and DynLoad libraries.

## Install and Package

```sh
cmake --install build/sdl --config Release --prefix /path/to/stage
cpack --config build/sdl/CPackConfig.cmake -C Release
```

Copy the staged `BuildingBlocks` plugin into a matching Player:
`AngelScript.dll` on Windows, `libAngelScript.so` on Linux, and
`libAngelScript.dylib` on macOS. The ZIP includes the plugin and its public
headers. Player supplies CK2, VxMath and SDL.

When BML+ builds CKAngelScript itself, its runtime install already includes
the matching plugin. When supplying the plugin separately, enable script
support in the BML+ build and use the headers from this CKAngelScript build.

## First Runtime Script

Runtime scripts live under a `Scripts` directory. A directory module uses `script.as` as the manifest.

```text
Data\Scripts\hello.runtime\script.as
Data\Scripts\hello.runtime\runtime.as
```

`script.as`:

```angelscript
[runtime id="hello.runtime" name="Hello Runtime" version="1.0.0" entry="runtime.as"]
[runtime.messages topics="hello.ping"]
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
tools\Validate-ScriptEntries.ps1 -ScriptRoot C:\Path\To\Game\Data
```

## First Component

Add the `AngelScript Component` Building Block to a behavior graph. Give it a class name and script source:

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

The `[param]` metadata creates a Virtools input parameter and injects its value into `ObjectName`.

## Next Steps

- Runtime scripts: [runtime-script-manager-v2.md](runtime-script-manager-v2.md)
- Components: [component-metadata-manifest.md](component-metadata-manifest.md)
- Scene objects: [scene-interop.md](scene-interop.md)
- Behavior graphs: [behavior-bridge-design.md](behavior-bridge-design.md)
- Troubleshooting: [validation-and-troubleshooting.md](validation-and-troubleshooting.md)
