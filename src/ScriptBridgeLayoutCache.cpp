#include "ScriptBridgeHandles.h"

#include <functional>
#include <limits.h>

#include <fmt/format.h>

namespace {

std::string LayoutGuidKey(CKGUID guid) {
    return fmt::format("{:08x}:{:08x}", guid.d[0], guid.d[1]);
}

void AppendLayoutSignature(std::string &signature, const std::string &text) {
    signature += text;
    signature += '\n';
}

void AppendLayoutSignature(std::string &signature, const CKGUID &guid) {
    signature += LayoutGuidKey(guid);
    signature += '\n';
}

int LayoutGenerationFromSignature(const std::string &signature) {
    return static_cast<int>(std::hash<std::string>{}(signature) & static_cast<size_t>(INT_MAX));
}

bool BehaviorFlagSet(CKDWORD flags, CKDWORD flag) {
    return (flags & flag) != 0;
}

bool IsBridgeScratchLocalName(const std::string &name) {
    return name.find("__CKAS_BridgeInput_") == 0 ||
           name.find("__CKAS_BridgeOutput_") == 0 ||
           name.find("__CKAS_BridgePrototypeDefault_") == 0 ||
           name.find("__CKAS_GraphEditInput_") == 0 ||
           name.find("__CKAS_Op") == 0 ||
           name == "__CKAS_GraphEdit_Target" ||
           name == "__CKAS_Target";
}

bool RequestNeedsConfiguredLayout(const ScriptBridgeBBInvocationSpec &request) {
    return request.OwnerId != 0 ||
           request.TargetId != 0 ||
           !request.IndexedSettings.empty() ||
           !request.NamedSettings.empty();
}

std::string LayoutStampKey(const ScriptBridgeObjectStamp &stamp) {
    return fmt::format("{}:{}:{}:{}:{}:{}:{}",
                       stamp.Address,
                       stamp.Flags,
                       stamp.ClassId,
                       stamp.OwnerId,
                       LayoutGuidKey(stamp.TypeGuid),
                       LayoutGuidKey(stamp.PrototypeGuid),
                       LayoutGuidKey(stamp.OperationGuid));
}

void AppendValueKey(std::string &key, const ScriptParamValue &value) {
    key += ScriptParamValueKindName(value.Kind);
    key += ':';
    key += GuidToString(value.TypeGuid);
    key += ':';
    key += value.TypeNameText();
    key += '=';
    key += ScriptParamValueToText(value);
}

bool SetPreviewLocalParameter(CKBehavior *behavior,
                              int index,
                              const ScriptParamValue &value,
                              std::string &error) {
    if (!behavior) {
        error = "Behavior is not valid.";
        return false;
    }
    if (index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = fmt::format("Local parameter #{} is out of range for Building Block '{}' (local count: {}).",
                            index,
                            SafeString(behavior->GetPrototypeName()),
                            behavior->GetLocalParameterCount());
        return false;
    }
    CKParameterLocal *local = behavior->GetLocalParameter(index);
    if (!local) {
        error = fmt::format("Local parameter #{} is not available.", index);
        return false;
    }
    const CKERROR err = SetBridgeParamValue(local, value, error);
    if (err != CK_OK) {
        error = fmt::format("Failed to set local parameter #{} '{}' for layout preview (expected {}, CKERROR {}): {}",
                            index,
                            SafeString(local->GetName()),
                            ParameterTypeLabel(behavior->GetCKContext(), local),
                            err,
                            error.empty() ? "conversion failed" : error);
        return false;
    }
    return true;
}

bool ApplyPreviewIndexedSettings(CKBehavior *behavior,
                                 const std::vector<ScriptBridgeIndexedValue> &settings,
                                 std::string &error) {
    for (const ScriptBridgeIndexedValue &entry : settings) {
        if (!SetPreviewLocalParameter(behavior, entry.PinIndex, entry.Value, error)) {
            return false;
        }
    }
    return true;
}

int FindPreviewSetting(CKBehavior *behavior,
                       const std::string &name,
                       int occurrence,
                       std::string &error) {
    if (!behavior) {
        error = "Behavior is not valid.";
        return -1;
    }
    if (occurrence < 0) {
        error = "Setting occurrence must be non-negative.";
        return -1;
    }

    int seen = 0;
    std::string candidates;
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (!behavior->IsLocalParameterSetting(i)) {
            continue;
        }
        const std::string candidate = SafeString(local ? local->GetName() : nullptr);
        if (!candidates.empty()) {
            candidates += ", ";
        }
        candidates += fmt::format("#{} '{}'", i, candidate);
        if (NameEquals(const_cast<CKSTRING>(candidate.c_str()), name)) {
            if (seen == occurrence) {
                return i;
            }
            ++seen;
        }
    }

