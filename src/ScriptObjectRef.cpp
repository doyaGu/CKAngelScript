#include "ScriptObjectRef.h"

#include <cassert>

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"

bool ScriptSceneIsObjectInScene(CKObject *object, CKScene *scene);

namespace {

std::uintptr_t ObjectAddress(CKObject *object) {
    return reinterpret_cast<std::uintptr_t>(object);
}

CKObject *ObjectById(CKContext *context, CK_ID id) {
    return id ? GetCKObjectById(context, id) : nullptr;
}

struct ObjectRefIdentityOptions {
    bool isValid = true;
    bool validProperty = true;
    bool describe = true;
    bool idProperty = true;
    bool nameProperty = true;
};

template <typename T>
void RegisterRefCounted(asIScriptEngine *engine, const char *typeName) {
    int r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);
    assert(r >= 0);
}

template <typename T>
void RegisterObjectRefIdentityMethods(asIScriptEngine *engine,
                                      const char *typeName,
                                      const ObjectRefIdentityOptions &options = {}) {
    int r = 0;
    if (options.isValid) {
        r = engine->RegisterObjectMethod(typeName, "bool IsValid() const", asMETHOD(T, IsValid), asCALL_THISCALL); assert(r >= 0);
    }
    if (options.validProperty) {
        r = engine->RegisterObjectMethod(typeName, "bool get_valid() const", asMETHOD(T, valid), asCALL_THISCALL); assert(r >= 0);
    }
    r = engine->RegisterObjectMethod(typeName, "string Error() const", asMETHOD(T, Error), asCALL_THISCALL); assert(r >= 0);
    if (options.describe) {
        r = engine->RegisterObjectMethod(typeName, "string Describe() const", asMETHOD(T, Describe), asCALL_THISCALL); assert(r >= 0);
    }
    r = engine->RegisterObjectMethod(typeName, "CK_ID Id() const", asMETHOD(T, Id), asCALL_THISCALL); assert(r >= 0);
    if (options.idProperty) {
        r = engine->RegisterObjectMethod(typeName, "CK_ID get_id() const", asMETHOD(T, Id), asCALL_THISCALL); assert(r >= 0);
    }
    r = engine->RegisterObjectMethod(typeName, "string Name() const", asMETHOD(T, Name), asCALL_THISCALL); assert(r >= 0);
    if (options.nameProperty) {
        r = engine->RegisterObjectMethod(typeName, "string get_name() const", asMETHOD(T, Name), asCALL_THISCALL); assert(r >= 0);
    }
    r = engine->RegisterObjectMethod(typeName, "CK_CLASSID ClassId() const", asMETHOD(T, ClassId), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool IsDynamic() const", asMETHOD(T, IsDynamic), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "CKObject@ Object() const", asMETHOD(T, Object), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool SetName(const string &in name, bool shared = false)", asMETHOD(T, SetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool SetDynamic(bool dynamic = true)", asMETHOD(T, SetDynamic), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool Show(CK_OBJECT_SHOWOPTION show = CKSHOW)", asMETHOD(T, Show), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool IsVisible() const", asMETHOD(T, IsVisible), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "CKDWORD ObjectFlags() const", asMETHOD(T, ObjectFlags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(typeName, "bool ModifyObjectFlags(CKDWORD add, CKDWORD remove = 0)", asMETHOD(T, ModifyObjectFlags), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
void RegisterObjectRefCommon(asIScriptEngine *engine,
                             const char *typeName,
                             const ObjectRefIdentityOptions &options = {}) {
    RegisterRefCounted<T>(engine, typeName);
    RegisterObjectRefIdentityMethods<T>(engine, typeName, options);
}

template <typename Derived, typename Base>
Derived *CheckedObjectRefDowncast(Base *base) {
    return dynamic_cast<Derived *>(base);
}

template <typename Derived, typename Base>
Base *ObjectRefUpcast(Derived *derived) {
    return static_cast<Base *>(derived);
}

template <typename Derived, typename Base>
void RegisterObjectRefCast(asIScriptEngine *engine, const char *derived, const char *base) {
    int r = 0;
    std::string decl = derived;
    decl.append("@ opCast()");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((CheckedObjectRefDowncast<Derived, Base>), (Base *), Derived *), asCALL_CDECL_OBJLAST); assert(r >= 0);

    decl = base;
    decl.append("@ opImplCast()");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((ObjectRefUpcast<Derived, Base>), (Derived *), Base *), asCALL_CDECL_OBJLAST); assert(r >= 0);

    decl = "const ";
    decl.append(derived).append("@ opCast() const");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((CheckedObjectRefDowncast<Derived, Base>), (Base *), Derived *), asCALL_CDECL_OBJLAST); assert(r >= 0);

    decl = "const ";
    decl.append(base).append("@ opImplCast() const");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((ObjectRefUpcast<Derived, Base>), (Derived *), Base *), asCALL_CDECL_OBJLAST); assert(r >= 0);
}

template <typename T>
void RegisterObjectRefType(asIScriptEngine *engine,
                           const char *typeName,
                           const ObjectRefIdentityOptions &options = {}) {
    int r = engine->RegisterObjectType(typeName, 0, asOBJ_REF);
    assert(r >= 0);
    RegisterObjectRefCommon<T>(engine, typeName, options);
}

template <typename T, typename R>
void RegisterObjectRefAccessor(asIScriptEngine *engine,
                               const char *typeName,
                               const char *decl,
                               R *(T::*method)() const) {
    int r = engine->RegisterObjectMethod(typeName,
                                         decl,
                                         asSMethodPtr<sizeof(void (T::*)())>::Convert((void (T::*)())method),
                                         asCALL_THISCALL);
    assert(r >= 0);
}

ScriptBehaviorBridge *BridgeFromContext(CKContext *context) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

ObjectRefIdentityOptions WithoutLegacyIdentityMethods() {
    ObjectRefIdentityOptions options;
    options.isValid = false;
    options.validProperty = false;
    options.idProperty = false;
    options.nameProperty = false;
    options.describe = false;
    return options;
}

ObjectRefIdentityOptions WithoutIsValidAndDescribe() {
    ObjectRefIdentityOptions options;
    options.isValid = false;
    options.describe = false;
    return options;
}

ObjectRefIdentityOptions WithoutIsValidValidAndDescribe() {
    ObjectRefIdentityOptions options;
    options.isValid = false;
    options.validProperty = false;
    options.describe = false;
    return options;
}

template <typename RefT>
RefT *MakeInvalidTypedRef(CKContext *context, const std::string &error) {
    return new RefT(context, 0, ScriptBridgeObjectStamp(), error);
}

template <typename RefT, typename ObjectT>
RefT *MakeTypedObjectRef(CKContext *context, ObjectT *object, const std::string &error) {
    if (!object) {
        return MakeInvalidTypedRef<RefT>(context, error);
    }
    return new RefT(object->GetCKContext(), object);
}

