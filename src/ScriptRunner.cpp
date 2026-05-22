#include "ScriptRunner.h"

#include <fmt/format.h>

#include "ScriptAsync.h"
#include "ScriptManager.h"

namespace ScriptRunnerInternal {

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

ScriptExecutionStatus HandleExecutionResult(ScriptRunner *runner, asIScriptContext *ctx, int result, const char *label) {
    if (result == asEXECUTION_FINISHED) {
        return ScriptExecutionStatus::Finished;
    }
    if (result == asEXECUTION_SUSPENDED) {
        return ScriptExecutionStatus::Suspended;
    }

    if (result == asEXECUTION_EXCEPTION) {
        runner->SetStackTrace(BuildContextStackTrace(ctx, label));
        runner->SetErrorMessage(std::string(label ? label : "Script") + " Threw Exception.");
    } else if (result == asEXECUTION_ABORTED) {
        runner->SetErrorMessage(std::string(label ? label : "Script") + " Aborted.");
    } else {
        runner->SetErrorMessage(fmt::format("{} failed with result code: {}",
                                            label ? label : "Script",
                                            result));
    }
    return ScriptExecutionStatus::Failed;
}

} // namespace ScriptRunnerInternal

ScriptRunner::ScriptRunner(ScriptManager *man) : m_ScriptManager(man) {}

ScriptRunner::~ScriptRunner() {
    ReleaseContext();

    // Release the cached script
    if (m_CachedScript) {
        m_CachedScript = nullptr;
    }
}

bool ScriptRunner::Attach(CKBehavior *behavior, bool runner) {
    Detach(behavior);

    if (!behavior) {
        SetErrorMessage("No behavior to attach.");
        return false;
    }

    auto *scriptName = (CKSTRING) behavior->GetInputParameterReadDataPtr(0);
    if (!scriptName || scriptName[0] == '\0') {
        SetErrorMessage("No script module specified.");
        return false;
    }

    if (!SetScript(scriptName)) {
        return false;
    }

    if (runner) {
        auto *funcName = (CKSTRING) behavior->GetInputParameterReadDataPtr(1);
        if (!funcName || funcName[0] == '\0') {
            SetErrorMessage("No function specified.");
            ResetScript();
            return false;
        }

        auto *func = GetFunctionByName(funcName);
        if (!func) {
            SetErrorMessage("Function not found: " + std::string(funcName));
            ResetScript();
            return false;
        }

        func->AddRef();
        behavior->SetLocalParameterValue(1, &func);
    }

    m_Attached = true;
    return true;
}

void ScriptRunner::Detach(CKBehavior *behavior, bool runner) {
    if (!IsAttached()) {
        ReleaseContext();
        return;
    }

    ReleaseContext();
    ResetScript();

    if (runner && behavior) {
        asIScriptFunction *func = nullptr;
        behavior->GetLocalParameterValue(1, &func);
        if (func) {
            func->Release();
            func = nullptr;
        }
        behavior->SetLocalParameterValue(1, &func);
    }

    m_Attached = false;
}

asIScriptContext *ScriptRunner::GetContext() const {
    return m_Context;
}

void ScriptRunner::SetContext(asIScriptContext* ctx) {
    if (m_Context == ctx) {
        return;
    }

    ReleaseContext();
    m_Context = ctx;
}

