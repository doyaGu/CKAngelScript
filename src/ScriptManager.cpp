#include "ScriptManager.h"

#include <cstring>
#include <fmt/format.h>

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

#ifndef CKAS_ENABLE_DYNCALL
#define CKAS_ENABLE_DYNCALL 0
#endif

#include "CKPathManager.h"
#include "Logger.h"
#include "ScriptInvoker.h"

#include "ScriptInfo.h"
#include "ScriptFormat.h"
#include "ScriptNativePointer.h"
#include "ScriptNativeBuffer.h"
#if CKAS_ENABLE_DYNCALL
#include "ScriptDynCall.h"
#endif
#include "ScriptUtils.h"
#include "ScriptVxMath.h"
#include "ScriptCK2.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptParameterRegistry.h"
#include "ScriptScene.h"
#include "ScriptRuntime.h"
#include "ScriptMessage.h"
#include "ScriptAsync.h"
#include "ScriptRegistration.h"

#if CKAS_BUILD_SELF_TESTS
#include "ScriptSelfTests.h"
#endif

// Application modules
#include "add_on/scripthelper/scripthelper.h"
#include "add_on/scriptbuilder/scriptbuilder.h"

// Script extensions
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "add_on/scriptarray/scriptarray.h"
#include "add_on/scriptany/scriptany.h"
#include "add_on/scripthandle/scripthandle.h"
#include "add_on/weakref/weakref.h"
#include "add_on/scriptdictionary/scriptdictionary.h"
#include "add_on/scriptfile/scriptfile.h"
#include "add_on/scriptfile/scriptfilesystem.h"
#include "add_on/scriptmath/scriptmath.h"
#include "add_on/scriptmath/scriptmathcomplex.h"
#include "add_on/scriptgrid/scriptgrid.h"
#include "add_on/datetime/datetime.h"

struct CKAngelScriptExecution {
    explicit CKAngelScriptExecution(ScriptManager *manager)
        : Manager(manager), Invoker(manager) {}

    ~CKAngelScriptExecution() {
        if (Function) {
            Function->Release();
            Function = nullptr;
        }
    }

    ScriptManager *Manager = nullptr;
    ScriptInvoker Invoker;
    asIScriptFunction *Function = nullptr;
    CKAS_EXECUTIONSTATE State = CKAS_EXECUTION_READY;
    CKAngelScriptResult Result = {sizeof(CKAngelScriptResult), CKAS_OK, 0, nullptr, nullptr};
    std::string ErrorMessage;
    std::string StackTrace;
    std::string ModuleName;
    std::string FunctionName;
    std::string FunctionDecl;
    CKDWORD ModuleGeneration = 0;
    CKBehaviorContext BehaviorContextStorage;
    bool HasBehaviorContext = false;
    CKAngelScriptContextCallback ConfigureContext = nullptr;
    CKAngelScriptContextCallback ReadResult = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_CALL_DEFAULT;
};

struct CKAngelScriptFunction {
    ScriptManager *Manager = nullptr;
    std::string ModuleName;
    std::string FunctionName;
    std::string FunctionDecl;
    CKDWORD ModuleGeneration = 0;
};

struct CKAngelScriptObject {
    ScriptManager *Manager = nullptr;
    asIScriptObject *Object = nullptr;
    std::string ModuleName;
    std::string ClassName;
    CKDWORD ModuleGeneration = 0;
};

struct CKAngelScriptMethod {
    ScriptManager *Manager = nullptr;
    std::string ModuleName;
    std::string ClassName;
    std::string MethodName;
    std::string MethodDecl;
    CKDWORD ModuleGeneration = 0;
    std::vector<int> ParamTypes;
    std::vector<asDWORD> ParamFlags;
    int ReturnType = asTYPEID_VOID;
    asDWORD ReturnFlags = 0;
};

struct CKAngelScriptStringArg {
    CKDWORD Index = 0;
    std::string Value;
};

struct CKAngelScriptArgWriter {
    asIScriptContext *Context = nullptr;
    const CKAngelScriptMethod *Method = nullptr;
    std::vector<CKAngelScriptStringArg> StringArgs;
};

struct CKAngelScriptResultReader {
    asIScriptContext *Context = nullptr;
    const CKAngelScriptMethod *Method = nullptr;
};

namespace ScriptManagerModuleReplacementInternal {

class MemoryByteCodeStream : public asIBinaryStream {
public:
    explicit MemoryByteCodeStream(std::vector<unsigned char> *buffer)
        : m_Buffer(buffer) {}

    int Read(void *ptr, asUINT size) override {
        if (!m_Buffer || !ptr || m_ReadOffset > m_Buffer->size() || size > m_Buffer->size() - m_ReadOffset) {
            return -1;
        }
        if (size > 0) {
            std::memcpy(ptr, m_Buffer->data() + m_ReadOffset, size);
            m_ReadOffset += size;
        }
        return 0;
    }

    int Write(const void *ptr, asUINT size) override {
        if (!m_Buffer || (!ptr && size > 0)) {
            return -1;
        }
        const size_t offset = m_Buffer->size();
        m_Buffer->resize(offset + size);
        if (size > 0) {
            std::memcpy(m_Buffer->data() + offset, ptr, size);
        }
        return 0;
    }

private:
    std::vector<unsigned char> *m_Buffer = nullptr;
    size_t m_ReadOffset = 0;
};

bool SaveModuleByteCode(asIScriptModule *module,
                        std::vector<unsigned char> &byteCode,
                        int &angelScriptCode) {
    byteCode.clear();
    if (!module) {
        angelScriptCode = -1;
        return false;
    }
    MemoryByteCodeStream stream(&byteCode);
    angelScriptCode = module->SaveByteCode(&stream);
    if (angelScriptCode < 0) {
        byteCode.clear();
        return false;
    }
    return true;
}

bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        std::vector<unsigned char> &byteCode,
                        asIScriptModule **outModule,
                        int &angelScriptCode) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!engine || !moduleName || moduleName[0] == '\0') {
        angelScriptCode = -1;
        return false;
    }
    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        angelScriptCode = -1;
        return false;
    }
    MemoryByteCodeStream stream(&byteCode);
    angelScriptCode = module->LoadByteCode(&stream);
    if (angelScriptCode < 0) {
        module->Discard();
        return false;
    }
    if (outModule) {
        *outModule = module;
    }
    return true;
}

std::string MakeTransientModuleName(const char *moduleName) {
    static unsigned int counter = 0;
    return fmt::format("__ckas_replace_candidate_{}_{}", moduleName ? moduleName : "module", ++counter);
}

} // namespace ScriptManagerModuleReplacementInternal

namespace ScriptManagerInternal {

CKAS_STATUS ToCKAS_STATUS(ScriptInvocationStatus status) {
    switch (status) {
        case ScriptInvocationStatus::Finished:
            return CKAS_OK;
        case ScriptInvocationStatus::Suspended:
            return CKAS_SUSPENDED;
        case ScriptInvocationStatus::Failed:
        default:
            return CKAS_EXECUTIONFAILED;
    }
}

bool IsStringType(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return false;
    }
    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    return type && type->GetName() && std::strcmp(type->GetName(), "string") == 0;
}

bool IsValidStringParam(asIScriptEngine *engine, int typeId, asDWORD flags) {
    if (!IsStringType(engine, typeId)) {
        return false;
    }
    return (flags & asTM_OUTREF) == 0;
}

bool IsValidBorrowedObjectParam(int typeId, asDWORD flags) {
    if ((typeId & asTYPEID_OBJHANDLE) != 0) {
        return false;
    }
    if ((typeId & asTYPEID_MASK_OBJECT) == 0) {
        return false;
    }
    return (flags & asTM_INREF) != 0 &&
           (flags & asTM_CONST) != 0 &&
           (flags & asTM_OUTREF) == 0;
}

const char *StatusMessage(CKAS_STATUS status) {
    switch (status) {
        case CKAS_OK:
            return "OK.";
        case CKAS_INVALIDARGUMENT:
            return "Invalid argument.";
        case CKAS_NOTINITIALIZED:
            return "AngelScript engine is not initialized.";
        case CKAS_NOTFOUND:
            return "Requested script item was not found.";
        case CKAS_COMPILEERROR:
            return "Script compile failed.";
        case CKAS_EXECUTIONFAILED:
            return "Script execution failed.";
        case CKAS_SUSPENDED:
            return "Script execution suspended.";
        case CKAS_CANCELLED:
            return "Script execution was cancelled.";
        case CKAS_STALEHANDLE:
            return "Script handle is stale.";
        case CKAS_UNSUPPORTED:
            return "Operation is unsupported.";
        case CKAS_TYPEMISMATCH:
            return "Script type mismatch.";
        case CKAS_BUFFERTOOSMALL:
            return "Result buffer is too small.";
        case CKAS_INVALIDSTATE:
            return "Operation is invalid for the current handle state.";
        case CKAS_INUSE:
            return "Requested script item is in use.";
        case CKAS_ALREADYEXISTS:
            return "Requested script item already exists.";
        case CKAS_AMBIGUOUS:
            return "Requested script symbol is ambiguous.";
        case CKAS_FOREIGNHANDLE:
            return "Handle belongs to another CKAngelScript manager.";
        default:
            return "Unknown CKAngelScript status.";
    }
}

bool ValidateArgIndex(const CKAngelScriptArgWriter *writer, CKDWORD index) {
    return writer &&
           writer->Context &&
           writer->Method &&
           index < writer->Method->ParamTypes.size();
}

struct ObjectCallOutcome {
    CKAS_STATUS Status = CKAS_OK;
    int AngelScriptCode = 0;
    std::string ErrorMessage;
    std::string StackTrace;
};

