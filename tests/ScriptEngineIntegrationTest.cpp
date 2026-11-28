#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#if !CKAS_RUNTIME_SDL
#include <windows.h>
#endif

#include "CKAll.h"
#include "CKAngelScript.h"
#include "CKPluginManager.h"
#include "ScriptObjectIdentity.h"

namespace {

const CKGUID kScriptManagerGuid(0x70955bd2, 0x30684456);
const CKGUID kLifecycleProbeManagerGuid(0x6b845a4d, 0x4dca61ab);
constexpr const char *kResetWaitModuleName = "__CKAS_EngineResetWaitIntegration";
constexpr const char *kComponentMutationModuleName = "__CKAS_EngineComponentMutationIntegration";
constexpr int kComponentScriptParameter = 0;
constexpr int kComponentClassParameter = 1;
constexpr int kComponentSourceParameter = 2;
constexpr int kComponentFileParameter = 3;
constexpr int kComponentManifestParameter = 4;

struct PublicApiSymbols {
    decltype(&CKGetAngelScript) GetAngelScript = nullptr;
    decltype(&CKAngelScriptInitResult) InitResult = nullptr;
    decltype(&CKAngelScriptInitLoadOptions) InitLoadOptions = nullptr;
    decltype(&CKAngelScriptInitFunctionOptions) InitFunctionOptions = nullptr;
    decltype(&CKAngelScriptInitFunctionExecutionOptions) InitFunctionExecutionOptions = nullptr;
    decltype(&CKAngelScriptInitExecutionStepOptions) InitExecutionStepOptions = nullptr;
    decltype(&CKAngelScriptLoadModule) LoadModule = nullptr;
    decltype(&CKAngelScriptUnloadModule) UnloadModule = nullptr;
    decltype(&CKAngelScriptGetModuleGeneration) GetModuleGeneration = nullptr;
    decltype(&CKAngelScriptFindFunction) FindFunction = nullptr;
    decltype(&CKAngelScriptReleaseFunction) ReleaseFunction = nullptr;
    decltype(&CKAngelScriptCreateFunctionExecution) CreateFunctionExecution = nullptr;
    decltype(&CKAngelScriptStartExecution) StartExecution = nullptr;
    decltype(&CKAngelScriptResumeExecution) ResumeExecution = nullptr;
    decltype(&CKAngelScriptReleaseExecution) ReleaseExecution = nullptr;
    decltype(&CKAngelScriptGetExecutionState) GetExecutionState = nullptr;
    decltype(&CKAngelScriptBorrowExecutionResult) BorrowExecutionResult = nullptr;

