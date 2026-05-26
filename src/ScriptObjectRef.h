#ifndef CK_SCRIPTOBJECTREF_H
#define CK_SCRIPTOBJECTREF_H

#include <cstdint>
#include <string>

#include <angelscript.h>

#include "CKAll.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptRefCounted.h"

class ScriptBehaviorBridge;

class ObjectRef : public RefCounted {
public:
    ObjectRef(CKContext *context, CKObject *object);
    ObjectRef(CKContext *context, CK_ID id, std::string error = std::string());
    ObjectRef(CKContext *context,
              CK_ID id,
              const ScriptBridgeObjectStamp &stamp,
              std::string error = std::string(),
              std::uintptr_t objectAddress = 0);

    bool IsValid() const;
    bool valid() const;
    std::string Error() const;
    virtual std::string Describe() const;

    CK_ID Id() const;
    CK_ID GetID() const;
    std::string Name() const;
    std::string GetName() const;
    CK_CLASSID ClassId() const;
    bool IsDynamic() const;
    CKObject *Object() const;

    CKContext *Context() const;
    const ScriptBridgeObjectStamp &Stamp() const;
    void SetError(std::string error) const;
    void Invalidate(std::string error = std::string());

protected:
    CKObject *Resolve() const;
    void ResetObject(CKContext *context, CKObject *object, std::string error = std::string());
    void ResetObject(CKContext *context,
                     CK_ID id,
                     const ScriptBridgeObjectStamp &stamp,
                     std::string error = std::string(),
                     std::uintptr_t objectAddress = 0);
    void ClearObject(std::string error = std::string());

private:
    CKContext *m_Context = nullptr;
    CK_ID m_Id = 0;
    ScriptBridgeObjectStamp m_Stamp;
    std::uintptr_t m_ObjectAddress = 0;
    mutable std::string m_Error;
};

class SceneObjectRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKSceneObject *SceneObject() const;
};

class Entity3DRef : public SceneObjectRef {
public:
    using SceneObjectRef::SceneObjectRef;
    CK3dEntity *Entity3D() const;
};

class Entity2DRef : public SceneObjectRef {
public:
    using SceneObjectRef::SceneObjectRef;
    CK2dEntity *Entity2D() const;
};

class MaterialRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKMaterial *Material() const;
};

class TextureRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKTexture *Texture() const;
};

class MeshRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKMesh *Mesh() const;
};

class SceneRef : public SceneObjectRef {
public:
    using SceneObjectRef::SceneObjectRef;
    CKScene *Scene() const;
};

class LevelRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKLevel *Level() const;
};

ObjectRef *MakeScriptObjectRef(CKContext *context,
                               CKObject *object,
                               ScriptBehaviorBridge *bridge = nullptr,
                               CK_ID componentId = 0,
                               const std::string &error = std::string());
ObjectRef *MakeInvalidObjectRef(CKContext *context, const std::string &error);

void RegisterScriptObjectRefCore(asIScriptEngine *engine);
void RegisterScriptObjectRefBridge(asIScriptEngine *engine);

#endif // CK_SCRIPTOBJECTREF_H
