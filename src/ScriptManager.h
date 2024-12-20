#ifndef CK_SCRIPTMANAGER_H
#define CK_SCRIPTMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"

#include <angelscript.h>

#define SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

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

    CKERROR OnCKReset() override;
    CKERROR OnCKPause() override;

    CKERROR PostLoad() override;

    CKERROR OnPostCopy(CKDependenciesContext &context) override;

    CKDWORD GetValidFunctionsMask() override {
        return CKMANAGER_FUNC_PostClearAll |
               CKMANAGER_FUNC_OnCKReset |
               CKMANAGER_FUNC_OnCKPause |
               CKMANAGER_FUNC_PostLoad |
               CKMANAGER_FUNC_OnPostCopy;
    }

    // Engine
    virtual asIScriptEngine *CreateScriptEngine(asDWORD version = ANGELSCRIPT_VERSION);
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

    virtual asIScriptEngine *GetScriptEngine();
    virtual asIScriptContext *GetScriptContext();

    // Script
    virtual int LoadScript(const char *moduleName, const char *filename);
    virtual void UnloadScript(const char *moduleName);

    asIScriptModule *GetScript(const char *moduleName);
    int ExecuteScript(const char *moduleName, const char *decl);

    bool IsInited() const {
        return (m_Flags & AS_INITED) != 0;
    }

    int Init();
    int Shutdown();

    void MessageCallback(const asSMessageInfo &msg);
    void ExceptionCallback(asIScriptContext *context);
    XString GetCallStack(asIScriptContext*context);

    static ScriptManager *GetManager(CKContext *context) {
        return (ScriptManager *)context->GetManagerByGuid(SCRIPT_MANAGER_GUID);
    }

protected:
    void RegisterStdTypes(asIScriptEngine *engine);
    void RegisterStdAddons(asIScriptEngine *engine);
    void RegisterVirtools(asIScriptEngine *engine);

    CKERROR ResolveScriptFileName(XString &filename);

    int m_Flags = 0;
    asIScriptEngine *m_ScriptEngine = nullptr;
    asIScriptContext *m_ScriptContext = nullptr;
    int m_ScriptPathCategoryIndex = -1;
};

#endif // CK_SCRIPTMANAGER_H