void ScriptRunner::ReleaseContext() {
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

bool ScriptRunner::SetScript(const char *scriptName) {
    auto &cache = m_ScriptManager->GetScriptCache();
    auto cachedScript = cache.GetCachedScript(scriptName);
    if (!cachedScript || !cachedScript->module) {
        SetErrorMessage("Script module is invalid or failed to compile.");
        return false;
    }

    m_CachedScript = std::move(cachedScript);
    return true;
}

void ScriptRunner::ResetScript() {
    if (m_CachedScript) {
        m_CachedScript = nullptr;
    }
}

asIScriptModule *ScriptRunner::GetModule() const {
    if (!m_CachedScript) {
        return nullptr;
    }

    return m_CachedScript->module;
}

asITypeInfo *ScriptRunner::GetTypeInfoByName(const char *name) const {
    asIScriptModule *module = GetModule();
    if (!module || !name || name[0] == '\0') {
        return nullptr;
    }

    return module->GetTypeInfoByName(name);
}

asIScriptObject *ScriptRunner::CreateScriptObject(asITypeInfo *type) {
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

asIScriptFunction *ScriptRunner::GetFunctionByName(const char *name) const {
    if (!m_CachedScript) {
        return nullptr;
    }

    asIScriptFunction *func = m_CachedScript->module->GetFunctionByName(name);
    if (!func) {
        return nullptr;
    }

    return func;
}

asIScriptFunction *ScriptRunner::GetFunctionByDecl(const char *decl) const {
    if (!m_CachedScript) {
        return nullptr;
    }

    asIScriptFunction *func = m_CachedScript->module->GetFunctionByDecl(decl);
    if (!func) {
        return nullptr;
    }

    return func;
}

bool ScriptRunner::ExecuteScript(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler, const ScriptFunctionArgumentHandler &retHandler) {
    return ExecuteScriptStatus(func, argsHandler, retHandler) == ScriptExecutionStatus::Finished;
}

ScriptExecutionStatus ScriptRunner::ExecuteScriptStatus(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler, const ScriptFunctionArgumentHandler &retHandler) {
    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptExecutionStatus::Failed;
    }

    if (!func) {
        SetErrorMessage("No function to execute.");
        return ScriptExecutionStatus::Failed;
    }

    auto *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return ScriptExecutionStatus::Failed;
    }

    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return ScriptExecutionStatus::Failed;
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
            return ScriptExecutionStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            SetErrorMessage(waitError.empty() ? "Awaited async task failed." : waitError);
            ctx->Abort();
            return ScriptExecutionStatus::Failed;
        }
    } else {
        if (func->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *delegate = func->GetDelegateFunction();
            void *delegateObject = func->GetDelegateObject();
            r = ctx->Prepare(delegate);
            if (r >= 0)
                ctx->SetObject(delegateObject);
        } else {
            r = ctx->Prepare(func);
        }

        if (r < 0) {
            SetErrorMessage("Failed to prepare script function.");
            return ScriptExecutionStatus::Failed;
        }

        if (argsHandler) {
            argsHandler(ctx);
        }
    }

    if (IsProfiling())
        StartTiming();

    r = ctx->Execute();
    const ScriptExecutionStatus status = ScriptRunnerInternal::HandleExecutionResult(this, ctx, r, "Script Execution");
    if (status != ScriptExecutionStatus::Finished) {
        if (IsProfiling())
            EndTiming();
        return status;
    }

    if (retHandler) {
        retHandler(ctx);
    }

    if (IsProfiling())
        EndTiming();

    return ScriptExecutionStatus::Finished;
}

bool ScriptRunner::ExecuteObjectMethod(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext) {
    return ExecuteObjectMethodStatus(object, func, behcontext) == ScriptExecutionStatus::Finished;
}

ScriptExecutionStatus ScriptRunner::ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext) {
    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptExecutionStatus::Failed;
    }

    if (!object) {
        SetErrorMessage("No script object to execute.");
        return ScriptExecutionStatus::Failed;
    }

    if (!func) {
        SetErrorMessage("No object method to execute.");
        return ScriptExecutionStatus::Failed;
    }

    auto *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return ScriptExecutionStatus::Failed;
    }

    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return ScriptExecutionStatus::Failed;
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
            return ScriptExecutionStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            SetErrorMessage(waitError.empty() ? "Awaited async task failed." : waitError);
            ctx->Abort();
            return ScriptExecutionStatus::Failed;
        }
    } else {
        r = ctx->Prepare(func);
        if (r < 0) {
            SetErrorMessage("Failed to prepare script method.");
            return ScriptExecutionStatus::Failed;
        }

        r = ctx->SetObject(object);
        if (r < 0) {
            SetErrorMessage("Failed to bind script method object.");
            return ScriptExecutionStatus::Failed;
        }

        if (func->GetParamCount() > 0) {
            ctx->SetArgObject(0, (void *) &m_BehaviorContextStorage);
        }
    }

    if (IsProfiling())
        StartTiming();

    r = ctx->Execute();
    const ScriptExecutionStatus status = ScriptRunnerInternal::HandleExecutionResult(this, ctx, r, "Script Method");
    if (status != ScriptExecutionStatus::Finished) {
        if (IsProfiling())
            EndTiming();
        return status;
    }

    if (IsProfiling())
        EndTiming();

    return ScriptExecutionStatus::Finished;
}

bool ScriptRunner::IsContextSuspended() const {
    return m_Context && m_Context->GetState() == asEXECUTION_SUSPENDED;
}

void ScriptRunner::AbortContext() {
    if (m_Context) {
        m_Context->Abort();
    }
    ReleaseContext();
}

void ScriptRunner::StartTiming() {
    m_StartTime = std::chrono::high_resolution_clock::now();
}

void ScriptRunner::EndTiming() {
    auto endTime = std::chrono::high_resolution_clock::now();
    m_ElapsedMs = std::chrono::duration<double, std::milli>(endTime - m_StartTime).count();
}

double ScriptRunner::GetElapsedTimeMs() const {
    return m_ElapsedMs;
}

const std::string &ScriptRunner::GetErrorMessage() const {
    return m_ErrorMessage;
}

void ScriptRunner::SetErrorMessage(const std::string& msg) {
    m_ErrorMessage = msg;
}

const std::string &ScriptRunner::GetStackTrace() const {
    return m_StackTrace;
}

void ScriptRunner::SetStackTrace(const std::string& trace) {
    m_StackTrace = trace;
}

void ScriptRunner::Reset() {
    ReleaseContext();
    ResetScript();
    SetErrorMessage("");
    SetStackTrace("");
}
