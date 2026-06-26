#include "ScriptManager.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <fmt/format.h>

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

#ifndef CKAS_ENABLE_DYNCALL
#define CKAS_ENABLE_DYNCALL 0
#endif

#include "CKPathManager.h"
#include "Logger.h"
#include "ScriptApiHandles.h"
#include "ScriptInvoker.h"
#include "ScriptPublicApiInternal.h"

#include "ScriptFormat.h"
#include "ScriptNativePointer.h"
#include "ScriptNativeBuffer.h"
#if CKAS_ENABLE_API_EXPORT
#include "ScriptInfo.h"
#endif
#if CKAS_ENABLE_DYNCALL
#include "ScriptDynCall.h"
#endif
#include "ScriptUtils.h"
#include "ScriptVxMath.h"
#include "ScriptCK2.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptComponentState.h"
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

bool IsIntOrEnumType(asIScriptEngine *engine, int typeId) {
    if (typeId == asTYPEID_INT32) {
        return true;
    }
    if (!engine) {
        return false;
    }
    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    return type && (type->GetFlags() & asOBJ_ENUM) != 0;
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

bool IsValidObjectHandleParam(int typeId, asDWORD flags) {
    if ((typeId & asTYPEID_OBJHANDLE) == 0) {
        return false;
    }
    if ((typeId & asTYPEID_MASK_OBJECT) == 0) {
        return false;
    }
    return (flags & asTM_OUTREF) == 0;
}

bool IsCompatibleObjectHandle(asIScriptEngine *engine,
                              int expectedTypeId,
                              const CKAngelScriptObject *objectHandle) {
    if (!engine || !objectHandle || !objectHandle->Object) {
        return false;
    }
    asITypeInfo *expectedType = engine->GetTypeInfoById(expectedTypeId);
    asITypeInfo *actualType = objectHandle->Object->GetObjectType();
    if (!expectedType || !actualType) {
        return false;
    }
    return actualType == expectedType ||
           actualType->DerivesFrom(expectedType) ||
           actualType->Implements(expectedType);
}

const char *StatusName(CKAS_STATUS status) {
    switch (status) {
        case CKAS_OK:
            return "CKAS_OK";
        case CKAS_INVALIDARGUMENT:
            return "CKAS_INVALIDARGUMENT";
        case CKAS_NOTINITIALIZED:
            return "CKAS_NOTINITIALIZED";
        case CKAS_NOTFOUND:
            return "CKAS_NOTFOUND";
        case CKAS_COMPILEERROR:
            return "CKAS_COMPILEERROR";
        case CKAS_EXECUTIONFAILED:
            return "CKAS_EXECUTIONFAILED";
        case CKAS_SUSPENDED:
            return "CKAS_SUSPENDED";
        case CKAS_CANCELLED:
            return "CKAS_CANCELLED";
        case CKAS_STALEHANDLE:
            return "CKAS_STALEHANDLE";
        case CKAS_UNSUPPORTED:
            return "CKAS_UNSUPPORTED";
        case CKAS_TYPEMISMATCH:
            return "CKAS_TYPEMISMATCH";
        case CKAS_BUFFERTOOSMALL:
            return "CKAS_BUFFERTOOSMALL";
        case CKAS_INVALIDSTATE:
            return "CKAS_INVALIDSTATE";
        case CKAS_INUSE:
            return "CKAS_INUSE";
        case CKAS_ALREADYEXISTS:
            return "CKAS_ALREADYEXISTS";
        case CKAS_AMBIGUOUS:
            return "CKAS_AMBIGUOUS";
        case CKAS_FOREIGNHANDLE:
            return "CKAS_FOREIGNHANDLE";
        default:
            return "CKAS_UNKNOWN";
    }
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

void ReturnPreparedContext(asIScriptEngine *engine, asIScriptContext *&ctx) {
    if (!engine || !ctx) {
        ctx = nullptr;
        return;
    }

    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
    engine->ReturnContext(ctx);
    ctx = nullptr;
}

ObjectCallOutcome ExecutePreparedObjectMethod(ScriptManager *manager,
                                              int &publicCallbackDepth,
                                              asIScriptObject *object,
                                              asIScriptFunction *function,
                                              const CKAngelScriptMethod *method,
                                              CKAngelScriptWriteArgsCallback writeArgs,
                                              CKAngelScriptReadResultCallback readResult,
                                              CKAngelScriptContextCallback configureContext,
                                              CKAngelScriptContextCallback readContextResult,
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
        ReturnPreparedContext(engine, ctx);
        outcome.Status = CKAS_EXECUTIONFAILED;
        outcome.AngelScriptCode = r;
        outcome.ErrorMessage = "Failed to prepare script object method.";
        return outcome;
    }

    if (configureContext) {
        {
            ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
            status = configureContext(ctx, userData);
        }
        if (status != CKAS_OK) {
            ReturnPreparedContext(engine, ctx);
            outcome.Status = status;
            outcome.ErrorMessage = StatusMessage(status);
            return outcome;
        }
    }

    CKAngelScriptArgWriter writer = {};
    writer.Context = ctx;
    writer.Method = method;
    if (writeArgs) {
        {
            ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
            status = writeArgs(&writer, userData);
        }
        if (status != CKAS_OK) {
            ReturnPreparedContext(engine, ctx);
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
            {
                ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
                status = readResult(&reader, userData);
            }
            if (status != CKAS_OK) {
                ReturnPreparedContext(engine, ctx);
                outcome.Status = status;
                outcome.ErrorMessage = StatusMessage(status);
                return outcome;
            }
        }
        if (readContextResult) {
            {
                ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
                status = readContextResult(ctx, userData);
            }
            if (status != CKAS_OK) {
                ReturnPreparedContext(engine, ctx);
                outcome.Status = status;
                outcome.ErrorMessage = StatusMessage(status);
                return outcome;
            }
        }
        ReturnPreparedContext(engine, ctx);
        outcome.Status = CKAS_OK;
        return outcome;
    }

    if (r == asEXECUTION_SUSPENDED) {
        ctx->Abort();
        ReturnPreparedContext(engine, ctx);
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
    ReturnPreparedContext(engine, ctx);
    (void)flags;
    outcome.Status = CKAS_EXECUTIONFAILED;
    return outcome;
}

bool HasPublicFlag(CKDWORD flags, CKDWORD flag) {
    return (flags & flag) != 0;
}

std::string MakeEngineExtensionConfigGroupName(const char *name) {
    return fmt::format("CKAngelScript.Extension.{}", name ? name : "");
}

CKAS_STATUS StoreStatelessPublicResult(CKAngelScriptResult *out,
                                       CKAS_STATUS status,
                                       int angelScriptCode,
                                       const char *errorMessage) {
    if (out) {
        out->Size = sizeof(*out);
        out->Status = status;
        out->AngelScriptCode = angelScriptCode;
        out->ErrorMessage = errorMessage && errorMessage[0] != '\0' ? errorMessage : nullptr;
        out->StackTrace = nullptr;
        out->CompilerMessages = nullptr;
        out->CompilerMessageCount = 0;
    }
    return status;
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

const char *CopyPublicString(const char *value, std::string &storage) {
    if (!value) {
        return nullptr;
    }
    storage = value;
    return storage.c_str();
}

CKAS_STATUS DispatchMetadata(const CKAngelScriptMetadataEntry &entry,
                             CKDWORD metadataCount,
                             const std::function<const char *(CKDWORD)> &metadataAt,
                             CKAngelScriptMetadataCallback callback,
                             void *userData) {
    if (!callback || !metadataAt) {
        return CKAS_INVALIDARGUMENT;
    }
    CKAngelScriptMetadataEntry publicEntry = entry;
    publicEntry.Size = sizeof(publicEntry);
    publicEntry.MetadataCount = metadataCount;
    std::string nameStorage;
    std::string namespaceStorage;
    std::string declarationStorage;
    std::string parentTypeNameStorage;
    std::string parentTypeNamespaceStorage;
    publicEntry.Name = CopyPublicString(entry.Name, nameStorage);
    publicEntry.Namespace = CopyPublicString(entry.Namespace, namespaceStorage);
    publicEntry.Declaration = CopyPublicString(entry.Declaration, declarationStorage);
    publicEntry.ParentTypeName = CopyPublicString(entry.ParentTypeName, parentTypeNameStorage);
    publicEntry.ParentTypeNamespace = CopyPublicString(entry.ParentTypeNamespace, parentTypeNamespaceStorage);
    for (CKDWORD i = 0; i < metadataCount; ++i) {
        const char *metadata = metadataAt(i);
        const std::string metadataStorage = metadata ? metadata : "";
        const CKAS_STATUS status = callback(&publicEntry, i, metadataStorage.c_str(), userData);
        if (status != CKAS_OK) {
            return status;
        }
    }
    return CKAS_OK;
}

CKAS_STATUS DispatchImport(const CKAngelScriptImportEntry &entry,
                           CKAngelScriptImportCallback callback,
                           void *userData) {
    if (!callback) {
        return CKAS_INVALIDARGUMENT;
    }
    CKAngelScriptImportEntry publicEntry = entry;
    publicEntry.Size = sizeof(publicEntry);
    std::string declarationStorage;
    std::string sourceModuleStorage;
    publicEntry.Declaration = CopyPublicString(entry.Declaration, declarationStorage);
    publicEntry.SourceModuleName = CopyPublicString(entry.SourceModuleName, sourceModuleStorage);
    return callback(&publicEntry, userData);
}

CKAS_STATUS DispatchBoundImportEdge(const CKAngelScriptBoundImportEdge &edge,
                                    CKAngelScriptBoundImportEdgeCallback callback,
                                    void *userData) {
    if (!callback) {
        return CKAS_INVALIDARGUMENT;
    }
    CKAngelScriptBoundImportEdge publicEdge = edge;
    publicEdge.Size = sizeof(publicEdge);
    std::string importModuleStorage;
    std::string sourceModuleStorage;
    std::string functionDeclStorage;
    publicEdge.ImportModuleName = CopyPublicString(edge.ImportModuleName, importModuleStorage);
    publicEdge.SourceModuleName = CopyPublicString(edge.SourceModuleName, sourceModuleStorage);
    publicEdge.FunctionDecl = CopyPublicString(edge.FunctionDecl, functionDeclStorage);
    return callback(&publicEdge, userData);
}

CKAS_STATUS DispatchIncludeEdge(const CKAngelScriptIncludeEdge &edge,
                                CKAngelScriptIncludeEdgeCallback callback,
                                void *userData) {
    if (!callback) {
        return CKAS_INVALIDARGUMENT;
    }
    CKAngelScriptIncludeEdge publicEdge = edge;
    publicEdge.Size = sizeof(publicEdge);
    std::string moduleNameStorage;
    std::string fromSectionStorage;
    std::string toSectionStorage;
    publicEdge.ModuleName = CopyPublicString(edge.ModuleName, moduleNameStorage);
    publicEdge.FromSection = CopyPublicString(edge.FromSection, fromSectionStorage);
    publicEdge.ToSection = CopyPublicString(edge.ToSection, toSectionStorage);
    return callback(&publicEdge, userData);
}

const unsigned long long kFnvOffsetBasis = 14695981039346656037ull;
constexpr unsigned long long kFnvPrime = 1099511628211ull;

void HashBytes(unsigned long long &hash, const void *data, size_t size) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned long long>(bytes[i]);
        hash *= kFnvPrime;
    }
}

void HashString(unsigned long long &hash, const std::string &value) {
    HashBytes(hash, value.data(), value.size());
    const unsigned char terminator = 0;
    HashBytes(hash, &terminator, sizeof(terminator));
}

void HashString(unsigned long long &hash, const char *value) {
    HashString(hash, value ? std::string(value) : std::string());
}

void HashValue(unsigned long long &hash, unsigned long long value) {
    HashBytes(hash, &value, sizeof(value));
}

void HashValue(unsigned long long &hash, CKDWORD value) {
    HashBytes(hash, &value, sizeof(value));
}

void HashBool(unsigned long long &hash, bool value) {
    const unsigned char byte = value ? 1 : 0;
    HashBytes(hash, &byte, sizeof(byte));
}

CKAS_STATUS StatusFromImportBindResult(int code) {
    switch (code) {
        case asSUCCESS:
            return CKAS_OK;
        case asINVALID_ARG:
        case asINVALID_DECLARATION:
            return CKAS_INVALIDARGUMENT;
        case asNO_MODULE:
        case asNO_FUNCTION:
            return CKAS_NOTFOUND;
        case asINVALID_TYPE:
        case asINVALID_INTERFACE:
        case asCANT_BIND_ALL_FUNCTIONS:
            return CKAS_TYPEMISMATCH;
        case asNOT_SUPPORTED:
            return CKAS_UNSUPPORTED;
        default:
            return CKAS_EXECUTIONFAILED;
    }
}

CKAngelScriptResult MakeExecutionResult(CKAngelScriptExecution *execution,
                                      CKAS_STATUS status,
                                      int angelScriptCode,
                                      const std::string &errorMessage,
                                      const std::string &stackTrace) {
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

CKAS_STATUS RunExecution(CKAngelScriptExecution *execution,
                         const CKAngelScriptExecutionStepOptions *options,
                         int &publicCallbackDepth) {
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
    CKAngelScriptContextCallback configureContext = nullptr;
    CKAngelScriptContextCallback readResult = nullptr;
    void *userData = nullptr;
    if (options) {
        configureContext = PublicField(*options,
                                       &CKAngelScriptExecutionStepOptions::ConfigureContext,
                                       static_cast<CKAngelScriptContextCallback>(nullptr));
        readResult = PublicField(*options,
                                 &CKAngelScriptExecutionStepOptions::ReadResult,
                                 static_cast<CKAngelScriptContextCallback>(nullptr));
        userData = PublicField(*options, &CKAngelScriptExecutionStepOptions::UserData, static_cast<void *>(nullptr));
    }
    const ScriptInvocationStatus scriptStatus = execution->Invoker.ExecuteScriptStatus(
        execution->Function,
        [execution, configureContext, userData, &callbackStatus, &publicCallbackDepth](asIScriptContext *ctx) {
            if (execution->HasBehaviorContext && execution->Function && execution->Function->GetParamCount() > 0) {
                const int argStatus = ctx->SetArgObject(0, (void *)&execution->BehaviorContextStorage);
                if (argStatus < 0) {
                    callbackStatus = CKAS_EXECUTIONFAILED;
                    return false;
                }
            }
            if (configureContext) {
                ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
                callbackStatus = configureContext(ctx, userData);
                if (callbackStatus != CKAS_OK) {
                    return false;
                }
            }
            return true;
        },
        [readResult, userData, &callbackStatus, &publicCallbackDepth](asIScriptContext *ctx) {
            if (readResult) {
                ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
                callbackStatus = readResult(ctx, userData);
                return callbackStatus == CKAS_OK;
            }
            return true;
        },
        [configureContext, userData, &callbackStatus, &publicCallbackDepth](asIScriptContext *ctx) {
            if (configureContext) {
                ScriptManagerModuleReplacementInternal::PublicCallbackScope callbackScope(publicCallbackDepth);
                callbackStatus = configureContext(ctx, userData);
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
using ScriptManagerInternal::InitPublicStruct;
using ScriptManagerInternal::DispatchMetadata;
using ScriptManagerInternal::DispatchBoundImportEdge;
using ScriptManagerInternal::DispatchIncludeEdge;
using ScriptManagerInternal::DispatchImport;
using ScriptManagerInternal::HashBool;
using ScriptManagerInternal::HashString;
using ScriptManagerInternal::HashValue;
using ScriptManagerInternal::IsIntOrEnumType;
using ScriptManagerInternal::IsCompatibleObjectHandle;
using ScriptManagerInternal::IsStringType;
using ScriptManagerInternal::kFnvOffsetBasis;
using ScriptManagerInternal::IsValidBorrowedObjectParam;
using ScriptManagerInternal::IsValidObjectHandleParam;
using ScriptManagerInternal::IsValidStringParam;
using ScriptManagerInternal::MakeEngineExtensionConfigGroupName;
using ScriptManagerInternal::MakeExecutionResult;
using ScriptManagerInternal::ObjectCallOutcome;
using ScriptManagerInternal::PublicField;
using ScriptManagerInternal::RunExecution;
using ScriptManagerInternal::StoreStatelessPublicResult;
using ScriptManagerInternal::StatusMessage;
using ScriptManagerInternal::StatusName;
using ScriptManagerInternal::StatusFromImportBindResult;
using ScriptManagerInternal::ValidateArgIndex;

asITypeInfo *FindTypeByNameAndNamespace(asIScriptModule *module,
                                        const char *className,
                                        const char *classNamespace) {
    if (!module || !className || className[0] == '\0') {
        return nullptr;
    }
    if (!classNamespace || classNamespace[0] == '\0') {
        return module->GetTypeInfoByName(className);
    }

    const asUINT typeCount = module->GetObjectTypeCount();
    for (asUINT i = 0; i < typeCount; ++i) {
        asITypeInfo *type = module->GetObjectTypeByIndex(i);
        if (!type) {
            continue;
        }
        const char *name = type->GetName();
        const char *nameSpace = type->GetNamespace();
        if (name && std::strcmp(name, className) == 0 &&
            nameSpace && std::strcmp(nameSpace, classNamespace) == 0) {
            return type;
        }
    }
    return nullptr;
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
    m_ModuleStates.clear();
    m_ScriptCache.Clear();

    m_BehaviorBridge.reset();
    m_Runtime.reset();
    m_AsyncScheduler.reset();
    m_ParameterRegistry.reset();

    if (m_ScriptEngine) {
        m_ScriptEngine->ShutDownAndRelease();
        m_ScriptEngine = nullptr;
    }
    for (ScriptEngineExtensionRegistration &extension : m_EngineExtensions) {
        extension.ActiveInCurrentEngine = false;
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

CKAS_STATUS ScriptManager::SetActiveContextException(const char *message, CKAngelScriptResult *result) {
    if (!message || message[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exception message is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    ctx->SetException(message);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    m_HostCallFilter = callback;
    m_HostCallFilterUserData = userData;
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::RejectHostCall(const char *apiName, CKDWORD flags) {
    if (!m_HostCallFilter) {
        return false;
    }
    return m_HostCallFilter(apiName, flags, m_HostCallFilterUserData) != CKAS_OK;
}

bool ScriptManager::RejectActiveHostCall(const char *apiName, CKDWORD flags) {
    asIScriptContext *ctx = asGetActiveContext();
    if (!ctx) {
        return false;
    }
    asIScriptEngine *engine = ctx->GetEngine();
    ScriptManager *manager = engine ? GetManager(engine) : nullptr;
    return manager ? manager->RejectHostCall(apiName, flags) : false;
}

CKAngelScriptResult ScriptManager::MakeResult(CKAS_STATUS status,
                                            int angelScriptCode,
                                            const std::string &errorMessage,
                                            const std::string &stackTrace,
                                            const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.MakeResult(status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

CKAS_STATUS ScriptManager::StoreResult(CKAngelScriptResult *out,
                                              CKAS_STATUS status,
                                              int angelScriptCode,
                                              const std::string &errorMessage,
                                              const std::string &stackTrace,
                                              const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.StoreResult(out, status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

const CKAngelScriptResult *ScriptManager::GetLastResult() const {
    return m_Diagnostics.GetLastResult();
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

    ScriptEngineExtensionRegistration retained = {};
    retained.Name = name;
    retained.ConfigGroupName = MakeEngineExtensionConfigGroupName(name);
    retained.Register = callback;
    retained.UserData = userData;
    retained.Flags = flags;

    if (m_ScriptEngine && IsInited() && !HasPublicFlag(flags, CKAS_ENGINEEXTENSION_DEFERRED)) {
        std::string message;
        const int code = RegisterEngineExtensionGroup(m_ScriptEngine, retained, message);
        if (code < 0) {
            const std::string summary = message.empty()
                                            ? fmt::format("Engine extension '{}' failed to register (code {}).", name, code)
                                            : message;
            if (m_Context) {
                m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s"), summary.c_str());
            }
            LOG_ERROR("%s", summary.c_str());
            return StoreResult(result, CKAS_EXECUTIONFAILED, code, summary);
        }
    }

    m_EngineExtensions.push_back(retained);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnregisterEngineExtension(const char *name,
                                                     CKAngelScriptResult *result) {
    if (!name || name[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension name is required.");
    }
    for (auto it = m_EngineExtensions.begin(); it != m_EngineExtensions.end(); ++it) {
        if (it->Name == name) {
            if (it->ActiveInCurrentEngine && m_ScriptEngine) {
                std::string message;
                const int code = RemoveEngineExtensionGroup(m_ScriptEngine, *it, message);
                if (code < 0) {
                    if (code == asCONFIG_GROUP_IS_IN_USE) {
                        return StoreResult(result,
                                           CKAS_INUSE,
                                           code,
                                           message.empty()
                                               ? fmt::format("Engine extension '{}' is in use.", name)
                                               : message);
                    }
                    return StoreResult(result,
                                       CKAS_EXECUTIONFAILED,
                                       code,
                                       message.empty()
                                           ? fmt::format("Failed to unregister engine extension '{}' (code {}).", name, code)
                                           : message);
                }
            }
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
    m_Diagnostics.BeginScriptMessageCapture();
}

std::string ScriptManager::EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages) {
    return m_Diagnostics.EndScriptMessageCapture(messages);
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

CKERROR ScriptManager::ResolveScriptFileName(XString &filename) {
    CKPathManager *pm = m_Context->GetPathManager();
    if (m_ScriptPathCategoryIndex == -1) {
        SetupScriptPathCategory();
    }
    return pm->ResolveFileName(filename, m_ScriptPathCategoryIndex);
}

void * ScriptManager::GetCKObjectData(CK_ID id) const {
    return m_CKObjectRetainer.GetData(id);
}

void ScriptManager::SetCKObjectData(CK_ID id, void *data) {
    m_CKObjectRetainer.SetData(id, data);
}

void ScriptManager::ReleaseCKObjectData(CK_ID id) {
    m_CKObjectRetainer.ReleaseData(id);
}

void ScriptManager::ClearCKObjectData() {
    m_CKObjectRetainer.Clear();
}

void ScriptManager::TrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    m_CKObjectRetainer.TrackCallback(id, func);
}

bool ScriptManager::UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    return m_CKObjectRetainer.UntrackCallback(id, func);
}

void ScriptManager::ReleaseCKObjectCallbacks(CK_ID id) {
    m_CKObjectRetainer.ReleaseCallbacks(id);
}

ScriptBehaviorBridge *ScriptManager::GetBehaviorBridge() {
    if (!m_BehaviorBridge) {
        m_BehaviorBridge = std::make_unique<ScriptBehaviorBridge>(this);
    }
    return m_BehaviorBridge.get();
}

