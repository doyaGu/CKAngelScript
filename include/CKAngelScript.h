#ifndef CK_ANGELSCRIPT_H
#define CK_ANGELSCRIPT_H

#include "CKTypes.h"

#include <stddef.h>

#include <angelscript.h>

#define CKAS_GUID CKGUID(0x70955bd2,0x30684456)

#if defined(_WIN32)
#if defined(CKAS_BUILDING_DLL) || defined(CKAngelScript_EXPORTS)
#define CKAS_API __declspec(dllexport)
#else
#define CKAS_API __declspec(dllimport)
#endif
#else
#define CKAS_API
#endif

typedef struct CKAngelScriptExecution CKAngelScriptExecution;
typedef struct CKBehaviorContext CKBehaviorContext;
typedef struct CKAngelScript CKAngelScript;

#ifdef __cplusplus
class CKContext;
#else
typedef struct CKContext CKContext;
#endif

typedef enum CKAS_STATUS {
    CKAS_OK = 0,
    CKAS_INVALIDARGUMENT,
    CKAS_NOTINITIALIZED,
    CKAS_NOTFOUND,
    CKAS_COMPILEERROR,
    CKAS_EXECUTIONFAILED,
    CKAS_SUSPENDED,
    CKAS_CANCELLED
} CKAS_STATUS;

typedef enum CKAS_EXECUTIONSTATE {
    CKAS_EXECUTION_READY = 0,
    CKAS_EXECUTION_RUNNING,
    CKAS_EXECUTION_SUSPENDED,
    CKAS_EXECUTION_FINISHED,
    CKAS_EXECUTION_FAILED,
    CKAS_EXECUTION_CANCELLED
} CKAS_EXECUTIONSTATE;

typedef enum CKAS_LOADFLAGS {
    CKAS_LOAD_DEFAULT = 0,
    CKAS_LOAD_REPLACEEXISTING = 0x00000001
} CKAS_LOADFLAGS;

typedef enum CKAS_COMPILEFLAGS {
    CKAS_COMPILE_DEFAULT = 0,
    CKAS_COMPILE_REPLACEEXISTING = 0x00000001
} CKAS_COMPILEFLAGS;

typedef enum CKAS_ENGINEEXTENSIONFLAGS {
    CKAS_ENGINEEXTENSION_DEFAULT = 0,
    CKAS_ENGINEEXTENSION_DEFERRED = 0x00000001
} CKAS_ENGINEEXTENSIONFLAGS;

typedef void (*CKAngelScriptContextCallback)(asIScriptContext *context, void *userData);
typedef int (*CKAngelScriptEngineExtensionCallback)(asIScriptEngine *engine,
                                                    CKAngelScript *angelScript,
                                                    void *userData,
                                                    const char **errorMessage);

typedef struct CKAngelScriptLoadOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *Filename;
    const char **Filenames;
    size_t FileCount;
    const char *Code;
    CKDWORD Flags;
} CKAngelScriptLoadOptions;

typedef struct CKAngelScriptExecuteOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *FunctionName;
    const char *FunctionDecl;
    const CKBehaviorContext *BehaviorContext;
    CKAngelScriptContextCallback ConfigureContext;
    CKAngelScriptContextCallback ReadResult;
    void *UserData;
} CKAngelScriptExecuteOptions;

typedef struct CKAngelScriptResult {
    CKDWORD Size;
    CKAS_STATUS Status;
    int AngelScriptCode;
    // ErrorMessage and StackTrace are borrowed strings. Results returned
    // through API output parameters and GetLastResult() remain valid until the
    // next CKAngelScript API call that updates the last result. GetExecutionResult()
    // strings remain valid until the execution handle is released or started,
    // resumed, or cancelled again.
    const char *ErrorMessage;
    const char *StackTrace;
} CKAngelScriptResult;

typedef struct CKAngelScriptEngineExtension {
    CKDWORD Size;
    const char *Name;
    CKAngelScriptEngineExtensionCallback Register;
    void *UserData;
    // By default RegisterEngineExtension invokes the callback immediately when
    // the engine is already initialized. Set CKAS_ENGINEEXTENSION_DEFERRED
    // to retain the extension only for the next engine rebuild.
    CKDWORD Flags;
} CKAngelScriptEngineExtension;

// C ABI. CKAngelScript is an opaque pointer owned by the plugin;
// never allocate or free it from host code. Execution handles returned by
// CKAngelScriptCreateExecution must be released with CKAngelScriptReleaseExecution.
#ifdef __cplusplus
extern "C" {
#endif

CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context);

CKAS_API asIScriptEngine *CKAngelScriptGetScriptEngine(CKAngelScript *angelScript);
CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript);
CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript);
CKAS_API asIScriptContext *CKAngelScriptGetActiveContext(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                             const CKAngelScriptLoadOptions *options,
                                             CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptCompileModule(CKAngelScript *angelScript,
                                                const char *moduleName,
                                                const char *scriptCode,
                                                CKDWORD flags,
                                                CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnloadModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               CKAngelScriptResult *result);
CKAS_API CKBOOL CKAngelScriptHasModule(CKAngelScript *angelScript, const char *moduleName);
CKAS_API asIScriptModule *CKAngelScriptGetModule(CKAngelScript *angelScript, const char *moduleName);
CKAS_API asIScriptFunction *CKAngelScriptFindFunctionByName(CKAngelScript *angelScript,
                                                            const char *moduleName,
                                                            const char *functionName);
CKAS_API asIScriptFunction *CKAngelScriptFindFunctionByDecl(CKAngelScript *angelScript,
                                                            const char *moduleName,
                                                            const char *functionDecl);

CKAS_API CKAngelScriptExecution *CKAngelScriptCreateExecution(CKAngelScript *angelScript,
                                                             const CKAngelScriptExecuteOptions *options,
                                                             CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                 CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API void CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                            CKAngelScriptExecution *execution);
CKAS_API CKAS_EXECUTIONSTATE CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                           const CKAngelScriptExecution *execution);
CKAS_API const CKAngelScriptResult *CKAngelScriptGetExecutionResult(CKAngelScript *angelScript,
                                                                    const CKAngelScriptExecution *execution);
CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                          const CKAngelScriptEngineExtension *extension,
                                                          CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                            const char *name,
                                                            void *userData,
                                                            CKAngelScriptResult *result);

#ifdef __cplusplus
}

// C++ convenience wrapper over the C ABI. This wrapper owns nothing and is safe
// to copy; it is invalid when Handle() returns nullptr.
class CKAngelScriptApi {
public:
    CKAngelScriptApi() = default;
    explicit CKAngelScriptApi(CKAngelScript *angelScript) : m_AngelScript(angelScript) {}

    static CKAngelScriptApi Get(CKContext *context) {
        return CKAngelScriptApi(CKGetAngelScript(context));
    }

    static CKAngelScriptLoadOptions LoadOptions() {
        CKAngelScriptLoadOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptExecuteOptions ExecuteOptions() {
        CKAngelScriptExecuteOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptEngineExtension EngineExtension() {
        CKAngelScriptEngineExtension extension = {};
        extension.Size = sizeof(extension);
        return extension;
    }

    CKAngelScript *Handle() const {
        return m_AngelScript;
    }

    CKBOOL IsValid() const {
        return m_AngelScript ? TRUE : FALSE;
    }

    CKAngelScriptApi *operator->() {
        return this;
    }

    const CKAngelScriptApi *operator->() const {
        return this;
    }

    asIScriptEngine *GetScriptEngine() const {
        return CKAngelScriptGetScriptEngine(m_AngelScript);
    }

    const char *GetVersion() const {
        return CKAngelScriptGetVersion(m_AngelScript);
    }

    const char *GetOptions() const {
        return CKAngelScriptGetOptions(m_AngelScript);
    }

    asIScriptContext *GetActiveContext() const {
        return CKAngelScriptGetActiveContext(m_AngelScript);
    }

    CKAS_STATUS LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptLoadModule(m_AngelScript, &options, result);
    }

    CKAS_STATUS CompileModule(const char *moduleName,
                              const char *scriptCode,
                              CKDWORD flags = CKAS_COMPILE_DEFAULT,
                              CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCompileModule(m_AngelScript, moduleName, scriptCode, flags, result);
    }

    CKAS_STATUS UnloadModule(const char *moduleName, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptUnloadModule(m_AngelScript, moduleName, result);
    }

    CKBOOL HasModule(const char *moduleName) const {
        return CKAngelScriptHasModule(m_AngelScript, moduleName);
    }

    asIScriptModule *GetModule(const char *moduleName) const {
        return CKAngelScriptGetModule(m_AngelScript, moduleName);
    }

    asIScriptFunction *FindFunctionByName(const char *moduleName, const char *functionName) const {
        return CKAngelScriptFindFunctionByName(m_AngelScript, moduleName, functionName);
    }

    asIScriptFunction *FindFunctionByDecl(const char *moduleName, const char *functionDecl) const {
        return CKAngelScriptFindFunctionByDecl(m_AngelScript, moduleName, functionDecl);
    }

    CKAngelScriptExecution *CreateExecution(const CKAngelScriptExecuteOptions &options,
                                            CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCreateExecution(m_AngelScript, &options, result);
    }

    CKAS_STATUS StartExecution(CKAngelScriptExecution *execution) const {
        return CKAngelScriptStartExecution(m_AngelScript, execution);
    }

    CKAS_STATUS ResumeExecution(CKAngelScriptExecution *execution) const {
        return CKAngelScriptResumeExecution(m_AngelScript, execution);
    }

    CKAS_STATUS CancelExecution(CKAngelScriptExecution *execution) const {
        return CKAngelScriptCancelExecution(m_AngelScript, execution);
    }

    void ReleaseExecution(CKAngelScriptExecution *execution) const {
        CKAngelScriptReleaseExecution(m_AngelScript, execution);
    }

    CKAS_EXECUTIONSTATE GetExecutionState(const CKAngelScriptExecution *execution) const {
        return CKAngelScriptGetExecutionState(m_AngelScript, execution);
    }

    const CKAngelScriptResult *GetExecutionResult(const CKAngelScriptExecution *execution) const {
        return CKAngelScriptGetExecutionResult(m_AngelScript, execution);
    }

    const CKAngelScriptResult *GetLastResult() const {
        return CKAngelScriptGetLastResult(m_AngelScript);
    }

    CKAS_STATUS RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                        CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptRegisterEngineExtension(m_AngelScript, &extension, result);
    }

    CKAS_STATUS UnregisterEngineExtension(const char *name,
                                          void *userData = nullptr,
                                          CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptUnregisterEngineExtension(m_AngelScript, name, userData, result);
    }

private:
    CKAngelScript *m_AngelScript = nullptr;
};
#endif // __cplusplus

#endif // CK_ANGELSCRIPT_H
