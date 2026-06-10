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
typedef struct CKAngelScriptFunction CKAngelScriptFunction;
typedef struct CKAngelScriptObject CKAngelScriptObject;
typedef struct CKAngelScriptMethod CKAngelScriptMethod;
typedef struct CKAngelScriptArgWriter CKAngelScriptArgWriter;
typedef struct CKAngelScriptResultReader CKAngelScriptResultReader;
typedef struct CKBehaviorContext CKBehaviorContext;
typedef struct CKAngelScript CKAngelScript;

#define CKAS_API_VERSION 3

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
    CKAS_CANCELLED,
    CKAS_STALEHANDLE,
    CKAS_UNSUPPORTED,
    CKAS_TYPEMISMATCH,
    CKAS_BUFFERTOOSMALL,
    CKAS_INVALIDSTATE,
    CKAS_INUSE,
    CKAS_ALREADYEXISTS,
    CKAS_AMBIGUOUS,
    CKAS_FOREIGNHANDLE
} CKAS_STATUS;

typedef enum CKAS_FEATURE {
    CKAS_FEATURE_MODULE_LIFECYCLE = 1,
    CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS = 2,
    CKAS_FEATURE_FUNCTION_HANDLE = 3,
    CKAS_FEATURE_FUNCTION_EXECUTION = 4,
    CKAS_FEATURE_FUNCTION_EXECUTION_RESUME = 5,
    CKAS_FEATURE_OBJECT_HANDLE = 6,
    CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL = 7,
    CKAS_FEATURE_TYPED_ARG_READER_WRITER = 8,
    CKAS_FEATURE_STACK_TRACE = 9,
    CKAS_FEATURE_ENGINE_EXTENSION = 10
} CKAS_FEATURE;

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

typedef enum CKAS_CALLFLAGS {
    CKAS_CALL_DEFAULT = 0,
    CKAS_CALL_NO_SUSPEND = 0x00000001
} CKAS_CALLFLAGS;

typedef CKAS_STATUS (*CKAngelScriptContextCallback)(asIScriptContext *context, void *userData);
typedef int (*CKAngelScriptEngineExtensionCallback)(asIScriptEngine *engine,
                                                    CKAngelScript *angelScript,
                                                    void *userData,
                                                    const char **errorMessage);
typedef CKAS_STATUS (*CKAngelScriptWriteArgsCallback)(CKAngelScriptArgWriter *writer,
                                                      void *userData);
typedef CKAS_STATUS (*CKAngelScriptReadResultCallback)(CKAngelScriptResultReader *reader,
                                                       void *userData);

typedef struct CKAngelScriptLoadOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *Filename;
    const char **Filenames;
    size_t FileCount;
    const char *Code;
    CKDWORD Flags;
} CKAngelScriptLoadOptions;

typedef struct CKAngelScriptFunctionOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *FunctionName;
    const char *FunctionDecl;
    CKDWORD Flags;
} CKAngelScriptFunctionOptions;

typedef struct CKAngelScriptFunctionExecutionOptions {
    CKDWORD Size;
    CKAngelScriptFunction *Function;
    const CKBehaviorContext *BehaviorContext;
    CKAngelScriptContextCallback ConfigureContext;
    CKAngelScriptContextCallback ReadResult;
    void *UserData;
    CKDWORD Flags;
} CKAngelScriptFunctionExecutionOptions;

typedef struct CKAngelScriptObjectOptions {
    CKDWORD Size;
    const char *ModuleName;
    const char *ClassName;
} CKAngelScriptObjectOptions;

typedef struct CKAngelScriptMethodOptions {
    CKDWORD Size;
    CKAngelScriptObject *Object;
    const char *MethodName;
    const char *MethodDecl;
} CKAngelScriptMethodOptions;

typedef struct CKAngelScriptObjectMethodExecuteOptions {
    CKDWORD Size;
    CKAngelScriptObject *Object;
    CKAngelScriptMethod *Method;
    CKAngelScriptWriteArgsCallback WriteArgs;
    CKAngelScriptReadResultCallback ReadResult;
    void *UserData;
    CKDWORD Flags;
} CKAngelScriptObjectMethodExecuteOptions;

