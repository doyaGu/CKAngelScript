#include "ScriptSelfTests.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "CKAll.h"
#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptBridgeCommon.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"

namespace {

constexpr int EXECUTOR_SETTING_SCRIPT = 1;
constexpr int EXECUTOR_SETTING_CLASS = 2;
constexpr int EXECUTOR_SETTING_INTERFACE = 3;
constexpr int EXECUTOR_SETTING_STRICT_INTERFACE = 4;
constexpr int EXECUTOR_SETTING_TRACE = 5;

CKBehaviorContext MakeExecutorSelfTestContext(CKContext *context, CKBehavior *behavior) {
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

void DestroyExecutorSelfTestObject(CKContext *context, CKObject *object) {
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
        if (m_Name.empty()) {
            return;
        }
        _putenv_s(m_Name.c_str(), m_HadValue ? m_OldValue.c_str() : "");
    }

private:
    std::string m_Name;
    std::string m_OldValue;
    bool m_HadValue = false;
};

bool WriteExecutorSelfTestFile(const std::filesystem::path &path, const char *text, std::string &error) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        error = "Executor self-test failed to create runtime script file: " + path.string();
        return false;
    }
    output << (text ? text : "");
    if (!output) {
        error = "Executor self-test failed to write runtime script file: " + path.string();
        return false;
    }
    return true;
}

bool SetExecutorStringSetting(CKBehavior *behavior, int index, const char *value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = "Executor self-test could not resolve string setting.";
        return false;
    }
    CKParameterLocal *setting = behavior->GetLocalParameter(index);
    if (!setting || setting->SetStringValue(const_cast<CKSTRING>(value ? value : "")) != CK_OK) {
        error = "Executor self-test failed to write string setting.";
        return false;
    }
    return true;
}

bool SetExecutorBoolSetting(CKBehavior *behavior, int index, bool value, std::string &error) {
    if (!behavior || index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = "Executor self-test could not resolve bool setting.";
        return false;
    }
    CKBOOL ckValue = value ? TRUE : FALSE;
    if (behavior->SetLocalParameterValue(index, &ckValue) != CK_OK) {
        error = "Executor self-test failed to write bool setting.";
        return false;
    }
    return true;
}

int ExecuteExecutorSelfTestBehavior(CKContext *context, CKBehavior *behavior, const CKBehaviorContext &behaviorContext) {
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

} // namespace

