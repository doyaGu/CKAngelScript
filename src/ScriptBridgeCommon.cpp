#include "ScriptBridgeCommon.h"

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

std::string SafeString(CKSTRING value) {
    return value ? std::string(value) : std::string();
}

CKObject *GetCKObjectById(CKContext *context, CK_ID id) {
    return context && id ? CKGetObject(context, id) : nullptr;
}

std::uintptr_t ScriptBridgeObjectAddress(CKObject *object) {
    return reinterpret_cast<std::uintptr_t>(object);
}

ScriptBridgeObjectStamp CaptureBridgeObjectStamp(CKObject *object) {
    ScriptBridgeObjectStamp stamp;
    if (!object) {
        return stamp;
    }

    stamp.ClassId = object->GetClassID();
    stamp.Set(ScriptBridgeObjectStampFlags::ClassId, true);

    if (CKBehavior *behavior = CKBehavior::Cast(object)) {
        stamp.PrototypeGuid = behavior->GetPrototypeGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::PrototypeGuid, CKGuidIsValid(stamp.PrototypeGuid));
        CKObject *owner = behavior->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterIn *input = CKParameterIn::Cast(object)) {
        stamp.TypeGuid = input->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, CKGuidIsValid(stamp.TypeGuid));
        CKObject *owner = input->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameterOperation *operation = CKParameterOperation::Cast(object)) {
        stamp.OperationGuid = operation->GetOperationGuid();
        stamp.Set(ScriptBridgeObjectStampFlags::OperationGuid, CKGuidIsValid(stamp.OperationGuid));
        CKBehavior *owner = operation->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    if (CKParameter *parameter = CKParameter::Cast(object)) {
        stamp.TypeGuid = parameter->GetGUID();
        stamp.Set(ScriptBridgeObjectStampFlags::TypeGuid, CKGuidIsValid(stamp.TypeGuid));
        CKObject *owner = parameter->GetOwner();
        stamp.OwnerId = owner ? owner->GetID() : 0;
        stamp.Set(ScriptBridgeObjectStampFlags::OwnerId, stamp.OwnerId != 0);
        return stamp;
    }

    return stamp;
}

