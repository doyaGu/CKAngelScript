#ifndef CK_ANGELSCRIPTMANAGER_H
#define CK_ANGELSCRIPTMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"
#include "CKTypes.h"

#include <cstddef>

#include <angelscript.h>

#define ANGEL_SCRIPT_MANAGER_GUID CKGUID(0x70955bd2,0x30684456)

struct AngelScriptExecution;
struct CKBehaviorContext;

enum AngelScriptStatus {
    ANGELSCRIPT_STATUS_OK = 0,
    ANGELSCRIPT_STATUS_INVALID_ARGUMENT,
    ANGELSCRIPT_STATUS_NOT_INITIALIZED,
    ANGELSCRIPT_STATUS_NOT_FOUND,
    ANGELSCRIPT_STATUS_COMPILE_ERROR,
    ANGELSCRIPT_STATUS_EXECUTION_FAILED,
    ANGELSCRIPT_STATUS_SUSPENDED,
    ANGELSCRIPT_STATUS_CANCELLED
};

enum AngelScriptExecutionState {
    ANGELSCRIPT_EXECUTION_READY = 0,
    ANGELSCRIPT_EXECUTION_RUNNING,
    ANGELSCRIPT_EXECUTION_SUSPENDED,
    ANGELSCRIPT_EXECUTION_FINISHED,
    ANGELSCRIPT_EXECUTION_FAILED,
    ANGELSCRIPT_EXECUTION_CANCELLED
};

typedef void (*AngelScriptContextCallback)(asIScriptContext *context, void *userData);

struct AngelScriptLoadOptions {
    const char *ModuleName = nullptr;
    const char *Filename = nullptr;
    const char **Filenames = nullptr;
    size_t FileCount = 0;
    const char *Code = nullptr;
    bool ReplaceExisting = false;
};

struct AngelScriptExecuteOptions {
    const char *ModuleName = nullptr;
    const char *FunctionName = nullptr;
    const char *FunctionDecl = nullptr;
    const CKBehaviorContext *BehaviorContext = nullptr;
    AngelScriptContextCallback ConfigureContext = nullptr;
    AngelScriptContextCallback ReadResult = nullptr;
    void *UserData = nullptr;
};

struct AngelScriptResult {
    AngelScriptStatus Status = ANGELSCRIPT_STATUS_OK;
    int AngelScriptCode = 0;
    // ErrorMessage and StackTrace are borrowed strings. Results returned
    // through API output parameters and GetLastResult() remain valid until the
    // next manager API call that updates the last result. GetExecutionResult()
    // strings remain valid until the execution handle is released or started,
    // resumed, or cancelled again.
    const char *ErrorMessage = nullptr;
    const char *StackTrace = nullptr;
};

class AngelScriptManager : public CKBaseManager {
public:
    AngelScriptManager(CKContext *context, CKGUID guid, CKSTRING name)
        : CKBaseManager(context, guid, name) {}
    ~AngelScriptManager() override = default;

    // Engine
    virtual asIScriptEngine *GetScriptEngine() = 0;
    virtual const char *GetVersion() = 0;
    virtual const char *GetOptions() = 0;

    // Context
    virtual asIScriptContext *GetActiveContext() = 0;

    // Modules
    virtual AngelScriptStatus LoadModule(const AngelScriptLoadOptions &options, AngelScriptResult *result = nullptr) = 0;
    virtual AngelScriptStatus CompileModule(const char *moduleName,
                                            const char *scriptCode,
                                            bool replaceExisting = false,
                                            AngelScriptResult *result = nullptr) = 0;
    virtual AngelScriptStatus UnloadModule(const char *moduleName, AngelScriptResult *result = nullptr) = 0;
    virtual bool HasModule(const char *moduleName) = 0;
    virtual asIScriptModule *GetModule(const char *moduleName) = 0;
    virtual asIScriptFunction *FindFunctionByName(const char *moduleName, const char *functionName) = 0;
    virtual asIScriptFunction *FindFunctionByDecl(const char *moduleName, const char *functionDecl) = 0;

    // Executions
    // CreateExecution returns an opaque handle owned by the manager. Release it
    // with ReleaseExecution when finished. StartExecution may return
    // ANGELSCRIPT_STATUS_SUSPENDED for scripts that await async work; call
    // ResumeExecution on a later tick after the async scheduler has advanced.
    virtual AngelScriptExecution *CreateExecution(const AngelScriptExecuteOptions &options,
                                                  AngelScriptResult *result = nullptr) = 0;
    virtual AngelScriptStatus StartExecution(AngelScriptExecution *execution) = 0;
    virtual AngelScriptStatus ResumeExecution(AngelScriptExecution *execution) = 0;
    virtual AngelScriptStatus CancelExecution(AngelScriptExecution *execution) = 0;
    virtual void ReleaseExecution(AngelScriptExecution *execution) = 0;
    virtual AngelScriptExecutionState GetExecutionState(const AngelScriptExecution *execution) const = 0;
    virtual const AngelScriptResult *GetExecutionResult(const AngelScriptExecution *execution) const = 0;
    virtual const AngelScriptResult *GetLastResult() const = 0;

    static AngelScriptManager *GetManager(CKContext *context) {
        return context ? (AngelScriptManager *)context->GetManagerByGuid(ANGEL_SCRIPT_MANAGER_GUID) : nullptr;
    }
};

#endif // CK_ANGELSCRIPTMANAGER_H
