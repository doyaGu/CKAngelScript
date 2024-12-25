#ifndef CK_SCRIPTRUNNER_H
#define CK_SCRIPTRUNNER_H

#include <string>
#include <chrono>
#include <functional>

#include <angelscript.h>

#include "CKBehavior.h"

class ScriptManager;
struct CachedScript;

using ScriptFunctionArgumentHandler = std::function<void(asIScriptContext *)>;

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

    asIScriptFunction *GetFunctionByName(const char *name);
    asIScriptFunction *GetFunctionByDecl(const char *decl);

    bool ExecuteScript(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler = nullptr, const ScriptFunctionArgumentHandler &retHandler = nullptr);

    // Timing / metrics
    void StartTiming();
    void EndTiming();
    double GetElapsedTimeMs() const;

    // Error / stack trace
    const std::string &GetErrorMessage() const;
    void SetErrorMessage(const std::string& msg);

    const std::string &GetStackTrace() const;
    void SetStackTrace(const std::string& trace);

    void Reset();

private:
    ScriptManager *m_ScriptManager = nullptr;
    asIScriptContext *m_Context = nullptr;
    std::shared_ptr<CachedScript> m_CachedScript;
    bool m_Attached = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    double m_ElapsedMs = 0.0;
    std::string m_ErrorMessage;
    std::string m_StackTrace;
};

#endif // CK_SCRIPTRUNNER_H