bool BridgeObjectStampMatches(CKObject *object, const ScriptBridgeObjectStamp &stamp) {
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

CKObject *GetStampedCKObjectById(CKContext *context, CK_ID id, const ScriptBridgeObjectStamp &stamp) {
    CKObject *object = GetCKObjectById(context, id);
    return BridgeObjectStampMatches(object, stamp) ? object : nullptr;
}

CKObjectDeclaration *ResolvePrototypeDeclaration(CKBehaviorPrototype *prototype, bool requireManagerMetadata) {
    if (!prototype) {
        return nullptr;
    }

    CKObjectDeclaration *decl = prototype->GetSoureObjectDeclaration();
    if (decl && (!requireManagerMetadata || decl->GetManagerNeededCount() > 0)) {
        return decl;
    }

    const CKGUID guid = prototype->GetGuid();
    for (int i = 0; i < CKGetPrototypeDeclarationCount(); ++i) {
        CKObjectDeclaration *candidate = CKGetPrototypeDeclaration(i);
        if (!candidate || candidate->GetGuid() != guid) {
            continue;
        }
        if (!requireManagerMetadata || candidate->GetManagerNeededCount() > 0) {
            return candidate;
        }
    }
    return decl;
}

CKERROR CallBridgeBehaviorCallback(CKBehavior *behavior,
                                   CKDWORD message,
                                   const CKBehaviorContext *sourceContext) {
    if (!behavior) {
        return CKERR_INVALIDPARAMETER;
    }

    CKBehaviorPrototype *prototype = behavior->GetPrototype();
    CKBEHAVIORCALLBACKFCT callback = prototype ? prototype->GetBehaviorCallbackFct() : nullptr;
    if (!callback) {
        return behavior->CallCallbackFunction(message);
    }

    CKContext *context = sourceContext && sourceContext->Context ? sourceContext->Context : behavior->GetCKContext();
    CKBehaviorContext callbackContext = sourceContext ? *sourceContext : CKBehaviorContext();
#if CKVERSION == 0x13022002
    if (!sourceContext && context) {
        callbackContext = context->m_BehaviorContext;
    }
#endif
    callbackContext.Context = context;
    callbackContext.Behavior = behavior;
    callbackContext.CallbackMessage = message;
    callbackContext.CallbackArg = nullptr;
    if (context && !callbackContext.ParameterManager) {
        callbackContext.ParameterManager = context->GetParameterManager();
    }
    return callback(callbackContext);
}

std::string IndexedName(const char *prefix, int index) {
    return fmt::format("{}{}", prefix, index);
}

std::string GuidToString(CKGUID guid) {
    return fmt::format("guid:0x{:08x},0x{:08x}", guid.d[0], guid.d[1]);
}

bool CKGuidIsValid(CKGUID guid) {
    return guid.IsValid() != FALSE;
}

bool NameEquals(CKSTRING actual, const std::string &expected) {
    return actual && actual[0] != '\0' && actual == expected;
}

CKBehavior *FindBehaviorByNameInContext(CKContext *context, const std::string &name) {
    const std::string text = TrimString(name);
    if (!context || text.empty()) {
        return nullptr;
    }

    const XObjectPointerArray &behaviors = context->GetObjectListByType(CKCID_BEHAVIOR, TRUE);
    for (int i = 0; i < behaviors.Size(); ++i) {
        CKBehavior *behavior = CKBehavior::Cast(behaviors[i]);
        if (behavior && NameEquals(behavior->GetName(), text)) {
            return behavior;
        }
    }
    return nullptr;
}

std::string TrimString(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return std::string();
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

void SetScriptException(const std::string &message) {
    if (asIScriptContext *ctx = asGetActiveContext()) {
        ctx->SetException(message.c_str());
    }
}

bool IsBehaviorErrorCode(int rc) {
    return rc == CKBR_ATTACHFAILED ||
           rc == CKBR_DETACHFAILED ||
           rc == CKBR_LOCKED ||
           rc == CKBR_INFINITELOOP ||
           rc >= CKBR_GENERICERROR;
}

std::string ParameterTypeLabel(CKContext *context, CKGUID guid) {
    std::string guidText = GuidToString(guid);
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    CKSTRING name = parameterManager ? parameterManager->ParameterGuidToName(guid) : nullptr;
    if (name && name[0] != '\0') {
        return fmt::format("{} ({})", name, guidText);
    }
    return guidText;
}

int ParameterDefaultSize(CKContext *context, CKGUID guid) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    CKParameterTypeDesc *desc = parameterManager ? parameterManager->GetParameterTypeDescription(guid) : nullptr;
    return desc ? desc->DefaultSize : 0;
}

std::string ParameterTypeLabel(CKContext *context, CKParameter *parameter) {
    return parameter ? ParameterTypeLabel(context, parameter->GetGUID()) : std::string("unknown");
}

std::string ParameterTypeLabel(CKContext *context, CKParameterIn *parameter) {
    return parameter ? ParameterTypeLabel(context, parameter->GetGUID()) : std::string("unknown");
}

std::string DescribePrototypeParameter(CKContext *context, CKPARAMETER_DESC *parameter) {
    if (!parameter) {
        return "<null>";
    }
    return fmt::format("{}: {}", SafeString(parameter->Name), ParameterTypeLabel(context, parameter->Guid));
}

CKBEHAVIORIO_DESC *GetPrototypeInput(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetInputCount()) {
        return nullptr;
    }
#if CKVERSION == 0x13022002
    CKBEHAVIORIO_DESC **list = prototype->GetInIOList();
    return list ? list[index] : nullptr;
#else
    const XArray<CKBEHAVIORIO_DESC> &list = prototype->GetInIOList();
    return const_cast<CKBEHAVIORIO_DESC *>(&list[index]);
#endif
}

CKBEHAVIORIO_DESC *GetPrototypeOutput(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetOutputCount()) {
        return nullptr;
    }
#if CKVERSION == 0x13022002
    CKBEHAVIORIO_DESC **list = prototype->GetOutIOList();
    return list ? list[index] : nullptr;
#else
    const XArray<CKBEHAVIORIO_DESC> &list = prototype->GetOutIOList();
    return const_cast<CKBEHAVIORIO_DESC *>(&list[index]);
#endif
}

CKPARAMETER_DESC *GetPrototypeInputParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetInParameterCount()) {
        return nullptr;
    }
#if CKVERSION == 0x13022002
    CKPARAMETER_DESC **list = prototype->GetInParameterList();
    return list ? list[index] : nullptr;