    error = fmt::format("Building Block '{}' could not resolve setting '{}' occurrence {} for layout preview. Candidate settings: {}.",
                        SafeString(behavior->GetPrototypeName()),
                        name,
                        occurrence,
                        candidates.empty() ? std::string("<none>") : candidates);
    return -1;
}

bool ResolvePreviewNamedSettings(CKBehavior *behavior,
                                 ScriptBridgeBBInvocationSpec &request,
                                 std::string &error) {
    if (request.NamedSettings.empty()) {
        return true;
    }
    for (const ScriptBridgeNamedValue &entry : request.NamedSettings) {
        const int index = FindPreviewSetting(behavior, entry.Name, entry.Occurrence, error);
        if (index < 0) {
            return false;
        }
        ScriptBridgeSetIndexedValue(request.IndexedSettings, index, entry.Value);
    }
    request.NamedSettings.clear();
    return true;
}

CKBeObject *ResolvePreviewBeObject(CKContext *context,
                                   CK_ID id,
                                   const ScriptBridgeObjectStamp &stamp,
                                   const char *role,
                                   std::string &error) {
    if (!id) {
        return nullptr;
    }
    CKObject *object = stamp.IsCaptured()
        ? GetStampedCKObjectById(context, id, stamp)
        : GetCKObjectById(context, id);
    if (!object) {
        error = fmt::format("{} object {} is no longer valid.", role ? role : "Building Block request", id);
        return nullptr;
    }
    CKBeObject *beObject = CKBeObject::Cast(object);
    if (!beObject) {
        error = fmt::format("{} object {} is no longer a CKBeObject.", role ? role : "Building Block request", id);
    }
    return beObject;
}

bool ConfigurePreviewOwnerAndTarget(CKContext *context,
                                    CKBehavior *behavior,
                                    const ScriptBridgeBBInvocationSpec &request,
                                    std::string &error) {
    if (!behavior) {
        error = "Behavior is not valid.";
        return false;
    }

    CKBeObject *owner = ResolvePreviewBeObject(context, request.OwnerId, request.OwnerStamp, "Owner", error);
    if (request.OwnerId && !owner) {
        return false;
    }
    CKBeObject *target = ResolvePreviewBeObject(context, request.TargetId, request.TargetStamp, "Target", error);
    if (request.TargetId && !target) {
        return false;
    }

    const CK_CLASSID compatibleClassId = behavior->GetCompatibleClassID();
    if (target) {
        if (!behavior->IsTargetable()) {
            error = fmt::format("Building Block '{}' is not targetable.", SafeString(behavior->GetPrototypeName()));
            return false;
        }
        if (!CKIsChildClassOf(target, compatibleClassId)) {
            error = fmt::format("Target '{}' is not compatible with Building Block '{}'.",
                                SafeString(target->GetName()),
                                SafeString(behavior->GetPrototypeName()));
            return false;
        }
        const CKERROR useTarget = behavior->UseTarget(TRUE);
        if (useTarget != CK_OK) {
            error = fmt::format("Failed to enable target parameter for Building Block '{}' (CKERROR {}).",
                                SafeString(behavior->GetPrototypeName()),
                                useTarget);
            return false;
        }
        CKParameterIn *targetParam = behavior->GetTargetParameter();
        if (!targetParam) {
            error = "Target parameter was not created.";
            return false;
        }
        CKParameterLocal *targetSource = behavior->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_Target"), targetParam->GetGUID());
        if (!targetSource) {
            error = "Failed to create target source parameter.";
            return false;
        }
        CK_ID targetId = target->GetID();
        if (targetSource->SetValue(&targetId, sizeof(targetId)) != CK_OK ||
            targetParam->SetDirectSource(targetSource) != CK_OK) {
            error = "Failed to set target parameter.";
            return false;
        }
    } else if (owner && !CKIsChildClassOf(owner, compatibleClassId)) {
        error = fmt::format("Owner '{}' is not compatible with Building Block '{}'.",
                            SafeString(owner->GetName()),
                            SafeString(behavior->GetPrototypeName()));
        return false;
    }

    const CKERROR ownerResult = behavior->SetOwner(owner, FALSE);
    if (ownerResult != CK_OK) {
        error = fmt::format("Failed to set Building Block owner (CKERROR {}).", ownerResult);
        return false;
    }
    return true;
}

