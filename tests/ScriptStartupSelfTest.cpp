#include "ScriptSelfTests.h"

#include <cstdlib>
#include <fstream>
#include <string>

#include "Logger.h"
#include "ScriptCKPrototypeHook.h"
#include "ScriptManager.h"

namespace {

bool StartupSelfTestsAttempted = false;

class ScopedSelfTestMessageOutputSuppression {
public:
    explicit ScopedSelfTestMessageOutputSuppression(ScriptManager *manager)
        : m_Manager(manager) {
        if (m_Manager) {
            m_Manager->PushMessageOutputSuppression();
        }
    }

    ~ScopedSelfTestMessageOutputSuppression() {
        if (m_Manager) {
            m_Manager->PopMessageOutputSuppression();
        }
    }

private:
    ScriptManager *m_Manager = nullptr;
};

bool IsTruthyEnvironmentValue(const char *name) {
    const char *value = std::getenv(name);
    return value && value[0] != '\0' && !(value[0] == '0' && value[1] == '\0');
}

bool ShouldRunStartupSelfTests() {
    return IsTruthyEnvironmentValue("CKAS_RUN_SELFTESTS") || IsTruthyEnvironmentValue("CKAS_SELFTEST_MARKER");
}

bool RunPrototypeHookFlagSelfTest(std::string &error) {
    const char *truthy[] = { "1", "true", "TRUE", "yes", "on", " on " };
    for (const char *value : truthy) {
        if (!ParseCKASEnvironmentFlag(value)) {
            error = std::string("Prototype hook flag parser rejected truthy value '") + value + "'.";
            return false;
        }
    }

    const char *falsey[] = { "", "0", "false", "no", "off", "oops", " off " };
    if (ParseCKASEnvironmentFlag(nullptr)) {
        error = "Prototype hook flag parser accepted a null value.";
        return false;
    }
    for (const char *value : falsey) {
        if (ParseCKASEnvironmentFlag(value)) {
            error = std::string("Prototype hook flag parser accepted falsey value '") + value + "'.";
            return false;
        }
    }
    return true;
}

void WriteStartupSelfTestMarker(const char *status, const char *stage, const std::string &message) {
    const char *path = std::getenv("CKAS_SELFTEST_MARKER");
    if (!path || path[0] == '\0') {
        return;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return;
    }

    out << "status=" << (status ? status : "unknown") << "\n";
    out << "stage=" << (stage ? stage : "unknown") << "\n";
    if (!message.empty()) {
        out << "message=" << message << "\n";
    }
}

bool ReportSelfTestFailure(ScriptManager *manager,
                           const char *stage,
                           const char *label,
                           const std::string &error) {
    if (manager && manager->GetCKContext()) {
        manager->GetCKContext()->OutputToConsoleEx(const_cast<char *>("[AngelScript] %s self-test failed: %s"),
                                                   label,
                                                   error.c_str());
    }
    LOG_ERROR("%s self-test failed: %s", label, error.c_str());
    WriteStartupSelfTestMarker("failed", stage, error);
    return false;
}

} // namespace

