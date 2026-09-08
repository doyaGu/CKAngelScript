# Scene Interop API

`Scene` is the high-level Virtools scene interop namespace for runtime entries and AngelScript Components. It sits above raw `CKContext` / `CKObject` bindings and returns checked `ObjectRef@` handles instead of encouraging scripts to cache SDK pointers directly.

## Object Refs

`ObjectRef@` stores the source `CKContext`, object id, native object address, and object stamp. Each accessor revalidates the object so deleted or reused ids become invalid.

```angelscript
ObjectRef@ ref = Scene::Find(ctx, "Ball");
Entity3DRef@ ball = cast<Entity3DRef>(ref);
if (ball !is null && ball.valid) {
  string identity = ball.Describe();
}
```

All object refs support `IsValid()`, `valid`, `Error()`, `Describe()`, `Id()`, `Name()`, `ClassId()`, and `IsDynamic()`. They deliberately do not expose raw CK object pointers. Invalid refs are still useful diagnostic values: `valid` becomes false and `Error()` explains the last known failure where the helper can provide one.

Precise ref types provide checked narrowing and make the expected object class explicit. Bridge refs such as `BehaviorRef@`, `ParamRef@`, `ParamStructRef@`, `ParamOperationRef@`, and `BehaviorLinkRef@` are also `ObjectRef@`-compatible. Transaction handles, values, builders, tasks, and graph cursors stay outside the ref hierarchy; use their methods to obtain `ObjectRef@`-derived handles when needed.

## Typed Ref Matrix

| Ref type | Checked relationship | Typical source |
| --- | --- | --- |
| `ObjectRef@` | Can narrow to any supported precise ref | `Scene::Find`, `Scene::ById`, `Scene::Ref` |
| `SceneObjectRef@` | Can narrow to 2D/3D entity or scene | Scene-membership helpers |
| `Entity3DRef@` | Upcasts to `SceneObjectRef@` / `ObjectRef@` | `FindEntity3D`, `CreateEntity3D` |
| `Entity2DRef@` | Upcasts to `SceneObjectRef@` / `ObjectRef@` | `FindEntity2D`, `CreateEntity2D` |
| `MaterialRef@` | Upcasts to `ObjectRef@` | `FindMaterial`, `CreateMaterial` |
| `TextureRef@` | Upcasts to `ObjectRef@` | `FindTexture`, `CreateTexture` |
| `MeshRef@` | Upcasts to `ObjectRef@` | `FindMesh`, `CreateMesh` |
| `BehaviorRef@` | Upcasts to `ObjectRef@` | `Scene::FindBehavior`, `Behavior::Find` |
| `SceneRef@` | Upcasts to `SceneObjectRef@` / `ObjectRef@` | `CurrentScene`, `AddToScene` |
| `LevelRef@` | Upcasts to `ObjectRef@` | `CurrentLevel` |

## Low-Level SDK Editing

`ObjectRef@`-derived handles are an identity-safe high-level interface, not an Adapter back to raw pointers. Code that deliberately needs the low-level SDK starts from `CKContext@` directly and keeps the resulting pointer local to that operation:

```angelscript
CKContext@ host = frame.Context();
CKObject@ object = host.GetObjectByNameAndParentClass("DebugMarker", CKCID_3DENTITY, null);
CK3dEntity@ marker = cast<CK3dEntity>(object);
if (marker !is null) {
  marker.Translate(VxVector(1.0f, 0.0f, 0.0f));
}
```

Do not cache that raw pointer as durable script state. Prefer focused `Scene`, `Behavior`, `BB`, and `Param` operations wherever they cover the required mutation.

## Context Entry Points

Every `Scene::*` helper has overloads for `ScriptContext` and `CKContext@`. Runtime Entries pass `ScriptContext`. Component and Executor code takes the host from its `BehaviorFrame`; it uses the frame itself for behavior-specific owner/target identity:

```angelscript
void Update(const ScriptContext &in ctx) {
  SceneRef@ current = Scene::CurrentScene(ctx);
  array<ObjectRef@>@ meshes = Scene::FindAll(ctx, "", CKCID_MESH);
}

class Component {
  void Start(BehaviorFrame@ frame) {
    CKContext@ host = frame.Context();
    ObjectRef@ target = Scene::Ref(host, frame.Target());
  }
}
```

## Lookup And Creation

`Scene::ById` and `Scene::Ref` create safe refs from existing objects. `Scene::Find` / `FindAll` search by name and class id, with derived-class matching enabled by default. Pass `currentSceneOnly=true` when global duplicate names are possible and the script only wants objects visible to the current scene. Scene objects use direct scene membership. Asset refs are resolved with a query-local known dependency index covering sprite, 2D material, 3D mesh/material, and texture paths; this is not a full CK dependency graph traversal. Behavior scoped lookup also considers the owner/root-owner scene object. The generic helpers return `ObjectRef@` / `array<ObjectRef@>@` and preserve the most specific wrapper internally, so `cast<TRef>(obj)` is checked and returns `null` for the wrong type.

Lookup behavior:

| Helper | Missing result | Duplicate result |
| --- | --- | --- |
| `Find*` | Invalid/null ref depending on helper | Uses `occurrence`. |
| `FindOne*` | Invalid ref with diagnostics | Invalid ref with match-count diagnostics. |
| `FindAll*` | Empty array | All matching refs. |
| `ById` | Invalid ref | Not applicable. |
| `Ref` | Invalid ref when object/context is missing | Not applicable. |

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

## Assets vs Scene Objects

Scene objects have direct membership in scenes. Assets such as textures, materials, and meshes are usually observed through the entities that reference them. Add or remove the owning scene object when you want visibility changes; do not expect adding a material or texture ref to a scene to make it visible by itself.

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

Use `allowPersistent=true` only for deliberate cleanup of non-dynamic objects. Stale refs remain usable as diagnostics: after a successful destruction, `ref.valid` becomes `false`.

Low-level raw pointers obtained independently through `CKContext@` become unsafe after `Destroy`, scene reset, or level clear.

## Runtime vs Component Contexts

Use the entry point matching your script:

```angelscript
Scene::FindEntity3D(const ScriptContext &in ctx, const string &in name = "", int occurrence = 0, bool currentSceneOnly = false)
Scene::FindEntity3D(CKContext@ ctx, const string &in name = "", int occurrence = 0, bool currentSceneOnly = false)
```

Runtime Entries pass `ScriptContext`. Components and Executors pass `frame.Context()` to scene operations and use `BehaviorFrame` for the current behavior's owner, target, graph, and lifecycle. Shared utility code can pass `CKContext@` directly.
