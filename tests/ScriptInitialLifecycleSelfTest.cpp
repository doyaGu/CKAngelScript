#include "ScriptSelfTests.h"

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
    return true;
}
