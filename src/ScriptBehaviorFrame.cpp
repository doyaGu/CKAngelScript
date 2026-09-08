#include <angelscript.h>

#include <fmt/format.h>

#include "ScriptBehaviorFrame.h"

#include "CKAll.h"
#include "ScriptAsync.h"
#include "ScriptBridgeHandles.h"
#include "ScriptHostScope.h"
#include "ScriptManager.h"
#include "ScriptNativeBuffer.h"
#include "ScriptParameterConversion.h"

#undef GetObject

namespace {

CKBehavior *RequireBehavior(CKBehavior *behavior, const char *method) {
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method ? method : "BehaviorFrame"));
    }
    return behavior;
}

bool ValidIndex(int index, int count, const char *method, const char *kind) {
    if (index >= 0 && index < count) {
        return true;
    }
    SetScriptException(fmt::format("{} index {} is outside {} range 0..{}.",
        method ? method : "BehaviorFrame",
        index,
        kind ? kind : "slot",
        count > 0 ? count - 1 : -1));
    return false;
}

NativeBuffer *ParameterReadBuffer(CKParameter *parameter, bool update = true) {
    if (!parameter) {
        return NativeBuffer::Create(0);
    }
    const int size = parameter->GetDataSize();
    void *data = size > 0 ? parameter->GetReadDataPtr(update ? TRUE : FALSE) : nullptr;
    return data ? NativeBuffer::Create(data, static_cast<size_t>(size)) : NativeBuffer::Create(0);
}

NativeBuffer *ParameterWriteBuffer(CKParameter *parameter) {
    if (!parameter) {
        return NativeBuffer::Create(0);
    }
    const int size = parameter->GetDataSize();
    void *data = size > 0 ? parameter->GetWriteDataPtr() : nullptr;
    return data ? NativeBuffer::Create(data, static_cast<size_t>(size)) : NativeBuffer::Create(0);
}

ScriptAsyncScheduler *RequireAsyncScheduler(CKContext *context, const char *method) {
    if (!context) {
        SetScriptException(fmt::format("{} requires a CKContext.", method ? method : "BehaviorFrame async API"));
        return nullptr;
    }
    ScriptAsyncScheduler *scheduler = ScriptHostScope::From(context).AsyncScheduler();
    if (!scheduler) {
        SetScriptException(fmt::format("{} requires an async scheduler.", method ? method : "BehaviorFrame async API"));
        return nullptr;
    }
    return scheduler;
}

CKBeObject *ResolveMessageWaitObject(CKBehavior *behavior, CKBeObject *explicitObject) {
    if (explicitObject) {
        return explicitObject;
    }
    if (!behavior) {
        return nullptr;
    }
    if (CKBeObject *target = behavior->GetTarget()) {
        return target;
    }
    return behavior->GetOwner();
}

template <typename ParamGetter>
int ResolveNamedParameter(CKBehavior *behavior,
                          const std::string &name,
                          int occurrence,
                          int count,
                          ParamGetter getter) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    if (name.empty()) {
        return count > 0 && occurrence == 0 ? 0 : -1;
    }

    int seen = 0;
    for (int i = 0; i < count; ++i) {
        auto *param = getter(i);
        if (!param || !NameEquals(param->GetName(), name)) {
            continue;
        }
        if (seen == occurrence) {
            return i;
        }
        ++seen;
    }
    return -1;
}

int ResolveNamedIo(CKBehavior *behavior, const std::string &name, int occurrence, bool input) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    const int count = input ? behavior->GetInputCount() : behavior->GetOutputCount();
    if (name.empty()) {
        return count > 0 && occurrence == 0 ? 0 : -1;
    }

    int seen = 0;
    for (int i = 0; i < count; ++i) {
        CKBehaviorIO *io = input ? behavior->GetInput(i) : behavior->GetOutput(i);
        if (!io || !NameEquals(io->GetName(), name)) {
            continue;
        }
        if (seen == occurrence) {
            return i;
        }
        ++seen;
    }
    return -1;
}

std::string DescribeIoNames(CKBehavior *behavior, bool input) {
    if (!behavior) {
        return "<behavior is null>";
    }
    const int count = input ? behavior->GetInputCount() : behavior->GetOutputCount();
    std::string result;
    for (int i = 0; i < count; ++i) {
        CKBehaviorIO *io = input ? behavior->GetInput(i) : behavior->GetOutput(i);
        if (!result.empty()) {
            result += ", ";
        }
        result += fmt::format("#{} '{}'", i, SafeString(io ? io->GetName() : nullptr));
    }
    return result.empty() ? "<none>" : result;
}

int SettingCount(CKBehavior *behavior) {
    if (!behavior) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        if (behavior->IsLocalParameterSetting(i)) {
            ++count;
        }
    }
    return count;
}

int FindSettingIndex(CKBehavior *behavior, const std::string &name, int occurrence) {
    if (!behavior || occurrence < 0) {
        return -1;
    }
    if (name.empty()) {
        if (occurrence != 0) {
            return -1;
        }
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            if (behavior->IsLocalParameterSetting(i)) {
                return i;
            }
        }
        return -1;
    }

    int seen = 0;
    for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
        if (!behavior->IsLocalParameterSetting(i)) {
            continue;
        }
        CKParameterLocal *param = behavior->GetLocalParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            if (seen == occurrence) {
                return i;
            }
            ++seen;
        }
    }
    return -1;
}

