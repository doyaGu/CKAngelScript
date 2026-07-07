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
        error = "Executor self-test failed to create script entry file: " + path.string();
        return false;
    }
    output << (text ? text : "");
    if (!output) {
        error = "Executor self-test failed to write script entry file: " + path.string();
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

    constexpr const char *scriptId = "ckas.executor.selftest";
    constexpr const char *className = "ExecutorProbe";
    const char *source =
        "class ExecutorProbe {\n"
        "  int Execute(BehaviorFrame@ frame) {\n"
        "    if (frame is null) return 101;\n"
        "    if (frame.Behavior() is null) return 102;\n"
        "    if (!frame.valid || frame.id == 0 || frame.name.length() == 0) return 115;\n"
        "    CKBehaviorContext frameCtx = frame.ToBehaviorContext();\n"
        "    if (frameCtx.Behavior is null || frameCtx.Context is null) return 116;\n"
        "    if (frame.Context() is null || frame.ParameterManager() is null || frame.MessageManager() is null || frame.AttributeManager() is null || frame.TimeManager() is null) return 117;\n"
        "    if (frame.InputCount() != 1 || frame.OutputCount() != 1 || frame.PinCount() != 1 || frame.PoutCount() != 1) return 118;\n"
        "    if (frame.InputName(0) != \"In\" || frame.OutputName(0) != \"Out\") return 119;\n"
        "    if (frame.FindInput(\"In\") != 0 || frame.FindOutput(\"Out\") != 0 || frame.FindPin(\"Value\") != 0 || frame.FindPout(\"Handled\") != 0) return 120;\n"
        "    if (frame.Input(0) is null || frame.Output(0) is null) return 121;\n"
        "    if (frame.Pin(0) is null || frame.Pin(\"Value\") is null || frame.Pout(0) is null || frame.Pout(\"Handled\") is null) return 122;\n"
        "    if (!frame.PinEnabled(0) || !frame.PinEnabled(\"Value\") || !frame.PoutEnabled(0) || !frame.PoutEnabled(\"Handled\")) return 123;\n"
        "    frame.EnablePin(0, false); if (frame.PinEnabled(0)) return 124; frame.EnablePin(\"Value\", true); if (!frame.PinEnabled(0)) return 125;\n"
        "    frame.EnablePout(0, false); if (frame.PoutEnabled(0)) return 126; frame.EnablePout(\"Handled\", true); if (!frame.PoutEnabled(0)) return 127;\n"
        "    BehaviorRef@ self = frame.Self(); if (self is null || !self.valid || self.id != frame.id) return 128;\n"
        "    if (frame.Layout() is null || self.Layout() is null || frame.Graph() is null || frame.AsGraph() is null) return 129;\n"
        "    CKBehavior@ raw = frame.Behavior();\n"
        "    if (raw is null) return 143;\n"
        "    CKObject@ rawObject = frame.CKGetObject(frame.id); if (rawObject is null || rawObject.GetID() != frame.id) return 166;\n"
        "    if (frame.GetParent() !is raw.GetParent() || frame.GetOwnerScript() !is raw.GetOwnerScript()) return 159;\n"
        "    if (frame.GetSubBehaviorCount() > 0 && frame.GetSubBehavior(0) !is raw.GetSubBehavior(0)) return 160;\n"
        "    frame.ClassId(); frame.ObjectFlags(); frame.IsDynamic(); frame.IsToBeDeleted(); frame.IsNotToBeSaved(); frame.IsPrivate(); frame.IsInterfaceObj(); frame.IsUpToDate(); frame.MemoryOccupation(); frame.AppData();\n"
        "    frame.SceneInCount(); frame.SceneIn(0); frame.IsInScene(frame.CurrentScene()); frame.IsActiveInCurrentScene(); frame.IsActiveInScene(frame.CurrentScene()); frame.IsVisible(); frame.CanBeHide(); frame.IsHiddenByParent(); frame.IsHierarchicallyHide(); frame.IsObjectUsed(raw, raw.GetClassID());\n"
        "    if (frame.GetCKContext() is null || frame.GetID() != frame.id || frame.GetName() != frame.name || frame.GetClassID() != frame.ClassId()) return 154;\n"
        "    if (frame.GetInputCount() != frame.InputCount() || frame.GetOutputCount() != frame.OutputCount() || frame.GetInputParameterCount() != frame.PinCount() || frame.GetOutputParameterCount() != frame.PoutCount() || frame.GetLocalParameterCount() != frame.LocalCount()) return 155;\n"
        "    if (!frame.IsInputActive(0) || frame.IsOutputActive(0) || !frame.IsInputParameterEnabled(0) || !frame.IsOutputParameterEnabled(0)) return 156;\n"
        "    if (frame.GetInput(0) is null || frame.GetOutput(0) is null) return 157;\n"
        "    if (frame.InputPosition(raw.GetInput(0)) != 0 || frame.OutputPosition(raw.GetOutput(0)) != 0) return 144;\n"
        "    if (frame.InputParameter(0) is null || frame.OutputParameter(0) is null) return 151;\n"
        "    if (frame.GetInputParameter(0) is null || frame.GetOutputParameter(0) is null) return 152;\n"
        "    if (frame.GetInputParameterPosition(frame.GetInputParameter(0)) != 0 || frame.GetOutputParameterPosition(frame.GetOutputParameter(0)) != 0) return 158;\n"
        "    if (frame.PinPosition(frame.InputParameter(0)) != 0 || frame.PoutPosition(frame.OutputParameter(0)) != 0) return 145;\n"
        "    if (frame.PinReadDataPtr(0).IsNull() || frame.GetInputParameterReadDataPtr(0).IsNull()) return 146;\n"
        "    int sdkPinValue = 0;\n"
        "    if (frame.GetInputParameterValue(0, sdkPinValue) != CK_OK || sdkPinValue != 41) return 161;\n"
        "    frame.Function(); frame.InterfaceChunk();\n"
        "    frame.Type(); frame.Flags(); frame.PrototypeGuid(); frame.PrototypeName(); frame.Version(); frame.Priority(); frame.LastExecutionTime(); frame.IsActive(); frame.IsUsingFunction(); frame.IsTargetable(); frame.IsUsingTarget(); frame.CompatibleClassId();\n"
        "    frame.SubBehaviorCount(); frame.SubBehaviorLinkCount(); frame.ParameterOperationCount(); frame.CurrentLevel(); frame.CurrentScene(); frame.PreviousScene(); frame.CurrentRenderContext(); frame.CallbackMessage();\n"
        "    if (!frame.InputActive(0) || !frame.InputActive(\"In\")) return 130;\n"
        "    if (frame.OutputActive(0) || frame.OutputActive(\"Out\")) return 131;\n"
        "    int scratchLocal = frame.FindLocal(\"__CKAS_FrameScratch\");\n"
        "    if (scratchLocal < 0 || frame.Local(scratchLocal) is null || frame.Local(\"__CKAS_FrameScratch\") is null) return 132;\n"
        "    int sdkLocalValue = 0;\n"
        "    if (frame.SetLocalParameterValue(scratchLocal, 222) != CK_OK) return 162;\n"
        "    if (frame.GetLocalParameterValue(scratchLocal, sdkLocalValue) != CK_OK || sdkLocalValue != 222) return 163;\n"
        "    if (!frame.SetLocal(scratchLocal, 123) || !frame.SetLocal(\"__CKAS_FrameScratch\", 124)) return 133;\n"
        "    if (frame.LocalInt(\"__CKAS_FrameScratch\", -1) != 124 || frame.LocalInt(scratchLocal, -1) != 124) return 168;\n"
        "    if (frame.LocalInt(\"__CKAS_MissingLocal\", 77) != 77) return 169;\n"
        "    if (frame.LocalString(\"__CKAS_FrameScratch\", \"fallback\") != \"fallback\") return 170;\n"
        "    if (!frame.LocalBool(\"__CKAS_FrameScratch\", true)) return 171;\n"
        "    if (frame.LocalFloat(\"__CKAS_FrameScratch\", -1.0f) != 124.0f || frame.LocalFloat(scratchLocal, -1.0f) != 124.0f) return 172;\n"
        "    if (frame.LocalObject(\"__CKAS_MissingLocal\") !is null) return 173;\n"
        "    if (frame.LocalParameter(scratchLocal) is null || frame.GetLocalParameter(scratchLocal) is null) return 153;\n"
        "    if (frame.LocalPosition(frame.LocalParameter(scratchLocal)) != scratchLocal || frame.GetLocalParameterPosition(frame.GetLocalParameter(scratchLocal)) != scratchLocal) return 147;\n"
        "    if (frame.LocalReadDataPtr(scratchLocal).IsNull() || frame.LocalWriteDataPtr(scratchLocal).IsNull()) return 148;\n"
        "    frame.LocalObject(scratchLocal);\n"
        "    frame.NotifyEdition(); frame.NotifySettingsEdition();\n"
        "    BehaviorLayout@ runtimeLayout = frame.Layout();\n"
        "    int traceSetting = frame.FindSetting(\"Trace\");\n"
        "    if (frame.SettingCount() <= 0 || runtimeLayout is null || traceSetting < 0 || traceSetting != runtimeLayout.FindSetting(\"Trace\")) return 134;\n"
        "    ParamRef@ traceRef = frame.Setting(traceSetting);\n"
        "    if (traceRef is null || traceRef.index != traceSetting || !frame.IsSetting(traceSetting)) return 140;\n"
        "    if (!frame.SetSetting(\"Trace\", true) || !traceRef.Get().AsBool()) return 141;\n"
        "    if (!frame.SetSetting(traceSetting, false) || traceRef.Get().AsBool()) return 142;\n"
        "    if (!frame.Take(0)) return 103;\n"
        "    if (frame.InputActive(\"In\")) return 135;\n"
        "    int value = frame.PinInt(\"Value\", -1);\n"
        "    if (value != 41) return 104;\n"
        "    if (frame.PinInt(0, -1) != 41) return 136;\n"
        "    if (frame.PinInt(\"Missing\", 42) != 42) return 106;\n"
        "    if (frame.PinString(\"MissingText\", \"fallback\") != \"fallback\") return 107;\n"
        "    if (!frame.PinBool(\"MissingBool\", true)) return 108;\n"
        "    if (frame.PinFloat(\"MissingFloat\", 3.5f) != 3.5f) return 109;\n"
        "    if (frame.PinString(0, \"int-fallback\") != \"int-fallback\" || frame.PinBool(0, true) != true || frame.PinFloat(0, -1.0f) != 41.0f) return 137;\n"
        "    if (frame.PinBool(\"Value\", false)) return 110;\n"
        "    if (frame.PinString(\"Value\", \"int-fallback\") != \"int-fallback\") return 114;\n"
        "    AsyncTask<void>@ delay = frame.LinkDelay(1);\n"
        "    if (delay is null) return 167;\n"
        "    Await(delay);\n"
        "    if (frame.Context() is null) return 111;\n"
        "    if (frame.Behavior() is null) return 112;\n"
        "    if (frame.Self() is null || frame.Layout() is null) return 138;\n"
        "    if (frame.PinInt(\"MissingAfterResume\", 43) != 43) return 113;\n"
        "    bool sdkPoutValue = false;\n"
        "    if (frame.SetOutputParameterValue(0, true) != CK_OK) return 164;\n"
        "    if (frame.GetOutputParameterValue(0, sdkPoutValue) != CK_OK || !sdkPoutValue) return 165;\n"
        "    if (!frame.SetPout(\"Handled\", false) || !frame.SetPout(0, true)) return 105;\n"
        "    frame.Emit(0);\n"
        "    if (!frame.OutputActive(\"Out\")) return 139;\n"
        "    return 0;\n"
        "  }\n"
        "  int Execute(const CKBehaviorContext &in ctx) {\n"
        "    return 1901;\n"
        "  }\n"
        "}\n";

    const std::filesystem::path scriptRoot =
        std::filesystem::temp_directory_path() / "__ckas_executor_runtime_selftest";
    const std::filesystem::path packageDir = scriptRoot / "Scripts" / scriptId;
    std::error_code fsError;
    std::filesystem::remove_all(scriptRoot, fsError);
    fsError.clear();
    if (!std::filesystem::create_directories(packageDir, fsError)) {
        error = "Executor self-test failed to create script entry directory: " + packageDir.string();
        return false;
    }
    if (!WriteExecutorSelfTestFile(packageDir / "script.as",
                                   "[executor id=\"ckas.executor.selftest\" name=\"Executor Self Test\" version=\"1.0.0\" entry=\"runtime.as\"]\n",
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
        error = reloadError.empty() ? "Executor self-test script entry package failed to load." : reloadError;
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_ExecutorSelfTestBehavior"),
        CK_OBJECTCREATION_DYNAMIC));
    CKParameterLocal *sourceParam = nullptr;
    CKParameterLocal *scratchParam = nullptr;
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
    if (!SetExecutorStringSetting(behavior, EXECUTOR_SETTING_SCRIPT, scriptId, error) ||
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
    scratchParam = behavior->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_FrameScratch"), CKPGUID_INT);
    int scratchValue = 7;
    if (!scratchParam || scratchParam->SetValue(&scratchValue, sizeof(scratchValue)) != CK_OK) {
        cleanup();
        error = "Executor self-test failed to create scratch local parameter.";
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
    scratchValue = 0;
    if (scratchParam->GetValue(&scratchValue) != CK_OK || scratchValue != 124) {
        cleanup();
        error = "Executor self-test did not write scratch local parameter through BehaviorFrame.";
        return false;
    }

    cleanup();
    error.clear();
    return true;
}