void NormalizeConfiguredPrototypeLayout(ScriptBridgeLayoutRecord &layout,
                                        CKBehaviorPrototype *prototype) {
    layout.Prototype = true;
    layout.BehaviorId = 0;
    layout.BehaviorStamp = ScriptBridgeObjectStamp();
    layout.PrototypeGuid = prototype ? prototype->GetGuid() : CKGUID();
    if (prototype) {
        CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype, true);
        layout.Name = SafeString(prototype->GetName());
        layout.Category = decl ? SafeString(decl->GetCategory()) : std::string();
        layout.QualifiedName = layout.Category.empty() ? layout.Name : layout.Category + "/" + layout.Name;
        layout.BehaviorFlags = static_cast<CKDWORD>(prototype->GetBehaviorFlags());
        layout.PrototypeFlags = static_cast<CKDWORD>(prototype->GetFlags());
        layout.CompatibleClassId = decl ? decl->GetCompatibleClassId() : prototype->GetApplyToClassID();
        layout.NeededManagers.clear();
        if (decl) {
            layout.NeededManagers.reserve(decl->GetManagerNeededCount());
            for (int i = 0; i < decl->GetManagerNeededCount(); ++i) {
                layout.NeededManagers.push_back(decl->GetManagerNeeded(i));
            }
        }
    }

    auto clearParameterIds = [](std::vector<ScriptBridgeLayoutParamSlot> &slots) {
        for (ScriptBridgeLayoutParamSlot &slot : slots) {
            slot.ParameterId = 0;
        }
    };
    clearParameterIds(layout.Pins);
    clearParameterIds(layout.Pouts);
    clearParameterIds(layout.Settings);
    clearParameterIds(layout.Locals);
}

} // namespace

CKBehaviorPrototype *ScriptBehaviorBridge::ResolvePrototypeObject(const ScriptBridgeBBInvocationSpec &request, std::string &error) const {
    CKGUID guid;
    if (!ResolvePrototype(request, guid, error)) {
        return nullptr;
    }

    CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(guid);
    if (!prototype) {
        error = fmt::format("Building Block prototype {} was not found.", GuidToString(guid));
    }
    return prototype;
}

const ScriptBridgeLayoutRecord *ScriptBehaviorBridge::GetBehaviorLayout(CK_ID behaviorId, const ScriptBridgeObjectStamp &stamp) const {
    if (!m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }

    CKBehavior *behavior = CKBehavior::Cast(GetStampedCKObjectById(m_Manager->GetCKContext(), behaviorId, stamp));
    if (!behavior) {
        m_BehaviorLayouts.erase(behaviorId);
        return nullptr;
    }

    ScriptBridgeLayoutRecord fresh = BuildBehaviorLayout(behavior);
    auto it = m_BehaviorLayouts.find(behaviorId);
    if (it != m_BehaviorLayouts.end() && it->second.Signature == fresh.Signature) {
        return &it->second;
    }

    auto [inserted, unused] = m_BehaviorLayouts.insert_or_assign(behaviorId, std::move(fresh));
    (void) unused;
    return &inserted->second;
}

const ScriptBridgeLayoutRecord *ScriptBehaviorBridge::GetPrototypeLayout(const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request) const {
    std::string error;
    CKBehaviorPrototype *prototype = ResolvePrototypeObject(request, error);
    if (!prototype) {
        return nullptr;
    }

    const std::string key = PrototypeLayoutKey(prototype, request);
    ScriptBridgeLayoutRecord fresh = RequestNeedsConfiguredLayout(request)
        ? BuildConfiguredPrototypeLayout(ctx, prototype, request, error)
        : BuildPrototypeLayout(prototype);
    if (!error.empty()) {
        return nullptr;
    }
    auto it = m_PrototypeLayouts.find(key);
    if (it != m_PrototypeLayouts.end() && it->second.Signature == fresh.Signature) {
        return &it->second;
    }

    auto [inserted, unused] = m_PrototypeLayouts.insert_or_assign(key, std::move(fresh));
    (void) unused;
    return &inserted->second;
}

