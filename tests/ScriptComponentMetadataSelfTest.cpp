#include "ScriptSelfTests.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "CKAll.h"
#include "ScriptAsync.h"
#include "ScriptBridgeCommon.h"
#include "ScriptComponentMetadata.h"
#include "ScriptComponentState.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"
#include "ScriptRuntime.h"

namespace {

constexpr int COMPONENT_PARAM_SCRIPT = 0;
constexpr int COMPONENT_PARAM_CLASS = 1;
constexpr int COMPONENT_PARAM_SOURCE = 2;
constexpr int COMPONENT_PARAM_FILE = 3;
constexpr int COMPONENT_PARAM_MANIFEST = 4;

CKBehaviorContext MakeComponentSelfTestContext(CKContext *context, CKBehavior *behavior) {
    CKBehaviorContext behaviorContext = {};
    behaviorContext.Behavior = behavior;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.ParameterManager = context ? context->GetParameterManager() : nullptr;
    behaviorContext.MessageManager = context ? context->GetMessageManager() : nullptr;
    behaviorContext.AttributeManager = context ? context->GetAttributeManager() : nullptr;
    behaviorContext.TimeManager = context ? context->GetTimeManager() : nullptr;
    return behaviorContext;
}

void DestroyComponentSelfTestObject(CKContext *context, CKObject *object) {
    if (context && object && !object->IsToBeDeleted()) {
        context->DestroyObject(object);
    }
}

class ScopedEnvVar {
public:
    ScopedEnvVar(const char *name, const std::string &value) : m_Name(name ? name : "") {
        if (m_Name.empty()) {
            return;
        }
        char *oldValue = nullptr;
        size_t oldSize = 0;
        if (_dupenv_s(&oldValue, &oldSize, m_Name.c_str()) == 0 && oldValue) {
            m_HadValue = true;
            m_OldValue = oldValue;
            std::free(oldValue);
        }
        _putenv_s(m_Name.c_str(), value.c_str());
    }

    ~ScopedEnvVar() {
        if (!m_Name.empty()) {
            _putenv_s(m_Name.c_str(), m_HadValue ? m_OldValue.c_str() : "");
        }
    }

    ScopedEnvVar(const ScopedEnvVar &) = delete;
    ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

private:
    std::string m_Name;
    std::string m_OldValue;
    bool m_HadValue = false;
};

bool WriteComponentSelfTestFile(const std::filesystem::path &path,
                                const std::string &text,
                                std::string &error) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        error = "Component runtime self-test failed to create script entry file: " + path.string();
        return false;
    }
    output << text;
    if (!output) {
        error = "Component runtime self-test failed to write script entry file: " + path.string();
        return false;
    }
    return true;
}

bool SetComponentStringInput(CKBehavior *behavior, int index, const char *value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        error = "Component runtime self-test could not resolve string input parameter.";
        return false;
    }

    CKParameterIn *input = behavior->GetInputParameter(index);
    const std::string localName = "__CKAS_ComponentFrameSelfTest_" + std::to_string(index);
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(localName.c_str()), CKPGUID_STRING);
    if (!input || !local) {
        error = "Component runtime self-test failed to create input source.";
        return false;
    }

    if (local->SetStringValue(const_cast<CKSTRING>(value ? value : "")) != CK_OK) {
        error = "Component runtime self-test failed to write input source.";
        return false;
    }
    behavior->SetInputParameterDefaultValue(input, local);
    if (input->SetDirectSource(local) != CK_OK) {
        error = "Component runtime self-test failed to write input source.";
        return false;
    }
    return true;
}

int ExecuteComponentSelfTestBehavior(CKContext *context, CKBehavior *behavior, const CKBehaviorContext &behaviorContext) {
    if (!behavior) {
        return CKBR_BEHAVIORERROR;
    }

#if CKVERSION == 0x13022002
    CKBehaviorContext savedContext = context->m_BehaviorContext;
    CKBehaviorManager *behaviorManager = context ? context->GetBehaviorManager() : nullptr;
    CKBehavior *savedCurrent = behaviorManager ? behaviorManager->m_CurrentBehavior : nullptr;

    context->m_BehaviorContext = behaviorContext;
    const int rc = behavior->Execute(behaviorContext.DeltaTime);

    context->m_BehaviorContext = savedContext;
    if (behaviorManager) {
        behaviorManager->m_CurrentBehavior = savedCurrent;
    }
    return rc;
#else
    return behavior->Execute(behaviorContext.DeltaTime);
#endif
}

