#include "ScriptScene.h"

#include <cassert>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptRuntime.h"

#undef GetObject

namespace {

CKContext *ContextFrom(const CKBehaviorContext &ctx) {
    if (ctx.Context) {
        return ctx.Context;
    }
    return ctx.Behavior ? ctx.Behavior->GetCKContext() : nullptr;
}

CKContext *ContextFrom(const ScriptContext &ctx) {
    return ctx.Context();
}

ScriptBehaviorBridge *BridgeFrom(CKContext *context) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

ScriptBehaviorBridge *BridgeFrom(const CKBehaviorContext &ctx) {
    ScriptManager *manager = ManagerFromContext(ctx);
    return manager ? manager->GetBehaviorBridge() : BridgeFrom(ContextFrom(ctx));
}

ScriptBehaviorBridge *BridgeFrom(const ScriptContext &ctx) {
    ScriptManager *manager = ScriptManager::GetManager(ctx.Context());
    return manager ? manager->GetBehaviorBridge() : BridgeFrom(ctx.Context());
}

ObjectRef *MakeRef(CKContext *context,
                   CKObject *object,
                   const std::string &error = std::string(),
                   ScriptBehaviorBridge *bridge = nullptr,
                   CK_ID componentId = 0) {
    return MakeScriptObjectRef(context, object, bridge, componentId, error);
}

ObjectRef *MakeInvalid(CKContext *context, const std::string &error) {
    return MakeInvalidObjectRef(context, error);
}

template <typename T>
T *MakeTypedRef(CKContext *context, CKObject *object, const std::string &error = std::string()) {
    CKContext *resolvedContext = object ? object->GetCKContext() : context;
    if (!object) {
        return new T(resolvedContext, 0, ScriptBridgeObjectStamp(), error.empty() ? "Object is null." : error);
    }
    return new T(resolvedContext, object);
}

bool NameMatches(CKObject *object, const std::string &name) {
    return name.empty() || NameEquals(object ? object->GetName() : nullptr, name);
}

bool IsInScene(CKSceneObject *object, CKScene *scene) {
    if (!object || !scene) {
        return false;
    }
    const int count = object->GetSceneInCount();
    for (int i = 0; i < count; ++i) {
        if (object->GetSceneIn(i) == scene) {
            return true;
        }
    }
    return false;
}

std::vector<CKObject *> CollectObjects(CKContext *context,
                                       const std::string &name,
                                       CK_CLASSID cid,
                                       bool derived,
                                       bool currentSceneOnly) {
    std::vector<CKObject *> objects;
    if (!context) {
        return objects;
    }
    CKScene *scene = currentSceneOnly ? context->GetCurrentScene() : nullptr;
    const XObjectPointerArray &raw = context->GetObjectListByType(cid, derived);
    const int size = raw.Size();
    objects.reserve(static_cast<std::size_t>(size));
    for (int i = 0; i < size; ++i) {
        CKObject *object = raw[i];
        if (!object || object->IsToBeDeleted() || !NameMatches(object, name)) {
            continue;
        }
        if (currentSceneOnly && !IsInScene(CKSceneObject::Cast(object), scene)) {
            continue;
        }
        objects.push_back(object);
    }
    return objects;
}

CScriptArray *CreateObjectRefArray(asIScriptEngine *engine) {
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl("array<ObjectRef@>") : nullptr;
    if (!arrayType) {
        SetScriptException("array<ObjectRef@> is not registered.");
        return nullptr;
    }
    return CScriptArray::Create(arrayType, asUINT(0));
}

void AppendObjectRef(CScriptArray *array, ObjectRef *ref) {
    if (!array || !ref) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &ref);
    ref->Release();
}

CScriptArray *FindAllImpl(CKContext *context,
                          const std::string &name,
                          CK_CLASSID cid,
                          bool derived,
                          bool currentSceneOnly,
                          ScriptBehaviorBridge *bridge = nullptr,
                          CK_ID componentId = 0) {
    asIScriptContext *active = asGetActiveContext();
    asIScriptEngine *engine = active ? active->GetEngine() : nullptr;
    CScriptArray *array = CreateObjectRefArray(engine);
    if (!array) {
        return nullptr;
    }
    for (CKObject *object : CollectObjects(context, name, cid, derived, currentSceneOnly)) {
        AppendObjectRef(array, MakeRef(context, object, std::string(), bridge, componentId));
    }
    return array;
}

