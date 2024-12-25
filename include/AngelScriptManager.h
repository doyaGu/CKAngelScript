#ifndef CK_ANGELSCRIPTMANAGER_H
#define CK_ANGELSCRIPTMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"

#include <angelscript.h>

#define ANGEL_SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

class AngelScriptManager : public CKBaseManager {
public:
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
    virtual int LoadScript(const char *scriptName, const char *filename);
    virtual int LoadScript(const char *scriptName, const char **filenames, size_t count);
    virtual int CompileScript(const char *scriptName, const char *scriptCode);
    virtual void UnloadScript(const char *scriptName);

    static AngelScriptManager *GetManager(CKContext *context) {
        return (AngelScriptManager *)context->GetManagerByGuid(ANGEL_SCRIPT_MANAGER_GUID);
    }
};

#endif // CK_ANGELSCRIPTMANAGER_H
