#include "ScriptCKObjects.h"

// Misc
#include "CKMaterial.h"
#include "CKTexture.h"
#include "CKRenderContext.h"
#include "CKSynchroObject.h"
#include "CKInterfaceObjectManager.h"

// Parameters
#include "CKParameter.h"
#include "CKParameterIn.h"
#include "CKParameterOut.h"
#include "CKParameterLocal.h"
#include "CKParameterOperation.h"

// Behaviors
#include "CKBehaviorIO.h"
#include "CKBehaviorLink.h"
#include "CKBehaviorPrototype.h"
#include "CKBehavior.h"
#include "CKMessage.h"

// Level/Scene/Place/Layer
#include "CKLevel.h"
#include "CKPlace.h"
#include "CKGroup.h"
#include "CKScene.h"
#include "CKLayer.h"

// Save/load
#include "CKStateChunk.h"
#include "CKFile.h"

// Sound
#include "CKSound.h"
#include "CKWaveSound.h"
#include "CKMidiSound.h"
#include "CKSoundReader.h"

// Curves
#include "CKCurve.h"
#include "CKCurvePoint.h"

// Character and Animation
#include "CKAnimation.h"
#include "CKKeyedAnimation.h"
#include "CKObjectAnimation.h"
#include "CKKinematicChain.h"
#include "CKCharacter.h"

// Base Objects
#include "CKObject.h"
#include "CKSceneObject.h"
#include "CKRenderObject.h"
#include "CKBeObject.h"
#include "CKDependencies.h"

// 2d Objects
#include "CK2dEntity.h"
#include "CKSprite.h"
#include "CKSpriteText.h"

// 3d Objects
#include "CKMesh.h"
#include "CKPatchMesh.h"
#include "CK3dEntity.h"
#include "CKCamera.h"
#include "CKTargetCamera.h"
#include "CKSprite3D.h"
#include "CKLight.h"
#include "CKTargetLight.h"
#include "CK3dObject.h"
#include "CKBodyPart.h"

// Containers
#include "CKDataArray.h"
#include "CKDebugContext.h"
#include "CKMemoryPool.h"

#include "ScriptUtils.h"

