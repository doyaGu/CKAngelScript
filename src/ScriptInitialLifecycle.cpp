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

void ScriptInitialLifecycle::SetDesiredEnabled(bool enabled) {
    m_DesiredEnabled = enabled;
    if (!enabled) {
        CompleteResume();
    }
}

bool ScriptInitialLifecycle::DesiredEnabled() const {
    return m_DesiredEnabled;
}

void ScriptInitialLifecycle::RequestPause(bool disableRequired) {
    const bool startsTransition = !m_Paused && !PausePending();
    m_Paused = true;
    m_ResumePending = false;
    if (startsTransition) {
        m_PausePhase = ScriptPausePhase::OnPause;
        m_DisableAfterPause = disableRequired;
    } else if (PausePending()) {
        m_DisableAfterPause = m_DisableAfterPause || disableRequired;
    }
}

ScriptLifecycleInvocationStatus ScriptInitialLifecycle::AdvancePause(
    const ScriptPauseLifecycleInvoker &invoke) {
    if (!PausePending()) {
        return ScriptLifecycleInvocationStatus::Finished;
    }
    if (!invoke) {
        return ScriptLifecycleInvocationStatus::Failed;
    }

    while (PausePending()) {
        const ScriptPausePhase phase = m_PausePhase;
        const ScriptLifecycleInvocationStatus status = invoke(phase);
        if (status == ScriptLifecycleInvocationStatus::Suspended) {
            return status;
        }
        if (status == ScriptLifecycleInvocationStatus::Failed) {
            CompletePause();
            return status;
        }

        if (phase == ScriptPausePhase::OnPause && m_DisableAfterPause) {
            m_PausePhase = ScriptPausePhase::OnDisable;
        } else {
            CompletePause();
        }
    }
    return ScriptLifecycleInvocationStatus::Finished;
}

bool ScriptInitialLifecycle::PausePending() const {
    return m_PausePhase != ScriptPausePhase::Ready;
}

void ScriptInitialLifecycle::CompletePause() {
    m_PausePhase = ScriptPausePhase::Ready;
    m_DisableAfterPause = false;
}

void ScriptInitialLifecycle::RequestResume(bool hostEligible) {
    m_Paused = false;
    m_ResumePending = m_DesiredEnabled && hostEligible;
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

void ScriptInitialLifecycle::RequestReset() {
    CompletePause();
    CompleteResume();
    ResetStart();
    m_ResetPending = true;
}

ScriptLifecycleInvocationStatus ScriptInitialLifecycle::AdvanceReset(
    const ScriptResetLifecycleInvoker &invoke) {
    if (!ResetPending()) {
        return ScriptLifecycleInvocationStatus::Finished;
    }
    if (!invoke) {
        CompleteReset();
        return ScriptLifecycleInvocationStatus::Failed;
    }

    const ScriptLifecycleInvocationStatus status = invoke();
    if (status != ScriptLifecycleInvocationStatus::Suspended) {
        CompleteReset();
    }
    return status;
}

bool ScriptInitialLifecycle::ResetPending() const {
    return m_ResetPending;
}

void ScriptInitialLifecycle::CompleteReset() {
    m_ResetPending = false;
}

void ScriptInitialLifecycle::ResetStart() {
    if (m_CompletedPhases > 2) {
        m_CompletedPhases = 2;
    }
}

void ScriptInitialLifecycle::Clear() {
    m_CompletedPhases = 0;
    CompletePause();
    CompleteReset();
    m_Paused = false;
    m_ResumePending = false;
}
