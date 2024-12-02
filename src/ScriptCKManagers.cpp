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

}

template <typename T>
static void RegisterCKBaseManagerMembers(asIScriptEngine *engine, const char *name) {
    int r = 0;

    r = engine->RegisterObjectMethod(name, "CKGUID GetGuid() const", asMETHODPR(T, GetGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "string GetName() const", asMETHODPR(T, GetName, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKStateChunk@ SaveData(CKFile@) const", asMETHODPR(T, SaveData, (CKFile*), CKStateChunk*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR LoadData(CKStateChunk@, CKFile@)", asMETHODPR(T, LoadData, (CKStateChunk*, CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreClearAll()", asMETHODPR(T, PreClearAll, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostClearAll()", asMETHODPR(T, PostClearAll, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreProcess()", asMETHODPR(T, PreProcess, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostProcess()", asMETHODPR(T, PostProcess, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // TODO: Use Array<CK_ID>
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceAddedToScene(CKScene@, CK_ID &in, int)", asMETHODPR(T, SequenceAddedToScene, (CKScene*, CK_ID*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceRemovedFromScene(CKScene@, CK_ID &in, int)", asMETHODPR(T, SequenceRemovedFromScene, (CKScene*, CK_ID*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreLaunchScene(CKScene@, CKScene@)", asMETHODPR(T, PreLaunchScene, (CKScene*, CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostLaunchScene(CKScene@, CKScene@)", asMETHODPR(T, PostLaunchScene, (CKScene*, CKScene*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKInit()", asMETHODPR(T, OnCKInit, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKEnd()", asMETHODPR(T, OnCKEnd, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKReset()", asMETHODPR(T, OnCKReset, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPostReset()", asMETHODPR(T, OnCKPostReset, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPause()", asMETHODPR(T, OnCKPause, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnCKPlay()", asMETHODPR(T, OnCKPlay, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // TODO: Use Array<CK_ID>
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceToBeDeleted(CK_ID &in, int)", asMETHODPR(T, SequenceToBeDeleted, (CK_ID*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR SequenceDeleted(CK_ID &in, int)", asMETHODPR(T, SequenceDeleted, (CK_ID*, int), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreLoad()", asMETHODPR(T, PreLoad, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostLoad()", asMETHODPR(T, PostLoad, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PreSave()", asMETHODPR(T, PreSave, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR PostSave()", asMETHODPR(T, PostSave, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPreCopy(CKDependenciesContext &in)", asMETHODPR(T, OnPreCopy, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostCopy(CKDependenciesContext &in)", asMETHODPR(T, OnPostCopy, (CKDependenciesContext&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPreRender(CKRenderContext@)", asMETHODPR(T, OnPreRender, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostRender(CKRenderContext@)", asMETHODPR(T, OnPostRender, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR OnPostSpriteRender(CKRenderContext@)", asMETHODPR(T, OnPostSpriteRender, (CKRenderContext*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "int GetFunctionPriority(CKMANAGER_FUNCTIONS)", asMETHODPR(T, GetFunctionPriority, (CKMANAGER_FUNCTIONS), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKDWORD GetValidFunctionsMask() const", asMETHODPR(T, GetValidFunctionsMask, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CKObject@, uint, CKDependencies &in)", asMETHODPR(T, CKDestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObject(CK_ID, uint, CKDependencies &in)", asMETHODPR(T, CKDestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // TODO: Use Array<CK_ID>
    r = engine->RegisterObjectMethod(name, "CKERROR CKDestroyObjects(CK_ID &in, int, uint, CKDependencies &in)", asMETHODPR(T, CKDestroyObjects, (CK_ID*, int, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(name, "CKObject@ CKGetObject(CK_ID)", asMETHODPR(T, CKGetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);
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

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterParameterType(CKParameterTypeDesc &in)", asMETHODPR(CKParameterManager, RegisterParameterType, (CKParameterTypeDesc*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterParameterType(const CKGUID &in)", asMETHODPR(CKParameterManager, UnRegisterParameterType, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterTypeDesc &GetParameterTypeDescription(int)", asMETHODPR(CKParameterManager, GetParameterTypeDescription, (int), CKParameterTypeDesc*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterTypeDesc &GetParameterTypeDescription(const CKGUID &in)", asMETHODPR(CKParameterManager, GetParameterTypeDescription, (CKGUID), CKParameterTypeDesc*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetParameterSize(CKParameterType)", asMETHODPR(CKParameterManager, GetParameterSize, (CKParameterType), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetParameterTypesCount()", asMETHODPR(CKParameterManager, GetParameterTypesCount, (), int), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ParameterGuidToType(const CKGUID &in)", asMETHODPR(CKParameterManager, ParameterGuidToType, (CKGUID), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string ParameterGuidToName(const CKGUID &in)", asMETHODPR(CKParameterManager, ParameterGuidToName, (CKGUID), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ParameterTypeToGuid(CKParameterType)", asMETHODPR(CKParameterManager, ParameterTypeToGuid, (CKParameterType), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "string ParameterTypeToName(CKParameterType)", asMETHODPR(CKParameterManager, ParameterTypeToName, (CKParameterType), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ParameterNameToGuid(const string &in)", asMETHODPR(CKParameterManager, ParameterNameToGuid, (CKSTRING), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ParameterNameToType(const string &in)", asMETHODPR(CKParameterManager, ParameterNameToType, (CKSTRING), CKParameterType), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsDerivedFrom(const CKGUID &in, const CKGUID &in)", asMETHODPR(CKParameterManager, IsDerivedFrom, (CKGUID, CKGUID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsDerivedFrom(CKParameterType, CKParameterType)", asMETHODPR(CKParameterManager, IsDerivedFrom, (CKParameterType, CKParameterType), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsTypeCompatible(const CKGUID &in, const CKGUID &in)", asMETHODPR(CKParameterManager, IsTypeCompatible, (CKGUID, CKGUID), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "bool IsTypeCompatible(CKParameterType, CKParameterType)", asMETHODPR(CKParameterManager, IsTypeCompatible, (CKParameterType, CKParameterType), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "int TypeToClassID(CKParameterType)", asMETHODPR(CKParameterManager, TypeToClassID, (CKParameterType), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GuidToClassID(const CKGUID &in)", asMETHODPR(CKParameterManager, GuidToClassID, (CKGUID), CK_CLASSID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKParameterType ClassIDToType(int)", asMETHODPR(CKParameterManager, ClassIDToType, (CK_CLASSID), CKParameterType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKGUID ClassIDToGuid(int)", asMETHODPR(CKParameterManager, ClassIDToGuid, (CK_CLASSID), CKGUID), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewFlags(const CKGUID &in, const string &in, const string &in)", asMETHODPR(CKParameterManager, RegisterNewFlags, (CKGUID, CKSTRING, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewEnum(const CKGUID &in, const string &in, const string &in)", asMETHODPR(CKParameterManager, RegisterNewEnum, (CKGUID, CKSTRING, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR ChangeEnumDeclaration(const CKGUID &in, const string &in)", asMETHODPR(CKParameterManager, ChangeEnumDeclaration, (CKGUID, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR ChangeFlagsDeclaration(const CKGUID &in, const string &in)", asMETHODPR(CKParameterManager, ChangeFlagsDeclaration, (CKGUID, CKSTRING), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewStructure(const CKGUID &in, const string &in, const string &in, ...)", asMETHODPR(CKParameterManager, RegisterNewStructure, (CKGUID, CKSTRING, CKSTRING, ...), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterNewStructure(const CKGUID &in, const string &in, const string &in, XArray<CKGUID> &in)", asMETHODPR(CKParameterManager, RegisterNewStructure, (CKGUID, CKSTRING, CKSTRING, XArray<CKGUID>&), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbFlagDefined()", asMETHODPR(CKParameterManager, GetNbFlagDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbEnumDefined()", asMETHODPR(CKParameterManager, GetNbEnumDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "int GetNbStructDefined()", asMETHODPR(CKParameterManager, GetNbStructDefined, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKFlagsStruct &GetFlagsDescByType(CKParameterType)", asMETHODPR(CKParameterManager, GetFlagsDescByType, (CKParameterType), CKFlagsStruct*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKEnumStruct &GetEnumDescByType(CKParameterType)", asMETHODPR(CKParameterManager, GetEnumDescByType, (CKParameterType), CKEnumStruct*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKStructStruct &GetStructDescByType(CKParameterType)", asMETHODPR(CKParameterManager, GetStructDescByType, (CKParameterType), CKStructStruct*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKOperationType RegisterOperationType(const CKGUID &in, const string &in)", asMETHODPR(CKParameterManager, RegisterOperationType, (CKGUID, CKSTRING), CKOperationType), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationType(const CKGUID &in)", asMETHODPR(CKParameterManager, UnRegisterOperationType, (CKGUID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationType(CKOperationType)", asMETHODPR(CKParameterManager, UnRegisterOperationType, (CKOperationType), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR RegisterOperationFunction(const CKGUID &in, const CKGUID &in, const CKGUID &in, const CKGUID &in, NativePointer)", asMETHODPR(CKParameterManager, RegisterOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &, CK_PARAMETEROPERATION), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "NativePointer GetOperationFunction(const CKGUID &in, const CKGUID &in, const CKGUID &in, const CKGUID &in)", asMETHODPR(CKParameterManager, GetOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &), CK_PARAMETEROPERATION), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKParameterManager", "CKERROR UnRegisterOperationFunction(const CKGUID &in, const CKGUID &in, const CKGUID &in, const CKGUID &in)", asMETHODPR(CKParameterManager, UnRegisterOperationFunction, (CKGUID &, CKGUID &, CKGUID &, CKGUID &), CKERROR), asCALL_THISCALL); assert(r >= 0);
}

void RegisterCKAttributeManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKAttributeManager>(engine, "CKAttributeManager");
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
}

void RegisterCKBehaviorManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKBehaviorManager>(engine, "CKBehaviorManager");
}

void RegisterCKPathManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKPathManager>(engine, "CKPathManager");
}

void RegisterCKRenderManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKRenderManager>(engine, "CKRenderManager");
}

void RegisterCKFloorManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKFloorManager>(engine, "CKFloorManager");
}

void RegisterCKGridManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKGridManager>(engine, "CKGridManager");
}

void RegisterCKInterfaceManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKInterfaceManager>(engine, "CKInterfaceManager");
}

void RegisterCKSoundManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKSoundManager>(engine, "CKSoundManager");
}

void RegisterCKMidiManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKMidiManager>(engine, "CKMidiManager");
}

void RegisterCKInputManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKInputManager>(engine, "CKInputManager");
}

void RegisterCKCollisionManager(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    RegisterCKBaseManagerMembers<CKCollisionManager>(engine, "CKCollisionManager");
}