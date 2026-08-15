#include "ScriptInvoker.h"

#include <utility>

#include "ScriptBehaviorFrame.h"
#include "ScriptManager.h"

ScriptInvoker::ScriptInvoker(ScriptManager *manager)
    : m_ScriptManager(manager),
      m_Invocation(manager) {}

ScriptInvoker::~ScriptInvoker() {
    ReleaseContext();
    m_CachedScript.reset();
}

asIScriptContext *ScriptInvoker::GetContext() const {
    return m_Invocation.GetContext();
}

void ScriptInvoker::ReleaseContext() {
    m_Invocation.Cancel();
    ReleaseBehaviorFrame();
}

void ScriptInvoker::ReleaseBehaviorFrame() {
    if (m_BehaviorFrameStorage) {
        m_BehaviorFrameStorage->Release();
        m_BehaviorFrameStorage = nullptr;
    }
}

ScriptInvocationStatus ScriptInvoker::ResumePreparedObjectContext(
    const CKBehaviorContext &behaviorContext,
    const char *label,
    const ScriptFunctionArgumentHandler &resultHandler) {
    m_BehaviorContextStorage = behaviorContext;
    if (m_BehaviorFrameStorage) {
        m_BehaviorFrameStorage->Refresh(behaviorContext);
    }

    ScriptInvocationSpec spec;
    spec.Label = label;
    spec.Result = resultHandler;
    spec.AsyncOwnerId = m_AsyncOwnerId;
    if (IsProfiling()) {
        spec.BeforeExecute = [this]() { StartTiming(); };
        spec.AfterExecute = [this]() { EndTiming(); };
    }
    const ScriptInvocationStatus status = m_Invocation.Resume(spec);
    if (status != ScriptInvocationStatus::Suspended) {
        ReleaseBehaviorFrame();
    }
    return status;
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
    m_CachedScript.reset();
}

asIScriptModule *ScriptInvoker::GetModule() const {
    return m_CachedScript ? m_CachedScript->module : nullptr;
}

asITypeInfo *ScriptInvoker::GetTypeInfoByName(const char *name) const {
    asIScriptModule *module = GetModule();
    return module && name && name[0] != '\0' ? module->GetTypeInfoByName(name) : nullptr;
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
    asIScriptModule *module = GetModule();
    return module ? module->GetFunctionByName(name) : nullptr;
}

asIScriptFunction *ScriptInvoker::GetFunctionByDecl(const char *declaration) const {
    asIScriptModule *module = GetModule();
    return module ? module->GetFunctionByDecl(declaration) : nullptr;
}

bool ScriptInvoker::ExecuteScript(asIScriptFunction *function,
                                  const ScriptFunctionArgumentHandler &argumentHandler,
                                  const ScriptFunctionArgumentHandler &resultHandler) {
    return ExecuteScriptStatus(function, argumentHandler, resultHandler) == ScriptInvocationStatus::Finished;
}

ScriptInvocationStatus ScriptInvoker::ExecuteScriptStatus(
    asIScriptFunction *function,
    const ScriptFunctionArgumentHandler &argumentHandler,
    const ScriptFunctionArgumentHandler &resultHandler,
    const ScriptFunctionArgumentHandler &resumeHandler) {
    ScriptInvocationSpec spec;
    spec.Label = "Script Execution";
    spec.Prepare = argumentHandler;
    spec.Result = resultHandler;
    spec.Resume = resumeHandler;
    spec.AsyncOwnerId = m_AsyncOwnerId;
    if (IsProfiling()) {
        spec.BeforeExecute = [this]() { StartTiming(); };
        spec.AfterExecute = [this]() { EndTiming(); };
    }
    if (!function) {
        return m_Invocation.Start(nullptr, spec);
    }
    return m_Invocation.IsSuspended()
        ? m_Invocation.Resume(spec)
        : m_Invocation.Start(function, spec);
}