template <typename RefT>
CScriptArray *CreateTypedRefArray(const char *decl) {
    asIScriptContext *active = asGetActiveContext();
    asIScriptEngine *engine = active ? active->GetEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl(decl) : nullptr;
    if (!arrayType) {
        SetScriptException(fmt::format("{} is not registered.", decl));
        return nullptr;
    }
    return CScriptArray::Create(arrayType, asUINT(0));
}

template <typename RefT>
void AppendTypedRef(CScriptArray *array, RefT *ref) {
    if (!array || !ref) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &ref);
    ref->Release();
}

CK3dEntity *ResolveEntity3D(const Entity3DRef *ref, const char *method) {
    CK3dEntity *entity = ref ? ref->Entity3D() : nullptr;
    if (!entity && ref) {
        ref->SetError(fmt::format("Entity3DRef::{} requires a valid CK3dEntity.", method));
    }
    return entity;
}

CK2dEntity *ResolveEntity2D(const Entity2DRef *ref, const char *method) {
    CK2dEntity *entity = ref ? ref->Entity2D() : nullptr;
    if (!entity && ref) {
        ref->SetError(fmt::format("Entity2DRef::{} requires a valid CK2dEntity.", method));
    }
    return entity;
}

bool ResolveEntity3DReference(const ObjectRef *owner,
                              Entity3DRef *reference,
                              CKContext *expectedContext,
                              const char *method,
                              CK3dEntity *&out) {
    out = nullptr;
    if (!reference) {
        return true;
    }
    out = reference->Entity3D();
    if (!out) {
        if (owner) {
            owner->SetError(fmt::format("Entity3DRef::{} reference entity is invalid.", method));
        }
        return false;
    }
    if (expectedContext && out->GetCKContext() != expectedContext) {
        if (owner) {
            owner->SetError(fmt::format("Entity3DRef::{} reference belongs to another CKContext.", method));
        }
        out = nullptr;
        return false;
    }
    return true;
}

bool ResolveEntity2DReference(const ObjectRef *owner,
                              Entity2DRef *reference,
                              CKContext *expectedContext,
                              const char *method,
                              CK2dEntity *&out) {
    out = nullptr;
    if (!reference) {
        return true;
    }
    out = reference->Entity2D();
    if (!out) {
        if (owner) {
            owner->SetError(fmt::format("Entity2DRef::{} reference entity is invalid.", method));
        }
        return false;
    }
    if (expectedContext && out->GetCKContext() != expectedContext) {
        if (owner) {
            owner->SetError(fmt::format("Entity2DRef::{} reference belongs to another CKContext.", method));
        }
        out = nullptr;
        return false;
    }
    return true;
}

bool IsAncestor3D(CK3dEntity *ancestor, CK3dEntity *entity) {
    if (!ancestor || !entity || ancestor == entity) {
        return false;
    }
    for (CK3dEntity *current = entity->GetParent(); current; current = current->GetParent()) {
        if (current == ancestor) {
            return true;
        }
    }
    return false;
}

bool IsAncestor2D(CK2dEntity *ancestor, CK2dEntity *entity) {
    if (!ancestor || !entity || ancestor == entity) {
        return false;
    }
    for (CK2dEntity *current = entity->GetParent(); current; current = current->GetParent()) {
        if (current == ancestor) {
            return true;
        }
    }
    return false;
}

} // namespace

ObjectRef::ObjectRef(CKContext *context, CKObject *object) {
    ResetObject(object ? object->GetCKContext() : context, object);
}

ObjectRef::ObjectRef(CKContext *context, CK_ID id, std::string error) {
    CKObject *object = ObjectById(context, id);
    ResetObject(context, id, CaptureBridgeObjectStamp(object), std::move(error), ObjectAddress(object));
}

ObjectRef::ObjectRef(CKContext *context,
                     CK_ID id,
                     const ScriptBridgeObjectStamp &stamp,
                     std::string error,
                     std::uintptr_t objectAddress) {
    ResetObject(context, id, stamp, std::move(error), objectAddress);
}

bool ObjectRef::IsValid() const { return Resolve() != nullptr; }
bool ObjectRef::valid() const { return IsValid(); }
std::string ObjectRef::Error() const { return m_Error; }

std::string ObjectRef::Describe() const {
    CKObject *object = Resolve();
    if (!object) {
        return m_Error.empty() ? "ObjectRef is not valid." : m_Error;
    }
    return fmt::format("{} '{}' id={} class={}",
                       object->IsDynamic() ? "dynamic object" : "object",
                       SafeString(object->GetName()),
                       object->GetID(),
                       object->GetClassID());
}

CK_ID ObjectRef::Id() const { return m_Id; }
CK_ID ObjectRef::GetID() const { return Id(); }

std::string ObjectRef::Name() const {
    CKObject *object = Resolve();
    return object ? SafeString(object->GetName()) : std::string();
}

std::string ObjectRef::GetName() const { return Name(); }

CK_CLASSID ObjectRef::ClassId() const {
    CKObject *object = Resolve();
    return object ? object->GetClassID() : 0;
}

bool ObjectRef::IsDynamic() const {
    CKObject *object = Resolve();
    return object && object->IsDynamic();
}

CKObject *ObjectRef::Object() const { return Resolve(); }

bool ObjectRef::SetName(const std::string &name, bool shared) {
    CKObject *object = Resolve();
    if (!object) {
        return false;
    }
    object->SetName(const_cast<CKSTRING>(name.c_str()), shared);
    return true;
}

bool ObjectRef::SetDynamic(bool dynamic) {
    CKObject *object = Resolve();
    if (!object) {
        return false;
    }
    CKContext *context = object->GetCKContext();
    if (!context) {
        SetError("ObjectRef::SetDynamic requires a CKContext.");
        return false;
    }
    context->ChangeObjectDynamic(object, dynamic);
    return true;
}

bool ObjectRef::Show(CK_OBJECT_SHOWOPTION show) {
    CKObject *object = Resolve();
    if (!object) {
        return false;
    }
    object->Show(show);
    return true;
}

bool ObjectRef::IsVisible() const {
    CKObject *object = Resolve();
    return object && object->IsVisible();
}

CKDWORD ObjectRef::ObjectFlags() const {
    CKObject *object = Resolve();
    return object ? object->GetObjectFlags() : 0;
}

bool ObjectRef::ModifyObjectFlags(CKDWORD add, CKDWORD remove) {
    CKObject *object = Resolve();
    if (!object) {
        return false;
    }
    object->ModifyObjectFlags(add, remove);
    return true;
}

CKContext *ObjectRef::Context() const { return m_Context; }
const ScriptBridgeObjectStamp &ObjectRef::Stamp() const { return m_Stamp; }

