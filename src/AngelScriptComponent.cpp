//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Component
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ScriptManager.h"
#include "ScriptComponentMetadata.h"
#include "ScriptComponentAutomation.h"
#include "ScriptComponentInjection.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptParameterConversion.h"
#include "ScriptRunner.h"

CKObjectDeclaration *FillBehaviorAngelScriptComponentDecl();
CKERROR CreateAngelScriptComponentProto(CKBehaviorPrototype **pproto);
int AngelScriptComponent(const CKBehaviorContext &behcontext);
CKERROR AngelScriptComponentCallBack(const CKBehaviorContext &behcontext);

namespace AngelScriptComponentInternal {

constexpr int COMPONENT_STATE = 0;
constexpr int OUTPUT_ERROR_MESSAGE = 1;
constexpr int SCRIPT_PARAM = 0;
constexpr int CLASS_PARAM = 1;
constexpr int SOURCE_PARAM = 2;
constexpr int FILE_PARAM = 3;
constexpr int MANIFEST_PARAM = 4;
constexpr int FIXED_INPUT_PARAMETER_COUNT = 5;

bool DestroyInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state);

std::string ReadStringParameter(CKBehavior *beh, int index) {
    if (!beh) {
        return {};
    }

    CKSTRING value = (CKSTRING) beh->GetInputParameterReadDataPtr(index);
    return value ? std::string(value) : std::string();
}

bool SetParameterDefaultValue(CKParameterLocal *local, const ScriptComponentBinding &binding, CKContext *context) {
    if (!local || !binding.HasDefault) {
        return true;
    }

    const ScriptParamValueKind valueKind = ValueKindFromComponentKind(binding.Kind);
    if (valueKind != ScriptParamValueKind::Empty) {
        std::string error;
        return SetParameterDefaultText(local, binding.DefaultValue, error);
    }

    switch (binding.Kind) {
        case ScriptComponentBindingKind::ParamRef:
        case ScriptComponentBindingKind::ParamValue:
        case ScriptComponentBindingKind::ParamTypeInfo: {
            std::string error;
            return SetParameterDefaultText(local, binding.DefaultValue, error);
        }
        case ScriptComponentBindingKind::BBPrototype:
        case ScriptComponentBindingKind::BBDecl:
        case ScriptComponentBindingKind::BBSlot:
        case ScriptComponentBindingKind::BBConfig:
            return local->SetStringValue(const_cast<CKSTRING>(binding.DefaultValue.c_str())) == CK_OK;
        case ScriptComponentBindingKind::Object: {
            CK_ID id = 0;
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(StripQuotes(binding.DefaultValue).c_str(), &end, 0);
            if (end && *end == '\0') {
                id = static_cast<CK_ID>(parsed);
            } else if (context) {
                const std::string name = StripQuotes(binding.DefaultValue);
                CKObject *object = context->GetObjectByName(const_cast<CKSTRING>(name.c_str()));
                id = object ? object->GetID() : 0;
            }
            return local->SetValue(&id, sizeof(id)) == CK_OK;
        }
        case ScriptComponentBindingKind::BehaviorRef: {
            CK_ID id = 0;
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(StripQuotes(binding.DefaultValue).c_str(), &end, 0);
            if (end && *end == '\0') {
                id = static_cast<CK_ID>(parsed);
            } else if (context) {
                const std::string name = StripQuotes(binding.DefaultValue);
                CKBehavior *behavior = FindBehaviorByNameInContext(context, name);
                if (!behavior) {
                    behavior = CKBehavior::Cast(context->GetObjectByName(const_cast<CKSTRING>(name.c_str())));
                }
                id = behavior ? behavior->GetID() : 0;
            }
            return local->SetValue(&id, sizeof(id)) == CK_OK;
        }
        default:
            return true;
    }
}

int FindComponentInputParameterIndex(CKBehavior *beh, const std::string &name) {
    if (!beh) {
        return -1;
    }
    for (int i = 0; i < beh->GetInputParameterCount(); ++i) {
        CKParameterIn *param = beh->GetInputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return i;
        }
    }
    return -1;
}