bool ScriptInvoker::ExecuteObjectMethod(asIScriptObject *object,
                                        asIScriptFunction *function,
                                        const CKBehaviorContext &behaviorContext) {
    return ExecuteObjectMethodStatus(object, function, behaviorContext) == ScriptInvocationStatus::Finished;
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const CKBehaviorContext &behaviorContext) {
    return ExecuteObjectMethodStatus(object,
                                     function,
                                     behaviorContext,
                                     ScriptBehaviorArgumentKind::BehaviorContext,
                                     nullptr);
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const CKBehaviorContext &behaviorContext,
    ScriptBehaviorArgumentKind argumentKind) {
    return ExecuteObjectMethodStatus(object, function, behaviorContext, argumentKind, nullptr);
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const CKBehaviorContext &behaviorContext,
    const ScriptFunctionArgumentHandler &resultHandler) {
    return ExecuteObjectMethodStatus(object,
                                     function,
                                     behaviorContext,
                                     ScriptBehaviorArgumentKind::BehaviorContext,
                                     resultHandler);
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const CKBehaviorContext &behaviorContext,
    ScriptBehaviorArgumentKind argumentKind,
    const ScriptFunctionArgumentHandler &resultHandler) {
    m_Invocation.ClearDiagnostics();
    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!object) {
        SetErrorMessage("No script object to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!function) {
        SetErrorMessage("No object method to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (m_Invocation.IsSuspended()) {
        return ResumePreparedObjectContext(behaviorContext, "Script Method", resultHandler);
    }

    m_BehaviorContextStorage = behaviorContext;
    ReleaseBehaviorFrame();
    if (argumentKind == ScriptBehaviorArgumentKind::BehaviorFrame) {
        m_BehaviorFrameStorage = new BehaviorFrame(behaviorContext);
    }

    ScriptInvocationSpec spec;
    spec.Label = "Script Method";
    spec.AsyncOwnerId = m_AsyncOwnerId;
    spec.Prepare = [this, object, function, argumentKind](asIScriptContext *context) {
        int result = context->SetObject(object);
        if (result < 0) {
            SetErrorMessage("Failed to bind script method object.");
            return false;
        }
        if (function->GetParamCount() == 0) {
            return true;
        }
        void *argument = argumentKind == ScriptBehaviorArgumentKind::BehaviorFrame
            ? static_cast<void *>(m_BehaviorFrameStorage)
            : static_cast<void *>(&m_BehaviorContextStorage);
        result = context->SetArgObject(0, argument);
        if (result < 0) {
            SetErrorMessage(argumentKind == ScriptBehaviorArgumentKind::BehaviorFrame
                ? "Failed to bind script method behavior frame."
                : "Failed to bind script method behavior context.");
            return false;
        }
        return true;
    };
    spec.Result = resultHandler;
    if (IsProfiling()) {
        spec.BeforeExecute = [this]() { StartTiming(); };
        spec.AfterExecute = [this]() { EndTiming(); };
    }

    const ScriptInvocationStatus status = m_Invocation.Start(function, spec);
    if (status != ScriptInvocationStatus::Suspended) {
        ReleaseBehaviorFrame();
    }
    return status;
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const ScriptMessage &message,
    const CKBehaviorContext &behaviorContext) {
    return ExecuteObjectMethodStatus(object,
                                     function,
                                     message,
                                     behaviorContext,
                                     ScriptBehaviorArgumentKind::BehaviorContext);
}

ScriptInvocationStatus ScriptInvoker::ExecuteObjectMethodStatus(
    asIScriptObject *object,
    asIScriptFunction *function,
    const ScriptMessage &message,
    const CKBehaviorContext &behaviorContext,
    ScriptBehaviorArgumentKind argumentKind) {
    m_Invocation.ClearDiagnostics();
    if (!m_CachedScript) {
        SetErrorMessage("No script to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!object) {
        SetErrorMessage("No script object to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (!function) {
        SetErrorMessage("No object method to execute.");
        return ScriptInvocationStatus::Failed;
    }
    if (m_Invocation.IsSuspended()) {
        return ResumePreparedObjectContext(behaviorContext, "Script Message", nullptr);
    }

    m_MessageStorage = message;
    m_BehaviorContextStorage = behaviorContext;
    ReleaseBehaviorFrame();
    if (argumentKind == ScriptBehaviorArgumentKind::BehaviorFrame) {
        m_BehaviorFrameStorage = new BehaviorFrame(behaviorContext);
    }

    ScriptInvocationSpec spec;
    spec.Label = "Script Message";
    spec.AsyncOwnerId = m_AsyncOwnerId;
    spec.Prepare = [this, object, argumentKind](asIScriptContext *context) {
        int result = context->SetObject(object);
        if (result >= 0) {
            result = context->SetArgObject(0, &m_MessageStorage);
        }
        if (result >= 0) {
            void *argument = argumentKind == ScriptBehaviorArgumentKind::BehaviorFrame
                ? static_cast<void *>(m_BehaviorFrameStorage)
                : static_cast<void *>(&m_BehaviorContextStorage);
            result = context->SetArgObject(1, argument);
        }
        if (result < 0) {
            SetErrorMessage("Failed to prepare script message method.");
            return false;
        }
        return true;
    };
    if (IsProfiling()) {
        spec.BeforeExecute = [this]() { StartTiming(); };
        spec.AfterExecute = [this]() { EndTiming(); };
    }

    const ScriptInvocationStatus status = m_Invocation.Start(function, spec);
    if (status != ScriptInvocationStatus::Suspended) {
        ReleaseBehaviorFrame();
    }
    return status;
}

ScriptInvocationStatus ScriptInvoker::ResumeObjectMessageStatus(const CKBehaviorContext &behaviorContext) {
    return ResumePreparedObjectContext(behaviorContext, "Script Message", nullptr);
}

bool ScriptInvoker::IsContextSuspended() const {
    return m_Invocation.IsSuspended();
}

void ScriptInvoker::AbortActiveContext() {
    m_Invocation.AbortActive();
}

void ScriptInvoker::AbortContext() {
    ReleaseContext();
}

void ScriptInvoker::StartTiming() {
    m_StartTime = std::chrono::high_resolution_clock::now();
}

void ScriptInvoker::EndTiming() {
    const auto endTime = std::chrono::high_resolution_clock::now();
    m_ElapsedMs = std::chrono::duration<double, std::milli>(endTime - m_StartTime).count();
}

double ScriptInvoker::GetElapsedTimeMs() const {
    return m_ElapsedMs;
}

const std::string &ScriptInvoker::GetErrorMessage() const {
    return m_Invocation.GetErrorMessage();
}

void ScriptInvoker::SetErrorMessage(const std::string &message) {
    m_Invocation.SetErrorMessage(message);
}

const std::string &ScriptInvoker::GetStackTrace() const {
    return m_Invocation.GetStackTrace();
}

void ScriptInvoker::SetStackTrace(const std::string &trace) {
    m_Invocation.SetStackTrace(trace);
}

void ScriptInvoker::Reset() {
    ReleaseContext();
    ResetScript();
    m_Invocation.ClearDiagnostics();
}