void ObjectRef::SetError(std::string error) const {
    m_Error = std::move(error);
}

void ObjectRef::Invalidate(std::string error) {
    ClearObject(std::move(error));
}

CKObject *ObjectRef::Resolve() const {
    if (!m_Context || !m_Id) {
        if (m_Error.empty()) {
            m_Error = "ObjectRef has no object.";
        }
        return nullptr;
    }
    CKObject *object = GetStampedCKObjectById(m_Context, m_Id, m_Stamp);
    if (!object) {
        m_Error = fmt::format("Object id={} is no longer valid.", m_Id);
        return nullptr;
    }
    if (object->IsToBeDeleted()) {
        m_Error = fmt::format("Object id={} is pending deletion.", m_Id);
        return nullptr;
    }
    if (m_ObjectAddress && ObjectAddress(object) != m_ObjectAddress) {
        m_Error = fmt::format("Object id={} was reused by another native object.", m_Id);
        return nullptr;
    }
    m_Error.clear();
    return object;
}

void ObjectRef::ResetObject(CKContext *context, CKObject *object, std::string error) {
    CKContext *resolvedContext = object ? object->GetCKContext() : context;
    ResetObject(resolvedContext,
                object ? object->GetID() : 0,
                CaptureBridgeObjectStamp(object),
                std::move(error),
                ObjectAddress(object));
}

void ObjectRef::ResetObject(CKContext *context,
                            CK_ID id,
                            const ScriptBridgeObjectStamp &stamp,
                            std::string error,
                            std::uintptr_t objectAddress) {
    m_Context = context;
    m_Id = id;
    m_Stamp = stamp;
    m_ObjectAddress = objectAddress;
    m_Error = std::move(error);
}

void ObjectRef::ClearObject(std::string error) {
    ResetObject(nullptr, 0, ScriptBridgeObjectStamp(), std::move(error), 0);
}

CKSceneObject *SceneObjectRef::SceneObject() const { return CKSceneObject::Cast(Resolve()); }
CK3dEntity *Entity3DRef::Entity3D() const { return CK3dEntity::Cast(Resolve()); }
CK2dEntity *Entity2DRef::Entity2D() const { return CK2dEntity::Cast(Resolve()); }
CKMaterial *MaterialRef::Material() const { return CKMaterial::Cast(Resolve()); }
CKTexture *TextureRef::Texture() const { return CKTexture::Cast(Resolve()); }
CKMesh *MeshRef::Mesh() const { return CKMesh::Cast(Resolve()); }
CKScene *SceneRef::Scene() const { return CKScene::Cast(Resolve()); }
CKLevel *LevelRef::Level() const { return CKLevel::Cast(Resolve()); }

bool SceneObjectRef::IsInScene(SceneRef *scene) const {
    CKObject *object = Object();
    CKScene *resolvedScene = scene ? scene->Scene() : nullptr;
    if (!object) {
        return false;
    }
    if (!resolvedScene) {
        SetError("SceneObjectRef::IsInScene requires a valid SceneRef.");
        return false;
    }
    if (object->GetCKContext() != resolvedScene->GetCKContext()) {
        SetError("SceneObjectRef::IsInScene requires scene and object from the same CKContext.");
        return false;
    }
    return ScriptSceneIsObjectInScene(object, resolvedScene);
}

bool SceneObjectRef::IsInCurrentScene() const {
    CKObject *object = Object();
    if (!object) {
        return false;
    }
    CKContext *context = object->GetCKContext();
    CKScene *scene = context ? context->GetCurrentScene() : nullptr;
    if (!scene) {
        SetError("Current scene is not available.");
        return false;
    }
    return ScriptSceneIsObjectInScene(object, scene);
}

bool Entity3DRef::SetPosition(const VxVector &pos, Entity3DRef *reference, bool keepChildren) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetPosition");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "SetPosition", referenceEntity)) {
        return false;
    }
    entity->SetPosition(const_cast<VxVector *>(&pos), referenceEntity, keepChildren);
    return true;
}

bool Entity3DRef::SetPosition(float x, float y, float z, Entity3DRef *reference, bool keepChildren) {
    return SetPosition(VxVector(x, y, z), reference, keepChildren);
}

bool Entity3DRef::GetPosition(VxVector &pos, Entity3DRef *reference) const {
    CK3dEntity *entity = ResolveEntity3D(this, "GetPosition");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "GetPosition", referenceEntity)) {
        return false;
    }
    entity->GetPosition(&pos, referenceEntity);
    return true;
}

bool Entity3DRef::Translate(const VxVector &delta, Entity3DRef *reference, bool keepChildren) {
    CK3dEntity *entity = ResolveEntity3D(this, "Translate");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "Translate", referenceEntity)) {
        return false;
    }
    entity->Translate(const_cast<VxVector *>(&delta), referenceEntity, keepChildren);
    return true;
}

bool Entity3DRef::Translate(float x, float y, float z, Entity3DRef *reference, bool keepChildren) {
    return Translate(VxVector(x, y, z), reference, keepChildren);
}

bool Entity3DRef::SetQuaternion(const VxQuaternion &quat,
                                Entity3DRef *reference,
                                bool keepChildren,
                                bool keepScale) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetQuaternion");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "SetQuaternion", referenceEntity)) {
        return false;
    }
    entity->SetQuaternion(const_cast<VxQuaternion *>(&quat), referenceEntity, keepChildren, keepScale);
    return true;
}

bool Entity3DRef::GetQuaternion(VxQuaternion &quat, Entity3DRef *reference) const {
    CK3dEntity *entity = ResolveEntity3D(this, "GetQuaternion");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "GetQuaternion", referenceEntity)) {
        return false;
    }
    entity->GetQuaternion(&quat, referenceEntity);
    return true;
}

bool Entity3DRef::SetScale(const VxVector &scale, bool keepChildren, bool local) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetScale");
    if (!entity) {
        return false;
    }
    entity->SetScale(const_cast<VxVector *>(&scale), keepChildren, local);
    return true;
}

bool Entity3DRef::SetScale(float x, float y, float z, bool keepChildren, bool local) {
    return SetScale(VxVector(x, y, z), keepChildren, local);
}

bool Entity3DRef::GetScale(VxVector &scale, bool local) const {
    CK3dEntity *entity = ResolveEntity3D(this, "GetScale");
    if (!entity) {
        return false;
    }
    entity->GetScale(&scale, local);
    return true;
}

bool Entity3DRef::LookAt(const VxVector &pos, Entity3DRef *reference, bool keepChildren) {
    CK3dEntity *entity = ResolveEntity3D(this, "LookAt");
    if (!entity) {
        return false;
    }
    CK3dEntity *referenceEntity = nullptr;
    if (!ResolveEntity3DReference(this, reference, entity->GetCKContext(), "LookAt", referenceEntity)) {
        return false;
    }
    entity->LookAt(const_cast<VxVector *>(&pos), referenceEntity, keepChildren);
    return true;
}