ObjectRef *FindImpl(CKContext *context,
                    const std::string &name,
                    CK_CLASSID cid,
                    bool derived,
                    int occurrence,
                    ScriptBehaviorBridge *bridge = nullptr,
                    CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    if (occurrence < 0) {
        return MakeInvalid(context, "Scene::Find occurrence must be non-negative.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, derived, false);
    if (occurrence >= static_cast<int>(objects.size())) {
        return MakeInvalid(context, fmt::format("Scene object '{}' was not found.", name.empty() ? "<any>" : name));
    }
    return MakeRef(context, objects[static_cast<std::size_t>(occurrence)], std::string(), bridge, componentId);
}

template <typename T>
T *FindTypedImpl(CKContext *context,
                 const std::string &name,
                 CK_CLASSID cid,
                 int occurrence,
                 const char *label) {
    if (!context) {
        return new T(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    if (occurrence < 0) {
        return new T(context, 0, ScriptBridgeObjectStamp(), "Scene::Find occurrence must be non-negative.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, true, false);
    if (occurrence >= static_cast<int>(objects.size())) {
        return new T(context,
                     0,
                     ScriptBridgeObjectStamp(),
                     fmt::format("{} '{}' was not found.", label, name.empty() ? "<any>" : name));
    }
    return MakeTypedRef<T>(context, objects[static_cast<std::size_t>(occurrence)]);
}

BehaviorRef *FindBehaviorImpl(CKContext *context,
                              const std::string &name,
                              int occurrence,
                              ScriptBehaviorBridge *bridge,
                              CK_ID componentId) {
    auto invalid = [&](const std::string &message) {
        BehaviorRef *ref = new BehaviorRef(bridge, 0, componentId, context);
        ref->SetError(message);
        return ref;
    };
    if (!context) {
        return invalid("CKContext is not available.");
    }
    if (occurrence < 0) {
        return invalid("Scene::Find occurrence must be non-negative.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, CKCID_BEHAVIOR, true, false);
    if (occurrence >= static_cast<int>(objects.size())) {
        return invalid(fmt::format("CKBehavior '{}' was not found.", name.empty() ? "<any>" : name));
    }
    CKBehavior *behavior = CKBehavior::Cast(objects[static_cast<std::size_t>(occurrence)]);
    if (!behavior) {
        return invalid("Matched object is not a CKBehavior.");
    }
    return new BehaviorRef(bridge, behavior->GetID(), componentId, behavior->GetCKContext());
}

ObjectRef *ByIdImpl(CKContext *context, CK_ID id, ScriptBehaviorBridge *bridge = nullptr, CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    CKObject *object = GetCKObjectById(context, id);
    if (!object || object->IsToBeDeleted()) {
        return MakeInvalid(context, fmt::format("Scene object id {} was not found.", static_cast<int>(id)));
    }
    return MakeRef(context, object, std::string(), bridge, componentId);
}

LevelRef *CurrentLevelImpl(CKContext *context) {
    if (!context) {
        return new LevelRef(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    return MakeTypedRef<LevelRef>(context, context->GetCurrentLevel(), "Current level is not available.");
}

SceneRef *CurrentSceneImpl(CKContext *context) {
    if (!context) {
        return new SceneRef(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    return MakeTypedRef<SceneRef>(context, context->GetCurrentScene(), "Current scene is not available.");
}

ObjectRef *TargetImpl(const CKBehaviorContext &ctx, ScriptBehaviorBridge *bridge = nullptr) {
    CKContext *context = ContextFrom(ctx);
    CKBeObject *target = ctx.Behavior ? ctx.Behavior->GetTarget() : nullptr;
    return MakeRef(context, target, "Behavior target is not available.", bridge, ComponentIdFromContext(ctx));
}

ObjectRef *OwnerImpl(const CKBehaviorContext &ctx, ScriptBehaviorBridge *bridge = nullptr) {
    CKContext *context = ContextFrom(ctx);
    CKObject *owner = ctx.Behavior ? ctx.Behavior->GetOwner() : nullptr;
    return MakeRef(context, owner, "Behavior owner is not available.", bridge, ComponentIdFromContext(ctx));
}

ObjectRef *CreateImpl(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic, ScriptBehaviorBridge *bridge = nullptr, CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    CKObject *object = context->CreateObject(cid,
                                            name.empty() ? nullptr : const_cast<CKSTRING>(name.c_str()),
                                            dynamic ? CK_OBJECTCREATION_DYNAMIC : CK_OBJECTCREATION_NONAMECHECK);
    if (!object) {
        return MakeInvalid(context, fmt::format("Scene::Create failed for class {}.", static_cast<int>(cid)));
    }
    return MakeRef(context, object, std::string(), bridge, componentId);
}

template <typename T>
T *CreateTypedImpl(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic) {
    if (!context) {
        return new T(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    CKObject *object = context->CreateObject(cid,
                                            name.empty() ? nullptr : const_cast<CKSTRING>(name.c_str()),
                                            dynamic ? CK_OBJECTCREATION_DYNAMIC : CK_OBJECTCREATION_NONAMECHECK);
    if (!object) {
        return new T(context, 0, ScriptBridgeObjectStamp(), fmt::format("Scene::Create failed for class {}.", static_cast<int>(cid)));
    }
    return MakeTypedRef<T>(context, object);
}

bool AddToCurrentSceneImpl(CKContext *context, ObjectRef *ref, bool dependencies) {
    if (!context || !ref) {
        return false;
    }
    CKScene *scene = context->GetCurrentScene();
    if (!scene) {
        ref->SetError("Current scene is not available.");
        return false;
    }
    CKSceneObject *object = CKSceneObject::Cast(ref->Object());
    if (!object) {
        ref->SetError("Scene::AddToCurrentScene requires a CKSceneObject.");
        return false;
    }
    scene->AddObjectToScene(object, dependencies);
    return true;
}

bool RemoveFromCurrentSceneImpl(CKContext *context, ObjectRef *ref, bool dependencies) {
    if (!context || !ref) {
        return false;
    }
    CKScene *scene = context->GetCurrentScene();
    if (!scene) {
        ref->SetError("Current scene is not available.");
        return false;
    }
    CKSceneObject *object = CKSceneObject::Cast(ref->Object());
    if (!object) {
        ref->SetError("Scene::RemoveFromCurrentScene requires a CKSceneObject.");
        return false;
    }
    scene->RemoveObjectFromScene(object, dependencies);
    return true;
}

bool DestroyImpl(CKContext *context, ObjectRef *ref, bool allowPersistent, CKDWORD flags) {
    if (!context || !ref) {
        return false;
    }
    CKObject *object = ref->Object();
    if (!object) {
        ref->SetError("Scene::Destroy requires a valid object reference.");
        return false;
    }
    if (!allowPersistent && !object->IsDynamic()) {
        ref->SetError("Scene::Destroy refuses persistent objects unless allowPersistent is true.");
        return false;
    }
    CKContext *ownerContext = object->GetCKContext() ? object->GetCKContext() : context;
    const CKERROR rc = ownerContext->DestroyObject(object, flags, nullptr);
    if (rc != CK_OK) {
        ref->SetError(fmt::format("CKContext::DestroyObject failed with {}.", rc));
        return false;
    }
    ref->Invalidate("Scene object was destroyed.");
    return true;
}

bool SelectImpl(CKContext *context, CScriptArray *objects, bool clearSelection) {
    if (!context || !objects) {
        return false;
    }
    XObjectArray ids;
    const asUINT count = objects->GetSize();
    for (asUINT i = 0; i < count; ++i) {
        ObjectRef *ref = *static_cast<ObjectRef **>(objects->At(i));
        if (!ref) {
            return false;
        }
        CKObject *object = ref->Object();
        if (!object) {
            ref->SetError("Scene::Select requires valid object references.");
            return false;
        }
        ids.PushBack(object->GetID());
    }
    return context->Select(ids, clearSelection) == CK_OK;
}

// CKContext overloads.
LevelRef *CurrentLevelCtx(CKContext *context) { return CurrentLevelImpl(context); }
SceneRef *CurrentSceneCtx(CKContext *context) { return CurrentSceneImpl(context); }
ObjectRef *TargetCtx(CKContext *context) { return MakeInvalid(context, "CKContext has no behavior target."); }
ObjectRef *OwnerCtx(CKContext *context) { return MakeInvalid(context, "CKContext has no behavior owner."); }
ObjectRef *RefCtx(CKContext *context, CKObject *object) { return MakeRef(context, object); }
ObjectRef *ByIdCtx(CKContext *context, CK_ID id) { return ByIdImpl(context, id); }
ObjectRef *FindCtx(CKContext *context, const std::string &name, CK_CLASSID cid, bool derived, int occurrence) { return FindImpl(context, name, cid, derived, occurrence); }
CScriptArray *FindAllCtx(CKContext *context, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(context, name, cid, derived, currentSceneOnly); }
Entity3DRef *FindEntity3DCtx(CKContext *context, const std::string &name, int occurrence) { return FindTypedImpl<Entity3DRef>(context, name, CKCID_3DENTITY, occurrence, "CK3dEntity"); }
Entity2DRef *FindEntity2DCtx(CKContext *context, const std::string &name, int occurrence) { return FindTypedImpl<Entity2DRef>(context, name, CKCID_2DENTITY, occurrence, "CK2dEntity"); }
MaterialRef *FindMaterialCtx(CKContext *context, const std::string &name, int occurrence) { return FindTypedImpl<MaterialRef>(context, name, CKCID_MATERIAL, occurrence, "CKMaterial"); }
TextureRef *FindTextureCtx(CKContext *context, const std::string &name, int occurrence) { return FindTypedImpl<TextureRef>(context, name, CKCID_TEXTURE, occurrence, "CKTexture"); }
MeshRef *FindMeshCtx(CKContext *context, const std::string &name, int occurrence) { return FindTypedImpl<MeshRef>(context, name, CKCID_MESH, occurrence, "CKMesh"); }
BehaviorRef *FindBehaviorCtx(CKContext *context, const std::string &name, int occurrence) { return FindBehaviorImpl(context, name, occurrence, BridgeFrom(context), 0); }
ObjectRef *CreateCtx(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(context, cid, name, dynamic); }
Entity3DRef *CreateEntity3DCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<Entity3DRef>(context, CKCID_3DENTITY, name, dynamic); }
Entity2DRef *CreateEntity2DCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<Entity2DRef>(context, CKCID_2DENTITY, name, dynamic); }
MaterialRef *CreateMaterialCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<MaterialRef>(context, CKCID_MATERIAL, name, dynamic); }
TextureRef *CreateTextureCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<TextureRef>(context, CKCID_TEXTURE, name, dynamic); }
MeshRef *CreateMeshCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<MeshRef>(context, CKCID_MESH, name, dynamic); }
bool AddToCurrentSceneCtx(CKContext *context, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(context, ref, dependencies); }
bool RemoveFromCurrentSceneCtx(CKContext *context, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(context, ref, dependencies); }
bool DestroyCtx(CKContext *context, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(context, ref, allowPersistent, flags); }
bool SelectCtx(CKContext *context, CScriptArray *objects, bool clearSelection) { return SelectImpl(context, objects, clearSelection); }

// CKBehaviorContext overloads.
LevelRef *CurrentLevelBehavior(const CKBehaviorContext &ctx) { return ctx.CurrentLevel ? MakeTypedRef<LevelRef>(ContextFrom(ctx), ctx.CurrentLevel) : CurrentLevelImpl(ContextFrom(ctx)); }
SceneRef *CurrentSceneBehavior(const CKBehaviorContext &ctx) { return ctx.CurrentScene ? MakeTypedRef<SceneRef>(ContextFrom(ctx), ctx.CurrentScene) : CurrentSceneImpl(ContextFrom(ctx)); }
ObjectRef *TargetBehavior(const CKBehaviorContext &ctx) { return TargetImpl(ctx, BridgeFrom(ctx)); }
ObjectRef *OwnerBehavior(const CKBehaviorContext &ctx) { return OwnerImpl(ctx, BridgeFrom(ctx)); }
ObjectRef *RefBehavior(const CKBehaviorContext &ctx, CKObject *object) { return MakeRef(ContextFrom(ctx), object, std::string(), BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *ByIdBehavior(const CKBehaviorContext &ctx, CK_ID id) { return ByIdImpl(ContextFrom(ctx), id, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *FindBehaviorContext(const CKBehaviorContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, int occurrence) { return FindImpl(ContextFrom(ctx), name, cid, derived, occurrence, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
CScriptArray *FindAllBehavior(const CKBehaviorContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
Entity3DRef *FindEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindEntity3DCtx(ContextFrom(ctx), name, occurrence); }
Entity2DRef *FindEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindEntity2DCtx(ContextFrom(ctx), name, occurrence); }
MaterialRef *FindMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindMaterialCtx(ContextFrom(ctx), name, occurrence); }
TextureRef *FindTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindTextureCtx(ContextFrom(ctx), name, occurrence); }
MeshRef *FindMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindMeshCtx(ContextFrom(ctx), name, occurrence); }
BehaviorRef *FindBehaviorBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence) { return FindBehaviorImpl(ContextFrom(ctx), name, occurrence, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *CreateBehavior(const CKBehaviorContext &ctx, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(ContextFrom(ctx), cid, name, dynamic, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
Entity3DRef *CreateEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateEntity3DCtx(ContextFrom(ctx), name, dynamic); }
Entity2DRef *CreateEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateEntity2DCtx(ContextFrom(ctx), name, dynamic); }
MaterialRef *CreateMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateMaterialCtx(ContextFrom(ctx), name, dynamic); }
TextureRef *CreateTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateTextureCtx(ContextFrom(ctx), name, dynamic); }
MeshRef *CreateMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateMeshCtx(ContextFrom(ctx), name, dynamic); }
bool AddToCurrentSceneBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool RemoveFromCurrentSceneBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool DestroyBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(ContextFrom(ctx), ref, allowPersistent, flags); }
bool SelectBehavior(const CKBehaviorContext &ctx, CScriptArray *objects, bool clearSelection) { return SelectImpl(ContextFrom(ctx), objects, clearSelection); }

// ScriptContext overloads.
LevelRef *CurrentLevelScript(const ScriptContext &ctx) { return CurrentLevelImpl(ContextFrom(ctx)); }
SceneRef *CurrentSceneScript(const ScriptContext &ctx) { return CurrentSceneImpl(ContextFrom(ctx)); }
ObjectRef *TargetScript(const ScriptContext &ctx) { return TargetImpl(ctx.ToBehaviorContext(), BridgeFrom(ctx)); }
ObjectRef *OwnerScript(const ScriptContext &ctx) { return OwnerImpl(ctx.ToBehaviorContext(), BridgeFrom(ctx)); }
ObjectRef *RefScript(const ScriptContext &ctx, CKObject *object) { return MakeRef(ContextFrom(ctx), object, std::string(), BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *ByIdScript(const ScriptContext &ctx, CK_ID id) { return ByIdImpl(ContextFrom(ctx), id, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *FindScript(const ScriptContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, int occurrence) { return FindImpl(ContextFrom(ctx), name, cid, derived, occurrence, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
CScriptArray *FindAllScript(const ScriptContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
Entity3DRef *FindEntity3DScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindEntity3DCtx(ContextFrom(ctx), name, occurrence); }
Entity2DRef *FindEntity2DScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindEntity2DCtx(ContextFrom(ctx), name, occurrence); }
MaterialRef *FindMaterialScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindMaterialCtx(ContextFrom(ctx), name, occurrence); }
TextureRef *FindTextureScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindTextureCtx(ContextFrom(ctx), name, occurrence); }
MeshRef *FindMeshScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindMeshCtx(ContextFrom(ctx), name, occurrence); }
BehaviorRef *FindBehaviorScript(const ScriptContext &ctx, const std::string &name, int occurrence) { return FindBehaviorImpl(ContextFrom(ctx), name, occurrence, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *CreateScript(const ScriptContext &ctx, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(ContextFrom(ctx), cid, name, dynamic, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
Entity3DRef *CreateEntity3DScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateEntity3DCtx(ContextFrom(ctx), name, dynamic); }
Entity2DRef *CreateEntity2DScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateEntity2DCtx(ContextFrom(ctx), name, dynamic); }
MaterialRef *CreateMaterialScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateMaterialCtx(ContextFrom(ctx), name, dynamic); }
TextureRef *CreateTextureScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateTextureCtx(ContextFrom(ctx), name, dynamic); }
MeshRef *CreateMeshScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateMeshCtx(ContextFrom(ctx), name, dynamic); }
bool AddToCurrentSceneScript(const ScriptContext &ctx, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool RemoveFromCurrentSceneScript(const ScriptContext &ctx, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool DestroyScript(const ScriptContext &ctx, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(ContextFrom(ctx), ref, allowPersistent, flags); }
bool SelectScript(const ScriptContext &ctx, CScriptArray *objects, bool clearSelection) { return SelectImpl(ContextFrom(ctx), objects, clearSelection); }

} // namespace

void RegisterScriptSceneCore(asIScriptEngine *engine) {
    assert(engine != nullptr);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    int r = engine->SetDefaultNamespace("Scene"); assert(r >= 0);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(CKContext@ ctx)", asFUNCTION(CurrentLevelCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(CKContext@ ctx)", asFUNCTION(CurrentSceneCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(CKContext@ ctx)", asFUNCTION(TargetCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(CKContext@ ctx)", asFUNCTION(OwnerCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(CKContext@ ctx, CKObject@ obj)", asFUNCTION(RefCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(CKContext@ ctx, CK_ID id)", asFUNCTION(ByIdCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(CKContext@ ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0)", asFUNCTION(FindCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(CKContext@ ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity3DCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity2DCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMaterialCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindTextureCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMeshCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(CKContext@ ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindBehaviorCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(CKContext@ ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(CKContext@ ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(CKContext@ ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Destroy(CKContext@ ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyCtx), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Select(CKContext@ ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectCtx), asCALL_CDECL); assert(r >= 0);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(const CKBehaviorContext &in ctx)", asFUNCTION(CurrentLevelBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(const CKBehaviorContext &in ctx)", asFUNCTION(CurrentSceneBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(const CKBehaviorContext &in ctx)", asFUNCTION(TargetBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(const CKBehaviorContext &in ctx)", asFUNCTION(OwnerBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(const CKBehaviorContext &in ctx, CKObject@ obj)", asFUNCTION(RefBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(const CKBehaviorContext &in ctx, CK_ID id)", asFUNCTION(ByIdBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(const CKBehaviorContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0)", asFUNCTION(FindBehaviorContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(const CKBehaviorContext &in ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity3DBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity2DBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMaterialBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindTextureBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMeshBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindBehaviorBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(const CKBehaviorContext &in ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Destroy(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Select(const CKBehaviorContext &in ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectBehavior), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

void RegisterScriptSceneRuntime(asIScriptEngine *engine) {
    assert(engine != nullptr);
    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    int r = engine->SetDefaultNamespace("Scene"); assert(r >= 0);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(const ScriptContext &in ctx)", asFUNCTION(CurrentLevelScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(const ScriptContext &in ctx)", asFUNCTION(CurrentSceneScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(const ScriptContext &in ctx)", asFUNCTION(TargetScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(const ScriptContext &in ctx)", asFUNCTION(OwnerScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(const ScriptContext &in ctx, CKObject@ obj)", asFUNCTION(RefScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(const ScriptContext &in ctx, CK_ID id)", asFUNCTION(ByIdScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(const ScriptContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0)", asFUNCTION(FindScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(const ScriptContext &in ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity3DScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindEntity2DScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMaterialScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindTextureScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindMeshScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0)", asFUNCTION(FindBehaviorScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(const ScriptContext &in ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(const ScriptContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(const ScriptContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Destroy(const ScriptContext &in ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyScript), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Select(const ScriptContext &in ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectScript), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}
