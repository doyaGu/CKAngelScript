#include "ScriptRegistration.h"

#include <fmt/format.h>

namespace ScriptRegistrationInternal {

thread_local ScriptRegistrationContext *g_ActiveRegistrationContext = nullptr;

} // namespace ScriptRegistrationInternal

ScriptRegistrationContext::ScriptRegistrationContext(const char *moduleName)
    : m_ModuleName(moduleName && moduleName[0] != '\0' ? moduleName : "AngelScript registration") {}

void ScriptRegistrationContext::RecordFailure(const char *file,
                                              int line,
                                              const char *function,
                                              const char *expression,
                                              int code) {
    ++m_FailureCount;
    if (m_FirstFailure.empty()) {
        m_FirstFailure = fmt::format("{}:{} in {}: {} returned {}",
                                     file ? file : "<unknown>",
                                     line,
                                     function ? function : "<unknown>",
                                     expression ? expression : "<unknown>",
                                     code);
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
    : m_Previous(ScriptRegistrationInternal::g_ActiveRegistrationContext) {
    ScriptRegistrationInternal::g_ActiveRegistrationContext = &context;
}

ScriptRegistrationScope::~ScriptRegistrationScope() {
    ScriptRegistrationInternal::g_ActiveRegistrationContext = m_Previous;
}

ScriptRegistrationContext *GetActiveScriptRegistrationContext() {
    return ScriptRegistrationInternal::g_ActiveRegistrationContext;
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