void ScriptBehaviorBridge::InvalidateBehaviorLayout(CK_ID behaviorId) const {
    m_BehaviorLayouts.erase(behaviorId);
}
ScriptBridgeLayoutRecord ScriptBehaviorBridge::BuildBehaviorLayout(CKBehavior *behavior) const {
    ScriptBridgeLayoutRecord layout;
    layout.Prototype = false;
    layout.BehaviorId = behavior ? behavior->GetID() : 0;
    layout.BehaviorStamp = CaptureBridgeObjectStamp(behavior);

    if (!behavior) {
        return layout;
    }

    CKContext *context = behavior->GetCKContext();
    layout.PrototypeGuid = behavior->GetPrototypeGuid();
    layout.Name = SafeString(behavior->GetPrototypeName());
    layout.QualifiedName = layout.Name;
    layout.BehaviorFlags = static_cast<CKDWORD>(behavior->GetFlags());
    layout.CompatibleClassId = behavior->GetCompatibleClassID();

    layout.Inputs.reserve(behavior->GetInputCount());
    for (int i = 0; i < behavior->GetInputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetInput(i);
        layout.Inputs.push_back(ScriptBridgeLayoutIoSlot{SafeString(io ? io->GetName() : nullptr)});
    }

    layout.Outputs.reserve(behavior->GetOutputCount());
    for (int i = 0; i < behavior->GetOutputCount(); ++i) {
        CKBehaviorIO *io = behavior->GetOutput(i);
        layout.Outputs.push_back(ScriptBridgeLayoutIoSlot{SafeString(io ? io->GetName() : nullptr)});
    }

    layout.Pins.reserve(behavior->GetInputParameterCount());
    for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
        CKParameterIn *pin = behavior->GetInputParameter(i);
        CKParameter *source = pin ? pin->GetRealSource() : nullptr;
        ScriptBridgeLayoutParamSlot slot;
        slot.Kind = ScriptBridgeSlotKind::Pin;
        slot.Index = i;
        slot.ParameterId = pin ? pin->GetID() : 0;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps,
                               ScriptBridgeSlotCaps::Dynamic,
                               BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETERINPUTS) ||
                                   BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS));
        slot.Name = SafeString(pin ? pin->GetName() : nullptr);
        slot.TypeGuid = pin ? pin->GetGUID() : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = source ? source->GetDataSize() : ParameterDefaultSize(context, slot.TypeGuid);
        layout.Pins.push_back(slot);
    }

    layout.Pouts.reserve(behavior->GetOutputParameterCount());
    for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
        CKParameterOut *param = behavior->GetOutputParameter(i);
        ScriptBridgeLayoutParamSlot slot;
        slot.Kind = ScriptBridgeSlotKind::Pout;
        slot.Index = i;
        slot.ParameterId = param ? param->GetID() : 0;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps,
                               ScriptBridgeSlotCaps::Dynamic,
                               BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETEROUTPUTS) ||
                                   BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
        slot.Name = SafeString(param ? param->GetName() : nullptr);
        slot.TypeGuid = param ? param->GetGUID() : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = param ? param->GetDataSize() : ParameterDefaultSize(context, slot.TypeGuid);
        layout.Pouts.push_back(slot);
    }

    layout.Locals.reserve(behavior->GetLocalParameterCount());
    layout.Settings.reserve(behavior->GetLocalParameterCount());
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *param = behavior->GetLocalParameter(i);
        ScriptBridgeLayoutParamSlot slot;
        const bool isSetting = behavior->IsLocalParameterSetting(i);
        const std::string name = SafeString(param ? param->GetName() : nullptr);
        if (!isSetting && IsBridgeScratchLocalName(name)) {
            continue;
        }
        slot.Kind = isSetting ? ScriptBridgeSlotKind::Setting : ScriptBridgeSlotKind::Local;
        slot.Index = i;
        slot.ParameterId = param ? param->GetID() : 0;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps, ScriptBridgeSlotCaps::Setting, isSetting);
        SetScriptBridgeSlotCap(slot.Caps, ScriptBridgeSlotCaps::Dynamic, true);
        slot.Name = name;
        slot.TypeGuid = param ? param->GetGUID() : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = param ? param->GetDataSize() : ParameterDefaultSize(context, slot.TypeGuid);
        if (isSetting) {
            layout.Settings.push_back(slot);
        } else {
            layout.Locals.push_back(slot);
        }
    }

    layout.Signature = LayoutSignature(layout);
    layout.LayoutGeneration = LayoutGenerationFromSignature(layout.Signature);
    return layout;
}

