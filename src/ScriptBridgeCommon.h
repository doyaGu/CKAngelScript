#ifndef CK_SCRIPTBRIDGECOMMON_H
#define CK_SCRIPTBRIDGECOMMON_H

#include "ScriptBehaviorBridge.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <fmt/format.h>

#include "CKAll.h"
#include "ScriptManager.h"
#include "XObjectArray.h"

#undef GetObject

class RefCounted {
public:
    void AddRef() const {
        ++m_RefCount;
    }

    void Release() const {
        if (--m_RefCount == 0) {
            delete this;
        }
    }

protected:
    virtual ~RefCounted() = default;

private:
    mutable int m_RefCount = 1;
};

static std::string SafeString(CKSTRING value) {
    return value ? std::string(value) : std::string();
}

static CKObject *GetCKObjectById(CKContext *context, CK_ID id) {
    return context && id ? CKGetObject(context, id) : nullptr;
}

static std::string IndexedName(const char *prefix, int index) {
    return fmt::format("{}{}", prefix, index);
}

static std::string GuidToString(CKGUID guid) {
    return fmt::format("guid:0x{:08x},0x{:08x}", guid.d[0], guid.d[1]);
}

static bool NameEquals(CKSTRING actual, const std::string &expected) {
    return actual && actual[0] != '\0' && actual == expected;
}

static std::string TrimString(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return std::string();
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

static void SetScriptException(const std::string &message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message.c_str());
    }
}

static bool IsBehaviorErrorCode(int rc) {
    return rc == CKBR_ATTACHFAILED ||
           rc == CKBR_DETACHFAILED ||
           rc == CKBR_LOCKED ||
           rc == CKBR_INFINITELOOP ||
           rc >= CKBR_GENERICERROR;
}

static std::string ParameterTypeLabel(CKContext *context, CKGUID guid) {
    std::string guidText = GuidToString(guid);
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    CKSTRING name = parameterManager ? parameterManager->ParameterGuidToName(guid) : nullptr;
    if (name && name[0] != '\0') {
        return fmt::format("{} ({})", name, guidText);
    }
    return guidText;
}

static int ParameterDefaultSize(CKContext *context, CKGUID guid) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    CKParameterTypeDesc *desc = parameterManager ? parameterManager->GetParameterTypeDescription(guid) : nullptr;
    return desc ? desc->DefaultSize : 0;
}

static std::string ParameterTypeLabel(CKContext *context, CKParameter *parameter) {
    return parameter ? ParameterTypeLabel(context, parameter->GetGUID()) : std::string("unknown");
}

static std::string ParameterTypeLabel(CKContext *context, CKParameterIn *parameter) {
    return parameter ? ParameterTypeLabel(context, parameter->GetGUID()) : std::string("unknown");
}

static std::string DescribePrototypeParameter(CKContext *context, CKPARAMETER_DESC *parameter) {
    if (!parameter) {
        return "<null>";
    }
    return fmt::format("{}: {}", SafeString(parameter->Name), ParameterTypeLabel(context, parameter->Guid));
}

static CKBEHAVIORIO_DESC *GetPrototypeInput(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetInputCount()) {
        return nullptr;
    }
    CKBEHAVIORIO_DESC **list = prototype->GetInIOList();
    return list ? list[index] : nullptr;
}

static CKBEHAVIORIO_DESC *GetPrototypeOutput(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetOutputCount()) {
        return nullptr;
    }
    CKBEHAVIORIO_DESC **list = prototype->GetOutIOList();
    return list ? list[index] : nullptr;
}

static CKPARAMETER_DESC *GetPrototypeInputParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetInParameterCount()) {
        return nullptr;
    }
    CKPARAMETER_DESC **list = prototype->GetInParameterList();
    return list ? list[index] : nullptr;
}

static CKPARAMETER_DESC *GetPrototypeOutputParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetOutParameterCount()) {
        return nullptr;
    }
    CKPARAMETER_DESC **list = prototype->GetOutParameterList();
    return list ? list[index] : nullptr;
}

static CKPARAMETER_DESC *GetPrototypeLocalParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetLocalParameterCount()) {
        return nullptr;
    }
    CKPARAMETER_DESC **list = prototype->GetLocalParameterList();
    return list ? list[index] : nullptr;
}