Entity3DRef *Entity3DRef::Parent() const {
    CK3dEntity *entity = ResolveEntity3D(this, "Parent");
    if (!entity) {
        return MakeInvalidTypedRef<Entity3DRef>(Context(), "Entity3DRef::Parent requires a valid CK3dEntity.");
    }
    return MakeTypedObjectRef<Entity3DRef>(entity->GetCKContext(),
                                           entity->GetParent(),
                                           "Entity3DRef has no parent.");
}

Entity3DRef *Entity3DRef::Child(int index) const {
    CK3dEntity *entity = ResolveEntity3D(this, "Child");
    if (!entity) {
        return MakeInvalidTypedRef<Entity3DRef>(Context(), "Entity3DRef::Child requires a valid CK3dEntity.");
    }
    if (index < 0 || index >= entity->GetChildrenCount()) {
        return MakeInvalidTypedRef<Entity3DRef>(entity->GetCKContext(), "Entity3DRef child index is out of range.");
    }
    return MakeTypedObjectRef<Entity3DRef>(entity->GetCKContext(), entity->GetChild(index), "Entity3DRef child is null.");
}

CScriptArray *Entity3DRef::Children() const {
    CScriptArray *array = CreateTypedRefArray<Entity3DRef>("array<Entity3DRef@>");
    if (!array) {
        return nullptr;
    }
    CK3dEntity *entity = ResolveEntity3D(this, "Children");
    if (!entity) {
        return array;
    }
    const int count = entity->GetChildrenCount();
    for (int i = 0; i < count; ++i) {
        CK3dEntity *child = entity->GetChild(i);
        if (child) {
            AppendTypedRef(array, new Entity3DRef(child->GetCKContext(), child));
        }
    }
    return array;
}

int Entity3DRef::ChildCount() const {
    CK3dEntity *entity = ResolveEntity3D(this, "ChildCount");
    return entity ? entity->GetChildrenCount() : 0;
}

bool Entity3DRef::SetParent(Entity3DRef *parent, bool keepWorldPos) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetParent");
    if (!entity) {
        return false;
    }
    CK3dEntity *parentEntity = nullptr;
    if (!ResolveEntity3DReference(this, parent, entity->GetCKContext(), "SetParent", parentEntity)) {
        return false;
    }
    if (parentEntity == entity || IsAncestor3D(entity, parentEntity)) {
        SetError("Entity3DRef::SetParent would create a hierarchy cycle.");
        return false;
    }
    return entity->SetParent(parentEntity, keepWorldPos) != FALSE;
}

bool Entity3DRef::AddChild(Entity3DRef *child, bool keepWorldPos) {
    CK3dEntity *entity = ResolveEntity3D(this, "AddChild");
    if (!entity) {
        return false;
    }
    CK3dEntity *childEntity = nullptr;
    if (!ResolveEntity3DReference(this, child, entity->GetCKContext(), "AddChild", childEntity) || !childEntity) {
        SetError("Entity3DRef::AddChild requires a valid child.");
        return false;
    }
    if (childEntity == entity || IsAncestor3D(childEntity, entity)) {
        SetError("Entity3DRef::AddChild would create a hierarchy cycle.");
        return false;
    }
    return entity->AddChild(childEntity, keepWorldPos) != FALSE;
}

bool Entity3DRef::RemoveChild(Entity3DRef *child) {
    CK3dEntity *entity = ResolveEntity3D(this, "RemoveChild");
    if (!entity) {
        return false;
    }
    CK3dEntity *childEntity = nullptr;
    if (!ResolveEntity3DReference(this, child, entity->GetCKContext(), "RemoveChild", childEntity) || !childEntity) {
        SetError("Entity3DRef::RemoveChild requires a valid child.");
        return false;
    }
    return entity->RemoveChild(childEntity) != FALSE;
}

bool Entity3DRef::IsAncestorOf(Entity3DRef *other) const {
    CK3dEntity *entity = ResolveEntity3D(this, "IsAncestorOf");
    CK3dEntity *otherEntity = nullptr;
    if (!entity || !ResolveEntity3DReference(this, other, entity->GetCKContext(), "IsAncestorOf", otherEntity)) {
        return false;
    }
    return IsAncestor3D(entity, otherEntity);
}

bool Entity3DRef::IsDescendantOf(Entity3DRef *other) const {
    CK3dEntity *entity = ResolveEntity3D(this, "IsDescendantOf");
    CK3dEntity *otherEntity = nullptr;
    if (!entity || !ResolveEntity3DReference(this, other, entity->GetCKContext(), "IsDescendantOf", otherEntity)) {
        return false;
    }
    return IsAncestor3D(otherEntity, entity);
}

bool Entity3DRef::SetPickable(bool pick) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetPickable");
    if (!entity) {
        return false;
    }
    entity->SetPickable(pick);
    return true;
}

bool Entity3DRef::IsPickable() const {
    CK3dEntity *entity = ResolveEntity3D(this, "IsPickable");
    return entity && entity->IsPickable();
}

MeshRef *Entity3DRef::CurrentMesh() const {
    CK3dEntity *entity = ResolveEntity3D(this, "CurrentMesh");
    if (!entity) {
        return MakeInvalidTypedRef<MeshRef>(Context(), "Entity3DRef::CurrentMesh requires a valid CK3dEntity.");
    }
    return MakeTypedObjectRef<MeshRef>(entity->GetCKContext(), entity->GetCurrentMesh(), "Entity3DRef has no current mesh.");
}

bool Entity3DRef::SetCurrentMesh(MeshRef *mesh, bool addIfNotHere) {
    CK3dEntity *entity = ResolveEntity3D(this, "SetCurrentMesh");
    if (!entity) {
        return false;
    }
    CKMesh *resolvedMesh = mesh ? mesh->Mesh() : nullptr;
    if (!resolvedMesh) {
        SetError("Entity3DRef::SetCurrentMesh requires a valid MeshRef.");
        return false;
    }
    if (resolvedMesh->GetCKContext() != entity->GetCKContext()) {
        SetError("Entity3DRef::SetCurrentMesh mesh belongs to another CKContext.");
        return false;
    }
    return entity->SetCurrentMesh(resolvedMesh, addIfNotHere) != nullptr;
}

int Entity3DRef::MeshCount() const {
    CK3dEntity *entity = ResolveEntity3D(this, "MeshCount");
    return entity ? entity->GetMeshCount() : 0;
}

