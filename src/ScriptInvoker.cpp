#include "ScriptInvoker.h"

#include <fmt/format.h>

#include "ScriptAsync.h"
#include "ScriptManager.h"

namespace {

std::string BuildContextStackTrace(asIScriptContext *ctx, const char *prefix) {
    if (!ctx) {
        return std::string();
    }

    const char *section = nullptr;
    int col = 0;
    int row = ctx->GetExceptionLineNumber(&col, &section);

    asIScriptFunction *exFunc = ctx->GetExceptionFunction();
    const char *exFuncDecl = exFunc ? exFunc->GetDeclaration() : nullptr;
    const char *exStr = ctx->GetExceptionString();

    std::string stackTrace = fmt::format("Exception in '{}' at {}({},{}): '{}'\n",
        exFuncDecl ? exFuncDecl : "<unknown function>",
        section ? section : "<unknown section>",
        row,
        col,
        exStr ? exStr : "");

    for (asUINT i = 0; i < ctx->GetCallstackSize(); i++) {
        asIScriptFunction *f = ctx->GetFunction(i);
        row = ctx->GetLineNumber(i, &col, &section);
        const char *funcDecl = f ? f->GetDeclaration() : nullptr;
        stackTrace.append(fmt::format("\t{} at {}({},{})\n",
            funcDecl ? funcDecl : "<unknown function>",
            section ? section : "<unknown section>",
            row,
            col));
    }

    (void) prefix;
    return stackTrace;
}

ScriptInvocationStatus HandleExecutionResult(ScriptInvoker *invoker, asIScriptContext *ctx, int result, const char *label) {
    if (result == asEXECUTION_FINISHED) {
        return ScriptInvocationStatus::Finished;
    }
    if (result == asEXECUTION_SUSPENDED) {
        return ScriptInvocationStatus::Suspended;
    }

    if (result == asEXECUTION_EXCEPTION) {
        invoker->SetStackTrace(BuildContextStackTrace(ctx, label));
        invoker->SetErrorMessage(std::string(label ? label : "Script") + " Threw Exception.");
    } else if (result == asEXECUTION_ABORTED) {
        invoker->SetErrorMessage(std::string(label ? label : "Script") + " Aborted.");
    } else {
        invoker->SetErrorMessage(fmt::format("{} failed with result code: {}",
                                            label ? label : "Script",
                                            result));
    }
    return ScriptInvocationStatus::Failed;
}

void ReleaseFinishedContextState(asIScriptContext *ctx) {
    if (ctx && ctx->GetState() == asEXECUTION_FINISHED) {
        ctx->Unprepare();
    }
}

void ReleaseAbortedContextState(asIScriptContext *ctx) {
    if (!ctx) {
        return;
    }
    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    ctx->Unprepare();
}

} // namespace

ScriptInvoker::ScriptInvoker(ScriptManager *man) : m_ScriptManager(man) {}

ScriptInvoker::~ScriptInvoker() {
    ReleaseContext();

    // Release the cached script
    if (m_CachedScript) {
        m_CachedScript = nullptr;
    }
}

asIScriptContext *ScriptInvoker::GetContext() const {
    return m_Context;
}

void ScriptInvoker::SetContext(asIScriptContext* ctx) {
    if (m_Context == ctx) {
        return;
    }

    ReleaseContext();
    m_Context = ctx;
}

void ScriptInvoker::ReleaseContext() {
    if (!m_Context) {
        return;
    }

    auto *ctx = m_Context;
    m_Context = nullptr;

    if (m_ScriptManager && m_ScriptManager->GetAsyncScheduler()) {
        m_ScriptManager->GetAsyncScheduler()->ForgetContext(ctx);
    }
    if (m_ScriptManager && m_ScriptManager->GetScriptEngine()) {
        m_ScriptManager->GetScriptEngine()->ReturnContext(ctx);
    } else {
        ctx->Release();
    }
}

