#ifndef CK_SCRIPTMANAGER_H
#define CK_SCRIPTMANAGER_H

#include <angelscript.h>

#include "CKBaseManager.h"
#include "CKContext.h"

#include "ScriptCache.h"

#define SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

#define SCRIPT_MANAGER_TYPE 3000

class ScriptManager : public CKBaseManager {
public:
    enum Flag {
        AS_INITED = 0x00000001,
    };

    explicit ScriptManager(CKContext *context);

    ~ScriptManager() override;

    CKStateChunk *SaveData(CKFile *SavedFile) override;
    CKERROR LoadData(CKStateChunk *chunk, CKFile *LoadedFile) override;

    CKERROR PostClearAll() override;

    CKERROR OnCKInit() override;
    CKERROR OnCKEnd() override;

    CKERROR OnCKReset() override;
    CKERROR OnCKPause() override;

    CKERROR PostLoad() override;

    CKERROR OnPostCopy(CKDependenciesContext &context) override;

    CKDWORD GetValidFunctionsMask() override {
        return CKMANAGER_FUNC_PostClearAll |
               CKMANAGER_FUNC_OnCKInit |
               CKMANAGER_FUNC_OnCKEnd |
               CKMANAGER_FUNC_OnCKReset |
               CKMANAGER_FUNC_OnCKPause |
               CKMANAGER_FUNC_PostLoad |
               CKMANAGER_FUNC_OnPostCopy;
    }

    // Engine
    virtual asIScriptEngine *GetScriptEngine();
    virtual const char *GetVersion();
    virtual const char *GetOptions();

    // Context
    virtual asIScriptContext *GetActiveContext();

    // Thread support
    virtual int PrepareMultithread(asIThreadManager *externalMgr = nullptr);
    virtual void UnprepareMultithread();
    virtual asIThreadManager *GetThreadManager();
    virtual void AcquireExclusiveLock();
    virtual void ReleaseExclusiveLock();
    virtual void AcquireSharedLock();
    virtual void ReleaseSharedLock();
    virtual int AtomicInc(int &value);
    virtual int AtomicDec(int &value);
    virtual int ThreadCleanup();

    // Memory management
    virtual int SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
    virtual int ResetGlobalMemoryFunctions();
    virtual void *AllocMem(size_t size);
    virtual void FreeMem(void *mem);

    // Auxiliary
    virtual asILockableSharedBool *CreateLockableSharedBool();

    // Script
    virtual int LoadScript(const char *scriptName, const char *filename);
    virtual int LoadScripts(const char *scriptName, const char **filenames, size_t count);
    virtual int CompileScript(const char *scriptName, const char *scriptCode);
    virtual bool UnloadScript(const char *scriptName);

    asIScriptModule *GetScript(const char *scriptName);

    ScriptCache &GetScriptCache() {
        return m_ScriptCache;
    }

    CKERROR ResolveScriptFileName(XString &filename);

    void *GetCKObjectData(CK_ID id) const;
    void SetCKObjectData(CK_ID id, void *data);

    bool IsInited() const {
        return (m_Flags & AS_INITED) != 0;
    }

    int Init();
    int Shutdown();

    void MessageCallback(const asSMessageInfo &msg);
    void ExceptionCallback(asIScriptContext *context);
    std::string GetCallStack(asIScriptContext *context);

    asIScriptContext *RequestContextFromPool();
    void ReturnContextToPool(asIScriptContext *ctx);

    static ScriptManager *GetManager(CKContext *context) {
        return (ScriptManager *) context->GetManagerByGuid(SCRIPT_MANAGER_GUID);
    }

    static ScriptManager *GetManager(asIScriptEngine *engine) {
        return (ScriptManager *) engine->GetUserData(SCRIPT_MANAGER_TYPE);
    }

protected:
    void SetupScriptPathCategory();
    int SetupScriptEngine();

    void RegisterStdTypes(asIScriptEngine *engine);
    void RegisterStdAddons(asIScriptEngine *engine);
    void RegisterVirtools(asIScriptEngine *engine);

    int m_Flags = 0;
    int m_ScriptPathCategoryIndex = -1;
    asIScriptEngine *m_ScriptEngine = nullptr;
    ScriptCache m_ScriptCache;
    std::vector<asIScriptContext *> m_ScriptContexts;
    std::unordered_map<CK_ID, void *> m_CKObjectDataMap;
};

#endif // CK_SCRIPTMANAGER_H
