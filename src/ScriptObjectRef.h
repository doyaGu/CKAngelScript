#ifndef CK_SCRIPTOBJECTREF_H
#define CK_SCRIPTOBJECTREF_H

#include <cstdint>
#include <string>

#include <angelscript.h>

#include "CKAll.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptRefCounted.h"

class ScriptBehaviorBridge;
class CScriptArray;
class SceneRef;
class MaterialRef;
class TextureRef;
class MeshRef;

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
    bool SetName(const std::string &name, bool shared = false);
    bool SetDynamic(bool dynamic = true);
    bool Show(CK_OBJECT_SHOWOPTION show = CKSHOW);
    bool IsVisible() const;
    CKDWORD ObjectFlags() const;
    bool ModifyObjectFlags(CKDWORD add, CKDWORD remove = 0);

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
    bool IsInScene(SceneRef *scene) const;
    bool IsInCurrentScene() const;
};

class Entity3DRef : public SceneObjectRef {
public:
    using SceneObjectRef::SceneObjectRef;
    CK3dEntity *Entity3D() const;

    bool SetPosition(const VxVector &pos, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool SetPosition(float x, float y, float z, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool GetPosition(VxVector &pos, Entity3DRef *reference = nullptr) const;
    bool Translate(const VxVector &delta, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool Translate(float x, float y, float z, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool SetQuaternion(const VxQuaternion &quat,
                       Entity3DRef *reference = nullptr,
                       bool keepChildren = false,
                       bool keepScale = false);
    bool GetQuaternion(VxQuaternion &quat, Entity3DRef *reference = nullptr) const;
    bool SetScale(const VxVector &scale, bool keepChildren = false, bool local = true);
    bool SetScale(float x, float y, float z, bool keepChildren = false, bool local = true);
    bool GetScale(VxVector &scale, bool local = true) const;
    bool LookAt(const VxVector &pos, Entity3DRef *reference = nullptr, bool keepChildren = false);

    Entity3DRef *Parent() const;
    Entity3DRef *Child(int index) const;
    CScriptArray *Children() const;
    int ChildCount() const;
    bool SetParent(Entity3DRef *parent = nullptr, bool keepWorldPos = true);
    bool AddChild(Entity3DRef *child, bool keepWorldPos = true);
    bool RemoveChild(Entity3DRef *child);
    bool IsAncestorOf(Entity3DRef *other) const;
    bool IsDescendantOf(Entity3DRef *other) const;
    bool SetPickable(bool pick = true);
    bool IsPickable() const;
    MeshRef *CurrentMesh() const;
    bool SetCurrentMesh(MeshRef *mesh, bool addIfNotHere = true);
    int MeshCount() const;
    MeshRef *Mesh(int index) const;
    bool AddMesh(MeshRef *mesh);
    bool SetOrientation(const VxVector &direction,
                        const VxVector &up,
                        const VxVector &right,
                        Entity3DRef *reference = nullptr,
                        bool keepChildren = false);
    bool GetOrientation(VxVector &direction,
                        VxVector &up,
                        VxVector &right,
                        Entity3DRef *reference = nullptr) const;
    bool SetDirection(const VxVector &direction, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool GetDirection(VxVector &direction, Entity3DRef *reference = nullptr) const;
    bool SetUp(const VxVector &up, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool GetUp(VxVector &up, Entity3DRef *reference = nullptr) const;
    bool SetRight(const VxVector &right, Entity3DRef *reference = nullptr, bool keepChildren = false);
    bool GetRight(VxVector &right, Entity3DRef *reference = nullptr) const;
    bool GetBoundingBox(VxBbox &bbox, bool local = false) const;
    bool SetBoundingBox(const VxBbox &bbox, bool local = false);
    float Radius() const;
    MaterialRef *Material(int meshIndex = 0, int materialIndex = 0) const;
};

class Entity2DRef : public SceneObjectRef {
public:
    using SceneObjectRef::SceneObjectRef;
    CK2dEntity *Entity2D() const;

    Entity2DRef *Parent() const;
    Entity2DRef *Child(int index) const;
    CScriptArray *Children() const;
    int ChildCount() const;
    bool SetParent(Entity2DRef *parent = nullptr);
    bool IsAncestorOf(Entity2DRef *other) const;
    bool IsDescendantOf(Entity2DRef *other) const;
    bool SetPosition(const Vx2DVector &pos,
                     bool homogeneous = false,
                     Entity2DRef *reference = nullptr,
                     bool keepChildren = false);
    bool SetPosition(float x,
                     float y,
                     bool homogeneous = false,
                     Entity2DRef *reference = nullptr,
                     bool keepChildren = false);
    bool GetPosition(Vx2DVector &pos, bool homogeneous = false, Entity2DRef *reference = nullptr) const;
    bool SetSize(const Vx2DVector &size, bool homogeneous = false, bool keepChildren = false);
    bool SetSize(float x, float y, bool homogeneous = false, bool keepChildren = false);
    bool GetSize(Vx2DVector &size, bool homogeneous = false) const;
    bool SetRect(const VxRect &rect, bool keepChildren = false);
    bool GetRect(VxRect &rect) const;
    bool SetSourceRect(const VxRect &rect);
    bool GetSourceRect(VxRect &rect) const;
    bool UseSourceRect(bool use = true);
    bool IsUsingSourceRect() const;
    bool SetMaterial(MaterialRef *material);
    MaterialRef *Material() const;
    bool SetPickable(bool pick = true);
    bool IsPickable() const;
    bool SetClipToParent(bool clip = true);
    bool IsClipToParent() const;
    bool SetBackground(bool background = true);
    bool IsBackground() const;
    bool EnableRatioOffset(bool ratio = true);
    bool IsRatioOffset() const;
    bool EnableClipToCamera(bool clip = true);
    bool IsClippedToCamera() const;
    CKDWORD Flags() const;
    bool SetFlags(CKDWORD flags);
    bool ModifyFlags(CKDWORD add, CKDWORD remove = 0);
    bool SetExtents(const VxRect &sourceRect, const VxRect &rect);
    bool GetExtents(VxRect &sourceRect, VxRect &rect) const;
};

class MaterialRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    CKMaterial *Material() const;
    TextureRef *Texture(int slot = 0) const;
    bool SetTexture(TextureRef *texture, int slot = 0);
    VxColor Ambient() const;
    bool SetAmbient(const VxColor &color);
    VxColor Diffuse() const;
    bool SetDiffuse(const VxColor &color);
    VxColor Specular() const;
    bool SetSpecular(const VxColor &color);
    VxColor Emissive() const;
    bool SetEmissive(const VxColor &color);
    float Power() const;
    bool SetPower(float power);
    VXTEXTURE_BLENDMODE TextureBlendMode() const;
    bool SetTextureBlendMode(VXTEXTURE_BLENDMODE mode);
    VXBLEND_MODE SourceBlend() const;
    bool SetSourceBlend(VXBLEND_MODE mode);
    VXBLEND_MODE DestBlend() const;
    bool SetDestBlend(VXBLEND_MODE mode);
    bool AlphaBlendEnabled() const;
    bool EnableAlphaBlend(bool blend = true);
    VXCMPFUNC AlphaFunc() const;
    bool SetAlphaFunc(VXCMPFUNC func = VXCMP_ALWAYS);
    CKBYTE AlphaRef() const;
    bool SetAlphaRef(CKBYTE alphaRef = 0);
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