bool ReadIntProperty(asIScriptObject *object, const char *name, int &value, std::string &error) {
    if (!object || !name || name[0] == '\0') {
        error = "Component runtime self-test script object is not available.";
        return false;
    }

    asITypeInfo *type = object->GetObjectType();
    if (!type) {
        error = "Component runtime self-test script object has no type.";
        return false;
    }

    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int typeId = 0;
        if (type->GetProperty(i, &propertyName, &typeId) < 0 || !propertyName) {
            continue;
        }
        if (std::strcmp(propertyName, name) != 0) {
            continue;
        }
        if (typeId != asTYPEID_INT32) {
            error = std::string("Component runtime self-test property has unexpected type: ") + name;
            return false;
        }
        void *address = object->GetAddressOfProperty(i);
        if (!address) {
            error = std::string("Component runtime self-test property address is null: ") + name;
            return false;
        }
        value = *static_cast<int *>(address);
        return true;
    }

    error = std::string("Component runtime self-test property not found: ") + name;
    return false;
}

bool ExpectIntProperty(asIScriptObject *object,
                       const char *name,
                       int expected,
                       std::string &error) {
    int actual = 0;
    if (!ReadIntProperty(object, name, actual, error)) {
        return false;
    }
    if (actual != expected) {
        error = std::string("Component runtime self-test property '") + name +
                "' expected " + std::to_string(expected) +
                " but got " + std::to_string(actual) + ".";
        return false;
    }
    return true;
}

bool RunComponentBehaviorFrameRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Component runtime self-test requires CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Component runtime self-test could not retrieve ScriptManager.";
        return false;
    }

    const char *source =
        "class ComponentFrameRuntimeProbe {\n"
        "  int startCount = 0;\n"
        "  int updateCount = 0;\n"
        "  int messageCount = 0;\n"
        "  int enableCount = 0;\n"
        "  int disableCount = 0;\n"
        "  int resetCount = 0;\n"
        "  int resetSawCancelled = 0;\n"
        "  int resetCompleted = 0;\n"
        "  int destroyCount = 0;\n"
        "  int frameError = 0;\n"
        "  AsyncTask<void>@ pendingMessage;\n"
        "  void Validate(BehaviorFrame@ frame, int code) {\n"
        "    if (frame is null) { frameError = code; return; }\n"
        "    if (frame.Context() is null) { frameError = code + 2; return; }\n"
        "    if (!frame.valid || frame.id == 0 || frame.Self() is null) { frameError = code + 4; return; }\n"
        "    if (frame.Context().GetParameterManager() is null || frame.Context().GetMessageManager() is null || frame.Context().GetAttributeManager() is null || frame.Context().GetTimeManager() is null) { frameError = code + 6; return; }\n"
        "    frame.InputCount(); frame.OutputCount(); frame.PinCount(); frame.PoutCount(); frame.LocalCount(); frame.SettingCount(); frame.Type(); frame.Flags(); frame.PrototypeGuid(); frame.PrototypeName();\n"
        "    if (frame.LocalCount() != frame.BehaviorLocalCount()) { frameError = code + 7; return; }\n"
        "    int componentLocal = frame.FindLocal(\"__CKAS_ComponentFrameSelfTest_0\");\n"
        "    if (componentLocal < 0 || frame.FindBehaviorLocal(\"__CKAS_ComponentFrameSelfTest_0\") != componentLocal) { frameError = code + 8; return; }\n"
        "    if (frame.Local(componentLocal) is null || frame.BehaviorLocal(componentLocal) is null) { frameError = code + 9; return; }\n"
        "    if (frame.LocalString(\"__CKAS_ComponentFrameSelfTest_0\", \"\") != \"__ckas_component_frame_runtime_selftest\") { frameError = code + 10; return; }\n"
        "    if (frame.PinInt(\"Missing\", code + 3) != code + 3) { frameError = code + 3; return; }\n"
        "  }\n"
        "  void Start(BehaviorFrame@ frame) { Validate(frame, 100); startCount++; }\n"
        "  void OnEnable(BehaviorFrame@ frame) {\n"
        "    Validate(frame, 150); enableCount++;\n"
        "    if (enableCount == 2 || enableCount == 3) { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); Validate(frame, 155); }\n"
        "  }\n"
        "  void OnDisable(BehaviorFrame@ frame) { Validate(frame, 160); disableCount++; AsyncTask<void>@ delay = Async::Delay(1); Await(delay); Validate(frame, 170); }\n"
        "  void Update(BehaviorFrame@ frame) { Validate(frame, 200); updateCount++; }\n"
        "  void OnMessage(const ScriptMessage &in msg, BehaviorFrame@ frame) {\n"
        "    Validate(frame, 300);\n"
        "    if (frameError != 0) return;\n"
        "    if (msg.Topic() != \"component.frame.runtime\") { frameError = 399; return; }\n"
        "    @pendingMessage = Async::Delay(1);\n"
        "    Await(pendingMessage);\n"
        "    Validate(frame, 310);\n"
        "    if (frameError != 0) return;\n"
        "    if (msg.Topic() == \"component.frame.runtime\") messageCount++; else frameError = 398;\n"
        "  }\n"
        "  void OnReset(BehaviorFrame@ frame) {\n"
        "    Validate(frame, 350);\n"
        "    resetCount++;\n"
        "    resetSawCancelled = pendingMessage !is null && pendingMessage.IsCancelled() ? 1 : -1;\n"
        "    AsyncTask<void>@ delay = Async::Delay(1);\n"
        "    Await(delay);\n"
        "    Validate(frame, 360);\n"
        "    resetCompleted++;\n"
        "  }\n"
        "  void OnDestroy(BehaviorFrame@ frame) { Validate(frame, 400); destroyCount++; }\n"
        "}\n";

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ComponentFrameSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    bool deleteCallbackSent = false;
    CKBehaviorContext behaviorContext = {};
    auto cleanup = [&]() {
        if (behavior && deleteCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &behaviorContext);
        }
        DestroyComponentSelfTestObject(context, behavior);
    };

    if (!behavior) {
        cleanup();
        error = "Component runtime self-test failed to create behavior.";
        return false;
    }

    behavior->UseFunction();
    const CKERROR initError = behavior->InitFromGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    if (initError != CK_OK) {
        cleanup();
        error = "Component runtime self-test failed to initialize AngelScript Component behavior.";
        return false;
    }

    if (!SetComponentStringInput(behavior, COMPONENT_PARAM_SCRIPT, "__ckas_component_frame_runtime_selftest", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_CLASS, "ComponentFrameRuntimeProbe", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_SOURCE, source, error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_FILE, "", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_MANIFEST, "", error)) {
        cleanup();
        return false;
    }

    behaviorContext = MakeComponentSelfTestContext(context, behavior);
    const CKERROR createError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &behaviorContext);
    if (createError != CK_OK) {
        cleanup();
        error = "Component runtime self-test CREATE callback failed.";
        return false;
    }
    deleteCallbackSent = true;

    behavior->ActivateInput(0, TRUE);
    const int returnCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (returnCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Component runtime self-test returned unexpected code " + std::to_string(returnCode) + ".";
        return false;
    }

    ScriptComponentState *state = manager->GetComponentState(behavior->GetID());
    if (!state || !state->Loaded || !state->InstanceEnabled || !state->Object ||
        state->ModuleKind != ScriptComponentModuleKind::Private || state->ModuleName.empty()) {
        cleanup();
        error = "Component runtime self-test did not create an enabled component instance.";
        return false;
    }
    const std::string privateModuleName = state->ModuleName;
    ScriptMessage message(1,
                          ScriptMessageKind::Direct,
                          "component.frame.runtime",
                          "component-runtime-self-test",
                          ScriptMessageTarget(),
                          0,
                          false,
                          nullptr);
    std::string messageError;
    if (!manager->DeliverComponentMessage(behavior->GetID(),
                                          state->MessageGeneration,
                                          message,
                                          true,
                                          messageError)) {
        cleanup();
        error = messageError.empty()
            ? "Component runtime self-test message delivery failed."
            : "Component runtime self-test message delivery failed: " + messageError;
        return false;
    }
    if (!state->Invoker->IsContextSuspended() || state->ActiveLifecycleName != "OnMessage") {
        cleanup();
        error = "Component runtime self-test expected OnMessage to suspend explicitly.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Component runtime self-test could not retrieve async scheduler.";
        return false;
    }
    scheduler->Tick();

    const int resumeCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (resumeCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Component runtime self-test returned unexpected message resume code " + std::to_string(resumeCode) + ".";
        return false;
    }
    if (state->Invoker->IsContextSuspended() || state->ActiveLifecycleName == "OnMessage") {
        cleanup();
        error = "Component runtime self-test did not finish suspended OnMessage.";
        return false;
    }

    if (CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORPAUSE, &behaviorContext) != CK_OK ||
        !state->InitialLifecycle.PausePending() ||
        !state->Invoker->IsContextSuspended() || state->ActiveLifecycleName != "OnDisable" ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORRESUME, &behaviorContext) != CK_OK ||
        !state->InitialLifecycle.PausePending()) {
        cleanup();
        error = "Component runtime self-test could not suspend disable across Pause and Resume.";
        return false;
    }
    scheduler->Tick();
    const int lifecycleResumeCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (lifecycleResumeCode != CKBR_ACTIVATENEXTFRAME ||
        !state->Invoker->IsContextSuspended() ||
        state->ActiveLifecycleName != "OnEnable" ||
        state->InstanceEnabled || state->InitialLifecycle.Paused() ||
        state->InitialLifecycle.PausePending() ||
        !state->InitialLifecycle.ResumePending()) {
        cleanup();
        error = "Component runtime self-test did not retain Resume through suspended OnEnable.";
        return false;
    }
    scheduler->Tick();
    const int enableResumeCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (enableResumeCode != CKBR_ACTIVATENEXTFRAME ||
        state->Invoker->IsContextSuspended() ||
        !state->InstanceEnabled || state->InitialLifecycle.Paused() ||
        state->InitialLifecycle.ResumePending()) {
        cleanup();
        error = "Component runtime self-test did not complete deferred Resume after OnEnable.";
        return false;
    }

    if (CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORPAUSE, &behaviorContext) != CK_OK ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORRESUME, &behaviorContext) != CK_OK) {
        cleanup();
        error = "Component explicit disable self-test could not begin Host Activation.";
        return false;
    }
    scheduler->Tick();
    const int secondLifecycleResumeCode =
        ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (secondLifecycleResumeCode != CKBR_ACTIVATENEXTFRAME ||
        !state->Invoker->IsContextSuspended() ||
        state->ActiveLifecycleName != "OnEnable" ||
        !state->InitialLifecycle.DesiredEnabled() ||
        !state->InitialLifecycle.ResumePending()) {
        cleanup();
        error = "Component explicit disable self-test did not reach suspended OnEnable.";
        return false;
    }
    behavior->ActivateInput(1, TRUE);
    const int disableCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (disableCode != CKBR_OK || state->Invoker->IsContextSuspended() ||
        !state->ActiveLifecycleName.empty() || state->InstanceEnabled ||
        state->InitialLifecycle.DesiredEnabled() ||
        state->InitialLifecycle.ResumePending()) {
        cleanup();
        error = "Component explicit Disable did not supersede suspended Host Activation.";
        return false;
    }
    behavior->ActivateInput(0, TRUE);
    const int reenableCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (reenableCode != CKBR_ACTIVATENEXTFRAME || !state->InstanceEnabled ||
        !state->InitialLifecycle.DesiredEnabled() ||
        state->InitialLifecycle.ResumePending()) {
        cleanup();
        error = "Component explicit disable self-test could not reactivate the instance.";
        return false;
    }

    messageError.clear();
    if (!manager->DeliverComponentMessage(behavior->GetID(),
                                          state->MessageGeneration,
                                          message,
                                          true,
                                          messageError) ||
        !state->Invoker->IsContextSuspended() ||
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORRESET, &behaviorContext) != CK_OK ||
        !state->Invoker->IsContextSuspended() ||
        state->ActiveLifecycleName != "OnReset") {
        cleanup();
        error = messageError.empty()
            ? "Component runtime self-test could not replace old suspended work with OnReset."
            : messageError;
        return false;
    }
    if (CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORRESET, &behaviorContext) != CK_OK ||
        !state->Invoker->IsContextSuspended() ||
        state->ActiveLifecycleName != "OnReset" ||
        !state->InitialLifecycle.ResetPending()) {
        cleanup();
        error = "Component runtime self-test did not supersede suspended OnReset with a new epoch.";
        return false;
    }
    scheduler->Tick();
    const int resetResumeCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (resetResumeCode != CKBR_ACTIVATENEXTFRAME ||
        state->Invoker->IsContextSuspended() ||
        state->ActiveLifecycleName == "OnReset" ||
        state->InitialLifecycle.ResetPending()) {
        cleanup();
        error = "Component runtime self-test did not complete new-epoch OnReset work.";
        return false;
    }

    asIScriptObject *object = state->Object;
    if (!ExpectIntProperty(object, "startCount", 1, error) ||
        !ExpectIntProperty(object, "updateCount", 2, error) ||
        !ExpectIntProperty(object, "messageCount", 1, error) ||
        !ExpectIntProperty(object, "enableCount", 4, error) ||
        !ExpectIntProperty(object, "disableCount", 2, error) ||
        !ExpectIntProperty(object, "resetCount", 2, error) ||
        !ExpectIntProperty(object, "resetSawCancelled", 1, error) ||
        !ExpectIntProperty(object, "resetCompleted", 1, error) ||
        !ExpectIntProperty(object, "frameError", 0, error)) {
        cleanup();
        return false;
    }

    object->AddRef();
    const CK_ID behaviorId = behavior->GetID();
    const CKERROR deleteError = context->DestroyObject(behavior);
    behavior = nullptr;
    deleteCallbackSent = false;
    if (deleteError != CK_OK || manager->GetComponentState(behaviorId) != nullptr ||
        manager->HasModule(privateModuleName.c_str()) ||
        !ExpectIntProperty(object, "destroyCount", 1, error) ||
        !ExpectIntProperty(object, "frameError", 0, error)) {
        object->Release();
        cleanup();
        if (error.empty()) {
            error = "Component runtime self-test did not run OnDestroy during CK object deletion.";
        }
        return false;
    }
    object->Release();

    cleanup();
    error.clear();
    return true;
}

bool RunComponentScriptEntryModuleSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Component script-entry self-test requires CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptRuntime *runtime = manager ? manager->GetRuntime() : nullptr;
    if (!manager || !runtime) {
        error = "Component script-entry self-test could not retrieve ScriptRuntime.";
        return false;
    }

    namespace fs = std::filesystem;
    constexpr const char *scriptId = "ckas.component.entry.selftest";
    const fs::path root = fs::temp_directory_path() / "__ckas_component_script_entry_selftest";
    const fs::path packageDir = root / "Scripts" / "component_entry";
    std::error_code fsError;
    fs::remove_all(root, fsError);
    fsError.clear();
    if (!fs::create_directories(packageDir, fsError)) {
        error = "Component script-entry self-test failed to create script entry directory: " + packageDir.string();
        return false;
    }

    const std::string manifest =
        "[runtime id=\"ckas.component.entry.selftest\" name=\"Component Entry Self Test\" version=\"1.0.0\" entry=\"component.as\"]\n";
    const std::string source =
        "class ComponentEntryProbe {\n"
        "  int startCount = 0;\n"
        "  int updateCount = 0;\n"
        "  int version = 1;\n"
        "  int frameError = 0;\n"
        "  void Validate(BehaviorFrame@ frame, int code) {\n"
        "    if (frame is null || !frame.valid || frame.Context() is null) { frameError = code; }\n"
        "  }\n"
        "  void Start(BehaviorFrame@ frame) { Validate(frame, 100); startCount++; }\n"
        "  void Update(BehaviorFrame@ frame) { Validate(frame, 200); updateCount++; }\n"
        "}\n";
    const std::string reloadedSource =
        "class ComponentEntryProbe {\n"
        "  int startCount = 0;\n"
        "  int updateCount = 0;\n"
        "  int version = 2;\n"
        "  int frameError = 0;\n"
        "  void Validate(BehaviorFrame@ frame, int code) {\n"
        "    if (frame is null || !frame.valid || frame.Context() is null) { frameError = code; }\n"
        "  }\n"
        "  void Start(BehaviorFrame@ frame) { Validate(frame, 100); startCount++; }\n"
        "  void Update(BehaviorFrame@ frame) { Validate(frame, 200); updateCount++; }\n"
        "}\n";

    if (!WriteComponentSelfTestFile(packageDir / "script.as", manifest, error) ||
        !WriteComponentSelfTestFile(packageDir / "component.as", source, error)) {
        fs::remove_all(root, fsError);
        return false;
    }

    const ScopedEnvVar scriptRoots("CKAS_SCRIPT_ROOTS", root.string());
    std::string reloadError;
    if (!runtime->ReloadAll(&reloadError)) {
        runtime->Clear();
        fs::remove_all(root, fsError);
        error = reloadError.empty()
                    ? "Component script-entry self-test script entry failed to load."
                    : reloadError;
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ComponentScriptEntrySelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    bool deleteCallbackSent = false;
    CKBehaviorContext behaviorContext = {};
    auto cleanup = [&]() {
        if (behavior && deleteCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &behaviorContext);
        }
        DestroyComponentSelfTestObject(context, behavior);
        runtime->Clear();
        fs::remove_all(root, fsError);
    };

    if (!behavior) {
        cleanup();
        error = "Component script-entry self-test failed to create behavior.";
        return false;
    }

    behavior->UseFunction();
    const CKERROR initError = behavior->InitFromGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    if (initError != CK_OK) {
        cleanup();
        error = "Component script-entry self-test failed to initialize AngelScript Component behavior.";
        return false;
    }

    if (!SetComponentStringInput(behavior, COMPONENT_PARAM_SCRIPT, scriptId, error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_CLASS, "ComponentEntryProbe", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_SOURCE, "", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_FILE, "", error) ||
        !SetComponentStringInput(behavior, COMPONENT_PARAM_MANIFEST, "", error)) {
        cleanup();
        return false;
    }

    behaviorContext = MakeComponentSelfTestContext(context, behavior);
    const CKERROR createError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &behaviorContext);
    if (createError != CK_OK) {
        cleanup();
        error = "Component script-entry self-test CREATE callback failed.";
        return false;
    }
    deleteCallbackSent = true;

    behavior->ActivateInput(0, TRUE);
    const int returnCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (returnCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Component script-entry self-test returned unexpected code " + std::to_string(returnCode) + ".";
        return false;
    }

    ScriptComponentState *state = manager->GetComponentState(behavior->GetID());
    if (!state || !state->Loaded || !state->Object ||
        state->ModuleKind != ScriptComponentModuleKind::ScriptEntry) {
        cleanup();
        error = "Component script-entry self-test did not create a shared component instance.";
        return false;
    }
    if (state->ScriptEntryId != scriptId ||
        state->ModuleName == scriptId ||
        state->ModuleName.find("__CKASRuntime_") == std::string::npos) {
        cleanup();
        error = "Component script-entry self-test did not resolve Script through the script entry manager.";
        return false;
    }
    if (state->AttachedModuleGeneration == 0 ||
        state->AttachedModuleGeneration != manager->GetModuleGeneration(state->ModuleName.c_str())) {
        cleanup();
        error = "Component script-entry self-test did not record its attached module generation.";
        return false;
    }
    if (!ExpectIntProperty(state->Object, "startCount", 1, error) ||
        !ExpectIntProperty(state->Object, "updateCount", 1, error) ||
        !ExpectIntProperty(state->Object, "version", 1, error) ||
        !ExpectIntProperty(state->Object, "frameError", 0, error)) {
        cleanup();
        return false;
    }

    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    CKAngelScriptResult mutationResult = {};
    const std::string liveModuleName = state->ModuleName;
    if (!api.IsValid() ||
        api->CompileModule(liveModuleName.c_str(),
                           "int replacementProbe = 1;\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &mutationResult) != CKAS_INUSE ||
        api->UnloadModule(liveModuleName.c_str(), &mutationResult) != CKAS_INUSE) {
        cleanup();
        error = "Component script-entry self-test expected live components to block public module mutation.";
        return false;
    }

    if (!WriteComponentSelfTestFile(packageDir / "component.as", reloadedSource, error)) {
        cleanup();
        return false;
    }
    reloadError.clear();
    if (!runtime->Reload(scriptId, &reloadError)) {
        cleanup();
        error = reloadError.empty()
                    ? "Component script-entry self-test failed to reload its Runtime Entry."
                    : reloadError;
        return false;
    }
    if (manager->HasModule(liveModuleName.c_str()) ||
        manager->GetComponentState(behavior->GetID(), behavior) != nullptr) {
        cleanup();
        error = "Component script-entry self-test did not retire the previous Runtime Entry generation.";
        return false;
    }

    behavior->ActivateInput(0, TRUE);
    const int reloadedReturnCode = ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    state = manager->GetComponentState(behavior->GetID(), behavior);
    if (reloadedReturnCode != CKBR_ACTIVATENEXTFRAME ||
        !state || !state->Loaded || !state->Object || state->ModuleName == liveModuleName ||
        !ExpectIntProperty(state->Object, "startCount", 1, error) ||
        !ExpectIntProperty(state->Object, "updateCount", 1, error) ||
        !ExpectIntProperty(state->Object, "version", 2, error) ||
        !ExpectIntProperty(state->Object, "frameError", 0, error)) {
        cleanup();
        if (error.empty()) {
            error = "Component script-entry self-test did not rebuild against the reloaded Runtime Entry.";
        }
        return false;
    }

    const CKDWORD attachedModuleGeneration = state->AttachedModuleGeneration;
    state->AttachedModuleGeneration = attachedModuleGeneration + 1;
    ScriptMessage staleModuleMessage;
    std::string staleModuleError;
    if (manager->DeliverComponentMessage(behavior->GetID(),
                                         state->MessageGeneration,
                                         staleModuleMessage,
                                         false,
                                         staleModuleError) ||
        staleModuleError.find("module generation is stale") == std::string::npos ||
        !state->Failed) {
        cleanup();
        error = "Component script-entry self-test expected direct messages to reject stale module generations.";
        return false;
    }

    state->AttachedModuleGeneration = attachedModuleGeneration;
    state->Failed = false;
    state->AttachedModuleGeneration = attachedModuleGeneration + 1;
    ExecuteComponentSelfTestBehavior(context, behavior, behaviorContext);
    if (!state->Failed || !ExpectIntProperty(state->Object, "updateCount", 1, error)) {
        cleanup();
        if (error.empty()) {
            error = "Component script-entry self-test executed a stale module lifecycle method.";
        }
        return false;
    }

    const std::uint64_t staleMessageGeneration = state->MessageGeneration;
    manager->ResetComponentStateRuntime(state, false);
    ScriptMessage staleMessage;
    std::string staleMessageError;
    if (state->MessageGeneration == staleMessageGeneration ||
        manager->DeliverComponentMessage(behavior->GetID(),
                                         staleMessageGeneration,
                                         staleMessage,
                                         false,
                                         staleMessageError) ||
        staleMessageError != "Component message target is stale.") {
        cleanup();
        error = "Component script-entry self-test expected runtime rebuild to invalidate queued message targets.";
        return false;
    }

    cleanup();
    error.clear();
    return true;
}

} // namespace

bool RunScriptComponentMetadataSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Component metadata self-test requires CKContext and AngelScript engine.";
        return false;
    }

    const int constEntityHandleType = engine->GetTypeIdByDecl("const CK3dEntity@");
    if (constEntityHandleType < 0) {
        error = "Component metadata self-test could not resolve const CK3dEntity@.";
        return false;
    }
    if (ScriptComponentSupport::InferKindFromProperty(engine, constEntityHandleType) != ScriptComponentBindingKind::Object) {
        error = "Component metadata self-test did not infer const CK object handles as Object bindings.";
        return false;
    }
    std::string expected;
    if (!ScriptComponentSupport::IsCompatiblePropertyType(engine, constEntityHandleType, ScriptComponentBindingKind::Object, expected)) {
        error = "Component metadata self-test did not accept const CK object handles as Object fields.";
        return false;
    }
    if (ScriptComponentSupport::ClassIdFromPropertyType(engine, constEntityHandleType) != CKCID_3DENTITY) {
        error = "Component metadata self-test lost const CK object handle class identity.";
        return false;
    }
    if (ScriptComponentSupport::GuidFromPropertyType(context, engine, constEntityHandleType) != CKPGUID_3DENTITY) {
        error = "Component metadata self-test lost const CK object handle parameter GUID.";
        return false;
    }

    std::vector<ScriptComponentBinding> bindings;
    auto addMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding binding;
        if (!ScriptComponentSupport::ParseBindingMetadata(metadata, "TextConfig", binding)) {
            error = "Component metadata self-test failed to parse '" + metadata + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(bindings, binding);
        return true;
    };

    if (!addMetadata("bbconfig prototype=\"Interface/Text/2D Text\" lifetime=\"component\"") ||
        !addMetadata("bbsetting \"Text Properties\"=\"Screen Proportionnal,WordWrap\"") ||
        !addMetadata("bbpin \"Text\"=\"FPS: ...\"") ||
        !addMetadata("bbsource \"Font\"=\"FontConfig.Font Created\"") ||
        !addMetadata("bboutput \"Out\"") ||
        !addMetadata("bbpout \"Rendered\"")) {
        return false;
    }

    if (bindings.size() != 1) {
        error = "Component metadata self-test did not merge stacked metadata.";
        return false;
    }
    const ScriptComponentBinding &binding = bindings.front();
    if (binding.Kind != ScriptComponentBindingKind::BBConfig ||
        binding.FieldName != "TextConfig" ||
        binding.SlotPrototypeName != "Interface/Text/2D Text" ||
        binding.BBConfigLifetime != ScriptComponentBBConfigLifetime::Component ||
        binding.ConfigPinValues.size() != 1 ||
        binding.ConfigPinValues[0].Name != "Text" ||
        binding.ConfigPinValues[0].Value != "FPS: ..." ||
        binding.ConfigSettingValues.size() != 1 ||
        binding.ConfigSettingValues[0].Name != "Text Properties" ||
        binding.ConfigSettingValues[0].Value != "Screen Proportionnal,WordWrap" ||
        binding.ConfigSources.size() != 1 ||
        binding.ConfigSources[0].PinName != "Font" ||
        binding.ConfigSources[0].SourceFieldName != "FontConfig" ||
        binding.ConfigSources[0].SourceSlotName != "Font Created") {
        error = "Component metadata self-test merged BBConfig fields incorrectly.";
        return false;
    }

    bool sawOutput = false;
    bool sawPout = false;
    bool sawFontPin = false;
    bool sawSetting = false;
    for (const ScriptComponentRequiredSlot &slot : binding.RequiredSlots) {
        sawOutput = sawOutput || (slot.KindName == "output" && slot.Name == "Out");
        sawPout = sawPout || (slot.KindName == "pout" && slot.Name == "Rendered");
        sawFontPin = sawFontPin || (slot.KindName == "pin" && slot.Name == "Font");
        sawSetting = sawSetting || (slot.KindName == "setting" && slot.Name == "Text Properties");
    }
    if (!sawOutput || !sawPout || !sawFontPin || !sawSetting) {
        error = "Component metadata self-test missed required slot fragments.";
        return false;
    }

    std::vector<ScriptComponentBinding> repeatedSlotBindings;
    for (const std::string &fieldName : {std::string("FirstConfig"), std::string("SecondConfig")}) {
        ScriptComponentBinding configBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata(
                "bbconfig prototype=\"Logics/Calculator/Identity\"",
                fieldName,
                configBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot config metadata.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(repeatedSlotBindings, configBinding);

        ScriptComponentBinding slotBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata("bbpout \"pOut 0\"", fieldName, slotBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot fragment metadata.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(repeatedSlotBindings, slotBinding);
    }
    if (repeatedSlotBindings.size() != 2 ||
        repeatedSlotBindings[0].ParameterName != "FirstConfig" ||
        repeatedSlotBindings[1].ParameterName != "SecondConfig") {
        error = "Component metadata self-test let BBConfig fragment slot names override field parameter names.";
        return false;
    }

    ScriptComponentBinding legacyManaged;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbconfig prototype=\"Interface/Text/2D Text\" managed=true",
            "LegacyConfig",
            legacyManaged)) {
        error = "Component metadata self-test failed to parse legacy managed= diagnostic metadata.";
        return false;
    }
    if (legacyManaged.MetadataError.find("lifetime=\"component\"") == std::string::npos ||
        legacyManaged.MetadataError.find("lifetime=\"manual\"") == std::string::npos) {
        error = "Component metadata self-test did not reject managed= with a lifetime replacement diagnostic.";
        return false;
    }

    std::vector<ScriptComponentBinding> aggregateBindings;
    auto addAggregateMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding aggregateBinding;
        if (!ScriptComponentSupport::ParseBindingMetadata(metadata, "AggregateConfig", aggregateBinding)) {
            error = "Component metadata self-test failed to parse aggregate metadata '" + metadata + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(aggregateBindings, aggregateBinding);
        return true;
    };
    if (!addAggregateMetadata("bbconfig prototype=\"Interface/Text/2D Text\" pins=\"Text='Aggregate'\" settings=\"Text Properties='Screen Proportionnal'\" sources=\"Font<-FontConfig.Font Created\"") ||
        !addAggregateMetadata("bbpin \"Text\"=\"Fragment\"")) {
        return false;
    }
    if (aggregateBindings.size() != 1 ||
        aggregateBindings[0].ConfigPinValues.size() != 2 ||
        aggregateBindings[0].ConfigPinValues.back().Name != "Text" ||
        aggregateBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        aggregateBindings[0].ConfigSettingValues.size() != 1 ||
        aggregateBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not preserve aggregate metadata with fragment overwrite order.";
        return false;
    }

    std::vector<ScriptComponentBinding> manifestBindings;
    for (const std::string &line : {
             std::string("bbconfig field=ManifestConfig prototype=\"Interface/Text/2D Text\" lifetime=manual pins=\"Text='Aggregate'\""),
             std::string("bbpin field=ManifestConfig \"Text\"=\"Fragment\""),
             std::string("bbsetting field=ManifestConfig \"Text Properties\"=\"Screen Proportionnal\""),
             std::string("bbsource field=ManifestConfig \"Font\"=\"FontConfig.Font Created\"")}) {
        ScriptComponentBinding manifestBinding;
        if (!ScriptComponentSupport::ParseManifestLine(line, manifestBinding)) {
            error = "Component metadata self-test failed to parse manifest line '" + line + "'.";
            return false;
        }
        ScriptComponentSupport::MergeOrAppendMetadataBinding(manifestBindings, manifestBinding);
    }
    if (manifestBindings.size() != 1 ||
        manifestBindings[0].BBConfigLifetime != ScriptComponentBBConfigLifetime::Manual ||
        manifestBindings[0].ConfigPinValues.size() != 2 ||
        manifestBindings[0].ConfigPinValues.back().Name != "Text" ||
        manifestBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        manifestBindings[0].ConfigSettingValues.size() != 1 ||
        manifestBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not merge manifest BBConfig fragments correctly.";
        return false;
    }

    ScriptComponentBinding occurrenceBinding;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbconfig prototype=\"Logics/Calculator/Identity\" pins=\"Value[3]='42'\" sources=\"pIn 0[1]<-SourceConfig.pout:Value[2]\" required=\"pout:Out[4]\"",
            "OccurrenceConfig",
            occurrenceBinding)) {
        error = "Component metadata self-test failed to parse occurrence metadata.";
        return false;
    }
    if (occurrenceBinding.ConfigPinValues.size() != 1 ||
        occurrenceBinding.ConfigPinValues[0].Name != "Value" ||
        occurrenceBinding.ConfigPinValues[0].Occurrence != 3 ||
        occurrenceBinding.ConfigSources.size() != 1 ||
        occurrenceBinding.ConfigSources[0].PinName != "pIn 0" ||
        occurrenceBinding.ConfigSources[0].PinOccurrence != 1 ||
        occurrenceBinding.ConfigSources[0].SourceFieldName != "SourceConfig" ||
        occurrenceBinding.ConfigSources[0].SourceSlotName != "pout:Value" ||
        occurrenceBinding.ConfigSources[0].SourceOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBConfig slot occurrences.";
        return false;
    }
    bool sawPoutOccurrence = false;
    for (const ScriptComponentRequiredSlot &slot : occurrenceBinding.RequiredSlots) {
        sawPoutOccurrence = sawPoutOccurrence || (slot.KindName == "pout" && slot.Name == "Out" && slot.Occurrence == 4);
    }
    if (!sawPoutOccurrence) {
        error = "Component metadata self-test did not preserve required slot occurrence.";
        return false;
    }

    ScriptComponentBinding slotOccurrenceBinding;
    if (!ScriptComponentSupport::ParseBindingMetadata(
            "bbslot from=\"OccurrenceConfig\" pout=\"Out[2]\"",
            "OutSlot",
            slotOccurrenceBinding)) {
        error = "Component metadata self-test failed to parse BBSlot occurrence metadata.";
        return false;
    }
    if (slotOccurrenceBinding.Kind != ScriptComponentBindingKind::BBSlot ||
        slotOccurrenceBinding.SlotName != "Out" ||
        slotOccurrenceBinding.SlotOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBSlot field occurrence.";
        return false;
    }
    const std::string occurrenceCacheText = ScriptComponentSupport::BuildBBConfigBindingCacheText(occurrenceBinding, 0, std::string());
    if (occurrenceCacheText.find("pin:Value[3]=") == std::string::npos ||
        occurrenceCacheText.find("source:pIn 0[1]<-SourceConfig.pout:Value[2]") == std::string::npos) {
        error = "Component metadata self-test did not include slot occurrences in BBConfig cache text.";
        return false;
    }

    if (!RunComponentBehaviorFrameRuntimeSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunComponentScriptEntryModuleSelfTest(context, engine, error)) {
        return false;
    }

    return true;
}