CKParameter *EnsureInputSource(CKBehavior *beh, CKParameterIn *input, const ScriptComponentBinding &binding) {
    if (!beh || !input) {
        return nullptr;
    }

    CKParameter *source = input->GetRealSource();
    const std::string sourceName = "__CKAS_ComponentInput_" + binding.FieldName;
    if (source) {
        CKParameterLocal *localSource = CKParameterLocal::Cast(source);
        if (localSource && NameEquals(localSource->GetName(), sourceName)) {
            SetParameterDefaultValue(localSource, binding, beh->GetCKContext());
        }
        return source;
    }

    CKParameterLocal *local = nullptr;
    for (int i = 0; i < beh->GetLocalParameterCount(); ++i) {
        CKParameterLocal *candidate = beh->GetLocalParameter(i);
        if (candidate && NameEquals(candidate->GetName(), sourceName)) {
            local = candidate;
            break;
        }
    }

    if (!local) {
        local = beh->CreateLocalParameter(const_cast<CKSTRING>(sourceName.c_str()), input->GetGUID());
    }
    if (!local) {
        return nullptr;
    }

    SetParameterDefaultValue(local, binding, beh->GetCKContext());
    beh->SetInputParameterDefaultValue(input, local);
    if (input->SetDirectSource(local) != CK_OK) {
        return nullptr;
    }
    return local;
}

bool SyncDeclaredInputParameters(CKBehavior *beh, ScriptComponentState *state, std::vector<ScriptComponentBinding> &bindings, std::string &error) {
    if (!beh || !state) {
        error = "Component behavior is not available.";
        return false;
    }

    std::unordered_set<std::string> currentNames;
    for (const ScriptComponentBinding &binding : bindings) {
        if (currentNames.find(binding.ParameterName) != currentNames.end()) {
            error = "Duplicate Component input parameter declaration: " + binding.ParameterName;
            return false;
        }
        currentNames.insert(binding.ParameterName);
    }

    for (const std::string &oldName : state->ManagedInputParameterNames) {
        if (currentNames.find(oldName) != currentNames.end()) {
            continue;
        }

        for (int i = beh->GetInputParameterCount() - 1; i >= FIXED_INPUT_PARAMETER_COUNT; --i) {
            CKParameterIn *param = beh->GetInputParameter(i);
            if (param && NameEquals(param->GetName(), oldName)) {
                CKParameterIn *removed = beh->RemoveInputParameter(i);
                if (removed) {
                    CKDestroyObject(removed);
                }
            }
        }
    }

    state->ManagedInputParameterNames.clear();
    for (ScriptComponentBinding &binding : bindings) {
        int index = FindComponentInputParameterIndex(beh, binding.ParameterName);
        if (index >= 0 && index < FIXED_INPUT_PARAMETER_COUNT) {
            error = "Component input parameter name is reserved: " + binding.ParameterName;
            return false;
        }
        if (index < 0) {
            CKParameterIn *created = beh->CreateInputParameter(const_cast<CKSTRING>(binding.ParameterName.c_str()), binding.ParameterGuid);
            if (!created) {
                error = "Failed to create Component input parameter: " + binding.ParameterName;
                return false;
            }
            index = beh->GetInputParameterPosition(created);
        }

        CKParameterIn *input = beh->GetInputParameter(index);
        if (!input) {
            error = "Component input parameter is not available: " + binding.ParameterName;
            return false;
        }
        if (input->GetGUID() != binding.ParameterGuid) {
            input->SetGUID(binding.ParameterGuid, TRUE, const_cast<CKSTRING>(binding.ParameterName.c_str()));
        }
        if (!EnsureInputSource(beh, input, binding)) {
            error = "Failed to create default source for Component input parameter: " + binding.ParameterName;
            return false;
        }

        binding.InputParameterIndex = index;
        state->ManagedInputParameterNames.push_back(binding.ParameterName);
    }

    return true;
}

std::string BuildRuntimeModuleName(CKBehavior *beh, const std::string &scriptName) {
    return "__CKASComponent_" + std::to_string(beh ? beh->GetID() : 0) + "_" + scriptName;
}

