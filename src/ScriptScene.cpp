#include "ScriptScene.h"

#include <cassert>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptRuntime.h"
#include "ScriptRegistration.h"

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

template <typename T>
CKObject *CastTypedTarget(CKObject *object) {
    return object;
}

template <>
CKObject *CastTypedTarget<SceneObjectRef>(CKObject *object) { return CKSceneObject::Cast(object); }
template <>
CKObject *CastTypedTarget<Entity3DRef>(CKObject *object) { return CK3dEntity::Cast(object); }
template <>
CKObject *CastTypedTarget<Entity2DRef>(CKObject *object) { return CK2dEntity::Cast(object); }
template <>
CKObject *CastTypedTarget<MaterialRef>(CKObject *object) { return CKMaterial::Cast(object); }
template <>
CKObject *CastTypedTarget<TextureRef>(CKObject *object) { return CKTexture::Cast(object); }
template <>
CKObject *CastTypedTarget<MeshRef>(CKObject *object) { return CKMesh::Cast(object); }
template <>
CKObject *CastTypedTarget<SceneRef>(CKObject *object) { return CKScene::Cast(object); }
template <>
CKObject *CastTypedTarget<LevelRef>(CKObject *object) { return CKLevel::Cast(object); }

std::string TypeMismatchError(const char *label, CKObject *object) {
    return fmt::format("Expected {}, got object '{}' id={} class={}.",
                       label ? label : "typed object",
                       object ? SafeString(object->GetName()) : "<null>",
                       object ? object->GetID() : 0,
                       object ? static_cast<int>(object->GetClassID()) : 0);
}

template <typename T>
T *MakeCheckedTypedRef(CKContext *context,
                      CKObject *object,
                      const char *label,
                      const std::string &error = std::string()) {
    CKContext *resolvedContext = object ? object->GetCKContext() : context;
    if (!object) {
        return new T(resolvedContext, 0, ScriptBridgeObjectStamp(), error.empty() ? "Object is null." : error);
    }
    CKObject *typed = CastTypedTarget<T>(object);
    if (!typed) {
        return new T(resolvedContext, 0, ScriptBridgeObjectStamp(), TypeMismatchError(label, object));
    }
    return new T(resolvedContext, typed);
}

bool NameMatches(CKObject *object, const std::string &name) {
    return name.empty() || NameEquals(object ? object->GetName() : nullptr, name);
}

bool SameObject(CKObject *lhs, CKObject *rhs) {
    return lhs && rhs && lhs->GetID() == rhs->GetID() && lhs->GetCKContext() == rhs->GetCKContext();
}

bool IsSceneMembershipAsset(CKObject *object) {
    return CKMaterial::Cast(object) || CKTexture::Cast(object) || CKMesh::Cast(object);
}

