#ifndef CK_SCRIPTBRIDGEHANDLES_H
#define CK_SCRIPTBRIDGEHANDLES_H

#include "ScriptBridgeCommon.h"
#include "ScriptBridgeParameterIO.h"
#include "ScriptNativeBuffer.h"
#include "ScriptParameterRegistry.h"

class ParamInfo final : public RefCounted {
public:
    ParamInfo(ScriptBridgeSlotKind kind,
              int index,
              const std::string &name,
              CKGUID typeGuid,
              const std::string &typeName,
              int dataSize)
        : m_Kind(kind),
          m_Index(index),
          m_Name(name),
          m_TypeGuid(typeGuid),
          m_TypeName(typeName),
          m_DataSize(dataSize) {}

    int GetKind() const { return static_cast<int>(m_Kind); }
    int GetIndex() const { return m_Index; }
    std::string GetName() const { return m_Name; }
    CKGUID GetTypeGuid() const { return m_TypeGuid; }
    std::string GetTypeName() const { return m_TypeName; }
    int GetDataSize() const { return m_DataSize; }

    std::string Describe() const {
        return fmt::format("#{} {}: {} size={}", m_Index, m_Name, m_TypeName, m_DataSize);
    }

private:
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    std::string m_Name;
    CKGUID m_TypeGuid;
    std::string m_TypeName;
    int m_DataSize = 0;
};

class ParamStructValue;

class ParamValue final : public RefCounted {
public:
    explicit ParamValue(const ScriptParamValue &value)
        : m_Value(value) {}

    const ScriptParamValue &Value() const { return m_Value; }

    bool IsValid() const { return m_Value.Kind != ScriptParamValueKind::Empty; }
    CKGUID TypeGuid() const {
        return m_Value.TypeGuid.IsValid() ? m_Value.TypeGuid : ScriptParameterGuidForValue(m_Value);
    }
    std::string TypeName() const { return TypeGuid().IsValid() ? GuidToString(TypeGuid()) : std::string(); }

    int AsInt() const {
        if (m_Value.Kind == ScriptParamValueKind::Int) return m_Value.Data.IntValue;
        if (m_Value.Kind == ScriptParamValueKind::Enum || m_Value.Kind == ScriptParamValueKind::Flags) return static_cast<int>(m_Value.Data.DwordValue);
        ReportWrongKind("AsInt", "int/enum/flags");
        return 0;
    }

    float AsFloat() const {
        if (m_Value.Kind == ScriptParamValueKind::Float) return m_Value.Data.FloatValue;
        if (m_Value.Kind == ScriptParamValueKind::Int) return static_cast<float>(m_Value.Data.IntValue);
        ReportWrongKind("AsFloat", "float/int");
        return 0.0f;
    }

    bool AsBool() const {
        if (m_Value.Kind == ScriptParamValueKind::Bool) return m_Value.Data.BoolValue;
        ReportWrongKind("AsBool", "bool");
        return false;
    }

    std::string AsString() const {
        if (m_Value.Kind == ScriptParamValueKind::String || m_Value.Kind == ScriptParamValueKind::Text) return m_Value.Text();
        ReportWrongKind("AsString", "string/text");
        return std::string();
    }

    CKGUID AsGuid() const {
        if (m_Value.Kind == ScriptParamValueKind::Guid) return m_Value.Data.GuidValue;
        ReportWrongKind("AsGuid", "CKGUID");
        return CKGUID();
    }

    VxVector AsVector() const {
        if (m_Value.Kind == ScriptParamValueKind::Vector) return m_Value.Data.VectorValue;
        ReportWrongKind("AsVector", "VxVector");
        return VxVector();
    }

    Vx2DVector AsVector2() const {
        if (m_Value.Kind == ScriptParamValueKind::Vector2) return m_Value.Data.Vector2Value;
        ReportWrongKind("AsVector2", "Vx2DVector");
        return Vx2DVector();
    }

    VxColor AsColor() const {
        if (m_Value.Kind == ScriptParamValueKind::Color) return m_Value.Data.ColorValue;
        ReportWrongKind("AsColor", "VxColor");
        return VxColor();
    }

    VxQuaternion AsQuaternion() const {
        if (m_Value.Kind == ScriptParamValueKind::Quaternion) return m_Value.Data.QuaternionValue;
        ReportWrongKind("AsQuaternion", "VxQuaternion");
        return VxQuaternion();
    }

    VxMatrix AsMatrix() const {
        if (m_Value.Kind == ScriptParamValueKind::Matrix) return m_Value.Data.MatrixValue;
        ReportWrongKind("AsMatrix", "VxMatrix");
        return VxMatrix();
    }
    ParamStructValue *AsStruct() const;

    std::string AsText() const {
        return ScriptParamValueToText(m_Value);
    }

    NativeBuffer *AsRaw() const {
        if (m_Value.Kind != ScriptParamValueKind::Raw || m_Value.RawBytes().empty()) {
            if (m_Value.Kind != ScriptParamValueKind::Raw) {
                ReportWrongKind("AsRaw", "raw");
            }
            return NativeBuffer::Create(0);
        }
        NativeBuffer *buffer = NativeBuffer::Create(m_Value.RawBytes().size());
        buffer->Write(const_cast<char *>(m_Value.RawBytes().data()), m_Value.RawBytes().size());
        buffer->Reset();
        return buffer;
    }

private:
    void ReportWrongKind(const char *method, const char *expected) const {
        SetScriptException(fmt::format("ParamValue.{} expected {}, got {}.",
                                       method,
                                       expected,
                                       ScriptParamValueKindName(m_Value.Kind)));
    }

    ScriptParamValue m_Value;
};

class ParamStructValue final : public RefCounted {
public:
    explicit ParamStructValue(const ScriptParamValue &value)
        : m_Value(value) {}

    bool IsValid() const { return m_Value.Kind == ScriptParamValueKind::Struct && m_Value.TypeGuid.IsValid(); }
    CKGUID TypeGuid() const { return m_Value.TypeGuid; }
    std::string TypeName() const { return m_Value.TypeGuid.IsValid() ? GuidToString(m_Value.TypeGuid) : std::string(); }

    ParamStructValue *Set(int index, ParamValue *value) {
        if (index < 0 || !value || !value->IsValid()) {
            SetScriptException("ParamStructValue.Set requires a valid member index and ParamValue.");
            return nullptr;
        }
        std::vector<ScriptParamStructMemberValue> &members = m_Value.MutableStructMembers();
        auto it = std::lower_bound(members.begin(), members.end(), index,
            [](const ScriptParamStructMemberValue &member, int memberIndex) {
                return member.Index < memberIndex;
            });
        if (it != members.end() && it->Index == index) {
            delete it->Value;
            it->Value = new ScriptParamValue(value->Value());
            AddRef();
            return this;
        }
        ScriptParamStructMemberValue member;
        member.Index = index;
        member.Value = new ScriptParamValue(value->Value());
        members.insert(it, std::move(member));
        AddRef();
        return this;
    }

    ParamValue *Value() const {
        return new ParamValue(m_Value);
    }

    ParamValue *AsValue() const {
        return Value();
    }

