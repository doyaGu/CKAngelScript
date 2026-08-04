#ifndef CK_SCRIPTINVOKER_H
#define CK_SCRIPTINVOKER_H

#include <string>
#include <chrono>
#include <memory>

#include <angelscript.h>

#include "CKBehavior.h"
#include "ScriptInvocation.h"
#include "ScriptMessage.h"

class ScriptManager;
class BehaviorFrame;
struct CachedScript;

using ScriptFunctionArgumentHandler = ScriptInvocationHandler;

enum class ScriptBehaviorArgumentKind {
    BehaviorContext,
    BehaviorFrame
};

class ScriptInvoker {
public:
    explicit ScriptInvoker(ScriptManager *man);
    ~ScriptInvoker();

    ScriptInvoker(const ScriptInvoker&) = delete;
    ScriptInvoker& operator=(const ScriptInvoker&) = delete;

    asIScriptContext *GetContext() const;

    bool SetScript(const char *scriptName);
    void ResetScript();

    std::shared_ptr<CachedScript> GetCachedScript() const { return m_CachedScript; }
    asIScriptModule *GetModule() const;
    asITypeInfo *GetTypeInfoByName(const char *name) const;
    asIScriptObject *CreateScriptObject(asITypeInfo *type);

    asIScriptFunction *GetFunctionByName(const char *name) const;
    asIScriptFunction *GetFunctionByDecl(const char *decl) const;

    bool ExecuteScript(asIScriptFunction *func, const ScriptFunctionArgumentHandler &argsHandler = nullptr, const ScriptFunctionArgumentHandler &retHandler = nullptr);
    ScriptInvocationStatus ExecuteScriptStatus(asIScriptFunction *func,
                                               const ScriptFunctionArgumentHandler &argsHandler = nullptr,
                                               const ScriptFunctionArgumentHandler &retHandler = nullptr,
                                               const ScriptFunctionArgumentHandler &resumeHandler = nullptr);
    bool ExecuteObjectMethod(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const CKBehaviorContext &behcontext);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object,
                                                     asIScriptFunction *func,
                                                     const CKBehaviorContext &behcontext,
                                                     ScriptBehaviorArgumentKind argumentKind);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object,
                                                     asIScriptFunction *func,
                                                     const CKBehaviorContext &behcontext,
                                                     const ScriptFunctionArgumentHandler &retHandler);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object,
                                                     asIScriptFunction *func,
                                                     const CKBehaviorContext &behcontext,
                                                     ScriptBehaviorArgumentKind argumentKind,
                                                     const ScriptFunctionArgumentHandler &retHandler);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object, asIScriptFunction *func, const ScriptMessage &message, const CKBehaviorContext &behcontext);
    ScriptInvocationStatus ExecuteObjectMethodStatus(asIScriptObject *object,
                                                     asIScriptFunction *func,
                                                     const ScriptMessage &message,
                                                     const CKBehaviorContext &behcontext,
                                                     ScriptBehaviorArgumentKind argumentKind);
    ScriptInvocationStatus ResumeObjectMessageStatus(const CKBehaviorContext &behcontext);
    bool IsContextSuspended() const;
    void AbortActiveContext();
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
    int GetLastResultCode() const { return m_Invocation.GetLastResultCode(); }

    void Reset();

private:
    void ReleaseContext();
    void ReleaseBehaviorFrame();
    ScriptInvocationStatus ResumePreparedObjectContext(const CKBehaviorContext &behcontext,
                                                       const char *label,
                                                       const ScriptFunctionArgumentHandler &retHandler);

    ScriptManager *m_ScriptManager = nullptr;
    ScriptInvocationContext m_Invocation;
    std::shared_ptr<CachedScript> m_CachedScript;
    bool m_Profiling = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    double m_ElapsedMs = 0.0;
    CKBehaviorContext m_BehaviorContextStorage;
    BehaviorFrame *m_BehaviorFrameStorage = nullptr;
    ScriptMessage m_MessageStorage;
};

#endif // CK_SCRIPTINVOKER_H