ObjectCallOutcome ExecutePreparedObjectMethod(ScriptManager *manager,
                                              asIScriptObject *object,
                                              asIScriptFunction *function,
                                              const CKAngelScriptMethod *method,
                                              CKAngelScriptWriteArgsCallback writeArgs,
                                              CKAngelScriptReadResultCallback readResult,
                                              void *userData,
                                              CKDWORD flags) {
    ObjectCallOutcome outcome = {};
    if (!manager || !object || !function || !method) {
        outcome.Status = CKAS_INVALIDARGUMENT;
        outcome.ErrorMessage = "Object method execution arguments are invalid.";
        return outcome;
    }
    asIScriptEngine *engine = manager->GetScriptEngine();
    if (!engine) {
        outcome.Status = CKAS_NOTINITIALIZED;
        outcome.ErrorMessage = "AngelScript engine is not initialized.";
        return outcome;
    }

    asIScriptContext *ctx = engine->RequestContext();
    if (!ctx) {
        outcome.Status = CKAS_EXECUTIONFAILED;
        outcome.ErrorMessage = "Failed to create AngelScript context.";
        return outcome;
    }

    CKAS_STATUS status = CKAS_OK;
    int r = ctx->Prepare(function);
    if (r >= 0) {
        r = ctx->SetObject(object);
    }
    if (r < 0) {
        engine->ReturnContext(ctx);
        outcome.Status = CKAS_EXECUTIONFAILED;
        outcome.AngelScriptCode = r;
        outcome.ErrorMessage = "Failed to prepare script object method.";
        return outcome;
    }

    CKAngelScriptArgWriter writer = {};
    writer.Context = ctx;
    writer.Method = method;
    if (writeArgs) {
        status = writeArgs(&writer, userData);
        if (status != CKAS_OK) {
            engine->ReturnContext(ctx);
            outcome.Status = status;
            outcome.ErrorMessage = StatusMessage(status);
            return outcome;
        }
    }

    r = ctx->Execute();
    outcome.AngelScriptCode = r;
    if (r == asEXECUTION_FINISHED) {
        CKAngelScriptResultReader reader = {};
        reader.Context = ctx;
        reader.Method = method;
        if (readResult) {
            status = readResult(&reader, userData);
            if (status != CKAS_OK) {
                engine->ReturnContext(ctx);
                outcome.Status = status;
                outcome.ErrorMessage = StatusMessage(status);
                return outcome;
            }
        }
        engine->ReturnContext(ctx);
        outcome.Status = CKAS_OK;
        return outcome;
    }

    if (r == asEXECUTION_SUSPENDED) {
        ctx->Abort();
        engine->ReturnContext(ctx);
        outcome.Status = CKAS_UNSUPPORTED;
        outcome.ErrorMessage = "Suspended object method executions are not supported by this ABI path.";
        return outcome;
    }

    if (r == asEXECUTION_EXCEPTION) {
        const char *section = nullptr;
        int col = 0;
        const int row = ctx->GetExceptionLineNumber(&col, &section);
        asIScriptFunction *func = ctx->GetExceptionFunction();
        const char *funcDecl = func ? func->GetDeclaration() : nullptr;
        const char *exception = ctx->GetExceptionString();
        outcome.StackTrace = manager->GetCallStack(ctx);
        outcome.ErrorMessage = fmt::format("Exception in '{}' at {}({},{}): '{}'",
                                           funcDecl ? funcDecl : "<unknown function>",
                                           section ? section : "<unknown section>",
                                           row,
                                           col,
                                           exception ? exception : "");
    } else if (r == asEXECUTION_ABORTED) {
        outcome.ErrorMessage = "Script object method aborted.";
    } else {
        outcome.ErrorMessage = fmt::format("Script object method failed with result code: {}", r);
    }
    engine->ReturnContext(ctx);
    (void)flags;
    outcome.Status = CKAS_EXECUTIONFAILED;
    return outcome;
}

bool HasPublicFlag(CKDWORD flags, CKDWORD flag) {
    return (flags & flag) != 0;
}

template <typename T, typename M>
bool HasPublicField(const T &value, M T::*field) {
    const CKDWORD size = value.Size;
    const char *base = reinterpret_cast<const char *>(&value);
    const char *member = reinterpret_cast<const char *>(&(value.*field));
    const size_t offset = static_cast<size_t>(member - base);
    return size >= offset + sizeof(M);
}

template <typename T>
bool HasCompletePublicStruct(const T &value) {
    return value.Size >= static_cast<CKDWORD>(sizeof(T));
}

template <typename T, typename M>
M PublicField(const T &value, M T::*field, M fallback = M()) {
    return HasPublicField(value, field) ? value.*field : fallback;
}

CKAS_EXECUTIONSTATE ToExecutionState(ScriptInvocationStatus status) {
    switch (status) {
        case ScriptInvocationStatus::Finished:
            return CKAS_EXECUTION_FINISHED;
        case ScriptInvocationStatus::Suspended:
            return CKAS_EXECUTION_SUSPENDED;
        case ScriptInvocationStatus::Failed:
        default:
            return CKAS_EXECUTION_FAILED;
    }
}

CKAngelScriptResult MakeExecutionResult(CKAngelScriptExecution *execution,
                                      CKAS_STATUS status,
                                      int angelScriptCode = 0,
                                      const std::string &errorMessage = std::string(),
                                      const std::string &stackTrace = std::string()) {
    CKAngelScriptResult result;
    result.Size = sizeof(result);
    result.Status = status;
    result.AngelScriptCode = angelScriptCode;
    if (execution) {
        execution->ErrorMessage = errorMessage;
        execution->StackTrace = stackTrace;
        result.ErrorMessage = execution->ErrorMessage.empty() ? nullptr : execution->ErrorMessage.c_str();
        result.StackTrace = execution->StackTrace.empty() ? nullptr : execution->StackTrace.c_str();
        execution->Result = result;
    }
    return result;
}

CKAS_STATUS RunExecution(CKAngelScriptExecution *execution) {
    if (!execution) {
        return CKAS_INVALIDARGUMENT;
    }

    if (!execution->Manager ||
        !execution->Manager->HasModule(execution->ModuleName.c_str()) ||
        execution->Manager->GetModuleGeneration(execution->ModuleName.c_str()) != execution->ModuleGeneration) {
        MakeExecutionResult(execution, CKAS_STALEHANDLE, 0, "Function execution handle is stale.");
        return CKAS_STALEHANDLE;
    }

    execution->State = CKAS_EXECUTION_RUNNING;
    CKAS_STATUS callbackStatus = CKAS_OK;
    const ScriptInvocationStatus scriptStatus = execution->Invoker.ExecuteScriptStatus(
        execution->Function,
        [execution, &callbackStatus](asIScriptContext *ctx) {
            if (execution->HasBehaviorContext && execution->Function && execution->Function->GetParamCount() > 0) {
                const int argStatus = ctx->SetArgObject(0, (void *)&execution->BehaviorContextStorage);
                if (argStatus < 0) {
                    callbackStatus = CKAS_EXECUTIONFAILED;
                    return false;
                }
            }
            if (execution->ConfigureContext) {
                callbackStatus = execution->ConfigureContext(ctx, execution->UserData);
                if (callbackStatus != CKAS_OK) {
                    return false;
                }
            }
            return true;
        },
        [execution, &callbackStatus](asIScriptContext *ctx) {
            if (execution->ReadResult) {
                callbackStatus = execution->ReadResult(ctx, execution->UserData);
                return callbackStatus == CKAS_OK;
            }
            return true;
        });

    CKAS_STATUS status = ToCKAS_STATUS(scriptStatus);
    const int resultCode = execution->Invoker.GetLastResultCode();
    if (scriptStatus == ScriptInvocationStatus::Suspended &&
        HasPublicFlag(execution->Flags, CKAS_CALL_NO_SUSPEND)) {
        execution->Invoker.AbortContext();
        execution->State = CKAS_EXECUTION_FAILED;
        MakeExecutionResult(execution,
                            CKAS_UNSUPPORTED,
                            resultCode,
                            "Script execution suspended but CKAS_CALL_NO_SUSPEND was requested.");
        return CKAS_UNSUPPORTED;
    }
    if (callbackStatus != CKAS_OK) {
        execution->State = CKAS_EXECUTION_FAILED;
        MakeExecutionResult(execution, callbackStatus, resultCode, StatusMessage(callbackStatus));
        return callbackStatus;
    }
    execution->State = ToExecutionState(scriptStatus);
    if (status == CKAS_EXECUTIONFAILED) {
        MakeExecutionResult(execution,
                            status,
                            resultCode,
                            execution->Invoker.GetErrorMessage(),
                            execution->Invoker.GetStackTrace());
    } else {
        MakeExecutionResult(execution, status, resultCode);
    }
    return status;
}

} // namespace ScriptManagerInternal

using ScriptManagerInternal::ExecutePreparedObjectMethod;
using ScriptManagerInternal::HasCompletePublicStruct;
using ScriptManagerInternal::HasPublicField;
using ScriptManagerInternal::HasPublicFlag;
using ScriptManagerInternal::IsStringType;
using ScriptManagerInternal::IsValidBorrowedObjectParam;
using ScriptManagerInternal::IsValidStringParam;
using ScriptManagerInternal::MakeExecutionResult;
using ScriptManagerInternal::ObjectCallOutcome;
using ScriptManagerInternal::PublicField;
using ScriptManagerInternal::RunExecution;
using ScriptManagerInternal::StatusMessage;
using ScriptManagerInternal::ValidateArgIndex;

static ScriptManager *FromPublicHandle(CKAngelScript *angelScript) {
    return reinterpret_cast<ScriptManager *>(angelScript);
}

static const ScriptManager *FromPublicHandle(const CKAngelScript *angelScript) {
    return reinterpret_cast<const ScriptManager *>(angelScript);
}

static CKAngelScript *ToPublicHandle(ScriptManager *manager) {
    return reinterpret_cast<CKAngelScript *>(manager);
}

extern "C" CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context) {
    return ToPublicHandle(context ? ScriptManager::GetManager(context) : nullptr);
}

extern "C" CKAS_API CKDWORD CKAngelScriptGetApiVersion() {
    return CKAS_API_VERSION;
}

extern "C" CKAS_API CKBOOL CKAngelScriptHasFeature(CKAS_FEATURE feature) {
    switch (feature) {
        case CKAS_FEATURE_MODULE_LIFECYCLE:
        case CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS:
        case CKAS_FEATURE_FUNCTION_HANDLE:
        case CKAS_FEATURE_FUNCTION_EXECUTION:
        case CKAS_FEATURE_FUNCTION_EXECUTION_RESUME:
        case CKAS_FEATURE_OBJECT_HANDLE:
        case CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL:
        case CKAS_FEATURE_TYPED_ARG_READER_WRITER:
        case CKAS_FEATURE_STACK_TRACE:
        case CKAS_FEATURE_ENGINE_EXTENSION:
            return TRUE;
        default:
            return FALSE;
    }
}

extern "C" CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetVersion() : nullptr;
}

