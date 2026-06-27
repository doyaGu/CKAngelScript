#include "ScriptApiSupport.h"

#include <cstring>

#include <fmt/format.h>

#include "ScriptManager.h"

namespace ScriptApiSupport {

namespace {

struct StatusText {
    CKAS_STATUS Status;
    const char *Name;
    const char *Message;
};

constexpr StatusText kStatusTexts[] = {
    {CKAS_OK, "CKAS_OK", "OK."},
    {CKAS_INVALIDARGUMENT, "CKAS_INVALIDARGUMENT", "Invalid argument."},
    {CKAS_NOTINITIALIZED, "CKAS_NOTINITIALIZED", "AngelScript engine is not initialized."},
    {CKAS_NOTFOUND, "CKAS_NOTFOUND", "Requested script item was not found."},
    {CKAS_COMPILEERROR, "CKAS_COMPILEERROR", "Script compile failed."},
    {CKAS_EXECUTIONFAILED, "CKAS_EXECUTIONFAILED", "Script execution failed."},
    {CKAS_SUSPENDED, "CKAS_SUSPENDED", "Script execution suspended."},
    {CKAS_CANCELLED, "CKAS_CANCELLED", "Script execution was cancelled."},
    {CKAS_STALEHANDLE, "CKAS_STALEHANDLE", "Script handle is stale."},
    {CKAS_UNSUPPORTED, "CKAS_UNSUPPORTED", "Operation is unsupported."},
    {CKAS_TYPEMISMATCH, "CKAS_TYPEMISMATCH", "Script type mismatch."},
    {CKAS_BUFFERTOOSMALL, "CKAS_BUFFERTOOSMALL", "Result buffer is too small."},
    {CKAS_INVALIDSTATE, "CKAS_INVALIDSTATE", "Operation is invalid for the current handle state."},
    {CKAS_INUSE, "CKAS_INUSE", "Requested script item is in use."},
    {CKAS_ALREADYEXISTS, "CKAS_ALREADYEXISTS", "Requested script item already exists."},
    {CKAS_AMBIGUOUS, "CKAS_AMBIGUOUS", "Requested script symbol is ambiguous."},
    {CKAS_FOREIGNHANDLE, "CKAS_FOREIGNHANDLE", "Handle belongs to another CKAngelScript manager."},
};

const StatusText *FindStatusText(CKAS_STATUS status) {
    for (const StatusText &text : kStatusTexts) {
        if (text.Status == status) {
            return &text;
        }
    }
    return nullptr;
}

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

} // namespace

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
    const StatusText *text = FindStatusText(status);
    return text ? text->Name : "CKAS_UNKNOWN";
}

const char *StatusMessage(CKAS_STATUS status) {
    const StatusText *text = FindStatusText(status);
    return text ? text->Message : "Unknown CKAngelScript status.";
}

bool ValidateArgIndex(const CKAngelScriptArgWriter *writer, CKDWORD index) {
    return writer &&
           writer->Context &&
           writer->Method &&
           index < writer->Method->ParamTypes.size();
}

namespace {

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

} // namespace

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
            CallbackDepthScope callbackScope(publicCallbackDepth);
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
            CallbackDepthScope callbackScope(publicCallbackDepth);
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
                CallbackDepthScope callbackScope(publicCallbackDepth);
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
                CallbackDepthScope callbackScope(publicCallbackDepth);
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

bool HasUnknownPublicFlags(CKDWORD flags, CKDWORD knownFlags) {
    return (flags & ~knownFlags) != 0;
}

CKAS_STATUS StoreStatelessPublicResult(CKAngelScriptResult *out,
                                       CKAS_STATUS status,
                                       int angelScriptCode,
                                       const char *errorMessage) {
    if (out) {
        out->Size = sizeof(*out);
        out->Status = status;
        out->AngelScriptCode = angelScriptCode;
        out->ErrorMessage = IsNonEmpty(errorMessage) ? errorMessage : nullptr;
        out->StackTrace = nullptr;
        out->CompilerMessages = nullptr;
        out->CompilerMessageCount = 0;
    }
    return status;
}

namespace {

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

} // namespace

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

namespace {

constexpr unsigned long long kFnvPrime = 1099511628211ull;

void HashBytes(unsigned long long &hash, const void *data, size_t size) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned long long>(bytes[i]);
        hash *= kFnvPrime;
    }
}

} // namespace

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
    CKAngelScriptResult result = {};
    result.Size = sizeof(result);
    result.Status = status;
    result.AngelScriptCode = angelScriptCode;
    result.ErrorMessage = nullptr;
    result.StackTrace = nullptr;
    result.CompilerMessages = nullptr;
    result.CompilerMessageCount = 0;
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
                CallbackDepthScope callbackScope(publicCallbackDepth);
                callbackStatus = configureContext(ctx, userData);
                if (callbackStatus != CKAS_OK) {
                    return false;
                }
            }
            return true;
        },
        [readResult, userData, &callbackStatus, &publicCallbackDepth](asIScriptContext *ctx) {
            if (readResult) {
                CallbackDepthScope callbackScope(publicCallbackDepth);
                callbackStatus = readResult(ctx, userData);
                return callbackStatus == CKAS_OK;
            }
            return true;
        },
        [configureContext, userData, &callbackStatus, &publicCallbackDepth](asIScriptContext *ctx) {
            if (configureContext) {
                CallbackDepthScope callbackScope(publicCallbackDepth);
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

asITypeInfo *FindTypeByNameAndNamespace(asIScriptModule *module,
                                        const char *className,
                                        const char *classNamespace) {
    if (!module || !IsNonEmpty(className)) {
        return nullptr;
    }
    if (!IsNonEmpty(classNamespace)) {
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

} // namespace ScriptApiSupport