bool IsOutputErrorEnabled(CKBehavior *beh) {
    if (!beh) {
        return false;
    }

    CKBOOL outputError = FALSE;
    beh->GetLocalParameterValue(OUTPUT_ERROR_MESSAGE, &outputError);
    return outputError != FALSE;
}

void SetErrorOutput(CKBehavior *beh, ScriptComponentState *state, const std::string &message, const std::string &stackTrace = std::string()) {
    if (state) {
        state->Failed = true;
    }

    if (!beh) {
        return;
    }

    if (!message.empty()) {
        if (CKContext *context = beh->GetCKContext()) {
            context->OutputToConsoleEx(const_cast<char *>("[AngelScript Component] %s(%d): %s"),
                beh->GetName() ? beh->GetName() : "<unnamed>",
                beh->GetID(),
                message.c_str());
            if (!stackTrace.empty()) {
                context->OutputToConsoleEx(const_cast<char *>("[AngelScript Component] stack: %s"), stackTrace.c_str());
            }
        }
    }

    if (IsOutputErrorEnabled(beh) && beh->GetOutputParameterCount() >= 2) {
        beh->SetOutputParameterValue(0, message.c_str());
        beh->SetOutputParameterValue(1, stackTrace.c_str());
    }

    beh->ActivateOutput(2);
}

void SetRunnerErrorOutput(CKBehavior *beh, ScriptComponentState *state, const char *context) {
    std::string message = context ? context : "AngelScript Component failed.";
    std::string stackTrace;

    if (state && state->Runner) {
        const std::string &runnerError = state->Runner->GetErrorMessage();
        if (!runnerError.empty()) {
            message += ": ";
            message += runnerError;
        }
        stackTrace = state->Runner->GetStackTrace();
    }

    SetErrorOutput(beh, state, message, stackTrace);
}

ScriptComponentState *GetState(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh) {
        return nullptr;
    }

    ScriptComponentState *state = nullptr;
    beh->GetLocalParameterValue(COMPONENT_STATE, &state);
    if (state) {
        return state;
    }

    ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
    if (!man) {
        return nullptr;
    }

    state = man->GetOrCreateComponentState(beh);
    beh->SetLocalParameterValue(COMPONENT_STATE, &state);
    return state;
}

bool IsContextLifecycleMethod(asIScriptFunction *func) {
    if (!func || func->GetReturnTypeId() != asTYPEID_VOID || func->GetParamCount() != 1) {
        return false;
    }

    int typeId = 0;
    asDWORD flags = 0;
    if (func->GetParam(0, &typeId, &flags) < 0) {
        return false;
    }

    int contextTypeId = func->GetEngine()->GetTypeIdByDecl("CKBehaviorContext");
    asDWORD refFlags = flags & asTM_INOUTREF;
    return typeId == contextTypeId && refFlags == asTM_INREF && (flags & asTM_CONST) != 0;
}

bool CacheLifecycleMethod(asITypeInfo *type, const char *name, bool required, asIScriptFunction *&out, ScriptRunner *runner) {
    out = nullptr;
    bool sawName = false;
    std::string invalidDecl;

    for (asUINT i = 0; i < type->GetMethodCount(); ++i) {
        asIScriptFunction *method = type->GetMethodByIndex(i);
        if (!method || std::strcmp(method->GetName(), name) != 0) {
            continue;
        }

        sawName = true;
        if (invalidDecl.empty()) {
            invalidDecl = method->GetDeclaration(false, false, true);
        }

        if (IsContextLifecycleMethod(method)) {
            method->AddRef();
            out = method;
            return true;
        }
    }

    if (sawName || required) {
        std::string message = "Invalid or missing lifecycle method: void ";
        message += name;
        message += "(const CKBehaviorContext &in ctx)";
        if (!invalidDecl.empty()) {
            message += " (found ";
            message += invalidDecl;
            message += ")";
        }
        if (runner) {
            runner->SetErrorMessage(message);
        }
        return false;
    }

    return true;
}