CKERROR RunScriptStartupSelfTests(ScriptManager *manager) {
    if (StartupSelfTestsAttempted) {
        return CK_OK;
    }
    if (!ShouldRunStartupSelfTests()) {
        return CK_OK;
    }
    StartupSelfTestsAttempted = true;
    ScopedSelfTestMessageOutputSuppression suppressExpectedDiagnostics(manager);

    WriteStartupSelfTestMarker("running", "start", std::string());

    std::string error;
    WriteStartupSelfTestMarker("running", "prototype-hook-flag", std::string());
    if (!RunPrototypeHookFlagSelfTest(error)) {
        ReportSelfTestFailure(manager, "prototype-hook-flag", "Prototype hook flag", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "initial-lifecycle", std::string());
    if (!RunScriptInitialLifecycleSelfTest(error)) {
        ReportSelfTestFailure(manager, "initial-lifecycle", "Initial lifecycle", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "parameter-conversion", std::string());
    if (!RunScriptParameterConversionSelfTest(manager ? manager->GetCKContext() : nullptr,
                                              manager ? manager->GetScriptEngine() : nullptr,
                                              error)) {
        ReportSelfTestFailure(manager,
                                                             "parameter-conversion",
                                                             "Parameter conversion",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "parameter-registry", std::string());
    if (!RunScriptParameterRegistrySelfTest(manager ? manager->GetCKContext() : nullptr,
                                            manager ? manager->GetScriptEngine() : nullptr,
                                            error)) {
        ReportSelfTestFailure(manager,
                                                             "parameter-registry",
                                                             "Parameter registry",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "manager-api", std::string());
    if (!RunScriptApiSelfTest(manager ? manager->GetCKContext() : nullptr, error)) {
        ReportSelfTestFailure(manager,
                                                             "manager-api",
                                                             "CKAngelScript API",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

#if CKAS_ENABLE_API_EXPORT
    WriteStartupSelfTestMarker("running", "script-info-export", std::string());
    if (!RunScriptInfoExportSelfTest(manager ? manager->GetScriptEngine() : nullptr, error)) {
        ReportSelfTestFailure(manager,
                                                             "script-info-export",
                                                             "ScriptInfo export",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }
#endif

    WriteStartupSelfTestMarker("running", "component-metadata", std::string());
    if (!RunScriptComponentMetadataSelfTest(manager ? manager->GetCKContext() : nullptr,
                                            manager ? manager->GetScriptEngine() : nullptr,
                                            error)) {
        ReportSelfTestFailure(manager,
                                                             "component-metadata",
                                                             "Component metadata",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "runtime-metadata", std::string());
    if (!ScriptRuntimeMetadata::RunScriptRuntimeMetadataSelfTest(error)) {
        ReportSelfTestFailure(manager,
                                                             "runtime-metadata",
                                                             "Runtime metadata",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "runtime-dependency", std::string());
    if (!ScriptRuntimeDependencyResolver::RunScriptRuntimeDependencySelfTest(error)) {
        ReportSelfTestFailure(manager,
                                                             "runtime-dependency",
                                                             "Runtime dependency",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "native-memory", std::string());
    if (!RunScriptNativeMemorySelfTest(manager ? manager->GetCKContext() : nullptr,
                                       manager ? manager->GetScriptEngine() : nullptr,
                                       error)) {
        ReportSelfTestFailure(manager,
                                                             "native-memory",
                                                             "Native memory",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

#if CKAS_ENABLE_DYNCALL
    WriteStartupSelfTestMarker("running", "dynload", std::string());
    if (!RunScriptDynLoadSelfTest(manager ? manager->GetScriptEngine() : nullptr, error)) {
        ReportSelfTestFailure(manager,
                                                             "dynload",
                                                             "DynLoad",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }
#endif

    WriteStartupSelfTestMarker("running", "vx-bindings", std::string());
    if (!RunScriptVxBindingSelfTest(manager ? manager->GetScriptEngine() : nullptr, error)) {
        ReportSelfTestFailure(manager,
                                                             "vx-bindings",
                                                             "Vx bindings",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "behavior-bridge", std::string());
    if (!RunScriptBehaviorBridgeSelfTest(manager ? manager->GetCKContext() : nullptr,
                                         manager ? manager->GetScriptEngine() : nullptr,
                                         error)) {
        ReportSelfTestFailure(manager,
                                                             "behavior-bridge",
                                                             "Behavior bridge",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "executor", std::string());
    if (!RunScriptExecutorSelfTest(manager ? manager->GetCKContext() : nullptr,
                                   manager ? manager->GetScriptEngine() : nullptr,
                                   error)) {
        ReportSelfTestFailure(manager,
                              "executor",
                              "AngelScript Executor",
                              error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "scene", std::string());
    if (!RunScriptSceneSelfTest(manager ? manager->GetCKContext() : nullptr,
                                manager ? manager->GetScriptEngine() : nullptr,
                                error)) {
        ReportSelfTestFailure(manager, "scene", "Scene API", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "message", std::string());
    if (!ScriptMessageSelfTest::RunScriptMessageSelfTest(manager ? manager->GetCKContext() : nullptr,
                                                         manager ? manager->GetScriptEngine() : nullptr,
                                                         error)) {
        ReportSelfTestFailure(manager, "message", "Message", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "runtime", std::string());
    if (!RunScriptRuntimeSelfTest(manager ? manager->GetCKContext() : nullptr,
                                  manager ? manager->GetScriptEngine() : nullptr,
                                  error)) {
        ReportSelfTestFailure(manager, "runtime", "Runtime", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("running", "async", std::string());
    if (!RunScriptAsyncSelfTest(manager ? manager->GetCKContext() : nullptr,
                                manager ? manager->GetScriptEngine() : nullptr,
                                error)) {
        ReportSelfTestFailure(manager, "async", "Async", error);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("ok", "complete", std::string());
    return CK_OK;
}