    std::string Describe() const {
        return fmt::format("ParamStructValue type={} members={}",
                           m_Value.TypeGuid.IsValid() ? GuidToString(m_Value.TypeGuid) : std::string("<unknown>"),
                           m_Value.StructMembers().size());
    }

private:
    ScriptParamValue m_Value;
};

inline ParamStructValue *ParamValue::AsStruct() const {
    if (m_Value.Kind != ScriptParamValueKind::Struct) {
        ReportWrongKind("AsStruct", "struct");
        return nullptr;
    }
    return new ParamStructValue(m_Value);
}

class ParamStructRef final : public RefCounted {
public:
    ParamStructRef(ScriptBehaviorBridge *bridge, CK_ID parameterId)
        : m_Bridge(bridge), m_ParameterId(parameterId) {
        m_Stamp = CaptureBridgeObjectStamp(RawGet());
    }

    CKParameter *Get() const {
        return CKParameter::Cast(RawGetStamped());
    }

private:
    CKObject *RawGet() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetCKObjectById(context, m_ParameterId);
    }

    CKObject *RawGetStamped() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetStampedCKObjectById(context, m_ParameterId, m_Stamp);
    }

public:

    bool IsValid() const {
        CKParameter *param = Get();
        ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
        const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
        return record && record->Has(ScriptParamTypeCaps::StructLike);
    }

    int Count() const {
        CKParameter *param = Get();
        ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
        const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
        return record && record->Has(ScriptParamTypeCaps::StructLike) ? static_cast<int>(record->StructMembers.size()) : 0;
    }

    ParamStructInfo *Info() const {
        CKParameter *param = Get();
        ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
        const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
        return record && record->Has(ScriptParamTypeCaps::StructLike) ? new ParamStructInfo(registry, record->Type) : nullptr;
    }

    ParamRef *Member(int index) const;

    int FindMember(const std::string &name, int occurrence = 0) const {
        ParamStructInfo *info = Info();
        if (!info) {
            return -1;
        }
        const int result = info->FindMember(name, occurrence);
        info->Release();
        return result;
    }

    std::string Describe() const {
        ParamStructInfo *info = Info();
        if (!info) {
            return "ParamStructRef is not valid.";
        }
        std::string result = info->Describe();
        info->Release();
        return result;
    }

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ParameterId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

class ParamSourceLinkRef final : public RefCounted {
public:
    ParamSourceLinkRef(ScriptBehaviorBridge *bridge,
                       CKParameterIn *target,
                       CKParameter *previousSource,
                       CKParameter *installedSource)
        : m_Bridge(bridge),
          m_TargetId(target ? target->GetID() : 0),
          m_PreviousSourceId(previousSource ? previousSource->GetID() : 0),
          m_InstalledSourceId(installedSource ? installedSource->GetID() : 0),
          m_TargetStamp(CaptureBridgeObjectStamp(target)),
          m_PreviousSourceStamp(CaptureBridgeObjectStamp(previousSource)),
          m_InstalledSourceStamp(CaptureBridgeObjectStamp(installedSource)) {}

    ~ParamSourceLinkRef() override {
        std::string error;
        (void) RestoreInternal(error, false);
    }

    bool IsValid() const {
        return Target() != nullptr && (m_InstalledSourceId == 0 || InstalledSource() != nullptr);
    }

    bool IsCommitted() const { return m_Committed; }
    bool IsRestored() const { return m_Restored; }

    bool Commit() {
        m_Committed = true;
        return true;
    }

    bool Restore() {
        std::string error;
        if (!RestoreInternal(error, true)) {
            SetScriptException(error.empty() ? "Failed to restore parameter source." : error);
            return false;
        }
        return true;
    }

    std::string Describe() const {
        CKParameterIn *target = Target();
        return fmt::format("ParamSourceLink target={} previous={} installed={} committed={} restored={}",
                           target ? SafeString(target->GetName()) : std::string("<invalid>"),
                           m_PreviousSourceId,
                           m_InstalledSourceId,
                           m_Committed ? "true" : "false",
                           m_Restored ? "true" : "false");
    }

private:
    CKContext *Context() const {
        return m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    }

    CKParameterIn *Target() const {
        return CKParameterIn::Cast(GetStampedCKObjectById(Context(), m_TargetId, m_TargetStamp));
    }

    CKParameter *PreviousSource() const {
        if (!m_PreviousSourceId) {
            return nullptr;
        }
        return ParameterSourceForConnection(GetStampedCKObjectById(Context(), m_PreviousSourceId, m_PreviousSourceStamp));
    }

    CKParameter *InstalledSource() const {
        if (!m_InstalledSourceId) {
            return nullptr;
        }
        return ParameterSourceForConnection(GetStampedCKObjectById(Context(), m_InstalledSourceId, m_InstalledSourceStamp));
    }

    bool RestoreInternal(std::string &error, bool explicitCall) {
        if (m_Committed || m_Restored) {
            return true;
        }

        CKParameterIn *target = Target();
        if (!target) {
            if (explicitCall) {
                error = fmt::format("Parameter source link target id={} is no longer valid.", m_TargetId);
                return false;
            }
            m_Restored = true;
            return true;
        }

        CKParameter *installed = InstalledSource();
        CKParameter *current = target->GetDirectSource();
        if (m_InstalledSourceId && current && installed && current->GetID() != installed->GetID()) {
            if (explicitCall) {
                error = fmt::format("Parameter source link for '{}' was already changed by another graph edit.",
                                    SafeString(target->GetName()));
                return false;
            }
            m_Restored = true;
            return true;
        }

        CKParameter *previous = PreviousSource();
        if (m_PreviousSourceId && !previous) {
            if (explicitCall) {
                error = fmt::format("Previous source id={} for '{}' is no longer valid.",
                                    m_PreviousSourceId,
                                    SafeString(target->GetName()));
                return false;
            }
            m_Restored = true;
            return true;
        }

        const CKERROR err = target->SetDirectSource(previous);
        if (err != CK_OK) {
            error = fmt::format("Failed to restore source for '{}' (CKERROR {}).",
                                SafeString(target->GetName()),
                                err);
            return false;
        }

        m_Restored = true;
        return true;
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_TargetId = 0;
    CK_ID m_PreviousSourceId = 0;
    CK_ID m_InstalledSourceId = 0;
    ScriptBridgeObjectStamp m_TargetStamp;
    ScriptBridgeObjectStamp m_PreviousSourceStamp;
    ScriptBridgeObjectStamp m_InstalledSourceStamp;
    bool m_Committed = false;
    bool m_Restored = false;
};

class BehaviorLayout final : public RefCounted {
public:
    BehaviorLayout(ScriptBehaviorBridge *bridge, CK_ID behaviorId)
        : m_Bridge(bridge), m_BehaviorId(behaviorId), m_IsPrototype(false) {
        m_BehaviorStamp = CaptureBridgeObjectStamp(RawBehavior());
    }

    BehaviorLayout(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request)
        : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_IsPrototype(true) {}

    int InputCount() const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout ? static_cast<int>(layout->Inputs.size()) : 0;
    }