ScriptBridgeLayoutRecord ScriptBehaviorBridge::BuildPrototypeLayout(CKBehaviorPrototype *prototype) const {
    ScriptBridgeLayoutRecord layout;
    layout.Prototype = true;
    layout.PrototypeGuid = prototype ? prototype->GetGuid() : CKGUID();

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    if (!prototype) {
        return layout;
    }
    CKObjectDeclaration *decl = ResolvePrototypeDeclaration(prototype, true);
    layout.Name = SafeString(prototype->GetName());
    layout.Category = decl ? SafeString(decl->GetCategory()) : std::string();
    layout.QualifiedName = layout.Category.empty() ? layout.Name : layout.Category + "/" + layout.Name;
    layout.BehaviorFlags = static_cast<CKDWORD>(prototype->GetBehaviorFlags());
    layout.PrototypeFlags = static_cast<CKDWORD>(prototype->GetFlags());
    layout.CompatibleClassId = decl ? decl->GetCompatibleClassId() : prototype->GetApplyToClassID();
    if (decl) {
        layout.NeededManagers.reserve(decl->GetManagerNeededCount());
        for (int i = 0; i < decl->GetManagerNeededCount(); ++i) {
            layout.NeededManagers.push_back(decl->GetManagerNeeded(i));
        }
    }

    layout.Inputs.reserve(prototype->GetInputCount());
    for (int i = 0; i < prototype->GetInputCount(); ++i) {
        CKBEHAVIORIO_DESC *io = GetPrototypeInput(prototype, i);
        layout.Inputs.push_back(ScriptBridgeLayoutIoSlot{SafeString(io ? io->Name : nullptr)});
    }

    layout.Outputs.reserve(prototype->GetOutputCount());
    for (int i = 0; i < prototype->GetOutputCount(); ++i) {
        CKBEHAVIORIO_DESC *io = GetPrototypeOutput(prototype, i);
        layout.Outputs.push_back(ScriptBridgeLayoutIoSlot{SafeString(io ? io->Name : nullptr)});
    }

    layout.Pins.reserve(prototype->GetInParameterCount());
    for (int i = 0; i < prototype->GetInParameterCount(); ++i) {
        CKPARAMETER_DESC *param = GetPrototypeInputParameter(prototype, i);
        ScriptBridgeLayoutParamSlot slot;
        slot.Kind = ScriptBridgeSlotKind::Pin;
        slot.Index = i;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps,
                               ScriptBridgeSlotCaps::Dynamic,
                               BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETERINPUTS) ||
                                   BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS));
        slot.Name = SafeString(param ? param->Name : nullptr);
        slot.TypeGuid = param ? param->Guid : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = ParameterDefaultSize(context, slot.TypeGuid);
        layout.Pins.push_back(slot);
    }

    layout.Pouts.reserve(prototype->GetOutParameterCount());
    for (int i = 0; i < prototype->GetOutParameterCount(); ++i) {
        CKPARAMETER_DESC *param = GetPrototypeOutputParameter(prototype, i);
        ScriptBridgeLayoutParamSlot slot;
        slot.Kind = ScriptBridgeSlotKind::Pout;
        slot.Index = i;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps,
                               ScriptBridgeSlotCaps::Dynamic,
                               BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETEROUTPUTS) ||
                                   BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
        slot.Name = SafeString(param ? param->Name : nullptr);
        slot.TypeGuid = param ? param->Guid : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = ParameterDefaultSize(context, slot.TypeGuid);
        layout.Pouts.push_back(slot);
    }

    layout.Locals.reserve(prototype->GetLocalParameterCount());
    layout.Settings.reserve(prototype->GetLocalParameterCount());
    for (int i = 0; i < prototype->GetLocalParameterCount(); ++i) {
        CKPARAMETER_DESC *param = GetPrototypeLocalParameter(prototype, i);
        ScriptBridgeLayoutParamSlot slot;
        const bool isSetting = param && param->Type == 3;
        slot.Kind = isSetting ? ScriptBridgeSlotKind::Setting : ScriptBridgeSlotKind::Local;
        slot.Index = i;
        slot.Caps = 0;
        SetScriptBridgeSlotCap(slot.Caps, ScriptBridgeSlotCaps::Setting, isSetting);
        SetScriptBridgeSlotCap(slot.Caps,
                               ScriptBridgeSlotCaps::Dynamic,
                               BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
        slot.Name = SafeString(param ? param->Name : nullptr);
        slot.TypeGuid = param ? param->Guid : CKGUID();
        slot.TypeName = ParameterTypeLabel(context, slot.TypeGuid);
        slot.DataSize = ParameterDefaultSize(context, slot.TypeGuid);
        if (isSetting) {
            layout.Settings.push_back(slot);
        } else {
            layout.Locals.push_back(slot);
        }
    }

    layout.Signature = LayoutSignature(layout);
    layout.LayoutGeneration = LayoutGenerationFromSignature(layout.Signature);
    return layout;
}