bool CacheComponentMethods(ScriptComponentState *state, asITypeInfo *type) {
    if (!state || !state->Runner || !type) {
        return false;
    }

    return CacheLifecycleMethod(type, "OnLoad", false, state->OnLoad, state->Runner) &&
           CacheLifecycleMethod(type, "Awake", false, state->Awake, state->Runner) &&
           CacheLifecycleMethod(type, "OnEnable", false, state->OnEnable, state->Runner) &&
           CacheLifecycleMethod(type, "Start", false, state->Start, state->Runner) &&
           CacheLifecycleMethod(type, "Update", true, state->Update, state->Runner) &&
           CacheLifecycleMethod(type, "OnDisable", false, state->OnDisable, state->Runner) &&
           CacheLifecycleMethod(type, "OnDestroy", false, state->OnDestroy, state->Runner) &&
           CacheLifecycleMethod(type, "OnReset", false, state->OnReset, state->Runner);
}

enum class LifecycleInvokeStatus {
    Finished,
    Suspended,
    Failed
};

LifecycleInvokeStatus InvokeLifecycle(CKBehavior *beh, ScriptComponentState *state, asIScriptFunction *method, const CKBehaviorContext &behcontext, const char *name) {
    if (!method) {
        return LifecycleInvokeStatus::Finished;
    }

    if (!state || !state->Runner || !state->Object) {
        SetErrorOutput(beh, state, std::string("Cannot execute lifecycle method: ") + name);
        return LifecycleInvokeStatus::Failed;
    }

    if (!state->Runner->IsContextSuspended()) {
        state->ActiveLifecycle = method;
        state->ActiveLifecycleName = name ? name : "";
    }

    const ScriptExecutionStatus status = state->Runner->ExecuteObjectMethodStatus(state->Object, method, behcontext);
    if (status == ScriptExecutionStatus::Suspended) {
        return LifecycleInvokeStatus::Suspended;
    }
    state->ActiveLifecycle = nullptr;
    state->ActiveLifecycleName.clear();
    if (status == ScriptExecutionStatus::Failed) {
        SetRunnerErrorOutput(beh, state, name);
        return LifecycleInvokeStatus::Failed;
    }

    return LifecycleInvokeStatus::Finished;
}

bool LifecycleFinished(LifecycleInvokeStatus status) {
    return status == LifecycleInvokeStatus::Finished;
}

void CancelSuspendedLifecycle(ScriptComponentState *state, const char *unlessName = nullptr) {
    if (!state || !state->Runner || !state->Runner->IsContextSuspended()) {
        return;
    }
    if (unlessName && state->ActiveLifecycleName == unlessName) {
        return;
    }
    state->Runner->AbortContext();
    state->ActiveLifecycle = nullptr;
    state->ActiveLifecycleName.clear();
}

bool CompleteInitialLifecycles(CKBehavior *beh, ScriptComponentState *state, const CKBehaviorContext &behcontext) {
    if (!state->OnLoadCalled) {
        const LifecycleInvokeStatus status = InvokeLifecycle(beh, state, state->OnLoad, behcontext, "OnLoad");
        if (!LifecycleFinished(status)) {
            return false;
        }
        state->OnLoadCalled = true;
    }

    if (!state->AwakeCalled) {
        const LifecycleInvokeStatus status = InvokeLifecycle(beh, state, state->Awake, behcontext, "Awake");
        if (!LifecycleFinished(status)) {
            return false;
        }
        state->AwakeCalled = true;
    }

    return true;
}

bool ComponentIdentityChanged(ScriptComponentState *state,
                              const std::string &scriptName,
                              const std::string &className,
                              const std::string &source,
                              const std::string &file,
                              const std::string &manifest,
                              const std::string &runtimeModuleName,
                              bool privateModule) {
    return !state->Loaded ||
           state->ScriptName != scriptName ||
           state->ClassName != className ||
           state->Source != source ||
           state->File != file ||
           state->Manifest != manifest ||
           state->RuntimeModuleName != runtimeModuleName ||
           state->PrivateModule != privateModule;
}