std::string DescribeParameterNames(CKBehavior *behavior, ScriptBridgeSlotKind kind) {
    if (!behavior) {
        return "<behavior is null>";
    }

    std::string result;
    auto append = [&](const std::string &text) {
        if (!result.empty()) {
            result += ", ";
        }
        result += text;
    };

    if (kind == ScriptBridgeSlotKind::Pin) {
        for (int i = 0; i < behavior->GetInputParameterCount(); ++i) {
            CKParameterIn *param = behavior->GetInputParameter(i);
            append(fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr)));
        }
    } else if (kind == ScriptBridgeSlotKind::Pout) {
        for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
            CKParameterOut *param = behavior->GetOutputParameter(i);
            append(fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr)));
        }
    } else if (kind == ScriptBridgeSlotKind::Setting) {
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            if (!behavior->IsLocalParameterSetting(i)) {
                continue;
            }
            CKParameterLocal *param = behavior->GetLocalParameter(i);
            append(fmt::format("#{} '{}'",
                i,
                SafeString(param ? param->GetName() : nullptr)));
        }
    } else {
        for (int i = 0; i < behavior->GetLocalParameterCount(); ++i) {
            CKParameterLocal *param = behavior->GetLocalParameter(i);
            append(fmt::format("#{} '{}'{}",
                i,
                SafeString(param ? param->GetName() : nullptr),
                behavior->IsLocalParameterSetting(i) ? " (setting)" : ""));
        }
    }

    return result.empty() ? "<none>" : result;
}

template <typename Setter>
bool ApplyParamRef(ParamRef *ref, Setter setter) {
    if (!ref) {
        return false;
    }
    const bool ok = setter(ref);
    ref->Release();
    return ok;
}

template <int SlotKind>
void BehaviorFrameGetParameterValueGeneric(asIScriptGeneric *generic) {
    auto *frame = static_cast<BehaviorFrame *>(generic->GetObject());
    CKBehavior *behavior = RequireBehavior(frame ? frame->Behavior() : nullptr, "BehaviorFrame.GetParameterValue");
    CKERROR err = ReadBehaviorParameterValue(behavior,
                                             static_cast<ScriptBehaviorParameterSlot>(SlotKind),
                                             generic->GetArgDWord(0),
                                             generic->GetArgTypeId(1),
                                             generic->GetArgAddress(1),
                                             generic->GetEngine(),
                                             asGetActiveContext());
    generic->SetReturnDWord(err);
}

template <int SlotKind>
void BehaviorFrameSetParameterValueGeneric(asIScriptGeneric *generic) {
    auto *frame = static_cast<BehaviorFrame *>(generic->GetObject());
    CKBehavior *behavior = RequireBehavior(frame ? frame->Behavior() : nullptr, "BehaviorFrame.SetParameterValue");
    CKERROR err = WriteBehaviorParameterValue(behavior,
                                              static_cast<ScriptBehaviorParameterSlot>(SlotKind),
                                              generic->GetArgDWord(0),
                                              generic->GetArgTypeId(1),
                                              generic->GetArgAddress(1),
                                              generic->GetEngine(),
                                              asGetActiveContext());
    generic->SetReturnDWord(err);
}

} // namespace

void BehaviorFrameGetInputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<0>(generic);
}

void BehaviorFrameGetOutputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<1>(generic);
}

void BehaviorFrameSetOutputParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameSetParameterValueGeneric<1>(generic);
}

void BehaviorFrameGetLocalParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameGetParameterValueGeneric<2>(generic);
}

void BehaviorFrameSetLocalParameterValueGeneric(asIScriptGeneric *generic) {
    BehaviorFrameSetParameterValueGeneric<2>(generic);
}

BehaviorFrame::BehaviorFrame(const CKBehaviorContext &ctx) {
    Refresh(ctx);
}

void BehaviorFrame::Refresh(const CKBehaviorContext &ctx) {
    m_ContextSnapshot = ctx;
    m_Context = ScriptHostScope::From(ctx).Context();
    m_BehaviorId = ctx.Behavior ? ctx.Behavior->GetID() : 0;
    m_BehaviorStamp = CaptureBridgeObjectStamp(ctx.Behavior);
    // Original CK2 marks objects CK_OBJECT_TOBEDELETED before it calls
    // SequenceToBeDeleted. A frame created for the synchronous terminal
    // lifecycle callback must still be able to inspect that exact behavior.
    // The captured identity continues to reject ID reuse, and normal object
    // references retain their stricter live-only policy.
    m_BehaviorResolvePolicy = ctx.Behavior && ctx.Behavior->IsToBeDeleted()
        ? ScriptBridgeObjectResolvePolicy::IncludePendingDeletion
        : ScriptBridgeObjectResolvePolicy::LiveOnly;
    m_ContextSnapshot.Behavior = nullptr;
    m_ContextSnapshot.Context = m_Context;
}

CKBehavior *BehaviorFrame::Behavior() const {
    return CKBehavior::Cast(GetStampedCKObjectById(
        m_Context,
        m_BehaviorId,
        m_BehaviorStamp,
        m_BehaviorResolvePolicy));
}

CKContext *BehaviorFrame::Context() const {
    return m_Context;
}

float BehaviorFrame::DeltaTime() const {
    return m_ContextSnapshot.DeltaTime;
}

