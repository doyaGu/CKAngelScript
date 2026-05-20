#ifndef CK_SCRIPTBRIDGEHANDLES_H
#define CK_SCRIPTBRIDGEHANDLES_H

#include "ScriptBridgeCommon.h"
#include "ScriptNativeBuffer.h"

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

class ParamValue final : public RefCounted {
public:
    explicit ParamValue(const ScriptBridgeParamValue &value)
        : m_Value(value) {}

    const ScriptBridgeParamValue &Value() const { return m_Value; }

    bool IsValid() const { return m_Value.ModeKind != ScriptBridgeParamValue::Mode::Empty; }
    CKGUID TypeGuid() const { return m_Value.TypeGuid; }
    std::string TypeName() const { return m_Value.TypeName; }

    int AsInt() const { return m_Value.Value.IntValue; }
    float AsFloat() const { return m_Value.Value.FloatValue; }
    bool AsBool() const { return m_Value.Value.BoolValue; }
    std::string AsString() const { return m_Value.Value.StringValue; }
    CKGUID AsGuid() const { return m_Value.Value.GuidValue; }
    VxVector AsVector() const { return m_Value.Value.VectorValue; }
    Vx2DVector AsVector2() const { return m_Value.Value.Vector2Value; }
    VxColor AsColor() const { return m_Value.Value.ColorValue; }
    VxQuaternion AsQuaternion() const { return m_Value.Value.QuaternionValue; }
    VxMatrix AsMatrix() const { return m_Value.Value.MatrixValue; }

    std::string AsText() const {
        if (m_Value.ModeKind == ScriptBridgeParamValue::Mode::Text) {
            return m_Value.TextValue;
        }
        if (m_Value.Value.Kind == ScriptBridgeValueKind::String || !m_Value.Value.StringValue.empty()) {
            return m_Value.Value.StringValue;
        }
        if (m_Value.Value.Kind == ScriptBridgeValueKind::Guid) {
            return ScriptGuidToString(m_Value.Value.GuidValue);
        }
        if (m_Value.Value.Kind == ScriptBridgeValueKind::Int) {
            return std::to_string(m_Value.Value.IntValue);
        }
        if (m_Value.Value.Kind == ScriptBridgeValueKind::Float) {
            return fmt::format("{}", m_Value.Value.FloatValue);
        }
        if (m_Value.Value.Kind == ScriptBridgeValueKind::Bool) {
            return m_Value.Value.BoolValue ? "true" : "false";
        }
        return std::string();
    }

    NativeBuffer *AsRaw() const {
        if (m_Value.ModeKind != ScriptBridgeParamValue::Mode::Raw || m_Value.RawData.empty()) {
            return NativeBuffer::Create(0);
        }
        NativeBuffer *buffer = NativeBuffer::Create(m_Value.RawData.size());
        buffer->Write(const_cast<char *>(m_Value.RawData.data()), m_Value.RawData.size());
        buffer->Reset();
        return buffer;
    }

private:
    ScriptBridgeParamValue m_Value;
};

class BehaviorLayout final : public RefCounted {
public:
    BehaviorLayout(ScriptBehaviorBridge *bridge, CK_ID behaviorId)
        : m_Bridge(bridge), m_BehaviorId(behaviorId), m_IsPrototype(false) {}

    BehaviorLayout(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request)
        : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_IsPrototype(true) {}

    int InputCount() const {
        if (CKBehavior *behavior = Behavior()) return behavior->GetInputCount();
        if (CKBehaviorPrototype *prototype = Prototype()) return prototype->GetInputCount();
        return 0;
    }

    int OutputCount() const {
        if (CKBehavior *behavior = Behavior()) return behavior->GetOutputCount();
        if (CKBehaviorPrototype *prototype = Prototype()) return prototype->GetOutputCount();
        return 0;
    }

    int PinCount() const {
        if (CKBehavior *behavior = Behavior()) return behavior->GetInputParameterCount();
        if (CKBehaviorPrototype *prototype = Prototype()) return prototype->GetInParameterCount();
        return 0;
    }

    int PoutCount() const {
        if (CKBehavior *behavior = Behavior()) return behavior->GetOutputParameterCount();
        if (CKBehaviorPrototype *prototype = Prototype()) return prototype->GetOutParameterCount();
        return 0;
    }

