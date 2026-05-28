#ifndef CK_SCRIPTREGISTRATION_H
#define CK_SCRIPTREGISTRATION_H

#include <cassert>
#include <string>

class ScriptRegistrationContext {
public:
    explicit ScriptRegistrationContext(const char *moduleName);

    void RecordFailure(const char *file,
                       int line,
                       const char *function,
                       const char *expression,
                       int code);

    const std::string &GetModuleName() const {
        return m_ModuleName;
    }

    int GetFailureCount() const {
        return m_FailureCount;
    }

    bool HasFailures() const {
        return m_FailureCount > 0;
    }

    const std::string &GetFirstFailure() const {
        return m_FirstFailure;
    }

    std::string GetSummary() const;

private:
    std::string m_ModuleName;
    int m_FailureCount = 0;
    std::string m_FirstFailure;
};

class ScriptRegistrationScope {
public:
    explicit ScriptRegistrationScope(ScriptRegistrationContext &context);
    ~ScriptRegistrationScope();

    ScriptRegistrationScope(const ScriptRegistrationScope &) = delete;
    ScriptRegistrationScope &operator=(const ScriptRegistrationScope &) = delete;

private:
    ScriptRegistrationContext *m_Previous = nullptr;
};

ScriptRegistrationContext *GetActiveScriptRegistrationContext();

bool CheckScriptRegistrationResult(int code,
                                   const char *expression,
                                   const char *file,
                                   int line,
                                   const char *function);

#define CKAS_CHECK_REGISTER(result)                                                       \
    do {                                                                                  \
        const int ckasRegistrationResult = (result);                                      \
        const bool ckasRegistrationOk =                                                   \
            ::CheckScriptRegistrationResult(ckasRegistrationResult,                       \
                                            #result,                                      \
                                            __FILE__,                                     \
                                            __LINE__,                                     \
                                            __func__);                                    \
        assert(ckasRegistrationOk);                                                       \
        (void)ckasRegistrationOk;                                                         \
    } while (false)

#endif // CK_SCRIPTREGISTRATION_H
