#include "ScriptInitialLifecycle.h"

namespace {

ScriptInitialPhase LastInvocablePhase(ScriptInitialPhase phase) {
    return phase == ScriptInitialPhase::Ready ? ScriptInitialPhase::Start : phase;
}

} // namespace

ScriptInitialPhase ScriptInitialLifecycle::Next() const {
    switch (m_CompletedPhases) {
    case 0:
        return ScriptInitialPhase::OnLoad;
    case 1:
        return ScriptInitialPhase::Awake;
    case 2:
        return ScriptInitialPhase::Start;
    default:
        return ScriptInitialPhase::Ready;
    }
}

ScriptLifecycleInvocationStatus ScriptInitialLifecycle::AdvanceThrough(
    ScriptInitialPhase finalPhase,
    const ScriptInitialLifecycleInvoker &invoke) {
    if (!invoke) {
        return ScriptLifecycleInvocationStatus::Failed;
    }

    const ScriptInitialPhase last = LastInvocablePhase(finalPhase);
    while (Next() != ScriptInitialPhase::Ready &&
           static_cast<std::uint8_t>(Next()) <= static_cast<std::uint8_t>(last)) {
        const ScriptInitialPhase phase = Next();
        const ScriptLifecycleInvocationStatus status = invoke(phase);
        if (status != ScriptLifecycleInvocationStatus::Finished) {
            return status;
        }
        if (!Complete(phase)) {
            return ScriptLifecycleInvocationStatus::Failed;
        }
    }
    return ScriptLifecycleInvocationStatus::Finished;
}

bool ScriptInitialLifecycle::Complete(ScriptInitialPhase phase) {
    if (phase == ScriptInitialPhase::Ready || phase != Next()) {
        return false;
    }
    ++m_CompletedPhases;
    return true;
}

void ScriptInitialLifecycle::RequestPause() {
    m_Paused = true;
    m_ResumePending = false;
}

void ScriptInitialLifecycle::RequestResume(bool eligible) {
    m_Paused = false;
    m_ResumePending = eligible;
}

bool ScriptInitialLifecycle::Paused() const {
    return m_Paused;
}

bool ScriptInitialLifecycle::ResumePending() const {
    return m_ResumePending;
}

bool ScriptInitialLifecycle::ResumeReady(bool transitionBlocked) const {
    return m_ResumePending && !m_Paused && !transitionBlocked;
}

void ScriptInitialLifecycle::CompleteResume() {
    m_ResumePending = false;
}

void ScriptInitialLifecycle::ResetStart() {
    if (m_CompletedPhases > 2) {
        m_CompletedPhases = 2;
    }
}

void ScriptInitialLifecycle::Clear() {
    m_CompletedPhases = 0;
    m_Paused = false;
    m_ResumePending = false;
}
