#include "ScriptInvocation.h"

#include <utility>

#include <fmt/format.h>

#include "ScriptAsync.h"
#include "ScriptManager.h"

namespace {

std::string BuildContextStackTrace(asIScriptContext *context) {
    if (!context) {
        return {};
    }

    const char *section = nullptr;
    int column = 0;
    int row = context->GetExceptionLineNumber(&column, &section);
    asIScriptFunction *exceptionFunction = context->GetExceptionFunction();
    const char *declaration = exceptionFunction ? exceptionFunction->GetDeclaration() : nullptr;
    const char *exception = context->GetExceptionString();

    std::string trace = fmt::format("Exception in '{}' at {}({},{}): '{}'\n",
                                    declaration ? declaration : "<unknown function>",
                                    section ? section : "<unknown section>",
                                    row,
                                    column,
                                    exception ? exception : "");
    for (asUINT i = 0; i < context->GetCallstackSize(); ++i) {
        asIScriptFunction *function = context->GetFunction(i);
        row = context->GetLineNumber(i, &column, &section);
        trace.append(fmt::format("\t{} at {}({},{})\n",
                                 function ? function->GetDeclaration() : "<unknown function>",
                                 section ? section : "<unknown section>",
                                 row,
                                 column));
    }
    return trace;
}

} // namespace

ScriptInvocationContext::ScriptInvocationContext(ScriptManager *manager)
    : m_Manager(manager) {}

ScriptInvocationContext::~ScriptInvocationContext() {
    Cancel();
}

void ScriptInvocationContext::ClearDiagnostics() {
    m_LastResultCode = 0;
    m_ErrorMessage.clear();
    m_StackTrace.clear();
}

bool ScriptInvocationContext::Acquire(asIScriptEngine *engine) {
    if (m_Context) {
        return true;
    }
    if (!engine) {
        m_ErrorMessage = "Script engine is null.";
        return false;
    }
    m_Context = engine->RequestContext();
    if (!m_Context) {
        m_ErrorMessage = "Failed to request AngelScript context.";
        return false;
    }
    return true;
}

ScriptInvocationStatus ScriptInvocationContext::Fail(int resultCode,
                                                     std::string message,
                                                     bool resetContext) {
    m_LastResultCode = resultCode;
    m_ErrorMessage = std::move(message);
    if (resetContext) {
        ResetPreparedContext();
    }
    return ScriptInvocationStatus::Failed;
}

ScriptInvocationStatus ScriptInvocationContext::Start(asIScriptFunction *function,
                                                       const ScriptInvocationSpec &spec) {
    ClearDiagnostics();
    if (!function) {
        return Fail(asNO_FUNCTION, "No function to execute.", false);
    }
    if (IsSuspended()) {
        return Fail(asCONTEXT_ACTIVE,
                    "Cannot start a script call while another call is suspended.",
                    false);
    }
    if (!Acquire(function->GetEngine())) {
        return ScriptInvocationStatus::Failed;
    }

    int result = 0;
    if (function->GetFuncType() == asFUNC_DELEGATE) {
        asIScriptFunction *delegate = function->GetDelegateFunction();
        if (!delegate) {
            result = asNO_FUNCTION;
        } else {
            result = m_Context->Prepare(delegate);
            if (result >= 0) {
                result = m_Context->SetObject(function->GetDelegateObject());
            }
        }
    } else {
        result = m_Context->Prepare(function);
    }
    if (result < 0) {
        return Fail(result, "Failed to prepare script function.", true);
    }
    if (spec.Prepare && !spec.Prepare(m_Context)) {
        std::string message = m_ErrorMessage.empty()
            ? "Script argument handler rejected execution."
            : m_ErrorMessage;
        return Fail(asEXECUTION_ABORTED,
                    std::move(message),
                    true);
    }
    return ExecutePrepared(spec);
}