MeshRef *Entity3DRef::Mesh(int index) const {
    CK3dEntity *entity = ResolveEntity3D(this, "Mesh");
    if (!entity) {
        return MakeInvalidTypedRef<MeshRef>(Context(), "Entity3DRef::Mesh requires a valid CK3dEntity.");
    }
    if (index < 0 || index >= entity->GetMeshCount()) {
        return MakeInvalidTypedRef<MeshRef>(entity->GetCKContext(), "Entity3DRef mesh index is out of range.");
    }
    return MakeTypedObjectRef<MeshRef>(entity->GetCKContext(), entity->GetMesh(index), "Entity3DRef mesh is null.");
}

bool Entity3DRef::AddMesh(MeshRef *mesh) {
    CK3dEntity *entity = ResolveEntity3D(this, "AddMesh");
    if (!entity) {
        return false;
    }
    CKMesh *resolvedMesh = mesh ? mesh->Mesh() : nullptr;
    if (!resolvedMesh) {
        SetError("Entity3DRef::AddMesh requires a valid MeshRef.");
        return false;
    }
    if (resolvedMesh->GetCKContext() != entity->GetCKContext()) {
        SetError("Entity3DRef::AddMesh mesh belongs to another CKContext.");
        return false;
    }
    return entity->AddMesh(resolvedMesh) == CK_OK;
}

Entity2DRef *Entity2DRef::Parent() const {
    CK2dEntity *entity = ResolveEntity2D(this, "Parent");
    if (!entity) {
        return MakeInvalidTypedRef<Entity2DRef>(Context(), "Entity2DRef::Parent requires a valid CK2dEntity.");
    }
    return MakeTypedObjectRef<Entity2DRef>(entity->GetCKContext(),
                                           entity->GetParent(),
                                           "Entity2DRef has no parent.");
}

Entity2DRef *Entity2DRef::Child(int index) const {
    CK2dEntity *entity = ResolveEntity2D(this, "Child");
    if (!entity) {
        return MakeInvalidTypedRef<Entity2DRef>(Context(), "Entity2DRef::Child requires a valid CK2dEntity.");
    }
    if (index < 0 || index >= entity->GetChildrenCount()) {
        return MakeInvalidTypedRef<Entity2DRef>(entity->GetCKContext(), "Entity2DRef child index is out of range.");
    }
    return MakeTypedObjectRef<Entity2DRef>(entity->GetCKContext(), entity->GetChild(index), "Entity2DRef child is null.");
}

CScriptArray *Entity2DRef::Children() const {
    CScriptArray *array = CreateTypedRefArray<Entity2DRef>("array<Entity2DRef@>");
    if (!array) {
        return nullptr;
    }
    CK2dEntity *entity = ResolveEntity2D(this, "Children");
    if (!entity) {
        return array;
    }
    const int count = entity->GetChildrenCount();
    for (int i = 0; i < count; ++i) {
        CK2dEntity *child = entity->GetChild(i);
        if (child) {
            AppendTypedRef(array, new Entity2DRef(child->GetCKContext(), child));
        }
    }
    return array;
}

int Entity2DRef::ChildCount() const {
    CK2dEntity *entity = ResolveEntity2D(this, "ChildCount");
    return entity ? entity->GetChildrenCount() : 0;
}

bool Entity2DRef::SetParent(Entity2DRef *parent) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetParent");
    if (!entity) {
        return false;
    }
    CK2dEntity *parentEntity = nullptr;
    if (parent) {
        parentEntity = parent->Entity2D();
        if (!parentEntity) {
            SetError("Entity2DRef::SetParent parent entity is invalid.");
            return false;
        }
        if (parentEntity->GetCKContext() != entity->GetCKContext()) {
            SetError("Entity2DRef::SetParent parent belongs to another CKContext.");
            return false;
        }
    }
    if (parentEntity == entity || IsAncestor2D(entity, parentEntity)) {
        SetError("Entity2DRef::SetParent would create a hierarchy cycle.");
        return false;
    }
    return entity->SetParent(parentEntity) != FALSE;
}

bool Entity2DRef::IsAncestorOf(Entity2DRef *other) const {
    CK2dEntity *entity = ResolveEntity2D(this, "IsAncestorOf");
    CK2dEntity *otherEntity = other ? other->Entity2D() : nullptr;
    if (!entity || !otherEntity) {
        return false;
    }
    if (otherEntity->GetCKContext() != entity->GetCKContext()) {
        SetError("Entity2DRef::IsAncestorOf target belongs to another CKContext.");
        return false;
    }
    return IsAncestor2D(entity, otherEntity);
}

bool Entity2DRef::IsDescendantOf(Entity2DRef *other) const {
    CK2dEntity *entity = ResolveEntity2D(this, "IsDescendantOf");
    CK2dEntity *otherEntity = other ? other->Entity2D() : nullptr;
    if (!entity || !otherEntity) {
        return false;
    }
    if (otherEntity->GetCKContext() != entity->GetCKContext()) {
        SetError("Entity2DRef::IsDescendantOf target belongs to another CKContext.");
        return false;
    }
    return IsAncestor2D(otherEntity, entity);
}

bool Entity2DRef::SetPosition(const Vx2DVector &pos,
                              bool homogeneous,
                              Entity2DRef *reference,
                              bool keepChildren) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetPosition");
    if (!entity) {
        return false;
    }
    CK2dEntity *referenceEntity = nullptr;
    if (!ResolveEntity2DReference(this, reference, entity->GetCKContext(), "SetPosition", referenceEntity)) {
        return false;
    }
    entity->SetPosition(pos, homogeneous, keepChildren, referenceEntity);
    return true;
}

bool Entity2DRef::SetPosition(float x,
                              float y,
                              bool homogeneous,
                              Entity2DRef *reference,
                              bool keepChildren) {
    return SetPosition(Vx2DVector(x, y), homogeneous, reference, keepChildren);
}

bool Entity2DRef::GetPosition(Vx2DVector &pos, bool homogeneous, Entity2DRef *reference) const {
    CK2dEntity *entity = ResolveEntity2D(this, "GetPosition");
    if (!entity) {
        return false;
    }
    CK2dEntity *referenceEntity = nullptr;
    if (!ResolveEntity2DReference(this, reference, entity->GetCKContext(), "GetPosition", referenceEntity)) {
        return false;
    }
    return entity->GetPosition(pos, homogeneous, referenceEntity) >= 0;
}

bool Entity2DRef::SetSize(const Vx2DVector &size, bool homogeneous, bool keepChildren) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetSize");
    if (!entity) {
        return false;
    }
    entity->SetSize(size, homogeneous, keepChildren);
    return true;
}

bool Entity2DRef::SetSize(float x, float y, bool homogeneous, bool keepChildren) {
    return SetSize(Vx2DVector(x, y), homogeneous, keepChildren);
}

bool Entity2DRef::GetSize(Vx2DVector &size, bool homogeneous) const {
    CK2dEntity *entity = ResolveEntity2D(this, "GetSize");
    return entity && entity->GetSize(size, homogeneous) >= 0;
}