#else
    const XClassArray<CKPARAMETER_DESC> &list = prototype->GetInParameterList();
    return const_cast<CKPARAMETER_DESC *>(&list[index]);
#endif
}

CKPARAMETER_DESC *GetPrototypeOutputParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetOutParameterCount()) {
        return nullptr;
    }
#if CKVERSION == 0x13022002
    CKPARAMETER_DESC **list = prototype->GetOutParameterList();
    return list ? list[index] : nullptr;
#else
    const XClassArray<CKPARAMETER_DESC> &list = prototype->GetOutParameterList();
    return const_cast<CKPARAMETER_DESC *>(&list[index]);
#endif
}

CKPARAMETER_DESC *GetPrototypeLocalParameter(CKBehaviorPrototype *prototype, int index) {
    if (!prototype || index < 0 || index >= prototype->GetLocalParameterCount()) {
        return nullptr;
    }
#if CKVERSION == 0x13022002
    CKPARAMETER_DESC **list = prototype->GetLocalParameterList();
    return list ? list[index] : nullptr;
#else
    const XClassArray<CKPARAMETER_DESC> &list = prototype->GetLocalParameterList();
    return const_cast<CKPARAMETER_DESC *>(&list[index]);
#endif
}

int FindInputIndex(CKBehavior *behavior, const std::string &name) {
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

int FindOutputIndex(CKBehavior *behavior, const std::string &name) {
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

int FindInputParameterIndex(CKBehavior *behavior, const std::string &name) {
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

std::vector<int> FindInputParameterIndices(CKBehavior *behavior, const std::string &name) {
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

std::string DescribeInputParameterList(CKBehavior *behavior) {
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

std::string DescribeInputParameterCandidates(CKBehavior *behavior, const std::vector<int> &indices) {
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

bool ResolveInputParameterIndex(CKBehavior *behavior,
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

int InputParameterIndexOrResolved(CKBehavior *behavior,
                                         int index,
                                         std::string &error) {
    return ResolveInputParameterIndex(behavior, index, error) ? index : -1;
}

CKParameter *FindReadableParameter(CKBehavior *behavior, const std::string &name) {
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

std::string OutputName(CKBehavior *behavior, int index) {
    CKBehaviorIO *io = behavior ? behavior->GetOutput(index) : nullptr;
    if (io && io->GetName() && io->GetName()[0] != '\0') {
        return io->GetName();
    }
    return IndexedName("#", index);
}

std::string OutputParamName(CKBehavior *behavior, int index) {
    CKParameterOut *param = behavior ? behavior->GetOutputParameter(index) : nullptr;
    if (param && param->GetName() && param->GetName()[0] != '\0') {
        return param->GetName();
    }
    return IndexedName("#", index);
}

CKGUID ResolveBridgeValueType(CKContext *context, const ScriptParamValue &value, CKGUID fallbackGuid) {
    if (CKGuidIsValid(value.TypeGuid)) {
        return value.TypeGuid;
    }
    if (CKGuidIsValid(fallbackGuid)) {
        return fallbackGuid;
    }
    (void) context;
    return ScriptParameterGuidForValue(value);
}

CKERROR SetBridgeParamValue(CKParameter *param, const ScriptParamValue &value, std::string &error) {
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

CKParameterLocal *EnsureInputSource(CKBehavior *behavior, int index, ScriptBridgeInputSourceBindings *inputSources) {
    if (!behavior || index < 0 || index >= behavior->GetInputParameterCount()) {
        return nullptr;
    }

    CKContext *context = behavior->GetCKContext();
    if (!context) {
        return nullptr;
    }

    if (inputSources) {
        const CK_ID sourceId = inputSources->Find(index);
        if (sourceId != 0) {
            if (auto *param = CKParameterLocal::Cast(GetCKObjectById(context, sourceId))) {
                return param;
            }
            inputSources->Remove(index);
        }
    }

    const std::string sourceName = fmt::format("__CKAS_BridgeInput_{}", index);
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (local && NameEquals(local->GetName(), sourceName)) {
            if (inputSources) {
                inputSources->Set(index, local->GetID());
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
        inputSources->Set(index, local->GetID());
    }
    return local;
}

std::string OutputSinkName(int index) {
    return fmt::format("__CKAS_BridgeOutput_{}", index);
}

CKParameterLocal *FindLocalParameterByName(CKBehavior *behavior, const std::string &name) {
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

CKParameterLocal *EnsureOutputSink(CKBehavior *behavior, int index) {
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

void EnsureOutputSinks(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }

    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        EnsureOutputSink(behavior, i);
    }
}

bool SetInputParameterValueByIndex(CKBehavior *behavior,
                                          int index,
                                          const ScriptParamValue &value,
                                          std::string &error,
                                          ScriptBridgeInputSourceBindings *inputSources) {
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

bool ApplyInputParameters(CKBehavior *behavior,
                                 const std::vector<ScriptBridgeIndexedValue> &parameters,
                                 std::string &error,
                                 ScriptBridgeInputSourceBindings *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.PinIndex, entry.Value, error, inputSources)) {
            return false;
        }
    }
    return true;
}

bool ApplyIndexedInputParameters(CKBehavior *behavior,
                                        const std::vector<ScriptBridgeIndexedValue> &parameters,
                                        std::string &error,
                                        ScriptBridgeInputSourceBindings *inputSources) {
    for (const auto &entry : parameters) {
        if (!SetInputParameterValueByIndex(behavior, entry.PinIndex, entry.Value, error, inputSources)) {
            return false;
        }
    }
    return true;
}

bool PulseInput(CKBehavior *behavior, const std::string &input, std::string &error, bool reset) {
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

bool PulseInputIndex(CKBehavior *behavior, int inputIndex, std::string &error, bool reset) {
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

void ClearInputs(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        behavior->ActivateInput(i, FALSE);
    }
}

void ClearOutputs(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        behavior->ActivateOutput(i, FALSE);
    }
}

bool ExecutionOutputActive(const ScriptBridgeExecutionState &state, int index) {
    return index < 0 ? !state.ActiveOutputs.Empty() : state.ActiveOutputs.Contains(index);
}

bool ExecutionOutputSeen(const ScriptBridgeExecutionState &state, int index) {
    return index < 0 ? !state.SeenOutputs.Empty() : state.SeenOutputs.Contains(index);
}

void MergeSeenOutputs(ScriptBridgeExecutionState &state) {
    state.SeenOutputs.MergeFrom(state.ActiveOutputs);
}

ScriptBridgeExecutionState CaptureExecutionState(CKBehavior *behavior, int returnCode) {
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

void ActivateParentChain(CKBehavior *behavior, bool reset) {
    for (CKBehavior *current = behavior ? behavior->GetParent() : nullptr; current; current = current->GetParent()) {
        current->Activate(TRUE, reset ? TRUE : FALSE);
    }
}

bool ActivateOwnerScriptInCurrentScene(CKBehavior *behavior, bool reset) {
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

bool RaiseExecutionState(const ScriptBridgeExecutionState &state, const CKBehaviorContext &ctx) {
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

ScriptBridgeBBInvocationSpec MakeDefaultRequest(const CKBehaviorContext &ctx) {
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

CK_ID ComponentIdFromContext(const CKBehaviorContext &ctx) {
    return ctx.Behavior ? ctx.Behavior->GetID() : 0;
}

ScriptManager *ManagerFromContext(const CKBehaviorContext &ctx) {
    return ctx.Context ? ScriptManager::GetManager(ctx.Context) : nullptr;
}

CKBehavior *FindBehaviorRecursive(CKBehavior *root, const std::string &name) {
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

CKBehavior *FindBehaviorOnOwner(CKBeObject *owner, const std::string &name) {
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

CKParameter *ParameterSourceForConnection(CKObject *parameter) {
    if (CKParameterIn *pin = CKParameterIn::Cast(parameter)) {
        return pin->GetRealSource();
    }
    return CKParameter::Cast(parameter);
}

CKParameter *ResolveParameterSource(CKContext *context, CK_ID id) {
    return ParameterSourceForConnection(GetCKObjectById(context, id));
}

CKParameter *ResolveStampedParameterSource(CKContext *context,
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

CKGUID ResolveOperationGuid(CKContext *context, CKGUID guid, const std::string &name, std::string &error) {
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (!pm) {
        error = "CKParameterManager is not available.";
        return CKGUID();
    }

    if (CKGuidIsValid(guid)) {
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
    if (!CKGuidIsValid(resolved) || pm->OperationGuidToCode(resolved) < 0) {
        error = fmt::format("Parameter operation '{}' was not found.", name);
        return CKGUID();
    }
    return resolved;
}
