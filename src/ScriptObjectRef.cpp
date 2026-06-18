#include "ScriptObjectRef.h"

#include <cassert>

#include <fmt/format.h>

#include "ScriptBridgeCommon.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptRegistration.h"

namespace {

CKObject *ObjectById(CKContext *context, CK_ID id) {
    return id ? GetCKObjectById(context, id) : nullptr;
}

struct ObjectRefIdentityOptions {
    bool isValid = true;
    bool validProperty = true;
    bool describe = true;
    bool idProperty = true;
    bool nameProperty = true;
    bool object = true;
};

template <typename T>
void RegisterRefCounted(asIScriptEngine *engine, const char *typeName) {
    int r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
    CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);
    CKAS_CHECK_REGISTER(r);
}

template <typename T>
void RegisterObjectRefIdentityMethods(asIScriptEngine *engine,
                                      const char *typeName,
                                      const ObjectRefIdentityOptions &options = {}) {
    int r = 0;
    if (options.isValid) {
        r = engine->RegisterObjectMethod(typeName, "bool IsValid() const", asMETHOD(T, IsValid), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
    if (options.validProperty) {
        r = engine->RegisterObjectMethod(typeName, "bool get_valid() const", asMETHOD(T, valid), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
    r = engine->RegisterObjectMethod(typeName, "string Error() const", asMETHOD(T, Error), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    if (options.describe) {
        r = engine->RegisterObjectMethod(typeName, "string Describe() const", asMETHOD(T, Describe), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
    r = engine->RegisterObjectMethod(typeName, "CK_ID Id() const", asMETHOD(T, Id), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    if (options.idProperty) {
        r = engine->RegisterObjectMethod(typeName, "CK_ID get_id() const", asMETHOD(T, Id), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
    r = engine->RegisterObjectMethod(typeName, "string Name() const", asMETHOD(T, Name), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    if (options.nameProperty) {
        r = engine->RegisterObjectMethod(typeName, "string get_name() const", asMETHOD(T, Name), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
    r = engine->RegisterObjectMethod(typeName, "CK_CLASSID ClassId() const", asMETHOD(T, ClassId), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod(typeName, "bool IsDynamic() const", asMETHOD(T, IsDynamic), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    if (options.object) {
        r = engine->RegisterObjectMethod(typeName, "CKObject@ Object() const", asMETHOD(T, Object), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    }
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
    Derived *derived = dynamic_cast<Derived *>(base);
    if (derived) {
        derived->AddRef();
    }
    return derived;
}

template <typename Derived, typename Base>
Base *ObjectRefUpcast(Derived *derived) {
    Base *base = static_cast<Base *>(derived);
    if (base) {
        base->AddRef();
    }
    return base;
}

template <typename Derived, typename Base>
void RegisterObjectRefCast(asIScriptEngine *engine, const char *derived, const char *base) {
    int r = 0;
    std::string decl = derived;
    decl.append("@ opCast()");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((CheckedObjectRefDowncast<Derived, Base>), (Base *), Derived *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = base;
    decl.append("@ opImplCast()");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((ObjectRefUpcast<Derived, Base>), (Derived *), Base *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = "const ";
    decl.append(derived).append("@ opCast() const");
    r = engine->RegisterObjectMethod(base, decl.c_str(), asFUNCTIONPR((CheckedObjectRefDowncast<Derived, Base>), (Base *), Derived *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);

    decl = "const ";
    decl.append(base).append("@ opImplCast() const");
    r = engine->RegisterObjectMethod(derived, decl.c_str(), asFUNCTIONPR((ObjectRefUpcast<Derived, Base>), (Derived *), Base *), asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
}

template <typename T>
void RegisterObjectRefType(asIScriptEngine *engine,
                           const char *typeName,
                           const ObjectRefIdentityOptions &options = {}) {
    int r = engine->RegisterObjectType(typeName, 0, asOBJ_REF);
    CKAS_CHECK_REGISTER(r);
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
    CKAS_CHECK_REGISTER(r);
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

ObjectRefIdentityOptions WithoutLegacyIdentityAndObjectMethods() {
    ObjectRefIdentityOptions options = WithoutLegacyIdentityMethods();
    options.object = false;
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

ObjectRefIdentityOptions WithoutIsValidValidDescribeAndObject() {
    ObjectRefIdentityOptions options = WithoutIsValidValidAndDescribe();
    options.object = false;
    return options;
}

} // namespace

ObjectRef::ObjectRef(CKContext *context, CKObject *object) {
    ResetObject(object ? object->GetCKContext() : context, object);
}

ObjectRef::ObjectRef(CKContext *context, CK_ID id, std::string error) {
    CKObject *object = ObjectById(context, id);
    ResetObject(context, id, CaptureBridgeObjectStamp(object), std::move(error), ScriptBridgeObjectAddress(object));
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
    if (m_ObjectAddress && ScriptBridgeObjectAddress(object) != m_ObjectAddress) {
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
                ScriptBridgeObjectAddress(object));
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
}

void RegisterScriptObjectRefBridge(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterObjectRefType<BehaviorRef>(engine, "BehaviorRef", WithoutLegacyIdentityAndObjectMethods());
    RegisterObjectRefType<ParamRef>(engine, "ParamRef", WithoutLegacyIdentityMethods());
    RegisterObjectRefType<ParamInRef>(engine, "ParamInRef");
    RegisterObjectRefType<ParamOutRef>(engine, "ParamOutRef");
    RegisterObjectRefType<ParamLocalRef>(engine, "ParamLocalRef");
    RegisterObjectRefType<ParamStructRef>(engine, "ParamStructRef", WithoutIsValidAndDescribe());
    RegisterObjectRefType<ParamOperationRef>(engine, "ParamOperationRef", WithoutIsValidAndDescribe());
    RegisterObjectRefType<BehaviorLinkRef>(engine, "BehaviorLinkRef", WithoutIsValidValidDescribeAndObject());

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
