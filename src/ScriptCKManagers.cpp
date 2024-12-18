#include "ScriptCKManagers.h"

#include <string>

#include "CKPluginManager.h"

// Builtin Managers
#include "CKBaseManager.h"
#include "CKParameterManager.h"
#include "CKTimeManager.h"
#include "CKMessageManager.h"
#include "CKRenderManager.h"
#include "CKBehaviorManager.h"
#include "CKAttributeManager.h"
#include "CKPathManager.h"

// External Managers
#include "CKFloorManager.h"
#include "CKGridManager.h"
#include "CKInterfaceManager.h"
#include "CKSoundManager.h"
#include "CKMidiManager.h"
#include "CKInputManager.h"
#include "CKCollisionManager.h"

#include "ScriptUtils.h"

void RegisterCKPluginManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    r = engine->RegisterObjectMethod("CKPluginManager", "int ParsePlugins(const string &in directory)", asMETHODPR(CKPluginManager, ParsePlugins, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR RegisterPlugin(const string &in plugin)", asMETHODPR(CKPluginManager, RegisterPlugin, (CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "CKPluginEntry &FindComponent(CKGUID component, int catIdx = -1)", asMETHODPR(CKPluginManager, FindComponent, (CKGUID, int), CKPluginEntry *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "int AddCategory(const string &in category)", asMETHODPR(CKPluginManager, AddCategory, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR RemoveCategory(int catIdx)", asMETHODPR(CKPluginManager, RemoveCategory, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "int GetCategoryCount() const", asMETHODPR(CKPluginManager, GetCategoryCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "string GetCategoryName(int index)", asMETHODPR(CKPluginManager, GetCategoryName, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "int GetCategoryIndex(const string &in category)", asMETHODPR(CKPluginManager, GetCategoryIndex, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR RenameCategory(int catIdx, const string &in newName)", asMETHODPR(CKPluginManager, RenameCategory, (int, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "int GetPluginDllCount() const", asMETHODPR(CKPluginManager, GetPluginDllCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKPluginDll &GetPluginDllInfo(int idx)", asMETHODPR(CKPluginManager, GetPluginDllInfo, (int), CKPluginDll*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKPluginDll &GetPluginDllInfo(const string &in name, int &out idx = void)", asMETHODPR(CKPluginManager, GetPluginDllInfo, (CKSTRING, int *), CKPluginDll *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR UnLoadPluginDll(int idx)", asMETHODPR(CKPluginManager, UnLoadPluginDll, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR ReLoadPluginDll(int idx)", asMETHODPR(CKPluginManager, ReLoadPluginDll, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "int GetPluginCount(int catIdx)", asMETHODPR(CKPluginManager, GetPluginCount, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKPluginEntry &GetPluginInfo(int catIdx, int pluginIdx)", asMETHODPR(CKPluginManager, GetPluginInfo, (int, int), CKPluginEntry*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "bool SetReaderOptionData(CKContext@ context, NativePointer data, CKParameterOut @param, CKFileExtension ext, CKGUID &in guid = void)", asMETHODPR(CKPluginManager, SetReaderOptionData, (CKContext *, void *, CKParameterOut *, CKFileExtension, CKGUID *), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKParameterOut@ GetReaderOptionData(CKContext@ context, NativePointer data, CKFileExtension ext, CKGUID &in guid = void)", asMETHODPR(CKPluginManager, GetReaderOptionData, (CKContext *, void *, CKFileExtension, CKGUID *), CKParameterOut *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "CKBitmapReader@ GetBitmapReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)",asMETHODPR(CKPluginManager, GetBitmapReader, (CKFileExtension &, CKGUID *), CKBitmapReader *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKSoundReader@ GetSoundReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)",asMETHODPR(CKPluginManager, GetSoundReader, (CKFileExtension &, CKGUID *), CKSoundReader *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKModelReader@ GetModelReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)",asMETHODPR(CKPluginManager, GetModelReader, (CKFileExtension &, CKGUID *), CKModelReader *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKMovieReader@ GetMovieReader(CKFileExtension &in ext, CKGUID &in preferredGUID = void)",asMETHODPR(CKPluginManager, GetMovieReader, (CKFileExtension &, CKGUID *), CKMovieReader *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR Load(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS loadFlags, CKCharacter@ carac = null, CKGUID &readerGuid = void)", asMETHODPR(CKPluginManager, Load, (CKContext *, CKSTRING, CKObjectArray *, CK_LOAD_FLAGS, CKCharacter *, CKGUID *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPluginManager", "CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS saveFlags, CKGUID &readerGuid = void)", asMETHODPR(CKPluginManager, Save, (CKContext *, CKSTRING, CKObjectArray *, CKDWORD, CKGUID *), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // Not existing in Virtools 2.1
    // r = engine->RegisterObjectMethod("CKPluginManager", "void ReleaseAllPlugins()", asMETHODPR(CKPluginManager, ReleaseAllPlugins, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKPluginManager", "void InitializePlugins(CKContext@ context)", asMETHODPR(CKPluginManager, InitializePlugins, (CKContext *), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKPluginManager", "void ComputeDependenciesList(CKFile@ file)", asMETHODPR(CKPluginManager, ComputeDependenciesList, (CKFile *), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKPluginManager", "void MarkComponentAsNeeded(CKGUID component, int catIdx)", asMETHODPR(CKPluginManager, MarkComponentAsNeeded, (CKGUID, int), void), asCALL_THISCALL); assert(r >= 0);
}

CKERROR CKBaseManager_SequenceAddedToScene(CKBaseManager *man, CKScene *scn, XObjectArray &array) {
    return man->SequenceAddedToScene(scn, array.Begin(), array.Size());
}

CKERROR CKBaseManager_SequenceRemovedFromScene(CKBaseManager *man, CKScene *scn, XObjectArray &array) {
    return man->SequenceRemovedFromScene(scn, array.Begin(), array.Size());
}

CKERROR CKBaseManager_SequenceToBeDeleted(CKBaseManager *man, XObjectArray &array) {
    return man->SequenceToBeDeleted(array.Begin(), array.Size());
}

CKERROR CKBaseManager_SequenceDeleted(CKBaseManager *man, XObjectArray &array) {
    return man->SequenceDeleted(array.Begin(), array.Size());
}

CKERROR CKBaseManager_CKDestroyObjects(CKBaseManager *man, XObjectArray &array, CKDWORD flags, CKDependencies *depoptions) {
    return man->CKDestroyObjects(array.Begin(), array.Size(), flags, depoptions);
}

template <typename T>
static void RegisterCKBaseManagerMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "CKGUID GetGuid() const", asMETHODPR(T, GetGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "string GetName() const", asMETHODPR(T, GetName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ SaveData(CKFile@ savedFile) const", asMETHODPR(T, SaveData, (CKFile*), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR LoadData(CKStateChunk@ chunk, CKFile@ loadedFile)", asMETHODPR(T, LoadData, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreClearAll()", asMETHODPR(T, PreClearAll, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostClearAll()", asMETHODPR(T, PostClearAll, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreProcess()", asMETHODPR(T, PreProcess, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostProcess()", asMETHODPR(T, PostProcess, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR SequenceAddedToScene(CKScene@ scene, XObjectArray &in objects)", asFUNCTIONPR(CKBaseManager_SequenceAddedToScene, (CKBaseManager *, CKScene *, XObjectArray &), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceRemovedFromScene(CKScene@ scene, XObjectArray &in objects)",asFUNCTIONPR(CKBaseManager_SequenceRemovedFromScene, (CKBaseManager *, CKScene *, XObjectArray &), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreLaunchScene(CKScene@ oldScene, CKScene@ newScene)", asMETHODPR(T, PreLaunchScene, (CKScene*, CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostLaunchScene(CKScene@ oldScene, CKScene@ newScene)", asMETHODPR(T, PostLaunchScene, (CKScene*, CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKInit()", asMETHODPR(T, OnCKInit, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKEnd()", asMETHODPR(T, OnCKEnd, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKReset()", asMETHODPR(T, OnCKReset, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPostReset()", asMETHODPR(T, OnCKPostReset, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPause()", asMETHODPR(T, OnCKPause, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPlay()", asMETHODPR(T, OnCKPlay, (), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR SequenceToBeDeleted(XObjectArray &in objects)", asFUNCTIONPR(CKBaseManager_SequenceToBeDeleted, (CKBaseManager *, XObjectArray &), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceDeleted(XObjectArray &in objects)", asFUNCTIONPR(CKBaseManager_SequenceDeleted, (CKBaseManager *, XObjectArray &), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreLoad()", asMETHODPR(T, PreLoad, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostLoad()", asMETHODPR(T, PostLoad, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreSave()", asMETHODPR(T, PreSave, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostSave()", asMETHODPR(T, PostSave, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPreCopy(CKDependenciesContext &in context)", asMETHODPR(T, OnPreCopy, (CKDependenciesContext &), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostCopy(CKDependenciesContext &in context)", asMETHODPR(T, OnPostCopy, (CKDependenciesContext &), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPreRender(CKRenderContext@ dev)", asMETHODPR(T, OnPreRender, (CKRenderContext *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostRender(CKRenderContext@ dev)", asMETHODPR(T, OnPostRender, (CKRenderContext *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostSpriteRender(CKRenderContext@ dev)", asMETHODPR(T, OnPostSpriteRender, (CKRenderContext *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetFunctionPriority(CKMANAGER_FUNCTIONS functions)", asMETHODPR(T, GetFunctionPriority, (CKMANAGER_FUNCTIONS), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetValidFunctionsMask() const", asMETHODPR(T, GetValidFunctionsMask, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(T, CKDestroyObject, (CKObject *, CKDWORD, CKDependencies *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID id, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(T, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies *), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(XObjectArray &in, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asFUNCTIONPR(CKBaseManager_CKDestroyObjects, (CKBaseManager *, XObjectArray &, CKDWORD, CKDependencies *), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID id)", asMETHODPR(T, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod(name, "void StartProfile()", asMETHODPR(T, StartProfile, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "void StopProfile()", asMETHODPR(T, StopProfile, (), void), asCALL_THISCALL); assert(r >= 0);

    if (strcmp(name, "CKBaseManager") != 0) {
        RegisterClassRefCast<T, CKBaseManager>(engine, name, "CKBaseManager");
    }
}

void RegisterCKBaseManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterCKBaseManagerMembers<CKBaseManager>(engine, "CKBaseManager");
}

void RegisterCKParameterManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKParameterManager>(engine, "CKParameterManager");

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterParameterType(CKParameterTypeDesc &in type)", asMETHODPR(CKParameterManager, RegisterParameterType, (CKParameterTypeDesc*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterParameterType(CKGUID guid)", asMETHODPR(CKParameterManager, UnRegisterParameterType, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterTypeDesc &GetParameterTypeDescription(int type)", asMETHODPR(CKParameterManager, GetParameterTypeDescription, (int), CKParameterTypeDesc*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterTypeDesc &GetParameterTypeDescription(CKGUID guid)", asMETHODPR(CKParameterManager, GetParameterTypeDescription, (CKGUID), CKParameterTypeDesc*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetParameterSize(CKParameterType type)", asMETHODPR(CKParameterManager, GetParameterSize, (CKParameterType), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetParameterTypesCount()", asMETHODPR(CKParameterManager, GetParameterTypesCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ParameterGuidToType(CKGUID guid)", asMETHODPR(CKParameterManager, ParameterGuidToType, (CKGUID), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string ParameterGuidToName(CKGUID guid)", asMETHODPR(CKParameterManager, ParameterGuidToName, (CKGUID), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ParameterTypeToGuid(CKParameterType type)", asMETHODPR(CKParameterManager, ParameterTypeToGuid, (CKParameterType), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string ParameterTypeToName(CKParameterType type)", asMETHODPR(CKParameterManager, ParameterTypeToName, (CKParameterType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ParameterNameToGuid(const string &in name)", asMETHODPR(CKParameterManager, ParameterNameToGuid, (CKSTRING), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ParameterNameToType(const string &in name)", asMETHODPR(CKParameterManager, ParameterNameToType, (CKSTRING), CKParameterType), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsDerivedFrom(CKGUID guid1, CKGUID guid2)", asMETHODPR(CKParameterManager, IsDerivedFrom, (CKGUID, CKGUID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsDerivedFrom(CKParameterType child, CKParameterType parent)", asMETHODPR(CKParameterManager, IsDerivedFrom, (CKParameterType, CKParameterType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsTypeCompatible(CKGUID guid1, CKGUID guid2)", asMETHODPR(CKParameterManager, IsTypeCompatible, (CKGUID, CKGUID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsTypeCompatible(CKParameterType type1, CKParameterType type2)", asMETHODPR(CKParameterManager, IsTypeCompatible, (CKParameterType, CKParameterType), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "int TypeToClassID(CKParameterType type)", asMETHODPR(CKParameterManager, TypeToClassID, (CKParameterType), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GuidToClassID(CKGUID guid)", asMETHODPR(CKParameterManager, GuidToClassID, (CKGUID), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ClassIDToType(CK_CLASSID cid)", asMETHODPR(CKParameterManager, ClassIDToType, (CK_CLASSID), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ClassIDToGuid(CK_CLASSID cid)", asMETHODPR(CKParameterManager, ClassIDToGuid, (CK_CLASSID), CKGUID), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewFlags(CKGUID flagsGuid, const string &in flagsName, const string &in flagsData)", asMETHODPR(CKParameterManager, RegisterNewFlags, (CKGUID, CKSTRING, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewEnum(CKGUID enumGuid, const string &in enumName, const string &in enumData)", asMETHODPR(CKParameterManager, RegisterNewEnum, (CKGUID, CKSTRING, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR ChangeEnumDeclaration(CKGUID enumGuid, const string &in enumData)", asMETHODPR(CKParameterManager, ChangeEnumDeclaration, (CKGUID, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR ChangeFlagsDeclaration(CKGUID flagsGuid, const string &in flagsData)", asMETHODPR(CKParameterManager, ChangeFlagsDeclaration, (CKGUID, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewStructure(const CKGUID &in, const string &in, const string &in, ...)", asMETHODPR(CKParameterManager, RegisterNewStructure, (CKGUID, CKSTRING, CKSTRING, ...), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewStructure(const CKGUID &in, const string &in, const string &in, XGUIDArray &in)", asMETHODPR(CKParameterManager, RegisterNewStructure, (CKGUID, CKSTRING, CKSTRING, XArray<CKGUID> &), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbFlagDefined()", asMETHODPR(CKParameterManager, GetNbFlagDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbEnumDefined()", asMETHODPR(CKParameterManager, GetNbEnumDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbStructDefined()", asMETHODPR(CKParameterManager, GetNbStructDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKFlagsStruct &GetFlagsDescByType(CKParameterType type)", asMETHODPR(CKParameterManager, GetFlagsDescByType, (CKParameterType), CKFlagsStruct*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKEnumStruct &GetEnumDescByType(CKParameterType type)", asMETHODPR(CKParameterManager, GetEnumDescByType, (CKParameterType), CKEnumStruct*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKStructStruct &GetStructDescByType(CKParameterType type)", asMETHODPR(CKParameterManager, GetStructDescByType, (CKParameterType), CKStructStruct*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKOperationType RegisterOperationType(CKGUID opGuid, const string &in name)", asMETHODPR(CKParameterManager, RegisterOperationType, (CKGUID, CKSTRING), CKOperationType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationType(CKGUID opGuid)", asMETHODPR(CKParameterManager, UnRegisterOperationType, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationType(CKOperationType opCode)", asMETHODPR(CKParameterManager, UnRegisterOperationType, (CKOperationType), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2, NativePointer op)", asMETHODPR(CKParameterManager, RegisterOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &, CK_PARAMETEROPERATION), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "NativePointer GetOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2)", asMETHODPR(CKParameterManager, GetOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &), CK_PARAMETEROPERATION), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationFunction(const CKGUID &in operation, const CKGUID &in paramRes, const CKGUID &in param1, const CKGUID &in param2)", asMETHODPR(CKParameterManager, UnRegisterOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID OperationCodeToGuid(CKOperationType type)", asMETHODPR(CKParameterManager, OperationCodeToGuid, (CKOperationType), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string OperationCodeToName(CKOperationType type)", asMETHODPR(CKParameterManager, OperationCodeToName, (CKOperationType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int OperationGuidToCode(CKGUID guid)", asMETHODPR(CKParameterManager, OperationGuidToCode, (CKGUIDCONSTREF), CKOperationType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string OperationGuidToName(CKGUID guid)", asMETHODPR(CKParameterManager, OperationGuidToName, (CKGUIDCONSTREF), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID OperationNameToGuid(const string &in name)", asMETHODPR(CKParameterManager, OperationNameToGuid, (CKSTRING), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int OperationNameToCode(const string &in name)", asMETHODPR(CKParameterManager, OperationNameToCode, (CKSTRING), CKOperationType), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "int GetAvailableOperationsDesc(const CKGUID &in opGuid, CKParameterOut@ res, CKParameterIn@ p1, CKParameterIn@ p2, CKOperationDesc &out list)", asMETHODPR(CKParameterManager, GetAvailableOperationsDesc, (const CKGUID&, CKParameterOut*, CKParameterIn*, CKParameterIn*, CKOperationDesc*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetParameterOperationCount()", asMETHODPR(CKParameterManager, GetParameterOperationCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsParameterTypeToBeShown(CKParameterType type)", asMETHODPR(CKParameterManager, IsParameterTypeToBeShown, (CKParameterType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsParameterTypeToBeShown(CKGUID guid)", asMETHODPR(CKParameterManager, IsParameterTypeToBeShown, (CKGUIDCONSTREF), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Not existing in Virtools 2.1
    // r = engine->RegisterObjectMethod("CKParameterManager", "void UpdateParameterEnum()", asMETHODPR(CKParameterManager, UpdateParameterEnum, (), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectProperty("CKParameterManager", "bool m_ParameterTypeEnumUpToDate", asOFFSET(CKParameterManager, m_ParameterTypeEnumUpToDate)); assert(r >= 0);
}

void RegisterCKAttributeManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKAttributeManager>(engine, "CKAttributeManager");

    r = engine->RegisterObjectMethod("CKAttributeManager", "CKAttributeType RegisterNewAttributeType(const string &in name, CKGUID type, CK_CLASSID compatibleCid = CKCID_BEOBJECT, CK_ATTRIBUT_FLAGS flags = CK_ATTRIBUT_SYSTEM)", asMETHODPR(CKAttributeManager, RegisterNewAttributeType, (CKSTRING, CKGUID, CK_CLASSID, CK_ATTRIBUT_FLAGS), CKAttributeType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "void UnRegisterAttribute(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, UnRegisterAttribute, (CKAttributeType), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "void UnRegisterAttribute(const string &in name)", asMETHODPR(CKAttributeManager, UnRegisterAttribute, (CKSTRING), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "string GetAttributeNameByType(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeNameByType, (CKAttributeType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "int GetAttributeTypeByName(const string &in attribName)", asMETHODPR(CKAttributeManager, GetAttributeTypeByName, (CKSTRING), CKAttributeType), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "void SetAttributeNameByType(CKAttributeType attribType, const string &in name)", asMETHODPR(CKAttributeManager, SetAttributeNameByType, (CKAttributeType, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "int GetAttributeCount()", asMETHODPR(CKAttributeManager, GetAttributeCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "CKGUID GetAttributeParameterGUID(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeParameterGUID, (CKAttributeType), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "CKParameterType GetAttributeParameterType(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeParameterType, (CKAttributeType), CKParameterType), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "CK_CLASSID GetAttributeCompatibleClassId(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeCompatibleClassId, (CKAttributeType), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "bool IsAttributeIndexValid(CKAttributeType index)", asMETHODPR(CKAttributeManager, IsAttributeIndexValid, (CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "bool IsCategoryIndexValid(CKAttributeCategory index)", asMETHODPR(CKAttributeManager, IsCategoryIndexValid, (CKAttributeCategory), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "CK_ATTRIBUT_FLAGS GetAttributeFlags(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeFlags, (CKAttributeType), CK_ATTRIBUT_FLAGS), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "void SetAttributeCallbackFunction(CKAttributeType attribType, NativePointer callback, NativePointer arg)", asMETHODPR(CKAttributeManager, SetAttributeCallbackFunction, (CKAttributeType, CKATTRIBUTECALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "void SetAttributeDefaultValue(CKAttributeType attribType, const string &in defaultVal)", asMETHODPR(CKAttributeManager, SetAttributeDefaultValue, (CKAttributeType, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "string GetAttributeDefaultValue(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeDefaultValue, (CKAttributeType), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "const XObjectPointerArray &GetAttributeListPtr(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeListPtr, (CKAttributeType), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "const XObjectPointerArray &GetGlobalAttributeListPtr(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetGlobalAttributeListPtr, (CKAttributeType), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKAttributeManager", "const XObjectPointerArray &FillListByAttributes(int[] &in attribList, int count)", asMETHODPR(CKAttributeManager, FillListByAttributes, (CKAttributeType*, int), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKAttributeManager", "const XObjectPointerArray &FillListByGlobalAttributes(int[] &in attribList, int count)", asMETHODPR(CKAttributeManager, FillListByGlobalAttributes, (CKAttributeType*, int), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "int GetCategoriesCount()", asMETHODPR(CKAttributeManager, GetCategoriesCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "string GetCategoryName(CKAttributeCategory index)", asMETHODPR(CKAttributeManager, GetCategoryName, (CKAttributeCategory), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "CKAttributeCategory GetCategoryByName(const string &in name)", asMETHODPR(CKAttributeManager, GetCategoryByName, (CKSTRING), CKAttributeCategory), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "void SetCategoryName(CKAttributeCategory catType, const string &in name)", asMETHODPR(CKAttributeManager, SetCategoryName, (CKAttributeCategory, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "CKAttributeCategory AddCategory(const string &in category, CKDWORD flags = 0)", asMETHODPR(CKAttributeManager, AddCategory, (CKSTRING, CKDWORD), CKAttributeCategory), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "void RemoveCategory(const string &in category)", asMETHODPR(CKAttributeManager, RemoveCategory, (CKSTRING), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "CKDWORD GetCategoryFlags(CKAttributeCategory cat)", asMETHODPR(CKAttributeManager, GetCategoryFlags, (CKAttributeCategory), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "CKDWORD GetCategoryFlags(const string &in cat)", asMETHODPR(CKAttributeManager, GetCategoryFlags, (CKSTRING), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKAttributeManager", "void SetAttributeCategory(CKAttributeType attribType, const string &in category)", asMETHODPR(CKAttributeManager, SetAttributeCategory, (CKAttributeType, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "string GetAttributeCategory(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeCategory, (CKAttributeType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKAttributeManager", "CKAttributeCategory GetAttributeCategoryIndex(CKAttributeType attribType)", asMETHODPR(CKAttributeManager, GetAttributeCategoryIndex, (CKAttributeType), CKAttributeCategory), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKTimeManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKTimeManager>(engine, "CKTimeManager");

    r = engine->RegisterObjectMethod("CKTimeManager", "uint GetMainTickCount()", asMETHOD(CKTimeManager, GetMainTickCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetTime()", asMETHOD(CKTimeManager, GetTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetLastDeltaTime()", asMETHOD(CKTimeManager, GetLastDeltaTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetLastDeltaTimeFree()", asMETHOD(CKTimeManager, GetLastDeltaTimeFree), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetAbsoluteTime()", asMETHOD(CKTimeManager, GetAbsoluteTime), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKTimeManager", "void SetTimeScaleFactor(float)", asMETHOD(CKTimeManager, SetTimeScaleFactor), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetTimeScaleFactor()", asMETHOD(CKTimeManager, GetTimeScaleFactor), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKTimeManager", "uint GetLimitOptions()", asMETHOD(CKTimeManager, GetLimitOptions), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetFrameRateLimit()", asMETHOD(CKTimeManager, GetFrameRateLimit), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetBehavioralRateLimit()", asMETHOD(CKTimeManager, GetBehavioralRateLimit), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetMinimumDeltaTime()", asMETHOD(CKTimeManager, GetMinimumDeltaTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "float GetMaximumDeltaTime()", asMETHOD(CKTimeManager, GetMaximumDeltaTime), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKTimeManager", "void ChangeLimitOptions(CK_FRAMERATE_LIMITS, CK_FRAMERATE_LIMITS = CK_RATE_NOP)", asMETHOD(CKTimeManager, ChangeLimitOptions), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void SetFrameRateLimit(float)", asMETHOD(CKTimeManager, SetFrameRateLimit), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void SetBehavioralRateLimit(float)", asMETHOD(CKTimeManager, SetBehavioralRateLimit), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void SetMinimumDeltaTime(float)", asMETHOD(CKTimeManager, SetMinimumDeltaTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void SetMaximumDeltaTime(float)", asMETHOD(CKTimeManager, SetMaximumDeltaTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void SetLastDeltaTime(float)", asMETHOD(CKTimeManager, SetLastDeltaTime), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKTimeManager", "void GetTimeToWaitForLimits(float &out, float &out)", asMETHOD(CKTimeManager, GetTimeToWaitForLimits), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKTimeManager", "void ResetChronos(bool, bool)", asMETHOD(CKTimeManager, ResetChronos), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKMessageManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKMessageManager>(engine, "CKMessageManager");

    r = engine->RegisterObjectMethod("CKMessageManager", "CKMessageType AddMessageType(const string &in msgName)", asMETHODPR(CKMessageManager, AddMessageType, (CKSTRING), CKMessageType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "string GetMessageTypeName(CKMessageType msgType)", asMETHODPR(CKMessageManager, GetMessageTypeName, (CKMessageType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "int GetMessageTypeCount()", asMETHODPR(CKMessageManager, GetMessageTypeCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "void RenameMessageType(CKMessageType msgType, const string &in newName)", asMETHODPR(CKMessageManager, RenameMessageType, (CKMessageType, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "void RenameMessageType(const string &in oldName, const string &in newName)", asMETHODPR(CKMessageManager, RenameMessageType, (CKSTRING, CKSTRING), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR SendMessage(CKMessage@ msg)", asMETHODPR(CKMessageManager, SendMessage, (CKMessage*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKMessage@ SendMessageSingle(int MsgType, CKBeObject@ dest, CKBeObject@ sender = null)", asMETHODPR(CKMessageManager, SendMessageSingle, (CKMessageType, CKBeObject*, CKBeObject*), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKMessage@ SendMessageGroup(int MsgType, CKGroup@ group, CKBeObject@ sender = null)", asMETHODPR(CKMessageManager, SendMessageGroup, (CKMessageType, CKGroup*, CKBeObject*), CKMessage*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKMessage@ SendMessageBroadcast(int MsgType, int id = 0, CKBeObject@ sender = null)", asMETHODPR(CKMessageManager, SendMessageBroadcast, (CKMessageType, CK_CLASSID, CKBeObject*), CKMessage*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR RegisterWait(CKMessageType msgType, CKBehavior@ behav, int outputToActivate, CKBeObject@ obj)", asMETHODPR(CKMessageManager, RegisterWait, (CKMessageType, CKBehavior*, int, CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR RegisterWait(const string &in msgName, CKBehavior@ behav, int OutputToActivate, CKBeObject@ obj)", asMETHODPR(CKMessageManager, RegisterWait, (CKSTRING, CKBehavior*, int, CKBeObject*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR UnRegisterWait(CKMessageType msgType, CKBehavior@ behav, int OutputToActivate)", asMETHODPR(CKMessageManager, UnRegisterWait, (CKMessageType, CKBehavior*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR UnRegisterWait(const string &in msgName, CKBehavior@ behav, int OutputToActivate)", asMETHODPR(CKMessageManager, UnRegisterWait, (CKSTRING, CKBehavior*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMessageManager", "CKERROR RegisterDefaultMessages()", asMETHODPR(CKMessageManager, RegisterDefaultMessages, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKBehaviorManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKBehaviorManager>(engine, "CKBehaviorManager");

    r = engine->RegisterObjectMethod("CKBehaviorManager", "CKERROR Execute(float delta)", asMETHODPR(CKBehaviorManager, Execute, (float), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorManager", "int GetObjectsCount()", asMETHODPR(CKBehaviorManager, GetObjectsCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorManager", "CKBeObject@ GetObject(int pos)", asMETHODPR(CKBehaviorManager, GetObject, (int), CKBeObject*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKBehaviorManager", "int GetBehaviorMaxIteration()", asMETHODPR(CKBehaviorManager, GetBehaviorMaxIteration, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKBehaviorManager", "void SetBehaviorMaxIteration(int n)", asMETHODPR(CKBehaviorManager, SetBehaviorMaxIteration, (int), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKPathManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKPathManager>(engine, "CKPathManager");

    r = engine->RegisterObjectMethod("CKPathManager", "int AddCategory(XString &in cat)", asMETHODPR(CKPathManager, AddCategory, (XString&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR RemoveCategory(int catIdx)", asMETHODPR(CKPathManager, RemoveCategory, (int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "int GetCategoryCount()", asMETHODPR(CKPathManager, GetCategoryCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR GetCategoryName(int catIdx, XString &out catName)", asMETHODPR(CKPathManager, GetCategoryName, (int, XString&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "int GetCategoryIndex(XString &in cat)", asMETHODPR(CKPathManager, GetCategoryIndex, (XString&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR RenameCategory(int catIdx, XString &in newName)", asMETHODPR(CKPathManager, RenameCategory, (int, XString&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPathManager", "int AddPath(int catIdx, XString &in path)", asMETHODPR(CKPathManager, AddPath, (int, XString&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR RemovePath(int catIdx, int pathIdx)", asMETHODPR(CKPathManager, RemovePath, (int, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR SwapPaths(int catIdx, int pathIdx1, int pathIdx2)", asMETHODPR(CKPathManager, SwapPaths, (int, int, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "int GetPathCount(int catIdx)", asMETHODPR(CKPathManager, GetPathCount, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR GetPathName(int catIdx, int pathIdx, XString &out path)", asMETHODPR(CKPathManager, GetPathName, (int, int, XString&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "int GetPathIndex(int catIdx, XString &in path)", asMETHODPR(CKPathManager, GetPathIndex, (int, XString&), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR RenamePath(int catIdx, int pathIdx, XString &in path)", asMETHODPR(CKPathManager, RenamePath, (int, int, XString&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // TODO: &inout is not supported here
    r = engine->RegisterObjectMethod("CKPathManager", "CKERROR ResolveFileName(string &inout file, int catIdx, int startIdx = -1)", asMETHODPR(CKPathManager, ResolveFileName, (XString&, int, int), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPathManager", "bool PathIsAbsolute(XString &in file)", asMETHODPR(CKPathManager, PathIsAbsolute, (XString&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "bool PathIsUNC(XString &in file)", asMETHODPR(CKPathManager, PathIsUNC, (XString&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "bool PathIsURL(XString &in file)", asMETHODPR(CKPathManager, PathIsURL, (XString&), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "bool PathIsFile(XString &in file)", asMETHODPR(CKPathManager, PathIsFile, (XString&), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // TODO: &inout is not supported here
    r = engine->RegisterObjectMethod("CKPathManager", "void RemoveEscapedSpace(string &inout str)", asMETHODPR(CKPathManager, RemoveEscapedSpace, (char*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKPathManager", "void AddEscapedSpace(XString &inout str)", asMETHODPR(CKPathManager, AddEscapedSpace, (XString&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKPathManager", "XString GetVirtoolsTemporaryFolder()", asMETHODPR(CKPathManager, GetVirtoolsTemporaryFolder, (), XString), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKRenderManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKRenderManager>(engine, "CKRenderManager");

    r = engine->RegisterObjectMethod("CKRenderManager", "int GetRenderDriverCount()", asMETHODPR(CKRenderManager, GetRenderDriverCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "VxDriverDesc &GetRenderDriverDescription(int driver)", asMETHODPR(CKRenderManager, GetRenderDriverDescription, (int), VxDriverDesc*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "void GetDesiredTexturesVideoFormat(VxImageDescEx &out format)", asMETHODPR(CKRenderManager, GetDesiredTexturesVideoFormat, (VxImageDescEx&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "void SetDesiredTexturesVideoFormat(VxImageDescEx &in format)", asMETHODPR(CKRenderManager, SetDesiredTexturesVideoFormat, (VxImageDescEx&), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "CKRenderContext@ GetRenderContext(int pos)", asMETHODPR(CKRenderManager, GetRenderContext, (int), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "CKRenderContext@ GetRenderContextFromPoint(CKPOINT &in pt)", asMETHODPR(CKRenderManager, GetRenderContextFromPoint, (CKPOINT&), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "int GetRenderContextCount()", asMETHODPR(CKRenderManager, GetRenderContextCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "void Process()", asMETHODPR(CKRenderManager, Process, (), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "void FlushTextures()", asMETHODPR(CKRenderManager, FlushTextures, (), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "CKRenderContext@ CreateRenderContext(WIN_HANDLE window, int driver = 0, CKRECT &in rect = void, bool fullscreen = false, int bpp = -1, int zbpp = -1, int stencilBpp = -1, int refreshRate = 0)", asMETHODPR(CKRenderManager, CreateRenderContext, (void*, int, CKRECT*, CKBOOL, int, int, int, int), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "CKERROR DestroyRenderContext(CKRenderContext@ context)", asMETHODPR(CKRenderManager, DestroyRenderContext, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "void RemoveRenderContext(CKRenderContext@ context)", asMETHODPR(CKRenderManager, RemoveRenderContext, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "CKVertexBuffer@ CreateVertexBuffer()", asMETHODPR(CKRenderManager, CreateVertexBuffer, (), CKVertexBuffer*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "void DestroyVertexBuffer(CKVertexBuffer@ VB)", asMETHODPR(CKRenderManager, DestroyVertexBuffer, (CKVertexBuffer*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "void SetRenderOptions(const string &in option, uint value)", asMETHODPR(CKRenderManager, SetRenderOptions, (CKSTRING, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKRenderManager", "const VxEffectDescription &GetEffectDescription(int effectIndex)", asMETHODPR(CKRenderManager, GetEffectDescription, (int), const VxEffectDescription&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "int GetEffectCount()", asMETHODPR(CKRenderManager, GetEffectCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKRenderManager", "int AddEffect(const VxEffectDescription &in effect)", asMETHODPR(CKRenderManager, AddEffect, (const VxEffectDescription&), int), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKFloorManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKFloorManager>(engine, "CKFloorManager");

    r = engine->RegisterObjectMethod("CKFloorManager", "CK_FLOORNEAREST GetNearestFloors(const VxVector &in position, CKFloorPoint &out floorPoint, CK3dEntity@ excludeFloor = null)", asMETHODPR(CKFloorManager, GetNearestFloors, (const VxVector&, CKFloorPoint*, CK3dEntity*), CK_FLOORNEAREST), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "CK_FLOORNEAREST GetNearestFloor(const VxVector &in position, CK3dEntity@ &out floor, int &out faceIndex, VxVector &out normal = void, float &out distance = void, CK3dEntity@ excludeFloor = null)", asMETHODPR(CKFloorManager, GetNearestFloor, (const VxVector&, CK3dEntity**, int*, VxVector*, float*, CK3dEntity*), CK_FLOORNEAREST), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKFloorManager", "void SetLimitAngle(float angle)", asMETHODPR(CKFloorManager, SetLimitAngle, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "float GetLimitAngle()", asMETHODPR(CKFloorManager, GetLimitAngle, (), float), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKFloorManager", "int AddFloorObjectsByName(CKLevel@ level, const string &in floorName, CK_FLOORGEOMETRY geo, bool moving, int type, bool hiera, bool first)", asMETHODPR(CKFloorManager, AddFloorObjectsByName, (CKLevel*, CKSTRING, CK_FLOORGEOMETRY, CKBOOL, int, CKBOOL, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "void AddFloorObject(CK3dEntity@ ent, CK_FLOORGEOMETRY geo, bool moving, int type, bool hiera, bool first)", asMETHODPR(CKFloorManager, AddFloorObject, (CK3dEntity*, CK_FLOORGEOMETRY, CKBOOL, int, CKBOOL, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "void RemoveFloorObject(CK3dEntity@ ent)", asMETHODPR(CKFloorManager, RemoveFloorObject, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "int GetFloorObjectCount()", asMETHODPR(CKFloorManager, GetFloorObjectCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "CK3dEntity@ GetFloorObject(int pos)", asMETHODPR(CKFloorManager, GetFloorObject, (int), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKFloorManager", "float GetCacheThreshold()", asMETHODPR(CKFloorManager, GetCacheThreshold, (), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "void SetCacheThreshold(float t)", asMETHODPR(CKFloorManager, SetCacheThreshold, (float), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "int GetCacheSize()", asMETHODPR(CKFloorManager, GetCacheSize, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "void SetCacheSize(int size)", asMETHODPR(CKFloorManager, SetCacheSize, (int), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKFloorManager", "bool ReadAttributeValues(CK3dEntity@ ent, uint &out geo = void, bool &out moving = void, int &out type = void, bool &out hiera = void, bool &out first = void)", asMETHODPR(CKFloorManager, ReadAttributeValues, (CK3dEntity*, CKDWORD*, CKBOOL*, int*, CKBOOL*, CKBOOL*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKFloorManager", "int GetFloorAttribute()", asMETHODPR(CKFloorManager, GetFloorAttribute, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKFloorManager", "bool ConstrainToFloor(const VxVector &in oldPos, const VxVector &in position, float radius, VxVector &out oPosition, int excludeAttribute = -1)", asMETHODPR(CKFloorManager, ConstrainToFloor, (const VxVector&, const VxVector&, float, VxVector*, CKAttributeType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKGridManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKGridManager>(engine, "CKGridManager");

    r = engine->RegisterObjectMethod("CKGridManager", "int GetTypeFromName(const string &in name)", asMETHODPR(CKGridManager, GetTypeFromName, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "string GetTypeName(int type)", asMETHODPR(CKGridManager, GetTypeName, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKERROR SetTypeName(int type, const string &in name)", asMETHODPR(CKGridManager, SetTypeName, (int, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int RegisterType(const string &in name)", asMETHODPR(CKGridManager, RegisterType, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int UnRegisterType(const string &in name)", asMETHODPR(CKGridManager, UnRegisterType, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGridManager", "CKERROR SetAssociatedParam(int type, CKGUID param)", asMETHODPR(CKGridManager, SetAssociatedParam, (int, CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKGUID GetAssociatedParam(int type)", asMETHODPR(CKGridManager, GetAssociatedParam, (int), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKERROR SetAssociatedColor(int type, VxColor &in color)", asMETHODPR(CKGridManager, SetAssociatedColor, (int, VxColor*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKERROR GetAssociatedColor(int type, VxColor &out color)", asMETHODPR(CKGridManager, GetAssociatedColor, (int, VxColor*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int GetLayerTypeCount()", asMETHODPR(CKGridManager, GetLayerTypeCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGridManager", "int GetClassificationFromName(const string &in name)", asMETHODPR(CKGridManager, GetClassificationFromName, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "string GetClassificationName(int classification)", asMETHODPR(CKGridManager, GetClassificationName, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int RegisterClassification(const string &in name)", asMETHODPR(CKGridManager, RegisterClassification, (CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int GetGridClassificationCategory()", asMETHODPR(CKGridManager, GetGridClassificationCategory, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGridManager", "const XObjectPointerArray &GetGridArray(int flag = 0)", asMETHODPR(CKGridManager, GetGridArray, (int), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKGrid@ GetNearestGrid(VxVector &in pos, CK3dEntity@ ref = null)", asMETHODPR(CKGridManager, GetNearestGrid, (VxVector*, CK3dEntity*), CKGrid*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKGrid@ GetPreferredGrid(VxVector &in pos, CK3dEntity@ ref = null)", asMETHODPR(CKGridManager, GetPreferredGrid, (VxVector*, CK3dEntity*), CKGrid*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "bool IsInGrid(CKGrid@ grid, VxVector &in pos, CK3dEntity@ ref = null)", asMETHODPR(CKGridManager, IsInGrid, (CKGrid*, VxVector*, CK3dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "int GetGridObjectCount(int flag = 0)", asMETHODPR(CKGridManager, GetGridObjectCount, (int), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "CKGrid@ GetGridObject(int pos, int flag = 0)", asMETHODPR(CKGridManager, GetGridObject, (int, int), CKGrid*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGridManager", "void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, int &in fillVal)", asMETHODPR(CKGridManager, FillGridWithObjectShape, (CK3dEntity*, int, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKGridManager", "void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, int &in fillVal)", asMETHODPR(CKGridManager, FillGridWithObjectShape, (CK3dEntity*, int, int, void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKGridManager", "void Init()", asMETHODPR(CKGridManager, Init, (), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKInterfaceManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKInterfaceManager>(engine, "CKInterfaceManager");

    r = engine->RegisterObjectMethod("CKInterfaceManager", "NativePointer GetEditorFunctionForParameterType(CKParameterTypeDesc &in param)", asMETHODPR(CKInterfaceManager, GetEditorFunctionForParameterType, (CKParameterTypeDesc*), CK_PARAMETERUICREATORFUNCTION), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInterfaceManager", "int CallBehaviorEditionFunction(CKBehavior@ beh, NativePointer arg)", asMETHODPR(CKInterfaceManager, CallBehaviorEditionFunction, (CKBehavior*, void*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInterfaceManager", "int CallBehaviorSettingsEditionFunction(CKBehavior@ beh, NativePointer arg)", asMETHODPR(CKInterfaceManager, CallBehaviorSettingsEditionFunction, (CKBehavior*, void*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInterfaceManager", "int CallEditionFunction(CK_CLASSID cid, NativePointer arg)", asMETHODPR(CKInterfaceManager, CallEditionFunction, (CK_CLASSID, void*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInterfaceManager", "int DoRenameDialog(string &in name, CK_CLASSID cid)", asMETHODPR(CKInterfaceManager, DoRenameDialog, (char*, CK_CLASSID), int), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKSoundManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKSoundManager>(engine, "CKSoundManager");

    r = engine->RegisterObjectMethod("CKSoundManager", "CK_SOUNDMANAGER_CAPS GetCaps()", asMETHODPR(CKSoundManager, GetCaps, (), CK_SOUNDMANAGER_CAPS), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "NativePointer CreateSource(CK_WAVESOUND_TYPE flags, CKWaveFormat &in wf, uint bytes, bool streamed)", asMETHODPR(CKSoundManager, CreateSource, (CK_WAVESOUND_TYPE, CKWaveFormat*, CKDWORD, CKBOOL), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "NativePointer DuplicateSource(NativePointer source)", asMETHODPR(CKSoundManager, DuplicateSource, (void*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void ReleaseSource(NativePointer source)", asMETHODPR(CKSoundManager, ReleaseSource, (void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "void Play(CKWaveSound@ sound, NativePointer source, bool loop)", asMETHODPR(CKSoundManager, Play, (CKWaveSound*, void*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void Pause(CKWaveSound@ sound, NativePointer source)", asMETHODPR(CKSoundManager, Pause, (CKWaveSound*, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void SetPlayPosition(NativePointer source, int pos)", asMETHODPR(CKSoundManager, SetPlayPosition, (void*, int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "int GetPlayPosition(NativePointer source)", asMETHODPR(CKSoundManager, GetPlayPosition, (void*), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void Stop(CKWaveSound@ sound, NativePointer source)", asMETHODPR(CKSoundManager, Stop, (CKWaveSound*, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "bool IsPlaying(NativePointer source)", asMETHODPR(CKSoundManager, IsPlaying, (void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "int SetWaveFormat(NativePointer source, CKWaveFormat &in wf)", asMETHODPR(CKSoundManager, SetWaveFormat, (void*, CKWaveFormat&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "int GetWaveFormat(NativePointer source, CKWaveFormat &out wf)", asMETHODPR(CKSoundManager, GetWaveFormat, (void*, CKWaveFormat&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "int GetWaveSize(NativePointer source)", asMETHODPR(CKSoundManager, GetWaveSize, (void*), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "int Lock(NativePointer source, uint writeCursor, uint numBytes, NativePointer &out audioPtr1, uint &out audioBytes1, NativePointer &out audioPtr2, uint &out audioBytes2, CK_WAVESOUND_LOCKMODE flags)", asMETHODPR(CKSoundManager, Lock, (void*, CKDWORD, CKDWORD, void**, CKDWORD*, void**, CKDWORD*, CK_WAVESOUND_LOCKMODE), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "int Unlock(NativePointer source, NativePointer audioPtr1, uint numBytes1, NativePointer audioPtr2, uint audioBytes2)", asMETHODPR(CKSoundManager, Unlock, (void*, void*, CKDWORD, void*, CKDWORD), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "void SetType(NativePointer source, CK_WAVESOUND_TYPE type)", asMETHODPR(CKSoundManager, SetType, (void*, CK_WAVESOUND_TYPE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "CK_WAVESOUND_TYPE GetType(NativePointer source)", asMETHODPR(CKSoundManager, GetType, (void*), CK_WAVESOUND_TYPE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "void UpdateSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSoundSettings &out settings, bool set = true)", asMETHODPR(CKSoundManager, UpdateSettings, (void *, CK_SOUNDMANAGER_CAPS, CKWaveSoundSettings &, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void Update3DSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSound3DSettings &out settings, bool set = true)", asMETHODPR(CKSoundManager, Update3DSettings, (void *, CK_SOUNDMANAGER_CAPS, CKWaveSound3DSettings &, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "void UpdateListenerSettings(CK_SOUNDMANAGER_CAPS settingsoptions, CKListenerSettings &out settings, bool set = true)", asMETHODPR(CKSoundManager, UpdateListenerSettings, (CK_SOUNDMANAGER_CAPS, CKListenerSettings&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "void SetListener(CK3dEntity@ listener)", asMETHODPR(CKSoundManager, SetListener, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "CK3dEntity@ GetListener()", asMETHODPR(CKSoundManager, GetListener, (), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "void SetStreamedBufferSize(uint size)", asMETHODPR(CKSoundManager, SetStreamedBufferSize, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKSoundManager", "uint GetStreamedBufferSize()", asMETHODPR(CKSoundManager, GetStreamedBufferSize, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKSoundManager", "bool IsInitialized()", asMETHODPR(CKSoundManager, IsInitialized, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKMidiManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKMidiManager>(engine, "CKMidiManager");

    r = engine->RegisterObjectMethod("CKMidiManager", "NativePointer Create(NativePointer hwnd)", asMETHODPR(CKMidiManager, Create, (void*), void*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "void Release(NativePointer source)", asMETHODPR(CKMidiManager, Release, (void*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR SetSoundFileName(NativePointer source, const string &in filename)", asMETHODPR(CKMidiManager, SetSoundFileName, (void*, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "string GetSoundFileName(NativePointer source)", asMETHODPR(CKMidiManager, GetSoundFileName, (void*), CKSTRING), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Play(NativePointer source)", asMETHODPR(CKMidiManager, Play, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Restart(NativePointer source)", asMETHODPR(CKMidiManager, Restart, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Stop(NativePointer source)", asMETHODPR(CKMidiManager, Stop, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Pause(NativePointer source, bool pause = true)", asMETHODPR(CKMidiManager, Pause, (void*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "bool IsPlaying(NativePointer source)", asMETHODPR(CKMidiManager, IsPlaying, (void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "bool IsPaused(NativePointer source)", asMETHODPR(CKMidiManager, IsPaused, (void*), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR OpenFile(NativePointer source)", asMETHODPR(CKMidiManager, OpenFile, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR CloseFile(NativePointer source)", asMETHODPR(CKMidiManager, CloseFile, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Preroll(NativePointer source)", asMETHODPR(CKMidiManager, Preroll, (void*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "CKERROR Time(NativePointer source, uint &out ticks)", asMETHODPR(CKMidiManager, Time, (void*, CKDWORD*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "uint MillisecsToTicks(NativePointer source, uint msOffset)", asMETHODPR(CKMidiManager, MillisecsToTicks, (void*, CKDWORD), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKMidiManager", "uint TicksToMillisecs(NativePointer source, uint tkOffset)", asMETHODPR(CKMidiManager, TicksToMillisecs, (void*, CKDWORD), CKDWORD), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKInputManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKInputManager>(engine, "CKInputManager");

    r = engine->RegisterObjectMethod("CKInputManager", "void EnableKeyboardRepetition(bool enable = true)", asMETHODPR(CKInputManager, EnableKeyboardRepetition, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsKeyboardRepetitionEnabled()", asMETHODPR(CKInputManager, IsKeyboardRepetitionEnabled, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsKeyDown(uint key, uint &out stamp = 0)", asMETHODPR(CKInputManager, IsKeyDown, (CKDWORD, CKDWORD*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsKeyUp(uint key)", asMETHODPR(CKInputManager, IsKeyUp, (CKDWORD), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsKeyToggled(uint key, uint &out stamp = 0)", asMETHODPR(CKInputManager, IsKeyToggled, (CKDWORD, CKDWORD*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "int GetKeyName(uint key, string &out keyName)", asMETHODPR(CKInputManager, GetKeyName, (CKDWORD, CKSTRING), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "uint GetKeyFromName(const string &in keyName)", asMETHODPR(CKInputManager, GetKeyFromName, (CKSTRING), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "NativePointer GetKeyboardState()", asMETHODPR(CKInputManager, GetKeyboardState, (), unsigned char*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsKeyboardAttached()", asMETHODPR(CKInputManager, IsKeyboardAttached, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKInputManager", "int GetNumberOfKeyInBuffer()", asMETHODPR(CKInputManager, GetNumberOfKeyInBuffer, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "int GetKeyFromBuffer(int index, uint &out key, uint &out timeStamp = void)", asMETHODPR(CKInputManager, GetKeyFromBuffer, (int, CKDWORD&, CKDWORD*), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKInputManager", "bool IsMouseButtonDown(CK_MOUSEBUTTON button)", asMETHODPR(CKInputManager, IsMouseButtonDown, (CK_MOUSEBUTTON), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsMouseClicked(CK_MOUSEBUTTON button)", asMETHODPR(CKInputManager, IsMouseClicked, (CK_MOUSEBUTTON), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsMouseToggled(CK_MOUSEBUTTON button)", asMETHODPR(CKInputManager, IsMouseToggled, (CK_MOUSEBUTTON), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetMouseButtonsState(uint &out states)", asMETHODPR(CKInputManager, GetMouseButtonsState, (CKBYTE[4]), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetMousePosition(Vx2DVector &out position, bool absolute = true)", asMETHODPR(CKInputManager, GetMousePosition, (Vx2DVector&, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetMouseRelativePosition(VxVector &out position)", asMETHODPR(CKInputManager, GetMouseRelativePosition, (VxVector&), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsMouseAttached()", asMETHODPR(CKInputManager, IsMouseAttached, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKInputManager", "bool IsJoystickAttached(int joystick)", asMETHODPR(CKInputManager, IsJoystickAttached, (int), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetJoystickPosition(int joystick, VxVector &out position)", asMETHODPR(CKInputManager, GetJoystickPosition, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetJoystickRotation(int joystick, VxVector &out rotation)", asMETHODPR(CKInputManager, GetJoystickRotation, (int, VxVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetJoystickSliders(int joystick, Vx2DVector &out position)", asMETHODPR(CKInputManager, GetJoystickSliders, (int, Vx2DVector*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void GetJoystickPointOfViewAngle(int joystick, float &out angle)", asMETHODPR(CKInputManager, GetJoystickPointOfViewAngle, (int, float*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "uint GetJoystickButtonsState(int joystick)", asMETHODPR(CKInputManager, GetJoystickButtonsState, (int), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool IsJoystickButtonDown(int joystick, int button)", asMETHODPR(CKInputManager, IsJoystickButtonDown, (int, int), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKInputManager", "void Pause(bool pause)", asMETHODPR(CKInputManager, Pause, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKInputManager", "void ShowCursor(bool show)", asMETHODPR(CKInputManager, ShowCursor, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "bool GetCursorVisibility()", asMETHODPR(CKInputManager, GetCursorVisibility, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "VXCURSOR_POINTER GetSystemCursor()", asMETHODPR(CKInputManager, GetSystemCursor, (), VXCURSOR_POINTER), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKInputManager", "void SetSystemCursor(VXCURSOR_POINTER cursor)", asMETHODPR(CKInputManager, SetSystemCursor, (VXCURSOR_POINTER), void), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKCollisionManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKCollisionManager>(engine, "CKCollisionManager");

    r = engine->RegisterObjectMethod("CKCollisionManager", "void AddObstacle(CK3dEntity@ ent, bool moving = false, CK_GEOMETRICPRECISION precision = CKCOLLISION_BOX, bool hiera = false)", asMETHODPR(CKCollisionManager, AddObstacle, (CK3dEntity*, CKBOOL, CK_GEOMETRICPRECISION, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "int AddObstaclesByName(CKLevel@ level, const string &in name, bool moving = false, CK_GEOMETRICPRECISION precision = CKCOLLISION_BOX, bool hiera = false)", asMETHODPR(CKCollisionManager, AddObstaclesByName, (CKLevel*, CKSTRING, CKBOOL, CK_GEOMETRICPRECISION, CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "void RemoveObstacle(CK3dEntity@ ent)", asMETHODPR(CKCollisionManager, RemoveObstacle, (CK3dEntity*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "void RemoveAllObstacles(bool level = true)", asMETHODPR(CKCollisionManager, RemoveAllObstacles, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool IsObstacle(CK3dEntity@ ent, bool moving = false)", asMETHODPR(CKCollisionManager, IsObstacle, (CK3dEntity*, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCollisionManager", "int GetFixedObstacleCount(bool level = false)", asMETHODPR(CKCollisionManager, GetFixedObstacleCount, (CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "CK3dEntity@ GetFixedObstacle(int pos, bool level = false)", asMETHODPR(CKCollisionManager, GetFixedObstacle, (int, CKBOOL), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "int GetMovingObstacleCount(bool level = false)", asMETHODPR(CKCollisionManager, GetMovingObstacleCount, (CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "CK3dEntity@ GetMovingObstacle(int pos, bool level = false)", asMETHODPR(CKCollisionManager, GetMovingObstacle, (int, CKBOOL), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "int GetObstacleCount(bool level = false)", asMETHODPR(CKCollisionManager, GetObstacleCount, (CKBOOL), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "CK3dEntity@ GetObstacle(int pos, bool level = false)", asMETHODPR(CKCollisionManager, GetObstacle, (int, CKBOOL), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCollisionManager", "bool DetectCollision(CK3dEntity@ ent, CK_GEOMETRICPRECISION precisionLevel, int replacementPrecision, int detectionPrecision, CK_IMPACTINFO impactFlags, ImpactDesc &out imp)", asMETHODPR(CKCollisionManager, DetectCollision, (CK3dEntity*, CK_GEOMETRICPRECISION, int, int, CK_IMPACTINFO, ImpactDesc*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool ObstacleBetween(const VxVector &in pos, const VxVector &in endpos, float width, float height)", asMETHODPR(CKCollisionManager, ObstacleBetween, (const VxVector&, const VxVector&, float, float), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKCollisionManager", "bool BoxBoxIntersection(CK3dEntity@ ent1, bool hiera1, bool local1, CK3dEntity@ ent2, bool hiera2, bool local2)", asMETHODPR(CKCollisionManager, BoxBoxIntersection, (CK3dEntity*, CKBOOL, CKBOOL, CK3dEntity*, CKBOOL, CKBOOL), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool BoxFaceIntersection(CK3dEntity@ ent1, bool hiera1, bool local1, CK3dEntity@ ent2)", asMETHODPR(CKCollisionManager, BoxFaceIntersection, (CK3dEntity*, CKBOOL, CKBOOL, CK3dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool FaceFaceIntersection(CK3dEntity@ ent1, CK3dEntity@ ent2)", asMETHODPR(CKCollisionManager, FaceFaceIntersection, (CK3dEntity*, CK3dEntity*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool IsInCollision(CK3dEntity@ ent, CK_GEOMETRICPRECISION precisionLevel1, CK3dEntity@ ent2, CK_GEOMETRICPRECISION precisionLevel2)", asMETHODPR(CKCollisionManager, IsInCollision, (CK3dEntity*, CK_GEOMETRICPRECISION, CK3dEntity*, CK_GEOMETRICPRECISION), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "CK3dEntity@ IsInCollisionWithHierarchy(CK3dEntity@ ent, CK_GEOMETRICPRECISION precisionLevel1, CK3dEntity@ ent2, CK_GEOMETRICPRECISION precisionLevel2)", asMETHODPR(CKCollisionManager, IsInCollisionWithHierarchy, (CK3dEntity*, CK_GEOMETRICPRECISION, CK3dEntity*, CK_GEOMETRICPRECISION), CK3dEntity*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKCollisionManager", "bool IsHierarchyInCollisionWithHierarchy(CK3dEntity@ ent1, CK_GEOMETRICPRECISION precisionLevel1, CK3dEntity@ ent2, CK_GEOMETRICPRECISION precisionLevel2, CK3dEntity@ &out sub, CK3dEntity@ &out subob)", asMETHODPR(CKCollisionManager, IsHierarchyInCollisionWithHierarchy, (CK3dEntity*, CK_GEOMETRICPRECISION, CK3dEntity*, CK_GEOMETRICPRECISION, CK3dEntity**, CK3dEntity**), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}