    int OutputCount() const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout ? static_cast<int>(layout->Outputs.size()) : 0;
    }

    int PinCount() const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout ? static_cast<int>(layout->Pins.size()) : 0;
    }

    int PoutCount() const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout ? static_cast<int>(layout->Pouts.size()) : 0;
    }

    int LocalCount() const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout ? static_cast<int>(layout->Locals.size()) : 0;
    }

    std::string InputName(int index) const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout && index >= 0 && index < static_cast<int>(layout->Inputs.size()) ? layout->Inputs[index].Name : std::string();
    }

    std::string OutputNameAt(int index) const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        return layout && index >= 0 && index < static_cast<int>(layout->Outputs.size()) ? layout->Outputs[index].Name : std::string();
    }

    ParamInfo *Pin(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Pin, index); }
    ParamInfo *Pout(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Pout, index); }
    ParamInfo *Local(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Local, index); }

    int FindInput(const std::string &name, int occurrence) const { return FindIo(true, name, occurrence); }
    int FindOutput(const std::string &name, int occurrence) const { return FindIo(false, name, occurrence); }
    int FindPin(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Pin, name, occurrence); }
    int FindPout(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Pout, name, occurrence); }
    int FindLocal(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Local, name, occurrence); }

    std::string Describe() const {
        std::string text;
        text += "Inputs:";
        for (int i = 0; i < InputCount(); ++i) text += fmt::format("\n  #{} {}", i, InputName(i));
        text += "\nOutputs:";
        for (int i = 0; i < OutputCount(); ++i) text += fmt::format("\n  #{} {}", i, OutputNameAt(i));
        text += "\nPins:";
        for (int i = 0; i < PinCount(); ++i) {
            ParamInfo *info = Pin(i);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", i));
            if (info) info->Release();
        }
        text += "\nPouts:";
        for (int i = 0; i < PoutCount(); ++i) {
            ParamInfo *info = Pout(i);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", i));
            if (info) info->Release();
        }
        text += "\nLocals:";
        for (int i = 0; i < LocalCount(); ++i) {
            ParamInfo *info = Local(i);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", i));
            if (info) info->Release();
        }
        return text;
    }

private:
    CKContext *Context() const {
        if (m_Context.Context) return m_Context.Context;
        return m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    }

    CKObject *RawBehavior() const {
        if (m_IsPrototype || !m_Bridge || !m_Bridge->GetManager()) return nullptr;
        return GetCKObjectById(m_Bridge->GetManager()->GetCKContext(), m_BehaviorId);
    }

    const ScriptBridgeLayoutRecord *LayoutRecord() const {
        if (!m_Bridge) return nullptr;
        return m_IsPrototype
            ? m_Bridge->GetPrototypeLayout(m_Context, m_Request)
            : m_Bridge->GetBehaviorLayout(m_BehaviorId, m_BehaviorStamp);
    }

    ParamInfo *ParameterInfo(ScriptBridgeSlotKind kind, int index) const {
        const ScriptBridgeLayoutRecord *layout = LayoutRecord();
        if (!layout) return nullptr;
        const std::vector<ScriptBridgeLayoutParamSlot> *slots = nullptr;
        if (kind == ScriptBridgeSlotKind::Pin) {
            slots = &layout->Pins;
        } else if (kind == ScriptBridgeSlotKind::Pout) {
            slots = &layout->Pouts;
        } else if (kind == ScriptBridgeSlotKind::Local) {
            slots = &layout->Locals;
        }
        if (!slots || index < 0 || index >= static_cast<int>(slots->size())) return nullptr;
        const ScriptBridgeLayoutParamSlot &slot = (*slots)[index];
        return new ParamInfo(kind, index, slot.Name, slot.TypeGuid, slot.TypeName, slot.DataSize);
    }

    int FindIo(bool input, const std::string &name, int occurrence) const {
        if (occurrence < 0) return -1;
        int seen = 0;
        const int count = input ? InputCount() : OutputCount();
        for (int i = 0; i < count; ++i) {
            const std::string itemName = input ? InputName(i) : OutputNameAt(i);
            if (itemName == name) {
                if (seen == occurrence) return i;
                ++seen;
            }
        }
        return -1;
    }

    int FindParameter(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
        if (occurrence < 0) return -1;
        const int count = kind == ScriptBridgeSlotKind::Pin ? PinCount() : (kind == ScriptBridgeSlotKind::Pout ? PoutCount() : LocalCount());
        int seen = 0;
        for (int i = 0; i < count; ++i) {
            ParamInfo *info = ParameterInfo(kind, i);
            const bool match = info && info->GetName() == name;
            if (info) info->Release();
            if (match) {
                if (seen == occurrence) return i;
                ++seen;
            }
        }
        return -1;
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_BehaviorId = 0;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    ScriptBridgeObjectStamp m_BehaviorStamp;
    bool m_IsPrototype = false;
};

class ParamRef final : public RefCounted {
public:
    ParamRef(ScriptBehaviorBridge *bridge,
             CK_ID parameterId,
             ScriptBridgeSlotKind kind = ScriptBridgeSlotKind::Standalone,
             int index = -1,
             CK_ID ownerBehaviorId = 0)
        : m_Bridge(bridge),
          m_ParameterId(parameterId),
          m_Kind(kind),
          m_Index(index),
          m_OwnerBehaviorId(ownerBehaviorId) {
        m_Stamp = CaptureBridgeObjectStamp(RawGet());
    }

    CKObject *Get() const {
        return RawGetStamped();
    }

    const ScriptBridgeObjectStamp &Stamp() const { return m_Stamp; }

    CKParameter *Source() const { return ParameterSourceForConnection(Get()); }

    bool IsValid() const {
        CKObject *parameter = Get();
        return CKParameterIn::Cast(parameter) != nullptr || CKParameter::Cast(parameter) != nullptr;
    }

    CK_ID GetID() const { return m_ParameterId; }
    int GetIndex() const { return m_Index; }
    int GetKind() const { return static_cast<int>(m_Kind); }

    std::string GetName() const {
        CKObject *parameter = Get();
        return parameter ? SafeString(parameter->GetName()) : std::string();
    }

    CKGUID TypeGuid() const {
        CKObject *parameter = Get();
        if (CKParameterIn *input = CKParameterIn::Cast(parameter)) return input->GetGUID();
        if (CKParameter *param = CKParameter::Cast(parameter)) return param->GetGUID();
        return CKGUID();
    }

    std::string TypeName() const {
        CKObject *parameter = Get();
        return ParameterTypeLabel(parameter ? parameter->GetCKContext() : nullptr, TypeGuid());
    }

    int DataSize() const {
        CKObject *parameter = Get();
        if (CKParameterIn *input = CKParameterIn::Cast(parameter)) {
            CKParameter *source = input->GetRealSource();
            return source ? source->GetDataSize() : ParameterDefaultSize(input->GetCKContext(), input->GetGUID());
        }
        if (CKParameter *param = CKParameter::Cast(parameter)) return param->GetDataSize();
        return 0;
    }

