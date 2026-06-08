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
        if (Object) {
            Object->Release();
            Object = nullptr;
        }
    }

    ScriptManager *Manager = nullptr;
    ScriptInvoker Invoker;
    asIScriptFunction *Function = nullptr;
    asIScriptObject *Object = nullptr;
    CKAS_EXECUTIONSTATE State = CKAS_EXECUTION_READY;
    CKAngelScriptResult Result;
    std::string ErrorMessage;
    std::string StackTrace;
    std::string ModuleName;
    std::string FunctionName;
    std::string FunctionDecl;
    std::string ClassName;
    CKDWORD ModuleGeneration = 0;
    CKBehaviorContext BehaviorContextStorage;
    bool HasBehaviorContext = false;
    CKAngelScriptContextCallback ConfigureContext = nullptr;
    CKAngelScriptContextCallback ReadResult = nullptr;
    CKAngelScriptWriteArgsCallback WriteMethodArgs = nullptr;
    CKAngelScriptReadResultCallback ReadMethodResult = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_CALL_DEFAULT;
    std::vector<int> ParamTypes;
    std::vector<asDWORD> ParamFlags;
    int ReturnType = asTYPEID_VOID;
    asDWORD ReturnFlags = 0;
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
    asIScriptFunction *Function = nullptr;
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

namespace {

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
                                              const CKAngelScriptMethod *method,
                                              CKAngelScriptWriteArgsCallback writeArgs,
                                              CKAngelScriptReadResultCallback readResult,
                                              void *userData,
                                              CKDWORD flags) {
    ObjectCallOutcome outcome = {};
    if (!manager || !object || !method || !method->Function) {
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
    int r = ctx->Prepare(method->Function);
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
    const CKDWORD size = value.Size ? value.Size : static_cast<CKDWORD>(sizeof(T));
    const char *base = reinterpret_cast<const char *>(&value);
    const char *member = reinterpret_cast<const char *>(&(value.*field));
    const size_t offset = static_cast<size_t>(member - base);
    return size >= offset + sizeof(M);
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

    if (execution->Object) {
        if (!execution->Manager ||
            !execution->Manager->HasModule(execution->ModuleName.c_str()) ||
            execution->Manager->GetModuleGeneration(execution->ModuleName.c_str()) != execution->ModuleGeneration) {
            MakeExecutionResult(execution, CKAS_STALEHANDLE, 0, "Object method execution handle is stale.");
            return CKAS_STALEHANDLE;
        }

        CKAngelScriptMethod method = {};
        method.Manager = execution->Manager;
        method.Function = execution->Function;
        method.ModuleName = execution->ModuleName;
        method.ClassName = execution->ClassName;
        method.MethodName = execution->FunctionName;
        method.MethodDecl = execution->FunctionDecl;
        method.ModuleGeneration = execution->ModuleGeneration;
        method.ParamTypes = execution->ParamTypes;
        method.ParamFlags = execution->ParamFlags;
        method.ReturnType = execution->ReturnType;
        method.ReturnFlags = execution->ReturnFlags;

        execution->State = CKAS_EXECUTION_RUNNING;
        const ObjectCallOutcome outcome = ExecutePreparedObjectMethod(execution->Manager,
                                                                      execution->Object,
                                                                      &method,
                                                                      execution->WriteMethodArgs,
                                                                      execution->ReadMethodResult,
                                                                      execution->UserData,
                                                                      execution->Flags);
        execution->State = outcome.Status == CKAS_OK ? CKAS_EXECUTION_FINISHED : CKAS_EXECUTION_FAILED;
        MakeExecutionResult(execution,
                            outcome.Status,
                            outcome.AngelScriptCode,
                            outcome.ErrorMessage,
                            outcome.StackTrace);
        return outcome.Status;
    }

    execution->State = CKAS_EXECUTION_RUNNING;
    const ScriptInvocationStatus scriptStatus = execution->Invoker.ExecuteScriptStatus(
        execution->Function,
        [execution](asIScriptContext *ctx) {
            if (execution->HasBehaviorContext && execution->Function && execution->Function->GetParamCount() > 0) {
                ctx->SetArgObject(0, (void *)&execution->BehaviorContextStorage);
            }
            if (execution->ConfigureContext) {
                execution->ConfigureContext(ctx, execution->UserData);
            }
        },
        [execution](asIScriptContext *ctx) {
            if (execution->ReadResult) {
                execution->ReadResult(ctx, execution->UserData);
            }
        });

    execution->State = ToExecutionState(scriptStatus);
    const CKAS_STATUS status = ToCKAS_STATUS(scriptStatus);
    const int resultCode = execution->Invoker.GetLastResultCode();
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

} // namespace

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

extern "C" CKAS_API asIScriptEngine *CKAngelScriptGetScriptEngine(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetScriptEngine() : nullptr;
}

extern "C" CKAS_API CKDWORD CKAngelScriptGetApiVersion() {
    return CKAS_API_VERSION;
}

extern "C" CKAS_API CKBOOL CKAngelScriptHasCapability(CKAngelScript *angelScript,
                                                       CKAS_APICAPABILITY capability) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager || !scriptManager->GetScriptEngine()) {
        return FALSE;
    }
    switch (capability) {
        case CKAS_CAP_MODULE_COMPILE:
        case CKAS_CAP_OBJECT_CREATE:
        case CKAS_CAP_OBJECT_METHOD_EXECUTION:
        case CKAS_CAP_ARG_WRITER:
        case CKAS_CAP_RESULT_READER:
        case CKAS_CAP_STACKTRACE:
            return TRUE;
        case CKAS_CAP_ASYNC_RESUME:
            return FALSE;
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

extern "C" CKAS_API asIScriptContext *CKAngelScriptGetActiveContext(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetActiveContext() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                                             const CKAngelScriptLoadOptions *options,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options
               ? scriptManager->LoadModule(*options, result)
               : CKAS_INVALIDARGUMENT;
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

extern "C" CKAS_API asIScriptModule *CKAngelScriptGetModule(CKAngelScript *angelScript, const char *moduleName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetModule(moduleName) : nullptr;
}

extern "C" CKAS_API asIScriptFunction *CKAngelScriptFindFunctionByName(CKAngelScript *angelScript,
                                                                      const char *moduleName,
                                                                      const char *functionName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->FindFunctionByName(moduleName, functionName) : nullptr;
}

extern "C" CKAS_API asIScriptFunction *CKAngelScriptFindFunctionByDecl(CKAngelScript *angelScript,
                                                                      const char *moduleName,
                                                                      const char *functionDecl) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->FindFunctionByDecl(moduleName, functionDecl) : nullptr;
}

extern "C" CKAS_API CKAngelScriptObject *CKAngelScriptCreateObject(CKAngelScript *angelScript,
                                                                   const CKAngelScriptObjectOptions *options,
                                                                   CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options ? scriptManager->CreateObject(*options, result) : nullptr;
}

extern "C" CKAS_API void CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                    CKAngelScriptObject *object) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (scriptManager) {
        scriptManager->ReleaseObject(object);
    }
}