bool BehaviorFrame::IsValid() const {
    return Behavior() != nullptr;
}

CK_ID BehaviorFrame::Id() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetID() : 0;
}

std::string BehaviorFrame::Name() const {
    CKBehavior *behavior = Behavior();
    return behavior ? SafeString(behavior->GetName()) : std::string();
}

BehaviorRef *BehaviorFrame::Self() const {
    return WrapBehavior(Behavior());
}

BehaviorRef *BehaviorFrame::Parent() const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior ? behavior->GetParent() : nullptr);
}

BehaviorRef *BehaviorFrame::OwnerScript() const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior ? behavior->GetOwnerScript() : nullptr);
}

BehaviorFrame *BehaviorFrame::FrameFor(CKBehavior *target) const {
    if (!target) {
        SetScriptException("BehaviorFrame.FrameFor requires a live behavior.");
        return nullptr;
    }

    CKContext *targetContext = target->GetCKContext();
    if (m_Context && targetContext && targetContext != m_Context) {
        SetScriptException("BehaviorFrame.FrameFor requires a behavior from the same CKContext.");
        return nullptr;
    }

    CKBehaviorContext ctx = MakeContext();
    ctx.Context = targetContext ? targetContext : m_Context;
    ctx.Behavior = target;
    return new BehaviorFrame(ctx);
}

BehaviorFrame *BehaviorFrame::FrameForRef(BehaviorRef *target) const {
    return FrameFor(target ? target->Get() : nullptr);
}

BehaviorFrame *BehaviorFrame::SubFrame(int index) const {
    CKBehavior *behavior = RequireBehavior(Behavior(), "BehaviorFrame.SubFrame");
    if (!behavior) {
        return nullptr;
    }
    if (index < 0 || index >= behavior->GetSubBehaviorCount()) {
        SetScriptException(fmt::format(
            "BehaviorFrame.SubFrame index {} is out of range 0..{}.",
            index,
            behavior->GetSubBehaviorCount() - 1));
        return nullptr;
    }
    return FrameFor(behavior->GetSubBehavior(index));
}

BehaviorLayout *BehaviorFrame::Layout() const {
    ScriptBehaviorBridge *bridge = Bridge();
    CKBehavior *behavior = Behavior();
    return bridge && behavior ? new BehaviorLayout(bridge, behavior->GetID()) : nullptr;
}

BehaviorGraph *BehaviorFrame::Graph() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Graph requires ScriptBehaviorBridge.");
        return nullptr;
    }
    BehaviorBridge *behavior = bridge->CreateBehaviorBridge(MakeContext());
    if (!behavior) {
        return nullptr;
    }
    BehaviorGraph *graph = behavior->Graph();
    behavior->Release();
    return graph;
}

BehaviorGraph *BehaviorFrame::AsGraph() const {
    BehaviorRef *self = Self();
    BehaviorGraph *graph = self ? self->AsGraph() : nullptr;
    if (self) {
        self->Release();
    }
    return graph;
}

int BehaviorFrame::InputCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetInputCount() : 0;
}

int BehaviorFrame::OutputCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOutputCount() : 0;
}

int BehaviorFrame::PinCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetInputParameterCount() : 0;
}

int BehaviorFrame::PoutCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOutputParameterCount() : 0;
}

int BehaviorFrame::LocalCount() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        return bridge->FrameLocalCount(m_BehaviorId);
    }
    return BehaviorLocalCount();
}

int BehaviorFrame::BehaviorLocalCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetLocalParameterCount() : 0;
}

int BehaviorFrame::SettingCount() const {
    return ::SettingCount(Behavior());
}

int BehaviorFrame::SubBehaviorCount() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetSubBehaviorCount() : 0;
}

std::string BehaviorFrame::InputName(int index) const {
    CKBehavior *behavior = Behavior();
    CKBehaviorIO *io = behavior && index >= 0 && index < behavior->GetInputCount() ? behavior->GetInput(index) : nullptr;
    return io ? SafeString(io->GetName()) : std::string();
}

std::string BehaviorFrame::OutputName(int index) const {
    CKBehavior *behavior = Behavior();
    CKBehaviorIO *io = behavior && index >= 0 && index < behavior->GetOutputCount() ? behavior->GetOutput(index) : nullptr;
    return io ? SafeString(io->GetName()) : std::string();
}

int BehaviorFrame::FindInput(const std::string &name, int occurrence) const {
    return ResolveNamedIo(Behavior(), name, occurrence, true);
}

int BehaviorFrame::FindOutput(const std::string &name, int occurrence) const {
    return ResolveNamedIo(Behavior(), name, occurrence, false);
}

int BehaviorFrame::FindPin(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetInputParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetInputParameter(i); });
}

int BehaviorFrame::FindPout(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetOutputParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetOutputParameter(i); });
}

int BehaviorFrame::FindLocal(const std::string &name, int occurrence) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        return bridge->FindFrameLocal(m_BehaviorId, name, occurrence);
    }
    return FindBehaviorLocal(name, occurrence);
}

int BehaviorFrame::FindBehaviorLocal(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    return ResolveNamedParameter(behavior,
                                 name,
                                 occurrence,
                                 behavior ? behavior->GetLocalParameterCount() : 0,
                                 [behavior](int i) { return behavior->GetLocalParameter(i); });
}

