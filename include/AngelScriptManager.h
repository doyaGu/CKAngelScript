#ifndef CK_ANGELSCRIPTMANAGER_H
#define CK_ANGELSCRIPTMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"

#include <angelscript.h>

#define ANGEL_SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

class AngelScriptManager : public CKBaseManager {
public:
    // Engine
    virtual asIScriptEngine *GetScriptEngine() = 0;
    virtual asIScriptEngine *CreateScriptEngine(asDWORD version = ANGELSCRIPT_VERSION) = 0;
    virtual const char *GetVersion() = 0;
    virtual const char *GetOptions() = 0;

    // Context
    virtual asIScriptContext *GetActiveContext() = 0;

    // Thread support
    virtual int PrepareMultithread(asIThreadManager *externalMgr = nullptr) = 0;
    virtual void UnprepareMultithread() = 0;
    virtual asIThreadManager *GetThreadManager() = 0;
    virtual void AcquireExclusiveLock() = 0;
    virtual void ReleaseExclusiveLock() = 0;
    virtual void AcquireSharedLock() = 0;
    virtual void ReleaseSharedLock() = 0;
    virtual int AtomicInc(int &value) = 0;
    virtual int AtomicDec(int &value) = 0;
    virtual int ThreadCleanup() = 0;

    // Memory management
    virtual int SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc) = 0;
    virtual int ResetGlobalMemoryFunctions() = 0;
    virtual void *AllocMem(size_t size) = 0;
    virtual void FreeMem(void *mem) = 0;

    // Auxiliary
    virtual asILockableSharedBool *CreateLockableSharedBool() = 0;

    // Script
    virtual int LoadScript(const char *scriptName, const char *filename) = 0;
    virtual int LoadScripts(const char *scriptName, const char **filenames, size_t count) = 0;
    virtual int CompileScript(const char *scriptName, const char *scriptCode) = 0;
    virtual void UnloadScript(const char *scriptName) = 0;

    static AngelScriptManager *GetManager(CKContext *context) {
        return (AngelScriptManager *)context->GetManagerByGuid(ANGEL_SCRIPT_MANAGER_GUID);
    }
};

#endif // CK_ANGELSCRIPTMANAGER_H
