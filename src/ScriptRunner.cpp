#include "ScriptRunner.h"

#include "ScriptManager.h"

ScriptRunner::ScriptRunner(ScriptManager *man) : m_ScriptManager(man) {}

ScriptRunner::~ScriptRunner() {
    // Release the AngelScript context if it exists
    if (m_Context) {
        m_Context->Release();
        m_Context = nullptr;
    }
}

bool ScriptRunner::Attach(CKBehavior *behavior, bool runner) {
    Detach(behavior);

    if (!behavior) {
        return false;
    }

    auto *scriptName = (CKSTRING) behavior->GetInputParameterReadDataPtr(0);
    if (!scriptName) {
        return false;
    }

    if (!SetScript(scriptName)) {
        return false;
    }

    if (runner) {
        auto *funcName = (CKSTRING) behavior->GetInputParameterReadDataPtr(1);
        if (funcName) {
            auto *func = GetFunctionByName(funcName);
            func->AddRef();
            behavior->SetLocalParameterValue(1, &func);
        }
    }

    m_Attached = true;
    return true;
}

void ScriptRunner::Detach(CKBehavior *behavior, bool runner) {
    if (!IsAttached()) {
        return;
    }

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

asIScriptContext* ScriptRunner::GetContext() const {
    return m_Context;
}

void ScriptRunner::SetContext(asIScriptContext* ctx) {
    // If there was a previous context, return it first
    if (m_Context) {
        m_Context->Release();
        m_ScriptManager->GetScriptEngine()->ReturnContext(m_Context);
    }

    m_Context = ctx;

    // AddRef to manage lifetime (AngelScript reference counting)
    if (m_Context) {
        m_Context->AddRef();
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

asIScriptFunction *ScriptRunner::GetFunctionByName(const char *name) {
    if (!m_CachedScript) {
        return nullptr;
    }

    asIScriptFunction *func = m_CachedScript->module->GetFunctionByName(name);
    if (!func) {
        return nullptr;
    }

    return func;
}

asIScriptFunction *ScriptRunner::GetFunctionByDecl(const char *decl) {
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
    if (!func) {
        SetErrorMessage("No function to execute.");
        return false;
    }

    auto *engine = func->GetEngine();
    if (!engine) {
        SetErrorMessage("Script engine is null.");
        return false;
    }

    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return false;
    };

    asIScriptContext *ctx = GetContext();
    if (!ctx) {
        ctx = engine->RequestContext();
        if (!ctx) {
            SetErrorMessage("Failed to create AngelScript context.");
            return false;
        }
        SetContext(ctx);
    }

    int r = 0;

    if (func->GetFuncType() == asFUNC_DELEGATE) {
        asIScriptFunction *delegate = func->GetDelegateFunction();
        void *delegateObject = func->GetDelegateObject();
        r = ctx->Prepare(delegate);
        ctx->SetObject(delegateObject);
    } else {
        r = ctx->Prepare(func);
    }

    if (r < 0) {
        SetErrorMessage("Failed to prepare script function.");
        return false;
    }

    if (argsHandler) {
        argsHandler(ctx);
    }

    StartTiming();

    r = ctx->Execute();
    if (r != asEXECUTION_FINISHED) {
        if (r == asEXECUTION_EXCEPTION) {
            // Gather exception info
            asIScriptFunction *exFunc = ctx->GetExceptionFunction();
            std::string exceptMsg = ctx->GetExceptionString();
            std::string stackTrace = "Exception in " + std::string(exFunc->GetDeclaration()) + ": " + exceptMsg;
            SetStackTrace(stackTrace);
            SetErrorMessage("Script Execution Threw Exception.");
        } else if (r == asEXECUTION_SUSPENDED) {
            SetErrorMessage("Script Execution Suspended.");
        } else {
            SetErrorMessage("Script did not complete normally.");
        }
        return false;
    }

    if (retHandler) {
        retHandler(ctx);
    }

    EndTiming();

    return true;
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
    ResetScript();
    SetErrorMessage("");
    SetStackTrace("");
}
