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

enum class ScriptPausePhase : std::uint8_t {
    OnPause,
    OnDisable,
    Ready,
};

using ScriptInitialLifecycleInvoker =
    std::function<ScriptLifecycleInvocationStatus(ScriptInitialPhase)>;
using ScriptPauseLifecycleInvoker =
    std::function<ScriptLifecycleInvocationStatus(ScriptPausePhase)>;

class ScriptInitialLifecycle final {
public:
    ScriptInitialPhase Next() const;
    ScriptLifecycleInvocationStatus AdvanceThrough(
        ScriptInitialPhase finalPhase,
        const ScriptInitialLifecycleInvoker &invoke);
    bool Complete(ScriptInitialPhase phase);
    void SetDesiredEnabled(bool enabled);
    bool DesiredEnabled() const;
    void RequestPause(bool disableRequired = false);
    ScriptLifecycleInvocationStatus AdvancePause(
        const ScriptPauseLifecycleInvoker &invoke);
    bool PausePending() const;
    void CompletePause();
    void RequestResume(bool eligible);
    bool Paused() const;
    bool ResumePending() const;
    bool ResumeReady(bool transitionBlocked) const;
    void CompleteResume();
    void ResetStart();
    void Clear();

private:
    std::uint8_t m_CompletedPhases = 0;
    ScriptPausePhase m_PausePhase = ScriptPausePhase::Ready;
    bool m_DisableAfterPause = false;
    bool m_DesiredEnabled = false;
    bool m_Paused = false;
    bool m_ResumePending = false;
};

#endif // CK_SCRIPT_INITIAL_LIFECYCLE_H