bool RunScriptExecutorSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Executor self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Executor self-test could not retrieve ScriptManager.";
        return false;
    }

    constexpr const char *runtimeId = "ckas.executor.selftest";
    constexpr const char *className = "ExecutorProbe";
    const char *source =
        "class ExecutorProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null) return 101;\n"
        "    if (frame.Behavior() is null) return 102;\n"
        "    if (!frame.Take(\"In\")) return 103;\n"
        "    int value = frame.PinInt(\"Value\", -1);\n"
        "    if (value != 41) return 104;\n"
        "    if (frame.PinInt(\"Missing\", 42) != 42) return 106;\n"
        "    if (frame.PinString(\"MissingText\", \"fallback\") != \"fallback\") return 107;\n"
        "    if (!frame.PinBool(\"MissingBool\", true)) return 108;\n"
        "    if (frame.PinFloat(\"MissingFloat\", 3.5f) != 3.5f) return 109;\n"
        "    if (frame.PinBool(\"Value\", false)) return 110;\n"
        "    if (frame.PinString(\"Value\", \"int-fallback\") != \"int-fallback\") return 114;\n"
        "    AsyncTask<void>@ delay = Async::Delay(1);\n"
        "    Await(delay);\n"
        "    if (frame.Context() is null) return 111;\n"
        "    if (frame.Behavior() is null) return 112;\n"
        "    if (frame.PinInt(\"MissingAfterResume\", 43) != 43) return 113;\n"
        "    if (!frame.SetPout(\"Handled\", true)) return 105;\n"
        "    frame.Emit(\"Out\");\n"
        "    return 0;\n"
        "  }\n"
        "  int Execute(const CKBehaviorContext &in ctx) {\n"
        "    return 1901;\n"
        "  }\n"
        "}\n";

    const std::filesystem::path scriptRoot =
        std::filesystem::temp_directory_path() / "__ckas_executor_runtime_selftest";
    const std::filesystem::path packageDir = scriptRoot / "Scripts" / runtimeId;
    std::error_code fsError;
    std::filesystem::remove_all(scriptRoot, fsError);
    fsError.clear();
    if (!std::filesystem::create_directories(packageDir, fsError)) {
        error = "Executor self-test failed to create runtime script directory: " + packageDir.string();
        return false;
    }
    if (!WriteExecutorSelfTestFile(packageDir / "script.as",
                                   "[script id=\"ckas.executor.selftest\" name=\"Executor Self Test\" version=\"1.0.0\" entry=\"runtime.as\"]\n",
                                   error) ||
        !WriteExecutorSelfTestFile(packageDir / "runtime.as", source, error)) {
        std::filesystem::remove_all(scriptRoot, fsError);
        return false;
    }

    ScopedEnvVar scriptRoots("CKAS_SCRIPT_ROOTS", scriptRoot.string());
    if (!manager->GetRuntime()) {
        std::filesystem::remove_all(scriptRoot, fsError);
        error = "Executor self-test could not retrieve ScriptRuntime.";
        return false;
    }
    std::string reloadError;
    if (!manager->GetRuntime()->ReloadAll(&reloadError)) {
        manager->GetRuntime()->Clear();
        std::filesystem::remove_all(scriptRoot, fsError);
        error = reloadError.empty() ? "Executor self-test runtime package failed to load." : reloadError;
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    CKParameterLocal *sourceParam = nullptr;
    bool deleteCallbackSent = false;
    auto cleanup = [&]() {
        if (behavior && deleteCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE);
        }
        DestroyExecutorSelfTestObject(context, sourceParam);
        DestroyExecutorSelfTestObject(context, behavior);
        if (manager->GetRuntime()) {
            manager->GetRuntime()->Clear();
        }
        std::filesystem::remove_all(scriptRoot, fsError);
    };

    if (!behavior) {
        cleanup();
        error = "Executor self-test failed to create behavior.";
        return false;
    }

    behavior->UseFunction();
    const CKERROR initError = behavior->InitFromGuid(CKGUID(0x15a41762, 0x0c7c4a91));
    if (initError != CK_OK) {
        cleanup();
        error = "Executor self-test failed to initialize AngelScript Executor behavior.";
        return false;
    }

    const char *interfaceText =
        "input 0 name=\"In\"\n"
        "output 0 name=\"Out\"\n"
        "pin 0 name=\"Value\" type=\"Integer\"\n"
        "pout 0 name=\"Handled\" type=\"Boolean\"\n";
    if (!SetExecutorStringSetting(behavior, EXECUTOR_SETTING_SCRIPT, runtimeId, error) ||
        !SetExecutorStringSetting(behavior, EXECUTOR_SETTING_CLASS, className, error) ||
        !SetExecutorStringSetting(behavior, EXECUTOR_SETTING_INTERFACE, interfaceText, error) ||
        !SetExecutorBoolSetting(behavior, EXECUTOR_SETTING_STRICT_INTERFACE, true, error) ||
        !SetExecutorBoolSetting(behavior, EXECUTOR_SETTING_TRACE, false, error)) {
        cleanup();
        return false;
    }

    CKBehaviorContext behaviorContext = MakeExecutorSelfTestContext(context, behavior);
    const CKERROR createError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &behaviorContext);
    if (createError != CK_OK) {
        cleanup();
        error = "Executor self-test CREATE callback failed.";
        return false;
    }
    deleteCallbackSent = true;

    const CKERROR editedError = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &behaviorContext);
    if (editedError != CK_OK) {
        cleanup();
        error = "Executor self-test SETTINGSEDITED callback failed.";
        return false;
    }

    if (behavior->GetInputCount() != 1 ||
        behavior->GetOutputCount() != 1 ||
        behavior->GetInputParameterCount() != 1 ||
        behavior->GetOutputParameterCount() != 1) {
        cleanup();
        error = "Executor self-test dynamic interface shape did not match.";
        return false;
    }

    sourceParam = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestValue"),
        CKPGUID_INT,
        TRUE);
    CKParameterIn *pin = behavior->GetInputParameter(0);
    int inputValue = 41;
    if (!sourceParam ||
        sourceParam->SetValue(&inputValue, sizeof(inputValue)) != CK_OK ||
        !pin ||
        pin->SetDirectSource(sourceParam) != CK_OK) {
        cleanup();
        error = "Executor self-test failed to install input parameter source.";
        return false;
    }

    behavior->ActivateInput(0, TRUE);
    const int suspendedCode = ExecuteExecutorSelfTestBehavior(context, behavior, behaviorContext);
    if (suspendedCode != CKBR_ACTIVATENEXTFRAME) {
        cleanup();
        error = "Executor self-test expected Execute to suspend, got code " + std::to_string(suspendedCode) + ".";
        return false;
    }
    if (behavior->IsInputActive(0)) {
        cleanup();
        error = "Executor self-test did not clear its active input.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Executor self-test could not retrieve async scheduler.";
        return false;
    }
    scheduler->Tick();

    const int returnCode = ExecuteExecutorSelfTestBehavior(context, behavior, behaviorContext);
    if (returnCode != CKBR_OK) {
        cleanup();
        error = "Executor self-test returned unexpected code " + std::to_string(returnCode) + ".";
        return false;
    }
    if (!behavior->IsOutputActive(0)) {
        cleanup();
        error = "Executor self-test did not activate output 0.";
        return false;
    }

    CKBOOL handled = FALSE;
    if (behavior->GetOutputParameterValue(0, &handled) != CK_OK || handled == FALSE) {
        cleanup();
        error = "Executor self-test did not write output parameter 0.";
        return false;
    }

    cleanup();
    error.clear();
    return true;
}