bool EnsureComponentReady(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh || !context || !state) {
        return false;
    }

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man) {
        SetErrorOutput(beh, state, "Can not get script manager.");
        return false;
    }

    const std::string scriptName = ReadStringParameter(beh, SCRIPT_PARAM);
    const std::string className = ReadStringParameter(beh, CLASS_PARAM);
    const std::string source = ReadStringParameter(beh, SOURCE_PARAM);
    const std::string file = ReadStringParameter(beh, FILE_PARAM);
    const std::string manifest = ReadStringParameter(beh, MANIFEST_PARAM);

    if (scriptName.empty()) {
        SetErrorOutput(beh, state, "No script module specified.");
        return false;
    }

    if (className.empty()) {
        SetErrorOutput(beh, state, "No component class specified.");
        return false;
    }

    const bool privateModule = !source.empty() || !file.empty();
    const std::string runtimeModuleName = privateModule ? BuildRuntimeModuleName(beh, scriptName) : scriptName;

    if (ComponentIdentityChanged(state, scriptName, className, source, file, manifest, runtimeModuleName, privateModule)) {
        if (state->Loaded && state->Object) {
            state->PendingResetRuntime = true;
            if (!DestroyInstance(behcontext, state)) {
                return false;
            }
            state->PendingResetRuntime = false;
        }
        if (man->GetBehaviorBridge()) {
            man->GetBehaviorBridge()->DestroyComponentTasks(state->BehaviorId);
        }
        man->ResetComponentStateRuntime(state, true);
        state->ScriptName = scriptName;
        state->ClassName = className;
        state->Source = source;
        state->File = file;
        state->Manifest = manifest;
        state->RuntimeModuleName = runtimeModuleName;
        state->PrivateModule = privateModule;
        state->Failed = false;
    }

    if (state->Failed) {
        return false;
    }

    if (state->Loaded && state->Object && state->Update) {
        std::string injectError;
        if (!InjectComponentParameters(behcontext, state, false, injectError)) {
            SetErrorOutput(beh, state, injectError);
            return false;
        }
        return CompleteInitialLifecycles(beh, state, behcontext);
    }

    if (privateModule) {
        man->UnloadScript(runtimeModuleName.c_str());
        int r = source.empty()
            ? man->LoadScript(runtimeModuleName.c_str(), file.c_str())
            : man->CompileScript(runtimeModuleName.c_str(), source.c_str());
        if (r < 0) {
            SetErrorOutput(beh, state, "Failed to load component script module.");
            return false;
        }
    }

    if (!state->Runner) {
        state->Runner = new ScriptRunner(man);
    }

    if (!state->Runner->SetScript(runtimeModuleName.c_str())) {
        SetRunnerErrorOutput(beh, state, "Failed to attach component script");
        return false;
    }

    asITypeInfo *type = state->Runner->GetTypeInfoByName(className.c_str());
    if (!type) {
        SetErrorOutput(beh, state, "Component class not found: " + className);
        return false;
    }

    std::vector<ScriptComponentBinding> bindings = BuildComponentBindingSpecs(state, type);
    for (ScriptComponentBinding &binding : bindings) {
        std::string bindingError;
        if (!ResolveComponentBinding(state->Runner->GetModule()->GetEngine(), type, behcontext.Context, binding, bindingError)) {
            SetErrorOutput(beh, state, bindingError);
            return false;
        }
    }

    std::string syncError;
    if (!SyncDeclaredInputParameters(beh, state, bindings, syncError)) {
        SetErrorOutput(beh, state, syncError);
        return false;
    }
    state->Bindings = std::move(bindings);

    state->Object = state->Runner->CreateScriptObject(type);
    if (!state->Object) {
        SetRunnerErrorOutput(beh, state, "Failed to instantiate component class");
        return false;
    }

    std::string injectError;
    if (!InjectComponentParameters(behcontext, state, true, injectError)) {
        SetErrorOutput(beh, state, injectError);
        return false;
    }

    if (!CacheComponentMethods(state, type)) {
        SetRunnerErrorOutput(beh, state, "Failed to cache component lifecycle methods");
        return false;
    }

    state->Loaded = true;

    return CompleteInitialLifecycles(beh, state, behcontext);
}