extern "C" CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetOptions() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowEngine(CKAngelScript *angelScript,
                                                          asIScriptEngine **outEngine,
                                                          CKAngelScriptResult *result) {
    if (outEngine) {
        *outEngine = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowEngine(outEngine, result) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowActiveContext(CKAngelScript *angelScript,
                                                                 asIScriptContext **outContext,
                                                                 CKAngelScriptResult *result) {
    if (outContext) {
        *outContext = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowActiveContext(outContext, result) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                                             const CKAngelScriptLoadOptions *options,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options ? scriptManager->LoadModule(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "LoadModule options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCompileModule(CKAngelScript *angelScript,
                                                                const char *moduleName,
                                                                const char *scriptCode,
                                                                CKDWORD flags,
                                                                CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->CompileModule(moduleName, scriptCode, flags, result)
               : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnloadModule(CKAngelScript *angelScript,
                                                               const char *moduleName,
                                                               CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnloadModule(moduleName, result) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKBOOL CKAngelScriptHasModule(CKAngelScript *angelScript, const char *moduleName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && scriptManager->HasModule(moduleName) ? TRUE : FALSE;
}

extern "C" CKAS_API CKDWORD CKAngelScriptGetModuleGeneration(CKAngelScript *angelScript,
                                                             const char *moduleName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetModuleGeneration(moduleName) : 0;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowModule(CKAngelScript *angelScript,
                                                          const char *moduleName,
                                                          asIScriptModule **outModule,
                                                          CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowModule(moduleName, outModule, result) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByName(CKAngelScript *angelScript,
                                                                  const char *moduleName,
                                                                  const char *functionName,
                                                                  asIScriptFunction **outFunction,
                                                                  CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowFunctionByName(moduleName, functionName, outFunction, result)
                         : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByDecl(CKAngelScript *angelScript,
                                                                  const char *moduleName,
                                                                  const char *functionDecl,
                                                                  asIScriptFunction **outFunction,
                                                                  CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowFunctionByDecl(moduleName, functionDecl, outFunction, result)
                         : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptFindFunction(CKAngelScript *angelScript,
                                                          const CKAngelScriptFunctionOptions *options,
                                                          CKAngelScriptFunction **outFunction,
                                                          CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options ? scriptManager->FindFunction(*options, outFunction, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "FindFunction options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseFunction(CKAngelScript *angelScript,
                                                             CKAngelScriptFunction *function) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseFunction(function) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateObject(CKAngelScript *angelScript,
                                                          const CKAngelScriptObjectOptions *options,
                                                          CKAngelScriptObject **outObject,
                                                          CKAngelScriptResult *result) {
    if (outObject) {
        *outObject = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options ? scriptManager->CreateObject(*options, outObject, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CreateObject options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                           CKAngelScriptObject *object) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseObject(object) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptFindObjectMethod(CKAngelScript *angelScript,
                                                              const CKAngelScriptMethodOptions *options,
                                                              CKAngelScriptMethod **outMethod,
                                                              CKAngelScriptResult *result) {
    if (outMethod) {
        *outMethod = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options ? scriptManager->FindObjectMethod(*options, outMethod, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "FindObjectMethod options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                           CKAngelScriptMethod *method) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseMethod(method) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options ? scriptManager->CallObjectMethod(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CallObjectMethod options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetBool(CKAngelScriptArgWriter *writer,
                                                        CKDWORD index,
                                                        CKBOOL value) {
    if (!ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (writer->Method->ParamTypes[index] != asTYPEID_BOOL) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgByte(static_cast<asUINT>(index), value ? 1 : 0);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetInt(CKAngelScriptArgWriter *writer,
                                                       CKDWORD index,
                                                       int value) {
    if (!ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (writer->Method->ParamTypes[index] != asTYPEID_INT32) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgDWord(static_cast<asUINT>(index), static_cast<asDWORD>(value));
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetFloat(CKAngelScriptArgWriter *writer,
                                                         CKDWORD index,
                                                         float value) {
    if (!ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (writer->Method->ParamTypes[index] != asTYPEID_FLOAT) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgFloat(static_cast<asUINT>(index), value);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetString(CKAngelScriptArgWriter *writer,
                                                          CKDWORD index,
                                                          const char *value) {
    if (!ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = writer->Context->GetEngine();
    if (!IsValidStringParam(engine, writer->Method->ParamTypes[index], writer->Method->ParamFlags[index])) {
        return CKAS_TYPEMISMATCH;
    }
    CKAngelScriptStringArg *stringArg = nullptr;
    for (CKAngelScriptStringArg &arg : writer->StringArgs) {
        if (arg.Index == index) {
            stringArg = &arg;
            break;
        }
    }
    if (!stringArg) {
        if (writer->StringArgs.capacity() == 0) {
            writer->StringArgs.reserve(writer->Method->ParamTypes.size());
        }
        writer->StringArgs.push_back(CKAngelScriptStringArg());
        stringArg = &writer->StringArgs.back();
        stringArg->Index = index;
    }
    stringArg->Value = value ? value : "";
    const int r = writer->Context->SetArgObject(static_cast<asUINT>(index), &stringArg->Value);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetBorrowedObject(CKAngelScriptArgWriter *writer,
                                                                  CKDWORD index,
                                                                  void *object) {
    if (!ValidateArgIndex(writer, index) || !object) {
        return CKAS_INVALIDARGUMENT;
    }
    if (!IsValidBorrowedObjectParam(writer->Method->ParamTypes[index], writer->Method->ParamFlags[index])) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgObject(static_cast<asUINT>(index), object);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetBool(CKAngelScriptResultReader *reader,
                                                           CKBOOL *value) {
    if (value) {
        *value = FALSE;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    if (reader->Method->ReturnType != asTYPEID_BOOL) {
        return CKAS_TYPEMISMATCH;
    }
    *value = reader->Context->GetReturnByte() ? TRUE : FALSE;
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetInt(CKAngelScriptResultReader *reader,
                                                          int *value) {
    if (value) {
        *value = 0;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    if (reader->Method->ReturnType != asTYPEID_INT32) {
        return CKAS_TYPEMISMATCH;
    }
    *value = static_cast<int>(reader->Context->GetReturnDWord());
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetFloat(CKAngelScriptResultReader *reader,
                                                            float *value) {
    if (value) {
        *value = 0.0f;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    if (reader->Method->ReturnType != asTYPEID_FLOAT) {
        return CKAS_TYPEMISMATCH;
    }
    *value = reader->Context->GetReturnFloat();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetStringView(CKAngelScriptResultReader *reader,
                                                                 const char **data,
                                                                 size_t *size) {
    if (data) {
        *data = nullptr;
    }
    if (size) {
        *size = 0;
    }
    if (!reader || !reader->Context || !reader->Method || !data || !size) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = reader->Context->GetEngine();
    if (!IsStringType(engine, reader->Method->ReturnType)) {
        return CKAS_TYPEMISMATCH;
    }
    auto *value = static_cast<std::string *>(reader->Context->GetReturnObject());
    if (!value) {
        *data = nullptr;
        *size = 0;
        return CKAS_OK;
    }
    *data = value->data();
    *size = value->size();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetString(CKAngelScriptResultReader *reader,
                                                             char *buffer,
                                                             size_t bufferSize,
                                                             size_t *outRequiredSize) {
    if (outRequiredSize) {
        *outRequiredSize = 0;
    }
    if (buffer && bufferSize > 0) {
        buffer[0] = '\0';
    }
    const char *data = nullptr;
    size_t size = 0;
    const CKAS_STATUS status = CKAngelScriptResultGetStringView(reader, &data, &size);
    if (status != CKAS_OK) {
        return status;
    }
    const size_t required = size + 1;
    if (outRequiredSize) {
        *outRequiredSize = required;
    }
    if (!buffer || bufferSize < required) {
        return CKAS_BUFFERTOOSMALL;
    }
    if (size > 0 && data) {
        std::memcpy(buffer, data, size);
    }
    buffer[size] = '\0';
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateFunctionExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionExecutionOptions *options,
    CKAngelScriptExecution **outExecution,
    CKAngelScriptResult *result) {
    if (outExecution) {
        *outExecution = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return options
               ? scriptManager->CreateFunctionExecution(*options, outExecution, result)
               : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CreateFunctionExecution options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                                 CKAngelScriptExecution *execution) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->StartExecution(execution) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                                  CKAngelScriptExecution *execution) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ResumeExecution(execution) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                                  CKAngelScriptExecution *execution) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->CancelExecution(execution) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                              CKAngelScriptExecution *execution) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseExecution(execution) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                               const CKAngelScriptExecution *execution,
                                                               CKAS_EXECUTIONSTATE *outState) {
    if (outState) {
        *outState = CKAS_EXECUTION_FAILED;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetExecutionState(execution, outState) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowExecutionResult(CKAngelScript *angelScript,
                                                                   const CKAngelScriptExecution *execution,
                                                                   const CKAngelScriptResult **outResult) {
    if (outResult) {
        *outResult = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowExecutionResult(execution, outResult) : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript) {
    const ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetLastResult() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                                          const CKAngelScriptEngineExtension *extension,
                                                                          CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    return extension
               ? scriptManager->RegisterEngineExtension(*extension, result)
               : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                                       const char *name,
                                                                       CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnregisterEngineExtension(name, result)
                         : CKAS_INVALIDARGUMENT;
}

ScriptManager::ScriptManager(CKContext *context) : CKBaseManager(context, SCRIPT_MANAGER_GUID, (CKSTRING) "AngelScript Manager") {
    int r = Init();
    if (r < 0)
        return;

    context->RegisterNewManager(this);
}

ScriptManager::~ScriptManager() {
    Shutdown();
}

CKStateChunk *ScriptManager::SaveData(CKFile *SavedFile) {
    return nullptr;
}

CKERROR ScriptManager::LoadData(CKStateChunk *chunk, CKFile *LoadedFile) {
    return CK_OK;
}

CKERROR ScriptManager::PostClearAll() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    ClearCKObjectData();
    return CK_OK;
}

CKERROR ScriptManager::PreProcess() {
#if CKAS_BUILD_SELF_TESTS
    const CKERROR selfTestResult = RunScriptStartupSelfTests(this);
    if (selfTestResult != CK_OK) {
        return selfTestResult;
    }
#endif
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Tick();
    }
    if (m_MessageBus) {
        m_MessageBus->Tick();
    }
    if (m_Runtime) {
        m_Runtime->PreProcess();
    }
    return m_BehaviorBridge ? m_BehaviorBridge->PreProcess() : CK_OK;
}

CKERROR ScriptManager::PostProcess() {
    if (m_Runtime) {
        m_Runtime->PostProcess();
    }
    return m_BehaviorBridge ? m_BehaviorBridge->PostProcess() : CK_OK;
}

CKERROR ScriptManager::OnCKInit() {
    return CK_OK;
}

CKERROR ScriptManager::OnCKEnd() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnEnd();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    ClearComponentStates();
    return CK_OK;
}

CKERROR ScriptManager::OnCKReset() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnReset();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPause() {
    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_Runtime) {
        m_Runtime->OnPause();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnCKPlay() {
    if (m_Runtime) {
        m_Runtime->OnResume();
    }
    return CK_OK;
}

CKERROR ScriptManager::PostLoad() {
    if (m_Runtime) {
        m_Runtime->PostLoad();
    }
    return CK_OK;
}

CKERROR ScriptManager::OnPostCopy(CKDependenciesContext &context) {
    return CK_OK;
}

int ScriptManager::Init() {
    if (IsInited())
        return -2;

    if (!m_ParameterRegistry) {
        m_ParameterRegistry = std::make_unique<ScriptParameterRegistry>(m_Context);
    }

    int r = SetupScriptEngine();
    if (r < 0)
        return r;

    if (!m_Runtime) {
        m_Runtime = std::make_unique<ScriptRuntime>(this);
    }
    if (!m_AsyncScheduler) {
        m_AsyncScheduler = std::make_unique<ScriptAsyncScheduler>(this);
    }
    if (!m_MessageBus) {
        m_MessageBus = std::make_unique<ScriptMessageBus>(this);
    }

    m_Flags |= AS_INITED;
    return r;
}

int ScriptManager::Shutdown() {
    if (!IsInited())
        return -2;

    for (auto *execution : m_Executions) {
        delete execution;
    }
    m_Executions.clear();

    for (auto *function : m_Functions) {
        delete function;
    }
    m_Functions.clear();

    for (auto *method : m_Methods) {
        delete method;
    }
    m_Methods.clear();

    for (auto *object : m_Objects) {
        if (object && object->Object) {
            object->Object->Release();
            object->Object = nullptr;
        }
        delete object;
    }
    m_Objects.clear();

    if (m_AsyncScheduler) {
        m_AsyncScheduler->Clear();
    }
    if (m_MessageBus) {
        m_MessageBus->Clear();
    }
    if (m_BehaviorBridge) {
        m_BehaviorBridge->Clear();
    }
    if (m_Runtime) {
        m_Runtime->Clear();
    }
    ClearComponentStates();

    for (auto *context : m_ScriptContexts) {
        context->Release();
    }
    m_ScriptContexts.clear();

    ClearCKObjectData();
    m_ScriptCache.Clear();

    m_BehaviorBridge.reset();
    m_Runtime.reset();
    m_AsyncScheduler.reset();
    m_ParameterRegistry.reset();

    if (m_ScriptEngine) {
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
    }

    m_Flags &= ~AS_INITED;
    return 0;
}

const char *ScriptManager::GetVersion() {
    return asGetLibraryVersion();
}

const char *ScriptManager::GetOptions() {
    return asGetLibraryOptions();
}

CKAS_STATUS ScriptManager::BorrowEngine(asIScriptEngine **outEngine, CKAngelScriptResult *result) {
    if (outEngine) {
        *outEngine = nullptr;
    }
    if (!outEngine) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine out pointer is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    *outEngine = m_ScriptEngine;
    return StoreResult(result, CKAS_OK);
}

asIScriptContext *ScriptManager::GetActiveContext() {
    return asGetActiveContext();
}

CKAS_STATUS ScriptManager::BorrowActiveContext(asIScriptContext **outContext, CKAngelScriptResult *result) {
    if (outContext) {
        *outContext = nullptr;
    }
    if (!outContext) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Active context out pointer is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    *outContext = ctx;
    return StoreResult(result, CKAS_OK);
}

CKAngelScriptResult ScriptManager::MakeResult(CKAS_STATUS status,
                                            int angelScriptCode,
                                            const std::string &errorMessage,
                                            const std::string &stackTrace) {
    m_LastErrorMessage = errorMessage;
    m_LastStackTrace = stackTrace;

    CKAngelScriptResult result;
    result.Size = sizeof(result);
    result.Status = status;
    result.AngelScriptCode = angelScriptCode;
    result.ErrorMessage = m_LastErrorMessage.empty() ? nullptr : m_LastErrorMessage.c_str();
    result.StackTrace = m_LastStackTrace.empty() ? nullptr : m_LastStackTrace.c_str();
    m_LastResult = result;
    return m_LastResult;
}

CKAS_STATUS ScriptManager::StoreResult(CKAngelScriptResult *out,
                                             CKAS_STATUS status,
                                             int angelScriptCode,
                                             const std::string &errorMessage,
                                             const std::string &stackTrace) {
    CKAngelScriptResult result = MakeResult(status, angelScriptCode, errorMessage, stackTrace);
    if (out) {
        *out = result;
    }
    return status;
}

const CKAngelScriptResult *ScriptManager::GetLastResult() const {
    return &m_LastResult;
}

CKAS_STATUS ScriptManager::StoreApiResult(CKAngelScriptResult *out,
                                          CKAS_STATUS status,
                                          int angelScriptCode,
                                          const char *errorMessage,
                                          const char *stackTrace) {
    return StoreResult(out,
                       status,
                       angelScriptCode,
                       std::string(errorMessage ? errorMessage : ""),
                       std::string(stackTrace ? stackTrace : ""));
}

CKAS_STATUS ScriptManager::RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                                         CKAngelScriptResult *result) {
    if (!HasCompletePublicStruct(extension)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension size is invalid.");
    }
    const char *name = PublicField(extension, &CKAngelScriptEngineExtension::Name, static_cast<const char *>(nullptr));
    CKAngelScriptEngineExtensionCallback callback =
        PublicField(extension, &CKAngelScriptEngineExtension::Register, static_cast<CKAngelScriptEngineExtensionCallback>(nullptr));
    void *userData = PublicField(extension, &CKAngelScriptEngineExtension::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = PublicField(extension,
                                      &CKAngelScriptEngineExtension::Flags,
                                      static_cast<CKDWORD>(CKAS_ENGINEEXTENSION_DEFAULT));

    if (!name || name[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension callback is required.");
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_ENGINEEXTENSION_DEFERRED)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown engine extension flags.");
    }
    for (const ScriptEngineExtensionRegistration &existing : m_EngineExtensions) {
        if (existing.Name == name) {
            return StoreResult(result,
                               CKAS_ALREADYEXISTS,
                               0,
                               fmt::format("Engine extension '{}' is already registered.", name));
        }
    }

    // Retain the extension first so it is replayed on the next engine rebuild
    // regardless of whether the immediate invocation below succeeds. This keeps
    // the registration path consistent with the setup/rebuild path, where a
    // failing extension is logged but never removes the others or tears the
    // engine down.
    ScriptEngineExtensionRegistration retained = {};
    retained.Name = name;
    retained.Register = callback;
    retained.UserData = userData;
    retained.Flags = flags;
    m_EngineExtensions.push_back(retained);

    if (m_ScriptEngine && IsInited() && !HasPublicFlag(flags, CKAS_ENGINEEXTENSION_DEFERRED)) {
        const char *extensionError = nullptr;
        const int code = callback(m_ScriptEngine, ToPublicHandle(this), userData, &extensionError);
        if (code < 0) {
            const std::string summary =
                extensionError && extensionError[0] != '\0'
                    ? fmt::format("Engine extension '{}' failed to register (code {}): {}", name, code, extensionError)
                    : fmt::format("Engine extension '{}' failed to register (code {}).", name, code);
            if (m_Context) {
                m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            // Non-fatal: the extension stays registered for the next rebuild,
            // but the caller is told this immediate attempt failed. Functions
            // the callback registered before failing may remain in the current
            // engine until the next rebuild.
            return StoreResult(result, CKAS_EXECUTIONFAILED, code, summary);
        }
    }

    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnregisterEngineExtension(const char *name,
                                                     CKAngelScriptResult *result) {
    if (!name || name[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    for (auto it = m_EngineExtensions.begin(); it != m_EngineExtensions.end(); ++it) {
        if (it->Name == name) {
            m_EngineExtensions.erase(it);
            return StoreResult(result, CKAS_OK);
        }
    }
    return StoreResult(result,
                       CKAS_NOTFOUND,
                       0,
                       fmt::format("Engine extension '{}' is not registered.", name));
}

void ScriptManager::BeginScriptMessageCapture() {
    m_CapturedScriptMessages.clear();
    m_CapturingScriptMessages = true;
}

std::string ScriptManager::EndScriptMessageCapture() {
    m_CapturingScriptMessages = false;
    return m_CapturedScriptMessages;
}

bool ScriptManager::OwnsExecution(const CKAngelScriptExecution *execution) const {
    return execution && m_Executions.find(const_cast<CKAngelScriptExecution *>(execution)) != m_Executions.end();
}

bool ScriptManager::OwnsFunction(const CKAngelScriptFunction *function) const {
    return function && m_Functions.find(const_cast<CKAngelScriptFunction *>(function)) != m_Functions.end();
}

bool ScriptManager::OwnsObject(const CKAngelScriptObject *object) const {
    return object && m_Objects.find(const_cast<CKAngelScriptObject *>(object)) != m_Objects.end();
}

bool ScriptManager::OwnsMethod(const CKAngelScriptMethod *method) const {
    return method && m_Methods.find(const_cast<CKAngelScriptMethod *>(method)) != m_Methods.end();
}

bool ScriptManager::HasExecutionForModule(const char *moduleName) const {
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    for (const CKAngelScriptExecution *execution : m_Executions) {
        if (execution && execution->ModuleName == moduleName) {
            return true;
        }
    }
    return false;
}

bool ScriptManager::HasRuntimeHandleForModule(const char *moduleName) const {
    if (HasExecutionForModule(moduleName)) {
        return true;
    }
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    for (const CKAngelScriptObject *object : m_Objects) {
        if (object && object->ModuleName == moduleName) {
            return true;
        }
    }
    return false;
}

void ScriptManager::BumpModuleGeneration(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    CKDWORD &generation = m_ModuleGenerations[moduleName];
    if (generation == 0) {
        generation = 1;
    } else {
        ++generation;
    }
}

std::shared_ptr<CachedScript> ScriptManager::BuildTransientModule(
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    int &angelScriptCode,
    std::string &diagnostics) {
    angelScriptCode = 0;
    diagnostics.clear();
    if (!m_ScriptEngine || !moduleName || moduleName[0] == '\0' || sections.empty()) {
        angelScriptCode = -1;
        return nullptr;
    }

    auto script = std::make_shared<CachedScript>();
    script->name = moduleName;
    for (const auto &section : sections) {
        script->AddSection(std::get<0>(section), std::get<1>(section));
    }

    BeginScriptMessageCapture();
    const bool built = script->Build(m_ScriptEngine);
    diagnostics = EndScriptMessageCapture();
    if (!built) {
        angelScriptCode = -3;
        if (script->module) {
            script->Discard();
        }
        return nullptr;
    }
    return script;
}

bool ScriptManager::CaptureModuleReplacementSnapshot(const char *moduleName,
                                                     ModuleReplacementSnapshot &snapshot,
                                                     int &angelScriptCode,
                                                     std::string &errorMessage) {
    snapshot = ModuleReplacementSnapshot();
    angelScriptCode = 0;
    errorMessage.clear();
    if (!moduleName || moduleName[0] == '\0') {
        angelScriptCode = -1;
        errorMessage = "Module name is required.";
        return false;
    }

    snapshot.Cache = m_ScriptCache.GetCachedScript(moduleName);
    if (snapshot.Cache) {
        snapshot.Sections = snapshot.Cache->sections;
        snapshot.Metadata = snapshot.Cache->metadata;
    }

    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return true;
    }

    snapshot.HasModule = true;
    if (!ScriptManagerModuleReplacementInternal::SaveModuleByteCode(module, snapshot.ByteCode, angelScriptCode)) {
        errorMessage = "Failed to snapshot existing script module bytecode.";
        return false;
    }
    return true;
}

void ScriptManager::RemoveModuleForReplacement(const char *moduleName,
                                               ModuleReplacementSnapshot &snapshot) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }

    m_ScriptCache.Invalidate(moduleName);
    if (snapshot.Cache && snapshot.Cache->module) {
        snapshot.Cache->module->Discard();
        snapshot.Cache->module = nullptr;
        return;
    }

    asIScriptModule *module = GetModule(moduleName);
    if (module) {
        module->Discard();
    }
}

bool ScriptManager::RestoreModuleReplacementSnapshot(const char *moduleName,
                                                     ModuleReplacementSnapshot &snapshot,
                                                     int &angelScriptCode,
                                                     std::string &errorMessage) {
    angelScriptCode = 0;
    errorMessage.clear();
    if (!snapshot.HasModule) {
        return true;
    }

    asIScriptModule *restoredModule = nullptr;
    if (!ScriptManagerModuleReplacementInternal::LoadModuleByteCode(m_ScriptEngine,
                                                                    moduleName,
                                                                    snapshot.ByteCode,
                                                                    &restoredModule,
                                                                    angelScriptCode)) {
        errorMessage = "Failed to restore previous script module bytecode.";
        return false;
    }

    std::shared_ptr<CachedScript> restoredCache = snapshot.Cache ? snapshot.Cache : std::make_shared<CachedScript>();
    restoredCache->name = moduleName ? moduleName : "";
    restoredCache->sections = snapshot.Sections;
    restoredCache->metadata = snapshot.Metadata;
    restoredCache->module = restoredModule;
    m_ScriptCache.CacheScript(moduleName, restoredCache);
    return true;
}

CKAS_STATUS ScriptManager::ReplaceModuleFromSections(
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    CKAngelScriptResult *result) {
    int angelScriptCode = 0;
    std::string diagnostics;
    const std::string transientName = ScriptManagerModuleReplacementInternal::MakeTransientModuleName(moduleName);
    std::shared_ptr<CachedScript> candidate =
        BuildTransientModule(transientName.c_str(), sections, angelScriptCode, diagnostics);
    if (!candidate || !candidate->module) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           angelScriptCode,
                           diagnostics.empty() ? "Failed to compile replacement script module." : diagnostics);
    }

    std::vector<unsigned char> candidateByteCode;
    if (!ScriptManagerModuleReplacementInternal::SaveModuleByteCode(candidate->module,
                                                                    candidateByteCode,
                                                                    angelScriptCode)) {
        candidate->Discard();
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           "Failed to snapshot replacement script module bytecode.");
    }

    ModuleReplacementSnapshot snapshot;
    std::string snapshotError;
    if (!CaptureModuleReplacementSnapshot(moduleName, snapshot, angelScriptCode, snapshotError)) {
        candidate->Discard();
        return StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, snapshotError);
    }

    RemoveModuleForReplacement(moduleName, snapshot);

    asIScriptModule *committedModule = nullptr;
    if (!ScriptManagerModuleReplacementInternal::LoadModuleByteCode(m_ScriptEngine,
                                                                    moduleName,
                                                                    candidateByteCode,
                                                                    &committedModule,
                                                                    angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored = RestoreModuleReplacementSnapshot(moduleName, snapshot, restoreCode, restoreError);
        candidate->Discard();
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           restored
                               ? "Failed to commit replacement script module bytecode."
                               : fmt::format("Failed to commit replacement script module bytecode; rollback also failed: {}",
                                             restoreError));
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = moduleName ? moduleName : "";
    committedCache->sections = candidate->sections;
    committedCache->metadata = candidate->metadata;
    committedCache->module = committedModule;
    m_ScriptCache.CacheScript(moduleName, committedCache);
    candidate->Discard();
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
    if (!HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "LoadModule options size is invalid.");
    }
    const char *moduleName = PublicField(options, &CKAngelScriptLoadOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *filename = PublicField(options, &CKAngelScriptLoadOptions::Filename, static_cast<const char *>(nullptr));
    const char **filenames = PublicField(options, &CKAngelScriptLoadOptions::Filenames, static_cast<const char **>(nullptr));
    const size_t fileCount = PublicField(options, &CKAngelScriptLoadOptions::FileCount, static_cast<size_t>(0));
    const char *code = PublicField(options, &CKAngelScriptLoadOptions::Code, static_cast<const char *>(nullptr));
    const CKDWORD flags = PublicField(options,
                                      &CKAngelScriptLoadOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_LOAD_DEFAULT));

    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_LOAD_REPLACEEXISTING)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown LoadModule flags.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool hasCode = code != nullptr;
    const bool hasFile = filename && filename[0] != '\0';
    const bool hasFiles = fileCount > 0;
    const int sourceCount = (hasCode ? 1 : 0) + (hasFile ? 1 : 0) + (hasFiles ? 1 : 0);
    if (sourceCount > 1) {
        return StoreResult(result,
                           CKAS_INVALIDARGUMENT,
                           0,
                           "LoadModule accepts only one source: Code, Filename, or Filenames.");
    }
    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!HasPublicFlag(flags, CKAS_LOAD_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        if (HasRuntimeHandleForModule(moduleName)) {
            return StoreResult(result,
                               CKAS_INUSE,
                               0,
                               "Module has live object or execution handles.");
        }
    }
    if (hasCode) {
        return CompileModule(moduleName, code, CKAS_COMPILE_REPLACEEXISTING, result);
    }
    if (hasFiles) {
        if (!filenames) {
            return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "File list is null.");
        }
        for (size_t i = 0; i < fileCount; ++i) {
            if (!filenames[i] || filenames[i][0] == '\0') {
                return StoreResult(result,
                                   CKAS_INVALIDARGUMENT,
                                   0,
                                   "File list contains an empty filename.");
            }
        }
        if (replacingExisting) {
            std::vector<std::tuple<std::string, std::string>> sections;
            sections.reserve(fileCount);
            for (size_t i = 0; i < fileCount; ++i) {
                XString scriptFilename = filenames[i];
                if (scriptFilename.Find(".as") == XString::NOTFOUND) {
                    scriptFilename += ".as";
                }
                ResolveScriptFileName(scriptFilename);
                sections.emplace_back(scriptFilename.CStr(), std::string());
            }
            return ReplaceModuleFromSections(moduleName, sections, result);
        }
        BeginScriptMessageCapture();
        const int loadResult = LoadModuleFromFiles(moduleName, filenames, fileCount);
        const std::string diagnostics = EndScriptMessageCapture();
        if (loadResult < 0) {
            return StoreResult(result,
                               CKAS_COMPILEERROR,
                               loadResult,
                               diagnostics.empty() ? "Failed to load script files." : diagnostics);
        }
        BumpModuleGeneration(moduleName);
        return StoreResult(result, CKAS_OK);
    }

    if (replacingExisting) {
        std::string scriptFilename;
        if (filename) {
            scriptFilename = filename;
        } else {
            scriptFilename = moduleName;
            scriptFilename += ".as";
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(std::move(scriptFilename), std::string());
        return ReplaceModuleFromSections(moduleName, sections, result);
    }

    BeginScriptMessageCapture();
    const int loadResult = LoadModuleFromDefaultOrFile(moduleName, filename);
    const std::string diagnostics = EndScriptMessageCapture();
    if (loadResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           loadResult,
                           diagnostics.empty() ? "Failed to load script file." : diagnostics);
    }
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                               const char *scriptCode,
                                               CKDWORD flags,
                                               CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0' || !scriptCode) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and script code are required.");
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_COMPILE_REPLACEEXISTING)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown CompileModule flags.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        if (HasRuntimeHandleForModule(moduleName)) {
            return StoreResult(result,
                               CKAS_INUSE,
                               0,
                               "Module has live object or execution handles.");
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(moduleName, scriptCode);
        return ReplaceModuleFromSections(moduleName, sections, result);
    }

    BeginScriptMessageCapture();
    const int compileResult = CompileModuleFromMemory(moduleName, scriptCode);
    const std::string diagnostics = EndScriptMessageCapture();
    if (compileResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           compileResult,
                           diagnostics.empty() ? "Failed to compile script module." : diagnostics);
    }
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (HasRuntimeHandleForModule(moduleName)) {
        return StoreResult(result,
                           CKAS_INUSE,
                           0,
                           "Module has live object or execution handles.");
    }
    if (!DiscardCachedModule(moduleName)) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not loaded.");
    }
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::HasModule(const char *moduleName) {
    return GetModule(moduleName) != nullptr;
}

CKDWORD ScriptManager::GetModuleGeneration(const char *moduleName) const {
    if (!moduleName || moduleName[0] == '\0') {
        return 0;
    }
    const auto it = m_ModuleGenerations.find(moduleName);
    return it == m_ModuleGenerations.end() ? 0 : it->second;
}

asIScriptModule *ScriptManager::GetModule(const char *moduleName) {
    return GetScript(moduleName);
}

CKAS_STATUS ScriptManager::BorrowModule(const char *moduleName,
                                        asIScriptModule **outModule,
                                        CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!outModule) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module out pointer is required.");
    }
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    *outModule = module;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByName(const char *moduleName,
                                                const char *functionName,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!functionName || functionName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *match = nullptr;
    asUINT matchCount = 0;
    const asUINT count = module->GetFunctionCount();
    for (asUINT i = 0; i < count; ++i) {
        asIScriptFunction *function = module->GetFunctionByIndex(i);
        const char *name = function ? function->GetName() : nullptr;
        if (name && std::strcmp(name, functionName) == 0) {
            match = function;
            ++matchCount;
        }
    }
    if (matchCount == 0) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    if (matchCount > 1) {
        return StoreResult(result, CKAS_AMBIGUOUS, 0, "Function name matched multiple overloads.");
    }
    *outFunction = match;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByDecl(const char *moduleName,
                                                const char *functionDecl,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!functionDecl || functionDecl[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function declaration is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *function = module->GetFunctionByDecl(functionDecl);
    if (!function) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::FindFunction(const CKAngelScriptFunctionOptions &options,
                                        CKAngelScriptFunction **outFunction,
                                        CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle out pointer is required.");
    }
    const char *moduleName = PublicField(options, &CKAngelScriptFunctionOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *functionName = PublicField(options, &CKAngelScriptFunctionOptions::FunctionName, static_cast<const char *>(nullptr));
    const char *functionDecl = PublicField(options, &CKAngelScriptFunctionOptions::FunctionDecl, static_cast<const char *>(nullptr));
    const CKDWORD flags = PublicField(options, &CKAngelScriptFunctionOptions::Flags, static_cast<CKDWORD>(0));
    const bool hasFunctionName = functionName && functionName[0] != '\0';
    const bool hasFunctionDecl = functionDecl && functionDecl[0] != '\0';
    if (flags != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown FindFunction flags.");
    }
    if (hasFunctionName == hasFunctionDecl) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exactly one of FunctionName or FunctionDecl is required.");
    }
    asIScriptFunction *scriptFunction = nullptr;
    const CKAS_STATUS status = hasFunctionDecl
                                   ? BorrowFunctionByDecl(moduleName, functionDecl, &scriptFunction, result)
                                   : BorrowFunctionByName(moduleName, functionName, &scriptFunction, result);
    if (status != CKAS_OK) {
        return status;
    }

    auto *function = new CKAngelScriptFunction();
    function->Manager = this;
    function->ModuleName = moduleName ? moduleName : "";
    function->FunctionName = scriptFunction->GetName() ? scriptFunction->GetName() : "";
    function->FunctionDecl = scriptFunction->GetDeclaration() ? scriptFunction->GetDeclaration() : "";
    function->ModuleGeneration = GetModuleGeneration(moduleName);
    m_Functions.insert(function);
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseFunction(CKAngelScriptFunction *function) {
    if (!function) {
        return StoreResult(nullptr, CKAS_OK);
    }
    if (function->Manager && function->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Function handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsFunction(function)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Function handle is invalid.");
    }
    m_Functions.erase(function);
    delete function;
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::CreateObject(const CKAngelScriptObjectOptions &options,
                                        CKAngelScriptObject **outObject,
                                        CKAngelScriptResult *result) {
    if (outObject) {
        *outObject = nullptr;
    }
    if (!outObject) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object out pointer is required.");
    }
    const char *moduleName = PublicField(options, &CKAngelScriptObjectOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *className = PublicField(options, &CKAngelScriptObjectOptions::ClassName, static_cast<const char *>(nullptr));
    if (!moduleName || moduleName[0] == '\0' || !className || className[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and class name are required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    asITypeInfo *type = module->GetTypeInfoByName(className);
    if (!type) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Script class was not found.");
    }
    auto *scriptObject = static_cast<asIScriptObject *>(m_ScriptEngine->CreateScriptObject(type));
    if (!scriptObject) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Failed to create script object.");
    }

    auto *object = new CKAngelScriptObject();
    object->Manager = this;
    object->Object = scriptObject;
    object->ModuleName = moduleName;
    object->ClassName = className;
    object->ModuleGeneration = GetModuleGeneration(moduleName);
    m_Objects.insert(object);
    *outObject = object;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseObject(CKAngelScriptObject *object) {
    if (!object) {
        return StoreResult(nullptr, CKAS_OK);
    }
    if (object->Manager && object->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Object handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsObject(object)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    m_Objects.erase(object);
    if (object->Object) {
        object->Object->Release();
        object->Object = nullptr;
    }
    delete object;
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                            CKAngelScriptMethod **outMethod,
                                            CKAngelScriptResult *result) {
    if (outMethod) {
        *outMethod = nullptr;
    }
    if (!outMethod) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle out pointer is required.");
    }
    CKAngelScriptObject *object = PublicField(options, &CKAngelScriptMethodOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    const char *methodName = PublicField(options, &CKAngelScriptMethodOptions::MethodName, static_cast<const char *>(nullptr));
    const char *methodDecl = PublicField(options, &CKAngelScriptMethodOptions::MethodDecl, static_cast<const char *>(nullptr));
    const bool hasMethodName = methodName && methodName[0] != '\0';
    const bool hasMethodDecl = methodDecl && methodDecl[0] != '\0';
    if (!object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is required.");
    }
    if (object->Manager && object->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Object handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsObject(object) || !object->Object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
    }
    if (hasMethodName == hasMethodDecl) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exactly one of MethodName or MethodDecl is required.");
    }
    if (!HasModule(object->ModuleName.c_str()) || GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object handle is stale.");
    }
    asITypeInfo *type = object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }

    asIScriptFunction *function = nullptr;
    if (hasMethodDecl) {
        function = type->GetMethodByDecl(methodDecl);
    } else {
        asUINT matchCount = 0;
        const asUINT count = type->GetMethodCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *candidate = type->GetMethodByIndex(i);
            const char *candidateName = candidate ? candidate->GetName() : nullptr;
            if (candidateName && std::strcmp(candidateName, methodName) == 0) {
                function = candidate;
                ++matchCount;
            }
        }
        if (matchCount > 1) {
            return StoreResult(result, CKAS_AMBIGUOUS, 0, "Method name matched multiple overloads.");
        }
    }
    if (!function) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Script object method was not found.");
    }

    auto *method = new CKAngelScriptMethod();
    method->Manager = this;
    method->ModuleName = object->ModuleName;
    method->ClassName = object->ClassName;
    method->MethodName = function->GetName() ? function->GetName() : "";
    method->MethodDecl = function->GetDeclaration(false) ? function->GetDeclaration(false) : "";
    method->ModuleGeneration = object->ModuleGeneration;
    const asUINT paramCount = function->GetParamCount();
    method->ParamTypes.resize(paramCount);
    method->ParamFlags.resize(paramCount);
    for (asUINT i = 0; i < paramCount; ++i) {
        int typeId = 0;
        asDWORD flags = 0;
        function->GetParam(i, &typeId, &flags);
        method->ParamTypes[i] = typeId;
        method->ParamFlags[i] = flags;
    }
    method->ReturnType = function->GetReturnTypeId(&method->ReturnFlags);
    m_Methods.insert(method);
    *outMethod = method;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseMethod(CKAngelScriptMethod *method) {
    if (!method) {
        return StoreResult(nullptr, CKAS_OK);
    }
    if (method->Manager && method->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Method handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsMethod(method)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Method handle is invalid.");
    }
    m_Methods.erase(method);
    delete method;
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                            CKAngelScriptResult *result) {
    if (!HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "CallObjectMethod options size is invalid.");
    }
    CKAngelScriptObject *object = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    CKAngelScriptMethod *method = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Method, static_cast<CKAngelScriptMethod *>(nullptr));
    const CKDWORD flags = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    const CKDWORD knownFlags = CKAS_CALL_NO_SUSPEND;
    if ((flags & ~knownFlags) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown object method call flags.");
    }
    if (!object || !method) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object and method handles are required.");
    }
    if ((object->Manager && object->Manager != this) || (method->Manager && method->Manager != this)) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Object or method handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsObject(object) || !OwnsMethod(method) || !object->Object) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object or method handle is invalid.");
    }
    if (object->ModuleName != method->ModuleName || object->ClassName != method->ClassName) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle does not belong to the object type.");
    }
    if (!HasModule(object->ModuleName.c_str()) ||
        GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration ||
        method->ModuleGeneration != object->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object or method handle is stale.");
    }
    asITypeInfo *type = object->Object->GetObjectType();
    if (!type) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
    }
    asIScriptFunction *function = type->GetMethodByDecl(method->MethodDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Object method handle is stale.");
    }
    CKAngelScriptWriteArgsCallback writeArgs =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::WriteArgs, static_cast<CKAngelScriptWriteArgsCallback>(nullptr));
    CKAngelScriptReadResultCallback readResult =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ReadResult, static_cast<CKAngelScriptReadResultCallback>(nullptr));
    void *userData = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::UserData, static_cast<void *>(nullptr));

    const ObjectCallOutcome outcome = ExecutePreparedObjectMethod(this,
                                                                  object->Object,
                                                                  function,
                                                                  method,
                                                                  writeArgs,
                                                                  readResult,
                                                                  userData,
                                                                  flags);
    return StoreResult(result, outcome.Status, outcome.AngelScriptCode, outcome.ErrorMessage, outcome.StackTrace);
}

CKAS_STATUS ScriptManager::CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                                   CKAngelScriptExecution **outExecution,
                                                   CKAngelScriptResult *result) {
    if (outExecution) {
        *outExecution = nullptr;
    }
    if (!outExecution) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Execution out pointer is required.");
    }
    CKAngelScriptFunction *functionHandle =
        PublicField(options, &CKAngelScriptFunctionExecutionOptions::Function, static_cast<CKAngelScriptFunction *>(nullptr));
    const CKBehaviorContext *behaviorContext =
        PublicField(options, &CKAngelScriptFunctionExecutionOptions::BehaviorContext, static_cast<const CKBehaviorContext *>(nullptr));
    CKAngelScriptContextCallback configureContext =
        PublicField(options, &CKAngelScriptFunctionExecutionOptions::ConfigureContext, static_cast<CKAngelScriptContextCallback>(nullptr));
    CKAngelScriptContextCallback readResult =
        PublicField(options, &CKAngelScriptFunctionExecutionOptions::ReadResult, static_cast<CKAngelScriptContextCallback>(nullptr));
    void *userData = PublicField(options, &CKAngelScriptFunctionExecutionOptions::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = PublicField(options, &CKAngelScriptFunctionExecutionOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    const CKDWORD knownFlags = CKAS_CALL_NO_SUSPEND;

    if ((flags & ~knownFlags) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown function execution flags.");
    }
    if (!functionHandle) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle is required.");
    }
    if (functionHandle->Manager && functionHandle->Manager != this) {
        return StoreResult(result, CKAS_FOREIGNHANDLE, 0, "Function handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsFunction(functionHandle)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function handle is invalid.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    if (!HasModule(functionHandle->ModuleName.c_str()) ||
        GetModuleGeneration(functionHandle->ModuleName.c_str()) != functionHandle->ModuleGeneration) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }
    asIScriptModule *module = GetModule(functionHandle->ModuleName.c_str());
    if (!module) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function module is no longer loaded.");
    }

    asIScriptFunction *function = functionHandle->FunctionDecl.empty()
                                      ? nullptr
                                      : module->GetFunctionByDecl(functionHandle->FunctionDecl.c_str());
    if (!function) {
        return StoreResult(result, CKAS_STALEHANDLE, 0, "Function handle is stale.");
    }

    auto *execution = new CKAngelScriptExecution(this);
    execution->ModuleName = functionHandle->ModuleName;
    execution->FunctionName = functionHandle->FunctionName;
    execution->FunctionDecl = functionHandle->FunctionDecl;
    execution->ModuleGeneration = functionHandle->ModuleGeneration;
    execution->ConfigureContext = configureContext;
    execution->ReadResult = readResult;
    execution->UserData = userData;
    execution->Flags = flags;
    if (behaviorContext) {
        execution->BehaviorContextStorage = *behaviorContext;
        execution->HasBehaviorContext = true;
    }
    function->AddRef();
    execution->Function = function;
    if (!execution->Invoker.SetScript(functionHandle->ModuleName.c_str())) {
        const std::string error = execution->Invoker.GetErrorMessage();
        delete execution;
        return StoreResult(result, CKAS_NOTFOUND, 0, error.empty() ? "Module cache was not found." : error);
    }

    MakeExecutionResult(execution, CKAS_OK);
    m_Executions.insert(execution);
    *outExecution = execution;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::StartExecution(CKAngelScriptExecution *execution) {
    if (!execution) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->State != CKAS_EXECUTION_READY) {
        MakeExecutionResult(execution, CKAS_INVALIDSTATE, 0, "Execution is not ready to start.");
        return StoreResult(nullptr, CKAS_INVALIDSTATE, 0, "Execution is not ready to start.");
    }
    const CKAS_STATUS status = RunExecution(execution);
    StoreResult(nullptr,
                status,
                execution->Result.AngelScriptCode,
                execution->ErrorMessage,
                execution->StackTrace);
    return status;
}

CKAS_STATUS ScriptManager::ResumeExecution(CKAngelScriptExecution *execution) {
    if (!execution) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->State != CKAS_EXECUTION_SUSPENDED) {
        MakeExecutionResult(execution, CKAS_INVALIDSTATE, 0, "Execution is not suspended.");
        return StoreResult(nullptr, CKAS_INVALIDSTATE, 0, "Execution is not suspended.");
    }
    const CKAS_STATUS status = RunExecution(execution);
    StoreResult(nullptr,
                status,
                execution->Result.AngelScriptCode,
                execution->ErrorMessage,
                execution->StackTrace);
    return status;
}

CKAS_STATUS ScriptManager::CancelExecution(CKAngelScriptExecution *execution) {
    if (!execution) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    execution->Invoker.AbortContext();
    execution->State = CKAS_EXECUTION_CANCELLED;
    MakeExecutionResult(execution, CKAS_CANCELLED);
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::ReleaseExecution(CKAngelScriptExecution *execution) {
    if (!execution) {
        return StoreResult(nullptr, CKAS_OK);
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    m_Executions.erase(execution);
    delete execution;
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetExecutionState(const CKAngelScriptExecution *execution,
                                             CKAS_EXECUTIONSTATE *outState) {
    if (outState) {
        *outState = CKAS_EXECUTION_FAILED;
    }
    if (!outState) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution state out pointer is required.");
    }
    if (!execution) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    *outState = execution->State;
    return StoreResult(nullptr, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                                 const CKAngelScriptResult **outResult) {
    if (outResult) {
        *outResult = nullptr;
    }
    if (!outResult) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution result out pointer is required.");
    }
    if (!execution) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->Manager && execution->Manager != this) {
        return StoreResult(nullptr, CKAS_FOREIGNHANDLE, 0, "Execution handle belongs to another CKAngelScript manager.");
    }
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    *outResult = &execution->Result;
    return StoreResult(nullptr, CKAS_OK);
}

int ScriptManager::PrepareMultithread(asIThreadManager *externalMgr) {
    return asPrepareMultithread(externalMgr);
}

void ScriptManager::UnprepareMultithread() {
    asUnprepareMultithread();
}

asIThreadManager *ScriptManager::GetThreadManager() {
    return asGetThreadManager();
}

void ScriptManager::AcquireExclusiveLock() {
    asAcquireExclusiveLock();
}

void ScriptManager::ReleaseExclusiveLock() {
    asReleaseExclusiveLock();
}

void ScriptManager::AcquireSharedLock() {
    asAcquireSharedLock();
}

void ScriptManager::ReleaseSharedLock() {
    asReleaseSharedLock();
}

int ScriptManager::AtomicInc(int &value) {
    return asAtomicInc(value);
}

int ScriptManager::AtomicDec(int &value) {
    return asAtomicDec(value);
}

int ScriptManager::ThreadCleanup() {
    return asThreadCleanup();
}

int ScriptManager::SetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc) {
    return asSetGlobalMemoryFunctions(allocFunc, freeFunc);
}

int ScriptManager::ResetGlobalMemoryFunctions() {
    return asResetGlobalMemoryFunctions();
}

void *ScriptManager::AllocMem(size_t size) {
    return asAllocMem(size);
}

void ScriptManager::FreeMem(void *mem) {
    asFreeMem(mem);
}

asILockableSharedBool *ScriptManager::CreateLockableSharedBool() {
    return asCreateLockableSharedBool();
}

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_ScriptEngine;
}

int ScriptManager::LoadModuleFromDefaultOrFile(const char *moduleName, const char *filename) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    XString scriptFilename;
    if (filename) {
        scriptFilename = filename;
    } else {
        scriptFilename = moduleName;
        scriptFilename += ".as";
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, moduleName, scriptFilename.CStr());
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::LoadModuleFromFiles(const char *moduleName, const char **filenames, size_t count) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    std::vector<std::string> files;
    for (size_t i = 0; i < count; i++) {
        XString scriptFilename = filenames[i];
        if (scriptFilename.Find(".as") == XString::NOTFOUND)
            scriptFilename += ".as";
        ResolveScriptFileName(scriptFilename);
        files.emplace_back(scriptFilename.CStr());
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, moduleName, files);
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::CompileModuleFromMemory(const char *moduleName, const char *scriptCode) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!scriptCode)
        return -1;

    if (!m_ScriptEngine)
        return -2;

    auto cache = m_ScriptCache.CompileScript(m_ScriptEngine, moduleName, scriptCode);
    if (!cache)
        return -3;
    return 0;
}

bool ScriptManager::DiscardCachedModule(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0')
        return false;
    return m_ScriptCache.UnloadScript(moduleName);
}

asIScriptModule *ScriptManager::GetScript(const char *scriptName) {
    if (!m_ScriptEngine)
        return nullptr;
    return m_ScriptEngine->GetModule(scriptName, asGM_ONLY_IF_EXISTS);
}

std::shared_ptr<CachedScript> ScriptManager::GetCachedScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0') {
        return nullptr;
    }
    return m_ScriptCache.GetCachedScript(scriptName);
}

std::shared_ptr<CachedScript> ScriptManager::NewCachedScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0') {
        return nullptr;
    }
    return m_ScriptCache.NewCachedScript(scriptName);
}

bool ScriptManager::RestoreCachedScriptFromChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = NewCachedScript(scriptName);
    if (!script) {
        return false;
    }
    if (script->module) {
        return true;
    }
    return script->LoadFromChunk(chunk);
}

bool ScriptManager::SaveCachedScriptToChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    return script ? script->SaveToChunk(chunk) : false;
}

bool ScriptManager::ClearCachedScriptCode(const char *scriptName) {
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    if (!script) {
        return false;
    }
    script->ClearCodeCache();
    return true;
}

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        SetupScriptPathCategory();
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}

void * ScriptManager::GetCKObjectData(CK_ID id) const {
    const auto it = m_CKObjectDataMap.find(id);
    if (it == m_CKObjectDataMap.end()) {
        return nullptr;
    }
    return it->second;
}

void ScriptManager::SetCKObjectData(CK_ID id, void *data) {
    if (data) {
        m_CKObjectDataMap[id] = data;
    } else {
        m_CKObjectDataMap.erase(id);
    }
}

void ScriptManager::ReleaseCKObjectData(CK_ID id) {
    const auto it = m_CKObjectDataMap.find(id);
    if (it == m_CKObjectDataMap.end()) {
        return;
    }

    auto *func = static_cast<asIScriptFunction *>(it->second);
    m_CKObjectDataMap.erase(it);
    if (func) {
        func->Release();
    }
}

void ScriptManager::ClearCKObjectData() {
    for (const auto &entry : m_CKObjectDataMap) {
        auto *func = static_cast<asIScriptFunction *>(entry.second);
        if (func) {
            func->Release();
        }
    }
    m_CKObjectDataMap.clear();

    for (const auto &entry : m_CKObjectCallbackMap) {
        for (auto *func : entry.second) {
            if (!func) {
                continue;
            }
            if (IsMarkedAsReleasedOnce(func)) {
                ClearReleasedOnceMark(func);
            } else {
                if (IsMarkedAsTemporary(func)) {
                    ClearTemporaryMark(func);
                }
                func->Release();
            }
        }
    }
    m_CKObjectCallbackMap.clear();
}

void ScriptManager::TrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    if (func) {
        m_CKObjectCallbackMap[id].push_back(func);
    }
}

