#include "ScriptManager.h"

#include <string>
#include <utility>

#include <fmt/format.h>

#include "ScriptApiSupport.h"

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_EngineHost.Engine();
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
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    *outEngine = engine;
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
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != engine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    *outContext = ctx;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetActiveContextException(const char *message, CKAngelScriptResult *result) {
    if (!ScriptApiSupport::IsNonEmpty(message)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Exception message is required.");
    }
    asIScriptEngine *engine = GetScriptEngine();
    if (!engine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptContext *ctx = GetActiveContext();
    if (!ctx || ctx->GetEngine() != engine) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "No active context belongs to this CKAngelScript manager.");
    }
    ctx->SetException(message);
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SetHostCallFilter(CKAngelScriptHostCallFilterCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    m_EngineHost.SetHostCallFilter(callback, userData);
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::RejectHostCall(const char *apiName, CKDWORD flags) {
    return m_EngineHost.RejectHostCall(apiName, flags);
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

CKAS_STATUS ScriptManager::RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                                   CKAngelScriptResult *result) {
    return m_EngineHost.RegisterExtension(*this, extension, result);
}

CKAS_STATUS ScriptManager::UnregisterEngineExtension(const char *name,
                                                     CKAngelScriptResult *result) {
    return m_EngineHost.UnregisterExtension(*this, name, result);
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

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    CKAS_MESSAGETYPE publicType = CKAS_MESSAGE_INFORMATION;
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            publicType = CKAS_MESSAGE_ERROR;
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            publicType = CKAS_MESSAGE_WARNING;
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            publicType = CKAS_MESSAGE_INFORMATION;
            break;
    }
    const std::string formatted = fmt::format("{}({},{}): {}: {}",
                                              msg.section ? msg.section : "<unknown section>",
                                              msg.row,
                                              msg.col,
                                              type,
                                              msg.message ? msg.message : "");
    if (m_Diagnostics.IsCapturingScriptMessages()) {
        CapturedScriptMessage captured;
        captured.Section = msg.section ? msg.section : "";
        captured.Row = msg.row;
        captured.Column = msg.col;
        captured.Type = publicType;
        captured.Message = msg.message ? msg.message : "";
        m_Diagnostics.CaptureScriptMessage(formatted, std::move(captured));
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
    return m_EngineHost.RequestContext(*this);
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    m_EngineHost.ReturnContext(*this, ctx);
}
