#ifndef CK_SCRIPTRUNNER_H
#define CK_SCRIPTRUNNER_H

#include <string>
#include <chrono>
#include <functional>
#include <memory>

#include <angelscript.h>

#include "CKBehavior.h"
#include "ScriptMessage.h"

class ScriptManager;
struct CachedScript;

using ScriptFunctionArgumentHandler = std::function<void(asIScriptContext *)>;

enum class ScriptExecutionStatus {
    Finished,
    Suspended,
    Failed
};

class ScriptRunner {
public:
    explicit ScriptRunner(ScriptManager *man);
    ~ScriptRunner();

    ScriptRunner(const ScriptRunner&) = delete;
    ScriptRunner& operator=(const ScriptRunner&) = delete;

    bool IsAttached() const { return m_Attached; }
    bool Attach(CKBehavior *behavior, bool runner = false);
    void Detach(CKBehavior *behavior, bool runner = false);

    asIScriptContext *GetContext() const;
    void SetContext(asIScriptContext *ctx);

    bool SetScript(const char *scriptName);
    void ResetScript();

    std::shared_ptr<CachedScript> GetCachedScript() const { return m_CachedScript; }
    asIScriptModule *GetModule() const;
    asITypeInfo *GetTypeInfoByName(const char *name) const;
    asIScriptObject *CreateScriptObject(asITypeInfo *type);

    asIScriptFunction *GetFunctionByName(const char *name) const;
    asIScriptFunction *GetFunctionByDecl(const char *decl) const;

    bool ExecuteScript(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler = nullptr, const ScriptFunctionArgumentHandler &retHandler = nullptr);
    ScriptExecutionStatus ExecuteScriptStatus(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler = nullptr, const ScriptFunctionArgumentHandler &retHandler = nullptr);
    bool ExecuteObjectMethod(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext);
    ScriptExecutionStatus ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext);
    ScriptExecutionStatus ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const ScriptMessage &message, const CKBehaviorContext &behcontext);
    bool IsContextSuspended() const;
    void AbortContext();

    // Timing / metrics
    bool IsProfiling() const { return m_Profiling; }
    void EnableProfiling(bool enable = true) { m_Profiling = enable; }
    void StartTiming();
    void EndTiming();
    double GetElapsedTimeMs() const;

    // Error / stack trace
    const std::string &GetErrorMessage() const;
    void SetErrorMessage(const std::string& msg);

    const std::string &GetStackTrace() const;
    void SetStackTrace(const std::string& trace);
    int GetLastResultCode() const { return m_LastResultCode; }

    void Reset();

private:
    void ReleaseContext();

    ScriptManager *m_ScriptManager = nullptr;
    asIScriptContext *m_Context = nullptr;
    std::shared_ptr<CachedScript> m_CachedScript;
    bool m_Attached = false;
    bool m_Profiling = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    double m_ElapsedMs = 0.0;
    int m_LastResultCode = 0;
    std::string m_ErrorMessage;
    std::string m_StackTrace;
    CKBehaviorContext m_BehaviorContextStorage;
    ScriptMessage m_MessageStorage;
};

#endif // CK_SCRIPTRUNNER_H
