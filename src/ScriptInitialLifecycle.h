#ifndef CK_SCRIPT_INITIAL_LIFECYCLE_H
#define CK_SCRIPT_INITIAL_LIFECYCLE_H

#include <cstdint>
#include <functional>

enum class ScriptInitialPhase : std::uint8_t {
    OnLoad,
    Awake,
    Start,
    Ready,
};

enum class ScriptLifecycleInvocationStatus : std::uint8_t {
    Finished,
    Suspended,
    Failed,
};

using ScriptInitialLifecycleInvoker =
    std::function<ScriptLifecycleInvocationStatus(ScriptInitialPhase)>;

class ScriptInitialLifecycle final {
public:
    ScriptInitialPhase Next() const;
    ScriptLifecycleInvocationStatus AdvanceThrough(
        ScriptInitialPhase finalPhase,
        const ScriptInitialLifecycleInvoker &invoke);
    bool Complete(ScriptInitialPhase phase);
    void ResetStart();
    void Clear();

private:
    std::uint8_t m_CompletedPhases = 0;
};

#endif // CK_SCRIPT_INITIAL_LIFECYCLE_H