void ScriptManager::UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    auto it = m_CKObjectCallbackMap.find(id);
    if (it == m_CKObjectCallbackMap.end()) {
        return;
    }

    auto &callbacks = it->second;
    for (auto cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (*cb == func) {
            callbacks.erase(cb);
            break;
        }
    }

    if (callbacks.empty()) {
        m_CKObjectCallbackMap.erase(it);
    }
}

void ScriptManager::ReleaseCKObjectCallbacks(CK_ID id) {
    auto it = m_CKObjectCallbackMap.find(id);
    if (it == m_CKObjectCallbackMap.end()) {
        return;
    }

    auto callbacks = std::move(it->second);
    m_CKObjectCallbackMap.erase(it);
    for (auto *func : callbacks) {
        if (!func) {
            continue;
        }
        if (IsMarkedAsReleasedOnce(func)) {
            ClearReleasedOnceMark(func);
        } else {
            if (IsMarkedAsTemporary(func)) {
                ClearTemporaryMark(func);
            }
            func->Release();
        }
    }
}

static void ReleaseScriptFunction(asIScriptFunction *&func) {
    if (!func) {
        return;
    }

    func->Release();
    func = nullptr;
}

ScriptComponentState *ScriptManager::GetOrCreateComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return nullptr;
    }

    const CK_ID id = behavior->GetID();
    auto it = m_ComponentStates.find(id);
    if (it != m_ComponentStates.end()) {
        it->second->Behavior = behavior;
        return it->second.get();
    }

    auto state = std::make_unique<ScriptComponentState>();
    state->BehaviorId = id;
    state->Behavior = behavior;
    state->MessageTarget = ScriptMessageBus::ComponentTarget(id);

    ScriptComponentState *raw = state.get();
    m_ComponentStates[id] = std::move(state);
    return raw;
}