    bool Resolve(const char *pluginPath, std::string &error) {
#if CKAS_RUNTIME_SDL
        auto *plugins = CKGetPluginManager();
        auto *module = plugins ? plugins->GetPluginDllInfo(pluginPath) : nullptr;
#else
        const std::string path = pluginPath ? pluginPath : "";
        const auto separator = path.find_last_of("\\/");
        const auto name = path.substr(separator == std::string::npos ? 0 : separator + 1);
        const auto module = name.empty() ? nullptr : GetModuleHandleA(name.c_str());
#endif
        if (!module) {
            error = "Could not find the CKAngelScript module loaded by CKPluginManager.";
            return false;
        }

#if CKAS_RUNTIME_SDL
#define CKAS_FIND_SYMBOL(module, name) (module)->GetFunctionPtr(name)
#else
#define CKAS_FIND_SYMBOL(module, name) GetProcAddress(module, name)
#endif
#define CKAS_RESOLVE_SYMBOL(Member, Symbol)                                                        \
    Member = reinterpret_cast<decltype(Member)>(CKAS_FIND_SYMBOL(module, #Symbol));                  \
    if (!Member) {                                                                                 \
        error = "CKAngelScript plugin is missing public ABI symbol " #Symbol ".";                 \
        return false;                                                                              \
    }

        CKAS_RESOLVE_SYMBOL(GetAngelScript, CKGetAngelScript)
        CKAS_RESOLVE_SYMBOL(InitResult, CKAngelScriptInitResult)
        CKAS_RESOLVE_SYMBOL(InitLoadOptions, CKAngelScriptInitLoadOptions)
        CKAS_RESOLVE_SYMBOL(InitFunctionOptions, CKAngelScriptInitFunctionOptions)
        CKAS_RESOLVE_SYMBOL(InitFunctionExecutionOptions, CKAngelScriptInitFunctionExecutionOptions)
        CKAS_RESOLVE_SYMBOL(InitExecutionStepOptions, CKAngelScriptInitExecutionStepOptions)
        CKAS_RESOLVE_SYMBOL(LoadModule, CKAngelScriptLoadModule)
        CKAS_RESOLVE_SYMBOL(UnloadModule, CKAngelScriptUnloadModule)
        CKAS_RESOLVE_SYMBOL(GetModuleGeneration, CKAngelScriptGetModuleGeneration)
        CKAS_RESOLVE_SYMBOL(FindFunction, CKAngelScriptFindFunction)
        CKAS_RESOLVE_SYMBOL(ReleaseFunction, CKAngelScriptReleaseFunction)
        CKAS_RESOLVE_SYMBOL(CreateFunctionExecution, CKAngelScriptCreateFunctionExecution)
        CKAS_RESOLVE_SYMBOL(StartExecution, CKAngelScriptStartExecution)
        CKAS_RESOLVE_SYMBOL(ResumeExecution, CKAngelScriptResumeExecution)
        CKAS_RESOLVE_SYMBOL(ReleaseExecution, CKAngelScriptReleaseExecution)
        CKAS_RESOLVE_SYMBOL(GetExecutionState, CKAngelScriptGetExecutionState)
        CKAS_RESOLVE_SYMBOL(BorrowExecutionResult, CKAngelScriptBorrowExecutionResult)

#undef CKAS_RESOLVE_SYMBOL
#undef CKAS_FIND_SYMBOL
        return true;
    }
};

enum class ProbeEvent {
    Reset,
    PostReset,
    SequenceToBeDeleted,
    PreClearAll,
    PostClearAll,
};

class LifecycleProbeManager final : public CKBaseManager {
public:
    explicit LifecycleProbeManager(CKContext *context)
        : CKBaseManager(context,
                        kLifecycleProbeManagerGuid,
                        const_cast<CKSTRING>("CKAngelScript Lifecycle Probe")) {
        RegistrationError = context ? context->RegisterNewManager(this) : CKERR_INVALIDPARAMETER;
    }

    CKDWORD GetValidFunctionsMask() override {
        return CKMANAGER_FUNC_OnCKReset |
               CKMANAGER_FUNC_OnCKPostReset |
               CKMANAGER_FUNC_OnSequenceToBeDeleted |
               CKMANAGER_FUNC_PreClearAll |
               CKMANAGER_FUNC_PostClearAll;
    }

    CKERROR OnCKReset() override {
        Events.push_back(ProbeEvent::Reset);
        ResetProbeWasLive = m_Context && m_Context->GetObject(ResetProbeId) != nullptr;
        CKLevel *currentLevel = m_Context ? m_Context->GetCurrentLevel() : nullptr;
        ResetCurrentLevelMatched = currentLevel && currentLevel->GetID() == ResetProbeId;
        return CK_OK;
    }

    CKERROR OnCKPostReset() override {
        Events.push_back(ProbeEvent::PostReset);
        return CK_OK;
    }

    CKERROR SequenceToBeDeleted(CK_ID *objids, int count) override {
        Events.push_back(ProbeEvent::SequenceToBeDeleted);
        for (int i = 0; objids && i < count; ++i) {
            if (objids[i] == DeleteProbeId) {
                SawDeleteProbe = true;
                CKObject *object = m_Context ? m_Context->GetObject(objids[i]) : nullptr;
                DeleteProbeWasLive = object != nullptr;
                DeleteProbeWasPendingDeletion = object && object->IsToBeDeleted();
                DefaultIdentityRejectedPendingDeletion =
                    GetStampedCKObjectById(m_Context, objids[i], DeleteProbeStamp) == nullptr;
                TerminalIdentityResolvedPendingDeletion =
                    GetStampedCKObjectById(
                        m_Context,
                        objids[i],
                        DeleteProbeStamp,
                        ScriptBridgeObjectResolvePolicy::IncludePendingDeletion) == object;
            }
        }
        return CK_OK;
    }

    CKERROR PreClearAll() override {
        Events.push_back(ProbeEvent::PreClearAll);
        ClearProbeWasLive = m_Context && m_Context->GetObject(ClearProbeId) != nullptr;
        return CK_OK;
    }

    CKERROR PostClearAll() override {
        Events.push_back(ProbeEvent::PostClearAll);
        ClearProbeWasGone = !m_Context || m_Context->GetObject(ClearProbeId) == nullptr;
        return CK_OK;
    }

    CKERROR RegistrationError = CKERR_NOTINITIALIZED;
    CK_ID DeleteProbeId = 0;
    ScriptBridgeObjectStamp DeleteProbeStamp;
    CK_ID ResetProbeId = 0;
    CK_ID ClearProbeId = 0;
    bool SawDeleteProbe = false;
    bool DeleteProbeWasLive = false;
    bool DeleteProbeWasPendingDeletion = false;
    bool DefaultIdentityRejectedPendingDeletion = false;
    bool TerminalIdentityResolvedPendingDeletion = false;
    bool ResetProbeWasLive = false;
    bool ResetCurrentLevelMatched = false;
    bool ClearProbeWasLive = false;
    bool ClearProbeWasGone = false;
    std::vector<ProbeEvent> Events;
};

bool SawOrderedPair(const std::vector<ProbeEvent> &events,
                    ProbeEvent first,
                    ProbeEvent second) {
    bool sawFirst = false;
    for (ProbeEvent event : events) {
        if (event == first) {
            sawFirst = true;
        } else if (event == second) {
            return sawFirst;
        }
    }
    return false;
}

std::string ResultError(const CKAngelScriptResult &result) {
    return result.ErrorMessage && result.ErrorMessage[0]
               ? result.ErrorMessage
               : "no CKAngelScript error detail";
}

bool ResultContains(const CKAngelScriptResult &result, const char *needle) {
    return result.ErrorMessage && needle && std::strstr(result.ErrorMessage, needle) != nullptr;
}

CKBehaviorContext MakeBehaviorContext(CKContext *context, CKBehavior *behavior) {
    CKBehaviorContext behaviorContext = {};
    behaviorContext.Behavior = behavior;
    behaviorContext.Context = context;
    behaviorContext.ParameterManager = context ? context->GetParameterManager() : nullptr;
    behaviorContext.MessageManager = context ? context->GetMessageManager() : nullptr;
    behaviorContext.AttributeManager = context ? context->GetAttributeManager() : nullptr;
    behaviorContext.TimeManager = context ? context->GetTimeManager() : nullptr;
    return behaviorContext;
}

bool SetComponentStringInput(CKBehavior *behavior,
                             int index,
                             const char *value,
                             std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        error = "Original CK2 Component behavior is missing a fixed string input.";
        return false;
    }

    CKParameterIn *input = behavior->GetInputParameter(index);
    const std::string localName = "__CKAS_EngineComponentInput_" +
                                  std::to_string(behavior->GetID()) + "_" +
                                  std::to_string(index);
    CKParameterLocal *local = behavior->CreateLocalParameter(
        const_cast<CKSTRING>(localName.c_str()),
        CKPGUID_STRING);
    if (!input || !local ||
        local->SetStringValue(const_cast<CKSTRING>(value ? value : "")) != CK_OK) {
        error = "Could not create an original CK2 Component input source.";
        return false;
    }

    behavior->SetInputParameterDefaultValue(input, local);
    if (input->SetDirectSource(local) != CK_OK) {
        error = "Could not connect an original CK2 Component input source.";
        return false;
    }
    return true;
}

int ExecuteBehavior(CKContext *context,
                    CKBehavior *behavior,
                    const CKBehaviorContext &behaviorContext) {
    if (!context || !behavior) {
        return CKBR_BEHAVIORERROR;
    }

#if CKVERSION == 0x13022002
    const CKBehaviorContext savedContext = context->m_BehaviorContext;
    CKBehaviorManager *behaviorManager = context->GetBehaviorManager();
    CKBehavior *savedCurrent = behaviorManager ? behaviorManager->m_CurrentBehavior : nullptr;

    context->m_BehaviorContext = behaviorContext;
    const int returnCode = behavior->Execute(behaviorContext.DeltaTime);

    context->m_BehaviorContext = savedContext;
    if (behaviorManager) {
        behaviorManager->m_CurrentBehavior = savedCurrent;
    }
    return returnCode;
#else
    return behavior->Execute(behaviorContext.DeltaTime);
#endif
}

CKAS_STATUS ReadIntReturn(asIScriptContext *context, void *userData) {
    if (!context || !userData) {
        return CKAS_INVALIDARGUMENT;
    }
    *static_cast<int *>(userData) = static_cast<int>(context->GetReturnDWord());
    return CKAS_OK;
}

bool ExecuteIntFunction(PublicApiSymbols &api,
                        CKAngelScript *angelScript,
                        const char *moduleName,
                        const char *functionDecl,
                        int &value,
                        std::string &error) {
    CKAngelScriptResult result;
    api.InitResult(&result);
    CKAngelScriptFunctionOptions functionOptions;
    api.InitFunctionOptions(&functionOptions);
    functionOptions.ModuleName = moduleName;
    functionOptions.FunctionDecl = functionDecl;

    CKAngelScriptFunction *function = nullptr;
    CKAngelScriptExecution *execution = nullptr;
    auto cleanup = [&]() {
        if (execution) {
            api.ReleaseExecution(angelScript, execution, nullptr);
            execution = nullptr;
        }
        if (function) {
            api.ReleaseFunction(angelScript, function, nullptr);
            function = nullptr;
        }
    };

    if (api.FindFunction(angelScript, &functionOptions, &function, &result) != CKAS_OK || !function) {
        error = "Could not resolve module-mutation value probe: " + ResultError(result);
        cleanup();
        return false;
    }

    CKAngelScriptFunctionExecutionOptions executionOptions;
    api.InitFunctionExecutionOptions(&executionOptions);
    executionOptions.Function = function;
    if (api.CreateFunctionExecution(angelScript, &executionOptions, &execution, &result) != CKAS_OK ||
        !execution) {
        error = "Could not create module-mutation value execution: " + ResultError(result);
        cleanup();
        return false;
    }

    CKAngelScriptExecutionStepOptions stepOptions;
    api.InitExecutionStepOptions(&stepOptions);
    stepOptions.ReadResult = ReadIntReturn;
    stepOptions.UserData = &value;
    if (api.StartExecution(angelScript, execution, &stepOptions, &result) != CKAS_OK) {
        error = "Could not run module-mutation value probe: " + ResultError(result);
        cleanup();
        return false;
    }

    cleanup();
    return true;
}

bool RunSuspendedWaitResetProbe(CKContext *context,
                                const char *pluginPath,
                                CKERROR &resetError,
                                std::string &error) {
    PublicApiSymbols api;
    if (!api.Resolve(pluginPath, error)) {
        return false;
    }

    CKAngelScript *angelScript = api.GetAngelScript(context);
    if (!angelScript) {
        error = "CKGetAngelScript did not expose the manager initialized by original CK2.";
        return false;
    }

    CKAngelScriptFunction *function = nullptr;
    CKAngelScriptExecution *execution = nullptr;
    bool moduleLoaded = false;

    auto cleanup = [&]() {
        if (execution) {
            api.ReleaseExecution(angelScript, execution, nullptr);
            execution = nullptr;
        }
        if (function) {
            api.ReleaseFunction(angelScript, function, nullptr);
            function = nullptr;
        }
        if (moduleLoaded) {
            api.UnloadModule(angelScript, kResetWaitModuleName, nullptr);
            moduleLoaded = false;
        }
    };
    auto fail = [&](const std::string &message) {
        error = message;
        cleanup();
        return false;
    };

    CKAngelScriptResult result;
    api.InitResult(&result);
    CKAngelScriptLoadOptions loadOptions;
    api.InitLoadOptions(&loadOptions);
    loadOptions.ModuleName = kResetWaitModuleName;
    loadOptions.Code =
        "void __ckas_wait_for_original_reset() {\n"
        "  AsyncTask<void>@ delay = Async::Delay(100);\n"
        "  Await(delay);\n"
        "}\n";
    if (api.LoadModule(angelScript, &loadOptions, &result) != CKAS_OK) {
        return fail("Could not load the suspended-wait reset probe: " + ResultError(result));
    }
    moduleLoaded = true;

    CKAngelScriptFunctionOptions functionOptions;
    api.InitFunctionOptions(&functionOptions);
    functionOptions.ModuleName = kResetWaitModuleName;
    functionOptions.FunctionDecl = "void __ckas_wait_for_original_reset()";
    if (api.FindFunction(angelScript, &functionOptions, &function, &result) != CKAS_OK || !function) {
        return fail("Could not resolve the suspended-wait reset probe: " + ResultError(result));
    }

    CKAngelScriptFunctionExecutionOptions executionOptions;
    api.InitFunctionExecutionOptions(&executionOptions);
    executionOptions.Function = function;
    if (api.CreateFunctionExecution(angelScript, &executionOptions, &execution, &result) != CKAS_OK ||
        !execution) {
        return fail("Could not create the suspended-wait execution handle: " + ResultError(result));
    }
    if (api.StartExecution(angelScript, execution, nullptr, &result) != CKAS_SUSPENDED) {
        return fail("The original-engine wait probe did not suspend: " + ResultError(result));
    }

    CKAS_EXECUTIONSTATE state = CKAS_EXECUTION_READY;
    if (api.GetExecutionState(angelScript, execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_SUSPENDED) {
        return fail("The public execution handle did not retain its suspended state before reset.");
    }

    resetError = context->Reset();
    if (resetError != CK_OK) {
        return fail("Original CK2 Reset failed while cancelling a suspended public execution.");
    }

    if (api.GetExecutionState(angelScript, execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_CANCELLED) {
        return fail("Original CK2 Reset did not move the suspended public execution to CANCELLED.");
    }

    const CKAngelScriptResult *storedResult = nullptr;
    if (api.BorrowExecutionResult(angelScript, execution, &storedResult, &result) != CKAS_OK ||
        !storedResult ||
        storedResult->Status != CKAS_CANCELLED ||
        !storedResult->ErrorMessage ||
        std::strcmp(storedResult->ErrorMessage,
                    "Execution was cancelled because CK is resetting.") != 0) {
        return fail("Original CK2 Reset did not preserve the public cancellation result and reason.");
    }

    api.InitResult(&result);
    if (api.ResumeExecution(angelScript, execution, nullptr, &result) != CKAS_INVALIDSTATE ||
        result.Status != CKAS_INVALIDSTATE) {
        return fail("A public execution cancelled by original CK2 Reset could still be resumed.");
    }

    storedResult = nullptr;
    if (api.BorrowExecutionResult(angelScript, execution, &storedResult, &result) != CKAS_OK ||
        !storedResult ||
        storedResult->Status != CKAS_CANCELLED ||
        !storedResult->ErrorMessage ||
        std::strcmp(storedResult->ErrorMessage,
                    "Execution was cancelled because CK is resetting.") != 0) {
        return fail("Rejected resume erased the stored reset cancellation result.");
    }

    cleanup();
    return true;
}

bool RunComponentModuleMutationProbe(CKContext *context,
                                     const char *pluginPath,
                                     std::string &error) {
    PublicApiSymbols api;
    if (!api.Resolve(pluginPath, error)) {
        return false;
    }

    CKAngelScript *angelScript = api.GetAngelScript(context);
    if (!angelScript) {
        error = "CKGetAngelScript did not expose the manager initialized by original CK2.";
        return false;
    }

    CKBehavior *behavior = nullptr;
    bool moduleLoaded = false;
    auto cleanup = [&]() {
        if (behavior && !behavior->IsToBeDeleted()) {
            context->DestroyObject(behavior);
        }
        behavior = nullptr;
        if (moduleLoaded) {
            api.UnloadModule(angelScript, kComponentMutationModuleName, nullptr);
            moduleLoaded = false;
        }
    };
    auto fail = [&](const std::string &message) {
        error = message;
        cleanup();
        return false;
    };

    constexpr const char *initialSource =
        "int __ckas_engine_component_updates = 0;\n"
        "int __ckas_engine_component_value() { return __ckas_engine_component_updates; }\n"
        "class EngineMutationComponent {\n"
        "  void Update(BehaviorFrame@ frame) { __ckas_engine_component_updates++; }\n"
        "}\n";
    constexpr const char *replacementSource =
        "int __ckas_engine_component_value() { return 1000; }\n"
        "class EngineMutationComponent {\n"
        "  void Update(BehaviorFrame@ frame) {}\n"
        "}\n";

    CKAngelScriptResult result;
    api.InitResult(&result);
    CKAngelScriptLoadOptions loadOptions;
    api.InitLoadOptions(&loadOptions);
    loadOptions.ModuleName = kComponentMutationModuleName;
    loadOptions.Code = initialSource;
    if (api.LoadModule(angelScript, &loadOptions, &result) != CKAS_OK) {
        return fail("Could not load the shared Component module: " + ResultError(result));
    }
    moduleLoaded = true;

    const CKDWORD initialGeneration =
        api.GetModuleGeneration(angelScript, kComponentMutationModuleName);
    if (initialGeneration == 0) {
        return fail("The shared Component module did not receive a generation.");
    }

    behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_EngineComponentMutationBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        return fail("Could not create the original CK2 Component behavior.");
    }
    behavior->UseFunction();
    const CKERROR initError =
        behavior->InitFromGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    if (initError != CK_OK) {
        return fail("Original CK2 could not initialize the AngelScript Component prototype.");
    }

    if (!SetComponentStringInput(behavior,
                                 kComponentScriptParameter,
                                 kComponentMutationModuleName,
                                 error) ||
        !SetComponentStringInput(behavior,
                                 kComponentClassParameter,
                                 "EngineMutationComponent",
                                 error) ||
        !SetComponentStringInput(behavior, kComponentSourceParameter, "", error) ||
        !SetComponentStringInput(behavior, kComponentFileParameter, "", error) ||
        !SetComponentStringInput(behavior, kComponentManifestParameter, "", error)) {
        cleanup();
        return false;
    }

    if (behavior->CallCallbackFunction(CKM_BEHAVIORCREATE) != CK_OK) {
        return fail("Original CK2 could not deliver CKM_BEHAVIORCREATE to the Component.");
    }
    const CKBehaviorContext behaviorContext = MakeBehaviorContext(context, behavior);
    behavior->ActivateInput(0, TRUE);
    if (ExecuteBehavior(context, behavior, behaviorContext) != CKBR_ACTIVATENEXTFRAME) {
        return fail("The shared-module Component did not enter its running state.");
    }

    int componentValue = -1;
    if (!ExecuteIntFunction(api,
                            angelScript,
                            kComponentMutationModuleName,
                            "int __ckas_engine_component_value()",
                            componentValue,
                            error) ||
        componentValue != 1) {
        if (error.empty()) {
            error = "The shared-module Component did not execute its original generation.";
        }
        cleanup();
        return false;
    }

    if (api.UnloadModule(angelScript, kComponentMutationModuleName, &result) != CKAS_INUSE ||
        !ResultContains(result, "component or executor") ||
        api.GetModuleGeneration(angelScript, kComponentMutationModuleName) != initialGeneration) {
        return fail("A live shared-module Component did not reject public unload without mutation.");
    }

    api.InitLoadOptions(&loadOptions);
    loadOptions.ModuleName = kComponentMutationModuleName;
    loadOptions.Code = replacementSource;
    loadOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api.LoadModule(angelScript, &loadOptions, &result) != CKAS_INUSE ||
        !ResultContains(result, "component or executor") ||
        api.GetModuleGeneration(angelScript, kComponentMutationModuleName) != initialGeneration) {
        return fail("A live shared-module Component did not reject same-name replacement without mutation.");
    }

    if (ExecuteBehavior(context, behavior, behaviorContext) != CKBR_ACTIVATENEXTFRAME) {
        return fail("Rejected module mutation disturbed the attached Component consumer.");
    }
    componentValue = -1;
    if (!ExecuteIntFunction(api,
                            angelScript,
                            kComponentMutationModuleName,
                            "int __ckas_engine_component_value()",
                            componentValue,
                            error) ||
        componentValue != 2) {
        if (error.empty()) {
            error = "Rejected module mutation changed the original Component generation.";
        }
        cleanup();
        return false;
    }

    const CKERROR destroyError = context->DestroyObject(behavior);
    behavior = nullptr;
    if (destroyError != CK_OK) {
        return fail("Original CK2 could not release the shared-module Component consumer.");
    }

    if (api.LoadModule(angelScript, &loadOptions, &result) != CKAS_OK) {
        return fail("Replacement remained blocked after the Component was released: " +
                    ResultError(result));
    }
    const CKDWORD replacementGeneration =
        api.GetModuleGeneration(angelScript, kComponentMutationModuleName);
    if (replacementGeneration != initialGeneration + 1) {
        return fail("Successful Component module replacement did not increment generation exactly once.");
    }

    componentValue = -1;
    if (!ExecuteIntFunction(api,
                            angelScript,
                            kComponentMutationModuleName,
                            "int __ckas_engine_component_value()",
                            componentValue,
                            error) ||
        componentValue != 1000) {
        if (error.empty()) {
            error = "Successful replacement did not publish the new Component module generation.";
        }
        cleanup();
        return false;
    }

    if (api.UnloadModule(angelScript, kComponentMutationModuleName, &result) != CKAS_OK) {
        return fail("Shared Component module did not unload after its consumer was released: " +
                    ResultError(result));
    }
    moduleLoaded = false;
    return true;
}

int Fail(const char *message, CKERROR error = CK_OK) {
    std::cerr << message;
    if (error != CK_OK) {
        std::cerr << " (CKERROR " << error << ")";
    }
    std::cerr << std::endl;
    return 1;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2 || !argv[1] || argv[1][0] == '\0') {
        return Fail("Expected the CKAngelScript plugin path.");
    }