    int LocalCount() const {
        if (CKBehavior *behavior = Behavior()) return behavior->GetLocalParameterCount();
        if (CKBehaviorPrototype *prototype = Prototype()) return prototype->GetLocalParameterCount();
        return 0;
    }

    std::string InputName(int index) const {
        if (CKBehavior *behavior = Behavior()) {
            CKBehaviorIO *io = index >= 0 && index < behavior->GetInputCount() ? behavior->GetInput(index) : nullptr;
            return io ? SafeString(io->GetName()) : std::string();
        }
        CKBEHAVIORIO_DESC *io = GetPrototypeInput(Prototype(), index);
        return io ? SafeString(io->Name) : std::string();
    }

    std::string OutputNameAt(int index) const {
        if (CKBehavior *behavior = Behavior()) {
            CKBehaviorIO *io = index >= 0 && index < behavior->GetOutputCount() ? behavior->GetOutput(index) : nullptr;
            return io ? SafeString(io->GetName()) : std::string();
        }
        CKBEHAVIORIO_DESC *io = GetPrototypeOutput(Prototype(), index);
        return io ? SafeString(io->Name) : std::string();
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

    CKBehavior *Behavior() const {
        if (m_IsPrototype || !m_Bridge || !m_Bridge->GetManager()) return nullptr;
        return CKBehavior::Cast(GetCKObjectById(m_Bridge->GetManager()->GetCKContext(), m_BehaviorId));
    }

    CKBehaviorPrototype *Prototype() const {
        if (!m_IsPrototype || !m_Bridge) return nullptr;
        std::string error;
        return m_Bridge->ResolvePrototypeObject(m_Request, error);
    }

    ParamInfo *ParameterInfo(ScriptBridgeSlotKind kind, int index) const {
        CKContext *context = Context();
        if (CKBehavior *behavior = Behavior()) {
            CKParameter *param = nullptr;
            if (kind == ScriptBridgeSlotKind::Pin) {
                if (index < 0 || index >= behavior->GetInputParameterCount()) return nullptr;
                CKParameterIn *pin = behavior->GetInputParameter(index);
                CKParameter *source = pin ? pin->GetRealSource() : nullptr;
                const int dataSize = source ? source->GetDataSize() : ParameterDefaultSize(context, pin ? pin->GetGUID() : CKGUID());
                return pin ? new ParamInfo(kind, index, SafeString(pin->GetName()), pin->GetGUID(), ParameterTypeLabel(context, pin->GetGUID()), dataSize) : nullptr;
            }
            if (kind == ScriptBridgeSlotKind::Pout) {
                param = index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
            } else if (kind == ScriptBridgeSlotKind::Local) {
                param = index >= 0 && index < behavior->GetLocalParameterCount() ? behavior->GetLocalParameter(index) : nullptr;
            }
            return param ? new ParamInfo(kind, index, SafeString(param->GetName()), param->GetGUID(), ParameterTypeLabel(context, param->GetGUID()), param->GetDataSize()) : nullptr;
        }

        CKBehaviorPrototype *prototype = Prototype();
        CKPARAMETER_DESC *desc = nullptr;
        if (kind == ScriptBridgeSlotKind::Pin) {
            desc = GetPrototypeInputParameter(prototype, index);
        } else if (kind == ScriptBridgeSlotKind::Pout) {
            desc = GetPrototypeOutputParameter(prototype, index);
        } else if (kind == ScriptBridgeSlotKind::Local) {
            desc = GetPrototypeLocalParameter(prototype, index);
        }
        if (!desc) return nullptr;
        int dataSize = 0;
        CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
        CKParameterTypeDesc *typeDesc = pm ? pm->GetParameterTypeDescription(desc->Guid) : nullptr;
        if (typeDesc) dataSize = typeDesc->DefaultSize;
        return new ParamInfo(kind, index, SafeString(desc->Name), desc->Guid, ParameterTypeLabel(context, desc->Guid), dataSize);
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
          m_OwnerBehaviorId(ownerBehaviorId) {}

    CKObject *Get() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return GetCKObjectById(context, m_ParameterId);
    }

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

    ParamValue *GetValue() const {
        CKParameter *source = Source();
        if (!source) {
            return new ParamValue(ScriptBridgeParamValue());
        }
        ScriptBridgeParamValue value = MakeBridgeParamValue(ReadParameterValue(source));
        value.TypeGuid = source->GetGUID();
        value.TypeName = ParameterTypeLabel(source->GetCKContext(), source->GetGUID());
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
        CKERROR err = target->CopyValue(source);
        if (err != CK_OK) {
            SetScriptException(fmt::format("Failed to copy parameter value (expected {}, got {}, CKERROR {}).",
                ParameterTypeLabel(target->GetCKContext(), target->GetGUID()),
                ParameterTypeLabel(source->GetCKContext(), source->GetGUID()),
                err));
            return false;
        }
        return true;
    }

    std::string GetText() const {
        std::string text;
        if (ReadParameterString(Source(), text)) {
            return text;
        }
        return std::string();
    }

    bool SetText(const std::string &text) {
        std::string error;
        CKParameter *target = WritableParameter(error);
        if (!target) {
            SetScriptException(error);
            return false;
        }
        CKERROR err = target->SetStringValue(const_cast<CKSTRING>(text.c_str()));
        if (err != CK_OK) {
            SetScriptException(fmt::format("Failed to set parameter text for '{}' (CKERROR {}).", SafeString(target->GetName()), err));
            return false;
        }
        return true;
    }

    NativeBuffer *GetRaw() const {
        CKParameter *source = Source();
        const int size = source ? source->GetDataSize() : 0;
        NativeBuffer *buffer = NativeBuffer::Create(size > 0 ? static_cast<size_t>(size) : 0);
        if (!source || size <= 0) {
            return buffer;
        }
        void *data = source->GetReadDataPtr(TRUE);
        if (data) {
            buffer->Write(data, static_cast<size_t>(size));
            buffer->Reset();
        }
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
        ScriptBridgeParamValue value = MakeBridgeRawValue(target->GetGUID(), TypeName(), buffer->Data(), static_cast<int>(buffer->Size()));
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
};

class ParamOperationRef final : public RefCounted {
public:
    ParamOperationRef(ScriptBehaviorBridge *bridge, CK_ID operationId)
        : m_Bridge(bridge), m_OperationId(operationId) {}

    CKParameterOperation *Get() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return CKParameterOperation::Cast(GetCKObjectById(context, m_OperationId));
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

    bool Destroy() {
        CKParameterOperation *op = Get();
        if (!op || !m_Bridge || !m_Bridge->GetManager() || !m_Bridge->GetManager()->GetCKContext()) {
            return false;
        }
        if (CKBehavior *owner = op->GetOwner()) {
            owner->RemoveParameterOperation(op);
        }
        m_Bridge->GetManager()->GetCKContext()->DestroyObject(op);
        m_OperationId = 0;
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
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_OperationId = 0;
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
        *input = ScriptBridgeOperationInput();
        input->SourceId = source ? source->GetID() : 0;
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
            input->HasValue = true;
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

ParamOperationRef *ConnectOperationToInput(ScriptBehaviorBridge *bridge,
                                           CKBehavior *behavior,
                                           int pinIndex,
                                           const ScriptBridgeOperationSpec &request,
                                           std::string &error,
                                           bool allowOwnerOnly,
                                           std::vector<CK_ID> *operationIds);

class BehaviorRef final : public RefCounted {
public:
    BehaviorRef(ScriptBehaviorBridge *bridge, CK_ID behaviorId, CK_ID componentId)
        : m_Bridge(bridge), m_BehaviorId(behaviorId), m_ComponentId(componentId) {}

    CKBehavior *Get() const {
        CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
        return CKBehavior::Cast(GetCKObjectById(context, m_BehaviorId));
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
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_BehaviorId = 0;
    CK_ID m_ComponentId = 0;
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
        if (value) m_Request.IndexedParameters[pinIndex] = value->Value();
        AddRef();
        return this;
    }

    BBCallBuilder *SetSource(int pinIndex, ParamRef *source) {
        m_Request.SourceParameters.push_back(ScriptBridgeInputSource{pinIndex, source ? source->GetID() : 0});
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
    BBTaskBuilder *Set(int pinIndex, ParamValue *value) { if (value) m_Request.IndexedParameters[pinIndex] = value->Value(); AddRef(); return this; }
    BBTaskBuilder *SetSource(int pinIndex, ParamRef *source) { m_Request.SourceParameters.push_back(ScriptBridgeInputSource{pinIndex, source ? source->GetID() : 0}); AddRef(); return this; }
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