ScriptComponentState *ScriptManager::GetComponentState(CK_ID id) const {
    auto it = m_ComponentStates.find(id);
    if (it == m_ComponentStates.end()) {
        return nullptr;
    }

    return it->second.get();
}

void ScriptManager::ResetComponentStateRuntime(ScriptComponentState *state, bool unloadPrivateModule) {
    if (!state) {
        return;
    }

    if (m_BehaviorBridge && state->BehaviorId) {
        m_BehaviorBridge->DestroyComponentTasks(state->BehaviorId);
    }

    ReleaseScriptFunction(state->OnLoad);
    ReleaseScriptFunction(state->Awake);
    ReleaseScriptFunction(state->OnEnable);
    ReleaseScriptFunction(state->Start);
    ReleaseScriptFunction(state->Update);
    ReleaseScriptFunction(state->OnDisable);
    ReleaseScriptFunction(state->OnDestroy);
    ReleaseScriptFunction(state->OnReset);
    ReleaseScriptFunction(state->OnMessage);
    state->ActiveLifecycle = nullptr;
    state->ActiveLifecycleName.clear();

    if (state->Object) {
        state->Object->Release();
        state->Object = nullptr;
    }

    if (state->Invoker) {
        state->Invoker->Reset();
        delete state->Invoker;
        state->Invoker = nullptr;
    }

    if (unloadPrivateModule && state->PrivateModule && !state->RuntimeModuleName.empty()) {
        UnloadModule(state->RuntimeModuleName.c_str(), nullptr);
    }

    state->RuntimeModuleName.clear();
    state->Bindings.clear();
    if (m_MessageBus) {
        m_MessageBus->ClearTarget(state->MessageTarget, "Component runtime was reset.");
    }
    state->MessageTopics.clear();
    state->StaticMessageSubscriptionsRegistered = false;
    state->PrivateModule = false;
    state->Loaded = false;
    state->OnLoadCalled = false;
    state->AwakeCalled = false;
    state->StartCalled = false;
    state->InstanceEnabled = false;
    state->Failed = false;
    state->PendingDestroy = false;
    state->PendingDisableOutput = false;
    state->PendingResetRuntime = false;
}