template <typename T>
static void RegisterCKObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "void SetName(const string &in, bool = false)", asMETHODPR(T, SetName, (CKSTRING, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint GetAppData()", asMETHODPR(T, GetAppData, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAppData(uint)", asMETHODPR(T, SetAppData, (void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Show(CK_OBJECT_SHOWOPTION = CKSHOW)", asMETHODPR(T, Show, (CK_OBJECT_SHOWOPTION), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsHiddenByParent()", asMETHODPR(T, IsHiddenByParent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CanBeHide()", asMETHODPR(T, CanBeHide, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsVisible()", asMETHODPR(T, IsVisible, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsHierarchicallyHide()", asMETHODPR(T, IsHierarchicallyHide, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKContext@ GetCKContext()", asMETHODPR(T, GetCKContext, (), CKContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_ID GetID()", asMETHODPR(T, GetID, (), CK_ID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "string GetName()", asMETHODPR(T, GetName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetObjectFlags()", asMETHODPR(T, GetObjectFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsDynamic()", asMETHODPR(T, IsDynamic, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeDeleted()", asMETHODPR(T, IsToBeDeleted, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ModifyObjectFlags(CKDWORD, CKDWORD)", asMETHODPR(T, ModifyObjectFlags, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@, CKDWORD, CKDependencies &in)", asMETHODPR(T, CKDestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID, CKDWORD, CKDependencies &in)", asMETHODPR(T, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(CK_ID@, int, CKDWORD, CKDependencies &in)", asMETHODPR(T, CKDestroyObjects, (CK_ID*, int, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID)", asMETHODPR(T, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsUpToDate()", asMETHODPR(T, IsUpToDate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPrivate()", asMETHODPR(T, IsPrivate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsNotToBeSaved()", asMETHODPR(T, IsNotToBeSaved, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInterfaceObj()", asMETHODPR(T, IsInterfaceObj, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreSave(CKFile@, CKDWORD)", asMETHODPR(T, PreSave, (CKFile*, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ Save(CKFile@, CKDWORD)", asMETHODPR(T, Save, (CKFile*, CKDWORD), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Load(CKStateChunk@, CKFile@)", asMETHODPR(T, Load, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PostLoad()", asMETHODPR(T, PostLoad, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreDelete()", asMETHODPR(T, PreDelete, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPreDeletion()", asMETHODPR(T, CheckPreDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPostDeletion()", asMETHODPR(T, CheckPostDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetMemoryOccupation()", asMETHODPR(T, GetMemoryOccupation, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsObjectUsed(CKObject@, CK_CLASSID)", asMETHODPR(T, IsObjectUsed, (CKObject *, CK_CLASSID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PrepareDependencies(CKDependenciesContext &in)", asMETHODPR(T, PrepareDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemapDependencies(CKDependenciesContext &in)", asMETHODPR(T, RemapDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Copy(CKObject&in, CKDependenciesContext &in)", asMETHODPR(T, Copy, (CKObject&, CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKObject") != 0) {
        RegisterClassRefCast<T, CKObject>(engine, name, "CKObject");
    }
}

template <>
static void RegisterCKObjectMembers<CKScene>(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "void SetName(const string &in, bool = false)", asMETHODPR(CKScene, SetName, (CKSTRING, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint GetAppData()", asMETHODPR(CKScene, GetAppData, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAppData(uint)", asMETHODPR(CKScene, SetAppData, (void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Show(CK_OBJECT_SHOWOPTION = CKSHOW)", asMETHODPR(CKScene, Show, (CK_OBJECT_SHOWOPTION), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsHiddenByParent()", asMETHODPR(CKScene, IsHiddenByParent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CanBeHide()", asMETHODPR(CKScene, CanBeHide, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsVisible()", asMETHODPR(CKScene, IsVisible, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsHierarchicallyHide()", asMETHODPR(CKScene, IsHierarchicallyHide, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKContext@ GetCKContext()", asMETHODPR(CKScene, GetCKContext, (), CKContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_ID GetID()", asMETHODPR(CKScene, GetID, (), CK_ID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "string GetName()", asMETHODPR(CKScene, GetName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetObjectFlags()", asMETHODPR(CKObject, GetObjectFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsDynamic()", asMETHODPR(CKScene, IsDynamic, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeDeleted()", asMETHODPR(CKScene, IsToBeDeleted, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ModifyObjectFlags(CKDWORD, CKDWORD)", asMETHODPR(CKObject, ModifyObjectFlags, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@, CKDWORD, CKDependencies &in)", asMETHODPR(CKScene, CKDestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID, CKDWORD, CKDependencies &in)", asMETHODPR(CKScene, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(CK_ID@, int, CKDWORD = 0, CKDependencies@ = null)", asMETHODPR(CKScene, CKDestroyObjects, (CK_ID*, int, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID)", asMETHODPR(CKScene, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsUpToDate()", asMETHODPR(CKScene, IsUpToDate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPrivate()", asMETHODPR(CKScene, IsPrivate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsNotToBeSaved()", asMETHODPR(CKScene, IsNotToBeSaved, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInterfaceObj()", asMETHODPR(CKScene, IsInterfaceObj, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreSave(CKFile@, CKDWORD)", asMETHODPR(CKScene, PreSave, (CKFile*, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ Save(CKFile@, CKDWORD)", asMETHODPR(CKScene, Save, (CKFile*, CKDWORD), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Load(CKStateChunk@, CKFile@)", asMETHODPR(CKScene, Load, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PostLoad()", asMETHODPR(CKScene, PostLoad, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreDelete()", asMETHODPR(CKScene, PreDelete, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPreDeletion()", asMETHODPR(CKScene, CheckPreDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPostDeletion()", asMETHODPR(CKScene, CheckPostDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetMemoryOccupation()", asMETHODPR(CKScene, GetMemoryOccupation, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsObjectUsed(CKObject@, CK_CLASSID)", asMETHODPR(CKScene, IsObjectUsed, (CKObject *, CK_CLASSID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PrepareDependencies(CKDependenciesContext &in)", asMETHODPR(CKScene, PrepareDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemapDependencies(CKDependenciesContext &in)", asMETHODPR(CKScene, RemapDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Copy(CKObject&in, CKDependenciesContext &in)", asMETHODPR(CKScene, Copy, (CKObject&, CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    RegisterClassRefCast<CKScene, CKObject>(engine, name, "CKObject");
}

void RegisterCKObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKObjectMembers<CKObject>(engine, "CKObject");
}

void RegisterCKInterfaceObjectManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKObjectMembers<CKInterfaceObjectManager>(engine, "CKInterfaceObjectManager");
}

template <typename T>
static void RegisterCKParameterInMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "CKERROR GetValue(NativePointer)", asMETHODPR(T, GetValue, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetReadDataPtr()", asMETHODPR(T, GetReadDataPtr, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterIn@ GetSharedSource()", asMETHODPR(T, GetSharedSource, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetRealSource()", asMETHODPR(T, GetRealSource, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetDirectSource()", asMETHODPR(T, GetDirectSource, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SetDirectSource(CKParameter@)", asMETHODPR(T, SetDirectSource, (CKParameter*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR ShareSourceWith(CKParameterIn@)", asMETHODPR(T, ShareSourceWith, (CKParameterIn*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetType(CKParameterType, bool, const string &in)", asMETHODPR(T, SetType, (CKParameterType, CKBOOL, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetGUID(CKGUID, bool, const string &in)", asMETHODPR(T, SetGUID, (CKGUID, CKBOOL, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterType GetType()", asMETHODPR(T, GetType, (), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKGUID GetGUID()", asMETHODPR(T, GetGUID, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetOwner(CKObject@)", asMETHODPR(T, SetOwner, (CKObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ GetOwner()", asMETHODPR(T, GetOwner, (), CKObject*), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKParameterIn") != 0) {
        RegisterClassRefCast<T, CKParameterIn>(engine, name, "CKParameterIn");
    }
}

void RegisterCKParameterIn(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKParameterInMembers<CKParameterIn>(engine, "CKParameterIn");
}

template <typename T>
static void RegisterCKParameterMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "CKObject@ GetValueObject(bool = true)", asMETHODPR(T, GetValueObject, (CKBOOL), CKObject*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "CKERROR GetValue(?&out, bool = true)", asMETHODPR(T, GetValue, (void*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "CKERROR SetValue(?&in, int = 0)", asMETHODPR(T, SetValue, (const void*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CopyValue(CKParameter@, bool = true)", asMETHODPR(T, CopyValue, (CKParameter*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsCompatibleWith(CKParameter@)", asMETHODPR(T, IsCompatibleWith, (CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetDataSize()", asMETHODPR(T, GetDataSize, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetReadDataPtr(bool = true)", asMETHODPR(T, GetReadDataPtr, (CKBOOL), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetWriteDataPtr()", asMETHODPR(T, GetWriteDataPtr, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SetStringValue(const string &in)", asMETHODPR(T, SetStringValue, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetStringValue(const string &in, bool = true)", asMETHODPR(T, GetStringValue, (CKSTRING, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterType GetType()", asMETHODPR(T, GetType, (), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetType(CKParameterType)", asMETHODPR(T, SetType, (CKParameterType), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKGUID GetGUID()", asMETHODPR(T, GetGUID, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetGUID(CKGUID)", asMETHODPR(T, SetGUID, (CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_CLASSID GetParameterClassID()", asMETHODPR(T, GetParameterClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetOwner(CKObject@)", asMETHODPR(T, SetOwner, (CKObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ GetOwner()", asMETHODPR(T, GetOwner, (), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Enable(bool = true)", asMETHODPR(T, Enable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsEnabled()", asMETHODPR(T, IsEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKParameter") != 0) {
        RegisterClassRefCast<T, CKParameter>(engine, name, "CKParameter");
    }
}

void RegisterCKParameter(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKParameterMembers<CKParameter>(engine, "CKParameter");
}

template <typename T>
static void RegisterCKParameterOutMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKParameterMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "void DataChanged()", asMETHODPR(T, DataChanged, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddDestination(CKParameter@, bool = true)", asMETHODPR(T, AddDestination, (CKParameter*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveDestination(CKParameter@)", asMETHODPR(T, RemoveDestination, (CKParameter*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetDestinationCount()", asMETHODPR(T, GetDestinationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetDestination(int)", asMETHODPR(T, GetDestination, (int), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllDestinations()", asMETHODPR(T, RemoveAllDestinations, (), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKParameterOut") != 0) {
        RegisterClassRefCast<T, CKParameterOut>(engine, name, "CKParameterOut");
    }
}

void RegisterCKParameterOut(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKParameterOutMembers<CKParameterOut>(engine, "CKParameterOut");
}

template <typename T>
static void RegisterCKParameterLocalMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKParameterMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "void SetAsMyselfParameter(bool)", asMETHODPR(T, SetAsMyselfParameter, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsMyselfParameter()", asMETHODPR(T, IsMyselfParameter, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKParameterLocal") != 0) {
        RegisterClassRefCast<T, CKParameterLocal>(engine, name, "CKParameterLocal");
    }
}

void RegisterCKParameterLocal(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKParameterLocalMembers<CKParameterLocal>(engine, "CKParameterLocal");
}

template <typename T>
static void RegisterCKParameterOperationMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "CKParameterIn@ GetInParameter1()", asMETHODPR(T, GetInParameter1, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterIn@ GetInParameter2()", asMETHODPR(T, GetInParameter2, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetOutParameter()", asMETHODPR(T, GetOutParameter, (), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ GetOwner()", asMETHODPR(T, GetOwner, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetOwner(CKBehavior@)", asMETHODPR(T, SetOwner, (CKBehavior*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR DoOperation()", asMETHODPR(T, DoOperation, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKGUID GetOperationGuid()", asMETHODPR(T, GetOperationGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Reconstruct(const string &in, CKGUID, CKGUID, CKGUID, CKGUID)", asMETHODPR(T, Reconstruct, (CKSTRING, CKGUID, CKGUID, CKGUID, CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetOperationFunction()", asMETHODPR(T, GetOperationFunction, (), CK_PARAMETEROPERATION), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKParameterOperation") != 0) {
        RegisterClassRefCast<T, CKParameterOperation>(engine, name, "CKParameterOperation");
    }
}

void RegisterCKParameterOperation(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKParameterOperationMembers<CKParameterOperation>(engine, "CKParameterOperation");
}

void RegisterCKBehaviorLink(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKBehaviorLink>(engine, "CKBehaviorLink");

    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKERROR SetOutBehaviorIO(CKBehaviorIO@)", asMETHODPR(CKBehaviorLink, SetOutBehaviorIO, (CKBehaviorIO*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKERROR SetInBehaviorIO(CKBehaviorIO@)", asMETHODPR(CKBehaviorLink, SetInBehaviorIO, (CKBehaviorIO*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKBehaviorIO@ GetOutBehaviorIO()", asMETHODPR(CKBehaviorLink, GetOutBehaviorIO, (), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKBehaviorIO@ GetInBehaviorIO()", asMETHODPR(CKBehaviorLink, GetInBehaviorIO, (), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "int GetActivationDelay()", asMETHODPR(CKBehaviorLink, GetActivationDelay, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetActivationDelay(int)", asMETHODPR(CKBehaviorLink, SetActivationDelay, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void ResetActivationDelay()", asMETHODPR(CKBehaviorLink, ResetActivationDelay, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetInitialActivationDelay(int)", asMETHODPR(CKBehaviorLink, SetInitialActivationDelay, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "int GetInitialActivationDelay()", asMETHODPR(CKBehaviorLink, GetInitialActivationDelay, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKDWORD GetFlags()", asMETHODPR(CKBehaviorLink, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetFlags(CKDWORD)", asMETHODPR(CKBehaviorLink, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKBehaviorIO(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKBehaviorIO>(engine, "CKBehaviorIO");

    r = engine->RegisterObjectMethod("CKBehaviorIO", "void SetType(int)", asMETHODPR(CKBehaviorIO, SetType, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "int GetType()", asMETHODPR(CKBehaviorIO, GetType, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "void Activate(bool = true)", asMETHODPR(CKBehaviorIO, Activate, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "bool IsActive()", asMETHODPR(CKBehaviorIO, IsActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "CKBehavior@ GetOwner()", asMETHODPR(CKBehaviorIO, GetOwner, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "void SetOwner(CKBehavior@)", asMETHODPR(CKBehaviorIO, SetOwner, (CKBehavior*), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKRenderContext(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKRenderContext>(engine, "CKRenderContext");

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddObject(CKRenderObject@)", asMETHODPR(CKRenderContext, AddObject, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddObjectWithHierarchy(CKRenderObject@)", asMETHODPR(CKRenderContext, AddObjectWithHierarchy, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RemoveObject(CKRenderObject@)", asMETHODPR(CKRenderContext, RemoveObject, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool IsObjectAttached(CKRenderObject@)", asMETHODPR(CKRenderContext, IsObjectAttached, (CKRenderObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "const XObjectArray& Compute3dRootObjects()", asMETHODPR(CKRenderContext, Compute3dRootObjects, (), const XObjectArray&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const XObjectArray& Compute2dRootObjects()", asMETHODPR(CKRenderContext, Compute2dRootObjects, (), const XObjectArray&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CK2dEntity@ Get2dRoot(bool)", asMETHODPR(CKRenderContext, Get2dRoot, (CKBOOL), CK2dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void DetachAll()", asMETHODPR(CKRenderContext, DetachAll, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void ForceCameraSettingsUpdate()", asMETHODPR(CKRenderContext, ForceCameraSettingsUpdate, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void PrepareCameras(CK_RENDER_FLAGS = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, PrepareCameras, (CK_RENDER_FLAGS), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Clear(CK_RENDER_FLAGS = CK_RENDER_USECURRENTSETTINGS, CKDWORD = 0)", asMETHODPR(CKRenderContext, Clear, (CK_RENDER_FLAGS, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR DrawScene(CK_RENDER_FLAGS = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, DrawScene, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR BackToFront(CK_RENDER_FLAGS = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, BackToFront, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Render(CK_RENDER_FLAGS = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, Render, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPreRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(CKRenderContext, AddPreRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePreRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePreRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostRenderCallBack(NativePointer, NativePointer, bool = false, bool = false)", asMETHODPR(CKRenderContext, AddPostRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePostRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostSpriteRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(CKRenderContext, AddPostSpriteRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostSpriteRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePostSpriteRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDrawPrimitiveData &GetDrawPrimitiveStructure(CKRST_DPFLAGS, int)", asMETHODPR(CKRenderContext, GetDrawPrimitiveStructure, (CKRST_DPFLAGS, int), VxDrawPrimitiveData*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKWORD &GetDrawPrimitiveIndices(int)", asMETHODPR(CKRenderContext, GetDrawPrimitiveIndices, (int), CKWORD*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void Transform(VxVector &out, const VxVector &in, CK3dEntity@ = null)", asMETHODPR(CKRenderContext, Transform, (VxVector*, VxVector*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void TransformVertices(int, VxTransformData &inout, CK3dEntity@ = null)", asMETHODPR(CKRenderContext, TransformVertices, (int, VxTransformData*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR GoFullScreen(int = 640, int = 480, int = -1, int = 0, int = 0)", asMETHODPR(CKRenderContext, GoFullScreen, (int, int, int, int, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR StopFullScreen()", asMETHODPR(CKRenderContext, StopFullScreen, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool IsFullScreen()", asMETHODPR(CKRenderContext, IsFullScreen, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int GetDriverIndex()", asMETHODPR(CKRenderContext, GetDriverIndex, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool ChangeDriver(int)", asMETHODPR(CKRenderContext, ChangeDriver, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "WIN_HANDLE GetWindowHandle()", asMETHODPR(CKRenderContext, GetWindowHandle, (), WIN_HANDLE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void ScreenToClient(Vx2DVector &out)", asMETHODPR(CKRenderContext, ScreenToClient, (Vx2DVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void ClientToScreen(Vx2DVector &out)", asMETHODPR(CKRenderContext, ClientToScreen, (Vx2DVector*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR SetWindowRect(const VxRect &in, CKDWORD = 0)", asMETHODPR(CKRenderContext, SetWindowRect, (VxRect&, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetWindowRect(VxRect &out, bool = false)", asMETHODPR(CKRenderContext, GetWindowRect, (VxRect&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int GetHeight()", asMETHODPR(CKRenderContext, GetHeight, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int GetWidth()", asMETHODPR(CKRenderContext, GetWidth, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Resize(int = 0, int = 0, int = 0, int = 0, CKDWORD = 0)", asMETHODPR(CKRenderContext, Resize, (int, int, int, int, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetViewRect(const VxRect &in)", asMETHODPR(CKRenderContext, SetViewRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetViewRect(VxRect &out)", asMETHODPR(CKRenderContext, GetViewRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VX_PIXELFORMAT GetPixelFormat(int &in, int &in, int &in)", asMETHODPR(CKRenderContext, GetPixelFormat, (int*, int*, int*), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetState(VXRENDERSTATETYPE, CKDWORD)", asMETHODPR(CKRenderContext, SetState, (VXRENDERSTATETYPE, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetState(VXRENDERSTATETYPE)", asMETHODPR(CKRenderContext, GetState, (VXRENDERSTATETYPE), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetTexture(CKTexture@, bool = 0, int = 0)", asMETHODPR(CKRenderContext, SetTexture, (CKTexture*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetTextureStageState(CKRST_TEXTURESTAGESTATETYPE, CKDWORD, int = 0)", asMETHODPR(CKRenderContext, SetTextureStageState, (CKRST_TEXTURESTAGESTATETYPE, CKDWORD, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKRenderContext", "CKRasterizerContext@ GetRasterizerContext()", asMETHODPR(CKRenderContext, GetRasterizerContext, (), CKRasterizerContext*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetClearBackground(bool = true)", asMETHODPR(CKRenderContext, SetClearBackground, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetClearBackground()", asMETHODPR(CKRenderContext, GetClearBackground, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetClearZBuffer(bool = true)", asMETHODPR(CKRenderContext, SetClearZBuffer, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetClearZBuffer()", asMETHODPR(CKRenderContext, GetClearZBuffer, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetGlobalRenderMode(VxShadeType &out, bool &out, bool &out)", asMETHODPR(CKRenderContext, GetGlobalRenderMode, (VxShadeType*, CKBOOL*, CKBOOL*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetGlobalRenderMode(VxShadeType = GouraudShading, bool = true, bool = false)", asMETHODPR(CKRenderContext, SetGlobalRenderMode, (VxShadeType, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentRenderOptions(CKDWORD)", asMETHODPR(CKRenderContext, SetCurrentRenderOptions, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetCurrentRenderOptions()", asMETHODPR(CKRenderContext, GetCurrentRenderOptions, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void ChangeCurrentRenderOptions(CKDWORD, CKDWORD)", asMETHODPR(CKRenderContext, ChangeCurrentRenderOptions, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentExtents(const VxRect &in)", asMETHODPR(CKRenderContext, SetCurrentExtents, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetCurrentExtents(VxRect &out)", asMETHODPR(CKRenderContext, GetCurrentExtents, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetAmbientLight(float, float, float)", asMETHODPR(CKRenderContext, SetAmbientLight, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetAmbientLight(CKDWORD)", asMETHODPR(CKRenderContext, SetAmbientLight, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetAmbientLight()", asMETHODPR(CKRenderContext, GetAmbientLight, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogMode(VXFOG_MODE)", asMETHODPR(CKRenderContext, SetFogMode, (VXFOG_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogStart(float)", asMETHODPR(CKRenderContext, SetFogStart, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogEnd(float)", asMETHODPR(CKRenderContext, SetFogEnd, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogDensity(float)", asMETHODPR(CKRenderContext, SetFogDensity, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogColor(CKDWORD)", asMETHODPR(CKRenderContext, SetFogColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VXFOG_MODE GetFogMode()", asMETHODPR(CKRenderContext, GetFogMode, (), VXFOG_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetFogColor()", asMETHODPR(CKRenderContext, GetFogColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogStart()", asMETHODPR(CKRenderContext, GetFogStart, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogEnd()", asMETHODPR(CKRenderContext, GetFogEnd, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogDensity()", asMETHODPR(CKRenderContext, GetFogDensity, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool DrawPrimitive(VXPRIMITIVETYPE, NativePointer, int, VxDrawPrimitiveData &in)", asMETHODPR(CKRenderContext, DrawPrimitive, (VXPRIMITIVETYPE, CKWORD*, int, VxDrawPrimitiveData*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetWorldTransformationMatrix(const VxMatrix &in)", asMETHODPR(CKRenderContext, SetWorldTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetProjectionTransformationMatrix(const VxMatrix &in)", asMETHODPR(CKRenderContext, SetProjectionTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetViewTransformationMatrix(const VxMatrix &in)", asMETHODPR(CKRenderContext, SetViewTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetWorldTransformationMatrix()", asMETHODPR(CKRenderContext, GetWorldTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetProjectionTransformationMatrix()", asMETHODPR(CKRenderContext, GetProjectionTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetViewTransformationMatrix()", asMETHODPR(CKRenderContext, GetViewTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetUserClipPlane(CKDWORD, const VxPlane &in)", asMETHODPR(CKRenderContext, SetUserClipPlane, (CKDWORD, const VxPlane&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetUserClipPlane(CKDWORD, VxPlane &out)", asMETHODPR(CKRenderContext, GetUserClipPlane, (CKDWORD, VxPlane&), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKRenderObject@ Pick(int, int, CKPICKRESULT &out, bool = false)", asMETHODPR(CKRenderContext, Pick, (int, int, CKPICKRESULT*, CKBOOL), CKRenderObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKRenderObject@ Pick(CKPOINT, CKPICKRESULT &out, bool = false)", asMETHODPR(CKRenderContext, Pick, (CKPOINT, CKPICKRESULT*, CKBOOL), CKRenderObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR RectPick(const VxRect &in, XObjectPointerArray &out, bool = true)", asMETHODPR(CKRenderContext, RectPick, (const VxRect&, XObjectPointerArray&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AttachViewpointToCamera(CKCamera@)", asMETHODPR(CKRenderContext, AttachViewpointToCamera, (CKCamera*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void DetachViewpointFromCamera()", asMETHODPR(CKRenderContext, DetachViewpointFromCamera, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKCamera@ GetAttachedCamera()", asMETHODPR(CKRenderContext, GetAttachedCamera, (), CKCamera*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CK3dEntity@ GetViewpoint()", asMETHODPR(CKRenderContext, GetViewpoint, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKMaterial@ GetBackgroundMaterial()", asMETHODPR(CKRenderContext, GetBackgroundMaterial, (), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetBoundingBox(VxBbox &out)", asMETHODPR(CKRenderContext, GetBoundingBox, (VxBbox*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetStats(VxStats &out)", asMETHODPR(CKRenderContext, GetStats, (VxStats*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentMaterial(CKMaterial@, bool = true)", asMETHODPR(CKRenderContext, SetCurrentMaterial, (CKMaterial*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void Activate(bool = true)", asMETHODPR(CKRenderContext, Activate, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int DumpToMemory(const VxRect &in, VXBUFFER_TYPE, VxImageDescEx &out)", asMETHODPR(CKRenderContext, DumpToMemory, (const VxRect*, VXBUFFER_TYPE, VxImageDescEx&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int CopyToVideo(const VxRect &in, VXBUFFER_TYPE, VxImageDescEx &out)", asMETHODPR(CKRenderContext, CopyToVideo, (const VxRect*, VXBUFFER_TYPE, VxImageDescEx&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR DumpToFile(const string &in, const VxRect &in, VXBUFFER_TYPE)", asMETHODPR(CKRenderContext, DumpToFile, (CKSTRING, const VxRect*, VXBUFFER_TYPE), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDirectXData &GetDirectXInfo()", asMETHODPR(CKRenderContext, GetDirectXInfo, (), VxDirectXData*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void WarnEnterThread()", asMETHODPR(CKRenderContext, WarnEnterThread, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void WarnExitThread()", asMETHODPR(CKRenderContext, WarnExitThread, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CK2dEntity@ Pick2D(const Vx2DVector &in)", asMETHODPR(CKRenderContext, Pick2D, (const Vx2DVector&), CK2dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetRenderTarget(CKTexture@, int = 0)", asMETHODPR(CKRenderContext, SetRenderTarget, (CKTexture*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddRemoveSequence(CKBOOL)", asMETHODPR(CKRenderContext, AddRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetTransparentMode(CKBOOL)", asMETHODPR(CKRenderContext, SetTransparentMode, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddDirtyRect(const CKRECT &in)", asMETHODPR(CKRenderContext, AddDirtyRect, (CKRECT*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RestoreScreenBackup()", asMETHODPR(CKRenderContext, RestoreScreenBackup, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetStencilFreeMask()", asMETHODPR(CKRenderContext, GetStencilFreeMask, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void UsedStencilBits(CKDWORD)", asMETHODPR(CKRenderContext, UsedStencilBits, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int GetFirstFreeStencilBits()", asMETHODPR(CKRenderContext, GetFirstFreeStencilBits, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDrawPrimitiveData &LockCurrentVB(CKDWORD)", asMETHODPR(CKRenderContext, LockCurrentVB, (CKDWORD), VxDrawPrimitiveData*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool ReleaseCurrentVB()", asMETHODPR(CKRenderContext, ReleaseCurrentVB, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetTextureMatrix(const VxMatrix &in, int = 0)", asMETHODPR(CKRenderContext, SetTextureMatrix, (const VxMatrix&, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetStereoParameters(float, float)", asMETHODPR(CKRenderContext, SetStereoParameters, (float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetStereoParameters(float &out, float &out)", asMETHODPR(CKRenderContext, GetStereoParameters, (float&, float&), void), asCALL_THISCALL); assert(r >= 0);

}

void RegisterCKSynchroObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKSynchroObject>(engine, "CKSynchroObject");

    r = engine->RegisterObjectMethod("CKSynchroObject", "void Reset()", asMETHODPR(CKSynchroObject, Reset, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "void SetRendezVousNumberOfWaiters(int)", asMETHODPR(CKSynchroObject, SetRendezVousNumberOfWaiters, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "int GetRendezVousNumberOfWaiters()", asMETHODPR(CKSynchroObject, GetRendezVousNumberOfWaiters, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "bool CanIPassRendezVous(CKBeObject@)", asMETHODPR(CKSynchroObject, CanIPassRendezVous, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "int GetRendezVousNumberOfArrivedObjects()", asMETHODPR(CKSynchroObject, GetRendezVousNumberOfArrivedObjects, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "CKBeObject@ GetRendezVousArrivedObject(int)", asMETHODPR(CKSynchroObject, GetRendezVousArrivedObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKStateObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKStateObject>(engine, "CKStateObject");

    r = engine->RegisterObjectMethod("CKStateObject", "bool IsStateActive()", asMETHODPR(CKStateObject, IsStateActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKStateObject", "void EnterState()", asMETHODPR(CKStateObject, EnterState, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKStateObject", "void LeaveState()", asMETHODPR(CKStateObject, LeaveState, (), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKCriticalSectionObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKCriticalSectionObject>(engine, "CKCriticalSectionObject");

    r = engine->RegisterObjectMethod("CKCriticalSectionObject", "void Reset()", asMETHODPR(CKCriticalSectionObject, Reset, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCriticalSectionObject", "bool EnterCriticalSection(CKBeObject@)", asMETHODPR(CKCriticalSectionObject, EnterCriticalSection, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCriticalSectionObject", "bool LeaveCriticalSection(CKBeObject@)", asMETHODPR(CKCriticalSectionObject, LeaveCriticalSection, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKKinematicChain(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKKinematicChain>(engine, "CKKinematicChain");

    // Info functions
    r = engine->RegisterObjectMethod("CKKinematicChain", "float GetChainLength(CKBodyPart@ = null)", asMETHODPR(CKKinematicChain, GetChainLength, (CKBodyPart*), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "int GetChainBodyCount(CKBodyPart@ = null)", asMETHODPR(CKKinematicChain, GetChainBodyCount, (CKBodyPart*), int), asCALL_THISCALL); assert(r >= 0);

    // Effectors functions
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetStartEffector()", asMETHODPR(CKKinematicChain, GetStartEffector, (), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR SetStartEffector(CKBodyPart@)", asMETHODPR(CKKinematicChain, SetStartEffector, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetEffector(int)", asMETHODPR(CKKinematicChain, GetEffector, (int), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetEndEffector()", asMETHODPR(CKKinematicChain, GetEndEffector, (), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR SetEndEffector(CKBodyPart@)", asMETHODPR(CKKinematicChain, SetEndEffector, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // Move effector
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR IKSetEffectorPos(const VxVector &in, CK3dEntity@ = null, CKBodyPart@ = null)", asMETHODPR(CKKinematicChain, IKSetEffectorPos, (VxVector*, CK3dEntity*, CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKLayer(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKLayer>(engine, "CKLayer");
}

template <typename T>
static void RegisterCKSceneObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool IsActiveInScene(CKScene@)", asMETHODPR(T, IsActiveInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsActiveInCurrentScene()", asMETHODPR(T, IsActiveInCurrentScene, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInScene(CKScene@)", asMETHODPR(T, IsInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetSceneInCount()", asMETHODPR(T, GetSceneInCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKScene@ GetSceneIn(int)", asMETHODPR(T, GetSceneIn, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKSceneObject") != 0) {
        RegisterClassRefCast<T, CKSceneObject>(engine, name, "CKSceneObject");
    }
}

void RegisterCKSceneObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKSceneObjectMembers<CKSceneObject>(engine, "CKSceneObject");
}

void RegisterCKBehavior(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSceneObjectMembers<CKBehavior>(engine, "CKBehavior");

    // Behavior type and flag
    r = engine->RegisterObjectMethod("CKBehavior", "CK_BEHAVIOR_TYPE GetType()", asMETHODPR(CKBehavior, GetType, (), CK_BEHAVIOR_TYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetType(CK_BEHAVIOR_TYPE)", asMETHODPR(CKBehavior, SetType, (CK_BEHAVIOR_TYPE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetFlags(CK_BEHAVIOR_FLAGS)", asMETHODPR(CKBehavior, SetFlags, (CK_BEHAVIOR_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CK_BEHAVIOR_FLAGS GetFlags()", asMETHODPR(CKBehavior, GetFlags, (), CK_BEHAVIOR_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CK_BEHAVIOR_FLAGS ModifyFlags(CKDWORD, CKDWORD)", asMETHODPR(CKBehavior, ModifyFlags, (CKDWORD, CKDWORD), CK_BEHAVIOR_FLAGS), asCALL_THISCALL); assert(r >= 0);

    // BuildingBlock or Graph
    r = engine->RegisterObjectMethod("CKBehavior", "void UseGraph()", asMETHODPR(CKBehavior, UseGraph, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void UseFunction()", asMETHODPR(CKBehavior, UseFunction, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int IsUsingFunction()", asMETHODPR(CKBehavior, IsUsingFunction, (), int), asCALL_THISCALL); assert(r >= 0);

    // Targetable Behavior
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsTargetable()", asMETHODPR(CKBehavior, IsTargetable, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBeObject@ GetTarget()", asMETHODPR(CKBehavior, GetTarget, (), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR UseTarget(bool = true)", asMETHODPR(CKBehavior, UseTarget, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsUsingTarget()", asMETHODPR(CKBehavior, IsUsingTarget, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ GetTargetParameter()", asMETHODPR(CKBehavior, GetTargetParameter, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetAsTargetable(bool = true)", asMETHODPR(CKBehavior, SetAsTargetable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ ReplaceTargetParameter(CKParameterIn@)", asMETHODPR(CKBehavior, ReplaceTargetParameter, (CKParameterIn*), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ RemoveTargetParameter()", asMETHODPR(CKBehavior, RemoveTargetParameter, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);

    // Compatible Class ID
    r = engine->RegisterObjectMethod("CKBehavior", "CK_CLASSID GetCompatibleClassID()", asMETHODPR(CKBehavior, GetCompatibleClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetCompatibleClassID(CK_CLASSID)", asMETHODPR(CKBehavior, SetCompatibleClassID, (CK_CLASSID), void), asCALL_THISCALL); assert(r >= 0);

    // Function
    r = engine->RegisterObjectMethod("CKBehavior", "void SetFunction(NativePointer)", asMETHODPR(CKBehavior, SetFunction, (CKBEHAVIORFCT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetFunction()", asMETHODPR(CKBehavior, GetFunction, (), CKBEHAVIORFCT), asCALL_THISCALL); assert(r >= 0);

    // Callbacks
    r = engine->RegisterObjectMethod("CKBehavior", "void SetCallbackFunction(NativePointer)", asMETHODPR(CKBehavior, SetCallbackFunction, (CKBEHAVIORCALLBACKFCT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int CallCallbackFunction(CKDWORD)", asMETHODPR(CKBehavior, CallCallbackFunction, (CKDWORD), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int CallSubBehaviorsCallbackFunction(CKDWORD, CKGUID &in)", asMETHODPR(CKBehavior, CallSubBehaviorsCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);

    // Execution
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsActive()", asMETHODPR(CKBehavior, IsActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int Execute(float)", asMETHODPR(CKBehavior, Execute, (float), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsParentScriptActiveInScene(CKScene@)", asMETHODPR(CKBehavior, IsParentScriptActiveInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetShortestDelay(CKBehavior@)", asMETHODPR(CKBehavior, GetShortestDelay, (CKBehavior*), int), asCALL_THISCALL); assert(r >= 0);

    // Owner and Parent
    r = engine->RegisterObjectMethod("CKBehavior", "CKBeObject@ GetOwner()", asMETHODPR(CKBehavior, GetOwner, (), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ GetParent()", asMETHODPR(CKBehavior, GetParent, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ GetOwnerScript()", asMETHODPR(CKBehavior, GetOwnerScript, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);

    // Prototypes
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFromPrototype(CKBehaviorPrototype &in)", asMETHODPR(CKBehavior, InitFromPrototype, (CKBehaviorPrototype*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFromGuid(CKGUID)", asMETHODPR(CKBehavior, InitFromGuid, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFctPtrFromGuid(CKGUID)", asMETHODPR(CKBehavior, InitFctPtrFromGuid, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFctPtrFromPrototype(CKBehaviorPrototype &in)", asMETHODPR(CKBehavior, InitFctPtrFromPrototype, (CKBehaviorPrototype*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKGUID GetPrototypeGuid()", asMETHODPR(CKBehavior, GetPrototypeGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorPrototype &GetPrototype()", asMETHODPR(CKBehavior, GetPrototype, (), CKBehaviorPrototype*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "string GetPrototypeName()", asMETHODPR(CKBehavior, GetPrototypeName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKDWORD GetVersion()", asMETHODPR(CKBehavior, GetVersion, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetVersion(CKDWORD)", asMETHODPR(CKBehavior, SetVersion, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKObjectAnimation(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSceneObjectMembers<CKObjectAnimation>(engine, "CKObjectAnimation");

    // Controller functions
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ CreateController(CKANIMATION_CONTROLLER)", asMETHODPR(CKObjectAnimation, CreateController, (CKANIMATION_CONTROLLER), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "bool DeleteController(CKANIMATION_CONTROLLER)", asMETHODPR(CKObjectAnimation, DeleteController, (CKANIMATION_CONTROLLER), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetPositionController()", asMETHODPR(CKObjectAnimation, GetPositionController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetScaleController()", asMETHODPR(CKObjectAnimation, GetScaleController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetRotationController()", asMETHODPR(CKObjectAnimation, GetRotationController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetScaleAxisController()", asMETHODPR(CKObjectAnimation, GetScaleAxisController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "CKMorphController@ GetMorphController()", asMETHODPR(CKObjectAnimation, GetMorphController, (), CKMorphController*), asCALL_THISCALL); assert(r >= 0);

    // Evaluate functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluatePosition(float, VxVector &out)", asMETHODPR(CKObjectAnimation, EvaluatePosition, (float, VxVector&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateScale(float, VxVector &out)", asMETHODPR(CKObjectAnimation, EvaluateScale, (float, VxVector&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateRotation(float, VxQuaternion &out)", asMETHODPR(CKObjectAnimation, EvaluateRotation, (float, VxQuaternion&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateScaleAxis(float, VxQuaternion &out)", asMETHODPR(CKObjectAnimation, EvaluateScaleAxis, (float, VxQuaternion&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateMorphTarget(float, int, VxVector &in, uint, VxCompressedVector &in)", asMETHODPR(CKObjectAnimation, EvaluateMorphTarget, (float, int, VxVector*, CKDWORD, VxCompressedVector*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateKeys(float, VxQuaternion &in, VxVector &in, VxVector &in, VxQuaternion &in)", asMETHODPR(CKObjectAnimation, EvaluateKeys, (float, VxQuaternion*, VxVector*, VxVector*, VxQuaternion*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Info functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasMorphNormalInfo()", asMETHODPR(CKObjectAnimation, HasMorphNormalInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasMorphInfo()", asMETHODPR(CKObjectAnimation, HasMorphInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasScaleInfo()", asMETHODPR(CKObjectAnimation, HasScaleInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasPositionInfo()", asMETHODPR(CKObjectAnimation, HasPositionInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasRotationInfo()", asMETHODPR(CKObjectAnimation, HasRotationInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasScaleAxisInfo()", asMETHODPR(CKObjectAnimation, HasScaleAxisInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Key functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddPositionKey(float, VxVector &in)", asMETHODPR(CKObjectAnimation, AddPositionKey, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddRotationKey(float, VxQuaternion &in)", asMETHODPR(CKObjectAnimation, AddRotationKey, (float, VxQuaternion*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddScaleKey(float, VxVector &in)", asMETHODPR(CKObjectAnimation, AddScaleKey, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddScaleAxisKey(float, VxQuaternion &in)", asMETHODPR(CKObjectAnimation, AddScaleAxisKey, (float, VxQuaternion*), void), asCALL_THISCALL); assert(r >= 0);

    // Comparison and sharing functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool Compare(CKObjectAnimation@, float = 0.0)", asMETHODPR(CKObjectAnimation, Compare, (CKObjectAnimation*, float), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool ShareDataFrom(CKObjectAnimation@)", asMETHODPR(CKObjectAnimation, ShareDataFrom, (CKObjectAnimation*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKObjectAnimation@ Shared()", asMETHODPR(CKObjectAnimation, Shared, (), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Flags
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetFlags(CKDWORD)", asMETHODPR(CKObjectAnimation, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKDWORD GetFlags()", asMETHODPR(CKObjectAnimation, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Clear functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Clear()", asMETHODPR(CKObjectAnimation, Clear, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void ClearAll()", asMETHODPR(CKObjectAnimation, ClearAll, (), void), asCALL_THISCALL); assert(r >= 0);

    // Merged animations
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetMergeFactor()", asMETHODPR(CKObjectAnimation, GetMergeFactor, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetMergeFactor(float)", asMETHODPR(CKObjectAnimation, SetMergeFactor, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool IsMerged()", asMETHODPR(CKObjectAnimation, IsMerged, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKObjectAnimation@ CreateMergedAnimation(CKObjectAnimation@, bool = false)", asMETHODPR(CKObjectAnimation, CreateMergedAnimation, (CKObjectAnimation*, CKBOOL), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Length
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetLength(float)", asMETHODPR(CKObjectAnimation, SetLength, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetLength()", asMETHODPR(CKObjectAnimation, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);

    // Velocity
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void GetVelocity(float, VxVector &out)", asMETHODPR(CKObjectAnimation, GetVelocity, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    // Step and frame
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKERROR SetStep(float, CKKeyedAnimation@ = null)", asMETHODPR(CKObjectAnimation, SetStep, (float, CKKeyedAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKERROR SetFrame(float, CKKeyedAnimation@ = null)", asMETHODPR(CKObjectAnimation, SetFrame, (float, CKKeyedAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetCurrentStep()", asMETHODPR(CKObjectAnimation, GetCurrentStep, (), float), asCALL_THISCALL); assert(r >= 0);

    // 3D Entity
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Set3dEntity(CK3dEntity@)", asMETHODPR(CKObjectAnimation, Set3dEntity, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CK3dEntity@ Get3dEntity()", asMETHODPR(CKObjectAnimation, Get3dEntity, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    // Morph Vertex Count
    r = engine->RegisterObjectMethod("CKObjectAnimation", "int GetMorphVertexCount()", asMETHODPR(CKObjectAnimation, GetMorphVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Transitions
    // r = engine->RegisterObjectMethod("CKObjectAnimation", "void CreateTransition(float, CKObjectAnimation@, float, CKObjectAnimation@, float, bool, bool, CKAnimKey@ = null)", asMETHODPR(CKObjectAnimation, CreateTransition, (float, CKObjectAnimation*, float, CKObjectAnimation*, float, CKBOOL, CKBOOL, CKAnimKey*), void), asCALL_THISCALL); assert(r >= 0);

    // Clone
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Clone(CKObjectAnimation@)", asMETHODPR(CKObjectAnimation, Clone, (CKObjectAnimation*), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKAnimationMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<CKAnimation>(engine, name);

    // Stepping along
    r = engine->RegisterObjectMethod(name, "float GetLength()", asMETHODPR(T, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetFrame()", asMETHODPR(T, GetFrame, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetNextFrame(float)", asMETHODPR(T, GetNextFrame, (float), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetStep()", asMETHODPR(T, GetStep, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFrame(float)", asMETHODPR(T, SetFrame, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetStep(float)", asMETHODPR(T, SetStep, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLength(float)", asMETHODPR(T, SetLength, (float), void), asCALL_THISCALL); assert(r >= 0);

    // Character functions
    r = engine->RegisterObjectMethod(name, "CKCharacter@ GetCharacter()", asMETHODPR(T, GetCharacter, (), CKCharacter*), asCALL_THISCALL); assert(r >= 0);

    // Frame rate link
    r = engine->RegisterObjectMethod(name, "void LinkToFrameRate(bool, float fps = 30.0f)", asMETHODPR(T, LinkToFrameRate, (CKBOOL, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLinkedFrameRate()", asMETHODPR(T, GetLinkedFrameRate, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsLinkedToFrameRate()", asMETHODPR(T, IsLinkedToFrameRate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Transition mode
    r = engine->RegisterObjectMethod(name, "void SetTransitionMode(CK_ANIMATION_TRANSITION_MODE)", asMETHODPR(T, SetTransitionMode, (CK_ANIMATION_TRANSITION_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_ANIMATION_TRANSITION_MODE GetTransitionMode()", asMETHODPR(T, GetTransitionMode, (), CK_ANIMATION_TRANSITION_MODE), asCALL_THISCALL); assert(r >= 0);

    // Secondary animation mode
    r = engine->RegisterObjectMethod(name, "void SetSecondaryAnimationMode(CK_SECONDARYANIMATION_FLAGS)", asMETHODPR(T, SetSecondaryAnimationMode, (CK_SECONDARYANIMATION_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_SECONDARYANIMATION_FLAGS GetSecondaryAnimationMode()", asMETHODPR(T, GetSecondaryAnimationMode, (), CK_SECONDARYANIMATION_FLAGS), asCALL_THISCALL); assert(r >= 0);

    // Priority and interruption
    r = engine->RegisterObjectMethod(name, "void SetCanBeInterrupt(bool can = true)", asMETHODPR(T, SetCanBeInterrupt, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool CanBeInterrupt()", asMETHODPR(T, CanBeInterrupt, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Orientation
    r = engine->RegisterObjectMethod(name, "void SetCharacterOrientation(bool orient = true)", asMETHODPR(T, SetCharacterOrientation, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool DoesCharacterTakeOrientation()", asMETHODPR(T, DoesCharacterTakeOrientation, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Flags
    r = engine->RegisterObjectMethod(name, "void SetFlags(uint)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint GetFlags()", asMETHODPR(T, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Root entity
    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetRootEntity()", asMETHODPR(T, GetRootEntity, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    // Merged animations
    r = engine->RegisterObjectMethod(name, "float GetMergeFactor()", asMETHODPR(T, GetMergeFactor, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetMergeFactor(float)", asMETHODPR(T, SetMergeFactor, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsMerged()", asMETHODPR(T, IsMerged, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKAnimation@ CreateMergedAnimation(CKAnimation@, bool = false)", asMETHODPR(T, CreateMergedAnimation, (CKAnimation*, CKBOOL), CKAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Set current step
    r = engine->RegisterObjectMethod(name, "void SetCurrentStep(float)", asMETHODPR(T, SetCurrentStep, (float), void), asCALL_THISCALL); assert(r >= 0);

    // Transition animation
    r = engine->RegisterObjectMethod(name, "float CreateTransition(CKAnimation@, CKAnimation@, uint, float = 6.0f, float = 0.0f)", asMETHODPR(T, CreateTransition, (CKAnimation*, CKAnimation*, CKDWORD, float, float), float), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKAnimation") != 0) {
        RegisterClassRefCast<T, CKAnimation>(engine, name, "CKAnimation");
    }
}

void RegisterCKAnimation(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKAnimationMembers<CKAnimation>(engine, "CKAnimation");
}

void RegisterCKKeyedAnimation(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKAnimationMembers<CKKeyedAnimation>(engine, "CKKeyedAnimation");

    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKERROR AddAnimation(CKObjectAnimation@)", asMETHODPR(CKKeyedAnimation, AddAnimation, (CKObjectAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKERROR RemoveAnimation(CKObjectAnimation@)", asMETHODPR(CKKeyedAnimation, RemoveAnimation, (CKObjectAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "int GetAnimationCount()", asMETHODPR(CKKeyedAnimation, GetAnimationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKObjectAnimation@ GetAnimation(CK3dEntity@)", asMETHODPR(CKKeyedAnimation, GetAnimation, (CK3dEntity*), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKObjectAnimation@ GetAnimation(int)", asMETHODPR(CKKeyedAnimation, GetAnimation, (int), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "void Clear()", asMETHODPR(CKKeyedAnimation, Clear, (), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKBeObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "void ExecuteBehaviors(float)", asMETHODPR(T, ExecuteBehaviors, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInGroup(CKGroup@)", asMETHODPR(T, IsInGroup, (CKGroup*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool HasAttribute(CKAttributeType)", asMETHODPR(T, HasAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetAttribute(CKAttributeType, CK_ID = 0)", asMETHODPR(T, SetAttribute, (CKAttributeType, CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveAttribute(CKAttributeType)", asMETHODPR(T, RemoveAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameter(CKAttributeType)", asMETHODPR(T, GetAttributeParameter, (CKAttributeType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeCount()", asMETHODPR(T, GetAttributeCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeType(int)", asMETHODPR(T, GetAttributeType, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameterByIndex(int)", asMETHODPR(T, GetAttributeParameterByIndex, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetAttributeList(CKAttributeVal &out)", asMETHODPR(T, GetAttributeList, (CKAttributeVal*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllAttributes()", asMETHODPR(T, RemoveAllAttributes, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddScript(CKBehavior@)", asMETHODPR(T, AddScript, (CKBehavior*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(CK_ID)", asMETHODPR(T, RemoveScript, (CK_ID), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(int)", asMETHODPR(T, RemoveScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemoveAllScripts()", asMETHODPR(T, RemoveAllScripts, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ GetScript(int)", asMETHODPR(T, GetScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetScriptCount()", asMETHODPR(T, GetScriptCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPriority()", asMETHODPR(T, GetPriority, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPriority(int)", asMETHODPR(T, SetPriority, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLastFrameMessageCount()", asMETHODPR(T, GetLastFrameMessageCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMessage@ GetLastFrameMessage(int)", asMETHODPR(T, GetLastFrameMessage, (int), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAsWaitingForMessages(bool = true)", asMETHODPR(T, SetAsWaitingForMessages, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsWaitingForMessages()", asMETHODPR(T, IsWaitingForMessages, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CallBehaviorCallbackFunction(CKDWORD, CKGUID &)", asMETHODPR(T, CallBehaviorCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLastExecutionTime()", asMETHODPR(T, GetLastExecutionTime, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyPatchForOlderVersion(int, CKFileObject &in)", asMETHODPR(T, ApplyPatchForOlderVersion, (int, CKFileObject*), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKBeObject") != 0) {
        RegisterClassRefCast<T, CKBeObject>(engine, name, "CKBeObject");
    }
}

template <>
static void RegisterCKBeObjectMembers<CKWaveSound>(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<CKWaveSound>(engine, name);

    r = engine->RegisterObjectMethod(name, "void ExecuteBehaviors(float)", asMETHODPR(CKWaveSound, ExecuteBehaviors, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInGroup(CKGroup@)", asMETHODPR(CKWaveSound, IsInGroup, (CKGroup*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool HasAttribute(CKAttributeType)", asMETHODPR(CKWaveSound, HasAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetAttribute(CKAttributeType, CK_ID = 0)", asMETHODPR(CKWaveSound, SetAttribute, (CKAttributeType, CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveAttribute(CKAttributeType)", asMETHODPR(CKWaveSound, RemoveAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameter(CKAttributeType)", asMETHODPR(CKWaveSound, GetAttributeParameter, (CKAttributeType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeCount()", asMETHODPR(CKWaveSound, GetAttributeCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeType(int)", asMETHODPR(CKWaveSound, GetAttributeType, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameterByIndex(int)", asMETHODPR(CKWaveSound, GetAttributeParameterByIndex, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetAttributeList(CKAttributeVal &out)", asMETHODPR(CKWaveSound, GetAttributeList, (CKAttributeVal*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllAttributes()", asMETHODPR(CKWaveSound, RemoveAllAttributes, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddScript(CKBehavior@)", asMETHODPR(CKWaveSound, AddScript, (CKBehavior*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(CK_ID)", asMETHODPR(CKWaveSound, RemoveScript, (CK_ID), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(int)", asMETHODPR(CKWaveSound, RemoveScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemoveAllScripts()", asMETHODPR(CKWaveSound, RemoveAllScripts, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ GetScript(int)", asMETHODPR(CKWaveSound, GetScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetScriptCount()", asMETHODPR(CKWaveSound, GetScriptCount, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "int GetPriority()", asMETHODPR(CKWaveSound, GetPriority, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "void SetPriority(int)", asMETHODPR(CKWaveSound, SetPriority, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLastFrameMessageCount()", asMETHODPR(CKWaveSound, GetLastFrameMessageCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMessage@ GetLastFrameMessage(int)", asMETHODPR(CKWaveSound, GetLastFrameMessage, (int), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAsWaitingForMessages(bool = true)", asMETHODPR(CKWaveSound, SetAsWaitingForMessages, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsWaitingForMessages()", asMETHODPR(CKWaveSound, IsWaitingForMessages, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CallBehaviorCallbackFunction(CKDWORD, CKGUID &in)", asMETHODPR(CKWaveSound, CallBehaviorCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLastExecutionTime()", asMETHODPR(CKWaveSound, GetLastExecutionTime, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyPatchForOlderVersion(int, CKFileObject &in)", asMETHODPR(CKWaveSound, ApplyPatchForOlderVersion, (int, CKFileObject*), void), asCALL_THISCALL); assert(r >= 0);

    RegisterClassRefCast<CKWaveSound, CKBeObject>(engine, name, "CKBeObject");
}

void RegisterCKBeObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKBeObjectMembers<CKBeObject>(engine, "CKBeObject");
}

void RegisterCKScene(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKScene>(engine, "CKScene");

    // Objects functions
    r = engine->RegisterObjectMethod("CKScene", "void AddObjectToScene(CKSceneObject@, bool = true)", asMETHODPR(CKScene, AddObjectToScene, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void RemoveObjectFromScene(CKSceneObject@, bool = true)", asMETHODPR(CKScene, RemoveObjectFromScene, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool IsObjectHere(CKObject@)", asMETHODPR(CKScene, IsObjectHere, (CKObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void BeginAddSequence(bool)", asMETHODPR(CKScene, BeginAddSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void BeginRemoveSequence(bool)", asMETHODPR(CKScene, BeginRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Object List
    r = engine->RegisterObjectMethod("CKScene", "int GetObjectCount()", asMETHODPR(CKScene, GetObjectCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "const XObjectPointerArray &ComputeObjectList(int, bool = true)", asMETHODPR(CKScene, ComputeObjectList, (CK_CLASSID, CKBOOL), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);

    // Object Settings by index in list
    // r = engine->RegisterObjectMethod("CKScene", "CKSceneObjectIterator GetObjectIterator()", asMETHODPR(CKScene, GetObjectIterator, (), CKSceneObjectIterator), asCALL_THISCALL); assert(r >= 0);

    // BeObject and Script Activation/deactivation
    r = engine->RegisterObjectMethod("CKScene", "void Activate(CKSceneObject@, bool)", asMETHODPR(CKScene, Activate, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void DeActivate(CKSceneObject@)", asMETHODPR(CKScene, DeActivate, (CKSceneObject*), void), asCALL_THISCALL); assert(r >= 0);

    // Object Settings by object
    r = engine->RegisterObjectMethod("CKScene", "void SetObjectFlags(CKSceneObject@, CK_SCENEOBJECT_FLAGS)", asMETHODPR(CKScene, SetObjectFlags, (CKSceneObject*, CK_SCENEOBJECT_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CK_SCENEOBJECT_FLAGS GetObjectFlags(CKSceneObject@)", asMETHODPR(CKScene, GetObjectFlags, (CKSceneObject*), CK_SCENEOBJECT_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CK_SCENEOBJECT_FLAGS ModifyObjectFlags(CKSceneObject@, uint, uint)", asMETHODPR(CKScene, ModifyObjectFlags, (CKSceneObject*, CKDWORD, CKDWORD), CK_SCENEOBJECT_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool SetObjectInitialValue(CKSceneObject@, CKStateChunk@)", asMETHODPR(CKScene, SetObjectInitialValue, (CKSceneObject*, CKStateChunk*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKStateChunk@ GetObjectInitialValue(CKSceneObject@)", asMETHODPR(CKScene, GetObjectInitialValue, (CKSceneObject*), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool IsObjectActive(CKSceneObject@)", asMETHODPR(CKScene, IsObjectActive, (CKSceneObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Render Settings
    r = engine->RegisterObjectMethod("CKScene", "void ApplyEnvironmentSettings(XObjectPointerArray &in)", asMETHODPR(CKScene, ApplyEnvironmentSettings, (XObjectPointerArray*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void UseEnvironmentSettings(bool = true)", asMETHODPR(CKScene, UseEnvironmentSettings, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool EnvironmentSettings()", asMETHODPR(CKScene, EnvironmentSettings, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Ambient Light
    r = engine->RegisterObjectMethod("CKScene", "void SetAmbientLight(uint)", asMETHODPR(CKScene, SetAmbientLight, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "uint GetAmbientLight()", asMETHODPR(CKScene, GetAmbientLight, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Fog Access
    r = engine->RegisterObjectMethod("CKScene", "void SetFogMode(int)", asMETHODPR(CKScene, SetFogMode, (VXFOG_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogStart(float)", asMETHODPR(CKScene, SetFogStart, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogEnd(float)", asMETHODPR(CKScene, SetFogEnd, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogDensity(float)", asMETHODPR(CKScene, SetFogDensity, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogColor(uint)", asMETHODPR(CKScene, SetFogColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "int GetFogMode()", asMETHODPR(CKScene, GetFogMode, (), VXFOG_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogStart()", asMETHODPR(CKScene, GetFogStart, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogEnd()", asMETHODPR(CKScene, GetFogEnd, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogDensity()", asMETHODPR(CKScene, GetFogDensity, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "uint GetFogColor()", asMETHODPR(CKScene, GetFogColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Background
    r = engine->RegisterObjectMethod("CKScene", "void SetBackgroundColor(uint)", asMETHODPR(CKScene, SetBackgroundColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "uint GetBackgroundColor()", asMETHODPR(CKScene, GetBackgroundColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetBackgroundTexture(CKTexture@)", asMETHODPR(CKScene, SetBackgroundTexture, (CKTexture*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKTexture@ GetBackgroundTexture()", asMETHODPR(CKScene, GetBackgroundTexture, (), CKTexture*), asCALL_THISCALL); assert(r >= 0);

    // Active camera
    r = engine->RegisterObjectMethod("CKScene", "void SetStartingCamera(CKCamera@)", asMETHODPR(CKScene, SetStartingCamera, (CKCamera*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKCamera@ GetStartingCamera()", asMETHODPR(CKScene, GetStartingCamera, (), CKCamera*), asCALL_THISCALL); assert(r >= 0);

    // Level functions
    r = engine->RegisterObjectMethod("CKScene", "CKLevel@ GetLevel()", asMETHODPR(CKScene, GetLevel, (), CKLevel*), asCALL_THISCALL); assert(r >= 0);

    // Merge functions
    r = engine->RegisterObjectMethod("CKScene", "CKERROR Merge(CKScene@, CKLevel@ = null)", asMETHODPR(CKScene, Merge, (CKScene*, CKLevel*), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKLevel(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKLevel>(engine, "CKLevel");

    // Object Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddObject(CKObject@)", asMETHODPR(CKLevel, AddObject, (CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveObject(CKObject@)", asMETHODPR(CKLevel, RemoveObject, (CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveObject(int)", asMETHODPR(CKLevel, RemoveObject, (CK_ID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void BeginAddSequence(bool)", asMETHODPR(CKLevel, BeginAddSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void BeginRemoveSequence(bool)", asMETHODPR(CKLevel, BeginRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Object List
    r = engine->RegisterObjectMethod("CKLevel", "const XObjectPointerArray &ComputeObjectList(int, bool = true)", asMETHODPR(CKLevel, ComputeObjectList, (CK_CLASSID, CKBOOL), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);

    // Place Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddPlace(CKPlace@)", asMETHODPR(CKLevel, AddPlace, (CKPlace*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemovePlace(CKPlace@)", asMETHODPR(CKLevel, RemovePlace, (CKPlace*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKPlace@ RemovePlace(int)", asMETHODPR(CKLevel, RemovePlace, (int), CKPlace*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKPlace@ GetPlace(int)", asMETHODPR(CKLevel, GetPlace, (int), CKPlace*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetPlaceCount()", asMETHODPR(CKLevel, GetPlaceCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Scene Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddScene(CKScene@)", asMETHODPR(CKLevel, AddScene, (CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveScene(CKScene@)", asMETHODPR(CKLevel, RemoveScene, (CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ RemoveScene(int)", asMETHODPR(CKLevel, RemoveScene, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetScene(int)", asMETHODPR(CKLevel, GetScene, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetSceneCount()", asMETHODPR(CKLevel, GetSceneCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Active Scene
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR SetNextActiveScene(CKScene@, int = CK_SCENEOBJECTACTIVITY_SCENEDEFAULT, int = CK_SCENEOBJECTRESET_RESET)", asMETHODPR(CKLevel, SetNextActiveScene, (CKScene*, CK_SCENEOBJECTACTIVITY_FLAGS, CK_SCENEOBJECTRESET_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR LaunchScene(CKScene@, int = CK_SCENEOBJECTACTIVITY_SCENEDEFAULT, int = CK_SCENEOBJECTRESET_RESET)", asMETHODPR(CKLevel, LaunchScene, (CKScene*, CK_SCENEOBJECTACTIVITY_FLAGS, CK_SCENEOBJECTRESET_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetCurrentScene()", asMETHODPR(CKLevel, GetCurrentScene, (), CKScene*), asCALL_THISCALL); assert(r >= 0);

    // Render Context functions
    r = engine->RegisterObjectMethod("CKLevel", "void AddRenderContext(CKRenderContext@, bool = false)", asMETHODPR(CKLevel, AddRenderContext, (CKRenderContext*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void RemoveRenderContext(CKRenderContext@)", asMETHODPR(CKLevel, RemoveRenderContext, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetRenderContextCount()", asMETHODPR(CKLevel, GetRenderContextCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKRenderContext@ GetRenderContext(int)", asMETHODPR(CKLevel, GetRenderContext, (int), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);

    // Main Scene for this Level
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetLevelScene()", asMETHODPR(CKLevel, GetLevelScene, (), CKScene*), asCALL_THISCALL); assert(r >= 0);

    // Merge
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR Merge(CKLevel@, bool)", asMETHODPR(CKLevel, Merge, (CKLevel*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKGroup(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKGroup>(engine, "CKGroup");

    // Insertion/Removal
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR AddObject(CKBeObject@)", asMETHODPR(CKGroup, AddObject, (CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR AddObjectFront(CKBeObject@)", asMETHODPR(CKGroup, AddObjectFront, (CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR InsertObjectAt(CKBeObject@, int)", asMETHODPR(CKGroup, InsertObjectAt, (CKBeObject*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGroup", "CKBeObject@ RemoveObject(int)", asMETHODPR(CKGroup, RemoveObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void RemoveObject(CKBeObject@)", asMETHODPR(CKGroup, RemoveObject, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void Clear()", asMETHODPR(CKGroup, Clear, (), void), asCALL_THISCALL); assert(r >= 0);

    // Order
    r = engine->RegisterObjectMethod("CKGroup", "void MoveObjectUp(CKBeObject@)", asMETHODPR(CKGroup, MoveObjectUp, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void MoveObjectDown(CKBeObject@)", asMETHODPR(CKGroup, MoveObjectDown, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);

    // Object Access
    r = engine->RegisterObjectMethod("CKGroup", "CKBeObject@ GetObject(int)", asMETHODPR(CKGroup, GetObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "int GetObjectCount()", asMETHODPR(CKGroup, GetObjectCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Common Class ID
    r = engine->RegisterObjectMethod("CKGroup", "CK_CLASSID GetCommonClassID()", asMETHODPR(CKGroup, GetCommonClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKMaterial(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKMaterial>(engine, "CKMaterial");

    r = engine->RegisterObjectMethod("CKMaterial", "float GetPower() const", asMETHODPR(CKMaterial, GetPower, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetPower(float)", asMETHODPR(CKMaterial, SetPower, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetAmbient() const", asMETHODPR(CKMaterial, GetAmbient, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAmbient(const VxColor &in)", asMETHODPR(CKMaterial, SetAmbient, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetDiffuse() const", asMETHODPR(CKMaterial, GetDiffuse, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetDiffuse(const VxColor &in)", asMETHODPR(CKMaterial, SetDiffuse, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetSpecular() const", asMETHODPR(CKMaterial, GetSpecular, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetSpecular(const VxColor &in)", asMETHODPR(CKMaterial, SetSpecular, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetEmissive() const", asMETHODPR(CKMaterial, GetEmissive, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetEmissive(const VxColor &in)", asMETHODPR(CKMaterial, SetEmissive, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKTexture@ GetTexture(int texIndex = 0)", asMETHODPR(CKMaterial, GetTexture, (int), CKTexture*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTexture(int texIndex, CKTexture@)", asMETHODPR(CKMaterial, SetTexture, (int, CKTexture*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTexture(CKTexture@)", asMETHODPR(CKMaterial, SetTexture0, (CKTexture*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_BLENDMODE GetTextureBlendMode() const", asMETHODPR(CKMaterial, GetTextureBlendMode, (), VXTEXTURE_BLENDMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureBlendMode(VXTEXTURE_BLENDMODE)", asMETHODPR(CKMaterial, SetTextureBlendMode, (VXTEXTURE_BLENDMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_FILTERMODE GetTextureMinMode() const", asMETHODPR(CKMaterial, GetTextureMinMode, (), VXTEXTURE_FILTERMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureMinMode(VXTEXTURE_FILTERMODE)", asMETHODPR(CKMaterial, SetTextureMinMode, (VXTEXTURE_FILTERMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_FILTERMODE GetTextureMagMode() const", asMETHODPR(CKMaterial, GetTextureMagMode, (), VXTEXTURE_FILTERMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureMagMode(VXTEXTURE_FILTERMODE)", asMETHODPR(CKMaterial, SetTextureMagMode, (VXTEXTURE_FILTERMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_ADDRESSMODE GetTextureAddressMode() const", asMETHODPR(CKMaterial, GetTextureAddressMode, (), VXTEXTURE_ADDRESSMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureAddressMode(VXTEXTURE_ADDRESSMODE)", asMETHODPR(CKMaterial, SetTextureAddressMode, (VXTEXTURE_ADDRESSMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKDWORD GetTextureBorderColor() const", asMETHODPR(CKMaterial, GetTextureBorderColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureBorderColor(CKDWORD)", asMETHODPR(CKMaterial, SetTextureBorderColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXBLEND_MODE GetSourceBlend() const", asMETHODPR(CKMaterial, GetSourceBlend, (), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetSourceBlend(VXBLEND_MODE)", asMETHODPR(CKMaterial, SetSourceBlend, (VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXBLEND_MODE GetDestBlend() const", asMETHODPR(CKMaterial, GetDestBlend, (), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetDestBlend(VXBLEND_MODE)", asMETHODPR(CKMaterial, SetDestBlend, (VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool IsTwoSided() const", asMETHODPR(CKMaterial, IsTwoSided, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTwoSided(bool)", asMETHODPR(CKMaterial, SetTwoSided, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool ZWriteEnabled() const", asMETHODPR(CKMaterial, ZWriteEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableZWrite(bool)", asMETHODPR(CKMaterial, EnableZWrite, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool AlphaBlendEnabled() const", asMETHODPR(CKMaterial, AlphaBlendEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableAlphaBlend(bool)", asMETHODPR(CKMaterial, EnableAlphaBlend, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXCMPFUNC GetZFunc() const", asMETHODPR(CKMaterial, GetZFunc, (), VXCMPFUNC), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetZFunc(VXCMPFUNC)", asMETHODPR(CKMaterial, SetZFunc, (VXCMPFUNC), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool PerspectiveCorrectionEnabled() const", asMETHODPR(CKMaterial, PerspectiveCorrectionEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnablePerspectiveCorrection(bool)", asMETHODPR(CKMaterial, EnablePerspectiveCorrection, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetFillMode(VXFILL_MODE)", asMETHODPR(CKMaterial, SetFillMode, (VXFILL_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VXFILL_MODE GetFillMode() const", asMETHODPR(CKMaterial, GetFillMode, (), VXFILL_MODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetShadeMode(VXSHADE_MODE)", asMETHODPR(CKMaterial, SetShadeMode, (VXSHADE_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VXSHADE_MODE GetShadeMode() const", asMETHODPR(CKMaterial, GetShadeMode, (), VXSHADE_MODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool SetAsCurrent(CKRenderContext@, bool lit = true, int textureStage = 0)", asMETHODPR(CKMaterial, SetAsCurrent, (CKRenderContext*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool IsAlphaTransparent() const", asMETHODPR(CKMaterial, IsAlphaTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool AlphaTestEnabled() const", asMETHODPR(CKMaterial, AlphaTestEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableAlphaTest(bool)", asMETHODPR(CKMaterial, EnableAlphaTest, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXCMPFUNC GetAlphaFunc() const", asMETHODPR(CKMaterial, GetAlphaFunc, (), VXCMPFUNC), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAlphaFunc(VXCMPFUNC)", asMETHODPR(CKMaterial, SetAlphaFunc, (VXCMPFUNC), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKBYTE GetAlphaRef() const", asMETHODPR(CKMaterial, GetAlphaRef, (), CKBYTE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAlphaRef(CKBYTE)", asMETHODPR(CKMaterial, SetAlphaRef, (CKBYTE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetCallback(NativePointer, NativePointer)", asMETHODPR(CKMaterial, SetCallback, (CK_MATERIALCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "NativePointer GetCallback(NativePointer &out) const", asMETHODPR(CKMaterial, GetCallback, (void**), CK_MATERIALCALLBACK), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetEffect(VX_EFFECT)", asMETHODPR(CKMaterial, SetEffect, (VX_EFFECT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VX_EFFECT GetEffect() const", asMETHODPR(CKMaterial, GetEffect, (), VX_EFFECT), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKParameter@ GetEffectParameter() const", asMETHODPR(CKMaterial, GetEffectParameter, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKTexture(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKTexture>(engine, "CKTexture");
    // RegisterCKBitmapDataMembers<CKTexture>(engine, "CKTexture");

    r = engine->RegisterObjectMethod("CKTexture", "bool Create(int, int, int bpp = 32, int slot = 0)", asMETHODPR(CKTexture, Create, (int, int, int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool LoadImage(const string &in, int slot = 0)", asMETHODPR(CKTexture, LoadImage, (CKSTRING, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool LoadMovie(const string &in)", asMETHODPR(CKTexture, LoadMovie, (CKSTRING), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SetAsCurrent(CKRenderContext@, bool clamping = false, int textureStage = 0)", asMETHODPR(CKTexture, SetAsCurrent, (CKRenderContext*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool Restore(bool clamp = false)", asMETHODPR(CKTexture, Restore, (CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SystemToVideoMemory(CKRenderContext@, bool clamping = false)", asMETHODPR(CKTexture, SystemToVideoMemory, (CKRenderContext*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool FreeVideoMemory()", asMETHODPR(CKTexture, FreeVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool IsInVideoMemory() const", asMETHODPR(CKTexture, IsInVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool CopyContext(CKRenderContext@, const VxRect &in, const VxRect &in, int cubeMapFace = 0)", asMETHODPR(CKTexture, CopyContext, (CKRenderContext*, VxRect*, VxRect*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool UseMipmap(bool useMipmap)", asMETHODPR(CKTexture, UseMipmap, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "int GetMipmapCount() const", asMETHODPR(CKTexture, GetMipmapCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetVideoTextureDesc(VxImageDescEx &out)", asMETHODPR(CKTexture, GetVideoTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "VX_PIXELFORMAT GetVideoPixelFormat() const", asMETHODPR(CKTexture, GetVideoPixelFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetSystemTextureDesc(VxImageDescEx &out)", asMETHODPR(CKTexture, GetSystemTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "void SetDesiredVideoFormat(VX_PIXELFORMAT)", asMETHODPR(CKTexture, SetDesiredVideoFormat, (VX_PIXELFORMAT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "VX_PIXELFORMAT GetDesiredVideoFormat() const", asMETHODPR(CKTexture, GetDesiredVideoFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SetUserMipMapMode(bool userMipmap)", asMETHODPR(CKTexture, SetUserMipMapMode, (CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetUserMipMapLevel(int, VxImageDescEx &out)", asMETHODPR(CKTexture, GetUserMipMapLevel, (int, VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "int GetRstTextureIndex() const", asMETHODPR(CKTexture, GetRstTextureIndex, (), int), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKMeshMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool IsTransparent() const", asMETHODPR(T, IsTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparent(bool)", asMETHODPR(T, SetTransparent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetWrapMode(VXTEXTURE_WRAPMODE)", asMETHODPR(T, SetWrapMode, (VXTEXTURE_WRAPMODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXTEXTURE_WRAPMODE GetWrapMode() const", asMETHODPR(T, GetWrapMode, (), VXTEXTURE_WRAPMODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetLitMode(VXMESH_LITMODE)", asMETHODPR(T, SetLitMode, (VXMESH_LITMODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXMESH_LITMODE GetLitMode() const", asMETHODPR(T, GetLitMode, (), VXMESH_LITMODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKDWORD GetFlags() const", asMETHODPR(T, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFlags(CKDWORD)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetModifierVertices(NativePointer)", asMETHODPR(T, GetModifierVertices, (CKDWORD*), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetModifierVertexCount() const", asMETHODPR(T, GetModifierVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ModifierVertexMove(bool, bool)", asMETHODPR(T, ModifierVertexMove, (CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetModifierUVs(NativePointer, int = -1)", asMETHODPR(T, GetModifierUVs, (CKDWORD*, int), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetModifierUVCount(int = -1) const", asMETHODPR(T, GetModifierUVCount, (int), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ModifierUVMove()", asMETHODPR(T, ModifierUVMove, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetVertexCount() const", asMETHODPR(T, GetVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetVertexCount(int)", asMETHODPR(T, SetVertexCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexColor(int, uint32)", asMETHODPR(T, SetVertexColor, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexSpecularColor(int, uint32)", asMETHODPR(T, SetVertexSpecularColor, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexNormal(int, const VxVector &in)", asMETHODPR(T, SetVertexNormal, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexPosition(int, const VxVector &in)", asMETHODPR(T, SetVertexPosition, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexTextureCoordinates(int, float, float, int = -1)", asMETHODPR(T, SetVertexTextureCoordinates, (int, float, float, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetColorsPtr(uint &out)", asMETHODPR(T, GetColorsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetSpecularColorsPtr(uint &out)", asMETHODPR(T, GetSpecularColorsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetNormalsPtr(uint &out)", asMETHODPR(T, GetNormalsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetPositionsPtr(uint &out)", asMETHODPR(T, GetPositionsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetTextureCoordinatesPtr(uint &out, int channel = -1)", asMETHODPR(T, GetTextureCoordinatesPtr, (CKDWORD*, int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint32 GetVertexColor(int) const", asMETHODPR(T, GetVertexColor, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint32 GetVertexSpecularColor(int) const", asMETHODPR(T, GetVertexSpecularColor, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexNormal(int, VxVector &)", asMETHODPR(T, GetVertexNormal, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexPosition(int, VxVector &)", asMETHODPR(T, GetVertexPosition, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexTextureCoordinates(int, float &, float &, int = -1)", asMETHODPR(T, GetVertexTextureCoordinates, (int, float*, float*, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void TranslateVertices(const VxVector &in)", asMETHODPR(T, TranslateVertices, (VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(const VxVector &in)", asFUNCTIONPR([](T *self, VxVector &v) { self->ScaleVertices(&v, nullptr); }, (T *, VxVector &), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(const VxVector &in, const VxVector &in)", asMETHODPR(T, ScaleVertices, (VxVector*, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(float, float, float)", asFUNCTIONPR([](T *self, float x, float y, float z) { self->ScaleVertices3f(x, y, z, nullptr); }, (T *, float, float, float), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(float, float, float, const VxVector &in)", asMETHODPR(T, ScaleVertices3f, (float, float, float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RotateVertices(const VxVector &in, float)", asMETHODPR(T, RotateVertices, (VxVector*, float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void VertexMove()", asMETHODPR(T, VertexMove, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void UVChanged()", asMETHODPR(T, UVChanged, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void NormalChanged()", asMETHODPR(T, NormalChanged, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ColorChanged()", asMETHODPR(T, ColorChanged, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetFaceCount() const", asMETHODPR(T, GetFaceCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetFaceCount(int)", asMETHODPR(T, SetFaceCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetFacesIndices()", asMETHODPR(T, GetFacesIndices, (), CKWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetFaceVertexIndex(int, int &out, int &out, int &out)", asMETHODPR(T, GetFaceVertexIndex, (int, int&, int&, int&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetFaceMaterial(int)", asMETHODPR(T, GetFaceMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxVector& GetFaceNormal(int)", asMETHODPR(T, GetFaceNormal, (int), const VxVector&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint16 GetFaceChannelMask(int)", asMETHODPR(T, GetFaceChannelMask, (int), CKWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VxVector &GetFaceVertex(int, int)", asMETHODPR(T, GetFaceVertex, (int, int), VxVector&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetFaceNormalsPtr(uint &out)", asMETHODPR(T, GetFaceNormalsPtr, (CKDWORD*), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceVertexIndex(int, int, int, int)", asMETHODPR(T, SetFaceVertexIndex, (int, int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceMaterialEx(NativePointer, int, CKMaterial@)", asMETHODPR(T, SetFaceMaterialEx, (int*, int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceMaterial(int, CKMaterial@)", asMETHODPR(T, SetFaceMaterial, (int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceChannelMask(int, uint16)", asMETHODPR(T, SetFaceChannelMask, (int, CKWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ReplaceMaterial(CKMaterial@, CKMaterial@)", asMETHODPR(T, ReplaceMaterial, (CKMaterial*, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ChangeFaceChannelMask(int, uint16, uint16)", asMETHODPR(T, ChangeFaceChannelMask, (int, CKWORD, CKWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyGlobalMaterial(CKMaterial@)", asMETHODPR(T, ApplyGlobalMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void DissociateAllFaces()", asMETHODPR(T, DissociateAllFaces, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetLineCount(int)", asMETHODPR(T, SetLineCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLineCount()", asMETHODPR(T, GetLineCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetLineIndices()", asMETHODPR(T, GetLineIndices, (), CKWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLine(int, int, int)", asMETHODPR(T, SetLine, (int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetLine(int, int &out, int &out)", asMETHODPR(T, GetLine, (int, int*, int*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CreateLineStrip(int, int, int)", asMETHODPR(T, CreateLineStrip, (int, int, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Clean(bool = false)", asMETHODPR(T, Clean, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void InverseWinding()", asMETHODPR(T, InverseWinding, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Consolidate()", asMETHODPR(T, Consolidate, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void UnOptimize()", asMETHODPR(T, UnOptimize, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetRadius() const", asMETHODPR(T, GetRadius, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxBbox &GetLocalBox() const", asMETHODPR(T, GetLocalBox, (), const VxBbox&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetBaryCenter(VxVector &out) const", asMETHODPR(T, GetBaryCenter, (VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetChannelCount() const", asMETHODPR(T, GetChannelCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int AddChannel(CKMaterial@, bool = true)", asMETHODPR(T, AddChannel, (CKMaterial*, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveChannelMaterial(CKMaterial@)", asMETHODPR(T, RemoveChannelMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveChannel(int)", asMETHODPR(T, RemoveChannel, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetChannelByMaterial(CKMaterial@) const", asMETHODPR(T, GetChannelByMaterial, (CKMaterial*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ActivateChannel(int, bool = true)", asMETHODPR(T, ActivateChannel, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsChannelActive(int) const", asMETHODPR(T, IsChannelActive, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ActivateAllChannels(bool = true)", asMETHODPR(T, ActivateAllChannels, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void LitChannel(int, bool = true)", asMETHODPR(T, LitChannel, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsChannelLit(int) const", asMETHODPR(T, IsChannelLit, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint32 GetChannelFlags(int) const", asMETHODPR(T, GetChannelFlags, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelFlags(int, uint32)", asMETHODPR(T, SetChannelFlags, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetChannelMaterial(int) const", asMETHODPR(T, GetChannelMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelMaterial(int, CKMaterial@)", asMETHODPR(T, SetChannelMaterial, (int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXBLEND_MODE GetChannelSourceBlend(int) const", asMETHODPR(T, GetChannelSourceBlend, (int), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXBLEND_MODE GetChannelDestBlend(int) const", asMETHODPR(T, GetChannelDestBlend, (int), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelSourceBlend(int, VXBLEND_MODE)", asMETHODPR(T, SetChannelSourceBlend, (int, VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelDestBlend(int, VXBLEND_MODE)", asMETHODPR(T, SetChannelDestBlend, (int, VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void BuildNormals()", asMETHODPR(T, BuildNormals, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void BuildFaceNormals()", asMETHODPR(T, BuildFaceNormals, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Render(CKRenderContext@, CK3dEntity@)", asMETHODPR(T, Render, (CKRenderContext*, CK3dEntity*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPreRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddPreRenderCallBack, (CK_MESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePreRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemovePreRenderCallBack, (CK_MESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AddPostRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddPostRenderCallBack, (CK_MESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePostRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemovePostRenderCallBack, (CK_MESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, SetRenderCallBack, (CK_MESHRENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetDefaultRenderCallBack()", asMETHODPR(T, SetDefaultRenderCallBack, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllCallbacks()", asMETHODPR(T, RemoveAllCallbacks, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetMaterialCount() const", asMETHODPR(T, GetMaterialCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetMaterial(int) const", asMETHODPR(T, GetMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetVertexWeightsCount() const", asMETHODPR(T, GetVertexWeightsCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexWeightsCount(int)", asMETHODPR(T, SetVertexWeightsCount, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetVertexWeightsPtr()", asMETHODPR(T, GetVertexWeightsPtr, (), float*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetVertexWeight(int) const", asMETHODPR(T, GetVertexWeight, (int), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexWeight(int, float)", asMETHODPR(T, SetVertexWeight, (int, float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void LoadVertices(CKStateChunk@)", asMETHODPR(T, LoadVertices, (CKStateChunk*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetVerticesRendered(int)", asMETHODPR(T, SetVerticesRendered, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetVerticesRendered() const", asMETHODPR(T, GetVerticesRendered, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR CreatePM()", asMETHODPR(T, CreatePM, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void DestroyPM()", asMETHODPR(T, DestroyPM, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPM() const", asMETHODPR(T, IsPM, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void EnablePMGeoMorph(bool)", asMETHODPR(T, EnablePMGeoMorph, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPMGeoMorphEnabled() const", asMETHODPR(T, IsPMGeoMorphEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPMGeoMorphStep(int)", asMETHODPR(T, SetPMGeoMorphStep, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPMGeoMorphStep() const", asMETHODPR(T, GetPMGeoMorphStep, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddSubMeshPreRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddSubMeshPreRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveSubMeshPreRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemoveSubMeshPreRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AddSubMeshPostRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddSubMeshPostRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveSubMeshPostRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemoveSubMeshPostRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKMesh") != 0) {
        RegisterClassRefCast<T, CKMesh>(engine, name, "CKMesh");
    }
}

void RegisterCKMesh(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKMeshMembers<CKMesh>(engine, "CKMesh");
}

void RegisterCKPatchMesh(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKMeshMembers<CKPatchMesh>(engine, "CKPatchMesh");

    // TODO: Add PatchMesh specific methods
}

void RegisterCKDataArray(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKDataArray>(engine, "CKDataArray");

    // Column/Format Functions
    r = engine->RegisterObjectMethod("CKDataArray", "void InsertColumn(int, CK_ARRAYTYPE, const string &in, CKGUID)", asMETHODPR(CKDataArray, InsertColumn, (int, CK_ARRAYTYPE, char*, CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void MoveColumn(int, int)", asMETHODPR(CKDataArray, MoveColumn, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RemoveColumn(int)", asMETHODPR(CKDataArray, RemoveColumn, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetColumnName(int, const string &in)", asMETHODPR(CKDataArray, SetColumnName, (int, char*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "string GetColumnName(int)", asMETHODPR(CKDataArray, GetColumnName, (int), char*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetColumnType(int, CK_ARRAYTYPE, CKGUID)", asMETHODPR(CKDataArray, SetColumnType, (int, CK_ARRAYTYPE, CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CK_ARRAYTYPE GetColumnType(int)", asMETHODPR(CKDataArray, GetColumnType, (int), CK_ARRAYTYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKGUID GetColumnParameterGuid(int)", asMETHODPR(CKDataArray, GetColumnParameterGuid, (int), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetKeyColumn()", asMETHODPR(CKDataArray, GetKeyColumn, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetKeyColumn(int)", asMETHODPR(CKDataArray, SetKeyColumn, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetColumnCount()", asMETHODPR(CKDataArray, GetColumnCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Elements Functions
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD &GetElement(int, int)", asMETHODPR(CKDataArray, GetElement, (int, int), CKDWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetElementValue(int, int, NativePointer)", asMETHODPR(CKDataArray, GetElementValue, (int, int, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKObject@ GetElementObject(int, int)", asMETHODPR(CKDataArray, GetElementObject, (int, int), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementValue(int, int, NativePointer, int)", asMETHODPR(CKDataArray, SetElementValue, (int, int, void*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementValueFromParameter(int, int, CKParameter@)", asMETHODPR(CKDataArray, SetElementValueFromParameter, (int, int, CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementObject(int, int, CKObject@)", asMETHODPR(CKDataArray, SetElementObject, (int, int, CKObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Parameters Shortcuts
    r = engine->RegisterObjectMethod("CKDataArray", "bool PasteShortcut(int, int, CKParameter@)", asMETHODPR(CKDataArray, PasteShortcut, (int, int, CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKParameterOut@ RemoveShortcut(int, int)", asMETHODPR(CKDataArray, RemoveShortcut, (int, int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);

    // String Value
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementStringValue(int, int, const string &in)", asMETHODPR(CKDataArray, SetElementStringValue, (int, int, char*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetStringValue(bool, int, string &out)", asMETHODPR(CKDataArray, GetStringValue, (CKDWORD, int, char*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetElementStringValue(int, int, string &out)", asMETHODPR(CKDataArray, GetElementStringValue, (int, int, char*), int), asCALL_THISCALL); assert(r >= 0);

    // Load / Write
    r = engine->RegisterObjectMethod("CKDataArray", "bool LoadElements(const string &in, bool, int)", asMETHODPR(CKDataArray, LoadElements, (CKSTRING, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool WriteElements(const string &in, int, int, bool)", asMETHODPR(CKDataArray, WriteElements, (CKSTRING, int, int, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Rows Functions
    r = engine->RegisterObjectMethod("CKDataArray", "int GetRowCount()", asMETHODPR(CKDataArray, GetRowCount, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKDataArray", "CKDataRow@ GetRow(int)", asMETHODPR(CKDataArray, GetRow, (int), CKDataRow*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void AddRow()", asMETHODPR(CKDataArray, AddRow, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKDataArray", "CKDataRow@ InsertRow(int)", asMETHODPR(CKDataArray, InsertRow, (int), CKDataRow*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RemoveRow(int)", asMETHODPR(CKDataArray, RemoveRow, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void MoveRow(int, int)", asMETHODPR(CKDataArray, MoveRow, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SwapRows(int, int)", asMETHODPR(CKDataArray, SwapRows, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Clear(bool)", asMETHODPR(CKDataArray, Clear, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Algorithm
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetHighest(int, int &out)", asMETHODPR(CKDataArray, GetHighest, (int, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetLowest(int, int &out)", asMETHODPR(CKDataArray, GetLowest, (int, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetNearest(int, NativePointer, int &out)", asMETHODPR(CKDataArray, GetNearest, (int, void*, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void ColumnTransform(int, CK_BINARYOPERATOR, CKDWORD)", asMETHODPR(CKDataArray, ColumnTransform, (int, CK_BINARYOPERATOR, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void ColumnsOperate(int, CK_BINARYOPERATOR, int, int)", asMETHODPR(CKDataArray, ColumnsOperate, (int, CK_BINARYOPERATOR, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Sort(int, bool)", asMETHODPR(CKDataArray, Sort, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Unique(int)", asMETHODPR(CKDataArray, Unique, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RandomShuffle()", asMETHODPR(CKDataArray, RandomShuffle, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Reverse()", asMETHODPR(CKDataArray, Reverse, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD Sum(int)", asMETHODPR(CKDataArray, Sum, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD Product(int)", asMETHODPR(CKDataArray, Product, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetCount(int, CK_COMPOPERATOR, CKDWORD, int)", asMETHODPR(CKDataArray, GetCount, (int, CK_COMPOPERATOR, CKDWORD, int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void CreateGroup(int, CK_COMPOPERATOR, CKDWORD, int, CKGroup@, int)", asMETHODPR(CKDataArray, CreateGroup, (int, CK_COMPOPERATOR, CKDWORD, int, CKGroup*, int), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKSoundMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "CK_SOUND_SAVEOPTIONS GetSaveOptions()", asMETHOD(T, GetSaveOptions), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_SOUND_SAVEOPTIONS)", asMETHOD(T, SetSaveOptions), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKSound") != 0) {
        RegisterClassRefCast<T, CKSound>(engine, name, "CKSound");
    }
}

template <>
static void RegisterCKSoundMembers<CKWaveSound>(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKWaveSound>(engine, name);

    r = engine->RegisterObjectMethod(name, "CK_SOUND_SAVEOPTIONS GetSaveOptions()", asMETHOD(CKWaveSound, GetSaveOptions), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_SOUND_SAVEOPTIONS)", asMETHOD(CKWaveSound, SetSaveOptions), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKSound") != 0) {
        RegisterClassRefCast<CKWaveSound, CKSound>(engine, name, "CKSound");
    }
}

void RegisterCKSound(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKSoundMembers<CKSound>(engine, "CKSound");
}

void RegisterCKWaveSound(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSoundMembers<CKWaveSound>(engine, "CKWaveSound");

    r = engine->RegisterObjectMethod("CKWaveSound", "CKSOUNDHANDLE PlayMinion(bool, CK3dEntity@, VxVector &in, VxVector &in, float)", asMETHODPR(CKWaveSound, PlayMinion, (CKBOOL, CK3dEntity *, VxVector *, VxVector *, float), CKSOUNDHANDLE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetSoundFileName(const string &in)", asMETHODPR(CKWaveSound, SetSoundFileName, (const CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "string GetSoundFileName()", asMETHODPR(CKWaveSound, GetSoundFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "int GetSoundLength()", asMETHODPR(CKWaveSound, GetSoundLength, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR GetSoundFormat(CKWaveFormat &out)", asMETHODPR(CKWaveSound, GetSoundFormat, (CKWaveFormat &), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CK_WAVESOUND_TYPE GetType()", asMETHODPR(CKWaveSound, GetType, (), CK_WAVESOUND_TYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void SetType(CK_WAVESOUND_TYPE)", asMETHODPR(CKWaveSound, SetType, (CK_WAVESOUND_TYPE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "uint GetState()", asMETHODPR(CKWaveSound, GetState, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void SetState(uint)", asMETHODPR(CKWaveSound, SetState, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPriority(float)", asMETHODPR(CKWaveSound, SetPriority, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPriority()", asMETHODPR(CKWaveSound, GetPriority, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetLoopMode(bool)", asMETHODPR(CKWaveSound, SetLoopMode, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "bool GetLoopMode()", asMETHODPR(CKWaveSound, GetLoopMode, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetFileStreaming(bool, bool = false)", asMETHODPR(CKWaveSound, SetFileStreaming, (CKBOOL, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "bool GetFileStreaming()", asMETHODPR(CKWaveSound, GetFileStreaming, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void Play(float = 0, float = 1.0)", asMETHODPR(CKWaveSound, Play, (float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Resume()", asMETHODPR(CKWaveSound, Resume, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Rewind()", asMETHODPR(CKWaveSound, Rewind, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Stop(float = 0)", asMETHODPR(CKWaveSound, Stop, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Pause()", asMETHODPR(CKWaveSound, Pause, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKBOOL IsPlaying()", asMETHODPR(CKWaveSound, IsPlaying, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKBOOL IsPaused()", asMETHODPR(CKWaveSound, IsPaused, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetGain(float)", asMETHODPR(CKWaveSound, SetGain, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetGain()", asMETHODPR(CKWaveSound, GetGain, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPitch(float)", asMETHODPR(CKWaveSound, SetPitch, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPitch()", asMETHODPR(CKWaveSound, GetPitch, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPan(float)", asMETHODPR(CKWaveSound, SetPan, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPan()", asMETHODPR(CKWaveSound, GetPan, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKSOUNDHANDLE GetSource()", asMETHODPR(CKWaveSound, GetSource, (), CKSOUNDHANDLE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void PositionSound(CK3dEntity@, VxVector &in, VxVector &in, bool = false)", asMETHODPR(CKWaveSound, PositionSound, (CK3dEntity *, VxVector *, VxVector *, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CK3dEntity@ GetAttachedEntity()", asMETHODPR(CKWaveSound, GetAttachedEntity, (), CK3dEntity *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetPosition(VxVector &out)", asMETHODPR(CKWaveSound, GetPosition, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetDirection(VxVector &out)", asMETHODPR(CKWaveSound, GetDirection, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetSound3DInformation(VxVector &out, VxVector &out, float &out)", asMETHODPR(CKWaveSound, GetSound3DInformation, (VxVector &, VxVector &, float &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetCone(float, float, float)", asMETHODPR(CKWaveSound, SetCone, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetCone(float &out, float &out, float &out)", asMETHODPR(CKWaveSound, GetCone, (float *, float *, float *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetMinMaxDistance(float, float, uint = 1)", asMETHODPR(CKWaveSound, SetMinMaxDistance, (float, float, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetMinMaxDistance(float &out, float &out, uint &out)", asMETHODPR(CKWaveSound, GetMinMaxDistance, (float *, float *, CKDWORD *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetVelocity(VxVector &in)", asMETHODPR(CKWaveSound, SetVelocity, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetVelocity(VxVector &out)", asMETHODPR(CKWaveSound, GetVelocity, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetOrientation(VxVector &in, VxVector &in)", asMETHODPR(CKWaveSound, SetOrientation, (VxVector &, VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetOrientation(VxVector &out, VxVector &out)", asMETHODPR(CKWaveSound, GetOrientation, (VxVector &, VxVector &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR WriteData(NativeBuffer)", asMETHODPR(CKWaveSound, WriteData, (CKBYTE *, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Lock(uint, uint, NativeBuffer, NativeBuffer, CK_WAVESOUND_LOCKMODE)", asMETHODPR(CKWaveSound, Lock, (CKDWORD, CKDWORD, void **, CKDWORD *, void **, CKDWORD *, CK_WAVESOUND_LOCKMODE), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Unlock(NativeBuffer, NativeBuffer)", asMETHODPR(CKWaveSound, Unlock, (void *, CKDWORD, void *, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKDWORD GetPlayPosition()", asMETHODPR(CKWaveSound, GetPlayPosition, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "int GetPlayedMs()", asMETHODPR(CKWaveSound, GetPlayedMs, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Create(bool, const string &in)", asMETHODPR(CKWaveSound, Create, (CKBOOL, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Create(CK_WAVESOUND_TYPE, const CKWaveFormat &, int)", asMETHODPR(CKWaveSound, Create, (CK_WAVESOUND_TYPE, CKWaveFormat *, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetReader(CKSoundReader@)", asMETHODPR(CKWaveSound, SetReader, (CKSoundReader *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKSoundReader@ GetReader()", asMETHODPR(CKWaveSound, GetReader, (), CKSoundReader *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetDataToRead(int)", asMETHODPR(CKWaveSound, SetDataToRead, (int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Recreate(bool = false)", asMETHODPR(CKWaveSound, Recreate, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Release()", asMETHODPR(CKWaveSound, Release, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR TryRecreate()", asMETHODPR(CKWaveSound, TryRecreate, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void UpdatePosition(float)", asMETHODPR(CKWaveSound, UpdatePosition, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void UpdateFade()", asMETHODPR(CKWaveSound, UpdateFade, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR WriteDataFromReader()", asMETHODPR(CKWaveSound, WriteDataFromReader, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void FillWithBlanks(bool = false)", asMETHODPR(CKWaveSound, FillWithBlanks, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void InternalStop()", asMETHODPR(CKWaveSound, InternalStop, (), void), asCALL_THISCALL); assert(r >= 0);

    // Not existing in Virtools 2.1
    // r = engine->RegisterObjectMethod("CKWaveSound", "int GetDistanceFromCursor()", asMETHODPR(CKWaveSound, GetDistanceFromCursor, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKWaveSound", "void InternalSetGain(float)", asMETHODPR(CKWaveSound, InternalSetGain, (float), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKWaveSound", "void SaveSettings()", asMETHODPR(CKWaveSound, SaveSettings, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKWaveSound", "void RestoreSettings()", asMETHODPR(CKWaveSound, RestoreSettings, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKWaveSound", "NativePointer ReallocSource(NativePointer, int, int)", asMETHODPR(CKWaveSound, ReallocSource, (void *, int, int), void *), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKMidiSound(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSoundMembers<CKMidiSound>(engine, "CKMidiSound");

    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR SetSoundFileName(const string &in)", asMETHODPR(CKMidiSound, SetSoundFileName, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "string GetSoundFileName()", asMETHODPR(CKMidiSound, GetSoundFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiSound", "CKDWORD GetCurrentPos()", asMETHODPR(CKMidiSound, GetCurrentPos, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Play()", asMETHODPR(CKMidiSound, Play, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Stop()", asMETHODPR(CKMidiSound, Stop, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Pause(bool = true)", asMETHODPR(CKMidiSound, Pause, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiSound", "CKBOOL IsPlaying()", asMETHODPR(CKMidiSound, IsPlaying, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "CKBOOL IsPaused()", asMETHODPR(CKMidiSound, IsPaused, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Not existing in Virtools 2.1
    // r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR OpenFile()", asMETHODPR(CKMidiSound, OpenFile, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR CloseFile()", asMETHODPR(CKMidiSound, CloseFile, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Prepare()", asMETHODPR(CKMidiSound, Prepare, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Start()", asMETHODPR(CKMidiSound, Start, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKRenderObjectMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool IsInRenderContext(CKRenderContext@)", asMETHODPR(T, IsInRenderContext, (CKRenderContext*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsRootObject()", asMETHODPR(T, IsRootObject, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeRendered()", asMETHODPR(T, IsToBeRendered, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetZOrder(int)", asMETHODPR(T, SetZOrder, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetZOrder()", asMETHODPR(T, GetZOrder, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeRenderedLast()", asMETHODPR(T, IsToBeRenderedLast, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPreRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddPreRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePreRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemovePreRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, SetRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveRenderCallBack()", asMETHODPR(T, RemoveRenderCallBack, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPostRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(T, AddPostRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePostRenderCallBack(NativePointer, NativePointer)", asMETHODPR(T, RemovePostRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void RemoveAllCallbacks()", asMETHODPR(T, RemoveAllCallbacks, (), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKRenderObject") != 0) {
        RegisterClassRefCast<T, CKRenderObject>(engine, name, "CKRenderObject");
    }
}

void RegisterCKRenderObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKRenderObjectMembers<CKRenderObject>(engine, "CKRenderObject");
}

void RegisterCK2dEntity(asIScriptEngine *engine) {
}

void RegisterCKSprite(asIScriptEngine *engine) {
}

void RegisterCKSpriteText(asIScriptEngine *engine) {
}

void RegisterCK3dEntity(asIScriptEngine *engine) {
}

void RegisterCKCamera(asIScriptEngine *engine) {
}

void RegisterCKTargetCamera(asIScriptEngine *engine) {
}

void RegisterCKPlace(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    // RegisterCK3dEntityMembers<CKPlace>(engine, "CKPlace");
}

void RegisterCKCurvePoint(asIScriptEngine *engine) {
}

void RegisterCKSprite3D(asIScriptEngine *engine) {
}

void RegisterCKLight(asIScriptEngine *engine) {
}

void RegisterCKTargetLight(asIScriptEngine *engine) {
}

void RegisterCKCharacter(asIScriptEngine *engine) {
}

void RegisterCK3dObject(asIScriptEngine *engine) {
}

void RegisterCKBodyPart(asIScriptEngine *engine) {
}

void RegisterCKCurve(asIScriptEngine *engine) {
}

void RegisterCKGrid(asIScriptEngine *engine) {
}