static int FindInputIndex(CKBehavior *behavior, const std::string &name) {
    if (!behavior) {
        return -1;
    }

    if (name.empty()) {
        return behavior->GetInputCount() > 0 ? 0 : -1;
    }

    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetInput(i);
        if (io && NameEquals(io->GetName(), name)) {
            return i;
        }
    }
    return -1;
}

static int FindOutputIndex(CKBehavior *behavior, const std::string &name) {
    if (!behavior) {
        return -1;
    }

    if (name.empty()) {
        return behavior->GetOutputCount() > 0 ? 0 : -1;
    }

    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetOutput(i);
        if (io && NameEquals(io->GetName(), name)) {
            return i;
        }
    }
    return -1;
}

static int FindInputParameterIndex(CKBehavior *behavior, const std::string &name) {
    if (!behavior) {
        return -1;
    }

    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return i;
        }
    }
    return -1;
}

static std::vector<int> FindInputParameterIndices(CKBehavior *behavior, const std::string &name) {
    std::vector<int> matches;
    if (!behavior) {
        return matches;
    }

    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            matches.push_back(i);
        }
    }
    return matches;
}

static std::string DescribeInputParameterList(CKBehavior *behavior) {
    if (!behavior) {
        return "<behavior is null>";
    }

    std::string result;
    CKContext *context = behavior->GetCKContext();
    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (!param) {
            continue;
        }

        if (!result.empty()) {
            result += ", ";
        }
        result += fmt::format("#{} {}: {}", i, SafeString(param->GetName()), ParameterTypeLabel(context, param));
    }

    return result.empty() ? "<none>" : result;
}

static std::string DescribeInputParameterCandidates(CKBehavior *behavior, const std::vector<int> &indices) {
    if (!behavior) {
        return "<behavior is null>";
    }

    std::string result;
    CKContext *context = behavior->GetCKContext();
    for (int index : indices) {
        CKParameterIn *param = index >= 0 && index < behavior->GetInputParameterCount() ? behavior->GetInputParameter(index) : nullptr;
        if (!param) {
            continue;
        }
        if (!result.empty()) {
            result += ", ";
        }
        result += fmt::format("#{} {}: {}", index, SafeString(param->GetName()), ParameterTypeLabel(context, param));
    }
    return result.empty() ? "<none>" : result;
}

static bool ResolveInputParameterIndex(CKBehavior *behavior,
                                       int index,
                                       std::string &error) {
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    if (index < 0 || index >= behavior->GetInputParameterCount()) {
        error = fmt::format("Input parameter index #{} is out of range for Building Block '{}' (input parameter count: {}).",
            index,
            SafeString(behavior->GetPrototypeName()),
            behavior->GetInputParameterCount());
        return false;
    }
    return true;
}

static int InputParameterIndexOrResolved(CKBehavior *behavior,
                                         int index,
                                         std::string &error) {
    return ResolveInputParameterIndex(behavior, index, error) ? index : -1;
}

static CKParameter *FindReadableParameter(CKBehavior *behavior, const std::string &name) {
    if (!behavior) {
        return nullptr;
    }

    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return param;
        }
    }

    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *param = behavior->GetLocalParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return param;
        }
    }

    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *param = behavior->GetInputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return param->GetRealSource();
        }
    }

    return nullptr;
}

static std::string OutputName(CKBehavior *behavior, int index) {
    CKBehaviorIO *io = behavior ? behavior->GetOutput(index) : nullptr;
    if (io && io->GetName() && io->GetName()[0] != '\0') {
        return io->GetName();
    }
    return IndexedName("#", index);
}

static std::string OutputParamName(CKBehavior *behavior, int index) {
    CKParameterOut *param = behavior ? behavior->GetOutputParameter(index) : nullptr;
    if (param && param->GetName() && param->GetName()[0] != '\0') {
        return param->GetName();
    }
    return IndexedName("#", index);
}

static XObjectArray ObjectArrayFromValue(const ScriptBridgeValue *value) {
    XObjectArray result;
    if (!value || value->Kind != ScriptBridgeValueKind::ObjectArray) {
        return result;
    }

    for (CK_ID id : value->ObjectIds) {
        result.PushBack(id);
    }
    return result;
}

static ScriptBridgeParamValue MakeBridgeParamValue(const ScriptBridgeValue &value) {
    ScriptBridgeParamValue result;
    result.ModeKind = ScriptBridgeParamValue::Mode::Value;
    result.Value = value;
    return result;
}

