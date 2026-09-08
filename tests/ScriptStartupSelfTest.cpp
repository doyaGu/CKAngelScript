#include "ScriptSelfTests.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include "Logger.h"
#include "ScriptCKPrototypeHook.h"
#include "ScriptManager.h"

namespace {

bool StartupSelfTestsAttempted = false;
constexpr const char *kPlayerLifecycleModuleName = "__CKAS_PlayerManagerLifecycleTrace";

struct PlayerLifecycleTraceState {
    bool Active = false;
    bool AwaitingEnd = false;
    bool ModuleLoaded = false;
    CKAngelScriptFunction *Function = nullptr;
    CKAngelScriptExecution *Execution = nullptr;
    std::string ExecutorTrace;
    std::string EndPreparationFailure;
    std::vector<std::string> ManagerEvents;
};

PlayerLifecycleTraceState PlayerLifecycleTrace;

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

bool ShouldRunPlayerLifecycleTrace() {
    return IsTruthyEnvironmentValue("CKAS_SELFTEST_PLAYER_LIFECYCLE");
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

void WriteStartupSelfTestMarker(const char *status,
                                const char *stage,
                                const std::string &message,
                                const std::string &trace = std::string()) {
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
    if (!trace.empty()) {
        out << "trace=" << trace << "\n";
    }
}

std::string BuildPlayerLifecycleTrace() {
    std::ostringstream trace;
    if (!PlayerLifecycleTrace.ExecutorTrace.empty()) {
        trace << PlayerLifecycleTrace.ExecutorTrace << ";";
    }
    trace << "manager:";
    for (size_t i = 0; i < PlayerLifecycleTrace.ManagerEvents.size(); ++i) {
        if (i > 0) {
            trace << ">";
        }
        trace << PlayerLifecycleTrace.ManagerEvents[i];
    }
    return trace.str();
}

bool ReleasePlayerLifecycleTraceHandles(ScriptManager *manager,
                                        std::string *cleanupError = nullptr) {
    CKAngelScriptApi api = manager && manager->GetCKContext()
                                ? CKAngelScriptApi::Get(manager->GetCKContext())
                                : CKAngelScriptApi();
    bool ok = api.IsValid();
    if (api.IsValid()) {
        if (PlayerLifecycleTrace.Execution) {
            ok = api.ReleaseExecution(PlayerLifecycleTrace.Execution, nullptr) == CKAS_OK && ok;
        }
        if (PlayerLifecycleTrace.Function) {
            ok = api.ReleaseFunction(PlayerLifecycleTrace.Function, nullptr) == CKAS_OK && ok;
        }
        if (PlayerLifecycleTrace.ModuleLoaded) {
            ok = api.UnloadModule(kPlayerLifecycleModuleName, nullptr) == CKAS_OK && ok;
        }
    }
    PlayerLifecycleTrace.Execution = nullptr;
    PlayerLifecycleTrace.Function = nullptr;
    PlayerLifecycleTrace.ModuleLoaded = false;
    if (!ok && cleanupError) {
        *cleanupError = "Player lifecycle trace could not release its public handles and module.";
    }
    return ok;
}

bool ExpectManagerTracePrefix(std::initializer_list<const char *> expected) {
    if (PlayerLifecycleTrace.ManagerEvents.size() != expected.size()) {
        return false;
    }
    size_t index = 0;
    for (const char *event : expected) {
        if (PlayerLifecycleTrace.ManagerEvents[index++] != (event ? event : "")) {
            return false;
        }
    }
    return true;
}

bool ManagerEndTraceMatches() {
    const std::vector<std::string> prefix = {
        "CKContext.Pause",
        "OnCKPause",
        "CKContext.Pause.return",
        "CKContext.Play",
        "OnCKPlay",
        "CKContext.Play.return",
        "WM_CLOSE.posted",
    };
    const std::vector<std::string> &events = PlayerLifecycleTrace.ManagerEvents;
    if (events.size() < prefix.size() + 2 ||
        !std::equal(prefix.begin(), prefix.end(), events.begin())) {
        return false;
    }

    size_t index = prefix.size();
    while (index + 1 < events.size()) {
        if (events[index] == "PreClearAll") {
            if (events[index + 1] != "PostClearAll") {
                return false;
            }
            index += 2;
            continue;
        }
        if (events[index] == "OnCKReset") {
            if (events[index + 1] != "OnCKPostReset") {
                return false;
            }
            index += 2;
            continue;
        }
        break;
    }
    return index + 3 == events.size() &&
           events[index] == "OnCKEnd.begin" &&
           events[index + 1] == "EndProbe.armed" &&
           events[index + 2] == "OnCKEnd.complete";
}

bool BeginPlayerLifecycleTrace(ScriptManager *manager,
                               const std::string &executorTrace,
                               std::string &error) {
    if (!manager || !manager->GetCKContext()) {
        error = "Player lifecycle trace requires a CKContext.";
        return false;
    }

    CKContext *context = manager->GetCKContext();
    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Player lifecycle trace could not retrieve the public API.";
        return false;
    }

    PlayerLifecycleTrace = PlayerLifecycleTraceState();
    PlayerLifecycleTrace.ExecutorTrace = executorTrace;
    CKLevel *temporaryLevel = nullptr;

    CKAngelScriptResult result = CKAngelScriptApi::Result();
    const char *source =
        "void __ckas_wait_for_player_end() {\n"
        "  AsyncTask<void>@ delay = Async::Delay(1000);\n"
        "  Await(delay);\n"
        "}\n";
    auto fail = [&](const std::string &message) {
        error = message;
        if (PlayerLifecycleTrace.Active) {
            error += " Trace: ";
            error += BuildPlayerLifecycleTrace();
        }
        if (temporaryLevel) {
            context->SetCurrentLevel(nullptr);
            if (!temporaryLevel->IsToBeDeleted()) {
                context->DestroyObject(temporaryLevel);
            }
            temporaryLevel = nullptr;
        }
        ReleasePlayerLifecycleTraceHandles(manager);
        PlayerLifecycleTrace = PlayerLifecycleTraceState();
        return false;
    };

    if (api.CompileModule(kPlayerLifecycleModuleName,
                          source,
                          CKAS_COMPILE_REPLACEEXISTING,
                          &result) != CKAS_OK) {
        return fail(std::string("Player lifecycle trace could not load its wait module: ") +
                    (result.ErrorMessage ? result.ErrorMessage : "no detail"));
    }
    PlayerLifecycleTrace.ModuleLoaded = true;

    const CKAngelScriptFunctionOptions functionOptions =
        CKAngelScriptApi::FunctionByDeclOptions(
            kPlayerLifecycleModuleName,
            "void __ckas_wait_for_player_end()");
    if (api.FindFunction(functionOptions, &PlayerLifecycleTrace.Function, &result) != CKAS_OK ||
        !PlayerLifecycleTrace.Function) {
        return fail(std::string("Player lifecycle trace could not resolve its wait function: ") +
                    (result.ErrorMessage ? result.ErrorMessage : "no detail"));
    }

    if (!context->IsPlaying()) {
        return fail("Player lifecycle trace did not begin on a playing CKContext.");
    }
    if (!context->GetCurrentLevel()) {
        temporaryLevel = CKLevel::Cast(context->CreateObject(
            CKCID_LEVEL,
            const_cast<CKSTRING>("__CKAS_PlayerLifecycleTraceLevel"),
            CK_OBJECTCREATION_DYNAMIC));
        if (!temporaryLevel) {
            return fail("Player lifecycle trace could not create a temporary current level.");
        }
        context->SetCurrentLevel(temporaryLevel);
    }

    // Player can enter its first processed frame with both Playing and Reseted
    // set. Normalize that original bootstrap state before creating the handle
    // whose terminal cause must be clear/end rather than the implicit Reset in
    // the first subsequent Play call.
    if (context->IsReseted()) {
        if (context->Pause() != CK_OK || context->IsPlaying()) {
            return fail("Original Player could not pause its bootstrap reset state.");
        }
        const CKERROR bootstrapPlayError = context->Play();
        if ((bootstrapPlayError != CK_OK && bootstrapPlayError != CKERR_NOCURRENTLEVEL) ||
            !context->IsPlaying() || context->IsReseted()) {
            return fail("Original Player could not normalize its bootstrap reset state "
                        "(CKERROR=" + std::to_string(bootstrapPlayError) + ").");
        }
    }

    const CKAngelScriptFunctionExecutionOptions executionOptions =
        CKAngelScriptApi::FunctionExecutionOptions(PlayerLifecycleTrace.Function);
    if (api.CreateFunctionExecution(executionOptions,
                                    &PlayerLifecycleTrace.Execution,
                                    &result) != CKAS_OK ||
        !PlayerLifecycleTrace.Execution ||
        api.StartExecution(PlayerLifecycleTrace.Execution, nullptr, &result) != CKAS_SUSPENDED) {
        return fail(std::string("Player lifecycle trace could not suspend its public execution: ") +
                    (result.ErrorMessage ? result.ErrorMessage : "no detail"));
    }

    PlayerLifecycleTrace.Active = true;
    PlayerLifecycleTrace.ManagerEvents.push_back("CKContext.Pause");
    if (context->Pause() != CK_OK || context->IsPlaying() ||
        !ExpectManagerTracePrefix({"CKContext.Pause", "OnCKPause"})) {
        return fail("Original Player did not dispatch manager OnCKPause through CKContext::Pause.");
    }
    PlayerLifecycleTrace.ManagerEvents.push_back("CKContext.Pause.return");
    PlayerLifecycleTrace.ManagerEvents.push_back("CKContext.Play");
    const CKERROR playError = context->Play();
    const CKBOOL playingAfterPlay = context->IsPlaying();
    const bool playTraceMatches =
        ExpectManagerTracePrefix({"CKContext.Pause",
                                  "OnCKPause",
                                  "CKContext.Pause.return",
                                  "CKContext.Play",
                                  "OnCKPlay"});
    const bool acceptedOriginalPlayResult =
        playError == CK_OK || playError == CKERR_NOCURRENTLEVEL;
    if (!acceptedOriginalPlayResult || !playingAfterPlay || !playTraceMatches) {
        return fail("Original Player did not complete CKContext::Play after manager OnCKPlay "
                    "(CKERROR=" + std::to_string(playError) +
                    ", playing=" + std::to_string(static_cast<int>(playingAfterPlay)) +
                    ", traceMatches=" + (playTraceMatches ? "true" : "false") + ").");
    }
    PlayerLifecycleTrace.ManagerEvents.push_back("CKContext.Play.return");

    if (temporaryLevel) {
        context->SetCurrentLevel(nullptr);
        const CKERROR destroyLevelError = context->DestroyObject(temporaryLevel);
        temporaryLevel = nullptr;
        if (destroyLevelError != CK_OK) {
            return fail("Player lifecycle trace could not release its temporary current level.");
        }
    }

    CKAS_EXECUTIONSTATE executionState = CKAS_EXECUTION_READY;
    if (api.GetExecutionState(PlayerLifecycleTrace.Execution,
                              &executionState,
                              &result) != CKAS_OK ||
        executionState != CKAS_EXECUTION_SUSPENDED) {
        return fail("Pause/Play changed the terminal state of a suspended public execution.");
    }

    const HWND window = static_cast<HWND>(context->GetMainWindow());
    if (!window || !PostMessageA(window, WM_CLOSE, 0, 0)) {
        return fail("Player lifecycle trace could not request a graceful Player close.");
    }
    PlayerLifecycleTrace.ManagerEvents.push_back("WM_CLOSE.posted");
    PlayerLifecycleTrace.AwaitingEnd = true;
    WriteStartupSelfTestMarker("running",
                               "manager-end",
                               std::string(),
                               BuildPlayerLifecycleTrace());
    return true;
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

void RecordScriptManagerLifecycleSelfTestEvent(const char *event) {
    if (PlayerLifecycleTrace.Active && event && event[0] != '\0') {
        PlayerLifecycleTrace.ManagerEvents.emplace_back(event);
    }
}

void PrepareScriptManagerLifecycleEndSelfTest(ScriptManager *manager) {
    if (!PlayerLifecycleTrace.Active || !PlayerLifecycleTrace.AwaitingEnd ||
        !manager || !manager->GetCKContext()) {
        return;
    }

    CKAngelScriptApi api = CKAngelScriptApi::Get(manager->GetCKContext());
    CKAngelScriptResult result = CKAngelScriptApi::Result();
    if (!api.IsValid() || !PlayerLifecycleTrace.Function) {
        PlayerLifecycleTrace.EndPreparationFailure =
            "Player OnCKEnd could not access the lifecycle trace function handle.";
        RecordScriptManagerLifecycleSelfTestEvent("EndProbe.failed");
        return;
    }

    if (PlayerLifecycleTrace.Execution) {
        const CKAS_STATUS releaseStatus =
            api.ReleaseExecution(PlayerLifecycleTrace.Execution, &result);
        PlayerLifecycleTrace.Execution = nullptr;
        if (releaseStatus != CKAS_OK) {
            PlayerLifecycleTrace.EndPreparationFailure =
                "Player OnCKEnd could not release the pre-End execution handle.";
            RecordScriptManagerLifecycleSelfTestEvent("EndProbe.failed");
            return;
        }
    }

    const CKAngelScriptFunctionExecutionOptions executionOptions =
        CKAngelScriptApi::FunctionExecutionOptions(PlayerLifecycleTrace.Function);
    if (api.CreateFunctionExecution(executionOptions,
                                    &PlayerLifecycleTrace.Execution,
                                    &result) != CKAS_OK ||
        !PlayerLifecycleTrace.Execution ||
        api.StartExecution(PlayerLifecycleTrace.Execution, nullptr, &result) != CKAS_SUSPENDED) {
        PlayerLifecycleTrace.EndPreparationFailure =
            std::string("Player OnCKEnd could not arm a suspended execution before cancellation: ") +
            (result.ErrorMessage ? result.ErrorMessage : "no detail");
        RecordScriptManagerLifecycleSelfTestEvent("EndProbe.failed");
        return;
    }

    RecordScriptManagerLifecycleSelfTestEvent("EndProbe.armed");
}

CKERROR CompleteScriptManagerLifecycleSelfTest(ScriptManager *manager) {
    if (!PlayerLifecycleTrace.Active || !PlayerLifecycleTrace.AwaitingEnd) {
        return CK_OK;
    }

    RecordScriptManagerLifecycleSelfTestEvent("OnCKEnd.complete");
    CKAngelScriptApi api = manager && manager->GetCKContext()
                                ? CKAngelScriptApi::Get(manager->GetCKContext())
                                : CKAngelScriptApi();
    CKAngelScriptResult result = CKAngelScriptApi::Result();
    const CKAngelScriptResult *storedResult = nullptr;
    CKAS_EXECUTIONSTATE executionState = CKAS_EXECUTION_READY;
    std::string failure;

    const CKAS_STATUS stateStatus = api.IsValid() && PlayerLifecycleTrace.Execution
                                        ? api.GetExecutionState(PlayerLifecycleTrace.Execution,
                                                                &executionState,
                                                                &result)
                                        : CKAS_NOTINITIALIZED;
    const CKAS_STATUS borrowStatus = stateStatus == CKAS_OK
                                         ? api.BorrowExecutionResult(PlayerLifecycleTrace.Execution,
                                                                     &storedResult,
                                                                     &result)
                                         : CKAS_NOTINITIALIZED;
    const std::string cancellationReason =
        storedResult && storedResult->ErrorMessage ? storedResult->ErrorMessage : "";
    const bool cancelledByEnd =
        cancellationReason == "Execution was cancelled because CK is ending.";

    if (!PlayerLifecycleTrace.EndPreparationFailure.empty()) {
        failure = PlayerLifecycleTrace.EndPreparationFailure;
    } else if (!ManagerEndTraceMatches()) {
        failure = "Original Player manager lifecycle callbacks were not delivered in the required order.";
    } else if (!api.IsValid() || !PlayerLifecycleTrace.Execution) {
        failure = "Player OnCKEnd lost the public execution test handle.";
    } else if (stateStatus != CKAS_OK ||
               executionState != CKAS_EXECUTION_CANCELLED ||
               borrowStatus != CKAS_OK ||
               !storedResult ||
               storedResult->Status != CKAS_CANCELLED ||
               !cancelledByEnd) {
        failure = "Player hard teardown did not preserve cancellation for a suspended public execution "
                  "(stateStatus=" + std::to_string(static_cast<int>(stateStatus)) +
                  ", state=" + std::to_string(static_cast<int>(executionState)) +
                  ", borrowStatus=" + std::to_string(static_cast<int>(borrowStatus)) +
                  ", storedStatus=" +
                  std::to_string(storedResult ? static_cast<int>(storedResult->Status) : -1) +
                  ", reason='" + cancellationReason + "').";
    } else if (api.ResumeExecution(PlayerLifecycleTrace.Execution,
                                   nullptr,
                                   &result) != CKAS_INVALIDSTATE ||
               result.Status != CKAS_INVALIDSTATE) {
        failure = "A public execution cancelled by Player OnCKEnd could still be resumed.";
    } else {
        storedResult = nullptr;
        if (api.BorrowExecutionResult(PlayerLifecycleTrace.Execution,
                                      &storedResult,
                                      &result) != CKAS_OK ||
            !storedResult ||
            storedResult->Status != CKAS_CANCELLED) {
            failure = "Rejected resume erased the Player OnCKEnd cancellation result.";
        }
    }

    const std::string trace = BuildPlayerLifecycleTrace();
    std::string cleanupFailure;
    if (!ReleasePlayerLifecycleTraceHandles(manager, &cleanupFailure) && failure.empty()) {
        failure = cleanupFailure;
    }
    PlayerLifecycleTrace = PlayerLifecycleTraceState();
    if (!failure.empty()) {
        WriteStartupSelfTestMarker("failed", "manager-end", failure, trace);
        return CKERR_INVALIDOPERATION;
    }

    WriteStartupSelfTestMarker("ok", "complete", std::string(), trace);
    return CK_OK;
}

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

    WriteStartupSelfTestMarker("running", "script-invocation", std::string());
    if (!RunScriptInvocationSelfTest(manager, error)) {
        ReportSelfTestFailure(manager, "script-invocation", "Script invocation", error);
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
    std::string executorPauseResumeTrace;
    if (!RunScriptExecutorSelfTest(manager ? manager->GetCKContext() : nullptr,
                                   manager ? manager->GetScriptEngine() : nullptr,
                                   error,
                                   &executorPauseResumeTrace)) {
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

    if (ShouldRunPlayerLifecycleTrace()) {
        WriteStartupSelfTestMarker("running", "manager-lifecycle", std::string(), executorPauseResumeTrace);
        if (!BeginPlayerLifecycleTrace(manager, executorPauseResumeTrace, error)) {
            ReportSelfTestFailure(manager,
                                  "manager-lifecycle",
                                  "Player manager lifecycle",
                                  error);
            return CKERR_INVALIDOPERATION;
        }
        return CK_OK;
    }

    WriteStartupSelfTestMarker("ok", "complete", std::string(), executorPauseResumeTrace);
    return CK_OK;
}