bool EnableInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    if (!state || state->InstanceEnabled || state->Paused || !state->ScriptActive) {
        return true;
    }

    if (!EnsureComponentReady(behcontext, state)) {
        return false;
    }

    const LifecycleInvokeStatus status = InvokeLifecycle(beh, state, state->OnEnable, behcontext, "OnEnable");
    if (!LifecycleFinished(status)) {
        return false;
    }

    state->InstanceEnabled = true;
    return true;
}

bool DisableInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    if (!state || (!state->InstanceEnabled && state->ActiveLifecycleName != "OnDisable")) {
        return true;
    }

    CancelSuspendedLifecycle(state, "OnDisable");
    const LifecycleInvokeStatus status = InvokeLifecycle(beh, state, state->OnDisable, behcontext, "OnDisable");
    if (!LifecycleFinished(status)) {
        if (status == LifecycleInvokeStatus::Failed) {
            state->InstanceEnabled = false;
            return false;
        }
        return false;
    }

    StopComponentLifetimeBBConfigs(behcontext, state);
    state->InstanceEnabled = false;
    return true;
}

bool DestroyInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    if (!state || !state->Object) {
        return true;
    }

    state->PendingDestroy = true;
    if (!DisableInstance(behcontext, state)) {
        return false;
    }
    CancelSuspendedLifecycle(state, "OnDestroy");
    const LifecycleInvokeStatus status = InvokeLifecycle(behcontext.Behavior, state, state->OnDestroy, behcontext, "OnDestroy");
    if (status != LifecycleInvokeStatus::Finished) {
        if (status == LifecycleInvokeStatus::Failed) {
            state->PendingDestroy = false;
        }
        return false;
    }
    DestroyComponentLifetimeBBConfigs(state);
    state->PendingDestroy = false;
    return true;
}

void SyncErrorOutputParameters(CKBehavior *beh) {
    if (!beh) {
        return;
    }

    if (IsOutputErrorEnabled(beh)) {
        while (beh->GetOutputParameterCount() > 0) {
            CKParameterOut *removed = beh->RemoveOutputParameter(0);
            if (removed) {
                CKDestroyObject(removed);
            }
        }
        beh->CreateOutputParameter("Error", CKPGUID_STRING);
        beh->CreateOutputParameter("StackTrace", CKPGUID_STRING);
    } else {
        while (beh->GetOutputParameterCount() > 0) {
            CKParameterOut *removed = beh->RemoveOutputParameter(0);
            if (removed) {
                CKDestroyObject(removed);
            }
        }
    }
}

} // namespace AngelScriptComponentInternal