static ScriptBridgeParamValue MakeBridgeTextValue(const std::string &text, CKGUID typeGuid = CKGUID(), const std::string &typeName = std::string()) {
    ScriptBridgeParamValue result;
    result.ModeKind = ScriptBridgeParamValue::Mode::Text;
    result.TextValue = text;
    result.TypeGuid = typeGuid;
    result.TypeName = typeName;
    return result;
}

static ScriptBridgeParamValue MakeBridgeRawValue(CKGUID typeGuid, const std::string &typeName, const void *data, int size) {
    ScriptBridgeParamValue result;
    result.ModeKind = ScriptBridgeParamValue::Mode::Raw;
    result.TypeGuid = typeGuid;
    result.TypeName = typeName;
    if (data && size > 0) {
        const auto *bytes = static_cast<const char *>(data);
        result.RawData.assign(bytes, bytes + size);
    }
    return result;
}

static CKGUID ResolveBridgeValueType(CKContext *context, const ScriptBridgeParamValue &value, ScriptBridgeValueKind fallbackKind) {
    if (value.TypeGuid.IsValid()) {
        return value.TypeGuid;
    }
    if (!value.TypeName.empty()) {
        return ScriptResolveParameterGuid(context, value.TypeName, fallbackKind);
    }
    return ScriptParameterGuidForValueKind(fallbackKind);
}

static bool IsBridgeRawTypeCompatible(CKParameter *param, const ScriptBridgeParamValue &value, std::string &error) {
    if (!param || !value.TypeGuid.IsValid()) {
        return true;
    }

    CKContext *context = param->GetCKContext();
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (!pm) {
        return value.TypeGuid == param->GetGUID();
    }

    if (pm->IsTypeCompatible(value.TypeGuid, param->GetGUID()) || pm->IsTypeCompatible(param->GetGUID(), value.TypeGuid)) {
        return true;
    }

    error = fmt::format("Raw parameter type mismatch (expected {}, got {}).",
        ParameterTypeLabel(context, param->GetGUID()),
        ParameterTypeLabel(context, value.TypeGuid));
    return false;
}

static CKERROR SetBridgeParamValue(CKParameter *param, const ScriptBridgeParamValue &value, std::string &error) {
    if (!param) {
        error = "Parameter is not valid.";
        return CKERR_INVALIDPARAMETER;
    }

    switch (value.ModeKind) {
        case ScriptBridgeParamValue::Mode::Value:
            return SetParameterValue(param, value.Value);
        case ScriptBridgeParamValue::Mode::Text: {
            CKGUID requested = ResolveBridgeValueType(param->GetCKContext(), value, ScriptBridgeValueKind::String);
            if (requested.IsValid()) {
                CKParameterManager *pm = param->GetCKContext() ? param->GetCKContext()->GetParameterManager() : nullptr;
                if (pm && !pm->IsTypeCompatible(requested, param->GetGUID()) && !pm->IsTypeCompatible(param->GetGUID(), requested)) {
                    error = fmt::format("Text parameter type mismatch (expected {}, got {}).",
                        ParameterTypeLabel(param->GetCKContext(), param->GetGUID()),
                        ParameterTypeLabel(param->GetCKContext(), requested));
                    return CKERR_INVALIDPARAMETER;
                }
            }
            return param->SetStringValue(const_cast<CKSTRING>(value.TextValue.c_str()));
        }
        case ScriptBridgeParamValue::Mode::Raw: {
            if (!IsBridgeRawTypeCompatible(param, value, error)) {
                return CKERR_INVALIDPARAMETER;
            }
            const int expectedSize = param->GetDataSize();
            if (expectedSize <= 0) {
                error = fmt::format("Parameter '{}' has no fixed-size storage for raw access.", SafeString(param->GetName()));
                return CKERR_INVALIDPARAMETER;
            }
            if (static_cast<int>(value.RawData.size()) != expectedSize) {
                error = fmt::format("Raw parameter size mismatch for '{}' (expected {} bytes, got {}).",
                    SafeString(param->GetName()),
                    expectedSize,
                    value.RawData.size());
                return CKERR_INVALIDPARAMETER;
            }
            return param->SetValue(value.RawData.empty() ? nullptr : value.RawData.data(), expectedSize);
        }
        case ScriptBridgeParamValue::Mode::Empty:
        default:
            error = "Parameter value is empty.";
            return CKERR_INVALIDPARAMETER;
    }
}