void ScriptManager::ReleaseComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }

    ScriptComponentState *state = GetComponentState(behavior->GetID());
    if (state && state->Behavior) {
        ScriptComponentState *nullState = nullptr;
        state->Behavior->SetLocalParameterValue(0, &nullState);
    }

    ReleaseComponentState(behavior->GetID());
}

void ScriptManager::ReleaseComponentState(CK_ID id) {
    auto it = m_ComponentStates.find(id);
    if (it == m_ComponentStates.end()) {
        return;
    }

    if (m_BehaviorBridge) {
        m_BehaviorBridge->DestroyComponentTasks(id);
    }
    ResetComponentStateRuntime(it->second.get(), true);
    m_ComponentStates.erase(it);
}

void ScriptManager::ClearComponentStates() {
    for (auto &entry : m_ComponentStates) {
        if (m_BehaviorBridge) {
            m_BehaviorBridge->DestroyComponentTasks(entry.first);
        }
        ResetComponentStateRuntime(entry.second.get(), true);
    }
    m_ComponentStates.clear();
}

bool ScriptManager::DeliverComponentMessage(CK_ID id, const ScriptMessage &message, bool immediate, std::string &error) {
    ScriptComponentState *state = GetComponentState(id);
    if (!state || !state->Loaded || state->Failed || !state->InstanceEnabled || !state->Object || !state->Invoker || !state->OnMessage) {
        error = "Component message target is not ready.";
        return false;
    }
    if (state->Invoker->IsContextSuspended()) {
        error = "Component message target is busy.";
        return false;
    }
    CKBehaviorContext ctx = {};
    ctx.Context = m_Context;
    ctx.Behavior = state->Behavior;
    const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(state->Object, state->OnMessage, message, ctx);
    if (status == ScriptInvocationStatus::Suspended) {
        return true;
    }
    if (status == ScriptInvocationStatus::Failed) {
        error = state->Invoker->GetErrorMessage();
        if (error.empty()) {
            error = "Component message handler failed.";
        }
        state->Failed = true;
        return false;
    }
    return true;
}