bool ScriptInvoker::SetScript(const char *scriptName) {
    auto cachedScript = m_ScriptManager ? m_ScriptManager->GetCachedScript(scriptName) : nullptr;
    if (!cachedScript || !cachedScript->module) {
        SetErrorMessage("Script module is invalid or failed to compile.");
        return false;
    }

    m_CachedScript = std::move(cachedScript);
    return true;
}

void ScriptInvoker::ResetScript() {
    if (m_CachedScript) {
        m_CachedScript = nullptr;
    }
}

asIScriptModule *ScriptInvoker::GetModule() const {
    if (!m_CachedScript) {
        return nullptr;
    }

    return m_CachedScript->module;
}

asITypeInfo *ScriptInvoker::GetTypeInfoByName(const char *name) const {
    asIScriptModule *module = GetModule();
    if (!module || !name || name[0] == '\0') {
        return nullptr;
    }

    return module->GetTypeInfoByName(name);
}

asIScriptObject *ScriptInvoker::CreateScriptObject(asITypeInfo *type) {
    if (!type) {
        SetErrorMessage("No script type to instantiate.");
        return nullptr;
    }

    asIScriptEngine *engine = type->GetEngine();
    if (!engine) {
        SetErrorMessage("Script type has no engine.");
        return nullptr;
    }

    void *object = engine->CreateScriptObject(type);
    if (!object) {
        SetErrorMessage("Failed to create script object.");
        return nullptr;
    }

    return static_cast<asIScriptObject *>(object);
}

asIScriptFunction *ScriptInvoker::GetFunctionByName(const char *name) const {
    if (!m_CachedScript) {
        return nullptr;
    }

    asIScriptFunction *func = m_CachedScript->module->GetFunctionByName(name);
    if (!func) {
        return nullptr;
    }

    return func;
}

asIScriptFunction *ScriptInvoker::GetFunctionByDecl(const char *decl) const {
    if (!m_CachedScript) {
        return nullptr;
    }

    asIScriptFunction *func = m_CachedScript->module->GetFunctionByDecl(decl);
    if (!func) {
        return nullptr;
    }

    return func;
}

bool ScriptInvoker::ExecuteScript(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler, const ScriptFunctionArgumentHandler &retHandler) {
    return ExecuteScriptStatus(func, argsHandler, retHandler) == ScriptInvocationStatus::Finished;
}

ScriptInvocationStatus ScriptInvoker::ExecuteScriptStatus(asIScriptFunction *func,
                                                          const ScriptFunctionArgumentHandler &argsHandler,
                                                          const ScriptFunctionArgumentHandler &retHandler,
                                                          const ScriptFunctionArgumentHandler &resumeHandler) {
    m_LastResultCode = 0;
    m_ErrorMessage.clear();
    m_StackTrace.clear();

    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptInvocationStatus::Failed;
    }

    if (!func) {
        SetErrorMessage("No function to execute.");
        return ScriptInvocationStatus::Failed;
    }

    auto *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return ScriptInvocationStatus::Failed;
    }

    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return ScriptInvocationStatus::Failed;
        }
        SetContext(ctx);
    }

    int r = 0;

    if (ctx->GetState() == asEXECUTION_SUSPENDED) {
        std::string waitError;
        ScriptAsyncScheduler *scheduler = m_ScriptManager ? m_ScriptManager->GetAsyncScheduler() : nullptr;
        const ScriptAsyncScheduler::ResumeState resume = scheduler
            ? scheduler->PrepareContextResume(ctx, waitError)
            : ScriptAsyncScheduler::ResumeState::Ready;
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            m_LastResultCode = asEXECUTION_SUSPENDED;
            return ScriptInvocationStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            m_LastResultCode = asEXECUTION_ABORTED;
            SetErrorMessage(waitError.empty() ? "Awaited async task failed." : waitError);
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }
        if (resumeHandler) {
            if (!resumeHandler(ctx)) {
                m_LastResultCode = asEXECUTION_ABORTED;
                SetErrorMessage("Script resume handler rejected execution.");
                ReleaseAbortedContextState(ctx);
                return ScriptInvocationStatus::Failed;
            }
        }
    } else {
        if (func->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *delegate = func->GetDelegateFunction();
            if (!delegate) {
                r = asNO_FUNCTION;
            } else {
                void *delegateObject = func->GetDelegateObject();
                r = ctx->Prepare(delegate);
                if (r >= 0) {
                    r = ctx->SetObject(delegateObject);
                }
            }
        } else {
            r = ctx->Prepare(func);
        }

        if (r < 0) {
            m_LastResultCode = r;
            SetErrorMessage("Failed to prepare script function.");
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }

        if (argsHandler) {
            if (!argsHandler(ctx)) {
                m_LastResultCode = asEXECUTION_ABORTED;
                SetErrorMessage("Script argument handler rejected execution.");
                ReleaseAbortedContextState(ctx);
                return ScriptInvocationStatus::Failed;
            }
        }
    }

    if (IsProfiling())
        StartTiming();

    r = ctx->Execute();
    m_LastResultCode = r;
    const ScriptInvocationStatus status = HandleExecutionResult(this, ctx, r, "Script Execution");
    if (status != ScriptInvocationStatus::Finished) {
        if (IsProfiling())
            EndTiming();
        return status;
    }

    if (retHandler) {
        if (!retHandler(ctx)) {
            m_LastResultCode = asEXECUTION_ABORTED;
            SetErrorMessage("Script result handler rejected execution.");
            ReleaseFinishedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }
    }

    if (IsProfiling())
        EndTiming();

    ReleaseFinishedContextState(ctx);
    return ScriptInvocationStatus::Finished;
}