static CKParameterLocal *EnsureInputSource(CKBehavior *behavior, int index, std::unordered_map<int, CK_ID> *inputSources) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        return nullptr;
    }

    CKContext *context = behavior->GetCKContext();
    if (!context) {
        return nullptr;
    }

    if (inputSources) {
        auto it = inputSources->find(index);
        if (it != inputSources->end()) {
            if (auto *param = CKParameterLocal::Cast(GetCKObjectById(context, it->second))) {
                return param;
            }
            inputSources->erase(it);
        }
    }

    const std::string sourceName = fmt::format("__CKAS_BridgeInput_{}", index);
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (local && NameEquals(local->GetName(), sourceName)) {
            if (inputSources) {
                (*inputSources)[index] = local->GetID();
            }
            return local;
        }
    }

    CKParameterIn *pin = behavior->GetInputParameter(index);
    if (!pin) {
        return nullptr;
    }

    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(sourceName.c_str()), pin->GetGUID());
    if (!local) {
        return nullptr;
    }

    behavior->SetInputParameterDefaultValue(pin, local);
    if (pin->SetDirectSource(local) != CK_OK) {
        return nullptr;
    }

    if (inputSources) {
        (*inputSources)[index] = local->GetID();
    }
    return local;
}

static std::string OutputSinkName(int index) {
    return fmt::format("__CKAS_BridgeOutput_{}", index);
}

static CKParameterLocal *FindLocalParameterByName(CKBehavior *behavior, const std::string &name) {
    if (!behavior) {
        return nullptr;
    }

    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (local && NameEquals(local->GetName(), name)) {
            return local;
        }
    }
    return nullptr;
}

static CKParameterLocal *EnsureOutputSink(CKBehavior *behavior, int index) {
    if (!behavior || index < 0 || index >= behavior->GetOutputParameterCount()) {
        return nullptr;
    }

    CKParameterOut *output = behavior->GetOutputParameter(index);
    if (!output) {
        return nullptr;
    }

    const std::string sinkName = OutputSinkName(index);
    CKParameterLocal *sink = FindLocalParameterByName(behavior, sinkName);
    if (!sink) {
        sink = behavior->CreateLocalParameter(const_cast<CKSTRING>(sinkName.c_str()), output->GetGUID());
    }
    if (!sink) {
        return nullptr;
    }

    const CKERROR err = output->AddDestination(sink, TRUE);
    if (err != CK_OK && err != CKERR_ALREADYPRESENT) {
        return nullptr;
    }
    return sink;
}

static void EnsureOutputSinks(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }

    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        EnsureOutputSink(behavior, i);
    }
}

static bool SetInputParameterValueByIndex(CKBehavior *behavior,
                                          int index,
                                          const ScriptBridgeParamValue &value,
                                          std::string &error,
                                          std::unordered_map<int, CK_ID> *inputSources) {
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    if (index < 0 || index >= behavior->GetInputParameterCount()) {
        error = fmt::format("Input parameter index #{} is out of range for Building Block '{}' (input parameter count: {}).",
            index,
            SafeString(behavior->GetPrototypeName()),
            behavior->GetInputParameterCount());
        return false;
    }

    CKParameterIn *input = behavior->GetInputParameter(index);
    CKParameterLocal *local = EnsureInputSource(behavior, index, inputSources);
    if (!local) {
        error = fmt::format("Failed to create input source for parameter #{}.", index);
        return false;
    }

    CKERROR err = SetBridgeParamValue(local, value, error);
    if (err != CK_OK) {
        if (error.empty()) {
            error = fmt::format("got {}", value.ModeKind == ScriptBridgeParamValue::Mode::Value
                ? ScriptBridgeValueKindName(value.Value.Kind)
                : (value.ModeKind == ScriptBridgeParamValue::Mode::Text ? "text" : "raw"));
        }
        error = fmt::format("Failed to set input parameter #{} '{}' on Building Block '{}' (expected {}, got {}, CKERROR {}).",
            index,
            SafeString(input ? input->GetName() : nullptr),
            SafeString(behavior->GetPrototypeName()),
            ParameterTypeLabel(behavior->GetCKContext(), input),
            error,
            err);
        return false;
    }

    return true;
}