    ParamRef *RealSource() const {
        CKParameter *source = Source();
        return m_Bridge && source ? m_Bridge->WrapParameter(source, ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    }

    ParamRef *DirectSource() const {
        CKParameterIn *input = CKParameterIn::Cast(Get());
        CKParameter *source = input ? input->GetDirectSource() : nullptr;
        return m_Bridge && source ? m_Bridge->WrapParameter(source, ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    }

    ParamSourceLinkRef *SetSourceScoped(ParamRef *sourceRef) {
        CKParameterIn *input = CKParameterIn::Cast(Get());
        CKParameter *source = sourceRef ? sourceRef->Source() : nullptr;
        if (!input || !source) {
            SetScriptException("SetSourceScoped requires an input parameter and a valid source parameter.");
            return nullptr;
        }

        CKParameter *previous = input->GetDirectSource();
        CKERROR err = input->SetDirectSource(source);
        if (err != CK_OK) {
            SetScriptException(fmt::format("Failed to set scoped source for '{}' (expected {}, got {}, CKERROR {}).",
                SafeString(input->GetName()),
                ParameterTypeLabel(input->GetCKContext(), input->GetGUID()),
                ParameterTypeLabel(input->GetCKContext(), source->GetGUID()),
                err));
            return nullptr;
        }

        return new ParamSourceLinkRef(m_Bridge, input, previous, source);
    }

    bool SetSource(ParamRef *sourceRef) {
        CKParameterIn *input = CKParameterIn::Cast(Get());
        CKParameter *source = sourceRef ? sourceRef->Source() : nullptr;
        if (!input || !source) {
            SetScriptException("SetSource requires an input parameter and a valid source parameter.");
            return false;
        }
        CKERROR err = input->SetDirectSource(source);
        if (err != CK_OK) {
            SetScriptException(fmt::format("Failed to set source for '{}' (expected {}, got {}, CKERROR {}).",
                SafeString(input->GetName()),
                ParameterTypeLabel(input->GetCKContext(), input->GetGUID()),
                ParameterTypeLabel(input->GetCKContext(), source->GetGUID()),
                err));
            return false;
        }
        return true;
    }

    bool Set(ParamValue *value) {
        if (!value) {
            SetScriptException("ParamValue is null.");
            return false;
        }
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        CKERROR err = SetBridgeParamValue(target, value->Value(), error);
        if (err != CK_OK) {
            SetScriptException(error.empty() ? fmt::format("Failed to set parameter (CKERROR {}).", err) : error);
            return false;
        }
        return true;
    }

    bool SetInt(int value) { return SetDirectValue(MakeScriptParamInt(value)); }
    bool SetFloat(float value) { return SetDirectValue(MakeScriptParamFloat(value)); }
    bool SetBool(bool value) { return SetDirectValue(MakeScriptParamBool(value)); }
    bool SetString(const std::string &value) { return SetDirectValue(MakeScriptParamString(value)); }
    bool SetObject(CKObject *value) { return SetDirectValue(MakeScriptParamObject(value)); }
    bool SetEnum(const std::string &nameOrValue) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(target->GetCKContext());
        int value = 0;
        if (!registry || !registry->ParseEnumValue(target->GetGUID(), nameOrValue, value, error)) {
            SetScriptException(error.empty() ? "Failed to resolve enum value." : error);
            return false;
        }
        return SetDirectValue(MakeScriptParamEnum(target->GetGUID(), TypeName(), static_cast<CKDWORD>(value)));
    }
    bool SetEnumInt(int value) {
        return SetDirectValue(MakeScriptParamEnum(TypeGuid(), TypeName(), static_cast<CKDWORD>(value)));
    }
    bool SetFlags(const std::string &namesOrMask) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(target->GetCKContext());
        CKDWORD value = 0;
        if (!registry || !registry->ParseFlagsValue(target->GetGUID(), namesOrMask, value, error)) {
            SetScriptException(error.empty() ? "Failed to resolve flags value." : error);
            return false;
        }
        return SetDirectValue(MakeScriptParamFlags(target->GetGUID(), TypeName(), value));
    }
    bool SetFlagsMask(CKDWORD value) {
        return SetDirectValue(MakeScriptParamFlags(TypeGuid(), TypeName(), value));
    }
    bool SetStruct(ParamStructValue *value) {
        if (!value) {
            SetScriptException("ParamStructValue is null.");
            return false;
        }
        ParamValue *asValue = value->Value();
        const bool result = Set(asValue);
        asValue->Release();
        return result;
    }

    ParamValue *GetValue() const {
        CKParameter *source = Source();
        if (!source) {
            return new ParamValue(ScriptParamValue());
        }
        std::string error;
        ScriptParamValue value = ReadParameterValue(source, &error);
        value.TypeGuid = source->GetGUID();
        value.Type = source->GetType();
        return new ParamValue(value);
    }

    bool CopyFrom(ParamRef *sourceRef) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        CKParameter *source = sourceRef ? sourceRef->Source() : nullptr;
        if (!target || !source) {
            SetScriptException(!error.empty() ? error : "CopyFrom requires a valid source parameter.");
            return false;
        }
        CKERROR err = CopyParameterValue(target, source, error);
        if (err != CK_OK) {
            SetScriptException(error.empty() ? fmt::format("Failed to copy parameter value (CKERROR {}).", err) : error);
            return false;
        }
        return true;
    }

    std::string GetText() const {
        std::string text;
        if (ReadParameterText(Source(), text)) {
            return text;
        }
        return std::string();
    }

    std::string GetEnumText() const {
        CKParameter *source = Source();
        ScriptParameterRegistry *registry = source ? ScriptParameterRegistry::FromContext(source->GetCKContext()) : nullptr;
        CKDWORD value = 0;
        if (!source || !registry || source->GetValue(&value, TRUE) != CK_OK) {
            return std::string();
        }
        std::string text;
        return registry->EnumNameOf(source->GetGUID(), static_cast<int>(value), text) ? text : std::to_string(value);
    }

    std::string GetFlagsText() const {
        CKParameter *source = Source();
        ScriptParameterRegistry *registry = source ? ScriptParameterRegistry::FromContext(source->GetCKContext()) : nullptr;
        CKDWORD value = 0;
        if (!source || !registry || source->GetValue(&value, TRUE) != CK_OK) {
            return std::string();
        }
        return registry->FlagsText(source->GetGUID(), value);
    }

    ParamStructRef *Struct() {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            CKParameter *source = Source();
            target = source;
        }
        if (!target) {
            SetScriptException(error.empty() ? "Struct requires a valid parameter." : error);
            return nullptr;
        }
        ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(target->GetCKContext());
        const ScriptParamTypeRecord *record = registry ? registry->GetType(target) : nullptr;
        if (!record || !record->Has(ScriptParamTypeCaps::StructLike)) {
            SetScriptException(fmt::format("Parameter '{}' is not a CK struct.", SafeString(target->GetName())));
            return nullptr;
        }
        return new ParamStructRef(m_Bridge, target->GetID());
    }

    bool SetText(const std::string &text) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        CKERROR err = WriteParameterText(target, text, error);
        if (err != CK_OK) {
            SetScriptException(error.empty() ? fmt::format("Failed to set parameter text for '{}' (CKERROR {}).", SafeString(target->GetName()), err) : error);
            return false;
        }
        return true;
    }

    NativeBuffer *GetRaw() const {
        CKParameter *source = Source();
        ScriptParamValue value;
        std::string error;
        if (!ReadParameterValueAs(source, ScriptParamValueKind::Raw, value, error) || value.RawBytes().empty()) {
            if (!error.empty()) {
                SetScriptException(error);
            }
            return NativeBuffer::Create(0);
        }
        NativeBuffer *buffer = NativeBuffer::Create(value.RawBytes().size());
        if (!buffer) {
            return buffer;
        }
        buffer->Write(const_cast<char *>(value.RawBytes().data()), value.RawBytes().size());
        buffer->Reset();
        return buffer;
    }

    bool SetRaw(NativeBuffer *buffer) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        if (!buffer) {
            SetScriptException("NativeBuffer is null.");
            return false;
        }
        ScriptParamValue value = MakeScriptParamRaw(target->GetGUID(), TypeName(), buffer->Data(), static_cast<int>(buffer->Size()));
        CKERROR err = SetBridgeParamValue(target, value, error);
        if (err != CK_OK) {
            SetScriptException(error.empty() ? fmt::format("Failed to set raw parameter (CKERROR {}).", err) : error);
            return false;
        }
        return true;
    }

    std::string Describe() const {
        if (!IsValid()) {
            return "ParamRef is not valid.";
        }
        return fmt::format("{} '{}' id={} index={} type={} size={}",
            KindName(),
            GetName(),
            m_ParameterId,
            m_Index,
            TypeName(),
            DataSize());
    }