ScriptInvocationStatus ScriptInvocationContext::Resume(const ScriptInvocationSpec &spec) {
    ClearDiagnostics();
    if (!IsSuspended()) {
        return Fail(asCONTEXT_NOT_PREPARED,
                    "No suspended script context to resume.",
                    false);
    }

    std::string waitError;
    ScriptAsyncScheduler *scheduler = m_Manager ? m_Manager->GetAsyncScheduler() : nullptr;
    if (!scheduler) {
        return Fail(asEXECUTION_ABORTED,
                    "Awaited async task was cancelled because the scheduler shut down.",
                    true);
    }
    const ScriptAsyncScheduler::ResumeState state =
        scheduler->PrepareContextResume(m_Context, waitError);
    if (state == ScriptAsyncScheduler::ResumeState::Pending) {
        m_LastResultCode = asEXECUTION_SUSPENDED;
        return ScriptInvocationStatus::Suspended;
    }
    if (state == ScriptAsyncScheduler::ResumeState::Failed) {
        return Fail(asEXECUTION_ABORTED,
                    waitError.empty() ? "Awaited async task failed." : std::move(waitError),
                    true);
    }
    if (spec.Resume && !spec.Resume(m_Context)) {
        std::string message = m_ErrorMessage.empty()
            ? "Script resume handler rejected execution."
            : m_ErrorMessage;
        return Fail(asEXECUTION_ABORTED,
                    std::move(message),
                    true);
    }
    return ExecutePrepared(spec);
}

ScriptInvocationStatus ScriptInvocationContext::ExecutePrepared(const ScriptInvocationSpec &spec) {
    ScriptAsyncScheduler *scheduler = m_Manager ? m_Manager->GetAsyncScheduler() : nullptr;
    if (scheduler) {
        scheduler->BeginOwnerExecution(m_Context, spec.AsyncOwnerId);
    }
    if (spec.BeforeExecute) {
        spec.BeforeExecute();
    }
    const int result = m_Context->Execute();
    if (spec.AfterExecute) {
        spec.AfterExecute();
    }
    if (scheduler) {
        scheduler->EndOwnerExecution(m_Context);
    }
    m_LastResultCode = result;

    if (result == asEXECUTION_SUSPENDED) {
        return ScriptInvocationStatus::Suspended;
    }
    if (result != asEXECUTION_FINISHED) {
        const char *label = spec.Label ? spec.Label : "Script";
        if (result == asEXECUTION_EXCEPTION) {
            m_StackTrace = BuildContextStackTrace(m_Context);
            return Fail(result, std::string(label) + " Threw Exception.", true);
        }
        if (result == asEXECUTION_ABORTED) {
            return Fail(result, std::string(label) + " Aborted.", true);
        }
        return Fail(result,
                    fmt::format("{} failed with result code: {}", label, result),
                    true);
    }

    if (spec.Result && !spec.Result(m_Context)) {
        std::string message = m_ErrorMessage.empty()
            ? "Script result handler rejected execution."
            : m_ErrorMessage;
        return Fail(asEXECUTION_ABORTED,
                    std::move(message),
                    true);
    }
    ResetPreparedContext();
    return ScriptInvocationStatus::Finished;
}

bool ScriptInvocationContext::IsSuspended() const {
    return m_Context && m_Context->GetState() == asEXECUTION_SUSPENDED;
}

void ScriptInvocationContext::AbortActive() {
    if (!m_Context) {
        return;
    }
    const int state = m_Context->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        m_Context->Abort();
    }
}

void ScriptInvocationContext::ResetPreparedContext() {
    if (!m_Context) {
        return;
    }
    AbortActive();
    m_Context->Unprepare();
}

void ScriptInvocationContext::Cancel() {
    if (!m_Context) {
        return;
    }
    asIScriptContext *context = m_Context;
    m_Context = nullptr;
    if (m_Manager && m_Manager->GetAsyncScheduler()) {
        m_Manager->GetAsyncScheduler()->ForgetContext(context);
    }
    const int state = context->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        context->Abort();
    }
    context->Unprepare();
    if (asIScriptEngine *engine = context->GetEngine()) {
        engine->ReturnContext(context);
    } else {
        context->Release();
    }
}