static bool ApplyInputParameters(CKBehavior *behavior,
                                 const std::unordered_map<int, ScriptBridgeParamValue> &parameters,
                                 std::string &error,
                                 std::unordered_map<int, CK_ID> *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.first, entry.second, error, inputSources)) {
            return false;
        }
    }
    return true;
}

static bool ApplyIndexedInputParameters(CKBehavior *behavior,
                                        const std::unordered_map<int, ScriptBridgeParamValue> &parameters,
                                        std::string &error,
                                        std::unordered_map<int, CK_ID> *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.first, entry.second, error, inputSources)) {
            return false;
        }
    }
    return true;
}

static bool PulseInput(CKBehavior *behavior, const std::string &input, std::string &error, bool reset = false) {
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    if (reset) {
        behavior->Activate(TRUE, TRUE);
    }

    if (behavior->GetInputCount() == 0) {
        if (!reset) {
            behavior->Activate(TRUE, FALSE);
        }
        return true;
    }

    const int index = FindInputIndex(behavior, input);
    if (index < 0) {
        error = input.empty()
            ? "Behavior has no default input."
            : fmt::format("Input '{}' not found.", input);
        return false;
    }

    behavior->ActivateInput(index, TRUE);
    behavior->Activate(TRUE, FALSE);
    return true;
}

static bool PulseInputIndex(CKBehavior *behavior, int inputIndex, std::string &error, bool reset = false) {
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    if (reset) {
        behavior->Activate(TRUE, TRUE);
    }

    if (behavior->GetInputCount() == 0) {
        if (!reset) {
            behavior->Activate(TRUE, FALSE);
        }
        return true;
    }

    if (inputIndex < 0 || inputIndex >= behavior->GetInputCount()) {
        error = fmt::format("Input index #{} is out of range for behavior '{}' (input count: {}).",
            inputIndex,
            SafeString(behavior->GetName()),
            behavior->GetInputCount());
        return false;
    }

    behavior->ActivateInput(inputIndex, TRUE);
    behavior->Activate(TRUE, FALSE);
    return true;
}

static void ClearInputs(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        behavior->ActivateInput(i, FALSE);
    }
}

static void ClearOutputs(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        behavior->ActivateOutput(i, FALSE);
    }
}

static bool ExecutionOutputActive(const ScriptBridgeExecutionState &state, int index) {
    if (index < 0) {
        return !state.ActiveOutputs.empty();
    }
    return std::find(state.ActiveOutputs.begin(), state.ActiveOutputs.end(), index) != state.ActiveOutputs.end();
}

static bool ExecutionOutputSeen(const ScriptBridgeExecutionState &state, int index) {
    if (index < 0) {
        return !state.SeenOutputs.empty();
    }
    return std::find(state.SeenOutputs.begin(), state.SeenOutputs.end(), index) != state.SeenOutputs.end();
}

static void MergeSeenOutputs(ScriptBridgeExecutionState &state) {
    for (int output : state.ActiveOutputs) {
        if (std::find(state.SeenOutputs.begin(), state.SeenOutputs.end(), output) == state.SeenOutputs.end()) {
            state.SeenOutputs.push_back(output);
        }
    }
}

static ScriptBridgeExecutionState CaptureExecutionState(CKBehavior *behavior, int returnCode) {
    ScriptBridgeExecutionState state;
    state.Ok = !IsBehaviorErrorCode(returnCode);
    state.ReturnCode = returnCode;

    if (!state.Ok) {
        state.Error = fmt::format("Behavior returned error code {}.", returnCode);
    }

    if (!behavior) {
        state.Ok = false;
        state.Error = "Behavior is null.";
        return state;
    }

    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        if (behavior->IsOutputActive(i)) {
            state.ActiveOutputs.push_back(i);
        }
    }

    MergeSeenOutputs(state);
    return state;
}

static void ActivateParentChain(CKBehavior *behavior, bool reset) {
    for (CKBehavior *current = behavior ? behavior->GetParent() : nullptr; current; current = current->GetParent()) {
        current->Activate(TRUE, reset ? TRUE : FALSE);
    }
}

static bool ActivateOwnerScriptInCurrentScene(CKBehavior *behavior, bool reset) {
    if (!behavior) {
        return false;
    }

    CKBehavior *ownerScript = behavior->GetOwnerScript();
    if (!ownerScript || ownerScript->GetType() != CKBEHAVIORTYPE_SCRIPT) {
        return false;
    }

    CKContext *context = ownerScript->GetCKContext();
    CKScene *scene = context ? context->GetCurrentScene() : nullptr;
    if (!scene) {
        return false;
    }

    scene->Activate(ownerScript, reset ? TRUE : FALSE);
    return true;
}

