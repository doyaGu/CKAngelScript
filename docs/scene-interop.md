# Scene Interop API

`Scene` is the high-level Virtools scene interop namespace for runtime scripts and AngelScript Components. It sits above raw `CKContext` / `CKObject` bindings and returns checked `ObjectRef@` handles instead of encouraging scripts to cache SDK pointers directly.

## Object Refs

`ObjectRef@` stores the source `CKContext`, object id, native object address, and object stamp. Each accessor revalidates the object so deleted or reused ids become invalid.

```angelscript
ObjectRef@ ref = Scene::Find(ctx, "Ball");
Entity3DRef@ ball = cast<Entity3DRef>(ref);
if (ball !is null && ball.valid) {
  CK3dEntity@ entity = ball.Entity3D();
}
```

All object refs support `IsValid()`, `valid`, `Error()`, `Describe()`, `Id()`, `Name()`, `ClassId()`, `IsDynamic()`, and `Object()`.

Precise ref types add typed accessors: `SceneObjectRef.SceneObject()`, `Entity3DRef.Entity3D()`, `Entity2DRef.Entity2D()`, `MaterialRef.Material()`, `TextureRef.Texture()`, `MeshRef.Mesh()`, `SceneRef.Scene()`, and `LevelRef.Level()`. Bridge refs such as `BehaviorRef@`, `ParamRef@`, `ParamStructRef@`, `ParamOperationRef@`, and `BehaviorLinkRef@` are also `ObjectRef@`-compatible. Transaction handles, values, builders, tasks, and graph cursors stay outside the ref hierarchy; use their methods to obtain `ObjectRef@`-derived handles when needed.

## Context Overloads

Every `Scene::*` helper has overloads for `ScriptContext`, `CKBehaviorContext`, and `CKContext@`. Use the natural context for the script you are writing:

```angelscript
void Update(const ScriptContext &in ctx) {
  SceneRef@ current = Scene::CurrentScene(ctx);
  array<ObjectRef@>@ meshes = Scene::FindAll(ctx, "", CKCID_MESH);
}

class Component {
  void Start(const CKBehaviorContext &in ctx) {
    ObjectRef@ target = Scene::Target(ctx);
  }
}
```

## Lookup And Creation

`Scene::ById` and `Scene::Ref` create safe refs from existing objects. `Scene::Find` / `FindAll` search by name and class id, with derived-class matching enabled by default. The generic helpers return `ObjectRef@` / `array<ObjectRef@>@` and preserve the most specific wrapper internally, so `cast<TRef>(obj)` is checked and returns `null` for the wrong type.

Typed finders cover common object classes: `FindEntity3D`, `FindEntity2D`, `FindMaterial`, `FindTexture`, `FindMesh`, and `FindBehavior`.

Creation defaults to dynamic objects:

```angelscript
Entity3DRef@ marker = Scene::CreateEntity3D(ctx, "DebugMarker");
Scene::AddToCurrentScene(ctx, marker);
```

Pass `dynamic=false` only when a persistent object is explicitly required.

## Scene Membership, Selection, And Destruction

`AddToCurrentScene` and `RemoveFromCurrentScene` accept `ObjectRef@` but require the resolved object to be a `CKSceneObject`. `Select` accepts `array<ObjectRef@>@`.

`Destroy` refuses invalid refs and persistent objects by default:

```angelscript
if (!Scene::Destroy(ctx, marker)) {
  ctx.Raise(marker.Error());
}
```

Use `allowPersistent=true` only for deliberate cleanup of non-dynamic objects. Stale refs remain usable as diagnostics: after a successful destruction, `ref.valid` becomes `false` and typed accessors return `null`.
