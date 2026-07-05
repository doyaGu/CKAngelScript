#include "ScriptBehaviorFrame.h"

#include <fmt/format.h>

#include "CKAll.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptParameterConversion.h"

namespace {

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

std::string DescribeParameterNames(CKBehavior *behavior, bool input) {
    if (!behavior) {
        return "<behavior is null>";
    }
    const int count = input ? behavior->GetInputParameterCount() : behavior->GetOutputParameterCount();
    std::string result;
    for (int i = 0; i < count; ++i) {
        if (!result.empty()) {
            result += ", ";
        }
        if (input) {
            CKParameterIn *param = behavior->GetInputParameter(i);
            result += fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr));
        } else {
            CKParameterOut *param = behavior->GetOutputParameter(i);
            result += fmt::format("#{} '{}'", i, SafeString(param ? param->GetName() : nullptr));
        }
    }
    return result.empty() ? "<none>" : result;
}

} // namespace

BehaviorFrame::BehaviorFrame(const CKBehaviorContext &ctx) {
    Refresh(ctx);
}

void BehaviorFrame::Refresh(const CKBehaviorContext &ctx) {
    m_ContextSnapshot = ctx;
    m_Context = ctx.Context ? ctx.Context : (ctx.Behavior ? ctx.Behavior->GetCKContext() : nullptr);
    m_BehaviorId = ctx.Behavior ? ctx.Behavior->GetID() : 0;
    m_BehaviorStamp = CaptureBridgeObjectStamp(ctx.Behavior);
    m_ContextSnapshot.Behavior = nullptr;
    m_ContextSnapshot.Context = m_Context;
}

CKBehavior *BehaviorFrame::Behavior() const {
    return CKBehavior::Cast(GetStampedCKObjectById(m_Context, m_BehaviorId, m_BehaviorStamp));
}

CKContext *BehaviorFrame::Context() const {
    return m_Context;
}

CKBeObject *BehaviorFrame::Owner() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetOwner() : nullptr;
}

CKBeObject *BehaviorFrame::Target() const {
    CKBehavior *behavior = Behavior();
    return behavior ? behavior->GetTarget() : nullptr;
}

bool BehaviorFrame::InputActive(const std::string &name) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolveInput(name, "BehaviorFrame.InputActive");
    return behavior && index >= 0 && behavior->IsInputActive(index);
}

bool BehaviorFrame::Take(const std::string &name) {
    CKBehavior *behavior = Behavior();
    const int index = ResolveInput(name, "BehaviorFrame.Take");
    if (!behavior || index < 0) {
        return false;
    }
    const bool active = behavior->IsInputActive(index);
    if (active) {
        behavior->ActivateInput(index, FALSE);
    }
    return active;
}

void BehaviorFrame::Emit(const std::string &name) {
    CKBehavior *behavior = Behavior();
    const int index = ResolveOutput(name, "BehaviorFrame.Emit");
    if (!behavior || index < 0) {
        return;
    }
    behavior->ActivateOutput(index, TRUE);
}

ParamRef *BehaviorFrame::Pin(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePin(name, occurrence, "BehaviorFrame.Pin");
    CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(index)
        : nullptr;
    if (!pin) {
        return nullptr;
    }
    if (ScriptBehaviorBridge *bridge = Bridge()) {
        return new ParamRef(bridge, pin->GetID(), ScriptBridgeSlotKind::Pin, index, behavior->GetID(), pin->GetCKContext());
    }
    SetScriptException("BehaviorFrame.Pin requires ScriptBehaviorBridge.");
    return nullptr;
}

ParamRef *BehaviorFrame::Pout(const std::string &name, int occurrence) const {
    CKBehavior *behavior = Behavior();
    const int index = ResolvePout(name, occurrence, "BehaviorFrame.Pout");
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount()
        ? behavior->GetOutputParameter(index)
        : nullptr;
    if (!pout) {
        return nullptr;
    }
    if (ScriptBehaviorBridge *bridge = Bridge()) {
        return new ParamRef(bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID(), pout->GetCKContext());
    }
    SetScriptException("BehaviorFrame.Pout requires ScriptBehaviorBridge.");
    return nullptr;
}

std::string BehaviorFrame::PinString(const std::string &name, const std::string &defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
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

bool BehaviorFrame::PinBool(const std::string &name, bool defaultValue, int occurrence) const {
    const ScriptParamValue value = ReadPinValueOrEmpty(name, occurrence);
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

bool BehaviorFrame::SetPoutInt(const std::string &name, int value, int occurrence) {
    ParamRef *ref = Pout(name, occurrence);
    const bool ok = ref && ref->SetInt(value);
    if (ref) {
        ref->Release();
    }
    return ok;
}

bool BehaviorFrame::SetPoutFloat(const std::string &name, float value, int occurrence) {
    ParamRef *ref = Pout(name, occurrence);
    const bool ok = ref && ref->SetFloat(value);
    if (ref) {
        ref->Release();
    }
    return ok;
}

bool BehaviorFrame::SetPoutBool(const std::string &name, bool value, int occurrence) {
    ParamRef *ref = Pout(name, occurrence);
    const bool ok = ref && ref->SetBool(value);
    if (ref) {
        ref->Release();
    }
    return ok;
}

bool BehaviorFrame::SetPoutString(const std::string &name, const std::string &value, int occurrence) {
    ParamRef *ref = Pout(name, occurrence);
    const bool ok = ref && ref->SetString(value);
    if (ref) {
        ref->Release();
    }
    return ok;
}

bool BehaviorFrame::SetPoutObject(const std::string &name, CKObject *value, int occurrence) {
    ParamRef *ref = Pout(name, occurrence);
    const bool ok = ref && ref->SetObject(value);
    if (ref) {
        ref->Release();
    }
    return ok;
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
    return ctx;
}

ScriptBehaviorBridge *BehaviorFrame::Bridge() const {
    ScriptManager *manager = ManagerFromContext(MakeContext());
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

int BehaviorFrame::ResolveInput(const std::string &name, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method));
        return -1;
    }
    const int index = FindInputIndex(behavior, name);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find input '{}' on behavior '{}' (candidates: {}).",
            method,
            name,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, true)));
    }
    return index;
}

int BehaviorFrame::ResolveOutput(const std::string &name, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method));
        return -1;
    }
    const int index = FindOutputIndex(behavior, name);
    if (index < 0) {
        SetScriptException(fmt::format("{} could not find output '{}' on behavior '{}' (candidates: {}).",
            method,
            name,
            SafeString(behavior->GetName()),
            DescribeIoNames(behavior, false)));
    }
    return index;
}

int BehaviorFrame::ResolvePin(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method));
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
            DescribeParameterNames(behavior, true)));
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

int BehaviorFrame::ResolvePout(const std::string &name, int occurrence, const char *method) const {
    CKBehavior *behavior = Behavior();
    if (!behavior) {
        SetScriptException(fmt::format("{} requires a live behavior.", method));
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
            DescribeParameterNames(behavior, false)));
    }
    return index;
}
