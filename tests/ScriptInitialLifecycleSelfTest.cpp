#include "ScriptCoreSelfTests.h"

#include <vector>

#include "ScriptInitialLifecycle.h"

bool RunScriptInitialLifecycleSelfTest(std::string &error) {
    ScriptInitialLifecycle lifecycle;
    if (lifecycle.Next() != ScriptInitialPhase::OnLoad ||
        lifecycle.Complete(ScriptInitialPhase::Awake)) {
        error = "Initial lifecycle accepted an out-of-order phase.";
        return false;
    }
    if (!lifecycle.Complete(ScriptInitialPhase::OnLoad) ||
        lifecycle.Next() != ScriptInitialPhase::Awake ||
        lifecycle.Complete(ScriptInitialPhase::OnLoad)) {
        error = "Initial lifecycle did not advance from OnLoad exactly once.";
        return false;
    }
    if (!lifecycle.Complete(ScriptInitialPhase::Awake) ||
        !lifecycle.Complete(ScriptInitialPhase::Start) ||
        lifecycle.Next() != ScriptInitialPhase::Ready ||
        lifecycle.Complete(ScriptInitialPhase::Ready)) {
        error = "Initial lifecycle did not reach Ready in order.";
        return false;
    }
    lifecycle.ResetStart();
    if (lifecycle.Next() != ScriptInitialPhase::Start ||
        !lifecycle.Complete(ScriptInitialPhase::Start)) {
        error = "Initial lifecycle did not reset Start.";
        return false;
    }
    lifecycle.Clear();
    if (lifecycle.Next() != ScriptInitialPhase::OnLoad) {
        error = "Initial lifecycle did not clear all phases.";
        return false;
    }

    std::vector<ScriptInitialPhase> invoked;
    bool suspendedAwake = false;
    const auto invoke = [&](ScriptInitialPhase phase) {
        invoked.push_back(phase);
        if (phase == ScriptInitialPhase::Awake && !suspendedAwake) {
            suspendedAwake = true;
            return ScriptLifecycleInvocationStatus::Suspended;
        }
        return ScriptLifecycleInvocationStatus::Finished;
    };
    if (lifecycle.AdvanceThrough(ScriptInitialPhase::Awake, invoke) !=
            ScriptLifecycleInvocationStatus::Suspended ||
        lifecycle.Next() != ScriptInitialPhase::Awake ||
        invoked != std::vector<ScriptInitialPhase>{ScriptInitialPhase::OnLoad,
                                                   ScriptInitialPhase::Awake}) {
        error = "Initial lifecycle did not retain a suspended phase.";
        return false;
    }
    if (lifecycle.AdvanceThrough(ScriptInitialPhase::Awake, invoke) !=
            ScriptLifecycleInvocationStatus::Finished ||
        lifecycle.Next() != ScriptInitialPhase::Start ||
        invoked != std::vector<ScriptInitialPhase>{ScriptInitialPhase::OnLoad,
                                                   ScriptInitialPhase::Awake,
                                                   ScriptInitialPhase::Awake}) {
        error = "Initial lifecycle did not resume from the suspended phase.";
        return false;
    }

    lifecycle.RequestPause();
    lifecycle.RequestResume(true);
    if (lifecycle.Paused() || !lifecycle.ResumePending() ||
        lifecycle.ResumeReady(true) || !lifecycle.ResumePending() ||
        !lifecycle.ResumeReady(false)) {
        error = "Lifecycle did not retain Resume while Pause completion was blocked.";
        return false;
    }
    lifecycle.CompleteResume();
    if (lifecycle.ResumePending()) {
        error = "Lifecycle did not complete the retained Resume event.";
        return false;
    }
    lifecycle.RequestResume(true);
    lifecycle.RequestPause();
    if (!lifecycle.Paused() || lifecycle.ResumePending()) {
        error = "Lifecycle did not let a new Pause supersede the retained Resume event.";
        return false;
    }
    return true;
}