    CKERROR error = CKStartUp();
    if (error != CK_OK) {
        return Fail("CKStartUp failed.", error);
    }

    CKPluginManager *pluginManager = CKGetPluginManager();
    if (!pluginManager) {
        CKShutdown();
        return Fail("CKGetPluginManager returned null.");
    }
    error = pluginManager->RegisterPlugin(argv[1]);
    if (error != CK_OK && error != CKERR_ALREADYPRESENT) {
        CKShutdown();
        return Fail("CKAngelScript plugin registration failed.", error);
    }

    CKContext *context = nullptr;
    error = CKCreateContext(&context, nullptr, 0, 0);
    if (error != CK_OK || !context) {
        CKShutdown();
        return Fail("CKCreateContext failed.", error);
    }

    CKBaseManager *manager = context->GetManagerByGuid(kScriptManagerGuid);
    if (!manager) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("CKAngelScript manager was not initialized by CK2.");
    }
    const CKDWORD requiredFunctions = CKMANAGER_FUNC_PreClearAll |
                                      CKMANAGER_FUNC_OnCKPostReset |
                                      CKMANAGER_FUNC_OnSequenceToBeDeleted;
    if ((manager->GetValidFunctionsMask() & requiredFunctions) != requiredFunctions) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("CKAngelScript manager did not register the required CK2 lifecycle hooks.");
    }

    auto *probe = new LifecycleProbeManager(context);
    if (probe->RegistrationError != CK_OK) {
        const CKERROR registrationError = probe->RegistrationError;
        CKCloseContext(context);
        CKShutdown();
        return Fail("Could not register the CK2 lifecycle probe manager.", registrationError);
    }

    CKObject *deleteProbe = context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_SequenceDeleteProbe"),
        CK_OBJECTCREATION_DYNAMIC);
    if (!deleteProbe) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Could not create the sequence-deletion probe behavior.");
    }
    probe->DeleteProbeId = deleteProbe->GetID();
    probe->DeleteProbeStamp = CaptureBridgeObjectStamp(deleteProbe);
    error = context->DestroyObject(deleteProbe);
    if (error != CK_OK || !probe->SawDeleteProbe || !probe->DeleteProbeWasLive ||
        !probe->DeleteProbeWasPendingDeletion ||
        !probe->DefaultIdentityRejectedPendingDeletion ||
        !probe->TerminalIdentityResolvedPendingDeletion) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Original CK2 pending-deletion identity semantics did not match the terminal-frame contract.", error);
    }

    CKLevel *level = CKLevel::Cast(context->CreateObject(
        CKCID_LEVEL,
        const_cast<CKSTRING>("__CKAS_EngineIntegrationLevel"),
        CK_OBJECTCREATION_NONAMECHECK));
    if (!level) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Could not create the reset probe level.");
    }
    context->SetCurrentLevel(level);
    probe->ResetProbeId = level->GetID();
    if (context->GetCurrentLevel() != level) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Original CK2 did not retain the newly assigned current level.");
    }
    std::string resetProbeError;
    if (!RunSuspendedWaitResetProbe(context, argv[1], error, resetProbeError)) {
        CKCloseContext(context);
        CKShutdown();
        return Fail(resetProbeError.c_str(), error);
    }
    if (!probe->ResetProbeWasLive || !probe->ResetCurrentLevelMatched) {
        const char *resetFailure = probe->ResetProbeWasLive
                                       ? "CK2 reset probe did not see the assigned current level in OnCKReset."
                                       : "CK2 reset probe level was already invalid in OnCKReset.";
        CKCloseContext(context);
        CKShutdown();
        return Fail(resetFailure);
    }
    if (!SawOrderedPair(probe->Events, ProbeEvent::Reset, ProbeEvent::PostReset)) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Original CK2 did not deliver OnCKReset before OnCKPostReset.");
    }

    std::string mutationProbeError;
    if (!RunComponentModuleMutationProbe(context, argv[1], mutationProbeError)) {
        CKCloseContext(context);
        CKShutdown();
        return Fail(mutationProbeError.c_str());
    }

    CKObject *clearProbe = context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ClearAllProbe"),
        CK_OBJECTCREATION_DYNAMIC);
    if (!clearProbe) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Could not create the clear-all probe behavior.");
    }
    probe->ClearProbeId = clearProbe->GetID();

    error = context->ClearAll();
    if (error != CK_OK) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("CK2 clear-all lifecycle failed.", error);
    }
    if (!probe->ClearProbeWasLive || !probe->ClearProbeWasGone ||
        !SawOrderedPair(probe->Events, ProbeEvent::PreClearAll, ProbeEvent::PostClearAll)) {
        CKCloseContext(context);
        CKShutdown();
        return Fail("Original CK2 did not bracket object clearing with live PreClearAll and final PostClearAll callbacks.");
    }

    error = CKCloseContext(context);
    const CKERROR shutdownError = CKShutdown();
    if (error != CK_OK) {
        return Fail("CKCloseContext failed.", error);
    }
    if (shutdownError != CK_OK) {
        return Fail("CKShutdown failed.", shutdownError);
    }
    return 0;
}