ScriptBehaviorBridge *ScriptManager::GetBehaviorBridge() {
    if (!m_BehaviorBridge) {
        m_BehaviorBridge = std::make_unique<ScriptBehaviorBridge>(this);
    }
    return m_BehaviorBridge.get();
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            break;
    }
    const std::string formatted = fmt::format("{}({},{}): {}: {}",
        msg.section ? msg.section : "<unknown section>",
        msg.row,
        msg.col,
        type,
        msg.message ? msg.message : "");
    if (m_CapturingScriptMessages) {
        if (!m_CapturedScriptMessages.empty()) {
            m_CapturedScriptMessages += "\n";
        }
        m_CapturedScriptMessages += formatted;
    }
    m_Context->OutputToConsoleEx(const_cast<char *>("%s"), formatted.c_str());
}

void ScriptManager::ExceptionCallback(asIScriptContext *context) {
    std::string callStack = GetCallStack(context);
    asIScriptFunction *func = context ? context->GetExceptionFunction() : nullptr;
    const char *funcDecl = func ? func->GetDeclaration() : nullptr;
    const char *exception = context ? context->GetExceptionString() : nullptr;
    std::string message = fmt::format("Exception in '{}': '{}'\n{}",
        funcDecl ? funcDecl : "<unknown function>",
        exception ? exception : "",
        callStack);

    asSMessageInfo info = {};
    if (context) {
        info.row = context->GetExceptionLineNumber(&info.col, &info.section);
    }
    info.type = asMSGTYPE_ERROR;
    info.message = message.c_str();
    MessageCallback(info);
}

