#include "ScriptCKContext.h"

#include <cassert>

#include "CKContext.h"

void RegisterCKContext(asIScriptEngine *engine) {
    int r = 0;

    // Objects Management
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ CreateObject(CK_CLASSID cid, const string &in name = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK, CK_LOADMODE &out res = void)", asMETHODPR(CKContext, CreateObject, (CK_CLASSID, CKSTRING, CK_OBJECTCREATION_OPTIONS, CK_CREATIONMODE*), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ CopyObject(CKObject@ src, CKDependencies &in dependencies = void, const string &in appendName = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK)", asMETHODPR(CKContext, CopyObject, (CKObject*, CKDependencies*, CKSTRING, CK_OBJECTCREATION_OPTIONS), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "const XObjectArray &CopyObjects(const XObjectArray &in srcObjects, CKDependencies &in dependencies = void, CK_OBJECTCREATION_OPTIONS options = CK_OBJECTCREATION_NONAMECHECK, const string &in appendName = void)", asMETHODPR(CKContext, CopyObjects, (const XObjectArray&, CKDependencies*, CK_OBJECTCREATION_OPTIONS, CKSTRING), const XObjectArray&), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObject(CK_ID id)", asMETHODPR(CKContext, GetObject, (CK_ID), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetObjectCount()", asMETHODPR(CKContext, GetObjectCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetObjectSize(CKObject@ obj)", asMETHODPR(CKContext, GetObjectSize, (CKObject*), int), asCALL_THISCALL); assert(r >= 0);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKContext, DestroyObject, (CKObject*, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CK_ID id, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKContext, DestroyObject, (CK_ID, CKDWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
#else
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CKObject@ obj, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKContext, DestroyObject, (CKObject*, DWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObject(CK_ID id, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asMETHODPR(CKContext, DestroyObject, (CK_ID, DWORD, CKDependencies*), CKERROR), asCALL_THISCALL); assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DestroyObjects(const XObjectArray &in ids, CKDWORD flags = 0, CKDependencies &in depoptions = void)", asFUNCTIONPR([](CKContext *ctx, const XObjectArray &objects, CKDWORD flags, CKDependencies *deps) { return ctx->DestroyObjects(objects.Begin(), objects.Size(), flags, deps); }, (CKContext *, const XObjectArray &, CKDWORD, CKDependencies *), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
#if CKVERSION == 0x13022002 
    r = engine->RegisterObjectMethod("CKContext", "void DestroyAllDynamicObjects()", asMETHODPR(CKContext, DestroyAllDynamicObjects, (), void), asCALL_THISCALL); assert(r >= 0);
#else
    r = engine->RegisterObjectMethod("CKContext", "void DestroyAllDynamicObjects(CKScene@ scene = null)", asMETHODPR(CKContext, DestroyAllDynamicObjects, (CKScene *), void), asCALL_THISCALL); assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("CKContext", "void ChangeObjectDynamic(CKObject@ obj, bool setDynamic = true)", asMETHODPR(CKContext, ChangeObjectDynamic, (CKObject*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "const XObjectPointerArray &CKFillObjectsUnused()", asMETHODPR(CKContext, CKFillObjectsUnused, (), const XObjectPointerArray &), asCALL_THISCALL); assert(r >= 0);

    // Object Access
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByName(const string &in name, CKObject@ previous = null)", asMETHODPR(CKContext, GetObjectByName, (CKSTRING, CKObject*), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByNameAndClass(const string &in name, CK_CLASSID cid, CKObject@ previous = null)", asMETHODPR(CKContext, GetObjectByNameAndClass, (CKSTRING, CK_CLASSID, CKObject*), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKObject@ GetObjectByNameAndParentClass(const string &in name, CK_CLASSID pcid, CKObject@ previous)", asMETHODPR(CKContext, GetObjectByNameAndParentClass, (CKSTRING, CK_CLASSID, CKObject*), CKObject*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "const XObjectPointerArray &GetObjectListByType(CK_CLASSID cid, bool derived)", asMETHODPR(CKContext, GetObjectListByType, (CK_CLASSID, CKBOOL), const XObjectPointerArray&), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetObjectsCountByClassID(CK_CLASSID cid)", asMETHODPR(CKContext, GetObjectsCountByClassID, (CK_CLASSID), int), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "NativePointer GetObjectsListByClassID(int)", asMETHODPR(CKContext, GetObjectsListByClassID, (CK_CLASSID), CK_ID*), asCALL_THISCALL); assert(r >= 0);

    // Engine runtime
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Play()", asMETHODPR(CKContext, Play, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Pause()", asMETHODPR(CKContext, Pause, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Reset()", asMETHODPR(CKContext, Reset, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
#else
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Reset(bool toLevelScene = false)", asMETHODPR(CKContext, Reset, (CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("CKContext", "bool IsPlaying()", asMETHODPR(CKContext, IsPlaying, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsReseted()", asMETHODPR(CKContext, IsReseted, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Process()", asMETHODPR(CKContext, Process, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ClearAll()", asMETHODPR(CKContext, ClearAll, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInClearAll()", asMETHODPR(CKContext, IsInClearAll, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Current Level & Scene functions
    r = engine->RegisterObjectMethod("CKContext", "CKLevel@ GetCurrentLevel()", asMETHODPR(CKContext, GetCurrentLevel, (), CKLevel*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKRenderContext@ GetPlayerRenderContext()", asMETHODPR(CKContext, GetPlayerRenderContext, (), CKRenderContext*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKScene@ GetCurrentScene()", asMETHODPR(CKContext, GetCurrentScene, (), CKScene*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetCurrentLevel(CKLevel@ level)", asMETHODPR(CKContext, SetCurrentLevel, (CKLevel*), void), asCALL_THISCALL); assert(r >= 0);

    // Object Management functions
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, CKParameterType type, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterIn, (CKSTRING, CKParameterType, CKBOOL), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, CKGUID guid, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterIn, (CKSTRING, CKGUID, CKBOOL), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterIn@ CreateCKParameterIn(const string &in name, const string &in typeName, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterIn, (CKSTRING, CKSTRING, CKBOOL), CKParameterIn*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, CKParameterType type, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterOut, (CKSTRING, CKParameterType, CKBOOL), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, CKGUID guid, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterOut, (CKSTRING, CKGUID, CKBOOL), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOut@ CreateCKParameterOut(const string &in name, const string &in typeName, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterOut, (CKSTRING, CKSTRING, CKBOOL), CKParameterOut*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, CKParameterType type, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterLocal, (CKSTRING, CKParameterType, CKBOOL), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, CKGUID guid, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterLocal, (CKSTRING, CKGUID, CKBOOL), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterLocal@ CreateCKParameterLocal(const string &in name, const string &in typeName, bool dynamic = false)", asMETHODPR(CKContext, CreateCKParameterLocal, (CKSTRING, CKSTRING, CKBOOL), CKParameterLocal*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterOperation@ CreateCKParameterOperation(const string &in name, CKGUID opguid, CKGUID resGuid, CKGUID p1Guid, CKGUID p2Guid)", asMETHODPR(CKContext, CreateCKParameterOperation, (CKSTRING, CKGUID, CKGUID, CKGUID, CKGUID), CKParameterOperation*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKFile@ CreateCKFile()", asMETHODPR(CKContext, CreateCKFile, (), CKFile*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR DeleteCKFile(CKFile@ file)", asMETHODPR(CKContext, DeleteCKFile, (CKFile*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // IHM
    // r = engine->RegisterObjectMethod("CKContext", "void SetInterfaceMode(bool mode = true, CKUICALLBACKFCT = null, void@ = null)", asMETHODPR(CKContext, SetInterfaceMode, (CKBOOL, CKUICALLBACKFCT, void*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInInterfaceMode()", asMETHODPR(CKContext, IsInInterfaceMode, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR OutputToConsole(const string &in str, bool beep = true)", asMETHODPR(CKContext, OutputToConsole, (CKSTRING, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR OutputToInfo(const string &in format)", asFUNCTIONPR([](CKContext *ctx, const char *info) { return ctx->OutputToInfo(const_cast<CKSTRING>("%s"), info); }, (CKContext *, const char *), CKERROR), asCALL_CDECL_OBJFIRST); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR RefreshBuildingBlocks(const XGUIDArray &in guids)", asMETHODPR(CKContext, RefreshBuildingBlocks, (const XArray<CKGUID>&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR ShowSetup(int)", asMETHODPR(CKContext, ShowSetup, (CK_ID), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "int ChooseObject(NativePointer dialogParentWnd)", asMETHODPR(CKContext, ChooseObject, (void*), CK_ID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Select(const XObjectArray &in objects, bool clearSelection = true)", asMETHODPR(CKContext, Select, (const XObjectArray&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKDWORD SendInterfaceMessage(CKDWORD reason, CKDWORD param1, CKDWORD param2)", asMETHODPR(CKContext, SendInterfaceMessage, (CKDWORD, CKDWORD, CKDWORD), CKDWORD), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR UICopyObjects(const XObjectArray &in, bool iClearClipboard = true)", asMETHODPR(CKContext, UICopyObjects, (const XObjectArray&, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR UIPasteObjects(const XObjectArray &in)", asMETHODPR(CKContext, UIPasteObjects, (const XObjectArray&), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // Managers
    r = engine->RegisterObjectMethod("CKContext", "CKRenderManager@ GetRenderManager()", asMETHODPR(CKContext, GetRenderManager, (), CKRenderManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBehaviorManager@ GetBehaviorManager()", asMETHODPR(CKContext, GetBehaviorManager, (), CKBehaviorManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKParameterManager@ GetParameterManager()", asMETHODPR(CKContext, GetParameterManager, (), CKParameterManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKMessageManager@ GetMessageManager()", asMETHODPR(CKContext, GetMessageManager, (), CKMessageManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKTimeManager@ GetTimeManager()", asMETHODPR(CKContext, GetTimeManager, (), CKTimeManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKAttributeManager@ GetAttributeManager()", asMETHODPR(CKContext, GetAttributeManager, (), CKAttributeManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKPathManager@ GetPathManager()", asMETHODPR(CKContext, GetPathManager, (), CKPathManager*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "XManagerHashTableIt GetManagers()", asMETHODPR(CKContext, GetManagers, (), XManagerHashTableIt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManagerByGuid(const CKGUID &in guid)", asMETHODPR(CKContext, GetManagerByGuid, (CKGUID), CKBaseManager*), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManagerByName(const string &in name)", asMETHODPR(CKContext, GetManagerByName, (CKSTRING), CKBaseManager*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "int GetManagerCount()", asMETHODPR(CKContext, GetManagerCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetManager(int index)", asMETHODPR(CKContext, GetManager, (int), CKBaseManager*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "bool IsManagerActive(CKBaseManager@ manager)", asMETHODPR(CKContext, IsManagerActive, (CKBaseManager*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool HasManagerDuplicates(CKBaseManager@ manager)", asMETHODPR(CKContext, HasManagerDuplicates, (CKBaseManager*), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void ActivateManager(CKBaseManager@ manager, bool active)", asMETHODPR(CKContext, ActivateManager, (CKBaseManager*, CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetInactiveManagerCount()", asMETHODPR(CKContext, GetInactiveManagerCount, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBaseManager@ GetInactiveManager(int index)", asMETHODPR(CKContext, GetInactiveManager, (int), CKBaseManager*), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "CKERROR RegisterNewManager(CKBaseManager@ manager)", asMETHODPR(CKContext, RegisterNewManager, (CKBaseManager*), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // Profiling Functions
    r = engine->RegisterObjectMethod("CKContext", "void EnableProfiling(bool enable)", asMETHODPR(CKContext, EnableProfiling, (CKBOOL), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsProfilingEnable()", asMETHODPR(CKContext, IsProfilingEnable, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void GetProfileStats(CKStats &out stats)", asMETHODPR(CKContext, GetProfileStats, (CKStats*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void UserProfileStart(CKDWORD slot)", asMETHODPR(CKContext, UserProfileStart, (CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "float UserProfileEnd(CKDWORD slot)", asMETHODPR(CKContext, UserProfileEnd, (CKDWORD), float), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "float GetLastUserProfileTime(CKDWORD slot)", asMETHODPR(CKContext, GetLastUserProfileTime, (CKDWORD), float), asCALL_THISCALL); assert(r >= 0);

    // Utilities
#if CKVERSION == 0x13022002
    r = engine->RegisterObjectMethod("CKContext", "string GetStringBuffer(int size)", asMETHODPR(CKContext, GetStringBuffer, (int), CKSTRING), asCALL_THISCALL); assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("CKContext", "CKGUID GetSecureGuid()", asMETHODPR(CKContext, GetSecureGuid, (), CKGUID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKDWORD GetStartOptions()", asMETHODPR(CKContext, GetStartOptions, (), CKDWORD), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "WIN_HANDLE GetMainWindow()", asMETHODPR(CKContext, GetMainWindow, (), WIN_HANDLE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetSelectedRenderEngine()", asMETHODPR(CKContext, GetSelectedRenderEngine, (), int), asCALL_THISCALL); assert(r >= 0);

    // File Save/Load Options
    r = engine->RegisterObjectMethod("CKContext", "void SetCompressionLevel(int level)", asMETHODPR(CKContext, SetCompressionLevel, (int), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetCompressionLevel()", asMETHODPR(CKContext, GetCompressionLevel, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetFileWriteMode(CK_FILE_WRITEMODE mode)", asMETHODPR(CKContext, SetFileWriteMode, (CK_FILE_WRITEMODE), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CK_FILE_WRITEMODE GetFileWriteMode()", asMETHODPR(CKContext, GetFileWriteMode, (), CK_FILE_WRITEMODE), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CK_TEXTURE_SAVEOPTIONS GetGlobalImagesSaveOptions()", asMETHODPR(CKContext, GetGlobalImagesSaveOptions, (), CK_TEXTURE_SAVEOPTIONS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalImagesSaveOptions(CK_TEXTURE_SAVEOPTIONS options)", asMETHODPR(CKContext, SetGlobalImagesSaveOptions, (CK_TEXTURE_SAVEOPTIONS), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKBitmapProperties@ GetGlobalImagesSaveFormat()", asMETHODPR(CKContext, GetGlobalImagesSaveFormat, (), CKBitmapProperties *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalImagesSaveFormat(CKBitmapProperties@ format)", asMETHODPR(CKContext, SetGlobalImagesSaveFormat, (CKBitmapProperties *), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CK_SOUND_SAVEOPTIONS GetGlobalSoundsSaveOptions()", asMETHODPR(CKContext, GetGlobalSoundsSaveOptions, (), CK_SOUND_SAVEOPTIONS), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetGlobalSoundsSaveOptions(CK_SOUND_SAVEOPTIONS options)", asMETHODPR(CKContext, SetGlobalSoundsSaveOptions, (CK_SOUND_SAVEOPTIONS), void), asCALL_THISCALL); assert(r >= 0);

    // Save/Load functions
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Load(const string &in fileName, CKObjectArray@ list, CK_LOAD_FLAGS loadFlags = CK_LOAD_DEFAULT, CKGUID &in readerGuid = void)", asMETHODPR(CKContext, Load, (CKSTRING, CKObjectArray*, CK_LOAD_FLAGS, CKGUID*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "CKERROR Load(NativeBuffer, CKObjectArray@, CK_LOAD_FLAGS = CK_LOAD_DEFAULT)", asMETHODPR(CKContext, Load, (int, void*, CKObjectArray*, CK_LOAD_FLAGS), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "string GetLastFileLoaded()", asMETHODPR(CKContext, GetLastFileLoaded, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "string GetLastCmoLoaded()", asMETHODPR(CKContext, GetLastCmoLoaded, (), CKSTRING), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void SetLastCmoLoaded(const string &in str)", asMETHODPR(CKContext, SetLastCmoLoaded, (CKSTRING), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR GetFileInfo(const string &in fileName, CKFileInfo &out fileInfo)", asMETHODPR(CKContext, GetFileInfo, (CKSTRING, CKFileInfo*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "CKERROR GetFileInfo(NativeBuffer, CKFileInfo@)", asMETHODPR(CKContext, GetFileInfo, (int, void*, CKFileInfo*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR Save(const string &in fileName, CKObjectArray@ list, CKDWORD saveFlags, CKDependencies &in dependencies = void, CKGUID &in readerGuid = void)", asMETHODPR(CKContext, Save, (CKSTRING, CKObjectArray*, CKDWORD, CKDependencies*, CKGUID*), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR LoadAnimationOnCharacter(const string &in fileName, CKObjectArray@ list, CKCharacter@ carac, CKGUID &in readerGuid = void, bool asDynamicObjects = false)", asMETHODPR(CKContext, LoadAnimationOnCharacter, (CKSTRING, CKObjectArray*, CKCharacter*, CKGUID*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "CKERROR LoadAnimationOnCharacter(NativeBuffer, CKObjectArray@, CKCharacter@, bool = false)", asMETHODPR(CKContext, LoadAnimationOnCharacter, (int, void*, CKObjectArray*, CKCharacter*, CKBOOL), CKERROR), asCALL_THISCALL); assert(r >= 0);

    // r = engine->RegisterObjectMethod("CKContext", "void SetAutomaticLoadMode(CK_LOADMODE generalMode, CK_LOADMODE _3DObjectsMode, CK_LOADMODE meshMode, CK_LOADMODE matTexturesMode)", asMETHODPR(CKContext, SetAutomaticLoadMode, (CK_LOADMODE, CK_LOADMODE, CK_LOADMODE, CK_LOADMODE), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "void SetUserLoadCallback(NativePointer, NativePointer)", asMETHODPR(CKContext, SetUserLoadCallback, (CK_USERLOADCALLBACK, void*), void), asCALL_THISCALL); assert(r >= 0);
    // r = engine->RegisterObjectMethod("CKContext", "CK_LOADMODE LoadVerifyObjectUnicity(const string &in oldName, CK_CLASSID cid, const string &in newName, CKObject@ &out newObj)", asMETHODPR(CKContext, LoadVerifyObjectUnicity, (CKSTRING, CK_CLASSID, const CKSTRING, CKObject**), CK_LOADMODE), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "bool IsInLoad()", asMETHODPR(CKContext, IsInLoad, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInSave()", asMETHODPR(CKContext, IsInSave, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsRunTime()", asMETHODPR(CKContext, IsRunTime, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);

    // Render Engine Implementation Specific
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPreRender(CKRenderContext@ dev)", asMETHODPR(CKContext, ExecuteManagersOnPreRender, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPostRender(CKRenderContext@ dev)", asMETHODPR(CKContext, ExecuteManagersOnPostRender, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "void ExecuteManagersOnPostSpriteRender(CKRenderContext@ dev)", asMETHODPR(CKContext, ExecuteManagersOnPostSpriteRender, (CKRenderContext*), void), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "void AddProfileTime(CK_PROFILE_CATEGORY category, float time)", asMETHODPR(CKContext, AddProfileTime, (CK_PROFILE_CATEGORY, float), void), asCALL_THISCALL); assert(r >= 0);

    // Runtime Debug Mode
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ProcessDebugStart(float deltaTime = 20.0)", asMETHODPR(CKContext, ProcessDebugStart, (float), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool ProcessDebugStep()", asMETHODPR(CKContext, ProcessDebugStep, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKERROR ProcessDebugEnd()", asMETHODPR(CKContext, ProcessDebugEnd, (), CKERROR), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "CKDebugContext@ GetDebugContext()", asMETHODPR(CKContext, GetDebugContext, (), CKDebugContext *), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("CKContext", "void SetVirtoolsVersion(CK_VIRTOOLS_VERSION ver, CKDWORD build)", asMETHODPR(CKContext, SetVirtoolsVersion, (CK_VIRTOOLS_VERSION, CKDWORD), void), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "int GetPVInformation()", asMETHODPR(CKContext, GetPVInformation, (), int), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("CKContext", "bool IsInDynamicCreationMode()", asMETHODPR(CKContext, IsInDynamicCreationMode, (), CKBOOL), asCALL_THISCALL); assert(r >= 0);
}
