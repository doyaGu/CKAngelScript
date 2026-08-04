#ifndef CK_SCRIPTINVOCATION_H
#define CK_SCRIPTINVOCATION_H

#include <functional>
#include <string>
#include <utility>

#include <angelscript.h>

class ScriptManager;

using ScriptInvocationHandler = std::function<bool(asIScriptContext *)>;

enum class ScriptInvocationStatus {
    Finished,
    Suspended,
    Failed
};

struct ScriptInvocationSpec {
    const char *Label = "Script";
    ScriptInvocationHandler Prepare;
    ScriptInvocationHandler Result;
    ScriptInvocationHandler Resume;
    std::function<void()> BeforeExecute;
    std::function<void()> AfterExecute;
};

// Owns one reusable AngelScript context and centralizes its execution contract.
// Host-specific lifecycle policy and argument storage remain outside this type.
class ScriptInvocationContext {
public:
    explicit ScriptInvocationContext(ScriptManager *manager);
    ~ScriptInvocationContext();

    ScriptInvocationContext(const ScriptInvocationContext &) = delete;
    ScriptInvocationContext &operator=(const ScriptInvocationContext &) = delete;

    ScriptInvocationStatus Start(asIScriptFunction *function, const ScriptInvocationSpec &spec = {});
    ScriptInvocationStatus Resume(const ScriptInvocationSpec &spec = {});

    bool IsSuspended() const;
    asIScriptContext *GetContext() const { return m_Context; }
    void AbortActive();
    void Cancel();

    int GetLastResultCode() const { return m_LastResultCode; }
    const std::string &GetErrorMessage() const { return m_ErrorMessage; }
    const std::string &GetStackTrace() const { return m_StackTrace; }
    void SetErrorMessage(std::string message) { m_ErrorMessage = std::move(message); }
    void SetStackTrace(std::string trace) { m_StackTrace = std::move(trace); }
    void ClearDiagnostics();

private:
    bool Acquire(asIScriptEngine *engine);
    ScriptInvocationStatus ExecutePrepared(const ScriptInvocationSpec &spec);
    ScriptInvocationStatus Fail(int resultCode, std::string message, bool resetContext);
    void ResetPreparedContext();

    ScriptManager *m_Manager = nullptr;
    asIScriptContext *m_Context = nullptr;
    int m_LastResultCode = 0;
    std::string m_ErrorMessage;
    std::string m_StackTrace;
};

#endif // CK_SCRIPTINVOCATION_H
