#include "ScriptSelfTests.h"

#include <cstdlib>
#include <fstream>
#include <string>

#include "Logger.h"
#include "ScriptManager.h"

namespace ScriptStartupSelfTestInternal {

bool StartupSelfTestsAttempted = false;

bool IsTruthyEnvironmentValue(const char *name) {
    const char *value = std::getenv(name);
    return value && value[0] != '\0' && !(value[0] == '0' && value[1] == '\0');
}

bool ShouldRunStartupSelfTests() {
    return IsTruthyEnvironmentValue("CKAS_RUN_SELFTESTS") || IsTruthyEnvironmentValue("CKAS_SELFTEST_MARKER");
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

} // namespace ScriptStartupSelfTestInternal

CKERROR RunScriptStartupSelfTests(ScriptManager *manager) {
    if (ScriptStartupSelfTestInternal::StartupSelfTestsAttempted) {
        return CK_OK;
    }
    if (!ScriptStartupSelfTestInternal::ShouldRunStartupSelfTests()) {
        return CK_OK;
    }
    ScriptStartupSelfTestInternal::StartupSelfTestsAttempted = true;

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "start", std::string());

    std::string error;
    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "parameter-conversion", std::string());
    if (!RunScriptParameterConversionSelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "parameter-conversion",
                                                             "Parameter conversion",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "parameter-registry", std::string());
    if (!RunScriptParameterRegistrySelfTest(manager ? manager->GetCKContext() : nullptr, error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "parameter-registry",
                                                             "Parameter registry",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "manager-api", std::string());
    if (!RunScriptApiSelfTest(manager ? manager->GetCKContext() : nullptr, error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "manager-api",
                                                             "CKAngelScript API",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

#if CKAS_ENABLE_API_EXPORT
    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "script-info-export", std::string());
    if (!RunScriptInfoExportSelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "script-info-export",
                                                             "ScriptInfo export",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }
#endif

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "component-metadata", std::string());
    if (!RunScriptComponentMetadataSelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "component-metadata",
                                                             "Component metadata",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "runtime-metadata", std::string());
    if (!ScriptRuntimeMetadata::RunScriptRuntimeMetadataSelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "runtime-metadata",
                                                             "Runtime metadata",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "runtime-dependency", std::string());
    if (!ScriptRuntimeDependencyResolver::RunScriptRuntimeDependencySelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "runtime-dependency",
                                                             "Runtime dependency",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "native-memory", std::string());
    if (!RunScriptNativeMemorySelfTest(manager ? manager->GetCKContext() : nullptr,
                                       manager ? manager->GetScriptEngine() : nullptr,
                                       error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "native-memory",
                                                             "Native memory",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "vx-bindings", std::string());
    if (!RunScriptVxBindingSelfTest(error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "vx-bindings",
                                                             "Vx bindings",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "behavior-bridge", std::string());
    if (!RunScriptBehaviorBridgeSelfTest(manager ? manager->GetCKContext() : nullptr,
                                         manager ? manager->GetScriptEngine() : nullptr,
                                         error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager,
                                                             "behavior-bridge",
                                                             "Behavior bridge",
                                                             error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "scene", std::string());
    if (!RunScriptSceneSelfTest(manager ? manager->GetCKContext() : nullptr,
                                manager ? manager->GetScriptEngine() : nullptr,
                                error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager, "scene", "Scene API", error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "message", std::string());
    if (!ScriptMessageSelfTest::RunScriptMessageSelfTest(manager ? manager->GetCKContext() : nullptr,
                                                         manager ? manager->GetScriptEngine() : nullptr,
                                                         error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager, "message", "Message", error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "runtime", std::string());
    if (!RunScriptRuntimeSelfTest(manager ? manager->GetCKContext() : nullptr,
                                  manager ? manager->GetScriptEngine() : nullptr,
                                  error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager, "runtime", "Runtime", error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("running", "async", std::string());
    if (!RunScriptAsyncSelfTest(manager ? manager->GetCKContext() : nullptr,
                                manager ? manager->GetScriptEngine() : nullptr,
                                error)) {
        ScriptStartupSelfTestInternal::ReportSelfTestFailure(manager, "async", "Async", error);
        return CKERR_INVALIDOPERATION;
    }

    ScriptStartupSelfTestInternal::WriteStartupSelfTestMarker("ok", "complete", std::string());
    return CK_OK;
}
