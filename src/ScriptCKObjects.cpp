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
#include "CKGrid.h"

// Containers
#include "CKDataArray.h"
#include "CKDebugContext.h"
#include "CKMemoryPool.h"

#include "ScriptUtils.h"
#include "ScriptFunctionInvoker.h"

template <typename T>
static void RegisterCKObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "void SetName(const string &in name, bool shared = false)", asMETHODPR(T, SetName, (CKSTRING, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetAppData()", asMETHODPR(T, GetAppData, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAppData(NativePointer data)", asMETHODPR(T, SetAppData, (void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Show(CK_OBJECT_SHOWOPTION show = CKSHOW)", asMETHODPR(T, Show, (CK_OBJECT_SHOWOPTION), void), asCALL_THISCALL); assert(r >= 0);
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
    r = engine->RegisterObjectMethod(name, "void ModifyObjectFlags(CKDWORD add, CKDWORD remove)", asMETHODPR(T, ModifyObjectFlags, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK_CLASSID GetClassID()", asMETHODPR(T, GetClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreSave(CKFile@ file, CKDWORD flags)", asMETHODPR(T, PreSave, (CKFile*, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ Save(CKFile@ file, CKDWORD flags)", asMETHODPR(T, Save, (CKFile*, CKDWORD), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Load(CKStateChunk@ chunk, CKFile@ file)", asMETHODPR(T, Load, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PostLoad()", asMETHODPR(T, PostLoad, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void PreDelete()", asMETHODPR(T, PreDelete, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPreDeletion()", asMETHODPR(T, CheckPreDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPostDeletion()", asMETHODPR(T, CheckPostDeletion, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetMemoryOccupation()", asMETHODPR(T, GetMemoryOccupation, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsObjectUsed(CKObject@ obj, CK_CLASSID cid)", asMETHODPR(T, IsObjectUsed, (CKObject *, CK_CLASSID), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR PrepareDependencies(CKDependenciesContext &in context)", asMETHODPR(T, PrepareDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemapDependencies(CKDependenciesContext &in context)", asMETHODPR(T, RemapDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Copy(CKObject@ obj, CKDependenciesContext &in context)", asMETHODPR(T, Copy, (CKObject&, CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool IsUpToDate()", asMETHODPR(T, IsUpToDate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPrivate()", asMETHODPR(T, IsPrivate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsNotToBeSaved()", asMETHODPR(T, IsNotToBeSaved, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInterfaceObj()", asMETHODPR(T, IsInterfaceObj, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(T, CKDestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID id, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(T, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(const XObjectArray &in ids, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asFUNCTIONPR([](T *obj, const XObjectArray &objects, CKDWORD flags, CKDependencies *deps) { return obj->CKDestroyObjects(objects.Begin(), objects.Size(), flags, deps); }, (T *, const XObjectArray &, CKDWORD, CKDependencies *), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID id)", asMETHODPR(T, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKObject") != 0) {
        RegisterClassRefCast<T, CKObject>(engine, name, "CKObject");
    }
}

template <>
static void RegisterCKObjectMembers<CKScene>(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "void SetName(const string &in name, bool shared = false)", asMETHODPR(CKScene, SetName, (CKSTRING, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetAppData()", asMETHODPR(CKScene, GetAppData, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAppData(NativePointer data)", asMETHODPR(CKScene, SetAppData, (void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Show(CK_OBJECT_SHOWOPTION show = CKSHOW)", asMETHODPR(CKScene, Show, (CK_OBJECT_SHOWOPTION), void), asCALL_THISCALL); assert(r >= 0);
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
    r = engine->RegisterObjectMethod(name, "void ModifyObjectFlags(CKDWORD add, CKDWORD remove)", asMETHODPR(CKObject, ModifyObjectFlags, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK_CLASSID GetClassID()", asMETHODPR(CKScene, GetClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PreSave(CKFile@ file, CKDWORD flags)", asMETHODPR(CKScene, PreSave, (CKFile*, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ Save(CKFile@ file, CKDWORD flags)", asMETHODPR(CKScene, Save, (CKFile*, CKDWORD), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Load(CKStateChunk@ chunk, CKFile@ file)", asMETHODPR(CKScene, Load, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void PostLoad()", asMETHODPR(CKScene, PostLoad, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void PreDelete()", asMETHODPR(CKScene, PreDelete, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPreDeletion()", asMETHODPR(CKScene, CheckPreDeletion, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CheckPostDeletion()", asMETHODPR(CKScene, CheckPostDeletion, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetMemoryOccupation()", asMETHODPR(CKScene, GetMemoryOccupation, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsObjectUsed(CKObject@ obj, CK_CLASSID cid)", asMETHODPR(CKScene, IsObjectUsed, (CKObject *, CK_CLASSID), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR PrepareDependencies(CKDependenciesContext &in context)", asMETHODPR(CKScene, PrepareDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemapDependencies(CKDependenciesContext &in context)", asMETHODPR(CKScene, RemapDependencies, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Copy(CKObject@ obj, CKDependenciesContext &in context)", asMETHODPR(CKScene, Copy, (CKObject&, CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool IsUpToDate()", asMETHODPR(CKScene, IsUpToDate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPrivate()", asMETHODPR(CKScene, IsPrivate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsNotToBeSaved()", asMETHODPR(CKScene, IsNotToBeSaved, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInterfaceObj()", asMETHODPR(CKScene, IsInterfaceObj, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKScene, CKDestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID id, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKScene, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(const XObjectArray &in ids, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asFUNCTIONPR([](CKObject *obj, const XObjectArray &objects, CKDWORD flags, CKDependencies *deps) { return obj->CKDestroyObjects(objects.Begin(), objects.Size(), flags, deps); }, (CKObject *, const XObjectArray &, CKDWORD, CKDependencies *), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID id)", asMETHODPR(CKScene, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod(name, "CKERROR GetValue(NativePointer buf)", asMETHODPR(T, GetValue, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetReadDataPtr()", asMETHODPR(T, GetReadDataPtr, (), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterIn@ GetSharedSource()", asMETHODPR(T, GetSharedSource, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetRealSource()", asMETHODPR(T, GetRealSource, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetDirectSource()", asMETHODPR(T, GetDirectSource, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SetDirectSource(CKParameter@)", asMETHODPR(T, SetDirectSource, (CKParameter*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR ShareSourceWith(CKParameterIn@)", asMETHODPR(T, ShareSourceWith, (CKParameterIn*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetType(CKParameterType type, bool updateSource = false, const string &in newName = void)", asMETHODPR(T, SetType, (CKParameterType, CKBOOL, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetGUID(CKGUID guid, bool updateSource = false, const string &in newName = void)", asMETHODPR(T, SetGUID, (CKGUID, CKBOOL, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterType GetType()", asMETHODPR(T, GetType, (), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKGUID GetGUID()", asMETHODPR(T, GetGUID, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetOwner(CKObject@ obj)", asMETHODPR(T, SetOwner, (CKObject*), void), asCALL_THISCALL); assert(r >= 0);
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

    r = engine->RegisterObjectMethod(name, "CKObject@ GetValueObject(bool update = true)", asMETHODPR(T, GetValueObject, (CKBOOL), CKObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR GetValue(NativePointer buf, bool update = true)", asMETHODPR(T, GetValue, (void*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SetValue(NativePointer buf, int size = 0)", asMETHODPR(T, SetValue, (const void*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CopyValue(CKParameter@ param, bool updateParam = true)", asMETHODPR(T, CopyValue, (CKParameter*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool IsCompatibleWith(CKParameter@ param)", asMETHODPR(T, IsCompatibleWith, (CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetDataSize()", asMETHODPR(T, GetDataSize, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetReadDataPtr(bool update = true)", asMETHODPR(T, GetReadDataPtr, (CKBOOL), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetWriteDataPtr()", asMETHODPR(T, GetWriteDataPtr, (), void*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR SetStringValue(const string &in value)", asMETHODPR(T, SetStringValue, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetStringValue(const string &in, bool = true)", asMETHODPR(T, GetStringValue, (CKSTRING, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKParameterType GetType()", asMETHODPR(T, GetType, (), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetType(CKParameterType type)", asMETHODPR(T, SetType, (CKParameterType), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKGUID GetGUID()", asMETHODPR(T, GetGUID, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetGUID(CKGUID guid)", asMETHODPR(T, SetGUID, (CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_CLASSID GetParameterClassID()", asMETHODPR(T, GetParameterClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetOwner(CKObject@ obj)", asMETHODPR(T, SetOwner, (CKObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ GetOwner()", asMETHODPR(T, GetOwner, (), CKObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Enable(bool act = true)", asMETHODPR(T, Enable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsEnabled()", asMETHODPR(T, IsEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod(name, "CKERROR CreateDefaultValue()", asMETHODPR(T, CreateDefaultValue, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "void MessageDeleteAfterUse(bool act)", asMETHODPR(T, MessageDeleteAfterUse, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterTypeDesc &GetParameterType()", asMETHODPR(T, GetParameterType, (), CKParameterTypeDesc*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "bool IsCandidateForFixedSize(CKParameterTypeDesc &in type)", asMETHODPR(T, IsCandidateForFixedSize, (CKParameterTypeDesc*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod(name, "CKERROR AddDestination(CKParameter@ param, bool checkType = true)", asMETHODPR(T, AddDestination, (CKParameter*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveDestination(CKParameter@ param)", asMETHODPR(T, RemoveDestination, (CKParameter*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetDestinationCount()", asMETHODPR(T, GetDestinationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameter@ GetDestination(int pos)", asMETHODPR(T, GetDestination, (int), CKParameter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllDestinations()", asMETHODPR(T, RemoveAllDestinations, (), void), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod(name, "void Update()", asMETHODPR(T, Update, (), void), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod(name, "void SetAsMyselfParameter(bool act)", asMETHODPR(T, SetAsMyselfParameter, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
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
    r = engine->RegisterObjectMethod(name, "void Reconstruct(const string &in name, CKGUID opGuid, CKGUID resGuid, CKGUID p1Guid, CKGUID p2Guid)", asMETHODPR(T, Reconstruct, (CKSTRING, CKGUID, CKGUID, CKGUID, CKGUID), void), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKERROR SetOutBehaviorIO(CKBehaviorIO@ ckbioin)", asMETHODPR(CKBehaviorLink, SetOutBehaviorIO, (CKBehaviorIO*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKERROR SetInBehaviorIO(CKBehaviorIO@ ckbioout)", asMETHODPR(CKBehaviorLink, SetInBehaviorIO, (CKBehaviorIO*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKBehaviorIO@ GetOutBehaviorIO()", asMETHODPR(CKBehaviorLink, GetOutBehaviorIO, (), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKBehaviorIO@ GetInBehaviorIO()", asMETHODPR(CKBehaviorLink, GetInBehaviorIO, (), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorLink", "int GetActivationDelay()", asMETHODPR(CKBehaviorLink, GetActivationDelay, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetActivationDelay(int delay)", asMETHODPR(CKBehaviorLink, SetActivationDelay, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void ResetActivationDelay()", asMETHODPR(CKBehaviorLink, ResetActivationDelay, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetInitialActivationDelay(int delay)", asMETHODPR(CKBehaviorLink, SetInitialActivationDelay, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "int GetInitialActivationDelay()", asMETHODPR(CKBehaviorLink, GetInitialActivationDelay, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorLink", "CKDWORD GetFlags()", asMETHODPR(CKBehaviorLink, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorLink", "void SetFlags(CKDWORD flags)", asMETHODPR(CKBehaviorLink, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKBehaviorIO(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKBehaviorIO>(engine, "CKBehaviorIO");

    r = engine->RegisterObjectMethod("CKBehaviorIO", "void SetType(int type)", asMETHODPR(CKBehaviorIO, SetType, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "int GetType()", asMETHODPR(CKBehaviorIO, GetType, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorIO", "void Activate(bool active = true)", asMETHODPR(CKBehaviorIO, Activate, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "bool IsActive()", asMETHODPR(CKBehaviorIO, IsActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorIO", "CKBehavior@ GetOwner()", asMETHODPR(CKBehaviorIO, GetOwner, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorIO", "void SetOwner(CKBehavior@ owner)", asMETHODPR(CKBehaviorIO, SetOwner, (CKBehavior*), void), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKBehaviorIO", "void SortLinks()", asMETHODPR(CKBehaviorIO, SortLinks, (), void), asCALL_THISCALL); assert(r >= 0);
}

void CKRenderContext_AddPreRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc, bool temporary = false) {
    scriptFunc->AddRef();
    context->AddPreRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc, temporary);
}

void CKRenderContext_RemovePreRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc) {
    context->RemovePreRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc);
    scriptFunc->Release();
}

void CKRenderContext_AddPostRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc, bool temporary = false) {
    scriptFunc->AddRef();
    context->AddPostRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc, temporary);
}

void CKRenderContext_RemovePostRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc) {
    context->RemovePostRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc);
    scriptFunc->Release();
}

void CKRenderContext_AddPostSpriteRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc, bool temporary = false) {
    scriptFunc->AddRef();
    context->AddPostSpriteRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc, temporary);
}

void CKRenderContext_RemovePostSpriteRenderCallBack(CKRenderContext *context, asIScriptFunction *scriptFunc) {
    context->RemovePostSpriteRenderCallBack(ScriptFunctionInvoker<CK_RENDERCALLBACK>::Invoke, scriptFunc);
    scriptFunc->Release();
}

void RegisterCKRenderContext(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKRenderContext>(engine, "CKRenderContext");

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddObject(CKRenderObject@ ojb)", asMETHODPR(CKRenderContext, AddObject, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddObjectWithHierarchy(CKRenderObject@ obj)", asMETHODPR(CKRenderContext, AddObjectWithHierarchy, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RemoveObject(CKRenderObject@ obj)", asMETHODPR(CKRenderContext, RemoveObject, (CKRenderObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool IsObjectAttached(CKRenderObject@ obj)", asMETHODPR(CKRenderContext, IsObjectAttached, (CKRenderObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "const XObjectArray &Compute3dRootObjects()", asMETHODPR(CKRenderContext, Compute3dRootObjects, (), const XObjectArray&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const XObjectArray &Compute2dRootObjects()", asMETHODPR(CKRenderContext, Compute2dRootObjects, (), const XObjectArray&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CK2dEntity@ Get2dRoot(bool background)", asMETHODPR(CKRenderContext, Get2dRoot, (CKBOOL), CK2dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void DetachAll()", asMETHODPR(CKRenderContext, DetachAll, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void ForceCameraSettingsUpdate()", asMETHODPR(CKRenderContext, ForceCameraSettingsUpdate, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void PrepareCameras(CK_RENDER_FLAGS flags = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, PrepareCameras, (CK_RENDER_FLAGS), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Clear(CK_RENDER_FLAGS flags = CK_RENDER_USECURRENTSETTINGS, CKDWORD = 0)", asMETHODPR(CKRenderContext, Clear, (CK_RENDER_FLAGS, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR DrawScene(CK_RENDER_FLAGS flags = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, DrawScene, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR BackToFront(CK_RENDER_FLAGS flags = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, BackToFront, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Render(CK_RENDER_FLAGS flags = CK_RENDER_USECURRENTSETTINGS)", asMETHODPR(CKRenderContext, Render, (CK_RENDER_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKRenderContext", "void AddPreRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(CKRenderContext, AddPreRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePreRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePreRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPreRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)",
        asFUNCTION(CKRenderContext_AddPreRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePreRenderCallBack(CK_RENDERCALLBACK@ callback)",
        asFUNCTION(CKRenderContext_RemovePreRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostRenderCallBack(NativePointer, NativePointer, bool = false, bool = false)", asMETHODPR(CKRenderContext, AddPostRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePostRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)",
        asFUNCTION(CKRenderContext_AddPostRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostRenderCallBack(CK_RENDERCALLBACK@ callback)",
        asFUNCTION(CKRenderContext_RemovePostRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostSpriteRenderCallBack(NativePointer, NativePointer, bool = false)", asMETHODPR(CKRenderContext, AddPostSpriteRenderCallBack, (CK_RENDERCALLBACK, void*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostSpriteRenderCallBack(NativePointer, NativePointer)", asMETHODPR(CKRenderContext, RemovePostSpriteRenderCallBack, (CK_RENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddPostSpriteRenderCallBack(CK_RENDERCALLBACK@ callback, bool temporary = false)",
        asFUNCTION(CKRenderContext_AddPostSpriteRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void RemovePostSpriteRenderCallBack(CK_RENDERCALLBACK@ callback)",
        asFUNCTION(CKRenderContext_RemovePostSpriteRenderCallBack), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDrawPrimitiveData &GetDrawPrimitiveStructure(CKRST_DPFLAGS flags, int vretexCount)", asMETHODPR(CKRenderContext, GetDrawPrimitiveStructure, (CKRST_DPFLAGS, int), VxDrawPrimitiveData*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKWORD &GetDrawPrimitiveIndices(int indicesCount)", asMETHODPR(CKRenderContext, GetDrawPrimitiveIndices, (int), CKWORD*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void Transform(VxVector &out dest, const VxVector &in src, CK3dEntity@ ref = null)", asMETHODPR(CKRenderContext, Transform, (VxVector*, VxVector*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void TransformVertices(int vertexCount, VxTransformData &out data, CK3dEntity@ ref = null)", asMETHODPR(CKRenderContext, TransformVertices, (int, VxTransformData*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR GoFullScreen(int width = 640, int height = 480, int bpp = -1, int driver = 0, int refreshRate = 0)", asMETHODPR(CKRenderContext, GoFullScreen, (int, int, int, int, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR StopFullScreen()", asMETHODPR(CKRenderContext, StopFullScreen, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool IsFullScreen()", asMETHODPR(CKRenderContext, IsFullScreen, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int GetDriverIndex()", asMETHODPR(CKRenderContext, GetDriverIndex, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool ChangeDriver(int newDriver)", asMETHODPR(CKRenderContext, ChangeDriver, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "WIN_HANDLE GetWindowHandle()", asMETHODPR(CKRenderContext, GetWindowHandle, (), WIN_HANDLE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void ScreenToClient(Vx2DVector &in iPoint, Vx2DVector &out oPoint)", asFUNCTIONPR([](CKRenderContext *dev, Vx2DVector *iPoint, Vx2DVector *oPoint) { dev->ScreenToClient(iPoint); *oPoint = *iPoint; }, (CKRenderContext *, Vx2DVector *, Vx2DVector *), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void ClientToScreen(Vx2DVector &in iPoint, Vx2DVector &out oPoint)", asFUNCTIONPR([](CKRenderContext *dev, Vx2DVector *iPoint, Vx2DVector *oPoint) { dev->ClientToScreen(iPoint); *oPoint = *iPoint; }, (CKRenderContext *, Vx2DVector *, Vx2DVector *), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR SetWindowRect(const VxRect &in rect, CKDWORD flags = 0)", asMETHODPR(CKRenderContext, SetWindowRect, (VxRect&, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetWindowRect(VxRect &out rect, bool screenRelative = false)", asMETHODPR(CKRenderContext, GetWindowRect, (VxRect&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int GetHeight()", asMETHODPR(CKRenderContext, GetHeight, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int GetWidth()", asMETHODPR(CKRenderContext, GetWidth, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR Resize(int posX = 0, int poxY = 0, int sizeX = 0, int sizeY = 0, CKDWORD flags = 0)", asMETHODPR(CKRenderContext, Resize, (int, int, int, int, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetViewRect(const VxRect &in rect)", asMETHODPR(CKRenderContext, SetViewRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetViewRect(VxRect &out rect)", asMETHODPR(CKRenderContext, GetViewRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VX_PIXELFORMAT GetPixelFormat(int &in bpp = void, int &in zbpp = void, int &in stencilBpp = void)", asMETHODPR(CKRenderContext, GetPixelFormat, (int*, int*, int*), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetState(VXRENDERSTATETYPE state, CKDWORD value)", asMETHODPR(CKRenderContext, SetState, (VXRENDERSTATETYPE, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetState(VXRENDERSTATETYPE state)", asMETHODPR(CKRenderContext, GetState, (VXRENDERSTATETYPE), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetTexture(CKTexture@ tex, bool clamped = false, int stage = 0)", asMETHODPR(CKRenderContext, SetTexture, (CKTexture*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetTextureStageState(CKRST_TEXTURESTAGESTATETYPE state, CKDWORD value, int stage = 0)", asMETHODPR(CKRenderContext, SetTextureStageState, (CKRST_TEXTURESTAGESTATETYPE, CKDWORD, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKRenderContext", "CKRasterizerContext@ GetRasterizerContext()", asMETHODPR(CKRenderContext, GetRasterizerContext, (), CKRasterizerContext*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetClearBackground(bool clearBack = true)", asMETHODPR(CKRenderContext, SetClearBackground, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetClearBackground()", asMETHODPR(CKRenderContext, GetClearBackground, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetClearZBuffer(bool clearZ = true)", asMETHODPR(CKRenderContext, SetClearZBuffer, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetClearZBuffer()", asMETHODPR(CKRenderContext, GetClearZBuffer, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetGlobalRenderMode(VxShadeType &out shading, bool &out texture, bool &out wireframe)", asMETHODPR(CKRenderContext, GetGlobalRenderMode, (VxShadeType*, CKBOOL*, CKBOOL*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetGlobalRenderMode(VxShadeType shading = GouraudShading, bool texture = true, bool wireframe = false)", asMETHODPR(CKRenderContext, SetGlobalRenderMode, (VxShadeType, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentRenderOptions(CKDWORD flags)", asMETHODPR(CKRenderContext, SetCurrentRenderOptions, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetCurrentRenderOptions()", asMETHODPR(CKRenderContext, GetCurrentRenderOptions, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void ChangeCurrentRenderOptions(CKDWORD add, CKDWORD remove)", asMETHODPR(CKRenderContext, ChangeCurrentRenderOptions, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentExtents(const VxRect &in extents)", asMETHODPR(CKRenderContext, SetCurrentExtents, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetCurrentExtents(VxRect &out extents)", asMETHODPR(CKRenderContext, GetCurrentExtents, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetAmbientLight(float r, float g, float b)", asMETHODPR(CKRenderContext, SetAmbientLight, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetAmbientLight(CKDWORD color)", asMETHODPR(CKRenderContext, SetAmbientLight, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetAmbientLight()", asMETHODPR(CKRenderContext, GetAmbientLight, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogMode(VXFOG_MODE mode)", asMETHODPR(CKRenderContext, SetFogMode, (VXFOG_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogStart(float start)", asMETHODPR(CKRenderContext, SetFogStart, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogEnd(float end)", asMETHODPR(CKRenderContext, SetFogEnd, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogDensity(float density)", asMETHODPR(CKRenderContext, SetFogDensity, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetFogColor(CKDWORD color)", asMETHODPR(CKRenderContext, SetFogColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VXFOG_MODE GetFogMode()", asMETHODPR(CKRenderContext, GetFogMode, (), VXFOG_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetFogColor()", asMETHODPR(CKRenderContext, GetFogColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogStart()", asMETHODPR(CKRenderContext, GetFogStart, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogEnd()", asMETHODPR(CKRenderContext, GetFogEnd, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "float GetFogDensity()", asMETHODPR(CKRenderContext, GetFogDensity, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool DrawPrimitive(VXPRIMITIVETYPE pType, NativePointer indices, int indexCount, VxDrawPrimitiveData &in data)", asMETHODPR(CKRenderContext, DrawPrimitive, (VXPRIMITIVETYPE, CKWORD*, int, VxDrawPrimitiveData*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetWorldTransformationMatrix(const VxMatrix &in mat)", asMETHODPR(CKRenderContext, SetWorldTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetProjectionTransformationMatrix(const VxMatrix &in mat)", asMETHODPR(CKRenderContext, SetProjectionTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetViewTransformationMatrix(const VxMatrix &in mat)", asMETHODPR(CKRenderContext, SetViewTransformationMatrix, (const VxMatrix&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetWorldTransformationMatrix()", asMETHODPR(CKRenderContext, GetWorldTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetProjectionTransformationMatrix()", asMETHODPR(CKRenderContext, GetProjectionTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "const VxMatrix &GetViewTransformationMatrix()", asMETHODPR(CKRenderContext, GetViewTransformationMatrix, (), const VxMatrix&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetUserClipPlane(CKDWORD clipPlaneIndex, const VxPlane &in planeEquation)", asMETHODPR(CKRenderContext, SetUserClipPlane, (CKDWORD, const VxPlane&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool GetUserClipPlane(CKDWORD clipPlaneIndex, VxPlane &out planeEquation)", asMETHODPR(CKRenderContext, GetUserClipPlane, (CKDWORD, VxPlane&), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKRenderObject@ Pick(int x, int y, CKPICKRESULT &out res, bool ignoreUnpickable = false)", asMETHODPR(CKRenderContext, Pick, (int, int, CKPICKRESULT*, CKBOOL), CKRenderObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKRenderObject@ Pick(CKPOINT pt, CKPICKRESULT &out res, bool ignoreUnpickable = false)", asMETHODPR(CKRenderContext, Pick, (CKPOINT, CKPICKRESULT*, CKBOOL), CKRenderObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR RectPick(const VxRect &in rect, XObjectPointerArray &out objects, bool intersect = true)", asMETHODPR(CKRenderContext, RectPick, (const VxRect&, XObjectPointerArray&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AttachViewpointToCamera(CKCamera@ cam)", asMETHODPR(CKRenderContext, AttachViewpointToCamera, (CKCamera*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void DetachViewpointFromCamera()", asMETHODPR(CKRenderContext, DetachViewpointFromCamera, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKCamera@ GetAttachedCamera()", asMETHODPR(CKRenderContext, GetAttachedCamera, (), CKCamera*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CK3dEntity@ GetViewpoint()", asMETHODPR(CKRenderContext, GetViewpoint, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKMaterial@ GetBackgroundMaterial()", asMETHODPR(CKRenderContext, GetBackgroundMaterial, (), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetBoundingBox(VxBbox &out bbox)", asMETHODPR(CKRenderContext, GetBoundingBox, (VxBbox*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void GetStats(VxStats &out stats)", asMETHODPR(CKRenderContext, GetStats, (VxStats*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetCurrentMaterial(CKMaterial@ mat, bool lit = true)", asMETHODPR(CKRenderContext, SetCurrentMaterial, (CKMaterial*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void Activate(bool active = true)", asMETHODPR(CKRenderContext, Activate, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "int DumpToMemory(const VxRect &in rect, VXBUFFER_TYPE bufferType, VxImageDescEx &out desc)", asMETHODPR(CKRenderContext, DumpToMemory, (const VxRect*, VXBUFFER_TYPE, VxImageDescEx&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int CopyToVideo(const VxRect &in rect, VXBUFFER_TYPE bufferType, VxImageDescEx &out desc)", asMETHODPR(CKRenderContext, CopyToVideo, (const VxRect*, VXBUFFER_TYPE, VxImageDescEx&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "CKERROR DumpToFile(const string &in filename, const VxRect &in rect, VXBUFFER_TYPE bufferType)", asMETHODPR(CKRenderContext, DumpToFile, (CKSTRING, const VxRect*, VXBUFFER_TYPE), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDirectXData &GetDirectXInfo()", asMETHODPR(CKRenderContext, GetDirectXInfo, (), VxDirectXData*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void WarnEnterThread()", asMETHODPR(CKRenderContext, WarnEnterThread, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void WarnExitThread()", asMETHODPR(CKRenderContext, WarnExitThread, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CK2dEntity@ Pick2D(const Vx2DVector &in v)", asMETHODPR(CKRenderContext, Pick2D, (const Vx2DVector&), CK2dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "bool SetRenderTarget(CKTexture@ texture = null, int cubeMapFace = 0)", asMETHODPR(CKRenderContext, SetRenderTarget, (CKTexture*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void AddRemoveSequence(bool start)", asMETHODPR(CKRenderContext, AddRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void SetTransparentMode(bool trans)", asMETHODPR(CKRenderContext, SetTransparentMode, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void AddDirtyRect(const CKRECT &in rect)", asMETHODPR(CKRenderContext, AddDirtyRect, (CKRECT*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void RestoreScreenBackup()", asMETHODPR(CKRenderContext, RestoreScreenBackup, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "CKDWORD GetStencilFreeMask()", asMETHODPR(CKRenderContext, GetStencilFreeMask, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void UsedStencilBits(CKDWORD stencilBits)", asMETHODPR(CKRenderContext, UsedStencilBits, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "int GetFirstFreeStencilBits()", asMETHODPR(CKRenderContext, GetFirstFreeStencilBits, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "VxDrawPrimitiveData &LockCurrentVB(CKDWORD vertexCount)", asMETHODPR(CKRenderContext, LockCurrentVB, (CKDWORD), VxDrawPrimitiveData*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "bool ReleaseCurrentVB()", asMETHODPR(CKRenderContext, ReleaseCurrentVB, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetTextureMatrix(const VxMatrix &in mat, int stage = 0)", asMETHODPR(CKRenderContext, SetTextureMatrix, (const VxMatrix&, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderContext", "void SetStereoParameters(float eyeSeparation, float focalLength)", asMETHODPR(CKRenderContext, SetStereoParameters, (float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderContext", "void GetStereoParameters(float &out eyeSeparation, float &out focalLength)", asMETHODPR(CKRenderContext, GetStereoParameters, (float&, float&), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKSynchroObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKSynchroObject>(engine, "CKSynchroObject");

    r = engine->RegisterObjectMethod("CKSynchroObject", "void Reset()", asMETHODPR(CKSynchroObject, Reset, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "void SetRendezVousNumberOfWaiters(int waiters)", asMETHODPR(CKSynchroObject, SetRendezVousNumberOfWaiters, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "int GetRendezVousNumberOfWaiters()", asMETHODPR(CKSynchroObject, GetRendezVousNumberOfWaiters, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "bool CanIPassRendezVous(CKBeObject@ asker)", asMETHODPR(CKSynchroObject, CanIPassRendezVous, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "int GetRendezVousNumberOfArrivedObjects()", asMETHODPR(CKSynchroObject, GetRendezVousNumberOfArrivedObjects, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSynchroObject", "CKBeObject@ GetRendezVousArrivedObject(int pos)", asMETHODPR(CKSynchroObject, GetRendezVousArrivedObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
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
    r = engine->RegisterObjectMethod("CKCriticalSectionObject", "bool EnterCriticalSection(CKBeObject@ asker)", asMETHODPR(CKCriticalSectionObject, EnterCriticalSection, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCriticalSectionObject", "bool LeaveCriticalSection(CKBeObject@ asker)", asMETHODPR(CKCriticalSectionObject, LeaveCriticalSection, (CKBeObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKKinematicChain(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKKinematicChain>(engine, "CKKinematicChain");

    r = engine->RegisterObjectMethod("CKKinematicChain", "float GetChainLength(CKBodyPart@ end = null)", asMETHODPR(CKKinematicChain, GetChainLength, (CKBodyPart*), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "int GetChainBodyCount(CKBodyPart@ end = null)", asMETHODPR(CKKinematicChain, GetChainBodyCount, (CKBodyPart*), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetStartEffector()", asMETHODPR(CKKinematicChain, GetStartEffector, (), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR SetStartEffector(CKBodyPart@ start)", asMETHODPR(CKKinematicChain, SetStartEffector, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetEffector(int pos)", asMETHODPR(CKKinematicChain, GetEffector, (int), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKBodyPart@ GetEndEffector()", asMETHODPR(CKKinematicChain, GetEndEffector, (), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR SetEndEffector(CKBodyPart@ end)", asMETHODPR(CKKinematicChain, SetEndEffector, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKKinematicChain", "CKERROR IKSetEffectorPos(const VxVector &in pos, CK3dEntity@ ref = null, CKBodyPart@ body = null)", asMETHODPR(CKKinematicChain, IKSetEffectorPos, (VxVector*, CK3dEntity*, CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKLayer(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKObjectMembers<CKLayer>(engine, "CKLayer");

    r = engine->RegisterObjectMethod("CKLayer", "void SetType(int type)", asMETHODPR(CKLayer, SetType, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "int GetType()", asMETHODPR(CKLayer, GetType, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "void SetFormat(int format)", asMETHODPR(CKLayer, SetFormat, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "int GetFormat()", asMETHODPR(CKLayer, GetFormat, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "void SetValue(int x, int y, NativePointer val)", asMETHODPR(CKLayer, SetValue, (int, int, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "void GetValue(int x, int y, NativePointer val)", asMETHODPR(CKLayer, GetValue, (int, int, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "bool SetValue2(int x, int y, NativePointer val)", asMETHODPR(CKLayer, SetValue2, (int, int, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "bool GetValue2(int x, int y, NativePointer val)", asMETHODPR(CKLayer, GetValue2, (int, int, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "CKSquare &GetSquareArray()", asMETHODPR(CKLayer, GetSquareArray, (), CKSquare*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "void SetSquareArray(CKSquare &sqArray)", asMETHODPR(CKLayer, SetSquareArray, (CKSquare*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "void SetVisible(bool vis = true)", asMETHODPR(CKLayer, SetVisible, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKLayer", "bool IsVisible()", asMETHODPR(CKLayer, IsVisible, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKLayer", "void InitOwner(CK_ID owner)", asMETHODPR(CKLayer, InitOwner, (CK_ID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "void SetOwner(CK_ID owner)", asMETHODPR(CKLayer, SetOwner, (CK_ID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLayer", "uint GetOwner()", asMETHODPR(CKLayer, GetOwner, (), CK_ID), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKSceneObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool IsActiveInScene(CKScene@ scene)", asMETHODPR(T, IsActiveInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsActiveInCurrentScene()", asMETHODPR(T, IsActiveInCurrentScene, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInScene(CKScene@ scene)", asMETHODPR(T, IsInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetSceneInCount()", asMETHODPR(T, GetSceneInCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKScene@ GetSceneIn(int index)", asMETHODPR(T, GetSceneIn, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("CKBehavior", "void SetType(CK_BEHAVIOR_TYPE type)", asMETHODPR(CKBehavior, SetType, (CK_BEHAVIOR_TYPE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetFlags(CK_BEHAVIOR_FLAGS flags)", asMETHODPR(CKBehavior, SetFlags, (CK_BEHAVIOR_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CK_BEHAVIOR_FLAGS GetFlags()", asMETHODPR(CKBehavior, GetFlags, (), CK_BEHAVIOR_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CK_BEHAVIOR_FLAGS ModifyFlags(CKDWORD add, CKDWORD remove)", asMETHODPR(CKBehavior, ModifyFlags, (CKDWORD, CKDWORD), CK_BEHAVIOR_FLAGS), asCALL_THISCALL); assert(r >= 0);

    // BuildingBlock or Graph
    r = engine->RegisterObjectMethod("CKBehavior", "void UseGraph()", asMETHODPR(CKBehavior, UseGraph, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void UseFunction()", asMETHODPR(CKBehavior, UseFunction, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int IsUsingFunction()", asMETHODPR(CKBehavior, IsUsingFunction, (), int), asCALL_THISCALL); assert(r >= 0);

    // Targetable Behavior
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsTargetable()", asMETHODPR(CKBehavior, IsTargetable, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBeObject@ GetTarget()", asMETHODPR(CKBehavior, GetTarget, (), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR UseTarget(bool use = true)", asMETHODPR(CKBehavior, UseTarget, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsUsingTarget()", asMETHODPR(CKBehavior, IsUsingTarget, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ GetTargetParameter()", asMETHODPR(CKBehavior, GetTargetParameter, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetAsTargetable(bool target = true)", asMETHODPR(CKBehavior, SetAsTargetable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ ReplaceTargetParameter(CKParameterIn@ targetParam)", asMETHODPR(CKBehavior, ReplaceTargetParameter, (CKParameterIn*), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ RemoveTargetParameter()", asMETHODPR(CKBehavior, RemoveTargetParameter, (), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);

    // Compatible Class ID
    r = engine->RegisterObjectMethod("CKBehavior", "CK_CLASSID GetCompatibleClassID()", asMETHODPR(CKBehavior, GetCompatibleClassID, (), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetCompatibleClassID(CK_CLASSID)", asMETHODPR(CKBehavior, SetCompatibleClassID, (CK_CLASSID), void), asCALL_THISCALL); assert(r >= 0);

    // Function
    r = engine->RegisterObjectMethod("CKBehavior", "void SetFunction(NativePointer fct)", asMETHODPR(CKBehavior, SetFunction, (CKBEHAVIORFCT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetFunction()", asMETHODPR(CKBehavior, GetFunction, (), CKBEHAVIORFCT), asCALL_THISCALL); assert(r >= 0);

    // Callbacks
    r = engine->RegisterObjectMethod("CKBehavior", "void SetCallbackFunction(NativePointer fct)", asMETHODPR(CKBehavior, SetCallbackFunction, (CKBEHAVIORCALLBACKFCT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int CallCallbackFunction(CKDWORD message)", asMETHODPR(CKBehavior, CallCallbackFunction, (CKDWORD), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int CallSubBehaviorsCallbackFunction(CKDWORD message, CKGUID &in behguid = void)", asMETHODPR(CKBehavior, CallSubBehaviorsCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);

    // Execution
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsActive()", asMETHODPR(CKBehavior, IsActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int Execute(float delta)", asMETHODPR(CKBehavior, Execute, (float), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsParentScriptActiveInScene(CKScene@ scn)", asMETHODPR(CKBehavior, IsParentScriptActiveInScene, (CKScene*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetShortestDelay(CKBehavior@ beh)", asMETHODPR(CKBehavior, GetShortestDelay, (CKBehavior*), int), asCALL_THISCALL); assert(r >= 0);

    // Owner and Parent
    r = engine->RegisterObjectMethod("CKBehavior", "CKBeObject@ GetOwner()", asMETHODPR(CKBehavior, GetOwner, (), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ GetParent()", asMETHODPR(CKBehavior, GetParent, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ GetOwnerScript()", asMETHODPR(CKBehavior, GetOwnerScript, (), CKBehavior*), asCALL_THISCALL); assert(r >= 0);

    // Prototypes
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFromPrototype(CKBehaviorPrototype &in proto)", asMETHODPR(CKBehavior, InitFromPrototype, (CKBehaviorPrototype*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFromGuid(CKGUID guid)", asMETHODPR(CKBehavior, InitFromGuid, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFctPtrFromGuid(CKGUID guid)", asMETHODPR(CKBehavior, InitFctPtrFromGuid, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR InitFctPtrFromPrototype(CKBehaviorPrototype &in proto)", asMETHODPR(CKBehavior, InitFctPtrFromPrototype, (CKBehaviorPrototype*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKGUID GetPrototypeGuid()", asMETHODPR(CKBehavior, GetPrototypeGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorPrototype &GetPrototype()", asMETHODPR(CKBehavior, GetPrototype, (), CKBehaviorPrototype*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "string GetPrototypeName()", asMETHODPR(CKBehavior, GetPrototypeName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKDWORD GetVersion()", asMETHODPR(CKBehavior, GetVersion, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetVersion(CKDWORD version)", asMETHODPR(CKBehavior, SetVersion, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    // Outputs
    r = engine->RegisterObjectMethod("CKBehavior", "void ActivateOutput(int pos, bool active = true)", asMETHODPR(CKBehavior, ActivateOutput, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsOutputActive(int pos)", asMETHODPR(CKBehavior, IsOutputActive, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ RemoveOutput(int pos)", asMETHODPR(CKBehavior, RemoveOutput, (int), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR DeleteOutput(int pos)", asMETHODPR(CKBehavior, DeleteOutput, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ GetOutput(int pos)", asMETHODPR(CKBehavior, GetOutput, (int), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetOutputCount()", asMETHODPR(CKBehavior, GetOutputCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetOutputPosition(CKBehaviorIO@ pbio)", asMETHODPR(CKBehavior, GetOutputPosition, (CKBehaviorIO*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int AddOutput(const string &in name)", asMETHODPR(CKBehavior, AddOutput, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ ReplaceOutput(int pos, CKBehaviorIO@ io)", asMETHODPR(CKBehavior, ReplaceOutput, (int, CKBehaviorIO*), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ CreateOutput(const string &in name)", asMETHODPR(CKBehavior, CreateOutput, (CKSTRING), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);

    // Inputs
    r = engine->RegisterObjectMethod("CKBehavior", "void ActivateInput(int pos, bool active = true)", asMETHODPR(CKBehavior, ActivateInput, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsInputActive(int pos)", asMETHODPR(CKBehavior, IsInputActive, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ RemoveInput(int pos)", asMETHODPR(CKBehavior, RemoveInput, (int), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR DeleteInput(int pos)", asMETHODPR(CKBehavior, DeleteInput, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ GetInput(int pos)", asMETHODPR(CKBehavior, GetInput, (int), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetInputCount()", asMETHODPR(CKBehavior, GetInputCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetInputPosition(CKBehaviorIO@ pbio)", asMETHODPR(CKBehavior, GetInputPosition, (CKBehaviorIO*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int AddInput(const string &in name)", asMETHODPR(CKBehavior, AddInput, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ ReplaceInput(int pos, CKBehaviorIO@ io)", asMETHODPR(CKBehavior, ReplaceInput, (int, CKBehaviorIO*), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorIO@ CreateInput(const string &in name)", asMETHODPR(CKBehavior, CreateInput, (CKSTRING), CKBehaviorIO*), asCALL_THISCALL); assert(r >= 0);

    // Inputs Parameters
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR ExportInputParameter(CKParameterIn@ p)", asMETHODPR(CKBehavior, ExportInputParameter, (CKParameterIn*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ CreateInputParameter(const string &in name, CKParameterType type)", asMETHODPR(CKBehavior, CreateInputParameter, (CKSTRING, CKParameterType), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ CreateInputParameter(const string &in name, CKGUID guid)", asMETHODPR(CKBehavior, CreateInputParameter, (CKSTRING, CKGUID), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ InsertInputParameter(int pos, const string &in name, CKParameterType type)", asMETHODPR(CKBehavior, InsertInputParameter, (int, CKSTRING, CKParameterType), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void AddInputParameter(CKParameterIn@ input)", asMETHODPR(CKBehavior, AddInputParameter, (CKParameterIn*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetInputParameterPosition(CKParameterIn@ param)", asMETHODPR(CKBehavior, GetInputParameterPosition, (CKParameterIn*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ GetInputParameter(int pos)", asMETHODPR(CKBehavior, GetInputParameter, (int), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ RemoveInputParameter(int pos)", asMETHODPR(CKBehavior, RemoveInputParameter, (int), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterIn@ ReplaceInputParameter(int pos, CKParameterIn@ param)", asMETHODPR(CKBehavior, ReplaceInputParameter, (int, CKParameterIn*), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetInputParameterCount()", asMETHODPR(CKBehavior, GetInputParameterCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR GetInputParameterValue(int pos, NativePointer buf)", asMETHODPR(CKBehavior, GetInputParameterValue, (int, void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetInputParameterReadDataPtr(int pos)", asMETHODPR(CKBehavior, GetInputParameterReadDataPtr, (int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKObject@ GetInputParameterObject(int pos)", asMETHODPR(CKBehavior, GetInputParameterObject, (int), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsInputParameterEnabled(int pos)", asMETHODPR(CKBehavior, IsInputParameterEnabled, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void EnableInputParameter(int pos, bool enable)", asMETHODPR(CKBehavior, EnableInputParameter, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Outputs Parameters
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR ExportOutputParameter(CKParameterOut@ p)", asMETHODPR(CKBehavior, ExportOutputParameter, (CKParameterOut*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ CreateOutputParameter(const string &in name, CKParameterType type)", asMETHODPR(CKBehavior, CreateOutputParameter, (CKSTRING, CKParameterType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ CreateOutputParameter(const string &in name, CKGUID guid)", asMETHODPR(CKBehavior, CreateOutputParameter, (CKSTRING, CKGUID), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ InsertOutputParameter(int pos, const string &in name, CKParameterType type)", asMETHODPR(CKBehavior, InsertOutputParameter, (int, CKSTRING, CKParameterType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ GetOutputParameter(int pos)", asMETHODPR(CKBehavior, GetOutputParameter, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetOutputParameterPosition(CKParameterOut@ param)", asMETHODPR(CKBehavior, GetOutputParameterPosition, (CKParameterOut*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ ReplaceOutputParameter(int pos, CKParameterOut@ param)", asMETHODPR(CKBehavior, ReplaceOutputParameter, (int, CKParameterOut*), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOut@ RemoveOutputParameter(int pos)", asMETHODPR(CKBehavior, RemoveOutputParameter, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void AddOutputParameter(CKParameterOut@ param)", asMETHODPR(CKBehavior, AddOutputParameter, (CKParameterOut*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetOutputParameterCount()", asMETHODPR(CKBehavior, GetOutputParameterCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR GetOutputParameterValue(int pos, NativePointer buf)", asMETHODPR(CKBehavior, GetOutputParameterValue, (int, void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetOutputParameterValue(int pos, const NativePointer buf, int size = 0)", asMETHODPR(CKBehavior, SetOutputParameterValue, (int, const void*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetOutputParameterWriteDataPtr(int pos)", asMETHODPR(CKBehavior, GetOutputParameterWriteDataPtr, (int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetOutputParameterObject(int pos, CKObject@ obj)", asMETHODPR(CKBehavior, SetOutputParameterObject, (int, CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKObject@ GetOutputParameterObject(int pos)", asMETHODPR(CKBehavior, GetOutputParameterObject, (int), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsOutputParameterEnabled(int pos)", asMETHODPR(CKBehavior, IsOutputParameterEnabled, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void EnableOutputParameter(int pos, bool enable)", asMETHODPR(CKBehavior, EnableOutputParameter, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Local Parameters
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterLocal@ CreateLocalParameter(const string &in name, CKParameterType type)", asMETHODPR(CKBehavior, CreateLocalParameter, (CKSTRING, CKParameterType), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterLocal@ CreateLocalParameter(const string &in name, CKGUID guid)", asMETHODPR(CKBehavior, CreateLocalParameter, (CKSTRING, CKGUID), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterLocal@ GetLocalParameter(int pos)", asMETHODPR(CKBehavior, GetLocalParameter, (int), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterLocal@ RemoveLocalParameter(int pos)", asMETHODPR(CKBehavior, RemoveLocalParameter, (int), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void AddLocalParameter(CKParameterLocal@ param)", asMETHODPR(CKBehavior, AddLocalParameter, (CKParameterLocal*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetLocalParameterPosition(CKParameterLocal@ param)", asMETHODPR(CKBehavior, GetLocalParameterPosition, (CKParameterLocal*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetLocalParameterCount()", asMETHODPR(CKBehavior, GetLocalParameterCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR GetLocalParameterValue(int pos, NativePointer buf)", asMETHODPR(CKBehavior, GetLocalParameterValue, (int, void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetLocalParameterValue(int pos, const NativePointer buf, int size = 0)", asMETHODPR(CKBehavior, SetLocalParameterValue, (int, const void*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetLocalParameterWriteDataPtr(int pos)", asMETHODPR(CKBehavior, GetLocalParameterWriteDataPtr, (int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "NativePointer GetLocalParameterReadDataPtr(int pos)", asMETHODPR(CKBehavior, GetLocalParameterReadDataPtr, (int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKObject@ GetLocalParameterObject(int pos)", asMETHODPR(CKBehavior, GetLocalParameterObject, (int), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetLocalParameterObject(int pos, CKObject@ obj)", asMETHODPR(CKBehavior, SetLocalParameterObject, (int, CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "bool IsLocalParameterSetting(int pos)", asMETHODPR(CKBehavior, IsLocalParameterSetting, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Run Time Graph: Activation
    r = engine->RegisterObjectMethod("CKBehavior", "void Activate(bool active = true, bool breset = false)", asMETHODPR(CKBehavior, Activate, (CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Run Time Graph: Sub Behaviors
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR AddSubBehavior(CKBehavior@ cbk)", asMETHODPR(CKBehavior, AddSubBehavior, (CKBehavior*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ RemoveSubBehavior(CKBehavior@ cbk)", asMETHODPR(CKBehavior, RemoveSubBehavior, (CKBehavior*), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ RemoveSubBehavior(int pos)", asMETHODPR(CKBehavior, RemoveSubBehavior, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehavior@ GetSubBehavior(int pos)", asMETHODPR(CKBehavior, GetSubBehavior, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetSubBehaviorCount()", asMETHODPR(CKBehavior, GetSubBehaviorCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Run Time Graph: Links between sub behaviors
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR AddSubBehaviorLink(CKBehaviorLink@ cbkl)", asMETHODPR(CKBehavior, AddSubBehaviorLink, (CKBehaviorLink*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorLink@ RemoveSubBehaviorLink(CKBehaviorLink@ cbkl)", asMETHODPR(CKBehavior, RemoveSubBehaviorLink, (CKBehaviorLink*), CKBehaviorLink*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorLink@ RemoveSubBehaviorLink(int pos)", asMETHODPR(CKBehavior, RemoveSubBehaviorLink, (int), CKBehaviorLink*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKBehaviorLink@ GetSubBehaviorLink(int pos)", asMETHODPR(CKBehavior, GetSubBehaviorLink, (int), CKBehaviorLink*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetSubBehaviorLinkCount()", asMETHODPR(CKBehavior, GetSubBehaviorLinkCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Run Time Graph: Parameter Operation
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR AddParameterOperation(CKParameterOperation@ op)", asMETHODPR(CKBehavior, AddParameterOperation, (CKParameterOperation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOperation@ GetParameterOperation(int pos)", asMETHODPR(CKBehavior, GetParameterOperation, (int), CKParameterOperation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOperation@ RemoveParameterOperation(int pos)", asMETHODPR(CKBehavior, RemoveParameterOperation, (int), CKParameterOperation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKParameterOperation@ RemoveParameterOperation(CKParameterOperation@ op)", asMETHODPR(CKBehavior, RemoveParameterOperation, (CKParameterOperation*), CKParameterOperation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "int GetParameterOperationCount()", asMETHODPR(CKBehavior, GetParameterOperationCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Run Time Graph: Behavior Priority
    r = engine->RegisterObjectMethod("CKBehavior", "int GetPriority()", asMETHODPR(CKBehavior, GetPriority, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetPriority(int priority)", asMETHODPR(CKBehavior, SetPriority, (int), void), asCALL_THISCALL); assert(r >= 0);

    // Profiling
    r = engine->RegisterObjectMethod("CKBehavior", "float GetLastExecutionTime()", asMETHODPR(CKBehavior, GetLastExecutionTime, (), float), asCALL_THISCALL); assert(r >= 0);

    // Internal Functions
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetOwner(CKBeObject@ beo, bool callback = true)", asMETHODPR(CKBehavior, SetOwner, (CKBeObject*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "CKERROR SetSubBehaviorOwner(CKBeObject@ beo, bool callback = true)", asMETHODPR(CKBehavior, SetSubBehaviorOwner, (CKBeObject*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehavior", "void NotifyEdition()", asMETHODPR(CKBehavior, NotifyEdition, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void NotifySettingsEdition()", asMETHODPR(CKBehavior, NotifySettingsEdition, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehavior", "CKStateChunk@ GetInterfaceChunk()", asMETHODPR(CKBehavior, GetInterfaceChunk, (), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehavior", "void SetInterfaceChunk(CKStateChunk@ state)", asMETHODPR(CKBehavior, SetInterfaceChunk, (CKStateChunk*), void), asCALL_THISCALL); assert(r >= 0);

    // Not implemented
    // r = engine->RegisterObjectMethod("CKBehavior", "void Reset()", asMETHODPR(CKBehavior, Reset, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKBehavior", "void ErrorMessage(const string &in error, const string &in context, bool showOwner = true, bool showScript = true)", asMETHODPR(CKBehavior, ErrorMessage, (CKSTRING, CKSTRING, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKBehavior", "void ErrorMessage(const string &in error, CKDWORD context, bool showOwner = true, bool showScript = true)", asMETHODPR(CKBehavior, ErrorMessage, (CKSTRING, CKDWORD, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKBehavior", "void SetPrototypeGuid(CKGUID ckguid)", asMETHODPR(CKBehavior, SetPrototypeGuid, (CKGUID), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKObjectAnimation(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSceneObjectMembers<CKObjectAnimation>(engine, "CKObjectAnimation");

    // Controller functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ CreateController(CKANIMATION_CONTROLLER controlType)", asMETHODPR(CKObjectAnimation, CreateController, (CKANIMATION_CONTROLLER), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool DeleteController(CKANIMATION_CONTROLLER controlType)", asMETHODPR(CKObjectAnimation, DeleteController, (CKANIMATION_CONTROLLER), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetPositionController()", asMETHODPR(CKObjectAnimation, GetPositionController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetScaleController()", asMETHODPR(CKObjectAnimation, GetScaleController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetRotationController()", asMETHODPR(CKObjectAnimation, GetRotationController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKAnimController@ GetScaleAxisController()", asMETHODPR(CKObjectAnimation, GetScaleAxisController, (), CKAnimController*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKMorphController@ GetMorphController()", asMETHODPR(CKObjectAnimation, GetMorphController, (), CKMorphController*), asCALL_THISCALL); assert(r >= 0);

    // Evaluate functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluatePosition(float time, VxVector &out pos)", asMETHODPR(CKObjectAnimation, EvaluatePosition, (float, VxVector&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateScale(float time, VxVector &out scl)", asMETHODPR(CKObjectAnimation, EvaluateScale, (float, VxVector&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateRotation(float time, VxQuaternion &out rot)", asMETHODPR(CKObjectAnimation, EvaluateRotation, (float, VxQuaternion&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateScaleAxis(float time, VxQuaternion &out scaleAxis)", asMETHODPR(CKObjectAnimation, EvaluateScaleAxis, (float, VxQuaternion&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateMorphTarget(float time, int vertexCount, NativePointer vertices, CKDWORD vStride, NativePointer normals)", asMETHODPR(CKObjectAnimation, EvaluateMorphTarget, (float, int, VxVector*, CKDWORD, VxCompressedVector*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool EvaluateKeys(float step, VxQuaternion &out rot, VxVector &out pos, VxVector &out scale, VxQuaternion &out scaleRot = void)", asMETHODPR(CKObjectAnimation, EvaluateKeys, (float, VxQuaternion*, VxVector*, VxVector*, VxQuaternion*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Info functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasMorphNormalInfo()", asMETHODPR(CKObjectAnimation, HasMorphNormalInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasMorphInfo()", asMETHODPR(CKObjectAnimation, HasMorphInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasScaleInfo()", asMETHODPR(CKObjectAnimation, HasScaleInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasPositionInfo()", asMETHODPR(CKObjectAnimation, HasPositionInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasRotationInfo()", asMETHODPR(CKObjectAnimation, HasRotationInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool HasScaleAxisInfo()", asMETHODPR(CKObjectAnimation, HasScaleAxisInfo, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Key functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddPositionKey(float timeStep, VxVector &in pos)", asMETHODPR(CKObjectAnimation, AddPositionKey, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddRotationKey(float timeStep, VxQuaternion &in rot)", asMETHODPR(CKObjectAnimation, AddRotationKey, (float, VxQuaternion*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddScaleKey(float timeStep, VxVector &in scl)", asMETHODPR(CKObjectAnimation, AddScaleKey, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void AddScaleAxisKey(float timeStep, VxQuaternion &in sclAxis)", asMETHODPR(CKObjectAnimation, AddScaleAxisKey, (float, VxQuaternion*), void), asCALL_THISCALL); assert(r >= 0);

    // Comparison and sharing functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool Compare(CKObjectAnimation@ anim, float threshold = 0.0)", asMETHODPR(CKObjectAnimation, Compare, (CKObjectAnimation*, float), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool ShareDataFrom(CKObjectAnimation@ anim)", asMETHODPR(CKObjectAnimation, ShareDataFrom, (CKObjectAnimation*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKObjectAnimation@ Shared()", asMETHODPR(CKObjectAnimation, Shared, (), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Flags
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetFlags(CKDWORD flags)", asMETHODPR(CKObjectAnimation, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKDWORD GetFlags()", asMETHODPR(CKObjectAnimation, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Clear functions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Clear()", asMETHODPR(CKObjectAnimation, Clear, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void ClearAll()", asMETHODPR(CKObjectAnimation, ClearAll, (), void), asCALL_THISCALL); assert(r >= 0);

    // Merged animations
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetMergeFactor()", asMETHODPR(CKObjectAnimation, GetMergeFactor, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetMergeFactor(float factor)", asMETHODPR(CKObjectAnimation, SetMergeFactor, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "bool IsMerged()", asMETHODPR(CKObjectAnimation, IsMerged, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKObjectAnimation@ CreateMergedAnimation(CKObjectAnimation@ subAnim2, bool dynamic = false)", asMETHODPR(CKObjectAnimation, CreateMergedAnimation, (CKObjectAnimation*, CKBOOL), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Length
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void SetLength(float nbFrame)", asMETHODPR(CKObjectAnimation, SetLength, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetLength()", asMETHODPR(CKObjectAnimation, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);

    // Velocity
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void GetVelocity(float step, VxVector &out vel)", asMETHODPR(CKObjectAnimation, GetVelocity, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    // Step and frame
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKERROR SetStep(float step, CKKeyedAnimation@ anim = null)", asMETHODPR(CKObjectAnimation, SetStep, (float, CKKeyedAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CKERROR SetFrame(float step, CKKeyedAnimation@ anim = null)", asMETHODPR(CKObjectAnimation, SetFrame, (float, CKKeyedAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "float GetCurrentStep()", asMETHODPR(CKObjectAnimation, GetCurrentStep, (), float), asCALL_THISCALL); assert(r >= 0);

    // 3D Entity
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Set3dEntity(CK3dEntity@ ent)", asMETHODPR(CKObjectAnimation, Set3dEntity, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKObjectAnimation", "CK3dEntity@ Get3dEntity()", asMETHODPR(CKObjectAnimation, Get3dEntity, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    // Morph Vertex Count
    r = engine->RegisterObjectMethod("CKObjectAnimation", "int GetMorphVertexCount()", asMETHODPR(CKObjectAnimation, GetMorphVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Transitions
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void CreateTransition(float length, CKObjectAnimation@ animIn, float stepFrom, CKObjectAnimation@ animOut, float stepTo, bool veloc, bool dontTurn, CKAnimKey &in startingSet = void)", asMETHODPR(CKObjectAnimation, CreateTransition, (float, CKObjectAnimation*, float, CKObjectAnimation*, float, CKBOOL, CKBOOL, CKAnimKey*), void), asCALL_THISCALL); assert(r >= 0);

    // Clone
    r = engine->RegisterObjectMethod("CKObjectAnimation", "void Clone(CKObjectAnimation@ anim)", asMETHODPR(CKObjectAnimation, Clone, (CKObjectAnimation*), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKAnimationMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<CKAnimation>(engine, name);

    // Stepping along
    r = engine->RegisterObjectMethod(name, "float GetLength()", asMETHODPR(T, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetFrame()", asMETHODPR(T, GetFrame, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetNextFrame(float delta)", asMETHODPR(T, GetNextFrame, (float), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetStep()", asMETHODPR(T, GetStep, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFrame(float frame)", asMETHODPR(T, SetFrame, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetStep(float step)", asMETHODPR(T, SetStep, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLength(float nbFrame)", asMETHODPR(T, SetLength, (float), void), asCALL_THISCALL); assert(r >= 0);

    // Character functions
    r = engine->RegisterObjectMethod(name, "CKCharacter@ GetCharacter()", asMETHODPR(T, GetCharacter, (), CKCharacter*), asCALL_THISCALL); assert(r >= 0);

    // Frame rate link
    r = engine->RegisterObjectMethod(name, "void LinkToFrameRate(bool link, float fps = 30.0)", asMETHODPR(T, LinkToFrameRate, (CKBOOL, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLinkedFrameRate()", asMETHODPR(T, GetLinkedFrameRate, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsLinkedToFrameRate()", asMETHODPR(T, IsLinkedToFrameRate, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Transition mode
    r = engine->RegisterObjectMethod(name, "void SetTransitionMode(CK_ANIMATION_TRANSITION_MODE mode)", asMETHODPR(T, SetTransitionMode, (CK_ANIMATION_TRANSITION_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_ANIMATION_TRANSITION_MODE GetTransitionMode()", asMETHODPR(T, GetTransitionMode, (), CK_ANIMATION_TRANSITION_MODE), asCALL_THISCALL); assert(r >= 0);

    // Secondary animation mode
    r = engine->RegisterObjectMethod(name, "void SetSecondaryAnimationMode(CK_SECONDARYANIMATION_FLAGS mode)", asMETHODPR(T, SetSecondaryAnimationMode, (CK_SECONDARYANIMATION_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK_SECONDARYANIMATION_FLAGS GetSecondaryAnimationMode()", asMETHODPR(T, GetSecondaryAnimationMode, (), CK_SECONDARYANIMATION_FLAGS), asCALL_THISCALL); assert(r >= 0);

    // Priority and interruption
    r = engine->RegisterObjectMethod(name, "void SetCanBeInterrupt(bool can = true)", asMETHODPR(T, SetCanBeInterrupt, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool CanBeInterrupt()", asMETHODPR(T, CanBeInterrupt, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Orientation
    r = engine->RegisterObjectMethod(name, "void SetCharacterOrientation(bool orient = true)", asMETHODPR(T, SetCharacterOrientation, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool DoesCharacterTakeOrientation()", asMETHODPR(T, DoesCharacterTakeOrientation, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Flags
    r = engine->RegisterObjectMethod(name, "void SetFlags(CKDWORD flags)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetFlags()", asMETHODPR(T, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Root entity
    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetRootEntity()", asMETHODPR(T, GetRootEntity, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    // Merged animations
    r = engine->RegisterObjectMethod(name, "float GetMergeFactor()", asMETHODPR(T, GetMergeFactor, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetMergeFactor(float frame)", asMETHODPR(T, SetMergeFactor, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsMerged()", asMETHODPR(T, IsMerged, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKAnimation@ CreateMergedAnimation(CKAnimation@ anim2, bool dynamic = false)", asMETHODPR(T, CreateMergedAnimation, (CKAnimation*, CKBOOL), CKAnimation*), asCALL_THISCALL); assert(r >= 0);

    // Set current step
    r = engine->RegisterObjectMethod(name, "void SetCurrentStep(float step)", asMETHODPR(T, SetCurrentStep, (float), void), asCALL_THISCALL); assert(r >= 0);

    // Transition animation
    r = engine->RegisterObjectMethod(name, "float CreateTransition(CKAnimation@ input, CKAnimation@ output, CKDWORD outTransitionMode, float length = 6.0, float frameTo = 0)", asMETHODPR(T, CreateTransition, (CKAnimation*, CKAnimation*, CKDWORD, float, float), float), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKERROR AddAnimation(CKObjectAnimation@ anim)", asMETHODPR(CKKeyedAnimation, AddAnimation, (CKObjectAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKERROR RemoveAnimation(CKObjectAnimation@ anim)", asMETHODPR(CKKeyedAnimation, RemoveAnimation, (CKObjectAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "int GetAnimationCount()", asMETHODPR(CKKeyedAnimation, GetAnimationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKObjectAnimation@ GetAnimation(CK3dEntity@ ent)", asMETHODPR(CKKeyedAnimation, GetAnimation, (CK3dEntity*), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "CKObjectAnimation@ GetAnimation(int index)", asMETHODPR(CKKeyedAnimation, GetAnimation, (int), CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKKeyedAnimation", "void Clear()", asMETHODPR(CKKeyedAnimation, Clear, (), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKBeObjectMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "void ExecuteBehaviors(float delta)", asMETHODPR(T, ExecuteBehaviors, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInGroup(CKGroup@ group)", asMETHODPR(T, IsInGroup, (CKGroup*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool HasAttribute(CKAttributeType attribType)", asMETHODPR(T, HasAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetAttribute(CKAttributeType attribType, CK_ID parameter = 0)", asMETHODPR(T, SetAttribute, (CKAttributeType, CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveAttribute(CKAttributeType attribType)", asMETHODPR(T, RemoveAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameter(CKAttributeType attribType)", asMETHODPR(T, GetAttributeParameter, (CKAttributeType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeCount()", asMETHODPR(T, GetAttributeCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeType(int index)", asMETHODPR(T, GetAttributeType, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameterByIndex(int index)", asMETHODPR(T, GetAttributeParameterByIndex, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "void GetAttributeList(CKAttributeVal &out)", asMETHODPR(T, GetAttributeList, (CKAttributeVal*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllAttributes()", asMETHODPR(T, RemoveAllAttributes, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddScript(CKBehavior@ ckb)", asMETHODPR(T, AddScript, (CKBehavior*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(CK_ID id)", asMETHODPR(T, RemoveScript, (CK_ID), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(int pos)", asMETHODPR(T, RemoveScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemoveAllScripts()", asMETHODPR(T, RemoveAllScripts, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ GetScript(int pos)", asMETHODPR(T, GetScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetScriptCount()", asMETHODPR(T, GetScriptCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPriority()", asMETHODPR(T, GetPriority, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPriority(int priority)", asMETHODPR(T, SetPriority, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLastFrameMessageCount()", asMETHODPR(T, GetLastFrameMessageCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMessage@ GetLastFrameMessage(int pos)", asMETHODPR(T, GetLastFrameMessage, (int), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAsWaitingForMessages(bool wait = true)", asMETHODPR(T, SetAsWaitingForMessages, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsWaitingForMessages()", asMETHODPR(T, IsWaitingForMessages, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CallBehaviorCallbackFunction(CKDWORD message, CKGUID &in behguid = void)", asMETHODPR(T, CallBehaviorCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLastExecutionTime()", asMETHODPR(T, GetLastExecutionTime, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)", asMETHODPR(T, ApplyPatchForOlderVersion, (int, CKFileObject*), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKBeObject") != 0) {
        RegisterClassRefCast<T, CKBeObject>(engine, name, "CKBeObject");
    }
}

template <>
static void RegisterCKBeObjectMembers<CKWaveSound>(asIScriptEngine *engine, const char *name) {
    int r = 0;

    RegisterCKSceneObjectMembers<CKWaveSound>(engine, name);

    r = engine->RegisterObjectMethod(name, "void ExecuteBehaviors(float delta)", asMETHODPR(CKWaveSound, ExecuteBehaviors, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInGroup(CKGroup@ group)", asMETHODPR(CKWaveSound, IsInGroup, (CKGroup*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool HasAttribute(CKAttributeType attribType)", asMETHODPR(CKWaveSound, HasAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetAttribute(CKAttributeType attribType, CK_ID parameter = 0)", asMETHODPR(CKWaveSound, SetAttribute, (CKAttributeType, CK_ID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveAttribute(CKAttributeType attribType)", asMETHODPR(CKWaveSound, RemoveAttribute, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameter(CKAttributeType attribType)", asMETHODPR(CKWaveSound, GetAttributeParameter, (CKAttributeType), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeCount()", asMETHODPR(CKWaveSound, GetAttributeCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAttributeType(int index)", asMETHODPR(CKWaveSound, GetAttributeType, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKParameterOut@ GetAttributeParameterByIndex(int index)", asMETHODPR(CKWaveSound, GetAttributeParameterByIndex, (int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "void GetAttributeList(CKAttributeVal &out)", asMETHODPR(CKWaveSound, GetAttributeList, (CKAttributeVal*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllAttributes()", asMETHODPR(CKWaveSound, RemoveAllAttributes, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddScript(CKBehavior@ ckb)", asMETHODPR(CKWaveSound, AddScript, (CKBehavior*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(CK_ID id)", asMETHODPR(CKWaveSound, RemoveScript, (CK_ID), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ RemoveScript(int pos)", asMETHODPR(CKWaveSound, RemoveScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemoveAllScripts()", asMETHODPR(CKWaveSound, RemoveAllScripts, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBehavior@ GetScript(int pos)", asMETHODPR(CKWaveSound, GetScript, (int), CKBehavior*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetScriptCount()", asMETHODPR(CKWaveSound, GetScriptCount, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "int GetPriority()", asMETHODPR(CKWaveSound, GetPriority, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "void SetPriority(int priority)", asMETHODPR(CKWaveSound, SetPriority, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLastFrameMessageCount()", asMETHODPR(CKWaveSound, GetLastFrameMessageCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMessage@ GetLastFrameMessage(int pos)", asMETHODPR(CKWaveSound, GetLastFrameMessage, (int), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetAsWaitingForMessages(bool wait = true)", asMETHODPR(CKWaveSound, SetAsWaitingForMessages, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsWaitingForMessages()", asMETHODPR(CKWaveSound, IsWaitingForMessages, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int CallBehaviorCallbackFunction(CKDWORD message, CKGUID &in behguid = void)", asMETHODPR(CKWaveSound, CallBehaviorCallbackFunction, (CKDWORD, CKGUID*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLastExecutionTime()", asMETHODPR(CKWaveSound, GetLastExecutionTime, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyPatchForOlderVersion(int nbObject, CKFileObject &in fileObjects)", asMETHODPR(CKWaveSound, ApplyPatchForOlderVersion, (int, CKFileObject*), void), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("CKScene", "void AddObjectToScene(CKSceneObject@ obj, bool dependencies = true)", asMETHODPR(CKScene, AddObjectToScene, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void RemoveObjectFromScene(CKSceneObject@ obj, bool dependencies = true)", asMETHODPR(CKScene, RemoveObjectFromScene, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool IsObjectHere(CKObject@ obj)", asMETHODPR(CKScene, IsObjectHere, (CKObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void BeginAddSequence(bool begin)", asMETHODPR(CKScene, BeginAddSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void BeginRemoveSequence(bool begin)", asMETHODPR(CKScene, BeginRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Object List
    r = engine->RegisterObjectMethod("CKScene", "int GetObjectCount()", asMETHODPR(CKScene, GetObjectCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "const XObjectPointerArray &ComputeObjectList(CK_CLASSID cid, bool derived = true)", asMETHODPR(CKScene, ComputeObjectList, (CK_CLASSID, CKBOOL), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);

    // Object Settings by index in list
    r = engine->RegisterObjectMethod("CKScene", "CKSODHashIt GetObjectIterator()", asMETHODPR(CKScene, GetObjectIterator, (), CKSceneObjectIterator), asCALL_THISCALL); assert(r >= 0);

    // BeObject and Script Activation/deactivation
    r = engine->RegisterObjectMethod("CKScene", "void Activate(CKSceneObject@ obj, bool reset)", asMETHODPR(CKScene, Activate, (CKSceneObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void DeActivate(CKSceneObject@ obj)", asMETHODPR(CKScene, DeActivate, (CKSceneObject*), void), asCALL_THISCALL); assert(r >= 0);

    // Object Settings by object
    r = engine->RegisterObjectMethod("CKScene", "void SetObjectFlags(CKSceneObject@ obj, CK_SCENEOBJECT_FLAGS flags)", asMETHODPR(CKScene, SetObjectFlags, (CKSceneObject*, CK_SCENEOBJECT_FLAGS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CK_SCENEOBJECT_FLAGS GetObjectFlags(CKSceneObject@ obj)", asMETHODPR(CKScene, GetObjectFlags, (CKSceneObject*), CK_SCENEOBJECT_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CK_SCENEOBJECT_FLAGS ModifyObjectFlags(CKSceneObject@ obj, CKDWORD add, CKDWORD remove)", asMETHODPR(CKScene, ModifyObjectFlags, (CKSceneObject*, CKDWORD, CKDWORD), CK_SCENEOBJECT_FLAGS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool SetObjectInitialValue(CKSceneObject@ obj, CKStateChunk@ chunk)", asMETHODPR(CKScene, SetObjectInitialValue, (CKSceneObject*, CKStateChunk*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKStateChunk@ GetObjectInitialValue(CKSceneObject@ obj)", asMETHODPR(CKScene, GetObjectInitialValue, (CKSceneObject*), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool IsObjectActive(CKSceneObject@ obj)", asMETHODPR(CKScene, IsObjectActive, (CKSceneObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Render Settings
    r = engine->RegisterObjectMethod("CKScene", "void ApplyEnvironmentSettings(XObjectPointerArray &in renderList = void)", asMETHODPR(CKScene, ApplyEnvironmentSettings, (XObjectPointerArray*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void UseEnvironmentSettings(bool use = true)", asMETHODPR(CKScene, UseEnvironmentSettings, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "bool EnvironmentSettings()", asMETHODPR(CKScene, EnvironmentSettings, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Ambient Light
    r = engine->RegisterObjectMethod("CKScene", "void SetAmbientLight(CKDWORD color)", asMETHODPR(CKScene, SetAmbientLight, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "uint GetAmbientLight()", asMETHODPR(CKScene, GetAmbientLight, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Fog Access
    r = engine->RegisterObjectMethod("CKScene", "void SetFogMode(VXFOG_MODE mode)", asMETHODPR(CKScene, SetFogMode, (VXFOG_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogStart(float start)", asMETHODPR(CKScene, SetFogStart, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogEnd(float end)", asMETHODPR(CKScene, SetFogEnd, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogDensity(float density)", asMETHODPR(CKScene, SetFogDensity, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetFogColor(CKDWORD color)", asMETHODPR(CKScene, SetFogColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKScene", "VXFOG_MODE GetFogMode()", asMETHODPR(CKScene, GetFogMode, (), VXFOG_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogStart()", asMETHODPR(CKScene, GetFogStart, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogEnd()", asMETHODPR(CKScene, GetFogEnd, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "float GetFogDensity()", asMETHODPR(CKScene, GetFogDensity, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKDWORD GetFogColor()", asMETHODPR(CKScene, GetFogColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    // Background
    r = engine->RegisterObjectMethod("CKScene", "void SetBackgroundColor(CKDWORD color)", asMETHODPR(CKScene, SetBackgroundColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKDWORD GetBackgroundColor()", asMETHODPR(CKScene, GetBackgroundColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "void SetBackgroundTexture(CKTexture@ texture)", asMETHODPR(CKScene, SetBackgroundTexture, (CKTexture*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKTexture@ GetBackgroundTexture()", asMETHODPR(CKScene, GetBackgroundTexture, (), CKTexture*), asCALL_THISCALL); assert(r >= 0);

    // Active camera
    r = engine->RegisterObjectMethod("CKScene", "void SetStartingCamera(CKCamera@ camera)", asMETHODPR(CKScene, SetStartingCamera, (CKCamera*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKScene", "CKCamera@ GetStartingCamera()", asMETHODPR(CKScene, GetStartingCamera, (), CKCamera*), asCALL_THISCALL); assert(r >= 0);

    // Level functions
    r = engine->RegisterObjectMethod("CKScene", "CKLevel@ GetLevel()", asMETHODPR(CKScene, GetLevel, (), CKLevel*), asCALL_THISCALL); assert(r >= 0);

    // Merge functions
    r = engine->RegisterObjectMethod("CKScene", "CKERROR Merge(CKScene@ mergedScene, CKLevel@ fromLevel = null)", asMETHODPR(CKScene, Merge, (CKScene*, CKLevel*), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKLevel(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKLevel>(engine, "CKLevel");

    // Object Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddObject(CKObject@ obj)", asMETHODPR(CKLevel, AddObject, (CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveObject(CKObject@ obj)", asMETHODPR(CKLevel, RemoveObject, (CKObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveObject(CK_ID id)", asMETHODPR(CKLevel, RemoveObject, (CK_ID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void BeginAddSequence(bool begin)", asMETHODPR(CKLevel, BeginAddSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void BeginRemoveSequence(bool begin)", asMETHODPR(CKLevel, BeginRemoveSequence, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Object List
    r = engine->RegisterObjectMethod("CKLevel", "const XObjectPointerArray &ComputeObjectList(CK_CLASSID cid, bool derived = true)", asMETHODPR(CKLevel, ComputeObjectList, (CK_CLASSID, CKBOOL), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);

    // Place Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddPlace(CKPlace@ place)", asMETHODPR(CKLevel, AddPlace, (CKPlace*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemovePlace(CKPlace@ place)", asMETHODPR(CKLevel, RemovePlace, (CKPlace*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKPlace@ RemovePlace(int pos)", asMETHODPR(CKLevel, RemovePlace, (int), CKPlace*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKPlace@ GetPlace(int pos)", asMETHODPR(CKLevel, GetPlace, (int), CKPlace*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetPlaceCount()", asMETHODPR(CKLevel, GetPlaceCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Scene Management
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR AddScene(CKScene@ scene)", asMETHODPR(CKLevel, AddScene, (CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR RemoveScene(CKScene@ scene)", asMETHODPR(CKLevel, RemoveScene, (CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ RemoveScene(int pos)", asMETHODPR(CKLevel, RemoveScene, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetScene(int pos)", asMETHODPR(CKLevel, GetScene, (int), CKScene*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetSceneCount()", asMETHODPR(CKLevel, GetSceneCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Active Scene
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR SetNextActiveScene(CKScene@ scene, CK_SCENEOBJECTACTIVITY_FLAGS active = CK_SCENEOBJECTACTIVITY_SCENEDEFAULT, CK_SCENEOBJECTRESET_FLAGS reset = CK_SCENEOBJECTRESET_RESET)", asMETHODPR(CKLevel, SetNextActiveScene, (CKScene*, CK_SCENEOBJECTACTIVITY_FLAGS, CK_SCENEOBJECTRESET_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR LaunchScene(CKScene@ scene, CK_SCENEOBJECTACTIVITY_FLAGS active = CK_SCENEOBJECTACTIVITY_SCENEDEFAULT, CK_SCENEOBJECTRESET_FLAGS reset = CK_SCENEOBJECTRESET_RESET)", asMETHODPR(CKLevel, LaunchScene, (CKScene*, CK_SCENEOBJECTACTIVITY_FLAGS, CK_SCENEOBJECTRESET_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetCurrentScene()", asMETHODPR(CKLevel, GetCurrentScene, (), CKScene*), asCALL_THISCALL); assert(r >= 0);

    // Render Context functions
    r = engine->RegisterObjectMethod("CKLevel", "void AddRenderContext(CKRenderContext@ dev, bool main = false)", asMETHODPR(CKLevel, AddRenderContext, (CKRenderContext*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "void RemoveRenderContext(CKRenderContext@ dev)", asMETHODPR(CKLevel, RemoveRenderContext, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "int GetRenderContextCount()", asMETHODPR(CKLevel, GetRenderContextCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKLevel", "CKRenderContext@ GetRenderContext(int count)", asMETHODPR(CKLevel, GetRenderContext, (int), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);

    // Main Scene for this Level
    r = engine->RegisterObjectMethod("CKLevel", "CKScene@ GetLevelScene()", asMETHODPR(CKLevel, GetLevelScene, (), CKScene*), asCALL_THISCALL); assert(r >= 0);

    // Merge
    r = engine->RegisterObjectMethod("CKLevel", "CKERROR Merge(CKLevel@ mergedLevel, bool asScene)", asMETHODPR(CKLevel, Merge, (CKLevel*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKGroup(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKGroup>(engine, "CKGroup");

    // Insertion/Removal
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR AddObject(CKBeObject@ obj)", asMETHODPR(CKGroup, AddObject, (CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR AddObjectFront(CKBeObject@ obj)", asMETHODPR(CKGroup, AddObjectFront, (CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "CKERROR InsertObjectAt(CKBeObject@ obj, int pos)", asMETHODPR(CKGroup, InsertObjectAt, (CKBeObject*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGroup", "CKBeObject@ RemoveObject(int pos)", asMETHODPR(CKGroup, RemoveObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void RemoveObject(CKBeObject@ obj)", asMETHODPR(CKGroup, RemoveObject, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void Clear()", asMETHODPR(CKGroup, Clear, (), void), asCALL_THISCALL); assert(r >= 0);

    // Order
    r = engine->RegisterObjectMethod("CKGroup", "void MoveObjectUp(CKBeObject@ obj)", asMETHODPR(CKGroup, MoveObjectUp, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGroup", "void MoveObjectDown(CKBeObject@ obj)", asMETHODPR(CKGroup, MoveObjectDown, (CKBeObject*), void), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("CKMaterial", "void SetPower(float value)", asMETHODPR(CKMaterial, SetPower, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetAmbient() const", asMETHODPR(CKMaterial, GetAmbient, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAmbient(const VxColor &in color)", asMETHODPR(CKMaterial, SetAmbient, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetDiffuse() const", asMETHODPR(CKMaterial, GetDiffuse, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetDiffuse(const VxColor &in color)", asMETHODPR(CKMaterial, SetDiffuse, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetSpecular() const", asMETHODPR(CKMaterial, GetSpecular, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetSpecular(const VxColor &in color)", asMETHODPR(CKMaterial, SetSpecular, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "const VxColor &GetEmissive() const", asMETHODPR(CKMaterial, GetEmissive, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetEmissive(const VxColor &in color)", asMETHODPR(CKMaterial, SetEmissive, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKTexture@ GetTexture(int texIndex = 0)", asMETHODPR(CKMaterial, GetTexture, (int), CKTexture*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTexture(int texIndex, CKTexture@ tex)", asMETHODPR(CKMaterial, SetTexture, (int, CKTexture*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTexture(CKTexture@ tex)", asMETHODPR(CKMaterial, SetTexture0, (CKTexture*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_BLENDMODE GetTextureBlendMode() const", asMETHODPR(CKMaterial, GetTextureBlendMode, (), VXTEXTURE_BLENDMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureBlendMode(VXTEXTURE_BLENDMODE blendMode)", asMETHODPR(CKMaterial, SetTextureBlendMode, (VXTEXTURE_BLENDMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_FILTERMODE GetTextureMinMode() const", asMETHODPR(CKMaterial, GetTextureMinMode, (), VXTEXTURE_FILTERMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureMinMode(VXTEXTURE_FILTERMODE filterMode)", asMETHODPR(CKMaterial, SetTextureMinMode, (VXTEXTURE_FILTERMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_FILTERMODE GetTextureMagMode() const", asMETHODPR(CKMaterial, GetTextureMagMode, (), VXTEXTURE_FILTERMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureMagMode(VXTEXTURE_FILTERMODE filterMode)", asMETHODPR(CKMaterial, SetTextureMagMode, (VXTEXTURE_FILTERMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXTEXTURE_ADDRESSMODE GetTextureAddressMode() const", asMETHODPR(CKMaterial, GetTextureAddressMode, (), VXTEXTURE_ADDRESSMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureAddressMode(VXTEXTURE_ADDRESSMODE mode)", asMETHODPR(CKMaterial, SetTextureAddressMode, (VXTEXTURE_ADDRESSMODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKDWORD GetTextureBorderColor() const", asMETHODPR(CKMaterial, GetTextureBorderColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTextureBorderColor(CKDWORD color)", asMETHODPR(CKMaterial, SetTextureBorderColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXBLEND_MODE GetSourceBlend() const", asMETHODPR(CKMaterial, GetSourceBlend, (), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetSourceBlend(VXBLEND_MODE blendMode)", asMETHODPR(CKMaterial, SetSourceBlend, (VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXBLEND_MODE GetDestBlend() const", asMETHODPR(CKMaterial, GetDestBlend, (), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetDestBlend(VXBLEND_MODE blendMode)", asMETHODPR(CKMaterial, SetDestBlend, (VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool IsTwoSided() const", asMETHODPR(CKMaterial, IsTwoSided, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetTwoSided(bool twoSided)", asMETHODPR(CKMaterial, SetTwoSided, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool ZWriteEnabled() const", asMETHODPR(CKMaterial, ZWriteEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableZWrite(bool zWrite = true)", asMETHODPR(CKMaterial, EnableZWrite, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool AlphaBlendEnabled() const", asMETHODPR(CKMaterial, AlphaBlendEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableAlphaBlend(bool blend = true)", asMETHODPR(CKMaterial, EnableAlphaBlend, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXCMPFUNC GetZFunc() const", asMETHODPR(CKMaterial, GetZFunc, (), VXCMPFUNC), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetZFunc(VXCMPFUNC zFunc = VXCMP_LESSEQUAL)", asMETHODPR(CKMaterial, SetZFunc, (VXCMPFUNC), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool PerspectiveCorrectionEnabled() const", asMETHODPR(CKMaterial, PerspectiveCorrectionEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnablePerspectiveCorrection(bool perspective = true)", asMETHODPR(CKMaterial, EnablePerspectiveCorrection, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetFillMode(VXFILL_MODE fillMode)", asMETHODPR(CKMaterial, SetFillMode, (VXFILL_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VXFILL_MODE GetFillMode() const", asMETHODPR(CKMaterial, GetFillMode, (), VXFILL_MODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetShadeMode(VXSHADE_MODE shadeMode)", asMETHODPR(CKMaterial, SetShadeMode, (VXSHADE_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VXSHADE_MODE GetShadeMode() const", asMETHODPR(CKMaterial, GetShadeMode, (), VXSHADE_MODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool SetAsCurrent(CKRenderContext@ dev, bool lit = true, int textureStage = 0)", asMETHODPR(CKMaterial, SetAsCurrent, (CKRenderContext*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool IsAlphaTransparent() const", asMETHODPR(CKMaterial, IsAlphaTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "bool AlphaTestEnabled() const", asMETHODPR(CKMaterial, AlphaTestEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void EnableAlphaTest(bool enable = true)", asMETHODPR(CKMaterial, EnableAlphaTest, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "VXCMPFUNC GetAlphaFunc() const", asMETHODPR(CKMaterial, GetAlphaFunc, (), VXCMPFUNC), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAlphaFunc(VXCMPFUNC alphaFunc = VXCMP_ALWAYS)", asMETHODPR(CKMaterial, SetAlphaFunc, (VXCMPFUNC), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKBYTE GetAlphaRef() const", asMETHODPR(CKMaterial, GetAlphaRef, (), CKBYTE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "void SetAlphaRef(CKBYTE alphaRef = 0)", asMETHODPR(CKMaterial, SetAlphaRef, (CKBYTE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetCallback(NativePointer fct, NativePointer argument)", asMETHODPR(CKMaterial, SetCallback, (CK_MATERIALCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "NativePointer GetCallback(NativePointer &out argument = void) const", asMETHODPR(CKMaterial, GetCallback, (void**), CK_MATERIALCALLBACK), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "void SetEffect(VX_EFFECT effect)", asMETHODPR(CKMaterial, SetEffect, (VX_EFFECT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMaterial", "VX_EFFECT GetEffect() const", asMETHODPR(CKMaterial, GetEffect, (), VX_EFFECT), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMaterial", "CKParameter@ GetEffectParameter() const", asMETHODPR(CKMaterial, GetEffectParameter, (), CKParameter*), asCALL_THISCALL); assert(r >= 0);
}

template<typename T>
void RegisterCKBitmapDataMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectProperty(name, "CKMovieInfo@ m_MovieInfo", asOFFSET(T, m_MovieInfo)); assert(r >= 0);
    // r = engine->RegisterObjectProperty(name, "XArray<CKBitmapSlot *> m_Slots", asOFFSET(T, m_Slots)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "int m_Width", asOFFSET(T, m_Width)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "int m_Height", asOFFSET(T, m_Height)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "int m_CurrentSlot", asOFFSET(T, m_CurrentSlot)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "int m_PickThreshold", asOFFSET(T, m_PickThreshold)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "CKDWORD m_BitmapFlags", asOFFSET(T, m_BitmapFlags)); assert(r >= 0);
    r = engine->RegisterObjectProperty(name, "CKDWORD m_TransColor", asOFFSET(T, m_TransColor)); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool CreateImage(int width, int height, int bpp = 32, int slot = 0)", asMETHODPR(T, CreateImage, (int, int, int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SaveImage(const string &in name, int slot = 0, bool useFormat = false)", asMETHODPR(T, SaveImage, (CKSTRING, int, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SaveImageAlpha(const string &in name, int slot = 0)", asMETHODPR(T, SaveImageAlpha, (CKSTRING, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "string GetMovieFileName()", asMETHODPR(T, GetMovieFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMovieReader@ GetMovieReader()", asMETHODPR(T, GetMovieReader, (), CKMovieReader *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer LockSurfacePtr(int slot = -1)", asMETHODPR(T, LockSurfacePtr, (int), CKBYTE *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseSurfacePtr(int slot = -1)", asMETHODPR(T, ReleaseSurfacePtr, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "string GetSlotFileName(int slot)", asMETHODPR(T, GetSlotFileName, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetSlotFileName(int slot, const string &in filename)", asMETHODPR(T, SetSlotFileName, (int, CKSTRING), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetWidth()", asMETHODPR(T, GetWidth, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetHeight()", asMETHODPR(T, GetHeight, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetImageDesc(VxImageDescEx &out desc)", asMETHODPR(T, GetImageDesc, (VxImageDescEx &), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetSlotCount()", asMETHODPR(T, GetSlotCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetSlotCount(int count)", asMETHODPR(T, SetSlotCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetCurrentSlot(int slot)", asMETHODPR(T, SetCurrentSlot, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetCurrentSlot()", asMETHODPR(T, GetCurrentSlot, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseSlot(int slot)", asMETHODPR(T, ReleaseSlot, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseAllSlots()", asMETHODPR(T, ReleaseAllSlots, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetPixel(int x, int y, CKDWORD color, int slot = -1)", asMETHODPR(T, SetPixel, (int, int, CKDWORD, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetPixel(int x, int y, int slot = -1)", asMETHODPR(T, GetPixel, (int, int, int), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKDWORD GetTransparentColor()", asMETHODPR(T, GetTransparentColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparentColor(CKDWORD color)", asMETHODPR(T, SetTransparentColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparent(bool transparency)", asMETHODPR(T, SetTransparent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsTransparent()", asMETHODPR(T, IsTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK_TEXTURE_SAVEOPTIONS GetSaveOptions()", asMETHODPR(T, GetSaveOptions, (), CK_BITMAP_SAVEOPTIONS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_TEXTURE_SAVEOPTIONS options)", asMETHODPR(T, SetSaveOptions, (CK_BITMAP_SAVEOPTIONS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBitmapProperties@ GetSaveFormat()", asMETHODPR(T, GetSaveFormat, (), CKBitmapProperties *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveFormat(CKBitmapProperties@ format)", asMETHODPR(T, SetSaveFormat, (CKBitmapProperties *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetPickThreshold(int pt)", asMETHODPR(T, SetPickThreshold, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPickThreshold()", asMETHODPR(T, GetPickThreshold, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetCubeMap(bool cubeMap)", asMETHODPR(T, SetCubeMap, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsCubeMap()", asMETHODPR(T, IsCubeMap, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool ResizeImages(int width, int height)", asMETHODPR(T, ResizeImages, (int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetDynamicHint(bool dynamic)", asMETHODPR(T, SetDynamicHint, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetDynamicHint()", asMETHODPR(T, GetDynamicHint, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool ToRestore()", asMETHODPR(T, ToRestore, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool LoadSlotImage(XString name, int slot = 0)", asMETHODPR(T, LoadSlotImage, (XString, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool LoadMovieFile(XString name)", asMETHODPR(T, LoadMovieFile, (XString), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMovieInfo@ CreateMovieInfo(XString s, CKMovieProperties@ &out mp)", asMETHODPR(T, CreateMovieInfo, (XString, CKMovieProperties **), CKMovieInfo *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetMovieInfo(CKMovieInfo@ mi)", asMETHODPR(T, SetMovieInfo, (CKMovieInfo *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetAlphaForTransparentColor(const VxImageDescEx &in desc)", asMETHODPR(T, SetAlphaForTransparentColor, (const VxImageDescEx &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetBorderColorForClamp(const VxImageDescEx &in desc)", asMETHODPR(T, SetBorderColorForClamp, (const VxImageDescEx &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetSlotImage(int slot, NativePointer buffer, const VxImageDescEx &in desc)", asMETHODPR(T, SetSlotImage, (int, void *, VxImageDescEx &), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod(name, "bool DumpToChunk(CKStateChunk@ chunk, CKContext@ context, CKFile@ file, NativeBuffer Identifiers)", asMETHOD(T, DumpToChunk), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod(name, "bool ReadFromChunk(CKStateChunk@ chunk, CKContext@ context, CKFile@ file, NativeBuffer Identifiers)", asMETHOD(T, ReadFromChunk), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKTexture(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<CKTexture>(engine, "CKTexture");
    RegisterCKBitmapDataMembers<CKTexture>(engine, "CKTexture");

    // r = engine->RegisterObjectMethod("CKTexture", "bool Create(int width, int height, int bpp = 32, int slot = 0)", asMETHODPR(CKTexture, Create, (int, int, int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool LoadImage(const string &in name, int slot = 0)", asMETHODPR(CKTexture, LoadImage, (CKSTRING, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool LoadMovie(const string &in name)", asMETHODPR(CKTexture, LoadMovie, (CKSTRING), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SetAsCurrent(CKRenderContext@ dev, bool clamping = false, int textureStage = 0)", asMETHODPR(CKTexture, SetAsCurrent, (CKRenderContext*, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool Restore(bool clamp = false)", asMETHODPR(CKTexture, Restore, (CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SystemToVideoMemory(CKRenderContext@ dev, bool clamping = false)", asMETHODPR(CKTexture, SystemToVideoMemory, (CKRenderContext*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool FreeVideoMemory()", asMETHODPR(CKTexture, FreeVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool IsInVideoMemory() const", asMETHODPR(CKTexture, IsInVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool CopyContext(CKRenderContext@ dev, const VxRect &in src, const VxRect &in dest, int cubeMapFace = 0)", asMETHODPR(CKTexture, CopyContext, (CKRenderContext*, VxRect*, VxRect*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool UseMipmap(bool useMipmap)", asMETHODPR(CKTexture, UseMipmap, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "int GetMipmapCount() const", asMETHODPR(CKTexture, GetMipmapCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetVideoTextureDesc(VxImageDescEx &out desc)", asMETHODPR(CKTexture, GetVideoTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "VX_PIXELFORMAT GetVideoPixelFormat() const", asMETHODPR(CKTexture, GetVideoPixelFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetSystemTextureDesc(VxImageDescEx &out desc)", asMETHODPR(CKTexture, GetSystemTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "void SetDesiredVideoFormat(VX_PIXELFORMAT format)", asMETHODPR(CKTexture, SetDesiredVideoFormat, (VX_PIXELFORMAT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "VX_PIXELFORMAT GetDesiredVideoFormat() const", asMETHODPR(CKTexture, GetDesiredVideoFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool SetUserMipMapMode(bool userMipmap)", asMETHODPR(CKTexture, SetUserMipMapMode, (CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "bool GetUserMipMapLevel(int level, VxImageDescEx &out resultImage)", asMETHODPR(CKTexture, GetUserMipMapLevel, (int, VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTexture", "int GetRstTextureIndex() const", asMETHODPR(CKTexture, GetRstTextureIndex, (), int), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKMeshMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool IsTransparent() const", asMETHODPR(T, IsTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparent(bool transparency)", asMETHODPR(T, SetTransparent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetWrapMode(VXTEXTURE_WRAPMODE mode)", asMETHODPR(T, SetWrapMode, (VXTEXTURE_WRAPMODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXTEXTURE_WRAPMODE GetWrapMode() const", asMETHODPR(T, GetWrapMode, (), VXTEXTURE_WRAPMODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetLitMode(VXMESH_LITMODE mode)", asMETHODPR(T, SetLitMode, (VXMESH_LITMODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXMESH_LITMODE GetLitMode() const", asMETHODPR(T, GetLitMode, (), VXMESH_LITMODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKDWORD GetFlags() const", asMETHODPR(T, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFlags(CKDWORD flags)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetModifierVertices(NativePointer stride)", asMETHODPR(T, GetModifierVertices, (CKDWORD*), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetModifierVertexCount() const", asMETHODPR(T, GetModifierVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ModifierVertexMove(bool rebuildNormals, bool rebuildFaceNormals)", asMETHODPR(T, ModifierVertexMove, (CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer GetModifierUVs(NativePointer stride, int channel = -1)", asMETHODPR(T, GetModifierUVs, (CKDWORD*, int), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetModifierUVCount(int channel = -1) const", asMETHODPR(T, GetModifierUVCount, (int), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ModifierUVMove()", asMETHODPR(T, ModifierUVMove, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetVertexCount() const", asMETHODPR(T, GetVertexCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetVertexCount(int count)", asMETHODPR(T, SetVertexCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexColor(int index, CKDWORD color)", asMETHODPR(T, SetVertexColor, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexSpecularColor(int index, CKDWORD color)", asMETHODPR(T, SetVertexSpecularColor, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexNormal(int index, const VxVector &in vector)", asMETHODPR(T, SetVertexNormal, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexPosition(int index, const VxVector &in vector)", asMETHODPR(T, SetVertexPosition, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexTextureCoordinates(int index, float u, float v, int channel = -1)", asMETHODPR(T, SetVertexTextureCoordinates, (int, float, float, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetColorsPtr(CKDWORD &out stride)", asMETHODPR(T, GetColorsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetSpecularColorsPtr(NativePointer stride)", asMETHODPR(T, GetSpecularColorsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetNormalsPtr(NativePointer stride)", asMETHODPR(T, GetNormalsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetPositionsPtr(NativePointer stride)", asMETHODPR(T, GetPositionsPtr, (CKDWORD*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetTextureCoordinatesPtr(NativePointer stride, int channel = -1)", asMETHODPR(T, GetTextureCoordinatesPtr, (CKDWORD*, int), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint32 GetVertexColor(int index) const", asMETHODPR(T, GetVertexColor, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint32 GetVertexSpecularColor(int index) const", asMETHODPR(T, GetVertexSpecularColor, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexNormal(int index, VxVector &out vector)", asMETHODPR(T, GetVertexNormal, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexPosition(int index, VxVector &out vector)", asMETHODPR(T, GetVertexPosition, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetVertexTextureCoordinates(int index, float &out u, float &out v, int channel = -1)", asMETHODPR(T, GetVertexTextureCoordinates, (int, float*, float*, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void TranslateVertices(NativePointer vector)", asMETHODPR(T, TranslateVertices, (VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(const VxVector &in vector, const VxVector &in pivot = void)", asMETHODPR(T, ScaleVertices, (VxVector*, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ScaleVertices(float x, float y, float z, const VxVector &in pivot = void)", asMETHODPR(T, ScaleVertices3f, (float, float, float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RotateVertices(const VxVector &in vector, float angle)", asMETHODPR(T, RotateVertices, (VxVector*, float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void VertexMove()", asMETHODPR(T, VertexMove, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void UVChanged()", asMETHODPR(T, UVChanged, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void NormalChanged()", asMETHODPR(T, NormalChanged, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ColorChanged()", asMETHODPR(T, ColorChanged, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetFaceCount() const", asMETHODPR(T, GetFaceCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetFaceCount(int count)", asMETHODPR(T, SetFaceCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetFacesIndices()", asMETHODPR(T, GetFacesIndices, (), CKWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetFaceVertexIndex(int index, int &out vertex1, int &out vertex2, int &out vertex3)", asMETHODPR(T, GetFaceVertexIndex, (int, int&, int&, int&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetFaceMaterial(int index)", asMETHODPR(T, GetFaceMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxVector &GetFaceNormal(int index)", asMETHODPR(T, GetFaceNormal, (int), const VxVector&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKWORD GetFaceChannelMask(int faceIndex)", asMETHODPR(T, GetFaceChannelMask, (int), CKWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VxVector &GetFaceVertex(int FaceIndex, int vertexIndex)", asMETHODPR(T, GetFaceVertex, (int, int), VxVector&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetFaceNormalsPtr(NativePointer stride)", asMETHODPR(T, GetFaceNormalsPtr, (CKDWORD*), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceVertexIndex(int faceIndex, int vertex1, int vertex2, int vertex3)", asMETHODPR(T, SetFaceVertexIndex, (int, int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceMaterialEx(NativePointer faceIndices, int faceCount, CKMaterial@ mat)", asMETHODPR(T, SetFaceMaterialEx, (int*, int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceMaterial(int faceIndex, CKMaterial@ mat)", asMETHODPR(T, SetFaceMaterial, (int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFaceChannelMask(int faceIndex, CKWORD channelMask)", asMETHODPR(T, SetFaceChannelMask, (int, CKWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ReplaceMaterial(CKMaterial@ oldMat, CKMaterial@ newMat)", asMETHODPR(T, ReplaceMaterial, (CKMaterial*, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ChangeFaceChannelMask(int faceIndex, CKWORD addChannelMask, CKWORD removeChannelMask)", asMETHODPR(T, ChangeFaceChannelMask, (int, CKWORD, CKWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ApplyGlobalMaterial(CKMaterial@ mat)", asMETHODPR(T, ApplyGlobalMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void DissociateAllFaces()", asMETHODPR(T, DissociateAllFaces, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetLineCount(int count)", asMETHODPR(T, SetLineCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetLineCount()", asMETHODPR(T, GetLineCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetLineIndices()", asMETHODPR(T, GetLineIndices, (), CKWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLine(int lineIndex, int vIndex1, int vIndex2)", asMETHODPR(T, SetLine, (int, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetLine(int lineIndex, int &out vIndex1, int &out vIndex2)", asMETHODPR(T, GetLine, (int, int*, int*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void CreateLineStrip(int startingLine, int count, int startingVertexIndex)", asMETHODPR(T, CreateLineStrip, (int, int, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Clean(bool keepVertices = false)", asMETHODPR(T, Clean, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void InverseWinding()", asMETHODPR(T, InverseWinding, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Consolidate()", asMETHODPR(T, Consolidate, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void UnOptimize()", asMETHODPR(T, UnOptimize, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetRadius() const", asMETHODPR(T, GetRadius, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxBbox &GetLocalBox() const", asMETHODPR(T, GetLocalBox, (), const VxBbox&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetBaryCenter(VxVector &out vector) const", asMETHODPR(T, GetBaryCenter, (VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetChannelCount() const", asMETHODPR(T, GetChannelCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int AddChannel(CKMaterial@ mat, bool copySrcUv = true)", asMETHODPR(T, AddChannel, (CKMaterial*, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveChannelMaterial(CKMaterial@ mat)", asMETHODPR(T, RemoveChannelMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveChannel(int index)", asMETHODPR(T, RemoveChannel, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetChannelByMaterial(CKMaterial@ mat) const", asMETHODPR(T, GetChannelByMaterial, (CKMaterial*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ActivateChannel(int index, bool active = true)", asMETHODPR(T, ActivateChannel, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsChannelActive(int index) const", asMETHODPR(T, IsChannelActive, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ActivateAllChannels(bool active = true)", asMETHODPR(T, ActivateAllChannels, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void LitChannel(int index, bool lit = true)", asMETHODPR(T, LitChannel, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsChannelLit(int index) const", asMETHODPR(T, IsChannelLit, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetChannelFlags(int index) const", asMETHODPR(T, GetChannelFlags, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelFlags(int index, CKDWORD flags)", asMETHODPR(T, SetChannelFlags, (int, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetChannelMaterial(int index) const", asMETHODPR(T, GetChannelMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelMaterial(int index, CKMaterial@ mat)", asMETHODPR(T, SetChannelMaterial, (int, CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXBLEND_MODE GetChannelSourceBlend(int index) const", asMETHODPR(T, GetChannelSourceBlend, (int), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VXBLEND_MODE GetChannelDestBlend(int index) const", asMETHODPR(T, GetChannelDestBlend, (int), VXBLEND_MODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelSourceBlend(int index, VXBLEND_MODE blendMode)", asMETHODPR(T, SetChannelSourceBlend, (int, VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetChannelDestBlend(int index, VXBLEND_MODE blendMode)", asMETHODPR(T, SetChannelDestBlend, (int, VXBLEND_MODE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void BuildNormals()", asMETHODPR(T, BuildNormals, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void BuildFaceNormals()", asMETHODPR(T, BuildFaceNormals, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR Render(CKRenderContext@ dev, CK3dEntity@ mov)", asMETHODPR(T, Render, (CKRenderContext*, CK3dEntity*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPreRenderCallBack(NativePointer func, NativePointer argument, bool temporary = false)", asMETHODPR(T, AddPreRenderCallBack, (CK_MESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePreRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemovePreRenderCallBack, (CK_MESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AddPostRenderCallBack(NativePointer func, NativePointer argument, bool temporary = false)", asMETHODPR(T, AddPostRenderCallBack, (CK_MESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePostRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemovePostRenderCallBack, (CK_MESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, SetRenderCallBack, (CK_MESHRENDERCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetDefaultRenderCallBack()", asMETHODPR(T, SetDefaultRenderCallBack, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveAllCallbacks()", asMETHODPR(T, RemoveAllCallbacks, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetMaterialCount() const", asMETHODPR(T, GetMaterialCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetMaterial(int index) const", asMETHODPR(T, GetMaterial, (int), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetVertexWeightsCount() const", asMETHODPR(T, GetVertexWeightsCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexWeightsCount(int count)", asMETHODPR(T, SetVertexWeightsCount, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "NativePointer GetVertexWeightsPtr()", asMETHODPR(T, GetVertexWeightsPtr, (), float*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetVertexWeight(int index) const", asMETHODPR(T, GetVertexWeight, (int), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetVertexWeight(int index, float w)", asMETHODPR(T, SetVertexWeight, (int, float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void LoadVertices(CKStateChunk@ chunk)", asMETHODPR(T, LoadVertices, (CKStateChunk*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetVerticesRendered(int count)", asMETHODPR(T, SetVerticesRendered, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetVerticesRendered() const", asMETHODPR(T, GetVerticesRendered, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR CreatePM()", asMETHODPR(T, CreatePM, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void DestroyPM()", asMETHODPR(T, DestroyPM, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPM() const", asMETHODPR(T, IsPM, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void EnablePMGeoMorph(bool enable)", asMETHODPR(T, EnablePMGeoMorph, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPMGeoMorphEnabled() const", asMETHODPR(T, IsPMGeoMorphEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPMGeoMorphStep(int gs)", asMETHODPR(T, SetPMGeoMorphStep, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPMGeoMorphStep() const", asMETHODPR(T, GetPMGeoMorphStep, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddSubMeshPreRenderCallBack(NativePointer func, NativePointer argument, bool temporary = false)", asMETHODPR(T, AddSubMeshPreRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveSubMeshPreRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemoveSubMeshPreRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AddSubMeshPostRenderCallBack(NativePointer func, NativePointer argument, bool temporary = false)", asMETHODPR(T, AddSubMeshPostRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveSubMeshPostRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemoveSubMeshPostRenderCallBack, (CK_SUBMESHRENDERCALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("CKDataArray", "void InsertColumn(int cDest, CK_ARRAYTYPE type, const string &in name, CKGUID paramGuid)", asMETHODPR(CKDataArray, InsertColumn, (int, CK_ARRAYTYPE, char*, CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void MoveColumn(int cSrc, int cDest)", asMETHODPR(CKDataArray, MoveColumn, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RemoveColumn(int c)", asMETHODPR(CKDataArray, RemoveColumn, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetColumnName(int c, const string &in name)", asMETHODPR(CKDataArray, SetColumnName, (int, char*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "string GetColumnName(int c)", asMETHODPR(CKDataArray, GetColumnName, (int), char*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetColumnType(int c, CK_ARRAYTYPE type, CKGUID paramGuid)", asMETHODPR(CKDataArray, SetColumnType, (int, CK_ARRAYTYPE, CKGUID), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CK_ARRAYTYPE GetColumnType(int c)", asMETHODPR(CKDataArray, GetColumnType, (int), CK_ARRAYTYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKGUID GetColumnParameterGuid(int c)", asMETHODPR(CKDataArray, GetColumnParameterGuid, (int), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetKeyColumn()", asMETHODPR(CKDataArray, GetKeyColumn, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SetKeyColumn(int c)", asMETHODPR(CKDataArray, SetKeyColumn, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetColumnCount()", asMETHODPR(CKDataArray, GetColumnCount, (), int), asCALL_THISCALL); assert(r >= 0);

    // Elements Functions
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD &GetElement(int i, int c)", asMETHODPR(CKDataArray, GetElement, (int, int), CKDWORD*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetElementValue(int i, int c, NativePointer value)", asMETHODPR(CKDataArray, GetElementValue, (int, int, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKObject@ GetElementObject(int i, int c)", asMETHODPR(CKDataArray, GetElementObject, (int, int), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementValue(int i, int c, NativePointer value, int size = 0)", asMETHODPR(CKDataArray, SetElementValue, (int, int, void*, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementValueFromParameter(int i, int c, CKParameter@ pout)", asMETHODPR(CKDataArray, SetElementValueFromParameter, (int, int, CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementObject(int i, int c, CKObject@ obj)", asMETHODPR(CKDataArray, SetElementObject, (int, int, CKObject*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Parameters Shortcuts
    r = engine->RegisterObjectMethod("CKDataArray", "bool PasteShortcut(int i, int c, CKParameter@ pout)", asMETHODPR(CKDataArray, PasteShortcut, (int, int, CKParameter*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKParameterOut@ RemoveShortcut(int i, int c)", asMETHODPR(CKDataArray, RemoveShortcut, (int, int), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);

    // String Value
    r = engine->RegisterObjectMethod("CKDataArray", "bool SetElementStringValue(int i, int c, const string &in value)", asMETHODPR(CKDataArray, SetElementStringValue, (int, int, char*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetStringValue(CKDWORD key, int c, string &out value)", asMETHODPR(CKDataArray, GetStringValue, (CKDWORD, int, char*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetElementStringValue(int i, int c, string &out value)", asMETHODPR(CKDataArray, GetElementStringValue, (int, int, char*), int), asCALL_THISCALL); assert(r >= 0);

    // Load / Write
    r = engine->RegisterObjectMethod("CKDataArray", "bool LoadElements(const string &in str, bool append, int column)", asMETHODPR(CKDataArray, LoadElements, (CKSTRING, CKBOOL, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool WriteElements(const string &in str, int column, int number, bool append = false)", asMETHODPR(CKDataArray, WriteElements, (CKSTRING, int, int, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Rows Functions
    r = engine->RegisterObjectMethod("CKDataArray", "int GetRowCount()", asMETHODPR(CKDataArray, GetRowCount, (), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKDataArray", "CKDataRow &GetRow(int)", asMETHODPR(CKDataArray, GetRow, (int), CKDataRow*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void AddRow()", asMETHODPR(CKDataArray, AddRow, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKDataArray", "CKDataRow &InsertRow(int)", asMETHODPR(CKDataArray, InsertRow, (int), CKDataRow*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool TestRow(int row, int c, CK_COMPOPERATOR op, CKDWORD key, int size = 0)", asMETHODPR(CKDataArray, TestRow, (int, int, CK_COMPOPERATOR, CKDWORD, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int FindRowIndex(int c, CK_COMPOPERATOR op, CKDWORD key, int size = 0, int startingIndex = 0)", asMETHODPR(CKDataArray, FindRowIndex, (int, CK_COMPOPERATOR, CKDWORD, int, int), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKDataArray", "CKDataRow &FindRow(int c, CK_COMPOPERATOR op, CKDWORD key, int size = 0, int startIndex = 0)", asMETHODPR(CKDataArray, FindRow, (int, CK_COMPOPERATOR, CKDWORD, int, int), CKDataRow *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RemoveRow(int row)", asMETHODPR(CKDataArray, RemoveRow, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void MoveRow(int rSrc, int rDest)", asMETHODPR(CKDataArray, MoveRow, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void SwapRows(int i1, int i2)", asMETHODPR(CKDataArray, SwapRows, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Clear(bool params = true)", asMETHODPR(CKDataArray, Clear, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    // Algorithm
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetHighest(int c, int &out row)", asMETHODPR(CKDataArray, GetHighest, (int, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetLowest(int c, int &out row)", asMETHODPR(CKDataArray, GetLowest, (int, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "bool GetNearest(int c, NativePointer value, int &out row)", asMETHODPR(CKDataArray, GetNearest, (int, void*, int&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void ColumnTransform(int c, CK_BINARYOPERATOR op, CKDWORD value)", asMETHODPR(CKDataArray, ColumnTransform, (int, CK_BINARYOPERATOR, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void ColumnsOperate(int c, CK_BINARYOPERATOR op, int c2, int cr)", asMETHODPR(CKDataArray, ColumnsOperate, (int, CK_BINARYOPERATOR, int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Sort(int c, bool ascending)", asMETHODPR(CKDataArray, Sort, (int, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Unique(int c)", asMETHODPR(CKDataArray, Unique, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void RandomShuffle()", asMETHODPR(CKDataArray, RandomShuffle, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void Reverse()", asMETHODPR(CKDataArray, Reverse, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD Sum(int c)", asMETHODPR(CKDataArray, Sum, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "CKDWORD Product(int c)", asMETHODPR(CKDataArray, Product, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "int GetCount(int c, CK_COMPOPERATOR op, CKDWORD key, int size = 0)", asMETHODPR(CKDataArray, GetCount, (int, CK_COMPOPERATOR, CKDWORD, int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKDataArray", "void CreateGroup(int mc, CK_COMPOPERATOR op, CKDWORD key, int size, CKGroup@ group, int ec = 0)", asMETHODPR(CKDataArray, CreateGroup, (int, CK_COMPOPERATOR, CKDWORD, int, CKGroup*, int), void), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
static void RegisterCKSoundMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBeObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "CK_SOUND_SAVEOPTIONS GetSaveOptions()", asMETHOD(T, GetSaveOptions), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_SOUND_SAVEOPTIONS options)", asMETHOD(T, SetSaveOptions), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_SOUND_SAVEOPTIONS options)", asMETHOD(CKWaveSound, SetSaveOptions), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod("CKWaveSound", "CKSOUNDHANDLE PlayMinion(bool background = true, CK3dEntity@ ent = null, VxVector &in position = void, VxVector &in direction = void, float minDelay = 0.0)", asMETHODPR(CKWaveSound, PlayMinion, (CKBOOL, CK3dEntity *, VxVector *, VxVector *, float), CKSOUNDHANDLE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetSoundFileName(const string &in filename)", asMETHODPR(CKWaveSound, SetSoundFileName, (const CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "string GetSoundFileName()", asMETHODPR(CKWaveSound, GetSoundFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "int GetSoundLength()", asMETHODPR(CKWaveSound, GetSoundLength, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR GetSoundFormat(CKWaveFormat &out format)", asMETHODPR(CKWaveSound, GetSoundFormat, (CKWaveFormat &), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CK_WAVESOUND_TYPE GetType()", asMETHODPR(CKWaveSound, GetType, (), CK_WAVESOUND_TYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void SetType(CK_WAVESOUND_TYPE type)", asMETHODPR(CKWaveSound, SetType, (CK_WAVESOUND_TYPE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKDWORD GetState()", asMETHODPR(CKWaveSound, GetState, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void SetState(CKDWORD state)", asMETHODPR(CKWaveSound, SetState, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPriority(float priority)", asMETHODPR(CKWaveSound, SetPriority, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPriority()", asMETHODPR(CKWaveSound, GetPriority, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetLoopMode(bool enabled)", asMETHODPR(CKWaveSound, SetLoopMode, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "bool GetLoopMode()", asMETHODPR(CKWaveSound, GetLoopMode, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetFileStreaming(bool enabled, bool recreateSound = false)", asMETHODPR(CKWaveSound, SetFileStreaming, (CKBOOL, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "bool GetFileStreaming()", asMETHODPR(CKWaveSound, GetFileStreaming, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void Play(float fadeIn = 0, float finalGain = 1.0)", asMETHODPR(CKWaveSound, Play, (float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Resume()", asMETHODPR(CKWaveSound, Resume, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Rewind()", asMETHODPR(CKWaveSound, Rewind, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Stop(float fadeOut = 0)", asMETHODPR(CKWaveSound, Stop, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Pause()", asMETHODPR(CKWaveSound, Pause, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKBOOL IsPlaying()", asMETHODPR(CKWaveSound, IsPlaying, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKBOOL IsPaused()", asMETHODPR(CKWaveSound, IsPaused, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetGain(float gain)", asMETHODPR(CKWaveSound, SetGain, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetGain()", asMETHODPR(CKWaveSound, GetGain, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPitch(float rate)", asMETHODPR(CKWaveSound, SetPitch, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPitch()", asMETHODPR(CKWaveSound, GetPitch, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetPan(float pan)", asMETHODPR(CKWaveSound, SetPan, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "float GetPan()", asMETHODPR(CKWaveSound, GetPan, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKSOUNDHANDLE GetSource()", asMETHODPR(CKWaveSound, GetSource, (), CKSOUNDHANDLE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void PositionSound(CK3dEntity@ obj, VxVector &in position = void, VxVector &in direction = void, bool commit = false)", asMETHODPR(CKWaveSound, PositionSound, (CK3dEntity *, VxVector *, VxVector *, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CK3dEntity@ GetAttachedEntity()", asMETHODPR(CKWaveSound, GetAttachedEntity, (), CK3dEntity *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetPosition(VxVector &out pos)", asMETHODPR(CKWaveSound, GetPosition, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetDirection(VxVector &out dir)", asMETHODPR(CKWaveSound, GetDirection, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetSound3DInformation(VxVector &out pos, VxVector &out dir, float &out distanceFromListener)", asMETHODPR(CKWaveSound, GetSound3DInformation, (VxVector &, VxVector &, float &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetCone(float inAngle, float outAngle, float outsideGain)", asMETHODPR(CKWaveSound, SetCone, (float, float, float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetCone(float &out inAngle, float &out outAngle, float &out outsideGain)", asMETHODPR(CKWaveSound, GetCone, (float *, float *, float *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetMinMaxDistance(float minDistance, float maxDistance, CKDWORD maxDistanceBehavior = 1)", asMETHODPR(CKWaveSound, SetMinMaxDistance, (float, float, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetMinMaxDistance(float &out minDistance, float &out maxDistance, CKDWORD &out maxDistanceBehavior)", asMETHODPR(CKWaveSound, GetMinMaxDistance, (float *, float *, CKDWORD *), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetVelocity(VxVector &in pos)", asMETHODPR(CKWaveSound, SetVelocity, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetVelocity(VxVector &out pos)", asMETHODPR(CKWaveSound, GetVelocity, (VxVector &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetOrientation(VxVector &in dir, VxVector &in up)", asMETHODPR(CKWaveSound, SetOrientation, (VxVector &, VxVector &), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void GetOrientation(VxVector &out dir, VxVector &out up)", asMETHODPR(CKWaveSound, GetOrientation, (VxVector &, VxVector &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR WriteData(NativePointer buffer, int size)", asMETHODPR(CKWaveSound, WriteData, (CKBYTE *, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Lock(CKDWORD writeCursor, CKDWORD numBytes, NativePointer &out ptr1, CKDWORD &out bytes1, NativePointer &out ptr2, CKDWORD &out bytes2, CK_WAVESOUND_LOCKMODE flags)", asMETHODPR(CKWaveSound, Lock, (CKDWORD, CKDWORD, void **, CKDWORD *, void **, CKDWORD *, CK_WAVESOUND_LOCKMODE), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Unlock(NativePointer ptr1, CKDWORD bytes1, NativePointer ptr2, CKDWORD bytes2)", asMETHODPR(CKWaveSound, Unlock, (void *, CKDWORD, void *, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKDWORD GetPlayPosition()", asMETHODPR(CKWaveSound, GetPlayPosition, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "int GetPlayedMs()", asMETHODPR(CKWaveSound, GetPlayedMs, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Create(bool fileStreaming, const string &in filename)", asMETHODPR(CKWaveSound, Create, (CKBOOL, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Create(CK_WAVESOUND_TYPE type, const CKWaveFormat &in format, int size)", asMETHODPR(CKWaveSound, Create, (CK_WAVESOUND_TYPE, CKWaveFormat *, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR SetReader(CKSoundReader@ reader)", asMETHODPR(CKWaveSound, SetReader, (CKSoundReader *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKSoundReader@ GetReader()", asMETHODPR(CKWaveSound, GetReader, (), CKSoundReader *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void SetDataToRead(int size)", asMETHODPR(CKWaveSound, SetDataToRead, (int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR Recreate(bool safe = false)", asMETHODPR(CKWaveSound, Recreate, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void Release()", asMETHODPR(CKWaveSound, Release, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR TryRecreate()", asMETHODPR(CKWaveSound, TryRecreate, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void UpdatePosition(float delta)", asMETHODPR(CKWaveSound, UpdatePosition, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKWaveSound", "void UpdateFade()", asMETHODPR(CKWaveSound, UpdateFade, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "CKERROR WriteDataFromReader()", asMETHODPR(CKWaveSound, WriteDataFromReader, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKWaveSound", "void FillWithBlanks(bool incBf = false)", asMETHODPR(CKWaveSound, FillWithBlanks, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
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

    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR SetSoundFileName(const string &in filename)", asMETHODPR(CKMidiSound, SetSoundFileName, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "string GetSoundFileName()", asMETHODPR(CKMidiSound, GetSoundFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiSound", "CKDWORD GetCurrentPos()", asMETHODPR(CKMidiSound, GetCurrentPos, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Play()", asMETHODPR(CKMidiSound, Play, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Stop()", asMETHODPR(CKMidiSound, Stop, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiSound", "CKERROR Pause(bool pause = true)", asMETHODPR(CKMidiSound, Pause, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

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

    r = engine->RegisterObjectMethod(name, "bool IsInRenderContext(CKRenderContext@ context)", asMETHODPR(T, IsInRenderContext, (CKRenderContext*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsRootObject()", asMETHODPR(T, IsRootObject, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeRendered()", asMETHODPR(T, IsToBeRendered, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetZOrder(int z)", asMETHODPR(T, SetZOrder, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetZOrder()", asMETHODPR(T, GetZOrder, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsToBeRenderedLast()", asMETHODPR(T, IsToBeRenderedLast, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPreRenderCallBack(NativePointer func, NativePointer argument, bool temp = false)", asMETHODPR(T, AddPreRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePreRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemovePreRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, SetRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveRenderCallBack()", asMETHODPR(T, RemoveRenderCallBack, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddPostRenderCallBack(NativePointer func, NativePointer argument, bool temp = false)", asMETHODPR(T, AddPostRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemovePostRenderCallBack(NativePointer func, NativePointer argument)", asMETHODPR(T, RemovePostRenderCallBack, (CK_RENDEROBJECT_CALLBACK, void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void RemoveAllCallbacks()", asMETHODPR(T, RemoveAllCallbacks, (), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKRenderObject") != 0) {
        RegisterClassRefCast<T, CKRenderObject>(engine, name, "CKRenderObject");
    }
}

void RegisterCKRenderObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKRenderObjectMembers<CKRenderObject>(engine, "CKRenderObject");
}

template <typename T>
void RegisterCK2dEntityMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKRenderObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "int GetPosition(Vx2DVector &out vect, bool hom = false, CK2dEntity@ ref = null)", asMETHODPR(T, GetPosition, (Vx2DVector&, CKBOOL, CK2dEntity*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPosition(const Vx2DVector &in vect, bool hom = false, bool keepChildren = false, CK2dEntity@ ref = null)", asMETHODPR(T, SetPosition, (const Vx2DVector&, CKBOOL, CKBOOL, CK2dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetSize(Vx2DVector &out vect, bool hom = false)", asMETHODPR(T, GetSize, (Vx2DVector&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSize(const Vx2DVector &in vect, bool hom = false, bool keepChildren = false)", asMETHODPR(T, SetSize, (const Vx2DVector&, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetRect(const VxRect &in rect, bool keepChildren = false)", asMETHODPR(T, SetRect, (const VxRect&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetRect(VxRect &out rect)", asMETHODPR(T, GetRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR SetHomogeneousRect(const VxRect &in rect, bool keepChildren = false)", asMETHODPR(T, SetHomogeneousRect, (const VxRect&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR GetHomogeneousRect(VxRect &out rect)", asMETHODPR(T, GetHomogeneousRect, (VxRect&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetSourceRect(VxRect &in rect)", asMETHODPR(T, SetSourceRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetSourceRect(VxRect &out rect)", asMETHODPR(T, GetSourceRect, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void UseSourceRect(bool use = true)", asMETHODPR(T, UseSourceRect, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsUsingSourceRect()", asMETHODPR(T, IsUsingSourceRect, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetPickable(bool pick = true)", asMETHODPR(T, SetPickable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPickable()", asMETHODPR(T, IsPickable, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetBackground(bool back = true)", asMETHODPR(T, SetBackground, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsBackground()", asMETHODPR(T, IsBackground, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetClipToParent(bool clip = true)", asMETHODPR(T, SetClipToParent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsClipToParent()", asMETHODPR(T, IsClipToParent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetFlags(uint flags)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ModifyFlags(uint add, uint remove = 0)", asMETHODPR(T, ModifyFlags, (CKDWORD, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint GetFlags()", asMETHODPR(T, GetFlags, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void EnableRatioOffset(bool ratio = true)", asMETHODPR(T, EnableRatioOffset, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsRatioOffset()", asMETHODPR(T, IsRatioOffset, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetParent(CK2dEntity@ parent)", asMETHODPR(T, SetParent, (CK2dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK2dEntity@ GetParent() const", asMETHODPR(T, GetParent, () const, CK2dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetChildrenCount() const", asMETHODPR(T, GetChildrenCount, () const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK2dEntity@ GetChild(int i) const", asMETHODPR(T, GetChild, (int) const, CK2dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK2dEntity@ HierarchyParser(CK2dEntity@ current) const", asMETHODPR(T, HierarchyParser, (CK2dEntity*) const, CK2dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetMaterial(CKMaterial@ mat)", asMETHODPR(T, SetMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMaterial@ GetMaterial()", asMETHODPR(T, GetMaterial, (), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetHomogeneousCoordinates(bool coord = true)", asMETHODPR(T, SetHomogeneousCoordinates, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsHomogeneousCoordinates()", asMETHODPR(T, IsHomogeneousCoordinates, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void EnableClipToCamera(bool Clip = true)", asMETHODPR(T, EnableClipToCamera, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsClippedToCamera()", asMETHODPR(T, IsClippedToCamera, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int Render(CKRenderContext@ context)", asMETHODPR(T, Render, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int Draw(CKRenderContext@ context)", asMETHODPR(T, Draw, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void GetExtents(VxRect &out srcrect, VxRect &out rect)", asMETHODPR(T, GetExtents, (VxRect&, VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetExtents(const VxRect &in srcrect, const VxRect &in rect)", asMETHODPR(T, SetExtents, (const VxRect&, const VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void RestoreInitialSize()", asMETHODPR(T, RestoreInitialSize, (), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CK2dEntity") != 0) {
        RegisterClassRefCast<T, CK2dEntity>(engine, name, "CK2dEntity");
    }
}

void RegisterCK2dEntity(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCK2dEntityMembers<CK2dEntity>(engine, "CK2dEntity");
}

template <typename T>
void RegisterCKSpriteMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK2dEntityMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "bool Create(int width, int height, int bpp = 32, int slot = 0)", asMETHODPR(T, Create, (int, int, int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool LoadImage(const string &in name, int slot = 0)", asMETHODPR(T, LoadImage, (CKSTRING, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SaveImage(const string &in name, int slot = 0, bool useFormat = false)", asMETHODPR(T, SaveImage, (CKSTRING, int, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool LoadMovie(const string &in name, int width = 0, int height = 0, int bpp = 16)", asMETHODPR(T, LoadMovie, (CKSTRING, int, int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "string GetMovieFileName()", asMETHODPR(T, GetMovieFileName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMovieReader@ GetMovieReader()", asMETHODPR(T, GetMovieReader, (), CKMovieReader*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "NativePointer LockSurfacePtr(int slot = -1)", asMETHODPR(T, LockSurfacePtr, (int), CKBYTE*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseSurfacePtr(int slot = -1)", asMETHODPR(T, ReleaseSurfacePtr, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "string GetSlotFileName(int slot)", asMETHODPR(T, GetSlotFileName, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetSlotFileName(int slot, const string &in filename)", asMETHODPR(T, SetSlotFileName, (int, CKSTRING), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetWidth()", asMETHODPR(T, GetWidth, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetHeight()", asMETHODPR(T, GetHeight, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetBitsPerPixel()", asMETHODPR(T, GetBitsPerPixel, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetBytesPerLine()", asMETHODPR(T, GetBytesPerLine, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetRedMask()", asMETHODPR(T, GetRedMask, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetGreenMask()", asMETHODPR(T, GetGreenMask, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetBlueMask()", asMETHODPR(T, GetBlueMask, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetAlphaMask()", asMETHODPR(T, GetAlphaMask, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetSlotCount()", asMETHODPR(T, GetSlotCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetSlotCount(int count)", asMETHODPR(T, SetSlotCount, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetCurrentSlot(int slot)", asMETHODPR(T, SetCurrentSlot, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetCurrentSlot()", asMETHODPR(T, GetCurrentSlot, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseSlot(int slot)", asMETHODPR(T, ReleaseSlot, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ReleaseAllSlots()", asMETHODPR(T, ReleaseAllSlots, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetPixel(int x, int y, uint col, int slot = -1)", asMETHODPR(T, SetPixel, (int, int, CKDWORD, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "uint GetPixel(int x, int y, int slot = -1)", asMETHODPR(T, GetPixel, (int, int, int), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "uint GetTransparentColor()", asMETHODPR(T, GetTransparentColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparentColor(uint Color)", asMETHODPR(T, SetTransparentColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTransparent(bool transparency)", asMETHODPR(T, SetTransparent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsTransparent()", asMETHODPR(T, IsTransparent, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool Restore(bool clamp = false)", asMETHODPR(T, Restore, (CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SystemToVideoMemory(CKRenderContext@ dev, bool clamping = false)", asMETHODPR(T, SystemToVideoMemory, (CKRenderContext*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool FreeVideoMemory()", asMETHODPR(T, FreeVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInVideoMemory()", asMETHODPR(T, IsInVideoMemory, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool CopyContext(CKRenderContext@ ctx, VxRect &in src, VxRect &in dest)", asMETHODPR(T, CopyContext, (CKRenderContext*, VxRect*, VxRect*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool GetVideoTextureDesc(VxImageDescEx &out desc)", asMETHODPR(T, GetVideoTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VX_PIXELFORMAT GetVideoPixelFormat()", asMETHODPR(T, GetVideoPixelFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetSystemTextureDesc(VxImageDescEx &out desc)", asMETHODPR(T, GetSystemTextureDesc, (VxImageDescEx&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetDesiredVideoFormat(VX_PIXELFORMAT pf)", asMETHODPR(T, SetDesiredVideoFormat, (VX_PIXELFORMAT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "VX_PIXELFORMAT GetDesiredVideoFormat()", asMETHODPR(T, GetDesiredVideoFormat, (), VX_PIXELFORMAT), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK_TEXTURE_SAVEOPTIONS GetSaveOptions()", asMETHODPR(T, GetSaveOptions, (), CK_BITMAP_SAVEOPTIONS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveOptions(CK_TEXTURE_SAVEOPTIONS options)", asMETHODPR(T, SetSaveOptions, (CK_BITMAP_SAVEOPTIONS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKBitmapProperties &GetSaveFormat()", asMETHODPR(T, GetSaveFormat, (), CKBitmapProperties*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSaveFormat(CKBitmapProperties &in format)", asMETHODPR(T, SetSaveFormat, (CKBitmapProperties*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetPickThreshold(int pt)", asMETHODPR(T, SetPickThreshold, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetPickThreshold()", asMETHODPR(T, GetPickThreshold, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool ToRestore()", asMETHODPR(T, ToRestore, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKSprite") != 0) {
        RegisterClassRefCast<T, CKSprite>(engine, name, "CKSprite");
    }
}

void RegisterCKSprite(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKSpriteMembers<CKSprite>(engine, "CKSprite");
}

void RegisterCKSpriteText(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKSpriteMembers<CKSpriteText>(engine, "CKSpriteText");

    r = engine->RegisterObjectMethod("CKSpriteText", "void SetText(const string &in text)", asMETHODPR(CKSpriteText, SetText, (CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSpriteText", "string GetText()", asMETHODPR(CKSpriteText, GetText, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSpriteText", "void SetTextColor(uint col)", asMETHODPR(CKSpriteText, SetTextColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSpriteText", "uint GetTextColor()", asMETHODPR(CKSpriteText, GetTextColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSpriteText", "void SetBackgroundColor(uint col)", asMETHODPR(CKSpriteText, SetBackgroundColor, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSpriteText", "uint GetBackgroundTextColor()", asMETHODPR(CKSpriteText, GetBackgroundTextColor, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSpriteText", "void SetFont(const string &in fontName, int fontSize = 12, int weight = 400, bool italic = false, bool underline = false)", asMETHODPR(CKSpriteText, SetFont, (CKSTRING, int, int, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSpriteText", "void SetAlign(CKSPRITETEXT_ALIGNMENT align)", asMETHODPR(CKSpriteText, SetAlign, (CKSPRITETEXT_ALIGNMENT), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSpriteText", "CKSPRITETEXT_ALIGNMENT GetAlign()", asMETHODPR(CKSpriteText, GetAlign, (), CKSPRITETEXT_ALIGNMENT), asCALL_THISCALL); assert(r >= 0);
}

template <typename T>
void RegisterCK3dEntityMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKRenderObjectMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "int GetChildrenCount() const", asMETHODPR(T, GetChildrenCount, () const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetChild(int pos) const", asMETHODPR(T, GetChild, (int) const, CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool SetParent(CK3dEntity@ parent, bool keepWorldPos = true)", asMETHODPR(T, SetParent, (CK3dEntity*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetParent() const", asMETHODPR(T, GetParent, () const, CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool AddChild(CK3dEntity@ child, bool keepWorldPos = true)", asMETHODPR(T, AddChild, (CK3dEntity*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AddChildren(const XObjectPointerArray &in children, bool keepWorldPos = true)", asMETHODPR(T, AddChildren, (const XObjectPointerArray&, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool RemoveChild(CK3dEntity@ mov)", asMETHODPR(T, RemoveChild, (CK3dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool CheckIfSameKindOfHierarchy(CK3dEntity@ mov, bool sameOrder = false) const", asMETHODPR(T, CheckIfSameKindOfHierarchy, (CK3dEntity*, CKBOOL) const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CK3dEntity@ HierarchyParser(CK3dEntity@ current) const", asMETHODPR(T, HierarchyParser, (CK3dEntity*) const, CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "uint GetFlags() const", asMETHODPR(T, GetFlags, () const, CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFlags(uint flags)", asMETHODPR(T, SetFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetPickable(bool pick = true)", asMETHODPR(T, SetPickable, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsPickable() const", asMETHODPR(T, IsPickable, () const, CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetRenderChannels(bool renderChannels = true)", asMETHODPR(T, SetRenderChannels, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AreRenderChannelsVisible() const", asMETHODPR(T, AreRenderChannelsVisible, () const, CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool IsInViewFrustrum(CKRenderContext@ dev, uint flags = 0)", asMETHODPR(T, IsInViewFrustrum, (CKRenderContext*, CKDWORD), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsInViewFrustrumHierarchic(CKRenderContext@ Dev)", asMETHODPR(T, IsInViewFrustrumHierarchic, (CKRenderContext*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void IgnoreAnimations(bool ignore = true)", asMETHODPR(T, IgnoreAnimations, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool AreAnimationIgnored() const", asMETHODPR(T, AreAnimationIgnored, () const, CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool IsAllInsideFrustrum() const", asMETHODPR(T, IsAllInsideFrustrum, () const, CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool IsAllOutsideFrustrum() const", asMETHODPR(T, IsAllOutsideFrustrum, () const, CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetRenderAsTransparent(bool trans = true)", asMETHODPR(T, SetRenderAsTransparent, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "uint GetMoveableFlags() const", asMETHODPR(T, GetMoveableFlags, () const, CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetMoveableFlags(uint flags)", asMETHODPR(T, SetMoveableFlags, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "uint ModifyMoveableFlags(uint add, uint remove)", asMETHODPR(T, ModifyMoveableFlags, (CKDWORD, CKDWORD), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKMesh@ GetCurrentMesh() const", asMETHODPR(T, GetCurrentMesh, () const, CKMesh*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMesh@ SetCurrentMesh(CKMesh@ m, bool addIfNotHere = true)", asMETHODPR(T, SetCurrentMesh, (CKMesh*, CKBOOL), CKMesh*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetMeshCount() const", asMETHODPR(T, GetMeshCount, () const, int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKMesh@ GetMesh(int pos) const", asMETHODPR(T, GetMesh, (int) const, CKMesh*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR AddMesh(CKMesh@ mesh)", asMETHODPR(T, AddMesh, (CKMesh*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR RemoveMesh(CKMesh@ mesh)", asMETHODPR(T, RemoveMesh, (CKMesh*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void LookAt(const VxVector &in pos, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, LookAt, (const VxVector*, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Rotate(float x, float y, float z, float angle, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, Rotate3f, (float, float, float, float, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Rotate(const VxVector &in axis, float angle, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, Rotate, (const VxVector*, float, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Translate(float x, float y, float z, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, Translate3f, (float, float, float, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Translate(const VxVector &in vect, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, Translate, (const VxVector*, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void AddScale(float x, float y, float z, bool keepChildren = false, bool local = true)", asMETHODPR(T, AddScale3f, (float, float, float, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void AddScale(const VxVector &in scale, bool keepChildren = false, bool local = true)", asMETHODPR(T, AddScale, (const VxVector*, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetPosition(float x, float y, float z, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, SetPosition3f, (float, float, float, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetPosition(const VxVector &in pos, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, SetPosition, (const VxVector*, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetPosition(VxVector&out Pos, CK3dEntity@ Ref = null) const", asMETHODPR(T, GetPosition, (VxVector*, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetOrientation(const VxVector &in dir, const VxVector &in up, const VxVector &in right = void, CK3dEntity@ ref = null, bool keepChildren = false)", asMETHODPR(T, SetOrientation, (const VxVector*, const VxVector*, const VxVector*, CK3dEntity*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetOrientation(VxVector &out dir, VxVector &out up, VxVector &out right = void, CK3dEntity@ ref = null)", asMETHODPR(T, GetOrientation, (VxVector*, VxVector*, VxVector*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetQuaternion(const VxQuaternion &in quat, CK3dEntity@ ref = null, bool keepChildren = false, bool keepScale = false)", asMETHODPR(T, SetQuaternion, (const VxQuaternion*, CK3dEntity*, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetQuaternion(VxQuaternion &out quat, CK3dEntity@ ref = null)", asMETHODPR(T, GetQuaternion, (VxQuaternion*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetScale(float x, float y, float z, bool keepChildren = false, bool local = true)", asMETHODPR(T, SetScale3f, (float, float, float, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetScale(const VxVector &in scale, bool keepChildren = false, bool local = true)", asMETHODPR(T, SetScale, (const VxVector*, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetScale(VxVector &out scale, bool local = true)", asMETHODPR(T, GetScale, (VxVector*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool ConstructWorldMatrix(const VxVector &in pos, const VxVector &in scale, const VxQuaternion &in quat)", asMETHODPR(T, ConstructWorldMatrix, (const VxVector*, const VxVector*, const VxQuaternion*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ConstructWorldMatrixEx(const VxVector &in pos, const VxVector &in scale, const VxQuaternion &in quat, const VxQuaternion &in shear, float sign)", asMETHODPR(T, ConstructWorldMatrixEx, (const VxVector*, const VxVector*, const VxQuaternion*, const VxQuaternion*, float), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ConstructLocalMatrix(const VxVector &in pos, const VxVector &in scale, const VxQuaternion &in quat)", asMETHODPR(T, ConstructLocalMatrix, (const VxVector*, const VxVector*, const VxQuaternion*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool ConstructLocalMatrixEx(const VxVector &in pos, const VxVector &in scale, const VxQuaternion &in quat, const VxQuaternion &in shear, float sign)", asMETHODPR(T, ConstructLocalMatrixEx, (const VxVector*, const VxVector*, const VxQuaternion*, const VxQuaternion*, float), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "bool Render(CKRenderContext@ dev, uint flags = CKRENDER_UPDATEEXTENTS)", asMETHODPR(T, Render, (CKRenderContext*, CKDWORD), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int RayIntersection(const VxVector &in pos1, const VxVector &in pos2, VxIntersectionDesc &out desc, CK3dEntity@ ref, CK_RAYINTERSECTION options = CKRAYINTERSECTION_DEFAULT)", asMETHODPR(T, RayIntersection, (const VxVector*, const VxVector*, VxIntersectionDesc*, CK3dEntity*, CK_RAYINTERSECTION), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetRenderExtents(VxRect &out rect) const", asMETHODPR(T, GetRenderExtents, (VxRect&) const, void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "const VxMatrix &GetLastFrameMatrix() const", asMETHODPR(T, GetLastFrameMatrix, () const, const VxMatrix&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetLocalMatrix(const VxMatrix&in Mat, bool KeepChildren = false)", asMETHODPR(T, SetLocalMatrix, (const VxMatrix&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxMatrix &GetLocalMatrix() const", asMETHODPR(T, GetLocalMatrix, () const, const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetWorldMatrix(const VxMatrix&in Mat, bool KeepChildren = false)", asMETHODPR(T, SetWorldMatrix, (const VxMatrix&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxMatrix &GetWorldMatrix() const", asMETHODPR(T, GetWorldMatrix, () const, const VxMatrix&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxMatrix &GetInverseWorldMatrix() const", asMETHODPR(T, GetInverseWorldMatrix, () const, const VxMatrix&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Transform(VxVector &out dest, const VxVector &in src, CK3dEntity@ ref = null) const", asMETHODPR(T, Transform, (VxVector*, const VxVector*, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void InverseTransform(VxVector &out dest, const VxVector&in src, CK3dEntity@ ref = null) const", asMETHODPR(T, InverseTransform, (VxVector*, const VxVector*, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void TransformVector(VxVector &out dest, const VxVector&in src, CK3dEntity@ ref = null) const", asMETHODPR(T, TransformVector, (VxVector*, const VxVector*, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void InverseTransformVector(VxVector &out dest, const VxVector &in src, CK3dEntity@ ref = null) const", asMETHODPR(T, InverseTransformVector, (VxVector*, const VxVector*, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void TransformMany(VxVector &out dest, const VxVector &in src, int count, CK3dEntity@ ref = null) const", asMETHODPR(T, TransformMany, (VxVector*, const VxVector*, int, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void InverseTransformMany(VxVector &out dest, const VxVector &in src, int count, CK3dEntity@ ref = null) const", asMETHODPR(T, InverseTransformMany, (VxVector*, const VxVector*, int, CK3dEntity*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void ChangeReferential(CK3dEntity@ ref = null)", asMETHODPR(T, ChangeReferential, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKPlace@ GetReferencePlace() const", asMETHODPR(T, GetReferencePlace, () const, CKPlace*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void AddObjectAnimation(CKObjectAnimation@ anim)", asMETHODPR(T, AddObjectAnimation, (CKObjectAnimation*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void RemoveObjectAnimation(CKObjectAnimation@ anim)", asMETHODPR(T, RemoveObjectAnimation, (CKObjectAnimation*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObjectAnimation@ GetObjectAnimation(int index) const", asMETHODPR(T, GetObjectAnimation, (int) const, CKObjectAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetObjectAnimationCount() const", asMETHODPR(T, GetObjectAnimationCount, () const, int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKSkin@ CreateSkin()", asMETHODPR(T, CreateSkin, (), CKSkin*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool DestroySkin()", asMETHODPR(T, DestroySkin, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool UpdateSkin()", asMETHODPR(T, UpdateSkin, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKSkin@ GetSkin() const", asMETHODPR(T, GetSkin, () const, CKSkin*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void UpdateBox(bool world = true)", asMETHODPR(T, UpdateBox, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxBbox &GetBoundingBox(bool local = false)", asMETHODPR(T, GetBoundingBox, (CKBOOL), const VxBbox&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool SetBoundingBox(const VxBbox &in bbox, bool local = false)", asMETHODPR(T, SetBoundingBox, (const VxBbox*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxBbox &GetHierarchicalBox(bool local = false)", asMETHODPR(T, GetHierarchicalBox, (CKBOOL), const VxBbox&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetBaryCenter(VxVector&out Pos)", asMETHODPR(T, GetBaryCenter, (VxVector*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "float GetRadius()", asMETHODPR(T, GetRadius, (), float), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, name) != 0) {
        RegisterClassRefCast<T, CK3dEntity>(engine, name, name);
    }
}

void RegisterCK3dEntity(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCK3dEntityMembers<CK3dEntity>(engine, "CK3dEntity");
}

template<typename T>
void RegisterCKCameraMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "float GetFrontPlane() const", asMETHODPR(T, GetFrontPlane, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFrontPlane(float front)", asMETHODPR(T, SetFrontPlane, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetBackPlane() const", asMETHODPR(T, GetBackPlane, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetBackPlane(float back)", asMETHODPR(T, SetBackPlane, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "float GetFov() const", asMETHODPR(T, GetFov, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFov(float fov)", asMETHODPR(T, SetFov, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "int GetProjectionType() const", asMETHODPR(T, GetProjectionType, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetProjectionType(int proj)", asMETHODPR(T, SetProjectionType, (int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetOrthographicZoom(float zoom)", asMETHODPR(T, SetOrthographicZoom, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetOrthographicZoom() const", asMETHODPR(T, GetOrthographicZoom, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetAspectRatio(int width, int height)", asMETHODPR(T, SetAspectRatio, (int, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void GetAspectRatio(int &out width, int &out height)", asMETHODPR(T, GetAspectRatio, (int &, int &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ComputeProjectionMatrix(VxMatrix &out mat)", asMETHODPR(T, ComputeProjectionMatrix, (VxMatrix &), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void ResetRoll()", asMETHODPR(T, ResetRoll, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void Roll(float angle)", asMETHODPR(T, Roll, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetTarget() const", asMETHODPR(T, GetTarget, (), CK3dEntity *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTarget(CK3dEntity@ target)", asMETHODPR(T, SetTarget, (CK3dEntity *), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, name) != 0) {
        RegisterClassRefCast<T, CKCamera>(engine, name, name);
    }
}

void RegisterCKCamera(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKCameraMembers<CKCamera>(engine, "CKCamera");
}

void RegisterCKTargetCamera(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKCameraMembers<CKTargetCamera>(engine, "CKTargetCamera");
}

void RegisterCKPlace(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKPlace>(engine, "CKPlace");

    r = engine->RegisterObjectMethod("CKPlace", "CKCamera@ GetDefaultCamera() const", asMETHODPR(CKPlace, GetDefaultCamera, (), CKCamera*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPlace", "void SetDefaultCamera(CKCamera@ cam)", asMETHODPR(CKPlace, SetDefaultCamera, (CKCamera*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPlace", "void AddPortal(CKPlace@ place, CK3dEntity@ portal)", asMETHODPR(CKPlace, AddPortal, (CKPlace*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPlace", "void RemovePortal(CKPlace@ place, CK3dEntity@ portal)", asMETHODPR(CKPlace, RemovePortal, (CKPlace*, CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPlace", "int GetPortalCount() const", asMETHODPR(CKPlace, GetPortalCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPlace", "CKPlace@ GetPortal(int i, CK3dEntity@ &out portal)", asMETHODPR(CKPlace, GetPortal, (int, CK3dEntity**), CKPlace*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPlace", "VxRect &ViewportClip()", asMETHODPR(CKPlace, ViewportClip, (), VxRect&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPlace", "bool ComputeBestFitBBox(CKPlace@ p2, VxMatrix &out bBoxMatrix)", asMETHODPR(CKPlace, ComputeBestFitBBox, (CKPlace*, VxMatrix&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKCurvePoint(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKCurvePoint>(engine, "CKCurvePoint");

    r = engine->RegisterObjectMethod("CKCurvePoint", "CKCurve@ GetCurve() const", asMETHODPR(CKCurvePoint, GetCurve, (), CKCurve*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "float GetBias() const", asMETHODPR(CKCurvePoint, GetBias, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "void SetBias(float b)", asMETHODPR(CKCurvePoint, SetBias, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "float GetTension() const", asMETHODPR(CKCurvePoint, GetTension, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "void SetTension(float t)", asMETHODPR(CKCurvePoint, SetTension, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "float GetContinuity() const", asMETHODPR(CKCurvePoint, GetContinuity, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "void SetContinuity(float c)", asMETHODPR(CKCurvePoint, SetContinuity, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "bool IsLinear() const", asMETHODPR(CKCurvePoint, IsLinear, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "void SetLinear(bool linear = false)", asMETHODPR(CKCurvePoint, SetLinear, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "void UseTCB(bool use = true)", asMETHODPR(CKCurvePoint, UseTCB, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "bool IsTCB() const", asMETHODPR(CKCurvePoint, IsTCB, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "float GetLength() const", asMETHODPR(CKCurvePoint, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "void GetTangents(VxVector &out input, VxVector &out output) const", asMETHODPR(CKCurvePoint, GetTangents, (VxVector*, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurvePoint", "void SetTangents(const VxVector &in input, const VxVector &in output)", asMETHODPR(CKCurvePoint, SetTangents, (VxVector*, VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurvePoint", "void NotifyUpdate()", asMETHODPR(CKCurvePoint, NotifyUpdate, (), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKSprite3D(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKSprite3D>(engine, "CKSprite3D");

    r = engine->RegisterObjectMethod("CKSprite3D", "void SetMaterial(CKMaterial@ mat)", asMETHODPR(CKSprite3D, SetMaterial, (CKMaterial*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSprite3D", "CKMaterial@ GetMaterial() const", asMETHODPR(CKSprite3D, GetMaterial, (), CKMaterial*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSprite3D", "void SetSize(const Vx2DVector &in vect)", asMETHODPR(CKSprite3D, SetSize, (Vx2DVector&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSprite3D", "void GetSize(Vx2DVector &out vect) const", asMETHODPR(CKSprite3D, GetSize, (Vx2DVector&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSprite3D", "void SetOffset(const Vx2DVector &in vect)", asMETHODPR(CKSprite3D, SetOffset, (Vx2DVector&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSprite3D", "void GetOffset(Vx2DVector &out vect) const", asMETHODPR(CKSprite3D, GetOffset, (Vx2DVector&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSprite3D", "void SetUVMapping(const VxRect &in rect)", asMETHODPR(CKSprite3D, SetUVMapping, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSprite3D", "void GetUVMapping(VxRect &out rect) const", asMETHODPR(CKSprite3D, GetUVMapping, (VxRect&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSprite3D", "void SetMode(VXSPRITE3D_TYPE mode)", asMETHODPR(CKSprite3D, SetMode, (VXSPRITE3D_TYPE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSprite3D", "VXSPRITE3D_TYPE GetMode() const", asMETHODPR(CKSprite3D, GetMode, (), VXSPRITE3D_TYPE), asCALL_THISCALL); assert(r >= 0);
}

template<typename T>
void RegisterCKLightMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<T>(engine, name);

    r = engine->RegisterObjectMethod(name, "void SetColor(const VxColor &in c)", asMETHODPR(T, SetColor, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "const VxColor &GetColor() const", asMETHODPR(T, GetColor, (), const VxColor&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void SetConstantAttenuation(float value)", asMETHODPR(T, SetConstantAttenuation, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLinearAttenuation(float value)", asMETHODPR(T, SetLinearAttenuation, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetQuadraticAttenuation(float value)", asMETHODPR(T, SetQuadraticAttenuation, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetConstantAttenuation() const", asMETHODPR(T, GetConstantAttenuation, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetLinearAttenuation() const", asMETHODPR(T, GetLinearAttenuation, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetQuadraticAttenuation() const", asMETHODPR(T, GetQuadraticAttenuation, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "VXLIGHT_TYPE GetType() const", asMETHODPR(T, GetType, (), VXLIGHT_TYPE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetType(VXLIGHT_TYPE type)", asMETHODPR(T, SetType, (VXLIGHT_TYPE), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "float GetRange() const", asMETHODPR(T, GetRange, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetRange(float value)", asMETHODPR(T, SetRange, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "float GetHotSpot() const", asMETHODPR(T, GetHotSpot, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetFallOff() const", asMETHODPR(T, GetFallOff, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetHotSpot(float value)", asMETHODPR(T, SetHotSpot, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFallOff(float value)", asMETHODPR(T, SetFallOff, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "float GetFallOffShape() const", asMETHODPR(T, GetFallOffShape, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetFallOffShape(float value)", asMETHODPR(T, SetFallOffShape, (float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void Active(bool active)", asMETHODPR(T, Active, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetActivity() const", asMETHODPR(T, GetActivity, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetSpecularFlag(bool specular)", asMETHODPR(T, SetSpecularFlag, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "bool GetSpecularFlag() const", asMETHODPR(T, GetSpecularFlag, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CK3dEntity@ GetTarget() const", asMETHODPR(T, GetTarget, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetTarget(CK3dEntity@ target)", asMETHODPR(T, SetTarget, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "float GetLightPower() const", asMETHODPR(T, GetLightPower, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void SetLightPower(float power = 1.0)", asMETHODPR(T, SetLightPower, (float), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, name) != 0) {
        RegisterClassRefCast<T, CKLight>(engine, name, name);
    }
}

void RegisterCKLight(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKLightMembers<CKLight>(engine, "CKLight");
}

void RegisterCKTargetLight(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKLightMembers<CKTargetLight>(engine, "CKTargetLight");
}

void RegisterCKCharacter(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKCharacter>(engine, "CKCharacter");

    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR AddBodyPart(CKBodyPart@ part)", asMETHODPR(CKCharacter, AddBodyPart, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR RemoveBodyPart(CKBodyPart@ part)", asMETHODPR(CKCharacter, RemoveBodyPart, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKBodyPart@ GetRootBodyPart()", asMETHODPR(CKCharacter, GetRootBodyPart, (), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR SetRootBodyPart(CKBodyPart@ part)", asMETHODPR(CKCharacter, SetRootBodyPart, (CKBodyPart*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKBodyPart@ GetBodyPart(int index)", asMETHODPR(CKCharacter, GetBodyPart, (int), CKBodyPart*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "int GetBodyPartCount()", asMETHODPR(CKCharacter, GetBodyPartCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR AddAnimation(CKAnimation@ anim)", asMETHODPR(CKCharacter, AddAnimation, (CKAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR RemoveAnimation(CKAnimation@ anim)", asMETHODPR(CKCharacter, RemoveAnimation, (CKAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKAnimation@ GetAnimation(int index)", asMETHODPR(CKCharacter, GetAnimation, (int), CKAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "int GetAnimationCount()", asMETHODPR(CKCharacter, GetAnimationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKAnimation@ GetWarper()", asMETHODPR(CKCharacter, GetWarper, (), CKAnimation*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCharacter", "CKAnimation@ GetActiveAnimation()", asMETHODPR(CKCharacter, GetActiveAnimation, (), CKAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKAnimation@ GetNextActiveAnimation()", asMETHODPR(CKCharacter, GetNextActiveAnimation, (), CKAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR SetActiveAnimation(CKAnimation@ anim)", asMETHODPR(CKCharacter, SetActiveAnimation, (CKAnimation*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR SetNextActiveAnimation(CKAnimation@ anim, uint transitionMode, float warpLength = 0.0)", asMETHODPR(CKCharacter, SetNextActiveAnimation, (CKAnimation*, CKDWORD, float), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCharacter", "void ProcessAnimation(float delta = 1.0)", asMETHODPR(CKCharacter, ProcessAnimation, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void SetAutomaticProcess(bool process = true)", asMETHODPR(CKCharacter, SetAutomaticProcess, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "bool IsAutomaticProcess()", asMETHODPR(CKCharacter, IsAutomaticProcess, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void GetEstimatedVelocity(float delta, VxVector &out velocity)", asMETHODPR(CKCharacter, GetEstimatedVelocity, (float, VxVector*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR PlaySecondaryAnimation(CKAnimation@ anim, float startingFrame = 0.0, CK_SECONDARYANIMATION_FLAGS playFlags = CKSECONDARYANIMATION_ONESHOT, float warpLength = 5.0, int loopCount = 0)", asMETHODPR(CKCharacter, PlaySecondaryAnimation, (CKAnimation*, float, CK_SECONDARYANIMATION_FLAGS, float, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKERROR StopSecondaryAnimation(CKAnimation@ anim, bool warp = false, float warpLength = 5.0)", asMETHODPR(CKCharacter, StopSecondaryAnimation, (CKAnimation*, CKBOOL, float), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "int GetSecondaryAnimationsCount()", asMETHODPR(CKCharacter, GetSecondaryAnimationsCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CKAnimation@ GetSecondaryAnimation(int index)", asMETHODPR(CKCharacter, GetSecondaryAnimation, (int), CKAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void FlushSecondaryAnimations()", asMETHODPR(CKCharacter, FlushSecondaryAnimations, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCharacter", "void AlignCharacterWithRootPosition()", asMETHODPR(CKCharacter, AlignCharacterWithRootPosition, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "CK3dEntity@ GetFloorReferenceObject()", asMETHODPR(CKCharacter, GetFloorReferenceObject, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void SetFloorReferenceObject(CK3dEntity@ floorRef)", asMETHODPR(CKCharacter, SetFloorReferenceObject, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void SetAnimationLevelOfDetail(float lod)", asMETHODPR(CKCharacter, SetAnimationLevelOfDetail, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "float GetAnimationLevelOfDetail()", asMETHODPR(CKCharacter, GetAnimationLevelOfDetail, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCharacter", "void GetWarperParameters(uint &out transitionMode, CKAnimation@ &out animSrc, float &out frameSrc, CKAnimation@ &out animDest, float &out frameDest)", asMETHODPR(CKCharacter, GetWarperParameters, (CKDWORD*, CKAnimation**, float*, CKAnimation**, float*), void), asCALL_THISCALL); assert(r >= 0);
}

template<typename T>
void RegisterCK3dObjectMembers(asIScriptEngine *engine, const char *name) {
    assert(engine != nullptr);

    RegisterCK3dEntityMembers<T>(engine, name);

    if (strcmp(name, name) != 0) {
        RegisterClassRefCast<T, CK3dObject>(engine, name, name);
    }
}

void RegisterCK3dObject(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCK3dObjectMembers<CK3dObject>(engine, "CK3dObject");
}

void RegisterCKBodyPart(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dObjectMembers<CKBodyPart>(engine, "CKBodyPart");

    r = engine->RegisterObjectMethod("CKBodyPart", "CKCharacter@ GetCharacter() const", asMETHODPR(CKBodyPart, GetCharacter, () const, CKCharacter*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBodyPart", "void SetExclusiveAnimation(const CKAnimation@ anim)", asMETHODPR(CKBodyPart, SetExclusiveAnimation, (const CKAnimation*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBodyPart", "CKAnimation@ GetExclusiveAnimation() const", asMETHODPR(CKBodyPart, GetExclusiveAnimation, () const, CKAnimation*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBodyPart", "void GetRotationJoint(CKIkJoint &out rotJoint) const", asMETHODPR(CKBodyPart, GetRotationJoint, (CKIkJoint*) const, void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBodyPart", "void SetRotationJoint(const CKIkJoint &in rotJoint)", asMETHODPR(CKBodyPart, SetRotationJoint, (const CKIkJoint*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBodyPart", "CKERROR FitToJoint()", asMETHODPR(CKBodyPart, FitToJoint, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKCurve(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKCurve>(engine, "CKCurve");

    r = engine->RegisterObjectMethod("CKCurve", "float GetLength() const", asMETHODPR(CKCurve, GetLength, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "void Open()", asMETHODPR(CKCurve, Open, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "void Close()", asMETHODPR(CKCurve, Close, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "bool IsOpen() const", asMETHODPR(CKCurve, IsOpen, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR GetPos(float step, VxVector &out pos, VxVector &out dir = void) const", asMETHODPR(CKCurve, GetPos, (float, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR GetLocalPos(float step, VxVector &out pos, VxVector &out dir = void) const", asMETHODPR(CKCurve, GetLocalPos, (float, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR GetTangents(int index, VxVector &out input, VxVector &out output) const", asMETHODPR(CKCurve, GetTangents, (int, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR GetTangents(CKCurvePoint@ pt, VxVector &out input, VxVector &out output) const", asMETHODPR(CKCurve, GetTangents, (CKCurvePoint*, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR SetTangents(int index, VxVector &in input, VxVector &in output)", asMETHODPR(CKCurve, SetTangents, (int, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR SetTangents(CKCurvePoint@ pt, VxVector &in input, VxVector &in output)", asMETHODPR(CKCurve, SetTangents, (CKCurvePoint*, VxVector*, VxVector*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "void SetFittingCoeff(float fit)", asMETHODPR(CKCurve, SetFittingCoeff, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "float GetFittingCoeff() const", asMETHODPR(CKCurve, GetFittingCoeff, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR RemoveControlPoint(CKCurvePoint@ pt, bool removeAll = false)", asMETHODPR(CKCurve, RemoveControlPoint, (CKCurvePoint*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR InsertControlPoint(CKCurvePoint@ prev, CKCurvePoint@ pt)", asMETHODPR(CKCurve, InsertControlPoint, (CKCurvePoint*, CKCurvePoint*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR AddControlPoint(CKCurvePoint@ pt)", asMETHODPR(CKCurve, AddControlPoint, (CKCurvePoint*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "int GetControlPointCount() const", asMETHODPR(CKCurve, GetControlPointCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKCurvePoint@ GetControlPoint(int pos) const", asMETHODPR(CKCurve, GetControlPoint, (int), CKCurvePoint*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR RemoveAllControlPoints()", asMETHODPR(CKCurve, RemoveAllControlPoints, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR SetStepCount(int steps)", asMETHODPR(CKCurve, SetStepCount, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "int GetStepCount() const", asMETHODPR(CKCurve, GetStepCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "CKERROR CreateLineMesh()", asMETHODPR(CKCurve, CreateLineMesh, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "CKERROR UpdateMesh()", asMETHODPR(CKCurve, UpdateMesh, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "VxColor GetColor() const", asMETHODPR(CKCurve, GetColor, (), VxColor), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCurve", "void SetColor(const VxColor &in color)", asMETHODPR(CKCurve, SetColor, (const VxColor&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCurve", "void Update()", asMETHODPR(CKCurve, Update, (), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKGrid(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCK3dEntityMembers<CKGrid>(engine, "CKGrid");

    r = engine->RegisterObjectMethod("CKGrid", "void ConstructMeshTexture(float opacity = 0.5)", asMETHODPR(CKGrid, ConstructMeshTexture, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "void DestroyMeshTexture()", asMETHODPR(CKGrid, DestroyMeshTexture, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "bool IsActive() const", asMETHODPR(CKGrid, IsActive, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "void SetHeightValidity(float heightValidity)", asMETHODPR(CKGrid, SetHeightValidity, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "float GetHeightValidity() const", asMETHODPR(CKGrid, GetHeightValidity, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "int GetWidth() const", asMETHODPR(CKGrid, GetWidth, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "int GetLength() const", asMETHODPR(CKGrid, GetLength, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "void SetDimensions(int width, int length, float sizeX, float sizeY)", asMETHODPR(CKGrid, SetDimensions, (int, int, float, float), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "float Get2dCoordsFrom3dPos(const VxVector &in pos3d, int &out x, int &out y) const", asMETHODPR(CKGrid, Get2dCoordsFrom3dPos, (const VxVector*, int*, int*), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "void Get3dPosFrom2dCoords(VxVector &out pos3d, int x, int z) const", asMETHODPR(CKGrid, Get3dPosFrom2dCoords, (VxVector*, int, int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "CKERROR AddClassification(int classification)", asMETHODPR(CKGrid, AddClassification, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR AddClassificationByName(const string &in name)", asMETHODPR(CKGrid, AddClassificationByName, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR RemoveClassification(int classification)", asMETHODPR(CKGrid, RemoveClassification, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR RemoveClassificationByName(const string &in name)", asMETHODPR(CKGrid, RemoveClassificationByName, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "bool HasCompatibleClass(CK3dEntity@ entity) const", asMETHODPR(CKGrid, HasCompatibleClass, (CK3dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "void SetGridPriority(int priority)", asMETHODPR(CKGrid, SetGridPriority, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "int GetGridPriority() const", asMETHODPR(CKGrid, GetGridPriority, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "void SetOrientationMode(CK_GRIDORIENTATION orientation)", asMETHODPR(CKGrid, SetOrientationMode, (CK_GRIDORIENTATION), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CK_GRIDORIENTATION GetOrientationMode() const", asMETHODPR(CKGrid, GetOrientationMode, (), CK_GRIDORIENTATION), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGrid", "CKLayer@ AddLayer(int type, int format = CKGRID_LAYER_FORMAT_NORMAL)", asMETHODPR(CKGrid, AddLayer, (int, int), CKLayer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKLayer@ AddLayerByName(const string &in typeName = void, int format = CKGRID_LAYER_FORMAT_NORMAL)", asMETHODPR(CKGrid, AddLayerByName, (CKSTRING, int), CKLayer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKLayer@ GetLayer(int type) const", asMETHODPR(CKGrid, GetLayer, (int), CKLayer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKLayer@ GetLayerByName(const string &in typeName) const", asMETHODPR(CKGrid, GetLayerByName, (CKSTRING), CKLayer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "int GetLayerCount() const", asMETHODPR(CKGrid, GetLayerCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKLayer@ GetLayerByIndex(int index) const", asMETHODPR(CKGrid, GetLayerByIndex, (int), CKLayer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR RemoveLayer(int type)", asMETHODPR(CKGrid, RemoveLayer, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR RemoveLayerByName(const string &in typeName)", asMETHODPR(CKGrid, RemoveLayerByName, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGrid", "CKERROR RemoveAllLayers()", asMETHODPR(CKGrid, RemoveAllLayers, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
}
