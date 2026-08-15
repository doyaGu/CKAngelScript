#include "ScriptCoreSelfTests.h"

#include <vector>

#include "ScriptInitialLifecycle.h"

bool RunScriptInitialLifecycleSelfTest(std::string &error) {
    ScriptInitialLifecycle lifecycle;
    if (lifecycle.DesiredEnabled() ||
        lifecycle.Next() != ScriptInitialPhase::OnLoad ||
        lifecycle.Complete(ScriptInitialPhase::Awake)) {
        error = "Initial lifecycle accepted an out-of-order phase.";
        return false;
    }
    lifecycle.SetDesiredEnabled(true);
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
    if (lifecycle.Next() != ScriptInitialPhase::OnLoad ||
        !lifecycle.DesiredEnabled()) {
        error = "Initial lifecycle did not clear phases while preserving desired activation.";
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

    std::vector<ScriptPausePhase> pauseInvoked;
    bool suspendedPause = false;
    const auto invokePause = [&](ScriptPausePhase phase) {
        pauseInvoked.push_back(phase);
        if (phase == ScriptPausePhase::OnPause && !suspendedPause) {
            suspendedPause = true;
            return ScriptLifecycleInvocationStatus::Suspended;
        }
        return ScriptLifecycleInvocationStatus::Finished;
    };
    lifecycle.RequestPause(true);
    if (!lifecycle.Paused() || !lifecycle.PausePending() ||
        lifecycle.AdvancePause(invokePause) != ScriptLifecycleInvocationStatus::Suspended ||
        !lifecycle.PausePending() ||
        pauseInvoked != std::vector<ScriptPausePhase>{ScriptPausePhase::OnPause}) {
        error = "Lifecycle did not retain a suspended Pause phase.";
        return false;
    }
    lifecycle.RequestResume(true);
    if (lifecycle.Paused() || !lifecycle.ResumePending() ||
        lifecycle.ResumeReady(lifecycle.PausePending()) || !lifecycle.ResumePending()) {
        error = "Lifecycle did not retain Resume while Pause completion was blocked.";
        return false;
    }
    if (lifecycle.AdvancePause(invokePause) != ScriptLifecycleInvocationStatus::Finished ||
        lifecycle.PausePending() ||
        pauseInvoked != std::vector<ScriptPausePhase>{ScriptPausePhase::OnPause,
                                                       ScriptPausePhase::OnPause,
                                                       ScriptPausePhase::OnDisable} ||
        !lifecycle.ResumeReady(lifecycle.PausePending())) {
        error = "Lifecycle did not finish Pause before releasing retained Resume.";
        return false;
    }
    lifecycle.SetDesiredEnabled(false);
    if (lifecycle.DesiredEnabled() || lifecycle.ResumePending()) {
        error = "Lifecycle explicit Disable did not supersede the retained Resume event.";
        return false;
    }
    lifecycle.RequestResume(true);
    if (lifecycle.ResumePending()) {
        error = "Lifecycle retained Resume while desired activation was disabled.";
        return false;
    }
    lifecycle.SetDesiredEnabled(true);
    lifecycle.RequestResume(true);
    lifecycle.CompleteResume();
    if (!lifecycle.DesiredEnabled() || lifecycle.ResumePending()) {
        error = "Lifecycle did not complete the retained Resume event.";
        return false;
    }
    lifecycle.RequestResume(true);
    lifecycle.RequestPause(false);
    if (!lifecycle.Paused() || lifecycle.ResumePending()) {
        error = "Lifecycle did not let a new Pause supersede the retained Resume event.";
        return false;
    }
    if (lifecycle.AdvancePause([](ScriptPausePhase) {
            return ScriptLifecycleInvocationStatus::Failed;
        }) != ScriptLifecycleInvocationStatus::Failed ||
        lifecycle.PausePending()) {
        error = "Lifecycle did not terminate a failed Pause transition.";
        return false;
    }
    return true;
}