static bool RaiseExecutionState(const ScriptBridgeExecutionState &state, const CKBehaviorContext &ctx) {
    if (state.Ok) {
        return true;
    }

    const std::string message = state.Error.empty()
        ? fmt::format("Building Block failed with return code {}.", state.ReturnCode)
        : state.Error;

    if (ctx.Context) {
        ctx.Context->OutputToConsoleEx(const_cast<char *>("[AngelScript BB] %s"), message.c_str());
    }

    if (ctx.Behavior) {
        if (ctx.Behavior->GetOutputParameterCount() >= 1) {
            ctx.Behavior->SetOutputParameterValue(0, message.c_str());
        }
        const int errorOutput = FindOutputIndex(ctx.Behavior, "Error");
        if (errorOutput >= 0) {
            ctx.Behavior->ActivateOutput(errorOutput);
        }
    }

    return false;
}

static ScriptBridgeBBInvocationSpec MakeDefaultRequest(const CKBehaviorContext &ctx) {
    ScriptBridgeBBInvocationSpec request;
    request.ComponentId = ctx.Behavior ? ctx.Behavior->GetID() : 0;

    if (ctx.Behavior) {
        if (CKBeObject *owner = ctx.Behavior->GetOwner()) {
            request.OwnerId = owner->GetID();
        }
        if (CKBeObject *target = ctx.Behavior->GetTarget()) {
            request.TargetId = target->GetID();
        }
    }

    return request;
}

static CK_ID ComponentIdFromContext(const CKBehaviorContext &ctx) {
    return ctx.Behavior ? ctx.Behavior->GetID() : 0;
}

static ScriptManager *ManagerFromContext(const CKBehaviorContext &ctx) {
    return ctx.Context ? ScriptManager::GetManager(ctx.Context) : nullptr;
}

static CKBehavior *FindBehaviorRecursive(CKBehavior *root, const std::string &name) {
    if (!root) {
        return nullptr;
    }

    if (NameEquals(root->GetName(), name) || SafeString(root->GetPrototypeName()) == name) {
        return root;
    }

    for (int i = 0; i < root->GetSubBehaviorCount(); ++i) {
        if (CKBehavior *found = FindBehaviorRecursive(root->GetSubBehavior(i), name)) {
            return found;
        }
    }

    return nullptr;
}

static CKBehavior *FindBehaviorOnOwner(CKBeObject *owner, const std::string &name) {
    if (!owner) {
        return nullptr;
    }

    for (int i = 0; i < owner->GetScriptCount(); ++i) {
        if (CKBehavior *found = FindBehaviorRecursive(owner->GetScript(i), name)) {
            return found;
        }
    }

    return nullptr;
}

static CKParameter *ParameterSourceForConnection(CKObject *parameter) {
    if (CKParameterIn *pin = CKParameterIn::Cast(parameter)) {
        return pin->GetRealSource();
    }
    return CKParameter::Cast(parameter);
}

static CKParameter *ResolveParameterSource(CKContext *context, CK_ID id) {
    return ParameterSourceForConnection(GetCKObjectById(context, id));
}

static CKGUID ResolveOperationGuid(CKContext *context, CKGUID guid, const std::string &name, std::string &error) {
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (!pm) {
        error = "CKParameterManager is not available.";
        return CKGUID();
    }

    if (guid.IsValid()) {
        if (pm->OperationGuidToCode(guid) < 0) {
            error = fmt::format("Parameter operation {} is not registered.", GuidToString(guid));
            return CKGUID();
        }
        return guid;
    }

    CKGUID parsed;
    if (ParseScriptGuidString(name, parsed)) {
        if (pm->OperationGuidToCode(parsed) < 0) {
            error = fmt::format("Parameter operation {} is not registered.", GuidToString(parsed));
            return CKGUID();
        }
        return parsed;
    }

    CKGUID resolved = pm->OperationNameToGuid(const_cast<CKSTRING>(name.c_str()));
    if (!resolved.IsValid() || pm->OperationGuidToCode(resolved) < 0) {
        error = fmt::format("Parameter operation '{}' was not found.", name);
        return CKGUID();
    }
    return resolved;
}


#endif // CK_SCRIPTBRIDGECOMMON_H