typedef struct CKAngelScriptResult {
    CKDWORD Size;
    CKAS_STATUS Status;
    int AngelScriptCode;
    // ErrorMessage and StackTrace are borrowed strings. Results returned
    // through API output parameters and GetLastResult() remain valid until the
    // next CKAngelScript API call that updates the last result. BorrowExecutionResult()
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

// C ABI. CKAngelScript is an opaque pointer owned by the plugin; never allocate
// or free it from host code. Borrowed AngelScript pointers are never AddRef'd by
// CKAngelScript and must not be released by the caller. CKAngelScript* handles
// returned from Find/Create calls must be released with the matching release API.
#ifdef __cplusplus
extern "C" {
#endif

CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context);

CKAS_API CKDWORD CKAngelScriptGetApiVersion();
CKAS_API CKBOOL CKAngelScriptHasFeature(CKAS_FEATURE feature);
CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript);
CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptBorrowEngine(CKAngelScript *angelScript,
                                               asIScriptEngine **outEngine,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowActiveContext(CKAngelScript *angelScript,
                                                      asIScriptContext **outContext,
                                                      CKAngelScriptResult *result);

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
CKAS_API CKDWORD CKAngelScriptGetModuleGeneration(CKAngelScript *angelScript,
                                                  const char *moduleName);
CKAS_API CKAS_STATUS CKAngelScriptBorrowModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               asIScriptModule **outModule,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByName(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionName,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByDecl(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionDecl,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptFindFunction(CKAngelScript *angelScript,
                                               const CKAngelScriptFunctionOptions *options,
                                               CKAngelScriptFunction **outFunction,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseFunction(CKAngelScript *angelScript,
                                                  CKAngelScriptFunction *function);

CKAS_API CKAS_STATUS CKAngelScriptCreateObject(CKAngelScript *angelScript,
                                               const CKAngelScriptObjectOptions *options,
                                               CKAngelScriptObject **outObject,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                CKAngelScriptObject *object);
CKAS_API CKAS_STATUS CKAngelScriptFindObjectMethod(CKAngelScript *angelScript,
                                                   const CKAngelScriptMethodOptions *options,
                                                   CKAngelScriptMethod **outMethod,
                                                   CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                CKAngelScriptMethod *method);

CKAS_API CKAS_STATUS CKAngelScriptArgSetBool(CKAngelScriptArgWriter *writer,
                                             CKDWORD index,
                                             CKBOOL value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetInt(CKAngelScriptArgWriter *writer,
                                            CKDWORD index,
                                            int value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetFloat(CKAngelScriptArgWriter *writer,
                                              CKDWORD index,
                                              float value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetString(CKAngelScriptArgWriter *writer,
                                               CKDWORD index,
                                               const char *value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetBorrowedObject(CKAngelScriptArgWriter *writer,
                                                       CKDWORD index,
                                                       void *object);

CKAS_API CKAS_STATUS CKAngelScriptResultGetBool(CKAngelScriptResultReader *reader,
                                                CKBOOL *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetInt(CKAngelScriptResultReader *reader,
                                               int *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetFloat(CKAngelScriptResultReader *reader,
                                                 float *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetString(CKAngelScriptResultReader *reader,
                                                  char *buffer,
                                                  size_t bufferSize,
                                                  size_t *outRequiredSize);
CKAS_API CKAS_STATUS CKAngelScriptResultGetStringView(CKAngelScriptResultReader *reader,
                                                      const char **data,
                                                      size_t *size);

CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptCreateFunctionExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionExecutionOptions *options,
    CKAngelScriptExecution **outExecution,
    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                 CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                   CKAngelScriptExecution *execution);
CKAS_API CKAS_STATUS CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                    const CKAngelScriptExecution *execution,
                                                    CKAS_EXECUTIONSTATE *outState);
CKAS_API CKAS_STATUS CKAngelScriptBorrowExecutionResult(CKAngelScript *angelScript,
                                                        const CKAngelScriptExecution *execution,
                                                        const CKAngelScriptResult **outResult);
CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                          const CKAngelScriptEngineExtension *extension,
                                                          CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                            const char *name,
                                                            CKAngelScriptResult *result);

#ifdef __cplusplus
}

class CKAngelScriptObjectHandle {
public:
    CKAngelScriptObjectHandle() = default;
    CKAngelScriptObjectHandle(CKAngelScript *angelScript, CKAngelScriptObject *object)
        : m_AngelScript(angelScript), m_Object(object) {}
    CKAngelScriptObjectHandle(const CKAngelScriptObjectHandle &) = delete;
    CKAngelScriptObjectHandle &operator=(const CKAngelScriptObjectHandle &) = delete;
    CKAngelScriptObjectHandle(CKAngelScriptObjectHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptObjectHandle &operator=(CKAngelScriptObjectHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptObjectHandle() {
        Reset();
    }
    CKAngelScriptObject *Get() const {
        return m_Object;
    }
    CKAngelScriptObject *Release() {
        CKAngelScriptObject *object = m_Object;
        m_Object = nullptr;
        m_AngelScript = nullptr;
        return object;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptObject *object = nullptr) {
        if (m_Object) {
            (void)CKAngelScriptReleaseObject(m_AngelScript, m_Object);
        }
        m_AngelScript = angelScript;
        m_Object = object;
    }
private:
    void MoveFrom(CKAngelScriptObjectHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Object = other.m_Object;
        other.m_AngelScript = nullptr;
        other.m_Object = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptObject *m_Object = nullptr;
};

class CKAngelScriptFunctionHandle {
public:
    CKAngelScriptFunctionHandle() = default;
    CKAngelScriptFunctionHandle(CKAngelScript *angelScript, CKAngelScriptFunction *function)
        : m_AngelScript(angelScript), m_Function(function) {}
    CKAngelScriptFunctionHandle(const CKAngelScriptFunctionHandle &) = delete;
    CKAngelScriptFunctionHandle &operator=(const CKAngelScriptFunctionHandle &) = delete;
    CKAngelScriptFunctionHandle(CKAngelScriptFunctionHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptFunctionHandle &operator=(CKAngelScriptFunctionHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptFunctionHandle() {
        Reset();
    }
    CKAngelScriptFunction *Get() const {
        return m_Function;
    }
    CKAngelScriptFunction *Release() {
        CKAngelScriptFunction *function = m_Function;
        m_Function = nullptr;
        m_AngelScript = nullptr;
        return function;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptFunction *function = nullptr) {
        if (m_Function) {
            (void)CKAngelScriptReleaseFunction(m_AngelScript, m_Function);
        }
        m_AngelScript = angelScript;
        m_Function = function;
    }
private:
    void MoveFrom(CKAngelScriptFunctionHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Function = other.m_Function;
        other.m_AngelScript = nullptr;
        other.m_Function = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptFunction *m_Function = nullptr;
};

class CKAngelScriptMethodHandle {
public:
    CKAngelScriptMethodHandle() = default;
    CKAngelScriptMethodHandle(CKAngelScript *angelScript, CKAngelScriptMethod *method)
        : m_AngelScript(angelScript), m_Method(method) {}
    CKAngelScriptMethodHandle(const CKAngelScriptMethodHandle &) = delete;
    CKAngelScriptMethodHandle &operator=(const CKAngelScriptMethodHandle &) = delete;
    CKAngelScriptMethodHandle(CKAngelScriptMethodHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptMethodHandle &operator=(CKAngelScriptMethodHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptMethodHandle() {
        Reset();
    }
    CKAngelScriptMethod *Get() const {
        return m_Method;
    }
    CKAngelScriptMethod *Release() {
        CKAngelScriptMethod *method = m_Method;
        m_Method = nullptr;
        m_AngelScript = nullptr;
        return method;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptMethod *method = nullptr) {
        if (m_Method) {
            (void)CKAngelScriptReleaseMethod(m_AngelScript, m_Method);
        }
        m_AngelScript = angelScript;
        m_Method = method;
    }
private:
    void MoveFrom(CKAngelScriptMethodHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Method = other.m_Method;
        other.m_AngelScript = nullptr;
        other.m_Method = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptMethod *m_Method = nullptr;
};

class CKAngelScriptExecutionHandle {
public:
    CKAngelScriptExecutionHandle() = default;
    CKAngelScriptExecutionHandle(CKAngelScript *angelScript, CKAngelScriptExecution *execution)
        : m_AngelScript(angelScript), m_Execution(execution) {}
    CKAngelScriptExecutionHandle(const CKAngelScriptExecutionHandle &) = delete;
    CKAngelScriptExecutionHandle &operator=(const CKAngelScriptExecutionHandle &) = delete;
    CKAngelScriptExecutionHandle(CKAngelScriptExecutionHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptExecutionHandle &operator=(CKAngelScriptExecutionHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptExecutionHandle() {
        Reset();
    }
    CKAngelScriptExecution *Get() const {
        return m_Execution;
    }
    CKAngelScriptExecution *Release() {
        CKAngelScriptExecution *execution = m_Execution;
        m_Execution = nullptr;
        m_AngelScript = nullptr;
        return execution;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptExecution *execution = nullptr) {
        if (m_Execution) {
            (void)CKAngelScriptReleaseExecution(m_AngelScript, m_Execution);
        }
        m_AngelScript = angelScript;
        m_Execution = execution;
    }
private:
    void MoveFrom(CKAngelScriptExecutionHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Execution = other.m_Execution;
        other.m_AngelScript = nullptr;
        other.m_Execution = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptExecution *m_Execution = nullptr;
};

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

    static CKAngelScriptFunctionOptions FunctionOptions() {
        CKAngelScriptFunctionOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptFunctionExecutionOptions FunctionExecutionOptions() {
        CKAngelScriptFunctionExecutionOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptObjectOptions ObjectOptions() {
        CKAngelScriptObjectOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptMethodOptions MethodOptions() {
        CKAngelScriptMethodOptions options = {};
        options.Size = sizeof(options);
        return options;
    }

    static CKAngelScriptObjectMethodExecuteOptions ObjectMethodExecuteOptions() {
        CKAngelScriptObjectMethodExecuteOptions options = {};
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

    CKDWORD GetApiVersion() const {
        (void)m_AngelScript;
        return CKAngelScriptGetApiVersion();
    }

    CKBOOL HasFeature(CKAS_FEATURE feature) const {
        (void)m_AngelScript;
        return CKAngelScriptHasFeature(feature);
    }

    const char *GetVersion() const {
        return CKAngelScriptGetVersion(m_AngelScript);
    }

    const char *GetOptions() const {
        return CKAngelScriptGetOptions(m_AngelScript);
    }

    CKAS_STATUS BorrowEngine(asIScriptEngine **outEngine, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowEngine(m_AngelScript, outEngine, result);
    }

    CKAS_STATUS BorrowActiveContext(asIScriptContext **outContext,
                                    CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowActiveContext(m_AngelScript, outContext, result);
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

    CKDWORD GetModuleGeneration(const char *moduleName) const {
        return CKAngelScriptGetModuleGeneration(m_AngelScript, moduleName);
    }

    CKAS_STATUS BorrowModule(const char *moduleName,
                             asIScriptModule **outModule,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowModule(m_AngelScript, moduleName, outModule, result);
    }

    CKAS_STATUS BorrowFunctionByName(const char *moduleName,
                                     const char *functionName,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowFunctionByName(m_AngelScript, moduleName, functionName, outFunction, result);
    }

    CKAS_STATUS BorrowFunctionByDecl(const char *moduleName,
                                     const char *functionDecl,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowFunctionByDecl(m_AngelScript, moduleName, functionDecl, outFunction, result);
    }

    CKAS_STATUS FindFunction(const CKAngelScriptFunctionOptions &options,
                             CKAngelScriptFunction **outFunction,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptFindFunction(m_AngelScript, &options, outFunction, result);
    }

    CKAS_STATUS ReleaseFunction(CKAngelScriptFunction *function) const {
        return CKAngelScriptReleaseFunction(m_AngelScript, function);
    }

    CKAS_STATUS CreateObject(const CKAngelScriptObjectOptions &options,
                             CKAngelScriptObject **outObject,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCreateObject(m_AngelScript, &options, outObject, result);
    }

    CKAS_STATUS ReleaseObject(CKAngelScriptObject *object) const {
        return CKAngelScriptReleaseObject(m_AngelScript, object);
    }

    CKAS_STATUS FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                 CKAngelScriptMethod **outMethod,
                                 CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptFindObjectMethod(m_AngelScript, &options, outMethod, result);
    }

    CKAS_STATUS ReleaseMethod(CKAngelScriptMethod *method) const {
        return CKAngelScriptReleaseMethod(m_AngelScript, method);
    }

    CKAS_STATUS CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                 CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCallObjectMethod(m_AngelScript, &options, result);
    }

    CKAS_STATUS CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                        CKAngelScriptExecution **outExecution,
                                        CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCreateFunctionExecution(m_AngelScript, &options, outExecution, result);
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

    CKAS_STATUS ReleaseExecution(CKAngelScriptExecution *execution) const {
        return CKAngelScriptReleaseExecution(m_AngelScript, execution);
    }

    CKAS_STATUS GetExecutionState(const CKAngelScriptExecution *execution,
                                  CKAS_EXECUTIONSTATE *outState) const {
        return CKAngelScriptGetExecutionState(m_AngelScript, execution, outState);
    }

    CKAS_STATUS BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                      const CKAngelScriptResult **outResult) const {
        return CKAngelScriptBorrowExecutionResult(m_AngelScript, execution, outResult);
    }

    const CKAngelScriptResult *GetLastResult() const {
        return CKAngelScriptGetLastResult(m_AngelScript);
    }

    CKAS_STATUS RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                        CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptRegisterEngineExtension(m_AngelScript, &extension, result);
    }

    CKAS_STATUS UnregisterEngineExtension(const char *name,
                                          CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptUnregisterEngineExtension(m_AngelScript, name, result);
    }

private:
    CKAngelScript *m_AngelScript = nullptr;
};
#endif // __cplusplus

#endif // CK_ANGELSCRIPT_H