bool ScriptInvoker::ExecuteObjectMethod(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext) {
    return ExecuteObjectMethodStatus(object, func, behcontext) == ScriptInvocationStatus::Finished;
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext) {
    m_LastResultCode = 0;
    m_ErrorMessage.clear();
    m_StackTrace.clear();

    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptInvocationStatus::Failed;
    }

    if (!object) {
        SetErrorMessage("No script object to execute.");
        return ScriptInvocationStatus::Failed;
    }

    if (!func) {
        SetErrorMessage("No object method to execute.");
        return ScriptInvocationStatus::Failed;
    }

    auto *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return ScriptInvocationStatus::Failed;
    }

    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return ScriptInvocationStatus::Failed;
        }
        SetContext(ctx);
    }

    m_BehaviorContextStorage = behcontext;
    int r = 0;
    if (ctx->GetState() == asEXECUTION_SUSPENDED) {
        std::string waitError;
        ScriptAsyncScheduler *scheduler = m_ScriptManager ? m_ScriptManager->GetAsyncScheduler() : nullptr;
        const ScriptAsyncScheduler::ResumeState resume = scheduler
            ? scheduler->PrepareContextResume(ctx, waitError)
            : ScriptAsyncScheduler::ResumeState::Ready;
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            m_LastResultCode = asEXECUTION_SUSPENDED;
            return ScriptInvocationStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            m_LastResultCode = asEXECUTION_ABORTED;
            SetErrorMessage(waitError.empty() ? "Awaited async task failed." : waitError);
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }
    } else {
        r = ctx->Prepare(func);
        if (r < 0) {
            m_LastResultCode = r;
            SetErrorMessage("Failed to prepare script method.");
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }

        r = ctx->SetObject(object);
        if (r < 0) {
            m_LastResultCode = r;
            SetErrorMessage("Failed to bind script method object.");
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }

        if (func->GetParamCount() > 0) {
            r = ctx->SetArgObject(0, (void *) &m_BehaviorContextStorage);
            if (r < 0) {
                m_LastResultCode = r;
                SetErrorMessage("Failed to bind script method behavior context.");
                ReleaseAbortedContextState(ctx);
                return ScriptInvocationStatus::Failed;
            }
        }
    }

    if (IsProfiling())
        StartTiming();

    r = ctx->Execute();
    m_LastResultCode = r;
    const ScriptInvocationStatus status = HandleExecutionResult(this, ctx, r, "Script Method");
    if (status != ScriptInvocationStatus::Finished) {
        if (IsProfiling())
            EndTiming();
        return status;
    }

    if (IsProfiling())
        EndTiming();

    ReleaseFinishedContextState(ctx);
    return ScriptInvocationStatus::Finished;
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(asIScriptObject *object,
                                                              asIScriptFunction *func,
                                                              const ScriptMessage &message,
                                                              const CKBehaviorContext &behcontext) {
    m_LastResultCode = 0;
    m_ErrorMessage.clear();
    m_StackTrace.clear();

    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!object) {
        SetErrorMessage("No script object to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!func) {
        SetErrorMessage("No object method to execute.");
        return ScriptInvocationStatus::Failed;
    }
    asIScriptEngine *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return ScriptInvocationStatus::Failed;
    }
    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return ScriptInvocationStatus::Failed;
        }
        SetContext(ctx);
    }

    int r = 0;
    if (ctx->GetState() == asEXECUTION_SUSPENDED) {
        std::string waitError;
        ScriptAsyncScheduler *scheduler = m_ScriptManager ? m_ScriptManager->GetAsyncScheduler() : nullptr;
        const ScriptAsyncScheduler::ResumeState resume = scheduler
            ? scheduler->PrepareContextResume(ctx, waitError)
            : ScriptAsyncScheduler::ResumeState::Ready;
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            m_LastResultCode = asEXECUTION_SUSPENDED;
            return ScriptInvocationStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            m_LastResultCode = asEXECUTION_ABORTED;
            SetErrorMessage(waitError.empty() ? "Awaited async task failed." : waitError);
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }
    } else {
        m_MessageStorage = message;
        m_BehaviorContextStorage = behcontext;
        r = ctx->Prepare(func);
        if (r >= 0) {
            r = ctx->SetObject(object);
        }
        if (r >= 0) {
            r = ctx->SetArgObject(0, &m_MessageStorage);
        }
        if (r >= 0) {
            r = ctx->SetArgObject(1, &m_BehaviorContextStorage);
        }
        if (r < 0) {
            m_LastResultCode = r;
            SetErrorMessage("Failed to prepare script message method.");
            ReleaseAbortedContextState(ctx);
            return ScriptInvocationStatus::Failed;
        }
    }

    if (IsProfiling())
        StartTiming();

    r = ctx->Execute();
    m_LastResultCode = r;
    const ScriptInvocationStatus status = HandleExecutionResult(this, ctx, r, "Script Message");
    if (status != ScriptInvocationStatus::Finished) {
        if (IsProfiling())
            EndTiming();
        return status;
    }

    if (IsProfiling())
        EndTiming();

    ReleaseFinishedContextState(ctx);
    return ScriptInvocationStatus::Finished;
}