ScriptBridgeLayoutRecord ScriptBehaviorBridge::BuildConfiguredPrototypeLayout(const CKBehaviorContext &ctx,
                                                                              CKBehaviorPrototype *prototype,
                                                                              const ScriptBridgeBBInvocationSpec &request,
                                                                              std::string &error) const {
    ScriptBridgeLayoutRecord layout;
    error.clear();
    if (!m_Manager || !m_Manager->GetCKContext()) {
        error = "Script manager or CKContext is not available.";
        return layout;
    }
    if (!prototype) {
        error = "Building Block prototype is not available.";
        return layout;
    }

    CKContext *context = m_Manager->GetCKContext();
    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR,
                                                                  const_cast<CKSTRING>("__CKAS_LayoutPreview"),
                                                                  CK_OBJECTCREATION_NONAMECHECK));
    if (!behavior) {
        error = "Failed to create layout preview Building Block behavior.";
        return layout;
    }

    CKBehaviorContext callbackContext = ctx;
    callbackContext.Context = callbackContext.Context ? callbackContext.Context : context;
    callbackContext.Behavior = behavior;
    callbackContext.ParameterManager = callbackContext.ParameterManager
        ? callbackContext.ParameterManager
        : context->GetParameterManager();
    callbackContext.MessageManager = callbackContext.MessageManager
        ? callbackContext.MessageManager
        : context->GetMessageManager();
    callbackContext.AttributeManager = callbackContext.AttributeManager
        ? callbackContext.AttributeManager
        : context->GetAttributeManager();
    callbackContext.TimeManager = callbackContext.TimeManager
        ? callbackContext.TimeManager
        : context->GetTimeManager();

    bool createCallbackSent = false;
    auto fail = [&](const std::string &message) -> ScriptBridgeLayoutRecord {
        error = message;
        if (createCallbackSent) {
            if (behavior->GetOwner()) {
                CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDETACH, &callbackContext);
            }
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &callbackContext);
        }
        context->DestroyObject(behavior);
        return ScriptBridgeLayoutRecord();
    };

    behavior->UseFunction();
    const CKGUID guid = prototype->GetGuid();
    CKERROR err = behavior->InitFromPrototype(prototype);
    if (err != CK_OK) {
        return fail(fmt::format("Failed to initialize layout preview Building Block from GUID ({}, {}) with CKERROR {}.",
                                guid.d[0],
                                guid.d[1],
                                err));
    }
    if (!EnsurePrototypeStaticLayout(behavior, prototype, error)) {
        return fail(error);
    }

    ScriptBridgeBBInvocationSpec resolvedRequest = request;
    if (!ResolvePreviewNamedSettings(behavior, resolvedRequest, error)) {
        return fail(error);
    }
    if (!ApplyPreviewIndexedSettings(behavior, resolvedRequest.IndexedSettings, error)) {
        return fail(error);
    }
    if (!ConfigurePreviewOwnerAndTarget(context, behavior, resolvedRequest, error)) {
        return fail(error);
    }

    err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &callbackContext);
    if (err != CK_OK) {
        return fail(fmt::format("Building Block CREATE callback failed during layout preview (CKERROR {}).", err));
    }
    createCallbackSent = true;

    if (behavior->GetOwner()) {
        err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORATTACH, &callbackContext);
        if (err != CK_OK) {
            return fail(fmt::format("Building Block ATTACH callback failed during layout preview (CKERROR {}).", err));
        }
    }

    if (!resolvedRequest.IndexedSettings.empty()) {
        err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &callbackContext);
        if (err != CK_OK) {
            return fail(fmt::format("Building Block SETTINGSEDITED callback failed during layout preview (CKERROR {}).", err));
        }
        if (!EnsurePrototypeStaticLayout(behavior, prototype, error)) {
            return fail(error);
        }
    }

    layout = BuildBehaviorLayout(behavior);
    NormalizeConfiguredPrototypeLayout(layout, prototype);
    layout.Signature = LayoutSignature(layout);
    layout.LayoutGeneration = LayoutGenerationFromSignature(layout.Signature);

    if (behavior->GetOwner()) {
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDETACH, &callbackContext);
    }
    CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &callbackContext);
    context->DestroyObject(behavior);
    return layout;
}