extern "C" CKAS_API CKAngelScriptMethod *CKAngelScriptFindObjectMethod(CKAngelScript *angelScript,
                                                                       const CKAngelScriptMethodOptions *options,
                                                                       CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options ? scriptManager->FindObjectMethod(*options, result) : nullptr;
}

extern "C" CKAS_API void CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                    CKAngelScriptMethod *method) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (scriptManager) {
        scriptManager->ReleaseMethod(method);
    }
}

extern "C" CKAS_API CKAngelScriptExecution *CKAngelScriptCreateObjectMethodExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options ? scriptManager->CreateObjectMethodExecution(*options, result) : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options ? scriptManager->CallObjectMethod(*options, result) : CKAS_INVALIDARGUMENT;
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

extern "C" CKAS_API CKAngelScriptExecution *CKAngelScriptCreateExecution(CKAngelScript *angelScript,
                                                                      const CKAngelScriptExecuteOptions *options,
                                                                      CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && options ? scriptManager->CreateExecution(*options, result) : nullptr;
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

extern "C" CKAS_API void CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                      CKAngelScriptExecution *execution) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (scriptManager) {
        scriptManager->ReleaseExecution(execution);
    }
}

extern "C" CKAS_API CKAS_EXECUTIONSTATE CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                                            const CKAngelScriptExecution *execution) {
    const ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetExecutionState(execution) : CKAS_EXECUTION_FAILED;
}

extern "C" CKAS_API const CKAngelScriptResult *CKAngelScriptGetExecutionResult(CKAngelScript *angelScript,
                                                                            const CKAngelScriptExecution *execution) {
    const ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetExecutionResult(execution) : nullptr;
}

