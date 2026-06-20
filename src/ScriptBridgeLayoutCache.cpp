#include "ScriptBridgeHandles.h"

#include <functional>
#include <limits.h>

#include <fmt/format.h>

namespace ScriptBehaviorBridgeInternal {

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

bool IsBridgeInternalLocalName(const std::string &name) {
    return name.find("__CKAS_BridgeInput_") == 0 ||
           name.find("__CKAS_BridgeOutput_") == 0 ||
           name.find("__CKAS_GraphEditInput_") == 0 ||
           name.find("__CKAS_Op") == 0 ||
           name == "__CKAS_GraphEdit_Target" ||
           name == "__CKAS_Target";
}

} // namespace ScriptBehaviorBridgeInternal

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
    (void) ctx;
    std::string error;
    CKBehaviorPrototype *prototype = ResolvePrototypeObject(request, error);
    if (!prototype) {
        return nullptr;
    }

    const std::string key = ScriptBehaviorBridgeInternal::LayoutGuidKey(prototype->GetGuid());
    ScriptBridgeLayoutRecord fresh = BuildPrototypeLayout(prototype);
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
                               ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETERINPUTS) ||
                                   ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS));
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
                               ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETEROUTPUTS) ||
                                   ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
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
        if (!isSetting && ScriptBehaviorBridgeInternal::IsBridgeInternalLocalName(name)) {
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
    layout.LayoutGeneration = ScriptBehaviorBridgeInternal::LayoutGenerationFromSignature(layout.Signature);
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
                               ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETERINPUTS) ||
                                   ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS));
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
                               ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_VARIABLEPARAMETEROUTPUTS) ||
                                   ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
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
                               ScriptBehaviorBridgeInternal::BehaviorFlagSet(layout.BehaviorFlags, CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
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
    layout.LayoutGeneration = ScriptBehaviorBridgeInternal::LayoutGenerationFromSignature(layout.Signature);
    return layout;
}

std::string ScriptBehaviorBridge::LayoutSignature(const ScriptBridgeLayoutRecord &layout) const {
    std::string signature;
    ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, layout.Prototype ? "prototype" : "behavior");
    ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, layout.PrototypeGuid);
    ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, fmt::format("behaviorFlags:{} prototypeFlags:{} compatible:{}",
        layout.BehaviorFlags,
        layout.PrototypeFlags,
        layout.CompatibleClassId));
    ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, fmt::format("in:{}", layout.Inputs.size()));
    for (const auto &slot : layout.Inputs) {
        ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, slot.Name);
    }
    ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, fmt::format("out:{}", layout.Outputs.size()));
    for (const auto &slot : layout.Outputs) {
        ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, slot.Name);
    }

    auto appendParams = [&](const char *label, const std::vector<ScriptBridgeLayoutParamSlot> &slots) {
        ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, fmt::format("{}:{}", label, slots.size()));
        for (const auto &slot : slots) {
            ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, fmt::format("{}:{}:{}:{}", slot.Index, slot.ParameterId, slot.Caps, slot.Name));
            ScriptBehaviorBridgeInternal::AppendLayoutSignature(signature, slot.TypeGuid);
        }
    };
    appendParams("pin", layout.Pins);
    appendParams("pout", layout.Pouts);
    appendParams("setting", layout.Settings);
    appendParams("local", layout.Locals);
    return signature;
}
