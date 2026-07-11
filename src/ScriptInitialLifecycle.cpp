#include "ScriptInitialLifecycle.h"

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

bool ScriptInitialLifecycle::Complete(ScriptInitialPhase phase) {
    if (phase == ScriptInitialPhase::Ready || phase != Next()) {
        return false;
    }
    ++m_CompletedPhases;
    return true;
}

void ScriptInitialLifecycle::ResetStart() {
    if (m_CompletedPhases > 2) {
        m_CompletedPhases = 2;
    }
}

void ScriptInitialLifecycle::Clear() {
    m_CompletedPhases = 0;
}
