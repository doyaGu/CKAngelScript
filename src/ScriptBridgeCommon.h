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
#include "ScriptRefCounted.h"
#include "ScriptManager.h"
#include "XObjectArray.h"

#undef GetObject

static std::string SafeString(CKSTRING value) {
    return value ? std::string(value) : std::string();
}

static CKObject *GetCKObjectById(CKContext *context, CK_ID id) {
    return context && id ? CKGetObject(context, id) : nullptr;
}

static ScriptBridgeObjectStamp CaptureBridgeObjectStamp(CKObject *object) {
    ScriptBridgeObjectStamp stamp;
    if (!object) {
        return stamp;
    }

    stamp.ClassId = object->GetClassID();
    stamp.Set(ScriptBridgeObjectStampFlags::ClassId, true);

    if (CKBehavior *behavior = CKBehavior::Cast(object)) {
        stamp.PrototypeGuid = behavior->GetPrototypeGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::PrototypeGuid, stamp.PrototypeGuid.IsValid());
        CKObject *owner = behavior->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterIn *input = CKParameterIn::Cast(object)) {
        stamp.TypeGuid = input->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, stamp.TypeGuid.IsValid());
        CKObject *owner = input->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
        stamp.OperationGuid = operation->GetOperationGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::OperationGuid, stamp.OperationGuid.IsValid());
        CKBehavior *owner = operation->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameter *parameter = CKParameter::Cast(object)) {
        stamp.TypeGuid = parameter->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, stamp.TypeGuid.IsValid());
        CKObject *owner = parameter->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    return stamp;
}

static bool BridgeObjectStampMatches(CKObject *object, const ScriptBridgeObjectStamp &stamp) {
    if (!object || object->IsToBeDeleted()) {
        return false;
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::ClassId) && object->GetClassID() != stamp.ClassId) {
        return false;
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::PrototypeGuid)) {
        CKBehavior *behavior = CKBehavior::Cast(object);
        if (!behavior || behavior->GetPrototypeGuid() != stamp.PrototypeGuid) {
            return false;
        }
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::TypeGuid)) {
        if (CKParameterIn *input = CKParameterIn::Cast(object)) {
            if (input->GetGUID() != stamp.TypeGuid) {
                return false;
            }
        } else if (CKParameter *parameter = CKParameter::Cast(object)) {
            if (parameter->GetGUID() != stamp.TypeGuid) {
                return false;
            }
        } else {
            return false;
        }
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::OperationGuid)) {
        CKParameterOperation *operation = CKParameterOperation::Cast(object);
        if (!operation || operation->GetOperationGuid() != stamp.OperationGuid) {
            return false;
        }
    }

    if (stamp.Has(ScriptBridgeObjectStampFlags::OwnerId)) {
        CK_ID ownerId = 0;
        if (CKBehavior *behavior = CKBehavior::Cast(object)) {
            ownerId = behavior->GetOwner() ? behavior->GetOwner()->GetID() : 0;
        } else if (CKParameterIn *input = CKParameterIn::Cast(object)) {
            ownerId = input->GetOwner() ? input->GetOwner()->GetID() : 0;
        } else if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
            ownerId = operation->GetOwner() ? operation->GetOwner()->GetID() : 0;
        } else if (CKParameter *parameter = CKParameter::Cast(object)) {
            ownerId = parameter->GetOwner() ? parameter->GetOwner()->GetID() : 0;
        }
        if (ownerId != stamp.OwnerId) {
            return false;
        }
    }

    return true;
}

static CKObject *GetStampedCKObjectById(CKContext *context, CK_ID id, const ScriptBridgeObjectStamp &stamp) {
    CKObject *object = GetCKObjectById(context, id);
    return BridgeObjectStampMatches(object, stamp) ? object : nullptr;
}

static CKERROR CallBridgeBehaviorCallback(CKBehavior *behavior,
                                          CKDWORD message,
                                          const CKBehaviorContext *sourceContext = nullptr) {
    if (!behavior) {
        return CKERR_INVALIDPARAMETER;
    }

    CKBehaviorPrototype *prototype = behavior->GetPrototype();
    CKBEHAVIORCALLBACKFCT callback = prototype ? prototype->GetBehaviorCallbackFct() : nullptr;
    if (!callback) {
        return behavior->CallCallbackFunction(message);
    }

    CKContext *context = sourceContext && sourceContext->Context ? sourceContext->Context : behavior->GetCKContext();
    CKBehaviorContext callbackContext = sourceContext ? *sourceContext : (context ? context->m_BehaviorContext : CKBehaviorContext());
    callbackContext.Context = context;
    callbackContext.Behavior = behavior;
    callbackContext.CallbackMessage = message;
    callbackContext.CallbackArg = nullptr;
    if (context && !callbackContext.ParameterManager) {
        callbackContext.ParameterManager = context->GetParameterManager();
    }
    return callback(callbackContext);
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

static CKGUID ResolveBridgeValueType(CKContext *context, const ScriptParamValue &value, CKGUID fallbackGuid = CKGUID()) {
    if (value.TypeGuid.IsValid()) {
        return value.TypeGuid;
    }
    if (fallbackGuid.IsValid()) {
        return fallbackGuid;
    }
    (void) context;
    return ScriptParameterGuidForValue(value);
}

static CKERROR SetBridgeParamValue(CKParameter *param, const ScriptParamValue &value, std::string &error) {
    if (!param) {
        error = "Parameter is not valid.";
        return CKERR_INVALIDPARAMETER;
    }

    if (value.Kind == ScriptParamValueKind::Empty) {
        error = "Parameter value is empty.";
        return CKERR_INVALIDPARAMETER;
    }

    return WriteParameterValue(param, value, error);
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
                                          const ScriptParamValue &value,
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
            error = fmt::format("got {}", ScriptParamValueKindName(value.Kind));
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
                                 const std::vector<ScriptBridgeIndexedValue> &parameters,
                                 std::string &error,
                                 std::unordered_map<int, CK_ID> *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.PinIndex, entry.Value, error, inputSources)) {
            return false;
        }
    }
    return true;
}

static bool ApplyIndexedInputParameters(CKBehavior *behavior,
                                        const std::vector<ScriptBridgeIndexedValue> &parameters,
                                        std::string &error,
                                        std::unordered_map<int, CK_ID> *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.PinIndex, entry.Value, error, inputSources)) {
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
    return index < 0 ? !state.ActiveOutputs.Empty() : state.ActiveOutputs.Contains(index);
}

static bool ExecutionOutputSeen(const ScriptBridgeExecutionState &state, int index) {
    return index < 0 ? !state.SeenOutputs.Empty() : state.SeenOutputs.Contains(index);
}

static void MergeSeenOutputs(ScriptBridgeExecutionState &state) {
    state.SeenOutputs.MergeFrom(state.ActiveOutputs);
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
            state.ActiveOutputs.Insert(i);
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

static CKParameter *ResolveStampedParameterSource(CKContext *context,
                                                 CK_ID id,
                                                 const ScriptBridgeObjectStamp &stamp,
                                                 std::string &error) {
    CKObject *object = GetStampedCKObjectById(context, id, stamp);
    if (!object) {
        error = fmt::format("Parameter source id={} is no longer valid or no longer matches the captured handle.", id);
        return nullptr;
    }

    CKParameter *source = ParameterSourceForConnection(object);
    if (!source) {
        error = fmt::format("Object id={} is not a readable parameter source.", id);
        return nullptr;
    }

    return source;
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