bool ScriptInvoker::IsContextSuspended() const {
    return m_Context && m_Context->GetState() == asEXECUTION_SUSPENDED;
}

void ScriptInvoker::AbortContext() {
    if (m_Context) {
        m_Context->Abort();
    }
    ReleaseContext();
}

void ScriptInvoker::StartTiming() {
    m_StartTime = std::chrono::high_resolution_clock::now();
}

void ScriptInvoker::EndTiming() {
    auto endTime = std::chrono::high_resolution_clock::now();
    m_ElapsedMs = std::chrono::duration<double, std::milli>(endTime - m_StartTime).count();
}

double ScriptInvoker::GetElapsedTimeMs() const {
    return m_ElapsedMs;
}

const std::string &ScriptInvoker::GetErrorMessage() const {
    return m_ErrorMessage;
}

void ScriptInvoker::SetErrorMessage(const std::string& msg) {
    m_ErrorMessage = msg;
}

const std::string &ScriptInvoker::GetStackTrace() const {
    return m_StackTrace;
}

void ScriptInvoker::SetStackTrace(const std::string& trace) {
    m_StackTrace = trace;
}

void ScriptInvoker::Reset() {
    ReleaseContext();
    ResetScript();
    SetErrorMessage("");
    SetStackTrace("");
    m_LastResultCode = 0;
}