bool IsDirectSceneMember(CKSceneObject *object, CKScene *scene) {
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

class SceneScopeIndex {
public:
    SceneScopeIndex(CKContext *context, CKScene *scene) : m_Context(context), m_Scene(scene) {
        Build();
    }

    bool Contains(CKObject *object) const {
        if (!object || object->IsToBeDeleted() || !m_Context || !m_Scene || object->GetCKContext() != m_Context) {
            return false;
        }
        const CK_ID id = object->GetID();
        if (m_DirectSceneObjects.find(id) != m_DirectSceneObjects.end()) {
            return true;
        }
        if (CKBehavior *behavior = CKBehavior::Cast(object)) {
            if (BehaviorOwnerInScene(behavior)) {
                return true;
            }
        }
        if (!IsSceneMembershipAsset(object)) {
            return false;
        }
        if (m_Scene->IsObjectHere(object) != FALSE) {
            return true;
        }
        return m_Assets.find(id) != m_Assets.end();
    }

private:
    void AddObject(CKObject *object, std::unordered_set<CK_ID> &set) {
        if (object && !object->IsToBeDeleted() && object->GetCKContext() == m_Context) {
            set.insert(object->GetID());
        }
    }

    void AddTexture(CKTexture *texture) {
        AddObject(texture, m_Assets);
    }

    void AddMaterial(CKMaterial *material) {
        if (!material) {
            return;
        }
        AddObject(material, m_Assets);
        for (int i = 0; i < 4; ++i) {
            AddTexture(material->GetTexture(i));
        }
    }

    void AddMesh(CKMesh *mesh) {
        if (!mesh) {
            return;
        }
        AddObject(mesh, m_Assets);
        const int materialCount = mesh->GetMaterialCount();
        for (int i = 0; i < materialCount; ++i) {
            AddMaterial(mesh->GetMaterial(i));
        }
    }

    void AddSceneObjectDependencies(CKSceneObject *sceneObject) {
        if (CKSprite3D *sprite = CKSprite3D::Cast(sceneObject)) {
            AddMaterial(sprite->GetMaterial());
        }
        if (CK2dEntity *entity2D = CK2dEntity::Cast(sceneObject)) {
            AddMaterial(entity2D->GetMaterial());
        }
        if (CK3dEntity *entity3D = CK3dEntity::Cast(sceneObject)) {
            AddMesh(entity3D->GetCurrentMesh());
            const int meshCount = entity3D->GetMeshCount();
            for (int i = 0; i < meshCount; ++i) {
                AddMesh(entity3D->GetMesh(i));
            }
        }
    }

    void AddComputedObjects(CK_CLASSID cid) {
        if (!m_Scene) {
            return;
        }
        const XObjectPointerArray &objects = m_Scene->ComputeObjectList(cid, TRUE);
        const int count = objects.Size();
        for (int i = 0; i < count; ++i) {
            CKObject *object = objects[i];
            if (IsSceneMembershipAsset(object)) {
                AddObject(object, m_Assets);
            }
        }
    }

    bool BehaviorOwnerInScene(CKBehavior *behavior) const {
        for (CKBehavior *current = behavior; current; current = current->GetParent()) {
            CKBeObject *owner = current->GetOwner();
            if (owner && m_DirectSceneObjects.find(owner->GetID()) != m_DirectSceneObjects.end()) {
                return true;
            }
            CKBehavior *ownerScript = current->GetOwnerScript();
            if (ownerScript && ownerScript != current &&
                m_DirectSceneObjects.find(ownerScript->GetID()) != m_DirectSceneObjects.end()) {
                return true;
            }
        }
        return false;
    }

    void Build() {
        if (!m_Context || !m_Scene) {
            return;
        }
        const XObjectPointerArray &sceneObjects = m_Context->GetObjectListByType(CKCID_SCENEOBJECT, TRUE);
        const int sceneObjectCount = sceneObjects.Size();
        for (int i = 0; i < sceneObjectCount; ++i) {
            CKSceneObject *sceneObject = CKSceneObject::Cast(sceneObjects[i]);
            if (sceneObject && IsDirectSceneMember(sceneObject, m_Scene)) {
                AddObject(sceneObject, m_DirectSceneObjects);
                AddSceneObjectDependencies(sceneObject);
            }
        }
        AddComputedObjects(CKCID_MATERIAL);
        AddComputedObjects(CKCID_TEXTURE);
        AddComputedObjects(CKCID_MESH);
    }

    CKContext *m_Context = nullptr;
    CKScene *m_Scene = nullptr;
    std::unordered_set<CK_ID> m_DirectSceneObjects;
    std::unordered_set<CK_ID> m_Assets;
};

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
    if (currentSceneOnly && !scene) {
        return objects;
    }
    SceneScopeIndex scope(context, scene);
    const XObjectPointerArray &raw = context->GetObjectListByType(cid, derived);
    const int size = raw.Size();
    objects.reserve(static_cast<std::size_t>(size));
    for (int i = 0; i < size; ++i) {
        CKObject *object = raw[i];
        if (!object || object->IsToBeDeleted() || !NameMatches(object, name)) {
            continue;
        }
        if (currentSceneOnly && !scope.Contains(object)) {
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

CScriptArray *CreateRefArray(const char *decl) {
    asIScriptContext *active = asGetActiveContext();
    asIScriptEngine *engine = active ? active->GetEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl(decl) : nullptr;
    if (!arrayType) {
        SetScriptException(fmt::format("{} is not registered.", decl));
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

template <typename T>
void AppendTypedRef(CScriptArray *array, T *ref) {
    if (!array || !ref) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &ref);
    ref->Release();
}

std::string LookupScopeLabel(bool currentSceneOnly) {
    return currentSceneOnly ? "current scene" : "all scenes";
}

std::string FindOneError(const char *label,
                         const std::string &name,
                         CK_CLASSID cid,
                         bool currentSceneOnly,
                         std::size_t count) {
    return fmt::format("Scene::FindOne expected exactly one {} match for name='{}' class={} scope={}, match count={}.",
                       label ? label : "object",
                       name.empty() ? "<any>" : name,
                       static_cast<int>(cid),
                       LookupScopeLabel(currentSceneOnly),
                       count);
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
                    bool currentSceneOnly,
                    ScriptBehaviorBridge *bridge = nullptr,
                    CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    if (occurrence < 0) {
        return MakeInvalid(context, "Scene::Find occurrence must be non-negative.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, derived, currentSceneOnly);
    if (occurrence >= static_cast<int>(objects.size())) {
        return MakeInvalid(context, fmt::format("Scene object '{}' was not found.", name.empty() ? "<any>" : name));
    }
    return MakeRef(context, objects[static_cast<std::size_t>(occurrence)], std::string(), bridge, componentId);
}

ObjectRef *FindOneImpl(CKContext *context,
                       const std::string &name,
                       CK_CLASSID cid,
                       bool derived,
                       bool currentSceneOnly,
                       ScriptBehaviorBridge *bridge = nullptr,
                       CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, derived, currentSceneOnly);
    if (objects.size() != 1) {
        return MakeInvalid(context, FindOneError("object", name, cid, currentSceneOnly, objects.size()));
    }
    return MakeRef(context, objects.front(), std::string(), bridge, componentId);
}

template <typename T>
T *FindTypedImpl(CKContext *context,
                 const std::string &name,
                 CK_CLASSID cid,
                 int occurrence,
                 bool currentSceneOnly,
                 const char *label) {
    if (!context) {
        return new T(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    if (occurrence < 0) {
        return new T(context, 0, ScriptBridgeObjectStamp(), "Scene::Find occurrence must be non-negative.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, true, currentSceneOnly);
    if (occurrence >= static_cast<int>(objects.size())) {
        return new T(context,
                     0,
                     ScriptBridgeObjectStamp(),
                     fmt::format("{} '{}' was not found.", label, name.empty() ? "<any>" : name));
    }
    return MakeCheckedTypedRef<T>(context, objects[static_cast<std::size_t>(occurrence)], label);
}

template <typename T>
T *FindOneTypedImpl(CKContext *context,
                    const std::string &name,
                    CK_CLASSID cid,
                    bool currentSceneOnly,
                    const char *label) {
    if (!context) {
        return new T(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    std::vector<CKObject *> objects = CollectObjects(context, name, cid, true, currentSceneOnly);
    if (objects.size() != 1) {
        return new T(context,
                     0,
                     ScriptBridgeObjectStamp(),
                     FindOneError(label, name, cid, currentSceneOnly, objects.size()));
    }
    return MakeCheckedTypedRef<T>(context, objects.front(), label);
}

template <typename T>
CScriptArray *FindAllTypedImpl(CKContext *context,
                               const std::string &name,
                               CK_CLASSID cid,
                               bool currentSceneOnly,
                               const char *arrayDecl) {
    CScriptArray *array = CreateRefArray(arrayDecl);
    if (!array) {
        return nullptr;
    }
    for (CKObject *object : CollectObjects(context, name, cid, true, currentSceneOnly)) {
        T *ref = MakeCheckedTypedRef<T>(context, object, arrayDecl);
        if (ref && ref->valid()) {
            AppendTypedRef(array, ref);
        } else if (ref) {
            ref->Release();
        }
    }
    return array;
}

BehaviorRef *FindBehaviorImpl(CKContext *context,
                              const std::string &name,
                              int occurrence,
                              bool currentSceneOnly,
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
    std::vector<CKObject *> objects = CollectObjects(context, name, CKCID_BEHAVIOR, true, currentSceneOnly);
    if (occurrence >= static_cast<int>(objects.size())) {
        return invalid(fmt::format("CKBehavior '{}' was not found.", name.empty() ? "<any>" : name));
    }
    CKBehavior *behavior = CKBehavior::Cast(objects[static_cast<std::size_t>(occurrence)]);
    if (!behavior) {
        return invalid("Matched object is not a CKBehavior.");
    }
    return new BehaviorRef(bridge, behavior->GetID(), componentId, behavior->GetCKContext());
}

BehaviorRef *FindOneBehaviorImpl(CKContext *context,
                                 const std::string &name,
                                 bool currentSceneOnly,
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
    std::vector<CKObject *> objects = CollectObjects(context, name, CKCID_BEHAVIOR, true, currentSceneOnly);
    if (objects.size() != 1) {
        return invalid(FindOneError("CKBehavior", name, CKCID_BEHAVIOR, currentSceneOnly, objects.size()));
    }
    CKBehavior *behavior = CKBehavior::Cast(objects.front());
    if (!behavior) {
        return invalid("Matched object is not a CKBehavior.");
    }
    return new BehaviorRef(bridge, behavior->GetID(), componentId, behavior->GetCKContext());
}

CScriptArray *FindAllBehaviorImpl(CKContext *context,
                                  const std::string &name,
                                  bool currentSceneOnly,
                                  ScriptBehaviorBridge *bridge,
                                  CK_ID componentId) {
    CScriptArray *array = CreateRefArray("array<BehaviorRef@>");
    if (!array) {
        return nullptr;
    }
    for (CKObject *object : CollectObjects(context, name, CKCID_BEHAVIOR, true, currentSceneOnly)) {
        if (CKBehavior *behavior = CKBehavior::Cast(object)) {
            AppendTypedRef(array, new BehaviorRef(bridge, behavior->GetID(), componentId, behavior->GetCKContext()));
        }
    }
    return array;
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

bool RejectSceneMutation(const char *apiName) {
    return ScriptManager::RejectActiveHostCall(apiName, CKAS_HOSTCALL_MUTATES_HOST_STATE);
}

ObjectRef *CreateImpl(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic, ScriptBehaviorBridge *bridge = nullptr, CK_ID componentId = 0) {
    if (!context) {
        return MakeInvalid(nullptr, "CKContext is not available.");
    }
    if (RejectSceneMutation("Scene::Create")) {
        return MakeInvalid(context, "Scene::Create is not available in the current script host phase.");
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
T *CreateTypedImpl(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic, const char *label) {
    if (!context) {
        return new T(nullptr, 0, ScriptBridgeObjectStamp(), "CKContext is not available.");
    }
    if (RejectSceneMutation("Scene::Create")) {
        return new T(context, 0, ScriptBridgeObjectStamp(), "Scene::Create is not available in the current script host phase.");
    }
    CKObject *object = context->CreateObject(cid,
                                            name.empty() ? nullptr : const_cast<CKSTRING>(name.c_str()),
                                            dynamic ? CK_OBJECTCREATION_DYNAMIC : CK_OBJECTCREATION_NONAMECHECK);
    if (!object) {
        return new T(context, 0, ScriptBridgeObjectStamp(), fmt::format("Scene::Create failed for class {}.", static_cast<int>(cid)));
    }
    return MakeCheckedTypedRef<T>(context, object, label);
}

bool AddToCurrentSceneImpl(CKContext *context, ObjectRef *ref, bool dependencies) {
    if (!context || !ref) {
        return false;
    }
    if (RejectSceneMutation("Scene::AddToCurrentScene")) {
        ref->SetError("Scene::AddToCurrentScene is not available in the current script host phase.");
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
    if (IsSceneMembershipAsset(object)) {
        ref->SetError("Scene::AddToCurrentScene requires a CKSceneObject.");
        return false;
    }
    scene->AddObjectToScene(object, dependencies);
    return true;
}

bool IsInSceneImpl(CKContext *context, SceneRef *sceneRef, ObjectRef *ref) {
    if (!context || !sceneRef || !ref) {
        return false;
    }
    CKScene *scene = sceneRef->Scene();
    if (!scene) {
        ref->SetError("Scene::IsInScene requires a valid SceneRef.");
        return false;
    }
    CKObject *object = ref->Object();
    if (!object) {
        ref->SetError("Scene::IsInScene requires a valid object.");
        return false;
    }
    if (scene->GetCKContext() != object->GetCKContext() || object->GetCKContext() != context) {
        ref->SetError("Scene::IsInScene requires scene and object from the same CKContext.");
        return false;
    }
    return ScriptSceneIsObjectInScene(object, scene);
}

bool IsInCurrentSceneImpl(CKContext *context, ObjectRef *ref) {
    if (!context || !ref) {
        return false;
    }
    CKScene *scene = context->GetCurrentScene();
    if (!scene) {
        ref->SetError("Current scene is not available.");
        return false;
    }
    CKObject *object = ref->Object();
    if (!object) {
        ref->SetError("Scene::IsInCurrentScene requires a valid object.");
        return false;
    }
    if (object->GetCKContext() != context) {
        ref->SetError("Scene::IsInCurrentScene requires an object from the current CKContext.");
        return false;
    }
    return ScriptSceneIsObjectInScene(object, scene);
}

bool AddToSceneImpl(CKContext *context, SceneRef *sceneRef, ObjectRef *ref, bool dependencies) {
    if (!context || !sceneRef || !ref) {
        return false;
    }
    if (RejectSceneMutation("Scene::AddToScene")) {
        ref->SetError("Scene::AddToScene is not available in the current script host phase.");
        return false;
    }
    CKScene *scene = sceneRef->Scene();
    if (!scene) {
        ref->SetError("Scene::AddToScene requires a valid SceneRef.");
        return false;
    }
    CKSceneObject *object = CKSceneObject::Cast(ref->Object());
    if (!object) {
        ref->SetError("Scene::AddToScene requires a CKSceneObject.");
        return false;
    }
    if (IsSceneMembershipAsset(object)) {
        ref->SetError("Scene::AddToScene requires a CKSceneObject.");
        return false;
    }
    if (scene->GetCKContext() != object->GetCKContext() || object->GetCKContext() != context) {
        ref->SetError("Scene::AddToScene requires scene and object from the same CKContext.");
        return false;
    }
    scene->AddObjectToScene(object, dependencies);
    return true;
}

bool RemoveFromCurrentSceneImpl(CKContext *context, ObjectRef *ref, bool dependencies) {
    if (!context || !ref) {
        return false;
    }
    if (RejectSceneMutation("Scene::RemoveFromCurrentScene")) {
        ref->SetError("Scene::RemoveFromCurrentScene is not available in the current script host phase.");
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
    if (IsSceneMembershipAsset(object)) {
        ref->SetError("Scene::RemoveFromCurrentScene requires a CKSceneObject.");
        return false;
    }
    scene->RemoveObjectFromScene(object, dependencies);
    return true;
}

bool RemoveFromSceneImpl(CKContext *context, SceneRef *sceneRef, ObjectRef *ref, bool dependencies) {
    if (!context || !sceneRef || !ref) {
        return false;
    }
    if (RejectSceneMutation("Scene::RemoveFromScene")) {
        ref->SetError("Scene::RemoveFromScene is not available in the current script host phase.");
        return false;
    }
    CKScene *scene = sceneRef->Scene();
    if (!scene) {
        ref->SetError("Scene::RemoveFromScene requires a valid SceneRef.");
        return false;
    }
    CKSceneObject *object = CKSceneObject::Cast(ref->Object());
    if (!object) {
        ref->SetError("Scene::RemoveFromScene requires a CKSceneObject.");
        return false;
    }
    if (IsSceneMembershipAsset(object)) {
        ref->SetError("Scene::RemoveFromScene requires a CKSceneObject.");
        return false;
    }
    if (scene->GetCKContext() != object->GetCKContext() || object->GetCKContext() != context) {
        ref->SetError("Scene::RemoveFromScene requires scene and object from the same CKContext.");
        return false;
    }
    scene->RemoveObjectFromScene(object, dependencies);
    return true;
}

bool DestroyImpl(CKContext *context, ObjectRef *ref, bool allowPersistent, CKDWORD flags) {
    if (!context || !ref) {
        return false;
    }
    if (RejectSceneMutation("Scene::Destroy")) {
        ref->SetError("Scene::Destroy is not available in the current script host phase.");
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
    if (RejectSceneMutation("Scene::Select")) {
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
ObjectRef *FindCtx(CKContext *context, const std::string &name, CK_CLASSID cid, bool derived, int occurrence, bool currentSceneOnly) { return FindImpl(context, name, cid, derived, occurrence, currentSceneOnly); }
ObjectRef *FindOneCtx(CKContext *context, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindOneImpl(context, name, cid, derived, currentSceneOnly); }
CScriptArray *FindAllCtx(CKContext *context, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(context, name, cid, derived, currentSceneOnly); }
Entity3DRef *FindEntity3DCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTypedImpl<Entity3DRef>(context, name, CKCID_3DENTITY, occurrence, currentSceneOnly, "CK3dEntity"); }
Entity2DRef *FindEntity2DCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTypedImpl<Entity2DRef>(context, name, CKCID_2DENTITY, occurrence, currentSceneOnly, "CK2dEntity"); }
MaterialRef *FindMaterialCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTypedImpl<MaterialRef>(context, name, CKCID_MATERIAL, occurrence, currentSceneOnly, "CKMaterial"); }
TextureRef *FindTextureCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTypedImpl<TextureRef>(context, name, CKCID_TEXTURE, occurrence, currentSceneOnly, "CKTexture"); }
MeshRef *FindMeshCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTypedImpl<MeshRef>(context, name, CKCID_MESH, occurrence, currentSceneOnly, "CKMesh"); }
BehaviorRef *FindBehaviorCtx(CKContext *context, const std::string &name, int occurrence, bool currentSceneOnly) { return FindBehaviorImpl(context, name, occurrence, currentSceneOnly, BridgeFrom(context), 0); }
Entity3DRef *FindOneEntity3DCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneTypedImpl<Entity3DRef>(context, name, CKCID_3DENTITY, currentSceneOnly, "CK3dEntity"); }
Entity2DRef *FindOneEntity2DCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneTypedImpl<Entity2DRef>(context, name, CKCID_2DENTITY, currentSceneOnly, "CK2dEntity"); }
MaterialRef *FindOneMaterialCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneTypedImpl<MaterialRef>(context, name, CKCID_MATERIAL, currentSceneOnly, "CKMaterial"); }
TextureRef *FindOneTextureCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneTypedImpl<TextureRef>(context, name, CKCID_TEXTURE, currentSceneOnly, "CKTexture"); }
MeshRef *FindOneMeshCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneTypedImpl<MeshRef>(context, name, CKCID_MESH, currentSceneOnly, "CKMesh"); }
BehaviorRef *FindOneBehaviorCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindOneBehaviorImpl(context, name, currentSceneOnly, BridgeFrom(context), 0); }
CScriptArray *FindAllEntity3DCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllTypedImpl<Entity3DRef>(context, name, CKCID_3DENTITY, currentSceneOnly, "array<Entity3DRef@>"); }
CScriptArray *FindAllEntity2DCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllTypedImpl<Entity2DRef>(context, name, CKCID_2DENTITY, currentSceneOnly, "array<Entity2DRef@>"); }
CScriptArray *FindAllMaterialCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllTypedImpl<MaterialRef>(context, name, CKCID_MATERIAL, currentSceneOnly, "array<MaterialRef@>"); }
CScriptArray *FindAllTextureCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllTypedImpl<TextureRef>(context, name, CKCID_TEXTURE, currentSceneOnly, "array<TextureRef@>"); }
CScriptArray *FindAllMeshCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllTypedImpl<MeshRef>(context, name, CKCID_MESH, currentSceneOnly, "array<MeshRef@>"); }
CScriptArray *FindAllBehaviorCtx(CKContext *context, const std::string &name, bool currentSceneOnly) { return FindAllBehaviorImpl(context, name, currentSceneOnly, BridgeFrom(context), 0); }
ObjectRef *CreateCtx(CKContext *context, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(context, cid, name, dynamic); }
Entity3DRef *CreateEntity3DCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<Entity3DRef>(context, CKCID_3DENTITY, name, dynamic, "CK3dEntity"); }
Entity2DRef *CreateEntity2DCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<Entity2DRef>(context, CKCID_2DENTITY, name, dynamic, "CK2dEntity"); }
MaterialRef *CreateMaterialCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<MaterialRef>(context, CKCID_MATERIAL, name, dynamic, "CKMaterial"); }
TextureRef *CreateTextureCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<TextureRef>(context, CKCID_TEXTURE, name, dynamic, "CKTexture"); }
MeshRef *CreateMeshCtx(CKContext *context, const std::string &name, bool dynamic) { return CreateTypedImpl<MeshRef>(context, CKCID_MESH, name, dynamic, "CKMesh"); }
void ReleaseScriptArrayParam(CScriptArray *array) {
    if (array) {
        array->Release();
    }
}