std::string ScriptBehaviorBridge::PrototypeLayoutKey(CKBehaviorPrototype *prototype,
                                                     const ScriptBridgeBBInvocationSpec &request) const {
    std::string key = prototype ? LayoutGuidKey(prototype->GetGuid()) : std::string("<null>");
    if (!RequestNeedsConfiguredLayout(request)) {
        return key;
    }

    key += "|configured";
    key += fmt::format("|owner:{}:{}|target:{}:{}",
                       request.OwnerId,
                       LayoutStampKey(request.OwnerStamp),
                       request.TargetId,
                       LayoutStampKey(request.TargetStamp));
    for (const ScriptBridgeIndexedValue &entry : request.IndexedSettings) {
        key += fmt::format("|setting#{}:", entry.PinIndex);
        AppendValueKey(key, entry.Value);
    }
    for (const ScriptBridgeNamedValue &entry : request.NamedSettings) {
        key += fmt::format("|setting:{}#{}:", entry.Name, entry.Occurrence);
        AppendValueKey(key, entry.Value);
    }
    return key;
}

std::string ScriptBehaviorBridge::LayoutSignature(const ScriptBridgeLayoutRecord &layout) const {
    std::string signature;
    AppendLayoutSignature(signature, layout.Prototype ? "prototype" : "behavior");
    AppendLayoutSignature(signature, layout.PrototypeGuid);
    AppendLayoutSignature(signature, fmt::format("behaviorFlags:{} prototypeFlags:{} compatible:{}",
        layout.BehaviorFlags,
        layout.PrototypeFlags,
        layout.CompatibleClassId));
    AppendLayoutSignature(signature, fmt::format("in:{}", layout.Inputs.size()));
    for (const auto &slot : layout.Inputs) {
        AppendLayoutSignature(signature, slot.Name);
    }
    AppendLayoutSignature(signature, fmt::format("out:{}", layout.Outputs.size()));
    for (const auto &slot : layout.Outputs) {
        AppendLayoutSignature(signature, slot.Name);
    }

    auto appendParams = [&](const char *label, const std::vector<ScriptBridgeLayoutParamSlot> &slots) {
        AppendLayoutSignature(signature, fmt::format("{}:{}", label, slots.size()));
        for (const auto &slot : slots) {
            AppendLayoutSignature(signature, fmt::format("{}:{}:{}:{}", slot.Index, slot.ParameterId, slot.Caps, slot.Name));
            AppendLayoutSignature(signature, slot.TypeGuid);
        }
    };
    appendParams("pin", layout.Pins);
    appendParams("pout", layout.Pouts);
    appendParams("setting", layout.Settings);
    appendParams("local", layout.Locals);
    return signature;
}