int BehaviorFrame::FindSetting(const std::string &name, int occurrence) const {
    return FindSettingIndex(Behavior(), name, occurrence);
}

bool BehaviorFrame::InputActive(const std::string &name, int occurrence) const {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.InputActive");
    return InputActiveIndex(index);
}

bool BehaviorFrame::InputActiveIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputCount() && behavior->IsInputActive(index);
}

bool BehaviorFrame::Take(const std::string &name, int occurrence) {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.Take");
    return TakeIndex(index);
}

bool BehaviorFrame::TakeIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (!behavior || index < 0 || index >= behavior->GetInputCount()) {
        return false;
    }
    const bool active = behavior->IsInputActive(index);
    if (active) {
        behavior->ActivateInput(index, FALSE);
    }
    return active;
}

void BehaviorFrame::Emit(const std::string &name, int occurrence) {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.Emit");
    EmitIndex(index);
}

void BehaviorFrame::EmitIndex(int index) {
    CKBehavior *behavior = Behavior();
    if (behavior && index >= 0 && index < behavior->GetOutputCount()) {
        behavior->ActivateOutput(index, TRUE);
    }
}

bool BehaviorFrame::OutputActive(const std::string &name, int occurrence) const {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.OutputActive");
    return OutputActiveIndex(index);
}

bool BehaviorFrame::OutputActiveIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputCount() && behavior->IsOutputActive(index);
}

void BehaviorFrame::ActivateInput(const std::string &name, bool active, int occurrence) {
    const int index = ResolveInput(name, occurrence, "BehaviorFrame.ActivateInput");
    ActivateInputIndex(index, active);
}

void BehaviorFrame::ActivateInputIndex(int index, bool active) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetInputCount(), "BehaviorFrame.ActivateInput", "input")) {
        behavior->ActivateInput(index, active);
    }
}

void BehaviorFrame::ActivateOutput(const std::string &name, bool active, int occurrence) {
    const int index = ResolveOutput(name, occurrence, "BehaviorFrame.ActivateOutput");
    ActivateOutputIndex(index, active);
}

void BehaviorFrame::ActivateOutputIndex(int index, bool active) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetOutputCount(), "BehaviorFrame.ActivateOutput", "output")) {
        behavior->ActivateOutput(index, active);
    }
}

CKParameterLocal *BehaviorFrame::LocalParameter(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount() ? behavior->GetLocalParameter(index) : nullptr;
}

BehaviorRef *BehaviorFrame::SubBehavior(int index) const {
    CKBehavior *behavior = Behavior();
    return WrapBehavior(behavior && index >= 0 && index < behavior->GetSubBehaviorCount() ? behavior->GetSubBehavior(index) : nullptr);
}

ParamRef *BehaviorFrame::Pin(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.Pin");
    return behavior ? PinIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::PinIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
    return WrapParameter(pin, ScriptBridgeSlotKind::Pin, index);
}

ParamRef *BehaviorFrame::Pout(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.Pout");
    return behavior ? PoutIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::PoutIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameter(index)
        : nullptr;
    return WrapParameter(pout, ScriptBridgeSlotKind::Pout, index);
}

ParamRef *BehaviorFrame::Local(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolveLocal(name, occurrence, "BehaviorFrame.Local");
    return behavior ? LocalIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::LocalIndex(int index) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        std::string error;
        CKParameter *local = bridge->ResolveFrameLocal(m_Context, m_BehaviorId, index, error);
        if (!local) {
            SetScriptException(error.empty()
                ? fmt::format("BehaviorFrame.Local could not resolve logical local index {}.", index)
                : error);
            return nullptr;
        }
        return WrapParameter(local, ScriptBridgeSlotKind::Local, index);
    }
    return BehaviorLocalIndex(index);
}

ParamRef *BehaviorFrame::BehaviorLocal(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolveBehaviorLocal(name, occurrence, "BehaviorFrame.BehaviorLocal");
    return behavior ? BehaviorLocalIndex(index) : nullptr;
}

ParamRef *BehaviorFrame::BehaviorLocalIndex(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterLocal *local = behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(index)
        : nullptr;
    return WrapParameter(local, ScriptBridgeSlotKind::Local, index);
}

ParamRef *BehaviorFrame::Setting(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int localIndex = ResolveSetting(name, occurrence, "BehaviorFrame.Setting");
    CKParameterLocal *setting = behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(localIndex)
        : nullptr;
    return WrapParameter(setting, ScriptBridgeSlotKind::Setting, localIndex);
}

ParamRef *BehaviorFrame::SettingIndex(int index) const {
    CKBehavior *behavior = Behavior();
    const int localIndex = ResolveSettingIndex(index, "BehaviorFrame.Setting");
    CKParameterLocal *setting = behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(localIndex)
        : nullptr;
    return WrapParameter(setting, ScriptBridgeSlotKind::Setting, localIndex);
}

