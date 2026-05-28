# SDK Bindings

CKAngelScript registers a broad CK/Vx SDK surface for AngelScript. These bindings are intentionally close to the native SDK, while higher-level APIs provide safer workflows for common scene and behavior tasks.

## Binding Areas

| Area | Source files | Examples |
| --- | --- | --- |
| Math and Vx types | `ScriptVxMath.cpp` | `VxVector`, `VxMatrix`, `VxColor`, geometry helpers |
| CK typedefs and containers | `ScriptCKTypes.cpp` | `CK_ID`, `CKGUID`, `XObjectArray`, hash/array wrappers |
| CK enums and flags | `ScriptCKEnums.cpp` | SDK enum constants and values |
| CK structs and defines | `ScriptCKDefines.cpp` | file structs, descriptors, state chunks, metadata structs |
| CK objects | `ScriptCKObjects.cpp` | `CKObject`, `CK3dEntity`, `CKBehavior`, `CKScene`, materials, meshes |
| CK managers/context | `ScriptCKContext.cpp`, `ScriptCKManagers.cpp` | `CKContext`, object manager, path manager, parameter manager |

## Raw Binding Style

Raw bindings generally preserve native naming and mutability. They are useful when a script needs a specific CK SDK call:

```angelscript
Entity3DRef@ ref = Scene::FindEntity3D(ctx, "Ball");
CK3dEntity@ entity = ref !is null ? ref.Entity3D() : null;
if (entity !is null) {
    entity.Translate(0.0f, 1.0f, 0.0f);
}
```

Use raw pointers near the operation. Do not cache raw CK object pointers long-term; cache `ObjectRef@`-derived handles instead.

## Prefer High-Level APIs First

| Task | Prefer |
| --- | --- |
| Find objects | `Scene::Find*`, `Scene::FindOne*`, `Scene::FindAll*` |
| Store object identity | `ObjectRef@`, `Entity3DRef@`, `BehaviorRef@` |
| Create/destroy dynamic objects | `Scene::Create*`, `Scene::Destroy` |
| Drive behavior graphs | `Behavior`, `BB`, `Param` |
| Read/write CK parameters | `ParamRef@`, `ParamValue@`, `Param::*` |
| Runtime communication | `Message::*` |

Raw SDK bindings are powerful but do not provide stale-id protection or high-level diagnostics.

## Registration Failures

All binding registrations are checked in Release and Debug builds. If a signature is wrong for the host SDK, startup fails early and logs the first failed registration. This is preferable to running with a partially registered script engine.

## Version Differences

Some bindings are guarded by `CKVERSION` because Virtools SDK versions expose different signatures. If you support another SDK build, validate with the target host and inspect registration failures before exposing the build to scripts.

## API Discovery

There is no full generated raw SDK reference committed in this repository. Use:

- Source registration files for exact AngelScript declarations.
- [catalog-ballance.md](catalog-ballance.md) for exported host parameter/BB metadata.
- High-level guides for stable user workflows.

When writing docs or scripts, prefer the exact declaration registered in source over assumptions from native C++ headers.