private:
    CKObject *RawGet() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetCKObjectById(context, m_ParameterId);
    }

    CKObject *RawGetStamped() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetStampedCKObjectById(context, m_ParameterId, m_Stamp);
    }

    CKBehavior *OwnerBehavior() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return CKBehavior::Cast(GetCKObjectById(context, m_OwnerBehaviorId));
    }

    CKParameter *WritableParameter(std::string &error) const {
        CKObject *object = Get();
        if (CKParameterIn *input = CKParameterIn::Cast(object)) {
            if (CKBehavior *behavior = OwnerBehavior()) {
                CKParameterLocal *local = EnsureInputSource(behavior, m_Index, nullptr);
                if (local) {
                    return local;
                }
            }
            CKParameter *source = input->GetRealSource();
            if (source) {
                return source;
            }
            error = fmt::format("Input parameter '{}' has no writable source.", SafeString(input->GetName()));
            return nullptr;
        }
        if (CKParameter *param = CKParameter::Cast(object)) {
            return param;
        }
        error = "ParamRef is not valid.";
        return nullptr;
    }

    bool SetDirectValue(const ScriptParamValue &value) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        CKERROR err = WriteParameterValue(target, value, error);
        if (err != CK_OK) {
            SetScriptException(error.empty() ? fmt::format("Failed to set parameter (CKERROR {}).", err) : error);
            return false;
        }
        return true;
    }

    const char *KindName() const {
        switch (m_Kind) {
            case ScriptBridgeSlotKind::Input: return "input";
            case ScriptBridgeSlotKind::Output: return "output";
            case ScriptBridgeSlotKind::Pin: return "pin";
            case ScriptBridgeSlotKind::Pout: return "pout";
            case ScriptBridgeSlotKind::Local: return "local";
            case ScriptBridgeSlotKind::OperationIn: return "operation input";
            case ScriptBridgeSlotKind::OperationOut: return "operation output";
            default: return "parameter";
        }
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ParameterId = 0;
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    CK_ID m_OwnerBehaviorId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

inline ParamRef *ParamStructRef::Member(int index) const {
    CKParameter *member = GetStructMemberParameter(Get(), index);
    return m_Bridge && member ? m_Bridge->WrapParameter(member, ScriptBridgeSlotKind::Standalone, index) : nullptr;
}

class ParamOperationRef final : public RefCounted {
public:
    ParamOperationRef(ScriptBehaviorBridge *bridge,
                      CK_ID operationId,
                      CKParameterIn *targetInput = nullptr,
                      CKParameter *previousSource = nullptr,
                      const std::vector<CK_ID> &ownedLocalSourceIds = {})
        : m_Bridge(bridge),
          m_OperationId(operationId),
          m_TargetInputId(targetInput ? targetInput->GetID() : 0),
          m_PreviousSourceId(previousSource ? previousSource->GetID() : 0),
          m_TargetInputStamp(CaptureBridgeObjectStamp(targetInput)),
          m_PreviousSourceStamp(CaptureBridgeObjectStamp(previousSource)) {
        m_Stamp = CaptureBridgeObjectStamp(RawGet());
        CKParameterOperation *operation = Get();
        CKParameterOut *out = operation ? operation->GetOutParameter() : nullptr;
        m_InstalledSourceId = out ? out->GetID() : 0;
        m_InstalledSourceStamp = CaptureBridgeObjectStamp(out);
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        m_OwnedLocalSources.reserve(ownedLocalSourceIds.size());
        for (CK_ID id : ownedLocalSourceIds) {
            CKObject *object = GetCKObjectById(context, id);
            if (CKParameterLocal::Cast(object)) {
                m_OwnedLocalSources.push_back(ScriptBridgeStampedObjectId{id, CaptureBridgeObjectStamp(object)});
            }
        }
    }

    CKParameterOperation *Get() const {
        return CKParameterOperation::Cast(RawGetStamped());
    }

    bool IsValid() const { return Get() != nullptr; }
    CK_ID GetID() const { return m_OperationId; }

    ParamRef *Out() const {
        CKParameterOperation *op = Get();
        return m_Bridge && op ? m_Bridge->WrapParameter(op->GetOutParameter(), ScriptBridgeSlotKind::OperationOut, 0) : nullptr;
    }

    ParamRef *In1() const {
        CKParameterOperation *op = Get();
        return m_Bridge && op ? m_Bridge->WrapParameter(op->GetInParameter1(), ScriptBridgeSlotKind::OperationIn, 0) : nullptr;
    }

    ParamRef *In2() const {
        CKParameterOperation *op = Get();
        return m_Bridge && op ? m_Bridge->WrapParameter(op->GetInParameter2(), ScriptBridgeSlotKind::OperationIn, 1) : nullptr;
    }

    CKERROR Do() const {
        CKParameterOperation *op = Get();
        return op ? op->DoOperation() : CKERR_INVALIDPARAMETER;
    }

    bool Restore() {
        std::string error;
        if (!RestoreTargetSource(error)) {
            SetScriptException(error.empty() ? "Failed to restore operation target source." : error);
            return false;
        }
        return true;
    }

    bool Destroy() {
        CKParameterOperation *op = Get();
        if (!op || !m_Bridge || !m_Bridge->GetManager() || !m_Bridge->GetManager()->GetCKContext()) {
            return false;
        }
        std::string error;
        if (!RestoreTargetSource(error)) {
            SetScriptException(error);
            return false;
        }
        CKBehavior *owner = op->GetOwner();
        if (owner) {
            owner->RemoveParameterOperation(op);
        }
        m_Bridge->GetManager()->GetCKContext()->DestroyObject(op);
        DestroyOwnedLocalSources(owner);
        m_OperationId = 0;
        m_Stamp = ScriptBridgeObjectStamp();
        return true;
    }

    std::string Describe() const {
        CKParameterOperation *op = Get();
        if (!op) return "ParamOperationRef is not valid.";
        CKParameterManager *pm = op->GetCKContext() ? op->GetCKContext()->GetParameterManager() : nullptr;
        CKSTRING name = pm ? pm->OperationGuidToName(op->GetOperationGuid()) : nullptr;
        return fmt::format("Parameter operation '{}' id={} guid={}",
            name && name[0] ? name : SafeString(op->GetName()),
            op->GetID(),
            GuidToString(op->GetOperationGuid()));
    }

private:
    CKObject *RawGet() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetCKObjectById(context, m_OperationId);
    }

    CKObject *RawGetStamped() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetStampedCKObjectById(context, m_OperationId, m_Stamp);
    }

    CKParameterIn *TargetInput() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return CKParameterIn::Cast(GetStampedCKObjectById(context, m_TargetInputId, m_TargetInputStamp));
    }

    CKParameter *PreviousSource() const {
        if (!m_PreviousSourceId) {
            return nullptr;
        }
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return ParameterSourceForConnection(GetStampedCKObjectById(context, m_PreviousSourceId, m_PreviousSourceStamp));
    }

    CKParameter *InstalledSource() const {
        if (!m_InstalledSourceId) {
            return nullptr;
        }
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return ParameterSourceForConnection(GetStampedCKObjectById(context, m_InstalledSourceId, m_InstalledSourceStamp));
    }

    void DestroyOwnedLocalSources(CKBehavior *owner) {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        if (!context) {
            m_OwnedLocalSources.clear();
            return;
        }

        for (const ScriptBridgeStampedObjectId &source : m_OwnedLocalSources) {
            CKObject *object = GetStampedCKObjectById(context, source.Id, source.Stamp);
            CKParameterLocal *local = CKParameterLocal::Cast(object);
            if (!local) {
                continue;
            }
            CKBehavior *localOwner = owner ? owner : CKBehavior::Cast(local->GetOwner());
            if (localOwner) {
                const int position = localOwner->GetLocalParameterPosition(local);
                if (position >= 0) {
                    localOwner->RemoveLocalParameter(position);
                }
            }
            if (!local->IsToBeDeleted()) {
                context->DestroyObject(local);
            }
        }
        m_OwnedLocalSources.clear();
    }

    bool RestoreTargetSource(std::string &error) {
        if (m_TargetRestored || !m_TargetInputId) {
            return true;
        }

        CKParameterIn *target = TargetInput();
        if (!target) {
            error = fmt::format("Operation target input id={} is no longer valid.", m_TargetInputId);
            return false;
        }

        CKParameter *installed = InstalledSource();
        CKParameter *current = target->GetDirectSource();
        if (m_InstalledSourceId && current && installed && current->GetID() != installed->GetID()) {
            error = fmt::format("Operation target '{}' was already changed by another graph edit.",
                                SafeString(target->GetName()));
            return false;
        }

        CKParameter *previous = PreviousSource();
        if (m_PreviousSourceId && !previous) {
            error = fmt::format("Previous source id={} for operation target '{}' is no longer valid.",
                                m_PreviousSourceId,
                                SafeString(target->GetName()));
            return false;
        }

        CKERROR err = target->SetDirectSource(previous);
        if (err != CK_OK) {
            error = fmt::format("Failed to restore operation target '{}' (CKERROR {}).",
                                SafeString(target->GetName()),
                                err);
            return false;
        }

        m_TargetRestored = true;
        return true;
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_OperationId = 0;
    CK_ID m_TargetInputId = 0;
    CK_ID m_PreviousSourceId = 0;
    CK_ID m_InstalledSourceId = 0;
    ScriptBridgeObjectStamp m_Stamp;
    ScriptBridgeObjectStamp m_TargetInputStamp;
    ScriptBridgeObjectStamp m_PreviousSourceStamp;
    ScriptBridgeObjectStamp m_InstalledSourceStamp;
    std::vector<ScriptBridgeStampedObjectId> m_OwnedLocalSources;
    bool m_TargetRestored = false;
};