#if CKAS_BUILD_SELF_TESTS
bool RunScriptComponentMetadataSelfTest(std::string &error) {
    std::vector<ScriptComponentBinding> bindings;
    auto addMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding binding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "TextConfig", binding)) {
            error = "Component metadata self-test failed to parse '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(bindings, binding);
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
        if (!AngelScriptComponentInternal::ParseBindingMetadata(
                "bbconfig prototype=\"Logics/Calculator/Identity\"",
                fieldName,
                configBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot config metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, configBinding);

        ScriptComponentBinding slotBinding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata("bbpout \"pOut 0\"", fieldName, slotBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot fragment metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, slotBinding);
    }
    if (repeatedSlotBindings.size() != 2 ||
        repeatedSlotBindings[0].ParameterName != "FirstConfig" ||
        repeatedSlotBindings[1].ParameterName != "SecondConfig") {
        error = "Component metadata self-test let BBConfig fragment slot names override field parameter names.";
        return false;
    }

    ScriptComponentBinding legacyManaged;
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "AggregateConfig", aggregateBinding)) {
            error = "Component metadata self-test failed to parse aggregate metadata '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(aggregateBindings, aggregateBinding);
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
        if (!AngelScriptComponentInternal::ParseManifestLine(line, manifestBinding)) {
            error = "Component metadata self-test failed to parse manifest line '" + line + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(manifestBindings, manifestBinding);
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
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
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
    const std::string occurrenceCacheText = AngelScriptComponentInternal::BuildBBConfigBindingCacheText(occurrenceBinding, 0, std::string());
    if (occurrenceCacheText.find("pin:Value[3]=") == std::string::npos ||
        occurrenceCacheText.find("source:pIn 0[1]<-SourceConfig.pout:Value[2]") == std::string::npos) {
        error = "Component metadata self-test did not include slot occurrences in BBConfig cache text.";
        return false;
    }

    return true;
}
#endif

CKObjectDeclaration *FillBehaviorAngelScriptComponentDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScript Component");
    od->SetDescription("Run an AngelScript class as a component");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptComponentProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptComponentProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScript Component");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Enable");
    proto->DeclareInput("Disable");

    proto->DeclareOutput("Enabled");
    proto->DeclareOutput("Disabled");
    proto->DeclareOutput("Error");

    proto->DeclareInParameter("Script", CKPGUID_STRING);
    proto->DeclareInParameter("Class", CKPGUID_STRING);
    proto->DeclareInParameter("Source", CKPGUID_STRING);
    proto->DeclareInParameter("File", CKPGUID_STRING);
    proto->DeclareInParameter("Manifest", CKPGUID_STRING);

    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareSetting("Output Error Message", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptComponent);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_MESSAGESENDER |
                                                 CKBEHAVIOR_MESSAGERECEIVER |
                                                 CKBEHAVIOR_TARGETABLE |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptComponentCallBack);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptComponent(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh) {
        return CKBR_PARAMETERERROR;
    }

    ScriptComponentState *state = AngelScriptComponentInternal::GetState(behcontext);
    if (!state) {
        beh->ActivateOutput(2);
        return CKBR_OWNERERROR;
    }

    if (state->PendingDestroy) {
        if (!AngelScriptComponentInternal::DestroyInstance(behcontext, state)) {
            return state->Runner && state->Runner->IsContextSuspended() ? CKBR_ACTIVATENEXTFRAME : CKBR_OK;
        }
        if (state->PendingResetRuntime) {
            if (ScriptManager *man = ScriptManager::GetManager(behcontext.Context)) {
                man->ResetComponentStateRuntime(state, true);
            }
            return CKBR_ACTIVATENEXTFRAME;
        }
        return CKBR_OK;
    }

    if (state->ActiveLifecycleName == "OnDisable") {
        if (!AngelScriptComponentInternal::DisableInstance(behcontext, state)) {
            return state->Runner && state->Runner->IsContextSuspended() ? CKBR_ACTIVATENEXTFRAME : CKBR_OK;
        }
        if (state->PendingDisableOutput) {
            state->PendingDisableOutput = false;
            beh->ActivateOutput(1);
        }
        return CKBR_OK;
    }

    if (beh->IsInputActive(1)) {
        beh->ActivateInput(1, FALSE);
        state->DesiredEnabled = false;
        if (!AngelScriptComponentInternal::DisableInstance(behcontext, state)) {
            state->PendingDisableOutput = true;
            return state->Runner && state->Runner->IsContextSuspended() ? CKBR_ACTIVATENEXTFRAME : CKBR_OK;
        }
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    if (beh->IsInputActive(0)) {
        beh->ActivateInput(0, FALSE);
        state->DesiredEnabled = true;
        state->ScriptActive = true;
        state->Paused = false;
        state->Failed = false;

        if (!AngelScriptComponentInternal::EnsureComponentReady(behcontext, state) ||
            !AngelScriptComponentInternal::EnableInstance(behcontext, state)) {
            return state->Runner && state->Runner->IsContextSuspended() ? CKBR_ACTIVATENEXTFRAME : CKBR_OK;
        }

        beh->ActivateOutput(0);
    }

    if (!state->DesiredEnabled || !state->ScriptActive || state->Paused || state->Failed) {
        return CKBR_OK;
    }

    if (!AngelScriptComponentInternal::EnsureComponentReady(behcontext, state) ||
        !AngelScriptComponentInternal::EnableInstance(behcontext, state)) {
        return state->Runner && state->Runner->IsContextSuspended() ? CKBR_ACTIVATENEXTFRAME : CKBR_OK;
    }

    if (!state->StartCalled) {
        const AngelScriptComponentInternal::LifecycleInvokeStatus status =
            AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->Start, behcontext, "Start");
        if (status != AngelScriptComponentInternal::LifecycleInvokeStatus::Finished) {
            return status == AngelScriptComponentInternal::LifecycleInvokeStatus::Suspended
                ? CKBR_ACTIVATENEXTFRAME
                : CKBR_OK;
        }
        state->StartCalled = true;
    }

    {
        std::string automationError;
        if (!AngelScriptComponentInternal::EnsureAutoStartedBBConfigs(behcontext, state, automationError)) {
            AngelScriptComponentInternal::SetErrorOutput(beh, state, automationError);
            return CKBR_OK;
        }
    }

    {
        const AngelScriptComponentInternal::LifecycleInvokeStatus status =
            AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->Update, behcontext, "Update");
        if (status != AngelScriptComponentInternal::LifecycleInvokeStatus::Finished) {
            return status == AngelScriptComponentInternal::LifecycleInvokeStatus::Suspended
                ? CKBR_ACTIVATENEXTFRAME
                : CKBR_OK;
        }
    }

    {
        std::string automationError;
        if (!AngelScriptComponentInternal::StepAutomatedBBConfigs(behcontext, state, automationError)) {
            AngelScriptComponentInternal::SetErrorOutput(beh, state, automationError);
            return CKBR_OK;
        }
    }

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR AngelScriptComponentCallBack(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh) {
        return CKBR_PARAMETERERROR;
    }

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man) {
        return CKBR_OWNERERROR;
    }

    ScriptComponentState *state = nullptr;

    switch (behcontext.CallbackMessage) {
        case CKM_BEHAVIORCREATE:
        case CKM_BEHAVIORLOAD: {
            state = man->GetOrCreateComponentState(beh);
            beh->SetLocalParameterValue(AngelScriptComponentInternal::COMPONENT_STATE, &state);
        }
        break;

        case CKM_BEHAVIORACTIVATESCRIPT: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->ScriptActive = true;
                if (state->DesiredEnabled) {
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIORDEACTIVATESCRIPT: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->ScriptActive = false;
                AngelScriptComponentInternal::DisableInstance(behcontext, state);
            }
        }
        break;

        case CKM_BEHAVIORPAUSE: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->PauseComponentTasks(beh->GetID(), true);
                }
                state->Paused = true;
                AngelScriptComponentInternal::DisableInstance(behcontext, state);
            }
        }
        break;

        case CKM_BEHAVIORRESUME: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->PauseComponentTasks(beh->GetID(), false);
                }
                state->Paused = false;
                if (state->DesiredEnabled && state->ScriptActive) {
                    AngelScriptComponentInternal::EnableInstance(behcontext, state);
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIORRESET: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                AngelScriptComponentInternal::DestroyComponentLifetimeBBConfigs(state);
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->ResetComponentTasks(beh->GetID());
                }
                state->Failed = false;
                state->StartCalled = false;
                if (state->Object) {
                    AngelScriptComponentInternal::CancelSuspendedLifecycle(state, "OnReset");
                    const AngelScriptComponentInternal::LifecycleInvokeStatus status =
                        AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->OnReset, behcontext, "OnReset");
                    if (status == AngelScriptComponentInternal::LifecycleInvokeStatus::Failed) {
                        return CKBR_OK;
                    }
                }
                if (state->DesiredEnabled && state->ScriptActive && !state->Paused) {
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIOREDITED: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->DestroyComponentTasks(beh->GetID());
                }
                man->ResetComponentStateRuntime(state, true);
            }
        }
        break;

        case CKM_BEHAVIORSETTINGSEDITED: {
            AngelScriptComponentInternal::SyncErrorOutputParameters(beh);
        }
        break;

        case CKM_BEHAVIORDELETE: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->DesiredEnabled = false;
                state->ScriptActive = false;
                man->ResetComponentStateRuntime(state, true);
            }
            man->ReleaseComponentState(beh);
        }
        break;

        default:
            break;
    }

    return CKBR_OK;
}