bool AddToCurrentSceneCtx(CKContext *context, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(context, ref, dependencies); }
bool RemoveFromCurrentSceneCtx(CKContext *context, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(context, ref, dependencies); }
bool IsInCurrentSceneCtx(CKContext *context, ObjectRef *ref) { return IsInCurrentSceneImpl(context, ref); }
bool IsInSceneCtx(CKContext *context, SceneRef *scene, ObjectRef *ref) { return IsInSceneImpl(context, scene, ref); }
bool AddToSceneCtx(CKContext *context, SceneRef *scene, ObjectRef *ref, bool dependencies) { return AddToSceneImpl(context, scene, ref, dependencies); }
bool RemoveFromSceneCtx(CKContext *context, SceneRef *scene, ObjectRef *ref, bool dependencies) { return RemoveFromSceneImpl(context, scene, ref, dependencies); }
bool DestroyCtx(CKContext *context, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(context, ref, allowPersistent, flags); }
bool SelectCtx(CKContext *context, CScriptArray *objects, bool clearSelection) {
    const bool result = SelectImpl(context, objects, clearSelection);
    ReleaseScriptArrayParam(objects);
    return result;
}

// CKBehaviorContext overloads.
LevelRef *CurrentLevelBehavior(const CKBehaviorContext &ctx) { return ctx.CurrentLevel ? MakeTypedRef<LevelRef>(ContextFrom(ctx), ctx.CurrentLevel) : CurrentLevelImpl(ContextFrom(ctx)); }
SceneRef *CurrentSceneBehavior(const CKBehaviorContext &ctx) { return ctx.CurrentScene ? MakeTypedRef<SceneRef>(ContextFrom(ctx), ctx.CurrentScene) : CurrentSceneImpl(ContextFrom(ctx)); }
ObjectRef *TargetBehavior(const CKBehaviorContext &ctx) { return TargetImpl(ctx, BridgeFrom(ctx)); }
ObjectRef *OwnerBehavior(const CKBehaviorContext &ctx) { return OwnerImpl(ctx, BridgeFrom(ctx)); }
ObjectRef *RefBehavior(const CKBehaviorContext &ctx, CKObject *object) { return MakeRef(ContextFrom(ctx), object, std::string(), BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *ByIdBehavior(const CKBehaviorContext &ctx, CK_ID id) { return ByIdImpl(ContextFrom(ctx), id, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *FindBehaviorContext(const CKBehaviorContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, int occurrence, bool currentSceneOnly) { return FindImpl(ContextFrom(ctx), name, cid, derived, occurrence, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *FindOneBehaviorContext(const CKBehaviorContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindOneImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
CScriptArray *FindAllBehavior(const CKBehaviorContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
Entity3DRef *FindEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindEntity3DCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
Entity2DRef *FindEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindEntity2DCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
MaterialRef *FindMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindMaterialCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
TextureRef *FindTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTextureCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
MeshRef *FindMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindMeshCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
BehaviorRef *FindBehaviorBehavior(const CKBehaviorContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindBehaviorImpl(ContextFrom(ctx), name, occurrence, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
Entity3DRef *FindOneEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneEntity3DCtx(ContextFrom(ctx), name, currentSceneOnly); }
Entity2DRef *FindOneEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneEntity2DCtx(ContextFrom(ctx), name, currentSceneOnly); }
MaterialRef *FindOneMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneMaterialCtx(ContextFrom(ctx), name, currentSceneOnly); }
TextureRef *FindOneTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneTextureCtx(ContextFrom(ctx), name, currentSceneOnly); }
MeshRef *FindOneMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneMeshCtx(ContextFrom(ctx), name, currentSceneOnly); }
BehaviorRef *FindOneBehaviorBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneBehaviorImpl(ContextFrom(ctx), name, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
CScriptArray *FindAllEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllEntity3DCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllEntity2DCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllMaterialCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllTextureCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllMeshCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllBehaviorBehavior(const CKBehaviorContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllBehaviorImpl(ContextFrom(ctx), name, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
ObjectRef *CreateBehavior(const CKBehaviorContext &ctx, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(ContextFrom(ctx), cid, name, dynamic, BridgeFrom(ctx), ComponentIdFromContext(ctx)); }
Entity3DRef *CreateEntity3DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateEntity3DCtx(ContextFrom(ctx), name, dynamic); }
Entity2DRef *CreateEntity2DBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateEntity2DCtx(ContextFrom(ctx), name, dynamic); }
MaterialRef *CreateMaterialBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateMaterialCtx(ContextFrom(ctx), name, dynamic); }
TextureRef *CreateTextureBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateTextureCtx(ContextFrom(ctx), name, dynamic); }
MeshRef *CreateMeshBehavior(const CKBehaviorContext &ctx, const std::string &name, bool dynamic) { return CreateMeshCtx(ContextFrom(ctx), name, dynamic); }
bool AddToCurrentSceneBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool RemoveFromCurrentSceneBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool IsInCurrentSceneBehavior(const CKBehaviorContext &ctx, ObjectRef *ref) { return IsInCurrentSceneImpl(ContextFrom(ctx), ref); }
bool IsInSceneBehavior(const CKBehaviorContext &ctx, SceneRef *scene, ObjectRef *ref) { return IsInSceneImpl(ContextFrom(ctx), scene, ref); }
bool AddToSceneBehavior(const CKBehaviorContext &ctx, SceneRef *scene, ObjectRef *ref, bool dependencies) { return AddToSceneImpl(ContextFrom(ctx), scene, ref, dependencies); }
bool RemoveFromSceneBehavior(const CKBehaviorContext &ctx, SceneRef *scene, ObjectRef *ref, bool dependencies) { return RemoveFromSceneImpl(ContextFrom(ctx), scene, ref, dependencies); }
bool DestroyBehavior(const CKBehaviorContext &ctx, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(ContextFrom(ctx), ref, allowPersistent, flags); }
bool SelectBehavior(const CKBehaviorContext &ctx, CScriptArray *objects, bool clearSelection) {
    const bool result = SelectImpl(ContextFrom(ctx), objects, clearSelection);
    ReleaseScriptArrayParam(objects);
    return result;
}

// ScriptContext overloads.
LevelRef *CurrentLevelScript(const ScriptContext &ctx) { return CurrentLevelImpl(ContextFrom(ctx)); }
SceneRef *CurrentSceneScript(const ScriptContext &ctx) { return CurrentSceneImpl(ContextFrom(ctx)); }
ObjectRef *TargetScript(const ScriptContext &ctx) { return TargetImpl(ctx.ToBehaviorContext(), BridgeFrom(ctx)); }
ObjectRef *OwnerScript(const ScriptContext &ctx) { return OwnerImpl(ctx.ToBehaviorContext(), BridgeFrom(ctx)); }
ObjectRef *RefScript(const ScriptContext &ctx, CKObject *object) { return MakeRef(ContextFrom(ctx), object, std::string(), BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *ByIdScript(const ScriptContext &ctx, CK_ID id) { return ByIdImpl(ContextFrom(ctx), id, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *FindScript(const ScriptContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, int occurrence, bool currentSceneOnly) { return FindImpl(ContextFrom(ctx), name, cid, derived, occurrence, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *FindOneScript(const ScriptContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindOneImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
CScriptArray *FindAllScript(const ScriptContext &ctx, const std::string &name, CK_CLASSID cid, bool derived, bool currentSceneOnly) { return FindAllImpl(ContextFrom(ctx), name, cid, derived, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
Entity3DRef *FindEntity3DScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindEntity3DCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
Entity2DRef *FindEntity2DScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindEntity2DCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
MaterialRef *FindMaterialScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindMaterialCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
TextureRef *FindTextureScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindTextureCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
MeshRef *FindMeshScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindMeshCtx(ContextFrom(ctx), name, occurrence, currentSceneOnly); }
BehaviorRef *FindBehaviorScript(const ScriptContext &ctx, const std::string &name, int occurrence, bool currentSceneOnly) { return FindBehaviorImpl(ContextFrom(ctx), name, occurrence, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
Entity3DRef *FindOneEntity3DScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneEntity3DCtx(ContextFrom(ctx), name, currentSceneOnly); }
Entity2DRef *FindOneEntity2DScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneEntity2DCtx(ContextFrom(ctx), name, currentSceneOnly); }
MaterialRef *FindOneMaterialScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneMaterialCtx(ContextFrom(ctx), name, currentSceneOnly); }
TextureRef *FindOneTextureScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneTextureCtx(ContextFrom(ctx), name, currentSceneOnly); }
MeshRef *FindOneMeshScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneMeshCtx(ContextFrom(ctx), name, currentSceneOnly); }
BehaviorRef *FindOneBehaviorScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindOneBehaviorImpl(ContextFrom(ctx), name, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
CScriptArray *FindAllEntity3DScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllEntity3DCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllEntity2DScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllEntity2DCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllMaterialScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllMaterialCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllTextureScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllTextureCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllMeshScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllMeshCtx(ContextFrom(ctx), name, currentSceneOnly); }
CScriptArray *FindAllBehaviorScript(const ScriptContext &ctx, const std::string &name, bool currentSceneOnly) { return FindAllBehaviorImpl(ContextFrom(ctx), name, currentSceneOnly, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
ObjectRef *CreateScript(const ScriptContext &ctx, CK_CLASSID cid, const std::string &name, bool dynamic) { return CreateImpl(ContextFrom(ctx), cid, name, dynamic, BridgeFrom(ctx), ComponentIdFromContext(ctx.ToBehaviorContext())); }
Entity3DRef *CreateEntity3DScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateEntity3DCtx(ContextFrom(ctx), name, dynamic); }
Entity2DRef *CreateEntity2DScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateEntity2DCtx(ContextFrom(ctx), name, dynamic); }
MaterialRef *CreateMaterialScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateMaterialCtx(ContextFrom(ctx), name, dynamic); }
TextureRef *CreateTextureScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateTextureCtx(ContextFrom(ctx), name, dynamic); }
MeshRef *CreateMeshScript(const ScriptContext &ctx, const std::string &name, bool dynamic) { return CreateMeshCtx(ContextFrom(ctx), name, dynamic); }
bool AddToCurrentSceneScript(const ScriptContext &ctx, ObjectRef *ref, bool dependencies) { return AddToCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool RemoveFromCurrentSceneScript(const ScriptContext &ctx, ObjectRef *ref, bool dependencies) { return RemoveFromCurrentSceneImpl(ContextFrom(ctx), ref, dependencies); }
bool IsInCurrentSceneScript(const ScriptContext &ctx, ObjectRef *ref) { return IsInCurrentSceneImpl(ContextFrom(ctx), ref); }
bool IsInSceneScript(const ScriptContext &ctx, SceneRef *scene, ObjectRef *ref) { return IsInSceneImpl(ContextFrom(ctx), scene, ref); }
bool AddToSceneScript(const ScriptContext &ctx, SceneRef *scene, ObjectRef *ref, bool dependencies) { return AddToSceneImpl(ContextFrom(ctx), scene, ref, dependencies); }
bool RemoveFromSceneScript(const ScriptContext &ctx, SceneRef *scene, ObjectRef *ref, bool dependencies) { return RemoveFromSceneImpl(ContextFrom(ctx), scene, ref, dependencies); }
bool DestroyScript(const ScriptContext &ctx, ObjectRef *ref, bool allowPersistent, CKDWORD flags) { return DestroyImpl(ContextFrom(ctx), ref, allowPersistent, flags); }
bool SelectScript(const ScriptContext &ctx, CScriptArray *objects, bool clearSelection) {
    const bool result = SelectImpl(ContextFrom(ctx), objects, clearSelection);
    ReleaseScriptArrayParam(objects);
    return result;
}

} // namespace

bool ScriptSceneIsObjectInScene(CKObject *object, CKScene *scene) {
    if (!object || !scene || object->GetCKContext() != scene->GetCKContext()) {
        return false;
    }
    SceneScopeIndex scope(object->GetCKContext(), scene);
    return scope.Contains(object);
}

void RegisterScriptSceneCore(asIScriptEngine *engine) {
    assert(engine != nullptr);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    int r = engine->SetDefaultNamespace("Scene"); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(CKContext@ ctx)", asFUNCTION(CurrentLevelCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(CKContext@ ctx)", asFUNCTION(CurrentSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(CKContext@ ctx)", asFUNCTION(TargetCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(CKContext@ ctx)", asFUNCTION(OwnerCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(CKContext@ ctx, CKObject@ obj)", asFUNCTION(RefCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(CKContext@ ctx, CK_ID id)", asFUNCTION(ByIdCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(CKContext@ ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ FindOne(CKContext@ ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindOneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(CKContext@ ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity3DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity2DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMaterialCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindTextureCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMeshCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(CKContext@ ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindBehaviorCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindOneEntity3D(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity3DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindOneEntity2D(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity2DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindOneMaterial(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMaterialCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindOneTexture(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneTextureCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindOneMesh(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMeshCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindOneBehavior(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneBehaviorCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity3DRef@>@ FindAllEntity3D(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity3DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity2DRef@>@ FindAllEntity2D(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity2DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MaterialRef@>@ FindAllMaterial(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMaterialCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<TextureRef@>@ FindAllTexture(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllTextureCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MeshRef@>@ FindAllMesh(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMeshCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<BehaviorRef@>@ FindAllBehavior(CKContext@ ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllBehaviorCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(CKContext@ ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(CKContext@ ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(CKContext@ ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(CKContext@ ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInCurrentScene(CKContext@ ctx, ObjectRef@ obj)", asFUNCTION(IsInCurrentSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInScene(CKContext@ ctx, SceneRef@ scene, ObjectRef@ obj)", asFUNCTION(IsInSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToScene(CKContext@ ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromScene(CKContext@ ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromSceneCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Destroy(CKContext@ ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Select(CKContext@ ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectCtx), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(const CKBehaviorContext &in ctx)", asFUNCTION(CurrentLevelBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(const CKBehaviorContext &in ctx)", asFUNCTION(CurrentSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(const CKBehaviorContext &in ctx)", asFUNCTION(TargetBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(const CKBehaviorContext &in ctx)", asFUNCTION(OwnerBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(const CKBehaviorContext &in ctx, CKObject@ obj)", asFUNCTION(RefBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(const CKBehaviorContext &in ctx, CK_ID id)", asFUNCTION(ByIdBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(const CKBehaviorContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindBehaviorContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ FindOne(const CKBehaviorContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindOneBehaviorContext), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(const CKBehaviorContext &in ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity3DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity2DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMaterialBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindTextureBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMeshBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(const CKBehaviorContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindBehaviorBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindOneEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity3DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindOneEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity2DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindOneMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMaterialBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindOneTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneTextureBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindOneMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMeshBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindOneBehavior(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneBehaviorBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity3DRef@>@ FindAllEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity3DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity2DRef@>@ FindAllEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity2DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MaterialRef@>@ FindAllMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMaterialBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<TextureRef@>@ FindAllTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllTextureBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MeshRef@>@ FindAllMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMeshBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<BehaviorRef@>@ FindAllBehavior(const CKBehaviorContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllBehaviorBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(const CKBehaviorContext &in ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(const CKBehaviorContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInCurrentScene(const CKBehaviorContext &in ctx, ObjectRef@ obj)", asFUNCTION(IsInCurrentSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInScene(const CKBehaviorContext &in ctx, SceneRef@ scene, ObjectRef@ obj)", asFUNCTION(IsInSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToScene(const CKBehaviorContext &in ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromScene(const CKBehaviorContext &in ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromSceneBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Destroy(const CKBehaviorContext &in ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Select(const CKBehaviorContext &in ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectBehavior), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace(previous.c_str()); CKAS_CHECK_REGISTER(r);
}

void RegisterScriptSceneRuntime(asIScriptEngine *engine) {
    assert(engine != nullptr);
    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    int r = engine->SetDefaultNamespace("Scene"); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterGlobalFunction("LevelRef@ CurrentLevel(const ScriptContext &in ctx)", asFUNCTION(CurrentLevelScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("SceneRef@ CurrentScene(const ScriptContext &in ctx)", asFUNCTION(CurrentSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Target(const ScriptContext &in ctx)", asFUNCTION(TargetScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Owner(const ScriptContext &in ctx)", asFUNCTION(OwnerScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Ref(const ScriptContext &in ctx, CKObject@ obj)", asFUNCTION(RefScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ ById(const ScriptContext &in ctx, CK_ID id)", asFUNCTION(ByIdScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Find(const ScriptContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ FindOne(const ScriptContext &in ctx, const string &in name, CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindOneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<ObjectRef@>@ FindAll(const ScriptContext &in ctx, const string &in name = \"\", CK_CLASSID cid = CKCID_OBJECT, bool derived = true, bool currentSceneOnly = false)", asFUNCTION(FindAllScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindEntity3D(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity3DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindEntity2D(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindEntity2DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindMaterial(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMaterialScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindTexture(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindTextureScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindMesh(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindMeshScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindBehavior(const ScriptContext &in ctx, const string &in name = \"\", int occurrence = 0, bool currentSceneOnly = false)", asFUNCTION(FindBehaviorScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ FindOneEntity3D(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity3DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ FindOneEntity2D(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneEntity2DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ FindOneMaterial(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMaterialScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ FindOneTexture(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneTextureScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ FindOneMesh(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneMeshScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindOneBehavior(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindOneBehaviorScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity3DRef@>@ FindAllEntity3D(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity3DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<Entity2DRef@>@ FindAllEntity2D(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllEntity2DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MaterialRef@>@ FindAllMaterial(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMaterialScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<TextureRef@>@ FindAllTexture(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllTextureScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<MeshRef@>@ FindAllMesh(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllMeshScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<BehaviorRef@>@ FindAllBehavior(const ScriptContext &in ctx, const string &in name = \"\", bool currentSceneOnly = false)", asFUNCTION(FindAllBehaviorScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ObjectRef@ Create(const ScriptContext &in ctx, CK_CLASSID cid, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity3DRef@ CreateEntity3D(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity3DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("Entity2DRef@ CreateEntity2D(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateEntity2DScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MaterialRef@ CreateMaterial(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMaterialScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("TextureRef@ CreateTexture(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateTextureScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("MeshRef@ CreateMesh(const ScriptContext &in ctx, const string &in name = \"\", bool dynamic = true)", asFUNCTION(CreateMeshScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToCurrentScene(const ScriptContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToCurrentSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromCurrentScene(const ScriptContext &in ctx, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromCurrentSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInCurrentScene(const ScriptContext &in ctx, ObjectRef@ obj)", asFUNCTION(IsInCurrentSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool IsInScene(const ScriptContext &in ctx, SceneRef@ scene, ObjectRef@ obj)", asFUNCTION(IsInSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool AddToScene(const ScriptContext &in ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(AddToSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool RemoveFromScene(const ScriptContext &in ctx, SceneRef@ scene, ObjectRef@ obj, bool dependencies = true)", asFUNCTION(RemoveFromSceneScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Destroy(const ScriptContext &in ctx, ObjectRef@ obj, bool allowPersistent = false, CKDWORD flags = 0)", asFUNCTION(DestroyScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Select(const ScriptContext &in ctx, array<ObjectRef@>@ objects, bool clearSelection = true)", asFUNCTION(SelectScript), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace(previous.c_str()); CKAS_CHECK_REGISTER(r);
}