class ParamOp final : public RefCounted {
public:
    ParamOp(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const std::string &operationName)
        : m_Bridge(bridge), m_Context(ctx) {
        m_Request.OperationName = operationName;
    }

    ParamOp(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CKGUID operationGuid)
        : m_Bridge(bridge), m_Context(ctx) {
        m_Request.OperationGuid = operationGuid;
    }

    ParamOp *Result(CKGUID guid) {
        m_Request.ResultTypeGuid = guid;
        AddRef();
        return this;
    }

    ParamOp *ResultName(const std::string &typeName) {
        m_Request.ResultTypeName = typeName;
        AddRef();
        return this;
    }

    ParamOp *InRef(int slot, ParamRef *source) {
        ScriptBridgeOperationInput *input = Slot(slot);
        if (!input) {
            SetScriptException("Parameter operation input slot must be 0 or 1.");
            return nullptr;
        }
        if (!source || !source->IsValid()) {
            SetScriptException("Parameter operation input source is not valid.");
            return nullptr;
        }
        *input = ScriptBridgeOperationInput();
        input->Kind = ScriptBridgeInputBindingKind::Source;
        input->SourceId = source->GetID();
        input->SourceStamp = source->Stamp();
        AddRef();
        return this;
    }

    ParamOp *InValue(int slot, ParamValue *value) {
        ScriptBridgeOperationInput *input = Slot(slot);
        if (!input) {
            SetScriptException("Parameter operation input slot must be 0 or 1.");
            return nullptr;
        }
        *input = ScriptBridgeOperationInput();
        if (value) {
            input->Kind = ScriptBridgeInputBindingKind::Value;
            input->Value = value->Value();
        }
        AddRef();
        return this;
    }

    ScriptBridgeOperationSpec RequestForPin(int pinIndex) const {
        ScriptBridgeOperationSpec request = m_Request;
        request.TargetPinIndex = pinIndex;
        return request;
    }

    std::string Describe() const {
        CKContext *context = m_Context.Context;
        CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
        std::string error;
        const CKGUID guid = ResolveOperationGuid(context, m_Request.OperationGuid, m_Request.OperationName, error);
        CKSTRING name = pm && guid.IsValid() ? pm->OperationGuidToName(guid) : nullptr;
        return fmt::format("ParamOp '{}' guid={} result={}",
            name && name[0] ? name : m_Request.OperationName,
            guid.IsValid() ? GuidToString(guid) : std::string("<unresolved>"),
            m_Request.ResultTypeGuid.IsValid() ? GuidToString(m_Request.ResultTypeGuid) : m_Request.ResultTypeName);
    }

private:
    ScriptBridgeOperationInput *Slot(int slot) {
        if (slot == 0) return &m_Request.In1;
        if (slot == 1) return &m_Request.In2;
        return nullptr;
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeOperationSpec m_Request;
};

class BehaviorRef final : public RefCounted {
public:
    BehaviorRef(ScriptBehaviorBridge *bridge, CK_ID behaviorId, CK_ID componentId)
        : m_Bridge(bridge), m_BehaviorId(behaviorId), m_ComponentId(componentId) {
        m_Stamp = CaptureBridgeObjectStamp(RawGet());
    }