extern "C" CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript) {
    const ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetLastResult() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                                          const CKAngelScriptEngineExtension *extension,
                                                                          CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && extension
               ? scriptManager->RegisterEngineExtension(*extension, result)
               : CKAS_INVALIDARGUMENT;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                                            const char *name,
                                                                            void *userData,
                                                                            CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnregisterEngineExtension(name, userData, result)
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

    for (auto *method : m_Methods) {
        if (method && method->Function) {
            method->Function->Release();
            method->Function = nullptr;
        }
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

asIScriptContext *ScriptManager::GetActiveContext() {
    return asGetActiveContext();
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

CKAS_STATUS ScriptManager::RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                                         CKAngelScriptResult *result) {
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
    for (const CKAngelScriptEngineExtension &existing : m_EngineExtensions) {
        if (existing.Name && std::string(existing.Name) == name) {
            return StoreResult(result,
                               CKAS_INVALIDARGUMENT,
                               0,
                               fmt::format("Engine extension '{}' is already registered.", name));
        }
    }

    // Retain the extension first so it is replayed on the next engine rebuild
    // regardless of whether the immediate invocation below succeeds. This keeps
    // the registration path consistent with the setup/rebuild path, where a
    // failing extension is logged but never removes the others or tears the
    // engine down.
    CKAngelScriptEngineExtension retained = {};
    retained.Size = sizeof(retained);
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
                                                           void *userData,
                                                           CKAngelScriptResult *result) {
    if (!name || name[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    for (auto it = m_EngineExtensions.begin(); it != m_EngineExtensions.end(); ++it) {
        const bool nameMatches = it->Name && std::string(it->Name) == name;
        const bool userDataMatches = !userData || it->UserData == userData;
        if (nameMatches && userDataMatches) {
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

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
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
    bool replacedExisting = false;
    if (HasModule(moduleName)) {
        if (!HasPublicFlag(flags, CKAS_LOAD_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Module already exists.");
        }
        if (!hasCode) {
            if (HasExecutionForModule(moduleName)) {
                return StoreResult(result,
                                   CKAS_EXECUTIONFAILED,
                                   0,
                                   "Module has active execution handles.");
            }
            DiscardCachedModule(moduleName);
            BumpModuleGeneration(moduleName);
            replacedExisting = true;
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
        BeginScriptMessageCapture();
        const int loadResult = LoadModuleFromFiles(moduleName, filenames, fileCount);
        const std::string diagnostics = EndScriptMessageCapture();
        if (loadResult < 0) {
            return StoreResult(result,
                               CKAS_COMPILEERROR,
                               loadResult,
                               diagnostics.empty() ? "Failed to load script files." : diagnostics);
        }
        if (!replacedExisting) {
            BumpModuleGeneration(moduleName);
        }
        return StoreResult(result, CKAS_OK);
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
    if (!replacedExisting) {
        BumpModuleGeneration(moduleName);
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                               const char *scriptCode,
                                               CKDWORD flags,
                                               CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0' || !scriptCode) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and script code are required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    bool replacedExisting = false;
    if (HasModule(moduleName)) {
        if (!HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Module already exists.");
        }
        if (HasExecutionForModule(moduleName)) {
            return StoreResult(result,
                               CKAS_EXECUTIONFAILED,
                               0,
                               "Module has active execution handles.");
        }
        DiscardCachedModule(moduleName);
        BumpModuleGeneration(moduleName);
        replacedExisting = true;
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
    if (!replacedExisting) {
        BumpModuleGeneration(moduleName);
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (HasExecutionForModule(moduleName)) {
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           0,
                           "Module has active execution handles.");
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

asIScriptFunction *ScriptManager::FindFunctionByName(const char *moduleName, const char *functionName) {
    asIScriptModule *module = GetModule(moduleName);
    if (!module || !functionName || functionName[0] == '\0') {
        return nullptr;
    }
    return module->GetFunctionByName(functionName);
}

asIScriptFunction *ScriptManager::FindFunctionByDecl(const char *moduleName, const char *functionDecl) {
    asIScriptModule *module = GetModule(moduleName);
    if (!module || !functionDecl || functionDecl[0] == '\0') {
        return nullptr;
    }
    return module->GetFunctionByDecl(functionDecl);
}

CKAngelScriptObject *ScriptManager::CreateObject(const CKAngelScriptObjectOptions &options,
                                                 CKAngelScriptResult *result) {
    const char *moduleName = PublicField(options, &CKAngelScriptObjectOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *className = PublicField(options, &CKAngelScriptObjectOptions::ClassName, static_cast<const char *>(nullptr));
    if (!moduleName || moduleName[0] == '\0' || !className || className[0] == '\0') {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and class name are required.");
        return nullptr;
    }
    if (!m_ScriptEngine) {
        StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
        return nullptr;
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
        return nullptr;
    }
    asITypeInfo *type = module->GetTypeInfoByName(className);
    if (!type) {
        StoreResult(result, CKAS_NOTFOUND, 0, "Script class was not found.");
        return nullptr;
    }
    auto *scriptObject = static_cast<asIScriptObject *>(m_ScriptEngine->CreateScriptObject(type));
    if (!scriptObject) {
        StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Failed to create script object.");
        return nullptr;
    }

    auto *object = new CKAngelScriptObject();
    object->Manager = this;
    object->Object = scriptObject;
    object->ModuleName = moduleName;
    object->ClassName = className;
    object->ModuleGeneration = GetModuleGeneration(moduleName);
    m_Objects.insert(object);
    StoreResult(result, CKAS_OK);
    return object;
}

void ScriptManager::ReleaseObject(CKAngelScriptObject *object) {
    if (!OwnsObject(object)) {
        return;
    }
    m_Objects.erase(object);
    if (object->Object) {
        object->Object->Release();
        object->Object = nullptr;
    }
    delete object;
}

CKAngelScriptMethod *ScriptManager::FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                                     CKAngelScriptResult *result) {
    CKAngelScriptObject *object = PublicField(options, &CKAngelScriptMethodOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    const char *methodName = PublicField(options, &CKAngelScriptMethodOptions::MethodName, static_cast<const char *>(nullptr));
    const char *methodDecl = PublicField(options, &CKAngelScriptMethodOptions::MethodDecl, static_cast<const char *>(nullptr));
    const CKBOOL optional = PublicField(options, &CKAngelScriptMethodOptions::Optional, static_cast<CKBOOL>(FALSE));
    const bool hasMethodName = methodName && methodName[0] != '\0';
    const bool hasMethodDecl = methodDecl && methodDecl[0] != '\0';
    if (!OwnsObject(object) || !object->Object) {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object handle is invalid.");
        return nullptr;
    }
    if (!hasMethodName && !hasMethodDecl) {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method name or declaration is required.");
        return nullptr;
    }
    if (!HasModule(object->ModuleName.c_str()) || GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration) {
        StoreResult(result, CKAS_STALEHANDLE, 0, "Object handle is stale.");
        return nullptr;
    }
    asITypeInfo *type = object->Object->GetObjectType();
    if (!type) {
        StoreResult(result, CKAS_EXECUTIONFAILED, 0, "Script object has no type information.");
        return nullptr;
    }

    asIScriptFunction *function = nullptr;
    if (hasMethodDecl) {
        function = type->GetMethodByDecl(methodDecl);
    } else {
        function = type->GetMethodByName(methodName);
    }
    if (!function) {
        StoreResult(result, optional ? CKAS_OK : CKAS_NOTFOUND, 0, "Script object method was not found.");
        return nullptr;
    }

    auto *method = new CKAngelScriptMethod();
    method->Manager = this;
    method->Function = function;
    method->Function->AddRef();
    method->ModuleName = object->ModuleName;
    method->ClassName = object->ClassName;
    method->MethodName = methodName ? methodName : "";
    method->MethodDecl = methodDecl ? methodDecl : function->GetDeclaration();
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
    StoreResult(result, CKAS_OK);
    return method;
}

void ScriptManager::ReleaseMethod(CKAngelScriptMethod *method) {
    if (!OwnsMethod(method)) {
        return;
    }
    m_Methods.erase(method);
    if (method->Function) {
        method->Function->Release();
        method->Function = nullptr;
    }
    delete method;
}

CKAngelScriptExecution *ScriptManager::CreateObjectMethodExecution(const CKAngelScriptObjectMethodExecuteOptions &options,
                                                                   CKAngelScriptResult *result) {
    const CKDWORD flags = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    if (HasPublicFlag(flags, CKAS_CALL_BORROWED_ARGS)) {
        StoreResult(result, CKAS_UNSUPPORTED, 0, "Borrowed arguments are only supported by synchronous object method calls.");
        return nullptr;
    }
    CKAngelScriptObject *object = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    CKAngelScriptMethod *method = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Method, static_cast<CKAngelScriptMethod *>(nullptr));
    if (!OwnsObject(object) || !OwnsMethod(method) || !object->Object || !method->Function) {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Object or method handle is invalid.");
        return nullptr;
    }
    if (object->ModuleName != method->ModuleName || object->ClassName != method->ClassName) {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Method handle does not belong to the object type.");
        return nullptr;
    }
    if (!HasModule(object->ModuleName.c_str()) ||
        GetModuleGeneration(object->ModuleName.c_str()) != object->ModuleGeneration ||
        method->ModuleGeneration != object->ModuleGeneration) {
        StoreResult(result, CKAS_STALEHANDLE, 0, "Object or method handle is stale.");
        return nullptr;
    }

    auto *execution = new CKAngelScriptExecution(this);
    execution->Object = object->Object;
    execution->Object->AddRef();
    execution->Function = method->Function;
    execution->Function->AddRef();
    execution->ModuleName = object->ModuleName;
    execution->ClassName = object->ClassName;
    execution->FunctionName = method->MethodName;
    execution->FunctionDecl = method->MethodDecl;
    execution->ModuleGeneration = object->ModuleGeneration;
    execution->WriteMethodArgs =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::WriteArgs, static_cast<CKAngelScriptWriteArgsCallback>(nullptr));
    execution->ReadMethodResult =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ReadResult, static_cast<CKAngelScriptReadResultCallback>(nullptr));
    execution->UserData = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::UserData, static_cast<void *>(nullptr));
    execution->Flags = flags;
    execution->ParamTypes = method->ParamTypes;
    execution->ParamFlags = method->ParamFlags;
    execution->ReturnType = method->ReturnType;
    execution->ReturnFlags = method->ReturnFlags;
    MakeExecutionResult(execution, CKAS_OK);
    m_Executions.insert(execution);
    StoreResult(result, CKAS_OK);
    return execution;
}

CKAS_STATUS ScriptManager::CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                            CKAngelScriptResult *result) {
    CKAngelScriptObject *object = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Object, static_cast<CKAngelScriptObject *>(nullptr));
    CKAngelScriptMethod *method = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Method, static_cast<CKAngelScriptMethod *>(nullptr));
    if (!OwnsObject(object) || !OwnsMethod(method) || !object->Object || !method->Function) {
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
    CKAngelScriptWriteArgsCallback writeArgs =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::WriteArgs, static_cast<CKAngelScriptWriteArgsCallback>(nullptr));
    CKAngelScriptReadResultCallback readResult =
        PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::ReadResult, static_cast<CKAngelScriptReadResultCallback>(nullptr));
    void *userData = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = PublicField(options, &CKAngelScriptObjectMethodExecuteOptions::Flags, static_cast<CKDWORD>(CKAS_CALL_DEFAULT));

    const ObjectCallOutcome outcome = ExecutePreparedObjectMethod(this,
                                                                  object->Object,
                                                                  method,
                                                                  writeArgs,
                                                                  readResult,
                                                                  userData,
                                                                  flags);
    return StoreResult(result, outcome.Status, outcome.AngelScriptCode, outcome.ErrorMessage, outcome.StackTrace);
}

CKAngelScriptExecution *ScriptManager::CreateExecution(const CKAngelScriptExecuteOptions &options, CKAngelScriptResult *result) {
    const char *moduleName = PublicField(options, &CKAngelScriptExecuteOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *functionName = PublicField(options, &CKAngelScriptExecuteOptions::FunctionName, static_cast<const char *>(nullptr));
    const char *functionDecl = PublicField(options, &CKAngelScriptExecuteOptions::FunctionDecl, static_cast<const char *>(nullptr));
    const CKBehaviorContext *behaviorContext =
        PublicField(options, &CKAngelScriptExecuteOptions::BehaviorContext, static_cast<const CKBehaviorContext *>(nullptr));
    CKAngelScriptContextCallback configureContext =
        PublicField(options, &CKAngelScriptExecuteOptions::ConfigureContext, static_cast<CKAngelScriptContextCallback>(nullptr));
    CKAngelScriptContextCallback readResult =
        PublicField(options, &CKAngelScriptExecuteOptions::ReadResult, static_cast<CKAngelScriptContextCallback>(nullptr));
    void *userData = PublicField(options, &CKAngelScriptExecuteOptions::UserData, static_cast<void *>(nullptr));

    if (!moduleName || moduleName[0] == '\0') {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
        return nullptr;
    }
    if (!m_ScriptEngine) {
        StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
        return nullptr;
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
        return nullptr;
    }

    asIScriptFunction *function = nullptr;
    if (functionDecl && functionDecl[0] != '\0') {
        function = module->GetFunctionByDecl(functionDecl);
    } else if (functionName && functionName[0] != '\0') {
        function = module->GetFunctionByName(functionName);
    } else {
        StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name or declaration is required.");
        return nullptr;
    }
    if (!function) {
        StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
        return nullptr;
    }

    auto *execution = new CKAngelScriptExecution(this);
    execution->ModuleName = moduleName;
    execution->FunctionName = functionName ? functionName : "";
    execution->FunctionDecl = functionDecl ? functionDecl : "";
    execution->ConfigureContext = configureContext;
    execution->ReadResult = readResult;
    execution->UserData = userData;
    if (behaviorContext) {
        execution->BehaviorContextStorage = *behaviorContext;
        execution->HasBehaviorContext = true;
    }
    function->AddRef();
    execution->Function = function;
    if (!execution->Invoker.SetScript(moduleName)) {
        const std::string error = execution->Invoker.GetErrorMessage();
        delete execution;
        StoreResult(result, CKAS_NOTFOUND, 0, error.empty() ? "Module cache was not found." : error);
        return nullptr;
    }

    MakeExecutionResult(execution, CKAS_OK);
    m_Executions.insert(execution);
    StoreResult(result, CKAS_OK);
    return execution;
}

CKAS_STATUS ScriptManager::StartExecution(CKAngelScriptExecution *execution) {
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->State != CKAS_EXECUTION_READY) {
        MakeExecutionResult(execution, CKAS_EXECUTIONFAILED, 0, "Execution has already started.");
        return StoreResult(nullptr, CKAS_EXECUTIONFAILED, 0, "Execution has already started.");
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
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->State != CKAS_EXECUTION_SUSPENDED) {
        MakeExecutionResult(execution, CKAS_EXECUTIONFAILED, 0, "Execution is not suspended.");
        return StoreResult(nullptr, CKAS_EXECUTIONFAILED, 0, "Execution is not suspended.");
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
    if (!OwnsExecution(execution)) {
        return StoreResult(nullptr, CKAS_INVALIDARGUMENT, 0, "Execution handle is invalid.");
    }
    if (execution->State == CKAS_EXECUTION_FINISHED ||
        execution->State == CKAS_EXECUTION_FAILED ||
        execution->State == CKAS_EXECUTION_CANCELLED) {
        MakeExecutionResult(execution, CKAS_CANCELLED);
        return StoreResult(nullptr, CKAS_CANCELLED);
    }
    execution->Invoker.AbortContext();
    execution->State = CKAS_EXECUTION_CANCELLED;
    MakeExecutionResult(execution, CKAS_CANCELLED);
    return StoreResult(nullptr, CKAS_CANCELLED);
}

void ScriptManager::ReleaseExecution(CKAngelScriptExecution *execution) {
    if (!OwnsExecution(execution)) {
        return;
    }
    m_Executions.erase(execution);
    delete execution;
}

CKAS_EXECUTIONSTATE ScriptManager::GetExecutionState(const CKAngelScriptExecution *execution) const {
    if (!OwnsExecution(execution)) {
        return CKAS_EXECUTION_FAILED;
    }
    return execution->State;
}

const CKAngelScriptResult *ScriptManager::GetExecutionResult(const CKAngelScriptExecution *execution) const {
    if (!OwnsExecution(execution)) {
        return nullptr;
    }
    return &execution->Result;
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
    for (const CKAngelScriptEngineExtension &extension : m_EngineExtensions) {
        if (!extension.Register) {
            continue;
        }
        const char *extensionError = nullptr;
        const int code = extension.Register(engine, ToPublicHandle(this), extension.UserData, &extensionError);
        if (code < 0) {
            const char *name = extension.Name ? extension.Name : "<unnamed extension>";
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