std::string ScriptManager::GetCallStack(asIScriptContext *context) {
    std::string str("Callstack:\n");
    if (!context) {
        return str;
    }

    for (asUINT i = 0; i < context->GetCallstackSize(); i++) {
        asIScriptFunction *func = context->GetFunction(i);
        int column;
        const char *section;
        int line = context->GetLineNumber(i, &column, &section);
        const char *funcDecl = func ? func->GetDeclaration() : nullptr;
        str.append(fmt::format("\t{} at {}({},{})\n",
            funcDecl ? funcDecl : "<unknown function>",
            section ? section : "<unknown section>",
            line,
            column));
    }
    return std::move(str);
}

asIScriptContext *ScriptManager::RequestContextFromPool() {
    asIScriptContext *ctx = nullptr;
    if (!m_ScriptContexts.empty()) {
        ctx = *m_ScriptContexts.rbegin();
        m_ScriptContexts.pop_back();
    } else
        ctx = m_ScriptEngine->CreateContext();

    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    ctx->SetExceptionCallback(asMETHOD(ScriptManager, ExceptionCallback), this, asCALL_THISCALL);
    return ctx;
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    if (!ctx) {
        return;
    }

    // Unprepare the context to free any objects that might be held
    // as we don't know when the context will be used again.
    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    m_ScriptContexts.push_back(ctx);
}

void ScriptManager::SetupScriptPathCategory() {
    if (m_ScriptPathCategoryIndex == -1) {
        XString category = "Script Paths";
        CKPathManager *pm = m_Context->GetPathManager();
        m_ScriptPathCategoryIndex = pm->GetCategoryIndex(category);
        if (m_ScriptPathCategoryIndex == -1)
            m_ScriptPathCategoryIndex = pm->AddCategory(category);
    }
}

int ScriptManager::SetupScriptEngine() {
    // #if CKVERSION == 0x13022002
    //     asSetGlobalMemoryFunctions(
    //         [](size_t size) { return VxMalloc(size); },
    //         [](void *ptr) { VxFree(ptr); }
    //     );
    // #endif

    m_ScriptEngine = asCreateScriptEngine();
    if (!m_ScriptEngine) {
        m_Context->OutputToConsole(const_cast<char *>("Failed to create script engine."));
        LOG_ERROR("Failed to create script engine.");
        return -1;
    }

    m_ScriptEngine->SetUserData(this, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    m_ScriptEngine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
    m_ScriptEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
    m_ScriptEngine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 1);

    // The script compiler will send any compiler messages to the callback
    int r = m_ScriptEngine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL);
    if (r < 0) {
        LOG_ERROR("SetMessageCallback failed with code %d.", r);
        return r;
    }

    // The script handle the pool of script contexts.
    r = m_ScriptEngine->SetContextCallbacks([](asIScriptEngine *, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        return man->RequestContextFromPool();
    }, [](asIScriptEngine *, asIScriptContext *ctx, void *param) {
        auto *man = static_cast<ScriptManager *>(param);
        man->ReturnContextToPool(ctx);
    }, this);
    if (r < 0) {
        LOG_ERROR("SetContextCallbacks failed with code %d.", r);
        return r;
    }

    m_ScriptEngine->SetEngineUserDataCleanupCallback([](asIScriptEngine *engine) {
        engine->SetUserData(nullptr, SCRIPT_MANAGER_TYPE);
    }, SCRIPT_MANAGER_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_TEMPORARY_FLAG_TYPE);
    }, AS_TEMPORARY_FLAG_TYPE);
    m_ScriptEngine->SetFunctionUserDataCleanupCallback([](asIScriptFunction *func) {
        func->SetUserData(nullptr, AS_RELEASED_ONCE_FLAG_TYPE);
    }, AS_RELEASED_ONCE_FLAG_TYPE);

    ScriptRegistrationContext registration("AngelScript engine registration");
    {
        ScriptRegistrationScope registrationScope(registration);

        // Register the standard types
        RegisterStdTypes(m_ScriptEngine);

        // Register the standard add-ons
        RegisterStdAddons(m_ScriptEngine);

        // Register the native types
        RegisterNativePointer(m_ScriptEngine);
        RegisterNativeBuffer(m_ScriptEngine);

#if CKAS_ENABLE_DYNCALL
        // Register the DynCall APIs
        RegisterScriptDynCall(m_ScriptEngine);
        RegisterScriptDynCallback(m_ScriptEngine);
        RegisterScriptDynLoad(m_ScriptEngine);
#endif

        // Register the function that we want the scripts to call
        RegisterScriptFormat(m_ScriptEngine);

        RegisterScriptInfo(m_ScriptEngine);

        // Register the Virtools API
        RegisterVirtools(m_ScriptEngine);
    }

    if (registration.HasFailures()) {
        const std::string summary = registration.GetSummary();
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
        LOG_ERROR("%s", summary.c_str());
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
        return -1;
    }

    // Register host-provided namespaces/types after CKAngelScript's own public
    // API is available. A failing extension is logged but is non-fatal: it must
    // not take down core scripting or the other extensions.
    RegisterEngineExtensions(m_ScriptEngine);

    return r;
}

void ScriptManager::RegisterStdTypes(asIScriptEngine *engine) {
    assert(engine != nullptr);

    int r = 0;

    if constexpr (sizeof(void *) == 4) {
        r = engine->RegisterTypedef("size_t", "uint"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("ptrdiff_t", "int"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("intptr_t", "int"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("uintptr_t", "uint"); CKAS_CHECK_REGISTER(r);
    } else {
        r = engine->RegisterTypedef("size_t", "uint64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("ptrdiff_t", "int64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("intptr_t", "int64"); CKAS_CHECK_REGISTER(r);
        r = engine->RegisterTypedef("uintptr_t", "uint64"); CKAS_CHECK_REGISTER(r);
    }
}

void ScriptManager::RegisterStdAddons(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterStdStringUtils(engine);
    RegisterScriptAny(engine);
    RegisterScriptHandle(engine);
    RegisterScriptWeakRef(engine);
    RegisterScriptDictionary(engine);
    RegisterScriptDateTime(engine);
    RegisterScriptFile(engine);
    RegisterScriptFileSystem(engine);
    RegisterScriptMath(engine);
    RegisterScriptMathComplex(engine);
    RegisterScriptGrid(engine);
    RegisterExceptionRoutines(engine);
}

void ScriptManager::RegisterVirtools(asIScriptEngine *engine) {
    assert(engine != nullptr);

    RegisterVxMath(m_ScriptEngine);
    RegisterCK2(m_ScriptEngine);
    RegisterScriptParameterRegistry(m_ScriptEngine);
    RegisterScriptBehaviorBridge(m_ScriptEngine);
    RegisterScriptSceneCore(m_ScriptEngine);
    RegisterScriptRuntime(m_ScriptEngine);
    RegisterScriptAsync(m_ScriptEngine);
    RegisterScriptMessage(m_ScriptEngine);
}

int ScriptManager::RegisterEngineExtensions(asIScriptEngine *engine) {
    assert(engine != nullptr);

    // A failing host extension must not bring down the whole engine: core
    // CKAngelScript scripting and the remaining extensions stay available. Each
    // failure is reported individually and the first failure code is returned
    // for callers that want to surface it.
    int firstFailure = 0;
    for (const ScriptEngineExtensionRegistration &extension : m_EngineExtensions) {
        if (!extension.Register) {
            continue;
        }
        const char *extensionError = nullptr;
        const int code = extension.Register(engine, ToPublicHandle(this), extension.UserData, &extensionError);
        if (code < 0) {
            const char *name = extension.Name.empty() ? "<unnamed extension>" : extension.Name.c_str();
            const std::string summary =
                extensionError && extensionError[0] != '\0'
                    ? fmt::format("Engine extension '{}' failed to register (code {}): {}", name, code, extensionError)
                    : fmt::format("Engine extension '{}' failed to register (code {}).", name, code);
            if (m_Context) {
                m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            if (firstFailure == 0) {
                firstFailure = code;
            }
        }
    }
    return firstFailure;
}
