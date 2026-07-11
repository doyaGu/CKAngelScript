#ifndef CK_SCRIPT_INITIAL_LIFECYCLE_H
#define CK_SCRIPT_INITIAL_LIFECYCLE_H

#include <cstdint>

enum class ScriptInitialPhase : std::uint8_t {
    OnLoad,
    Awake,
    Start,
    Ready,
};

class ScriptInitialLifecycle final {
public:
    ScriptInitialPhase Next() const;
    bool Complete(ScriptInitialPhase phase);
    void ResetStart();
    void Clear();

private:
    std::uint8_t m_CompletedPhases = 0;
};

#endif // CK_SCRIPT_INITIAL_LIFECYCLE_H