    CKBehavior *Get() const {
        return CKBehavior::Cast(RawGetStamped());
    }

    bool IsValid() const { return Get() != nullptr; }
    CK_ID GetID() const { return m_BehaviorId; }
    std::string GetName() const { CKBehavior *b = Get(); return b ? SafeString(b->GetName()) : std::string(); }
    bool IsActive() const { CKBehavior *b = Get(); return b && b->IsActive(); }
    CKGUID GetPrototypeGuid() const { CKBehavior *b = Get(); return b ? b->GetPrototypeGuid() : CKGUID(); }
    std::string GetPrototypeName() const { CKBehavior *b = Get(); return b ? SafeString(b->GetPrototypeName()) : std::string(); }

    BehaviorLayout *Layout() const { return m_Bridge ? new BehaviorLayout(m_Bridge, m_BehaviorId) : nullptr; }

    bool Trigger(int inputIndex, bool reset) {
        CKBehavior *behavior = Get();
        if (!behavior) {
            SetScriptException("BehaviorRef is not valid.");
            return false;
        }
        if (reset) {
            ActivateParentChain(behavior, true);
        }
        std::string error;
        if (!PulseInputIndex(behavior, inputIndex, error, reset)) {
            SetScriptException(error);
            return false;
        }
        ActivateParentChain(behavior, false);
        ActivateOwnerScriptInCurrentScene(behavior, false);
        return true;
    }

    GraphTask *Start(int inputIndex, bool reset, float timeoutSeconds) {
        CKBehavior *behavior = Get();
        if (!behavior || !Trigger(inputIndex, reset)) {
            if (!behavior) SetScriptException("BehaviorRef is not valid.");
            return nullptr;
        }
        return m_Bridge ? m_Bridge->CreateGraphWatch(behavior, m_ComponentId, timeoutSeconds) : nullptr;
    }

    GraphTask *Watch(float timeoutSeconds) {
        CKBehavior *behavior = Get();
        if (!behavior) {
            SetScriptException("BehaviorRef is not valid.");
            return nullptr;
        }
        return m_Bridge ? m_Bridge->CreateGraphWatch(behavior, m_ComponentId, timeoutSeconds) : nullptr;
    }

    bool InputActive(int inputIndex) const {
        CKBehavior *behavior = Get();
        return behavior && inputIndex >= 0 && inputIndex < behavior->GetInputCount() && behavior->IsInputActive(inputIndex);
    }

    bool OutputActive(int outputIndex) const {
        CKBehavior *behavior = Get();
        return behavior && outputIndex >= 0 && outputIndex < behavior->GetOutputCount() && behavior->IsOutputActive(outputIndex);
    }

    ParamRef *Pin(int index) const {
        CKBehavior *behavior = Get();
        CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount() ? behavior->GetInputParameter(index) : nullptr;
        return m_Bridge && pin ? new ParamRef(m_Bridge, pin->GetID(), ScriptBridgeSlotKind::Pin, index, m_BehaviorId) : nullptr;
    }

    ParamRef *Pout(int index) const {
        CKBehavior *behavior = Get();
        CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
        return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, m_BehaviorId) : nullptr;
    }

    ParamRef *Local(int index) const {
        CKBehavior *behavior = Get();
        CKParameterLocal *local = behavior && index >= 0 && index < behavior->GetLocalParameterCount() ? behavior->GetLocalParameter(index) : nullptr;
        return m_Bridge && local ? new ParamRef(m_Bridge, local->GetID(), ScriptBridgeSlotKind::Local, index, m_BehaviorId) : nullptr;
    }

    ParamOperationRef *ConnectOperation(int pinIndex, ParamOp *operation) {
        CKBehavior *behavior = Get();
        if (!behavior || !operation) {
            SetScriptException("BehaviorRef or ParamOp is not valid.");
            return nullptr;
        }
        std::string error;
        ParamOperationRef *result = ConnectOperationToInput(m_Bridge, behavior, pinIndex, operation->RequestForPin(pinIndex), error, false, nullptr);
        if (!result) {
            SetScriptException(error);
        }
        return result;
    }

    std::string Describe() const {
        BehaviorLayout *layout = Layout();
        std::string result = fmt::format("Behavior '{}' id={} prototype={}", GetName(), m_BehaviorId, GetPrototypeName());
        if (layout) {
            result += "\n" + layout->Describe();
            layout->Release();
        }
        return result;
    }

private:
    CKObject *RawGet() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetCKObjectById(context, m_BehaviorId);
    }

    CKObject *RawGetStamped() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetStampedCKObjectById(context, m_BehaviorId, m_Stamp);
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_BehaviorId = 0;
    CK_ID m_ComponentId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

class BehaviorBridge final : public RefCounted {
public:
    BehaviorBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx)
        : m_Bridge(bridge), m_Context(ctx) {}

    BehaviorRef *Self() const {
        return m_Bridge && m_Context.Behavior ? m_Bridge->WrapBehavior(m_Context.Behavior, ComponentIdFromContext(m_Context)) : nullptr;
    }

    BehaviorRef *OwnerScript() const {
        return m_Bridge && m_Context.Behavior ? m_Bridge->WrapBehavior(m_Context.Behavior->GetOwnerScript(), ComponentIdFromContext(m_Context)) : nullptr;
    }

    BehaviorRef *Find(const std::string &name) const {
        if (!m_Bridge || name.empty()) return nullptr;
        if (m_Context.Behavior) {
            if (CKBehavior *ownerScript = m_Context.Behavior->GetOwnerScript()) {
                if (CKBehavior *found = FindBehaviorRecursive(ownerScript, name)) {
                    return m_Bridge->WrapBehavior(found, ComponentIdFromContext(m_Context));
                }
            }
        }
        CKBeObject *owner = m_Context.Behavior ? m_Context.Behavior->GetOwner() : nullptr;
        if (CKBehavior *found = FindBehaviorOnOwner(owner, name)) {
            return m_Bridge->WrapBehavior(found, ComponentIdFromContext(m_Context));
        }
        return nullptr;
    }

    BehaviorRef *FindOn(CKBeObject *owner, const std::string &name) const {
        if (!m_Bridge || !owner || name.empty()) return nullptr;
        return m_Bridge->WrapBehavior(FindBehaviorOnOwner(owner, name), ComponentIdFromContext(m_Context));
    }

    BehaviorRef *FindByID(CK_ID id) const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return m_Bridge ? m_Bridge->WrapBehavior(CKBehavior::Cast(GetCKObjectById(context, id)), ComponentIdFromContext(m_Context)) : nullptr;
    }

    BehaviorRef *FromParameter(const std::string &name) const {
        if (!m_Bridge || !m_Context.Behavior) return nullptr;
        CKParameter *param = FindReadableParameter(m_Context.Behavior, name);
        CKObject *obj = param ? param->GetValueObject(TRUE) : nullptr;
        return m_Bridge->WrapBehavior(CKBehavior::Cast(obj), ComponentIdFromContext(m_Context));
    }

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
};

