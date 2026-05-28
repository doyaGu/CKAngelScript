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

All object refs support `IsValid()`, `valid`, `Error()`, `Describe()`, `Id()`, `Name()`, `ClassId()`, `IsDynamic()`, and `Object()`. They deliberately do not mirror the full CK SDK editing surface; use typed accessors when you need raw SDK methods. Invalid refs are still useful diagnostic values: `valid` becomes false, typed accessors return `null`, and `Error()` explains the last known failure where the helper can provide one.

Precise ref types add typed accessors: `SceneObjectRef.SceneObject()`, `Entity3DRef.Entity3D()`, `Entity2DRef.Entity2D()`, `MaterialRef.Material()`, `TextureRef.Texture()`, `MeshRef.Mesh()`, `SceneRef.Scene()`, and `LevelRef.Level()`. Bridge refs such as `BehaviorRef@`, `ParamRef@`, `ParamStructRef@`, `ParamOperationRef@`, and `BehaviorLinkRef@` are also `ObjectRef@`-compatible. Transaction handles, values, builders, tasks, and graph cursors stay outside the ref hierarchy; use their methods to obtain `ObjectRef@`-derived handles when needed.

## Editing With Raw SDK Accessors

Prefer `ObjectRef@`-derived handles for lookup, identity, stale-id protection, and typed narrowing. For actual scene editing, resolve a short-lived raw SDK pointer from the typed accessor and call the registered SDK API directly.

```angelscript
Entity3DRef@ marker = Scene::CreateEntity3D(ctx, "DebugMarker");
Entity3DRef@ parent = Scene::FindEntity3D(ctx, "Level");

CK3dEntity@ markerEntity = marker !is null ? marker.Entity3D() : null;
CK3dEntity@ parentEntity = parent !is null ? parent.Entity3D() : null;
if (markerEntity !is null) {
  markerEntity.SetParent(parentEntity);
  markerEntity.SetPosition(0.0f, 2.0f, 0.0f, parentEntity);
  markerEntity.Translate(VxVector(1.0f, 0.0f, 0.0f));
}
```

The same pattern applies to `Entity2DRef.Entity2D()`, `MaterialRef.Material()`, `TextureRef.Texture()`, `MeshRef.Mesh()`, and other precise refs. Avoid storing raw pointers long-term; keep `ObjectRef@`-derived handles as the durable script-side identity and reacquire raw SDK pointers near the operation.

## Context Overloads

Every `Scene::*` helper has overloads for `ScriptContext`, `CKBehaviorContext`, and `CKContext@`. Runtime scripts should pass `ScriptContext`; AngelScript Components should pass `CKBehaviorContext`; low-level utility code can pass `CKContext@` when it is not tied to either runtime. Use the natural context for the script you are writing:

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

`Scene::ById` and `Scene::Ref` create safe refs from existing objects. `Scene::Find` / `FindAll` search by name and class id, with derived-class matching enabled by default. Pass `currentSceneOnly=true` when global duplicate names are possible and the script only wants objects visible to the current scene. Scene objects use direct scene membership. Asset refs are resolved with a query-local known dependency index covering sprite, 2D material, 3D mesh/material, and texture paths; this is not a full CK dependency graph traversal. Behavior scoped lookup also considers the owner/root-owner scene object. The generic helpers return `ObjectRef@` / `array<ObjectRef@>@` and preserve the most specific wrapper internally, so `cast<TRef>(obj)` is checked and returns `null` for the wrong type.

Lookup failures do not throw. Single-result helpers return an invalid ref when the target is missing or ambiguous; batch helpers return an empty array when no matches exist. Prefer `FindOne*` for configuration objects where duplicates indicate bad scene data, and prefer `FindAll*` for repeated gameplay objects.

Use `Find` with `occurrence` when selecting from a known duplicate set. Use `FindOne` when duplicates are a configuration error: it returns an invalid ref unless exactly one object matches, and `Error()` includes the match count, class id, and lookup scope.

Typed finders cover common object classes: `FindEntity3D`, `FindEntity2D`, `FindMaterial`, `FindTexture`, `FindMesh`, and `FindBehavior`.

Typed strict and batch helpers avoid manual casts:

```angelscript
Entity3DRef@ player = Scene::FindOneEntity3D(ctx, "Player", true);
if (!player.valid) {
  string reason = player.Error();
}

array<Entity3DRef@>@ markers = Scene::FindAllEntity3D(ctx, "DebugMarker", true);
array<BehaviorRef@>@ behaviors = Scene::FindAllBehavior(ctx, "", false);
```

Creation defaults to dynamic objects:

```angelscript
Entity3DRef@ marker = Scene::CreateEntity3D(ctx, "DebugMarker");
Scene::AddToCurrentScene(ctx, marker);
```

Pass `dynamic=false` only when a persistent object is explicitly required. Creation helpers return invalid refs when the CK context cannot create the requested class; check `valid` before mutating or adding the object to a scene.

## Scene Membership, Selection, And Destruction

`AddToCurrentScene` and `RemoveFromCurrentScene` accept `ObjectRef@` but require an addable scene object. Use `AddToScene` / `RemoveFromScene` with a `SceneRef@` when working with a scene that is not necessarily current. Asset refs are observed through the scene objects that use them; add or remove the owning entity instead of adding materials, textures, or meshes directly. `IsInCurrentScene` and `IsInScene` expose the same checks used by scoped lookup and can be used with asset refs. `Select` accepts `array<ObjectRef@>@`.

```angelscript
SceneRef@ scene = Scene::CurrentScene(ctx);
Entity3DRef@ marker = Scene::CreateEntity3D(ctx, "DebugMarker");

Scene::AddToScene(ctx, scene, marker);
bool visibleToScopedLookup = Scene::IsInCurrentScene(ctx, marker);
Scene::RemoveFromScene(ctx, scene, marker);
```

`Destroy` refuses invalid refs and persistent objects by default:

```angelscript
if (!Scene::Destroy(ctx, marker)) {
  string reason = marker.Error();
}
```

Use `allowPersistent=true` only for deliberate cleanup of non-dynamic objects. Stale refs remain usable as diagnostics: after a successful destruction, `ref.valid` becomes `false` and typed accessors return `null`.

Destruction does not make previously copied raw CK pointers safe. If a script needs to perform several operations, keep the durable `ObjectRef@`, reacquire the raw pointer immediately before each SDK call, and stop using any raw pointer after `Destroy`, scene reset, or level clear.
