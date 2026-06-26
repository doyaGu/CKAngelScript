#include "ScriptRegistration.h"

#include <fmt/format.h>

namespace {

thread_local ScriptRegistrationContext *g_ActiveRegistrationContext = nullptr;

} // namespace

ScriptRegistrationContext::ScriptRegistrationContext(const char *moduleName)
    : m_ModuleName(moduleName && moduleName[0] != '\0' ? moduleName : "AngelScript registration") {}

void ScriptRegistrationContext::RecordFailure(const char *file,
                                              int line,
                                              const char *function,
                                              const char *expression,
                                              int code,
                                              const char *detail) {
    ++m_FailureCount;
    if (m_FirstFailure.empty()) {
        m_FirstFailure = fmt::format("{}:{} in {}: {} returned {}",
                                     file ? file : "<unknown>",
                                     line,
                                     function ? function : "<unknown>",
                                     expression ? expression : "<unknown>",
                                     code);
        if (detail && detail[0] != '\0') {
            m_FirstFailure += ": ";
            m_FirstFailure += detail;
        }
    }
}

std::string ScriptRegistrationContext::GetSummary() const {
    if (!HasFailures()) {
        return fmt::format("{} completed without registration failures.", m_ModuleName);
    }
    return fmt::format("{} failed with {} registration error(s). First failure: {}",
                       m_ModuleName,
                       m_FailureCount,
                       m_FirstFailure);
}

ScriptRegistrationScope::ScriptRegistrationScope(ScriptRegistrationContext &context)
    : m_Previous(g_ActiveRegistrationContext) {
    g_ActiveRegistrationContext = &context;
}

ScriptRegistrationScope::~ScriptRegistrationScope() {
    g_ActiveRegistrationContext = m_Previous;
}

ScriptRegistrationContext *GetActiveScriptRegistrationContext() {
    return g_ActiveRegistrationContext;
}

bool CheckScriptRegistrationResult(int code,
                                   const char *expression,
                                   const char *file,
                                   int line,
                                   const char *function) {
    if (code >= 0) {
        return true;
    }

    ScriptRegistrationContext *context = GetActiveScriptRegistrationContext();
    if (context) {
        context->RecordFailure(file, line, function, expression, code);
    }
    return false;
}