class BBCallBuilder final : public RefCounted {
public:
    BBCallBuilder(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request)
        : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

    BBCallBuilder *Owner(CKBeObject *owner) { m_Request.OwnerId = owner ? owner->GetID() : 0; AddRef(); return this; }
    BBCallBuilder *Target(CKBeObject *target) { m_Request.TargetId = target ? target->GetID() : 0; AddRef(); return this; }

    BBCallBuilder *Set(int pinIndex, ParamValue *value) {
        if (value) ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value());
        AddRef();
        return this;
    }

    BBCallBuilder *SetSource(int pinIndex, ParamRef *source) {
        if (!source || !source->IsValid()) {
            SetScriptException("BBCallBuilder.SetSource requires a valid ParamRef source.");
            return nullptr;
        }
        ScriptBridgeInputSource request;
        request.PinIndex = pinIndex;
        request.SourceId = source->GetID();
        request.SourceStamp = source->Stamp();
        m_Request.SourceParameters.push_back(request);
        AddRef();
        return this;
    }

    BBCallBuilder *SetOperation(int pinIndex, ParamOp *operation) {
        if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex));
        AddRef();
        return this;
    }

    BBResult *Run(int inputIndex) { return m_Bridge ? m_Bridge->RunCall(m_Request, m_Context, inputIndex) : nullptr; }

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBTaskBuilder final : public RefCounted {
public:
    BBTaskBuilder(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request)
        : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

    BBTaskBuilder *Owner(CKBeObject *owner) { m_Request.OwnerId = owner ? owner->GetID() : 0; AddRef(); return this; }
    BBTaskBuilder *Target(CKBeObject *target) { m_Request.TargetId = target ? target->GetID() : 0; AddRef(); return this; }
    BBTaskBuilder *Set(int pinIndex, ParamValue *value) { if (value) ScriptBridgeSetIndexedValue(m_Request.IndexedParameters, pinIndex, value->Value()); AddRef(); return this; }
    BBTaskBuilder *SetSource(int pinIndex, ParamRef *source) {
        if (!source || !source->IsValid()) {
            SetScriptException("BBTaskBuilder.SetSource requires a valid ParamRef source.");
            return nullptr;
        }
        ScriptBridgeInputSource request;
        request.PinIndex = pinIndex;
        request.SourceId = source->GetID();
        request.SourceStamp = source->Stamp();
        m_Request.SourceParameters.push_back(request);
        AddRef();
        return this;
    }
    BBTaskBuilder *SetOperation(int pinIndex, ParamOp *operation) { if (operation) m_Request.OperationParameters.push_back(operation->RequestForPin(pinIndex)); AddRef(); return this; }
    BBTask *Start(int inputIndex) { return m_Bridge ? m_Bridge->StartTask(m_Request, m_Context, inputIndex) : nullptr; }

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBPrototype final : public RefCounted {
public:
    BBPrototype(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request)
        : m_Bridge(bridge), m_Context(ctx), m_Request(request) {}

    BBCallBuilder *Call() { return m_Bridge ? new BBCallBuilder(m_Bridge, m_Context, m_Request) : nullptr; }
    BBTaskBuilder *Spawn() { return m_Bridge ? new BBTaskBuilder(m_Bridge, m_Context, m_Request) : nullptr; }
    BehaviorLayout *Layout() const { return m_Bridge ? new BehaviorLayout(m_Bridge, m_Context, m_Request) : nullptr; }

    bool IsValid() const {
        std::string error;
        return Prototype(error) != nullptr;
    }

    CKGUID GetGuid() const {
        std::string error;
        CKBehaviorPrototype *prototype = Prototype(error);
        return prototype ? prototype->GetGuid() : CKGUID();
    }

    std::string GetName() const {
        std::string error;
        CKBehaviorPrototype *prototype = Prototype(error);
        return prototype ? SafeString(prototype->GetName()) : std::string();
    }

    std::string GetCategory() const {
        std::string error;
        CKBehaviorPrototype *prototype = Prototype(error);
        CKObjectDeclaration *decl = prototype ? prototype->GetSoureObjectDeclaration() : nullptr;
        return decl ? SafeString(decl->GetCategory()) : std::string();
    }

    std::string GetQualifiedName() const {
        const std::string category = GetCategory();
        const std::string name = GetName();
        return category.empty() ? name : category + "/" + name;
    }

    std::string Describe() const {
        BehaviorLayout *layout = Layout();
        std::string result = fmt::format("Building Block '{}' guid={}", GetQualifiedName(), GuidToString(GetGuid()));
        if (layout) {
            result += "\n" + layout->Describe();
            layout->Release();
        }
        return result;
    }

private:
    CKBehaviorPrototype *Prototype(std::string &error) const {
        return m_Bridge ? m_Bridge->ResolvePrototypeObject(m_Request, error) : nullptr;
    }

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBBridge final : public RefCounted {
public:
    BBBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx)
        : m_Bridge(bridge), m_Context(ctx) {}

    BBPrototype *PrototypeByName(const std::string &name) const {
        return m_Bridge ? m_Bridge->CreatePrototype(m_Context, name) : nullptr;
    }

    BBPrototype *PrototypeByGuid(CKGUID guid) const {
        return m_Bridge ? m_Bridge->CreatePrototype(m_Context, guid) : nullptr;
    }

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
};

class BBResult final : public RefCounted {
public:
    BBResult(ScriptBehaviorBridge *bridge, CK_ID resultId, int generation);
    ~BBResult() override;

    bool Ok() const;
    int ReturnCode() const;
    std::string Error() const;
    bool OutputActive(int outputIndex) const;
    ParamRef *Pout(int index) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ResultId = 0;
    int m_Generation = 0;
};

class BBTask final : public RefCounted {
public:
    BBTask(ScriptBehaviorBridge *bridge, CK_ID taskId, int generation);
    ~BBTask() override;

    bool IsValid() const;
    bool IsAlive() const;
    bool IsPaused() const;
    int ReturnCode() const;
    std::string Error() const;
    bool OutputActive(int outputIndex) const;
    bool Step(const CKBehaviorContext &ctx, int inputIndex);
    bool Reset();
    bool Destroy();
    BehaviorRef *Behavior() const;
    ParamRef *Pout(int index) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_TaskId = 0;
    int m_Generation = 0;
};

class GraphTask final : public RefCounted {
public:
    GraphTask(ScriptBehaviorBridge *bridge, CK_ID watchId, int generation);
    ~GraphTask() override;

    bool IsValid() const;
    bool IsAlive() const;
    bool IsPaused() const;
    bool TimedOut() const;
    float Elapsed() const;
    std::string Error() const;
    GraphTask *Timeout(float seconds);
    bool Step(const CKBehaviorContext &ctx);
    bool Done(int outputIndex) const;
    bool OutputActive(int outputIndex) const;
    bool Cancel();
    bool Reset();
    BehaviorRef *Behavior() const;
    ParamRef *Pout(int index) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_WatchId = 0;
    int m_Generation = 0;
};

#endif // CK_SCRIPTBRIDGEHANDLES_H