std::string BehaviorFrame::PinString(const std::string &name, const std::string &defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

std::string BehaviorFrame::PinStringIndex(int index, const std::string &defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

int BehaviorFrame::PinInt(const std::string &name, int defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

int BehaviorFrame::PinIntIndex(int index, int defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

bool BehaviorFrame::PinBool(const std::string &name, bool defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

bool BehaviorFrame::PinBoolIndex(int index, bool defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

float BehaviorFrame::PinFloat(const std::string &name, float defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

float BehaviorFrame::PinFloatIndex(int index, float defaultValue) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

CKObject *BehaviorFrame::PinObject(const std::string &name, int occurrence) const {
    ParamRef *ref = Pin(name, occurrence);
    if (!ref) {
        return nullptr;
    }
    CKParameter *source = ref->Source();
    CKObject *object = source ? source->GetValueObject(TRUE) : nullptr;
    ref->Release();
    return object;
}

CKObject *BehaviorFrame::PinObjectIndex(int index) const {
    ParamRef *ref = PinIndex(index);
    if (!ref) {
        return nullptr;
    }
    CKParameter *source = ref->Source();
    CKObject *object = source ? source->GetValueObject(TRUE) : nullptr;
    ref->Release();
    return object;
}

std::string BehaviorFrame::LocalString(const std::string &name, const std::string &defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

std::string BehaviorFrame::LocalStringIndex(int index, const std::string &defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::String || value.Kind == ScriptParamValueKind::Text) {
        return value.Text();
    }
    return defaultValue;
}

int BehaviorFrame::LocalInt(const std::string &name, int defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

int BehaviorFrame::LocalIntIndex(int index, int defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Int) {
        return value.Data.IntValue;
    }
    if (value.Kind == ScriptParamValueKind::Enum || value.Kind == ScriptParamValueKind::Flags) {
        return static_cast<int>(value.Data.DwordValue);
    }
    return defaultValue;
}

bool BehaviorFrame::LocalBool(const std::string &name, bool defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

bool BehaviorFrame::LocalBoolIndex(int index, bool defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    return value.Kind == ScriptParamValueKind::Bool ? value.Data.BoolValue : defaultValue;
}

float BehaviorFrame::LocalFloat(const std::string &name, float defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(name, occurrence);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

float BehaviorFrame::LocalFloatIndex(int index, float defaultValue) const {
    const ScriptParamValue value = ReadLocalValueOrEmpty(index);
    if (value.Kind == ScriptParamValueKind::Float) {
        return value.Data.FloatValue;
    }
    if (value.Kind == ScriptParamValueKind::Int) {
        return static_cast<float>(value.Data.IntValue);
    }
    return defaultValue;
}

CKObject *BehaviorFrame::LocalObject(const std::string &name, int occurrence) const {
    CKParameter *local = FindLocalSource(name, occurrence);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

CKObject *BehaviorFrame::BehaviorLocalObject(const std::string &name, int occurrence) const {
    CKParameterLocal *local = FindLocalParameter(name, occurrence);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

CKObject *BehaviorFrame::BehaviorLocalObjectIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameterObject(index)
        : nullptr;
}

bool BehaviorFrame::PinEnabled(const std::string &name, int occurrence) const {
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.PinEnabled");
    return PinEnabledIndex(index);
}

bool BehaviorFrame::PinEnabledIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputParameterCount() && behavior->IsInputParameterEnabled(index);
}

void BehaviorFrame::EnablePin(const std::string &name, bool enable, int occurrence) {
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.EnablePin");
    EnablePinIndex(index, enable);
}

void BehaviorFrame::EnablePinIndex(int index, bool enable) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetInputParameterCount(), "BehaviorFrame.EnablePin", "pin")) {
        behavior->EnableInputParameter(index, enable);
    }
}

NativeBuffer *BehaviorFrame::PinReadData(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
    return ParameterReadBuffer(pin ? pin->GetRealSource() : nullptr);
}

bool BehaviorFrame::PoutEnabled(const std::string &name, int occurrence) const {
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.PoutEnabled");
    return PoutEnabledIndex(index);
}

bool BehaviorFrame::PoutEnabledIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputParameterCount() && behavior->IsOutputParameterEnabled(index);
}

void BehaviorFrame::EnablePout(const std::string &name, bool enable, int occurrence) {
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.EnablePout");
    EnablePoutIndex(index, enable);
}

void BehaviorFrame::EnablePoutIndex(int index, bool enable) {
    CKBehavior *behavior = Behavior();
    if (behavior && ValidIndex(index, behavior->GetOutputParameterCount(), "BehaviorFrame.EnablePout", "pout")) {
        behavior->EnableOutputParameter(index, enable);
    }
}

NativeBuffer *BehaviorFrame::PoutWriteData(int index) const {
    CKBehavior *behavior = Behavior();
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameter(index)
        : nullptr;
    return ParameterWriteBuffer(pout);
}

CKObject *BehaviorFrame::PoutObject(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameterObject(index)
        : nullptr;
}

CKERROR BehaviorFrame::SetOutputParameterObject(int index, CKObject *object) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->SetOutputParameterObject(index, object) : CKERR_INVALIDOBJECT;
}

bool BehaviorFrame::SetPoutInt(const std::string &name, int value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetPoutIntIndex(int index, int value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetPoutFloat(const std::string &name, float value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetPoutFloatIndex(int index, float value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetPoutBool(const std::string &name, bool value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetPoutBoolIndex(int index, bool value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetPoutString(const std::string &name, const std::string &value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetPoutStringIndex(int index, const std::string &value) {
    return ApplyParamRef(PoutIndex(index), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetPoutObject(const std::string &name, CKObject *value, int occurrence) {
    return ApplyParamRef(Pout(name, occurrence), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetPoutObjectIndex(int index, CKObject *value) {
    return ApplyParamRef(PoutIndex(index), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetLocalInt(const std::string &name, int value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetLocalIntIndex(int index, int value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetInt(value); });
}

bool BehaviorFrame::SetLocalFloat(const std::string &name, float value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetLocalFloatIndex(int index, float value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetFloat(value); });
}

bool BehaviorFrame::SetLocalBool(const std::string &name, bool value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetLocalBoolIndex(int index, bool value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetBool(value); });
}

bool BehaviorFrame::SetLocalString(const std::string &name, const std::string &value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetLocalStringIndex(int index, const std::string &value) {
    return ApplyParamRef(LocalIndex(index), [&value](ParamRef *ref) { return ref->SetString(value); });
}

bool BehaviorFrame::SetLocalObject(const std::string &name, CKObject *value, int occurrence) {
    return ApplyParamRef(Local(name, occurrence), [value](ParamRef *ref) { return ref->SetObject(value); });
}

bool BehaviorFrame::SetLocalObjectIndex(int index, CKObject *value) {
    return ApplyParamRef(LocalIndex(index), [value](ParamRef *ref) { return ref->SetObject(value); });
}

NativeBuffer *BehaviorFrame::LocalWriteData(int index) const {
    return ParameterWriteBuffer(FindLocalSourceIndex(index));
}

NativeBuffer *BehaviorFrame::LocalReadData(int index) const {
    return ParameterReadBuffer(FindLocalSourceIndex(index));
}

CKObject *BehaviorFrame::LocalObject(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    return local ? local->GetValueObject(TRUE) : nullptr;
}

NativeBuffer *BehaviorFrame::BehaviorLocalWriteData(int index) const {
    return ParameterWriteBuffer(LocalParameter(index));
}

NativeBuffer *BehaviorFrame::BehaviorLocalReadData(int index) const {
    return ParameterReadBuffer(LocalParameter(index));
}

CKERROR BehaviorFrame::SetLocalObject(int index, CKObject *object) {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->SetLocalParameterObject(index, object) : CKERR_INVALIDOBJECT;
}

bool BehaviorFrame::SetSettingInt(const std::string &name, int value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamInt(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingIntIndex(int index, int value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamInt(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingFloat(const std::string &name, float value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamFloat(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingFloatIndex(int index, float value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamFloat(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingBool(const std::string &name, bool value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamBool(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingBoolIndex(int index, bool value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamBool(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingString(const std::string &name, const std::string &value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamString(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingStringIndex(int index, const std::string &value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamString(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingObject(const std::string &name, CKObject *value, int occurrence) {
    return SetSettingValue(ResolveSetting(name, occurrence, "BehaviorFrame.SetSetting"), MakeScriptParamObject(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::SetSettingObjectIndex(int index, CKObject *value) {
    return SetSettingValue(ResolveSettingIndex(index, "BehaviorFrame.SetSetting"), MakeScriptParamObject(value), "BehaviorFrame.SetSetting");
}

bool BehaviorFrame::IsSetting(int localIndex) const {
    CKBehavior *behavior = Behavior();
    return behavior && localIndex >= 0 && localIndex < behavior->GetLocalParameterCount() && behavior->IsLocalParameterSetting(localIndex);
}

BBCallBuilder *BehaviorFrame::CallName(const std::string &prototype) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Call requires ScriptBehaviorBridge.");
        return nullptr;
    }
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(MakeContext());
    request.PrototypeKind = ScriptBridgePrototypeKind::Name;
    request.PrototypeName = prototype;
    return new BBCallBuilder(bridge, MakeContext(), request);
}

BBCallBuilder *BehaviorFrame::CallGuid(CKGUID guid) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.Call requires ScriptBehaviorBridge.");
        return nullptr;
    }
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(MakeContext());
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;
    return new BBCallBuilder(bridge, MakeContext(), request);
}

BBBridge *BehaviorFrame::BB() const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame.BB requires ScriptBehaviorBridge.");
        return nullptr;
    }
    return new BBBridge(bridge, MakeContext());
}

ScriptAsyncTaskBase *BehaviorFrame::LinkDelay(int frames) const {
    if (frames < 0) {
        SetScriptException(fmt::format("BehaviorFrame.LinkDelay requires a non-negative frame count, got {}.", frames));
        return nullptr;
    }
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.LinkDelay");
    return scheduler ? scheduler->CreateDelay(frames) : nullptr;
}

ScriptAsyncTaskBase *BehaviorFrame::WaitBBTask(BBTask *task, int inputIndex) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.Wait");
    if (!scheduler) {
        return nullptr;
    }
    CKBehaviorContext ctx = MakeContext();
    return scheduler->CreateBBTask(task, &ctx, inputIndex);
}

ScriptAsyncTaskBase *BehaviorFrame::WaitGraphTask(GraphTask *task) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.Wait");
    if (!scheduler) {
        return nullptr;
    }
    CKBehaviorContext ctx = MakeContext();
    return scheduler->CreateGraphTask(task, &ctx);
}

ScriptAsyncTaskBase *BehaviorFrame::WaitMessageName(const std::string &message, CKBeObject *object) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.WaitMessage");
    if (!scheduler) {
        return nullptr;
    }
    CKBeObject *waitObject = ResolveMessageWaitObject(Behavior(), object);
    if (!waitObject) {
        SetScriptException("BehaviorFrame.WaitMessage requires a non-null object or behavior target/owner.");
        return nullptr;
    }
    std::string error;
    ScriptAsyncTaskBase *task = scheduler->CreateMessageWait(m_Context, message, waitObject, &error);
    if (!task) {
        SetScriptException(error.empty() ? "BehaviorFrame.WaitMessage failed to create a message wait task." : error);
    }
    return task;
}

ScriptAsyncTaskBase *BehaviorFrame::WaitMessageType(CKMessageType messageType, CKBeObject *object) const {
    ScriptAsyncScheduler *scheduler = RequireAsyncScheduler(m_Context, "BehaviorFrame.WaitMessage");
    if (!scheduler) {
        return nullptr;
    }
    CKBeObject *waitObject = ResolveMessageWaitObject(Behavior(), object);
    if (!waitObject) {
        SetScriptException("BehaviorFrame.WaitMessage requires a non-null object or behavior target/owner.");
        return nullptr;
    }
    std::string error;
    ScriptAsyncTaskBase *task = scheduler->CreateMessageWait(m_Context, messageType, waitObject, &error);
    if (!task) {
        SetScriptException(error.empty() ? "BehaviorFrame.WaitMessage failed to create a message wait task." : error);
    }
    return task;
}

void BehaviorFrame::Trace(const std::string &message) const {
    CKContext *context = Context();
    if (!context) {
        return;
    }
    CKBehavior *behavior = Behavior();
    const std::string prefix = behavior
        ? fmt::format("[CKAS:{}#{}] ", SafeString(behavior->GetName()), behavior->GetID())
        : "[CKAS:<behavior unavailable>] ";
    const std::string line = prefix + message;
    context->OutputToConsoleEx(const_cast<char *>("%s"), line.c_str());
}

CKBehaviorContext BehaviorFrame::MakeContext() const {
    CKBehaviorContext ctx = m_ContextSnapshot;
    ctx.Context = m_Context;
    ctx.Behavior = Behavior();
    ctx.CurrentLevel = ctx.CurrentLevel ? ctx.CurrentLevel : (m_Context ? m_Context->GetCurrentLevel() : nullptr);
    ctx.CurrentScene = ctx.CurrentScene ? ctx.CurrentScene : (m_Context ? m_Context->GetCurrentScene() : nullptr);
    ctx.CurrentRenderContext = ctx.CurrentRenderContext ? ctx.CurrentRenderContext : (m_Context ? m_Context->GetPlayerRenderContext() : nullptr);
    ctx.ParameterManager = ctx.ParameterManager ? ctx.ParameterManager : (m_Context ? m_Context->GetParameterManager() : nullptr);
    ctx.MessageManager = ctx.MessageManager ? ctx.MessageManager : (m_Context ? m_Context->GetMessageManager() : nullptr);
    ctx.AttributeManager = ctx.AttributeManager ? ctx.AttributeManager : (m_Context ? m_Context->GetAttributeManager() : nullptr);
    ctx.TimeManager = ctx.TimeManager ? ctx.TimeManager : (m_Context ? m_Context->GetTimeManager() : nullptr);
    return ctx;
}

ScriptBehaviorBridge *BehaviorFrame::Bridge() const {
    const CKBehaviorContext context = MakeContext();
    if (!context.Behavior) {
        return nullptr;
    }
    return ScriptHostScope::From(
        context, ScriptHostScopeContextResolution::DirectContext).Bridge();
}

BehaviorRef *BehaviorFrame::WrapBehavior(CKBehavior *behavior) const {
    ScriptBehaviorBridge *bridge = Bridge();
    return bridge && behavior ? bridge->WrapBehavior(behavior, m_BehaviorId) : nullptr;
}

ParamRef *BehaviorFrame::WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind, int index) const {
    if (!parameter) {
        return nullptr;
    }
    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException("BehaviorFrame parameter access requires ScriptBehaviorBridge.");
        return nullptr;
    }
    CKBehavior *behavior = Behavior();
    return new ParamRef(bridge,
                        parameter->GetID(),
                        kind,
                        index,
                        behavior ? behavior->GetID() : 0,
                        parameter->GetCKContext());
}

bool BehaviorFrame::SetSettingValue(int localIndex, const ScriptParamValue &value, const char *method) {
    if (localIndex < 0) {
        return false;
    }

    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return false;
    }
    if (localIndex >= behavior->GetLocalParameterCount() || !behavior->IsLocalParameterSetting(localIndex)) {
        SetScriptException(fmt::format("{} could not find setting local index {} on behavior '{}' (candidates: {}).",
            method ? method : "BehaviorFrame.SetSetting",
            localIndex,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
        return false;
    }

    ScriptBehaviorBridge *bridge = Bridge();
    if (!bridge) {
        SetScriptException(fmt::format("{} requires ScriptBehaviorBridge.", method ? method : "BehaviorFrame.SetSetting"));
        return false;
    }

    std::string error;
    CKBehaviorContext ctx = MakeContext();
    if (!bridge->SetBehaviorSetting(behavior, localIndex, value, ctx, error)) {
        SetScriptException(error.empty()
            ? fmt::format("{} failed to set setting local index {}.", method ? method : "BehaviorFrame.SetSetting", localIndex)
            : error);
        return false;
    }
    return true;
}

int BehaviorFrame::ResolveInput(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedIo(behavior, name, occurrence, true);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find input '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, true)));
    }
    return index;
}

int BehaviorFrame::ResolveOutput(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedIo(behavior, name, occurrence, false);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find output '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, false)));
    }
    return index;
}

int BehaviorFrame::ResolvePin(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetInputParameterCount(),
                                            [behavior](int i) { return behavior->GetInputParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find pin '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Pin)));
    }
    return index;
}

int BehaviorFrame::ResolvePout(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetOutputParameterCount(),
                                            [behavior](int i) { return behavior->GetOutputParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find pout '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Pout)));
    }
    return index;
}

int BehaviorFrame::ResolveLocal(const std::string &name, int occurrence, const char *method) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        const int index = bridge->FindFrameLocal(m_BehaviorId, name, occurrence);
        if (index < 0) {
            CKBehavior *behavior = Behavior();
            SetScriptException(fmt::format("{} could not find logical local '{}' occurrence {} on behavior '{}' (candidates: {}).",
                method,
                name,
                occurrence,
                SafeString(behavior ? behavior->GetName() : nullptr),
                bridge->DescribeFrameLocals(m_BehaviorId)));
        }
        return index;
    }
    return ResolveBehaviorLocal(name, occurrence, method);
}

int BehaviorFrame::ResolveBehaviorLocal(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetLocalParameterCount(),
                                            [behavior](int i) { return behavior->GetLocalParameter(i); });
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find local '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Local)));
    }
    return index;
}

int BehaviorFrame::ResolveSetting(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    const int localIndex = FindSettingIndex(behavior, name, occurrence);
    if (localIndex < 0) {
        SetScriptException(fmt::format("{} could not find setting '{}' occurrence {} on behavior '{}' (candidates: {}).",
            method,
            name,
            occurrence,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
    }
    return localIndex;
}

int BehaviorFrame::ResolveSettingIndex(int index, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!RequireBehavior(behavior, method)) {
        return -1;
    }
    if (index < 0 ||
        index >= behavior->GetLocalParameterCount() ||
        !behavior->IsLocalParameterSetting(index)) {
        SetScriptException(fmt::format("{} could not find setting local index {} on behavior '{}' (candidates: {}).",
            method,
            index,
            SafeString(behavior->GetName()),
            DescribeParameterNames(behavior, ScriptBridgeSlotKind::Setting)));
        return -1;
    }
    return index;
}

CKParameterIn *BehaviorFrame::FindPinParameter(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        return nullptr;
    }
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            behavior->GetInputParameterCount(),
                                            [behavior](int i) { return behavior->GetInputParameter(i); });
    return index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
}

CKParameterIn *BehaviorFrame::FindPinParameterIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
}

CKParameterLocal *BehaviorFrame::FindLocalParameter(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int count = behavior ? behavior->GetLocalParameterCount() : 0;
    const int index = ResolveNamedParameter(behavior,
                                            name,
                                            occurrence,
                                            count,
                                            [behavior](int i) { return behavior->GetLocalParameter(i); });
    return index >= 0 && index < count
        ? behavior->GetLocalParameter(index)
        : nullptr;
}

CKParameterLocal *BehaviorFrame::FindLocalParameterIndex(int index) const {
    CKBehavior *behavior = Behavior();
    return behavior && index >= 0 && index < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(index)
        : nullptr;
}

CKParameter *BehaviorFrame::FindLocalSource(const std::string &name, int occurrence) const {
    const int index = FindLocal(name, occurrence);
    return FindLocalSourceIndex(index);
}

CKParameter *BehaviorFrame::FindLocalSourceIndex(int index) const {
    ScriptBehaviorBridge *bridge = Bridge();
    if (bridge && m_BehaviorId != 0 && bridge->HasFrameLocalInterface(m_BehaviorId)) {
        std::string error;
        return bridge->ResolveFrameLocal(m_Context, m_BehaviorId, index, error);
    }
    return FindLocalParameterIndex(index);
}

ScriptParamValue BehaviorFrame::ReadPinValueOrEmpty(const std::string &name, int occurrence) const {
    CKParameterIn *pin = FindPinParameter(name, occurrence);
    CKParameter *source = pin ? ParameterSourceForConnection(pin) : nullptr;
    if (!source) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(source, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = source->GetGUID();
    value.Type = source->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadLocalValueOrEmpty(const std::string &name, int occurrence) const {
    CKParameter *local = FindLocalSource(name, occurrence);
    if (!local) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(local, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = local->GetGUID();
    value.Type = local->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadLocalValueOrEmpty(int index) const {
    CKParameter *local = FindLocalSourceIndex(index);
    if (!local) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(local, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = local->GetGUID();
    value.Type = local->GetType();
    return value;
}

ScriptParamValue BehaviorFrame::ReadPinValueOrEmpty(int index) const {
    CKParameterIn *pin = FindPinParameterIndex(index);
    CKParameter *source = pin ? ParameterSourceForConnection(pin) : nullptr;
    if (!source) {
        return ScriptParamValue();
    }
    std::string ignoredError;
    ScriptParamValue value = ReadParameterValue(source, &ignoredError);
    if (value.Kind == ScriptParamValueKind::Empty) {
        return value;
    }
    value.TypeGuid = source->GetGUID();
    value.Type = source->GetType();
    return value;
}