bool Entity2DRef::SetRect(const VxRect &rect, bool keepChildren) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetRect");
    if (!entity) {
        return false;
    }
    entity->SetRect(rect, keepChildren);
    return true;
}

bool Entity2DRef::GetRect(VxRect &rect) const {
    CK2dEntity *entity = ResolveEntity2D(this, "GetRect");
    if (!entity) {
        return false;
    }
    entity->GetRect(rect);
    return true;
}

bool Entity2DRef::SetSourceRect(const VxRect &rect) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetSourceRect");
    if (!entity) {
        return false;
    }
    VxRect copy = rect;
    entity->SetSourceRect(copy);
    return true;
}

bool Entity2DRef::GetSourceRect(VxRect &rect) const {
    CK2dEntity *entity = ResolveEntity2D(this, "GetSourceRect");
    if (!entity) {
        return false;
    }
    entity->GetSourceRect(rect);
    return true;
}

bool Entity2DRef::UseSourceRect(bool use) {
    CK2dEntity *entity = ResolveEntity2D(this, "UseSourceRect");
    if (!entity) {
        return false;
    }
    entity->UseSourceRect(use);
    return true;
}

bool Entity2DRef::IsUsingSourceRect() const {
    CK2dEntity *entity = ResolveEntity2D(this, "IsUsingSourceRect");
    return entity && entity->IsUsingSourceRect();
}

bool Entity2DRef::SetMaterial(MaterialRef *material) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetMaterial");
    if (!entity) {
        return false;
    }
    CKMaterial *resolvedMaterial = material ? material->Material() : nullptr;
    if (material && !resolvedMaterial) {
        SetError("Entity2DRef::SetMaterial material is invalid.");
        return false;
    }
    if (resolvedMaterial && resolvedMaterial->GetCKContext() != entity->GetCKContext()) {
        SetError("Entity2DRef::SetMaterial material belongs to another CKContext.");
        return false;
    }
    entity->SetMaterial(resolvedMaterial);
    return true;
}

MaterialRef *Entity2DRef::Material() const {
    CK2dEntity *entity = ResolveEntity2D(this, "Material");
    if (!entity) {
        return MakeInvalidTypedRef<MaterialRef>(Context(), "Entity2DRef::Material requires a valid CK2dEntity.");
    }
    return MakeTypedObjectRef<MaterialRef>(entity->GetCKContext(), entity->GetMaterial(), "Entity2DRef has no material.");
}

bool Entity2DRef::SetPickable(bool pick) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetPickable");
    if (!entity) {
        return false;
    }
    entity->SetPickable(pick);
    return true;
}

bool Entity2DRef::IsPickable() const {
    CK2dEntity *entity = ResolveEntity2D(this, "IsPickable");
    return entity && entity->IsPickable();
}

bool Entity2DRef::SetClipToParent(bool clip) {
    CK2dEntity *entity = ResolveEntity2D(this, "SetClipToParent");
    if (!entity) {
        return false;
    }
    entity->SetClipToParent(clip);
    return true;
}

bool Entity2DRef::IsClipToParent() const {
    CK2dEntity *entity = ResolveEntity2D(this, "IsClipToParent");
    return entity && entity->IsClipToParent();
}

TextureRef *MaterialRef::Texture(int slot) const {
    CKMaterial *material = Material();
    if (!material) {
        return MakeInvalidTypedRef<TextureRef>(Context(), "MaterialRef::Texture requires a valid CKMaterial.");
    }
    if (slot < 0 || slot >= 4) {
        return MakeInvalidTypedRef<TextureRef>(material->GetCKContext(), "MaterialRef texture slot is out of range.");
    }
    return MakeTypedObjectRef<TextureRef>(material->GetCKContext(),
                                          material->GetTexture(slot),
                                          "MaterialRef texture slot is empty.");
}

bool MaterialRef::SetTexture(TextureRef *texture, int slot) {
    CKMaterial *material = Material();
    if (!material) {
        SetError("MaterialRef::SetTexture requires a valid CKMaterial.");
        return false;
    }
    if (slot < 0 || slot >= 4) {
        SetError("MaterialRef texture slot is out of range.");
        return false;
    }
    CKTexture *resolvedTexture = texture ? texture->Texture() : nullptr;
    if (texture && !resolvedTexture) {
        SetError("MaterialRef::SetTexture texture is invalid.");
        return false;
    }
    if (resolvedTexture && resolvedTexture->GetCKContext() != material->GetCKContext()) {
        SetError("MaterialRef::SetTexture texture belongs to another CKContext.");
        return false;
    }
    material->SetTexture(slot, resolvedTexture);
    return true;
}

ObjectRef *MakeScriptObjectRef(CKContext *context,
                               CKObject *object,
                               ScriptBehaviorBridge *bridge,
                               CK_ID componentId,
                               const std::string &error) {
    CKContext *resolvedContext = object ? object->GetCKContext() : context;
    if (!object) {
        return MakeInvalidObjectRef(resolvedContext, error.empty() ? "Object is null." : error);
    }
    if (!bridge) {
        bridge = BridgeFromContext(resolvedContext);
    }

    if (CKBehavior *behavior = CKBehavior::Cast(object)) {
        return new BehaviorRef(bridge, behavior->GetID(), componentId, resolvedContext);
    }
    if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
        return new ParamOperationRef(bridge, operation->GetID(), nullptr, nullptr, {}, resolvedContext);
    }
    if (CKParameterIn *param = CKParameterIn::Cast(object)) {
        return new ParamInRef(bridge, param->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, resolvedContext);
    }
    if (CKParameterOut *param = CKParameterOut::Cast(object)) {
        return new ParamOutRef(bridge, param->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, resolvedContext);
    }
    if (CKParameterLocal *param = CKParameterLocal::Cast(object)) {
        return new ParamLocalRef(bridge, param->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, resolvedContext);
    }
    if (CKParameter *param = CKParameter::Cast(object)) {
        return new ParamRef(bridge, param->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, resolvedContext);
    }
    if (CKBehaviorLink *link = CKBehaviorLink::Cast(object)) {
        return new BehaviorLinkRef(bridge, 0, link->GetID(), componentId, resolvedContext);
    }
    if (CK3dEntity *entity = CK3dEntity::Cast(object)) {
        return new Entity3DRef(resolvedContext, entity);
    }
    if (CK2dEntity *entity = CK2dEntity::Cast(object)) {
        return new Entity2DRef(resolvedContext, entity);
    }
    if (CKMaterial *material = CKMaterial::Cast(object)) {
        return new MaterialRef(resolvedContext, material);
    }
    if (CKTexture *texture = CKTexture::Cast(object)) {
        return new TextureRef(resolvedContext, texture);
    }
    if (CKMesh *mesh = CKMesh::Cast(object)) {
        return new MeshRef(resolvedContext, mesh);
    }
    if (CKScene *scene = CKScene::Cast(object)) {
        return new SceneRef(resolvedContext, scene);
    }
    if (CKLevel *level = CKLevel::Cast(object)) {
        return new LevelRef(resolvedContext, level);
    }
    if (CKSceneObject *sceneObject = CKSceneObject::Cast(object)) {
        return new SceneObjectRef(resolvedContext, sceneObject);
    }
    return new ObjectRef(resolvedContext, object);
}

