#include "ScriptBridgeHandles.h"

#include <algorithm>

#include <fmt/format.h>

namespace {

CKContext *BridgeContext(ScriptBehaviorBridge *bridge, CKContext *fallback = nullptr) {
    return bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : fallback;
}

} // namespace

ParamInfo::ParamInfo(ScriptBridgeSlotKind kind,
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

int ParamInfo::GetKind() const { return static_cast<int>(m_Kind); }
int ParamInfo::GetIndex() const { return m_Index; }
std::string ParamInfo::GetName() const { return m_Name; }
CKGUID ParamInfo::GetTypeGuid() const { return m_TypeGuid; }
std::string ParamInfo::GetTypeName() const { return m_TypeName; }
int ParamInfo::GetDataSize() const { return m_DataSize; }

std::string ParamInfo::Describe() const {
    return fmt::format("#{} {}: {} size={}", m_Index, m_Name, m_TypeName, m_DataSize);
}

ParamValue::ParamValue(const ScriptParamValue &value)
    : m_Value(value) {}

const ScriptParamValue &ParamValue::Value() const { return m_Value; }
bool ParamValue::IsValid() const { return m_Value.Kind != ScriptParamValueKind::Empty; }

CKGUID ParamValue::TypeGuid() const {
    return CKGuidIsValid(m_Value.TypeGuid) ? m_Value.TypeGuid : ScriptParameterGuidForValue(m_Value);
}

std::string ParamValue::TypeName() const {
    if (!m_Value.TypeNameText().empty()) {
        return m_Value.TypeNameText();
    }
    return CKGuidIsValid(TypeGuid()) ? GuidToString(TypeGuid()) : std::string();
}

int ParamValue::AsInt() const {
    if (m_Value.Kind == ScriptParamValueKind::Int) return m_Value.Data.IntValue;
    if (m_Value.Kind == ScriptParamValueKind::Enum || m_Value.Kind == ScriptParamValueKind::Flags) return static_cast<int>(m_Value.Data.DwordValue);
    ReportWrongKind("AsInt", "int/enum/flags");
    return 0;
}

float ParamValue::AsFloat() const {
    if (m_Value.Kind == ScriptParamValueKind::Float) return m_Value.Data.FloatValue;
    if (m_Value.Kind == ScriptParamValueKind::Int) return static_cast<float>(m_Value.Data.IntValue);
    ReportWrongKind("AsFloat", "float/int");
    return 0.0f;
}

bool ParamValue::AsBool() const {
    if (m_Value.Kind == ScriptParamValueKind::Bool) return m_Value.Data.BoolValue;
    ReportWrongKind("AsBool", "bool");
    return false;
}

std::string ParamValue::AsString() const {
    if (m_Value.Kind == ScriptParamValueKind::String || m_Value.Kind == ScriptParamValueKind::Text) return m_Value.Text();
    ReportWrongKind("AsString", "string/text");
    return std::string();
}

CKGUID ParamValue::AsGuid() const {
    if (m_Value.Kind == ScriptParamValueKind::Guid) return m_Value.Data.GuidValue;
    ReportWrongKind("AsGuid", "CKGUID");
    return CKGUID();
}

VxVector ParamValue::AsVector() const {
    if (m_Value.Kind == ScriptParamValueKind::Vector) return m_Value.Data.VectorValue;
    ReportWrongKind("AsVector", "VxVector");
    return VxVector();
}

Vx2DVector ParamValue::AsVector2() const {
    if (m_Value.Kind == ScriptParamValueKind::Vector2) return m_Value.Data.Vector2Value;
    ReportWrongKind("AsVector2", "Vx2DVector");
    return Vx2DVector();
}

VxColor ParamValue::AsColor() const {
    if (m_Value.Kind == ScriptParamValueKind::Color) return m_Value.Data.ColorValue;
    ReportWrongKind("AsColor", "VxColor");
    return VxColor();
}

VxQuaternion ParamValue::AsQuaternion() const {
    if (m_Value.Kind == ScriptParamValueKind::Quaternion) return m_Value.Data.QuaternionValue;
    ReportWrongKind("AsQuaternion", "VxQuaternion");
    return VxQuaternion();
}

VxMatrix ParamValue::AsMatrix() const {
    if (m_Value.Kind == ScriptParamValueKind::Matrix) return m_Value.Data.MatrixValue;
    ReportWrongKind("AsMatrix", "VxMatrix");
    return VxMatrix();
}

ParamStructValue *ParamValue::AsStruct() const {
    if (m_Value.Kind != ScriptParamValueKind::Struct) {
        ReportWrongKind("AsStruct", "struct");
        return nullptr;
    }
    return new ParamStructValue(m_Value);
}

std::string ParamValue::AsText() const {
    return ScriptParamValueToText(m_Value);
}

NativeBuffer *ParamValue::AsRaw() const {
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

void ParamValue::ReportWrongKind(const char *method, const char *expected) const {
    SetScriptException(fmt::format("ParamValue.{} expected {}, got {}.",
                                   method,
                                   expected,
                                   ScriptParamValueKindName(m_Value.Kind)));
}

ParamStructValue::ParamStructValue(const ScriptParamValue &value)
    : m_Value(value) {}

bool ParamStructValue::IsValid() const {
    return m_Value.Kind == ScriptParamValueKind::Struct && CKGuidIsValid(m_Value.TypeGuid);
}

CKGUID ParamStructValue::TypeGuid() const { return m_Value.TypeGuid; }

std::string ParamStructValue::TypeName() const {
    if (!m_Value.TypeNameText().empty()) {
        return m_Value.TypeNameText();
    }
    return CKGuidIsValid(m_Value.TypeGuid) ? GuidToString(m_Value.TypeGuid) : std::string();
}

ParamStructValue *ParamStructValue::Set(int index, ParamValue *value) {
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

ParamValue *ParamStructValue::Value() const {
    return new ParamValue(m_Value);
}

ParamValue *ParamStructValue::AsValue() const {
    return Value();
}

std::string ParamStructValue::Describe() const {
    return fmt::format("ParamStructValue type={} members={}",
                       CKGuidIsValid(m_Value.TypeGuid) ? GuidToString(m_Value.TypeGuid) : std::string("<unknown>"),
                       m_Value.StructMembers().size());
}

ParamStructRef::ParamStructRef(ScriptBehaviorBridge *bridge, CK_ID parameterId, CKContext *context)
    : ParamRef(bridge, parameterId, ScriptBridgeSlotKind::Standalone, -1, 0, context) {}

bool ParamStructRef::IsValid() const {
    CKParameter *param = CKParameter::Cast(Get());
    ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::StructLike);
}

int ParamStructRef::Count() const {
    CKParameter *param = CKParameter::Cast(Get());
    ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::StructLike) ? static_cast<int>(record->StructMembers.size()) : 0;
}

ParamStructInfo *ParamStructRef::Info() const {
    CKParameter *param = CKParameter::Cast(Get());
    ScriptParameterRegistry *registry = param ? ScriptParameterRegistry::FromContext(param->GetCKContext()) : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(param) : nullptr;
    return record && record->Has(ScriptParamTypeCaps::StructLike) ? new ParamStructInfo(registry, record->Type) : nullptr;
}

ParamRef *ParamStructRef::Member(int index) const {
    CKParameter *member = GetStructMemberParameter(CKParameter::Cast(Get()), index);
    if (!member) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(member, ScriptBridgeSlotKind::Standalone, index)
                    : new ParamRef(nullptr, member->GetID(), ScriptBridgeSlotKind::Standalone, index, 0, member->GetCKContext());
}

int ParamStructRef::FindMember(const std::string &name, int occurrence) const {
    ParamStructInfo *info = Info();
    if (!info) {
        return -1;
    }
    const int result = info->FindMember(name, occurrence);
    info->Release();
    return result;
}

std::string ParamStructRef::Describe() const {
    ParamStructInfo *info = Info();
    if (!info) {
        return "ParamStructRef is not valid.";
    }
    std::string result = info->Describe();
    info->Release();
    return result;
}

ParamSourceLinkRef::ParamSourceLinkRef(ScriptBehaviorBridge *bridge,
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

ParamSourceLinkRef::~ParamSourceLinkRef() {
    std::string error;
    (void) RestoreInternal(error, false);
}

bool ParamSourceLinkRef::IsValid() const {
    return Target() != nullptr && (m_InstalledSourceId == 0 || InstalledSource() != nullptr);
}

bool ParamSourceLinkRef::IsCommitted() const { return m_State == OwnershipState::Committed; }
bool ParamSourceLinkRef::IsRestored() const {
    return m_State == OwnershipState::Restored || m_State == OwnershipState::DetachedCleanup;
}

bool ParamSourceLinkRef::Commit() {
    if (m_State == OwnershipState::Scoped) {
        m_State = OwnershipState::Committed;
    }
    return true;
}

bool ParamSourceLinkRef::Restore() {
    std::string error;
    if (!RestoreInternal(error, true)) {
        SetScriptException(error.empty() ? "Failed to restore parameter source." : error);
        return false;
    }
    return true;
}

bool ParamSourceLinkRef::DestroyDetached() {
    m_State = OwnershipState::DetachedCleanup;
    return true;
}

std::string ParamSourceLinkRef::Describe() const {
    CKParameterIn *target = Target();
    const char *state = "Scoped";
    switch (m_State) {
        case OwnershipState::Scoped: state = "Scoped"; break;
        case OwnershipState::Committed: state = "Committed"; break;
        case OwnershipState::Restored: state = "Restored"; break;
        case OwnershipState::DetachedCleanup: state = "DetachedCleanup"; break;
    }
    return fmt::format("ParamSourceLink target={} previous={} installed={} state={}",
                       target ? SafeString(target->GetName()) : std::string("<invalid>"),
                       m_PreviousSourceId,
                       m_InstalledSourceId,
                       state);
}

CKContext *ParamSourceLinkRef::Context() const {
    return m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
}

CKParameterIn *ParamSourceLinkRef::Target() const {
    return CKParameterIn::Cast(GetStampedCKObjectById(Context(), m_TargetId, m_TargetStamp));
}

CKParameter *ParamSourceLinkRef::PreviousSource() const {
    if (!m_PreviousSourceId) {
        return nullptr;
    }
    return ParameterSourceForConnection(GetStampedCKObjectById(Context(), m_PreviousSourceId, m_PreviousSourceStamp));
}

CKParameter *ParamSourceLinkRef::InstalledSource() const {
    if (!m_InstalledSourceId) {
        return nullptr;
    }
    return ParameterSourceForConnection(GetStampedCKObjectById(Context(), m_InstalledSourceId, m_InstalledSourceStamp));
}

bool ParamSourceLinkRef::RestoreInternal(std::string &error, bool explicitCall) {
    if (m_State != OwnershipState::Scoped) {
        return true;
    }

    CKParameterIn *target = Target();
    if (!target) {
        if (explicitCall) {
            error = fmt::format("Parameter source link target id={} is no longer valid.", m_TargetId);
            return false;
        }
        m_State = OwnershipState::Restored;
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
        m_State = OwnershipState::Restored;
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
        m_State = OwnershipState::Restored;
        return true;
    }

    const CKERROR err = target->SetDirectSource(previous);
    if (err != CK_OK) {
        error = fmt::format("Failed to restore source for '{}' (CKERROR {}).",
                            SafeString(target->GetName()),
                            err);
        return false;
    }

    m_State = OwnershipState::Restored;
    return true;
}

ParamRef::ParamRef(ScriptBehaviorBridge *bridge,
                   CK_ID parameterId,
                   ScriptBridgeSlotKind kind,
                   int index,
                   CK_ID ownerBehaviorId,
                   CKContext *context)
    : ObjectRef(BridgeContext(bridge, context), parameterId),
      m_Bridge(bridge),
      m_Kind(kind),
      m_Index(index),
      m_OwnerBehaviorId(ownerBehaviorId) {}

CKObject *ParamRef::Get() const {
    return Object();
}

const ScriptBridgeObjectStamp &ParamRef::Stamp() const { return ObjectRef::Stamp(); }
CKParameter *ParamRef::Source() const { return ParameterSourceForConnection(Get()); }

bool ParamRef::IsValid() const {
    CKObject *parameter = Get();
    return CKParameterIn::Cast(parameter) != nullptr || CKParameter::Cast(parameter) != nullptr;
}

CK_ID ParamRef::GetID() const { return Id(); }
int ParamRef::GetIndex() const { return m_Index; }
int ParamRef::GetKind() const { return static_cast<int>(m_Kind); }

std::string ParamRef::GetName() const {
    CKObject *parameter = Get();
    return parameter ? SafeString(parameter->GetName()) : std::string();
}

CKGUID ParamRef::TypeGuid() const {
    CKObject *parameter = Get();
    if (CKParameterIn *input = CKParameterIn::Cast(parameter)) return input->GetGUID();
    if (CKParameter *param = CKParameter::Cast(parameter)) return param->GetGUID();
    return CKGUID();
}

std::string ParamRef::TypeName() const {
    CKObject *parameter = Get();
    return ParameterTypeLabel(parameter ? parameter->GetCKContext() : nullptr, TypeGuid());
}

int ParamRef::DataSize() const {
    CKObject *parameter = Get();
    if (CKParameterIn *input = CKParameterIn::Cast(parameter)) {
        CKParameter *source = input->GetRealSource();
        return source ? source->GetDataSize() : ParameterDefaultSize(input->GetCKContext(), input->GetGUID());
    }
    if (CKParameter *param = CKParameter::Cast(parameter)) return param->GetDataSize();
    return 0;
}

ParamRef *ParamRef::RealSource() const {
    CKParameter *source = Source();
    if (!source) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(source, ScriptBridgeSlotKind::Standalone, -1)
                    : new ParamRef(nullptr, source->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, source->GetCKContext());
}

ParamRef *ParamRef::DirectSource() const {
    CKParameterIn *input = CKParameterIn::Cast(Get());
    CKParameter *source = input ? input->GetDirectSource() : nullptr;
    if (!source) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(source, ScriptBridgeSlotKind::Standalone, -1)
                    : new ParamRef(nullptr, source->GetID(), ScriptBridgeSlotKind::Standalone, -1, 0, source->GetCKContext());
}

ParamSourceLinkRef *ParamRef::SetSourceScoped(ParamRef *sourceRef) {
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

bool ParamRef::SetSource(ParamRef *sourceRef) {
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

bool ParamRef::Set(ParamValue *value) {
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

bool ParamRef::SetInt(int value) { return SetDirectValue(MakeScriptParamInt(value)); }
bool ParamRef::SetFloat(float value) { return SetDirectValue(MakeScriptParamFloat(value)); }
bool ParamRef::SetBool(bool value) { return SetDirectValue(MakeScriptParamBool(value)); }
bool ParamRef::SetString(const std::string &value) { return SetDirectValue(MakeScriptParamString(value)); }
bool ParamRef::SetObject(CKObject *value) { return SetDirectValue(MakeScriptParamObject(value)); }

bool ParamRef::SetEnum(const std::string &nameOrValue) {
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

bool ParamRef::SetEnumInt(int value) {
    return SetDirectValue(MakeScriptParamEnum(TypeGuid(), TypeName(), static_cast<CKDWORD>(value)));
}

bool ParamRef::SetFlags(const std::string &namesOrMask) {
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

bool ParamRef::SetFlagsMask(CKDWORD value) {
    return SetDirectValue(MakeScriptParamFlags(TypeGuid(), TypeName(), value));
}

bool ParamRef::SetStruct(ParamStructValue *value) {
    if (!value) {
        SetScriptException("ParamStructValue is null.");
        return false;
    }
    ParamValue *asValue = value->Value();
    const bool result = Set(asValue);
    asValue->Release();
    return result;
}

ParamValue *ParamRef::GetValue() const {
    CKParameter *source = Source();
    if (!source) {
        return new ParamValue(ScriptParamValue());
    }
    std::string error;
    ScriptParamValue value = ReadParameterValue(source, &error);
    value.TypeGuid = source->GetGUID();
    value.Type = source->GetType();
    const std::string typeName = DescribeScriptParamType(source->GetCKContext(), source->GetGUID());
    if (!typeName.empty()) {
        value.MutableTypeNameText() = typeName;
    }
    return new ParamValue(value);
}

bool ParamRef::CopyFrom(ParamRef *sourceRef) {
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

std::string ParamRef::GetText() const {
    std::string text;
    if (ReadParameterText(Source(), text)) {
        return text;
    }
    return std::string();
}

std::string ParamRef::GetEnumText() const {
    CKParameter *source = Source();
    ScriptParameterRegistry *registry = source ? ScriptParameterRegistry::FromContext(source->GetCKContext()) : nullptr;
    CKDWORD value = 0;
    if (!source || !registry || source->GetValue(&value, TRUE) != CK_OK) {
        return std::string();
    }
    std::string text;
    return registry->EnumNameOf(source->GetGUID(), static_cast<int>(value), text) ? text : std::to_string(value);
}

std::string ParamRef::GetFlagsText() const {
    CKParameter *source = Source();
    ScriptParameterRegistry *registry = source ? ScriptParameterRegistry::FromContext(source->GetCKContext()) : nullptr;
    CKDWORD value = 0;
    if (!source || !registry || source->GetValue(&value, TRUE) != CK_OK) {
        return std::string();
    }
    return registry->FlagsText(source->GetGUID(), value);
}

ParamStructRef *ParamRef::Struct() {
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
    return new ParamStructRef(m_Bridge, target->GetID(), target->GetCKContext());
}

bool ParamRef::SetText(const std::string &text) {
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

NativeBuffer *ParamRef::GetRaw() const {
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

bool ParamRef::SetRaw(NativeBuffer *buffer) {
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

std::string ParamRef::Describe() const {
    if (!IsValid()) {
        return "ParamRef is not valid.";
    }
    return fmt::format("{} '{}' id={} index={} type={} size={}",
        KindName(),
        GetName(),
        Id(),
        m_Index,
        TypeName(),
        DataSize());
}

CKBehavior *ParamRef::OwnerBehavior() const {
    CKContext *context = BridgeContext(m_Bridge, Context());
    return CKBehavior::Cast(GetCKObjectById(context, m_OwnerBehaviorId));
}

CKParameter *ParamRef::WritableParameter(std::string &error) const {
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

bool ParamRef::SetDirectValue(const ScriptParamValue &value) {
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

const char *ParamRef::KindName() const {
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

ParamOperationRef::ParamOperationRef(ScriptBehaviorBridge *bridge,
                                     CK_ID operationId,
                                     CKParameterIn *targetInput,
                                     CKParameter *previousSource,
                                     const std::vector<CK_ID> &ownedLocalSourceIds,
                                     CKContext *context)
    : ObjectRef(BridgeContext(bridge, context), operationId),
      m_Bridge(bridge),
      m_TargetInputId(targetInput ? targetInput->GetID() : 0),
      m_PreviousSourceId(previousSource ? previousSource->GetID() : 0),
      m_TargetInputStamp(CaptureBridgeObjectStamp(targetInput)),
      m_PreviousSourceStamp(CaptureBridgeObjectStamp(previousSource)) {
    CKParameterOperation *operation = Get();
    CKParameterOut *out = operation ? operation->GetOutParameter() : nullptr;
    m_InstalledSourceId = out ? out->GetID() : 0;
    m_InstalledSourceStamp = CaptureBridgeObjectStamp(out);
    CKContext *resolvedContext = BridgeContext(m_Bridge, Context());
    m_OwnedLocalSources.reserve(ownedLocalSourceIds.size());
    for (CK_ID id : ownedLocalSourceIds) {
        CKObject *object = GetCKObjectById(resolvedContext, id);
        if (CKParameterLocal::Cast(object)) {
            m_OwnedLocalSources.push_back(ScriptBridgeStampedObjectId{id, CaptureBridgeObjectStamp(object)});
        }
    }
}

CKParameterOperation *ParamOperationRef::Get() const {
    return CKParameterOperation::Cast(Object());
}

bool ParamOperationRef::IsValid() const { return Get() != nullptr; }
CK_ID ParamOperationRef::GetID() const { return Id(); }

ParamRef *ParamOperationRef::Out() const {
    CKParameterOperation *op = Get();
    CKObject *param = op ? op->GetOutParameter() : nullptr;
    if (!param) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(param, ScriptBridgeSlotKind::OperationOut, 0)
                    : new ParamRef(nullptr, param->GetID(), ScriptBridgeSlotKind::OperationOut, 0, 0, param->GetCKContext());
}

ParamRef *ParamOperationRef::In1() const {
    CKParameterOperation *op = Get();
    CKObject *param = op ? op->GetInParameter1() : nullptr;
    if (!param) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(param, ScriptBridgeSlotKind::OperationIn, 0)
                    : new ParamRef(nullptr, param->GetID(), ScriptBridgeSlotKind::OperationIn, 0, 0, param->GetCKContext());
}

ParamRef *ParamOperationRef::In2() const {
    CKParameterOperation *op = Get();
    CKObject *param = op ? op->GetInParameter2() : nullptr;
    if (!param) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapParameter(param, ScriptBridgeSlotKind::OperationIn, 1)
                    : new ParamRef(nullptr, param->GetID(), ScriptBridgeSlotKind::OperationIn, 1, 0, param->GetCKContext());
}

CKERROR ParamOperationRef::Do() const {
    CKParameterOperation *op = Get();
    return op ? op->DoOperation() : CKERR_INVALIDPARAMETER;
}

bool ParamOperationRef::Restore() {
    if (!m_Bridge) {
        SetScriptException("ParamOperationRef.Restore requires a bridge-owned operation.");
        return false;
    }
    std::string error;
    if (!RestoreTargetSource(error)) {
        SetScriptException(error.empty() ? "Failed to restore operation target source." : error);
        return false;
    }
    return true;
}

bool ParamOperationRef::Destroy() {
    CKParameterOperation *op = Get();
    if (!op || !m_Bridge || !m_Bridge->GetManager() || !m_Bridge->GetManager()->GetCKContext()) {
        if (!m_Bridge) {
            SetScriptException("ParamOperationRef.Destroy requires a bridge-owned operation.");
        }
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
    m_State = OwnershipState::Invalid;
    ClearObject("Parameter operation was destroyed.");
    return true;
}

bool ParamOperationRef::DestroyDetached() {
    CKParameterOperation *op = Get();
    if (!op || !m_Bridge || !m_Bridge->GetManager() || !m_Bridge->GetManager()->GetCKContext()) {
        return false;
    }

    CKBehavior *owner = op->GetOwner();
    if (owner) {
        owner->RemoveParameterOperation(op);
    }
    m_Bridge->GetManager()->GetCKContext()->DestroyObject(op);
    DestroyOwnedLocalSources(owner);
    m_State = OwnershipState::DetachedDestroyed;
    ClearObject("Parameter operation was destroyed.");
    return true;
}

std::string ParamOperationRef::Describe() const {
    CKParameterOperation *op = Get();
    if (!op) return "ParamOperationRef is not valid.";
    const char *state = "BridgeOwned";
    switch (m_State) {
        case OwnershipState::BridgeOwned: state = "BridgeOwned"; break;
        case OwnershipState::TargetRestored: state = "TargetRestored"; break;
        case OwnershipState::DetachedDestroyed: state = "DetachedDestroyed"; break;
        case OwnershipState::Invalid: state = "Invalid"; break;
    }
    CKParameterManager *pm = op->GetCKContext() ? op->GetCKContext()->GetParameterManager() : nullptr;
    CKSTRING name = pm ? pm->OperationGuidToName(op->GetOperationGuid()) : nullptr;
    return fmt::format("Parameter operation '{}' id={} guid={} state={}",
        name && name[0] ? name : SafeString(op->GetName()),
        op->GetID(),
        GuidToString(op->GetOperationGuid()),
        state);
}

CKParameterIn *ParamOperationRef::TargetInput() const {
    CKContext *context = BridgeContext(m_Bridge, Context());
    return CKParameterIn::Cast(GetStampedCKObjectById(context, m_TargetInputId, m_TargetInputStamp));
}

CKParameter *ParamOperationRef::PreviousSource() const {
    if (!m_PreviousSourceId) {
        return nullptr;
    }
    CKContext *context = BridgeContext(m_Bridge, Context());
    return ParameterSourceForConnection(GetStampedCKObjectById(context, m_PreviousSourceId, m_PreviousSourceStamp));
}

CKParameter *ParamOperationRef::InstalledSource() const {
    if (!m_InstalledSourceId) {
        return nullptr;
    }
    CKContext *context = BridgeContext(m_Bridge, Context());
    return ParameterSourceForConnection(GetStampedCKObjectById(context, m_InstalledSourceId, m_InstalledSourceStamp));
}

void ParamOperationRef::DestroyOwnedLocalSources(CKBehavior *owner) {
    CKContext *context = BridgeContext(m_Bridge, Context());
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

bool ParamOperationRef::RestoreTargetSource(std::string &error) {
    if (m_State == OwnershipState::TargetRestored ||
        m_State == OwnershipState::DetachedDestroyed ||
        m_State == OwnershipState::Invalid ||
        !m_TargetInputId) {
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

    m_State = OwnershipState::TargetRestored;
    return true;
}

ParamOp::ParamOp(CKContext *context, const std::string &operationName)
    : m_Context(context) {
    m_Request.OperationName = operationName;
}

ParamOp::ParamOp(CKContext *context, CKGUID operationGuid)
    : m_Context(context) {
    m_Request.OperationGuid = operationGuid;
}

ParamOp *ParamOp::Result(CKGUID guid) {
    m_Request.ResultTypeGuid = guid;
    AddRef();
    return this;
}

ParamOp *ParamOp::ResultName(const std::string &typeName) {
    m_Request.ResultTypeName = typeName;
    AddRef();
    return this;
}

ParamOp *ParamOp::InRef(int slot, ParamRef *source) {
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

ParamOp *ParamOp::InValue(int slot, ParamValue *value) {
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

ScriptBridgeOperationSpec ParamOp::RequestForPin(int pinIndex) const {
    ScriptBridgeOperationSpec request = m_Request;
    request.TargetPinIndex = pinIndex;
    return request;
}

std::string ParamOp::Describe() const {
    CKParameterManager *pm = m_Context ? m_Context->GetParameterManager() : nullptr;
    std::string error;
    const CKGUID guid = ResolveOperationGuid(m_Context, m_Request.OperationGuid, m_Request.OperationName, error);
    CKSTRING name = pm && CKGuidIsValid(guid) ? pm->OperationGuidToName(guid) : nullptr;
    return fmt::format("ParamOp '{}' guid={} result={}",
        name && name[0] ? name : m_Request.OperationName,
        CKGuidIsValid(guid) ? GuidToString(guid) : std::string("<unresolved>"),
        CKGuidIsValid(m_Request.ResultTypeGuid) ? GuidToString(m_Request.ResultTypeGuid) : m_Request.ResultTypeName);
}

ScriptBridgeOperationInput *ParamOp::Slot(int slot) {
    if (slot == 0) return &m_Request.In1;
    if (slot == 1) return &m_Request.In2;
    return nullptr;
}