ObjectRef *MakeInvalidObjectRef(CKContext *context, const std::string &error) {
    return new ObjectRef(context, 0, ScriptBridgeObjectStamp(), error.empty() ? "Object is null." : error);
}

void RegisterScriptObjectRefCore(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;
    // Core refs are registered with the raw CK object bindings. Bridge refs are
    // added later after their classes are visible, and Scene::* is registered last
    // because it returns both core and bridge refs.
    RegisterObjectRefType<ObjectRef>(engine, "ObjectRef");
    RegisterObjectRefType<SceneObjectRef>(engine, "SceneObjectRef");
    RegisterObjectRefType<Entity3DRef>(engine, "Entity3DRef");
    RegisterObjectRefType<Entity2DRef>(engine, "Entity2DRef");
    RegisterObjectRefType<MaterialRef>(engine, "MaterialRef");
    RegisterObjectRefType<TextureRef>(engine, "TextureRef");
    RegisterObjectRefType<MeshRef>(engine, "MeshRef");
    RegisterObjectRefType<SceneRef>(engine, "SceneRef");
    RegisterObjectRefType<LevelRef>(engine, "LevelRef");

    RegisterObjectRefCast<SceneObjectRef, ObjectRef>(engine, "SceneObjectRef", "ObjectRef");
    RegisterObjectRefCast<Entity3DRef, ObjectRef>(engine, "Entity3DRef", "ObjectRef");
    RegisterObjectRefCast<Entity2DRef, ObjectRef>(engine, "Entity2DRef", "ObjectRef");
    RegisterObjectRefCast<MaterialRef, ObjectRef>(engine, "MaterialRef", "ObjectRef");
    RegisterObjectRefCast<TextureRef, ObjectRef>(engine, "TextureRef", "ObjectRef");
    RegisterObjectRefCast<MeshRef, ObjectRef>(engine, "MeshRef", "ObjectRef");
    RegisterObjectRefCast<SceneRef, ObjectRef>(engine, "SceneRef", "ObjectRef");
    RegisterObjectRefCast<LevelRef, ObjectRef>(engine, "LevelRef", "ObjectRef");
    RegisterObjectRefCast<Entity3DRef, SceneObjectRef>(engine, "Entity3DRef", "SceneObjectRef");
    RegisterObjectRefCast<Entity2DRef, SceneObjectRef>(engine, "Entity2DRef", "SceneObjectRef");
    RegisterObjectRefCast<SceneRef, SceneObjectRef>(engine, "SceneRef", "SceneObjectRef");

    RegisterObjectRefAccessor(engine, "SceneObjectRef", "CKSceneObject@ SceneObject() const", &SceneObjectRef::SceneObject);
    RegisterObjectRefAccessor(engine, "Entity3DRef", "CK3dEntity@ Entity3D() const", &Entity3DRef::Entity3D);
    RegisterObjectRefAccessor(engine, "Entity2DRef", "CK2dEntity@ Entity2D() const", &Entity2DRef::Entity2D);
    RegisterObjectRefAccessor(engine, "MaterialRef", "CKMaterial@ Material() const", &MaterialRef::Material);
    RegisterObjectRefAccessor(engine, "TextureRef", "CKTexture@ Texture() const", &TextureRef::Texture);
    RegisterObjectRefAccessor(engine, "MeshRef", "CKMesh@ Mesh() const", &MeshRef::Mesh);
    RegisterObjectRefAccessor(engine, "SceneRef", "CKScene@ Scene() const", &SceneRef::Scene);
    RegisterObjectRefAccessor(engine, "LevelRef", "CKLevel@ Level() const", &LevelRef::Level);

    r = engine->RegisterObjectMethod("SceneObjectRef", "bool IsInScene(SceneRef@ scene) const", asMETHOD(SceneObjectRef, IsInScene), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("SceneObjectRef", "bool IsInCurrentScene() const", asMETHOD(SceneObjectRef, IsInCurrentScene), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetPosition(const VxVector &in pos, Entity3DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity3DRef, SetPosition, (const VxVector &, Entity3DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetPosition(float x, float y, float z, Entity3DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity3DRef, SetPosition, (float, float, float, Entity3DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool GetPosition(VxVector &out pos, Entity3DRef@ reference = null) const", asMETHODPR(Entity3DRef, GetPosition, (VxVector &, Entity3DRef *) const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool Translate(const VxVector &in delta, Entity3DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity3DRef, Translate, (const VxVector &, Entity3DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool Translate(float x, float y, float z, Entity3DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity3DRef, Translate, (float, float, float, Entity3DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetQuaternion(const VxQuaternion &in quat, Entity3DRef@ reference = null, bool keepChildren = false, bool keepScale = false)", asMETHODPR(Entity3DRef, SetQuaternion, (const VxQuaternion &, Entity3DRef *, bool, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool GetQuaternion(VxQuaternion &out quat, Entity3DRef@ reference = null) const", asMETHODPR(Entity3DRef, GetQuaternion, (VxQuaternion &, Entity3DRef *) const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetScale(const VxVector &in scale, bool keepChildren = false, bool local = true)", asMETHODPR(Entity3DRef, SetScale, (const VxVector &, bool, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetScale(float x, float y, float z, bool keepChildren = false, bool local = true)", asMETHODPR(Entity3DRef, SetScale, (float, float, float, bool, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool GetScale(VxVector &out scale, bool local = true) const", asMETHODPR(Entity3DRef, GetScale, (VxVector &, bool) const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool LookAt(const VxVector &in pos, Entity3DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity3DRef, LookAt, (const VxVector &, Entity3DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "Entity3DRef@ Parent() const", asMETHOD(Entity3DRef, Parent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "Entity3DRef@ Child(int index) const", asMETHOD(Entity3DRef, Child), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "array<Entity3DRef@>@ Children() const", asMETHOD(Entity3DRef, Children), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "int ChildCount() const", asMETHOD(Entity3DRef, ChildCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetParent(Entity3DRef@ parent = null, bool keepWorldPos = true)", asMETHOD(Entity3DRef, SetParent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool AddChild(Entity3DRef@ child, bool keepWorldPos = true)", asMETHOD(Entity3DRef, AddChild), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool RemoveChild(Entity3DRef@ child)", asMETHOD(Entity3DRef, RemoveChild), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool IsAncestorOf(Entity3DRef@ other) const", asMETHOD(Entity3DRef, IsAncestorOf), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool IsDescendantOf(Entity3DRef@ other) const", asMETHOD(Entity3DRef, IsDescendantOf), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetPickable(bool pick = true)", asMETHOD(Entity3DRef, SetPickable), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool IsPickable() const", asMETHOD(Entity3DRef, IsPickable), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "MeshRef@ CurrentMesh() const", asMETHOD(Entity3DRef, CurrentMesh), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool SetCurrentMesh(MeshRef@ mesh, bool addIfNotHere = true)", asMETHOD(Entity3DRef, SetCurrentMesh), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "int MeshCount() const", asMETHOD(Entity3DRef, MeshCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "MeshRef@ Mesh(int index) const", asMETHOD(Entity3DRef, Mesh), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity3DRef", "bool AddMesh(MeshRef@ mesh)", asMETHOD(Entity3DRef, AddMesh), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetPosition(const Vx2DVector &in pos, bool homogeneous = false, Entity2DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity2DRef, SetPosition, (const Vx2DVector &, bool, Entity2DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetPosition(float x, float y, bool homogeneous = false, Entity2DRef@ reference = null, bool keepChildren = false)", asMETHODPR(Entity2DRef, SetPosition, (float, float, bool, Entity2DRef *, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool GetPosition(Vx2DVector &out pos, bool homogeneous = false, Entity2DRef@ reference = null) const", asMETHODPR(Entity2DRef, GetPosition, (Vx2DVector &, bool, Entity2DRef *) const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetSize(const Vx2DVector &in size, bool homogeneous = false, bool keepChildren = false)", asMETHODPR(Entity2DRef, SetSize, (const Vx2DVector &, bool, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetSize(float x, float y, bool homogeneous = false, bool keepChildren = false)", asMETHODPR(Entity2DRef, SetSize, (float, float, bool, bool), bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool GetSize(Vx2DVector &out size, bool homogeneous = false) const", asMETHODPR(Entity2DRef, GetSize, (Vx2DVector &, bool) const, bool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetRect(const VxRect &in rect, bool keepChildren = false)", asMETHOD(Entity2DRef, SetRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool GetRect(VxRect &out rect) const", asMETHOD(Entity2DRef, GetRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetSourceRect(const VxRect &in rect)", asMETHOD(Entity2DRef, SetSourceRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool GetSourceRect(VxRect &out rect) const", asMETHOD(Entity2DRef, GetSourceRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool UseSourceRect(bool use = true)", asMETHOD(Entity2DRef, UseSourceRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool IsUsingSourceRect() const", asMETHOD(Entity2DRef, IsUsingSourceRect), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetMaterial(MaterialRef@ material)", asMETHOD(Entity2DRef, SetMaterial), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "MaterialRef@ Material() const", asMETHOD(Entity2DRef, Material), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetPickable(bool pick = true)", asMETHOD(Entity2DRef, SetPickable), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool IsPickable() const", asMETHOD(Entity2DRef, IsPickable), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetClipToParent(bool clip = true)", asMETHOD(Entity2DRef, SetClipToParent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool IsClipToParent() const", asMETHOD(Entity2DRef, IsClipToParent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "Entity2DRef@ Parent() const", asMETHOD(Entity2DRef, Parent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "Entity2DRef@ Child(int index) const", asMETHOD(Entity2DRef, Child), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "array<Entity2DRef@>@ Children() const", asMETHOD(Entity2DRef, Children), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "int ChildCount() const", asMETHOD(Entity2DRef, ChildCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool SetParent(Entity2DRef@ parent = null)", asMETHOD(Entity2DRef, SetParent), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool IsAncestorOf(Entity2DRef@ other) const", asMETHOD(Entity2DRef, IsAncestorOf), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Entity2DRef", "bool IsDescendantOf(Entity2DRef@ other) const", asMETHOD(Entity2DRef, IsDescendantOf), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("MaterialRef", "TextureRef@ Texture(int slot = 0) const", asMETHOD(MaterialRef, Texture), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("MaterialRef", "bool SetTexture(TextureRef@ texture, int slot = 0)", asMETHOD(MaterialRef, SetTexture), asCALL_THISCALL); assert(r >= 0);
}

void RegisterScriptObjectRefBridge(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterObjectRefType<BehaviorRef>(engine, "BehaviorRef", WithoutLegacyIdentityMethods());
    RegisterObjectRefType<ParamRef>(engine, "ParamRef", WithoutLegacyIdentityMethods());
    RegisterObjectRefType<ParamInRef>(engine, "ParamInRef");
    RegisterObjectRefType<ParamOutRef>(engine, "ParamOutRef");
    RegisterObjectRefType<ParamLocalRef>(engine, "ParamLocalRef");
    RegisterObjectRefType<ParamStructRef>(engine, "ParamStructRef", WithoutIsValidAndDescribe());
    RegisterObjectRefType<ParamOperationRef>(engine, "ParamOperationRef", WithoutIsValidAndDescribe());
    RegisterObjectRefType<BehaviorLinkRef>(engine, "BehaviorLinkRef", WithoutIsValidValidAndDescribe());

    RegisterObjectRefCast<BehaviorRef, ObjectRef>(engine, "BehaviorRef", "ObjectRef");
    RegisterObjectRefCast<ParamRef, ObjectRef>(engine, "ParamRef", "ObjectRef");
    RegisterObjectRefCast<ParamInRef, ObjectRef>(engine, "ParamInRef", "ObjectRef");
    RegisterObjectRefCast<ParamOutRef, ObjectRef>(engine, "ParamOutRef", "ObjectRef");
    RegisterObjectRefCast<ParamLocalRef, ObjectRef>(engine, "ParamLocalRef", "ObjectRef");
    RegisterObjectRefCast<ParamStructRef, ObjectRef>(engine, "ParamStructRef", "ObjectRef");
    RegisterObjectRefCast<ParamOperationRef, ObjectRef>(engine, "ParamOperationRef", "ObjectRef");
    RegisterObjectRefCast<BehaviorLinkRef, ObjectRef>(engine, "BehaviorLinkRef", "ObjectRef");
    RegisterObjectRefCast<ParamInRef, ParamRef>(engine, "ParamInRef", "ParamRef");
    RegisterObjectRefCast<ParamOutRef, ParamRef>(engine, "ParamOutRef", "ParamRef");
    RegisterObjectRefCast<ParamLocalRef, ParamRef>(engine, "ParamLocalRef", "ParamRef");
    RegisterObjectRefCast<ParamStructRef, ParamRef>(engine, "ParamStructRef", "ParamRef");
}
