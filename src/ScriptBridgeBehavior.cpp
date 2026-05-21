#include "ScriptBridgeHandles.h"

#include <algorithm>
#include <set>

#include <fmt/format.h>

namespace ScriptBridgeBehaviorInternal {

int SubBehaviorDepth(CKBehavior *root, CKBehavior *target, std::set<CK_ID> &visited) {
    if (!root || !target) {
        return -1;
    }
    if (root == target) {
        return 0;
    }
    if (!visited.insert(root->GetID()).second) {
        return -1;
    }
    for (int i = 0; i < root->GetSubBehaviorCount(); ++i) {
        const int childDepth = SubBehaviorDepth(root->GetSubBehavior(i), target, visited);
        if (childDepth >= 0) {
            return childDepth + 1;
        }
    }
    return -1;
}

int SubBehaviorDepth(CKBehavior *root, CKBehavior *target) {
    std::set<CK_ID> visited;
    return SubBehaviorDepth(root, target, visited);
}

CKBehavior *FindContainingOwnerScript(CKBehavior *behavior) {
    CKBeObject *owner = behavior ? behavior->GetOwner() : nullptr;
    if (!owner) {
        return nullptr;
    }

    CKBehavior *selfScript = nullptr;
    for (int i = 0; i < owner->GetScriptCount(); ++i) {
        CKBehavior *script = owner->GetScript(i);
        if (!script) {
            continue;
        }
        if (script == behavior) {
            selfScript = script;
            continue;
        }
        if (SubBehaviorDepth(script, behavior) > 0) {
            return script;
        }
    }
    return selfScript;
}

CKBehavior *FindContainingBehaviorInContext(CKBehavior *behavior) {
    CKContext *context = behavior ? behavior->GetCKContext() : nullptr;
    if (!context) {
        return nullptr;
    }

    CKBehavior *best = nullptr;
    int bestDepth = 0;
    const XObjectPointerArray &behaviors = context->GetObjectListByType(CKCID_BEHAVIOR, TRUE);
    for (int i = 0; i < behaviors.Size(); ++i) {
        CKBehavior *candidate = CKBehavior::Cast(behaviors[i]);
        const int depth = SubBehaviorDepth(candidate, behavior);
        if (depth > bestDepth) {
            best = candidate;
            bestDepth = depth;
        }
    }
    return best;
}

} // namespace ScriptBridgeBehaviorInternal

BehaviorLayout::BehaviorLayout(ScriptBehaviorBridge *bridge, CK_ID behaviorId)
    : m_Bridge(bridge), m_BehaviorId(behaviorId), m_IsPrototype(false) {
    m_BehaviorStamp = CaptureBridgeObjectStamp(RawBehavior());
}

BehaviorLayout::BehaviorLayout(ScriptBehaviorBridge *bridge,
                               const CKBehaviorContext &ctx,
                               const ScriptBridgeBBInvocationSpec &request)
    : m_Bridge(bridge), m_Context(ctx), m_Request(request), m_IsPrototype(true) {}

int BehaviorLayout::InputCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Inputs.size()) : 0;
}

int BehaviorLayout::OutputCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Outputs.size()) : 0;
}

int BehaviorLayout::PinCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Pins.size()) : 0;
}

int BehaviorLayout::PoutCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Pouts.size()) : 0;
}

int BehaviorLayout::SettingCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Settings.size()) : 0;
}

int BehaviorLayout::LocalCount() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout ? static_cast<int>(layout->Locals.size()) : 0;
}

std::string BehaviorLayout::InputName(int index) const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout && index >= 0 && index < static_cast<int>(layout->Inputs.size()) ? layout->Inputs[index].Name : std::string();
}

std::string BehaviorLayout::OutputNameAt(int index) const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    return layout && index >= 0 && index < static_cast<int>(layout->Outputs.size()) ? layout->Outputs[index].Name : std::string();
}

ParamInfo *BehaviorLayout::Pin(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Pin, index); }
ParamInfo *BehaviorLayout::Pout(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Pout, index); }
ParamInfo *BehaviorLayout::Setting(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Setting, index); }
ParamInfo *BehaviorLayout::Local(int index) const { return ParameterInfo(ScriptBridgeSlotKind::Local, index); }

int BehaviorLayout::FindInput(const std::string &name, int occurrence) const { return FindIo(true, name, occurrence); }
int BehaviorLayout::FindOutput(const std::string &name, int occurrence) const { return FindIo(false, name, occurrence); }
int BehaviorLayout::FindPin(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Pin, name, occurrence); }
int BehaviorLayout::FindPout(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Pout, name, occurrence); }
int BehaviorLayout::FindSetting(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Setting, name, occurrence); }
int BehaviorLayout::FindLocal(const std::string &name, int occurrence) const { return FindParameter(ScriptBridgeSlotKind::Local, name, occurrence); }

std::string BehaviorLayout::Describe() const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    std::string text;
    text += "Inputs:";
    for (int i = 0; i < InputCount(); ++i) text += fmt::format("\n  #{} {}", i, InputName(i));
    text += "\nOutputs:";
    for (int i = 0; i < OutputCount(); ++i) text += fmt::format("\n  #{} {}", i, OutputNameAt(i));
    text += "\nPins:";
    if (layout) {
        for (const ScriptBridgeLayoutParamSlot &slot : layout->Pins) {
            ParamInfo *info = Pin(slot.Index);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", slot.Index));
            if (info) info->Release();
        }
    }
    text += "\nPouts:";
    if (layout) {
        for (const ScriptBridgeLayoutParamSlot &slot : layout->Pouts) {
            ParamInfo *info = Pout(slot.Index);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", slot.Index));
            if (info) info->Release();
        }
    }
    text += "\nSettings:";
    if (layout) {
        for (const ScriptBridgeLayoutParamSlot &slot : layout->Settings) {
            ParamInfo *info = Setting(slot.Index);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", slot.Index));
            if (info) info->Release();
        }
    }
    text += "\nLocals:";
    if (layout) {
        for (const ScriptBridgeLayoutParamSlot &slot : layout->Locals) {
            ParamInfo *info = Local(slot.Index);
            text += fmt::format("\n  {}", info ? info->Describe() : fmt::format("#{} <invalid>", slot.Index));
            if (info) info->Release();
        }
    }
    return text;
}

CKContext *BehaviorLayout::Context() const {
    if (m_Context.Context) return m_Context.Context;
    return m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
}

CKObject *BehaviorLayout::RawBehavior() const {
    if (m_IsPrototype || !m_Bridge || !m_Bridge->GetManager()) return nullptr;
    return GetCKObjectById(m_Bridge->GetManager()->GetCKContext(), m_BehaviorId);
}

const ScriptBridgeLayoutRecord *BehaviorLayout::LayoutRecord() const {
    if (!m_Bridge) return nullptr;
    return m_IsPrototype
        ? m_Bridge->GetPrototypeLayout(m_Context, m_Request)
        : m_Bridge->GetBehaviorLayout(m_BehaviorId, m_BehaviorStamp);
}

ParamInfo *BehaviorLayout::ParameterInfo(ScriptBridgeSlotKind kind, int index) const {
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    if (!layout) return nullptr;
    const std::vector<ScriptBridgeLayoutParamSlot> *slots = nullptr;
    if (kind == ScriptBridgeSlotKind::Pin) {
        slots = &layout->Pins;
    } else if (kind == ScriptBridgeSlotKind::Pout) {
        slots = &layout->Pouts;
    } else if (kind == ScriptBridgeSlotKind::Setting) {
        slots = &layout->Settings;
    } else if (kind == ScriptBridgeSlotKind::Local) {
        slots = &layout->Locals;
    }
    if (!slots || index < 0) return nullptr;
    const auto it = std::find_if(slots->begin(), slots->end(), [index](const ScriptBridgeLayoutParamSlot &slot) {
        return slot.Index == index;
    });
    if (it == slots->end()) return nullptr;
    return new ParamInfo(kind, it->Index, it->Name, it->TypeGuid, it->TypeName, it->DataSize);
}

int BehaviorLayout::FindIo(bool input, const std::string &name, int occurrence) const {
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

int BehaviorLayout::FindParameter(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const {
    if (occurrence < 0) return -1;
    int seen = 0;
    const ScriptBridgeLayoutRecord *layout = LayoutRecord();
    if (!layout) return -1;
    const std::vector<ScriptBridgeLayoutParamSlot> *slots = kind == ScriptBridgeSlotKind::Pin
        ? &layout->Pins
        : (kind == ScriptBridgeSlotKind::Pout
            ? &layout->Pouts
            : (kind == ScriptBridgeSlotKind::Setting ? &layout->Settings : &layout->Locals));
    for (const ScriptBridgeLayoutParamSlot &slot : *slots) {
        if (slot.Name == name) {
            if (seen == occurrence) return slot.Index;
            ++seen;
        }
    }
    return -1;
}

BehaviorRef::BehaviorRef(ScriptBehaviorBridge *bridge, CK_ID behaviorId, CK_ID componentId)
    : m_Bridge(bridge), m_BehaviorId(behaviorId), m_ComponentId(componentId) {
    m_Stamp = CaptureBridgeObjectStamp(RawGet());
}

CKBehavior *BehaviorRef::Get() const {
    return CKBehavior::Cast(RawGetStamped());
}

bool BehaviorRef::IsValid() const { return Get() != nullptr; }
CK_ID BehaviorRef::GetID() const { return m_BehaviorId; }
std::string BehaviorRef::GetName() const { CKBehavior *b = Get(); return b ? SafeString(b->GetName()) : std::string(); }
bool BehaviorRef::IsActive() const { CKBehavior *b = Get(); return b && b->IsActive(); }
CKGUID BehaviorRef::GetPrototypeGuid() const { CKBehavior *b = Get(); return b ? b->GetPrototypeGuid() : CKGUID(); }
std::string BehaviorRef::GetPrototypeName() const { CKBehavior *b = Get(); return b ? SafeString(b->GetPrototypeName()) : std::string(); }

BehaviorLayout *BehaviorRef::Layout() const { return m_Bridge ? new BehaviorLayout(m_Bridge, m_BehaviorId) : nullptr; }

BehaviorGraph *BehaviorRef::AsGraph() const {
    CKBehavior *behavior = Get();
    if (!behavior) {
        return nullptr;
    }

    CKBehaviorContext context = {};
    context.Context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    context.Behavior = CKBehavior::Cast(GetCKObjectById(context.Context, m_ComponentId));
    if (!context.Behavior) {
        context.Behavior = behavior;
    }
    return new BehaviorGraph(m_Bridge, context, behavior->GetID());
}

bool BehaviorRef::Trigger(int inputIndex, bool reset) {
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

bool BehaviorRef::TriggerSlot(BBSlot *input, bool reset) {
    int inputIndex = -1;
    std::string error;
    if (!input || !input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.Trigger requires an input BBSlot." : error);
        return false;
    }
    return Trigger(inputIndex, reset);
}

GraphTask *BehaviorRef::Start(int inputIndex, bool reset, float timeoutSeconds) {
    CKBehavior *behavior = Get();
    if (!behavior || !Trigger(inputIndex, reset)) {
        if (!behavior) SetScriptException("BehaviorRef is not valid.");
        return nullptr;
    }
    return m_Bridge ? m_Bridge->CreateGraphWatch(behavior, m_ComponentId, timeoutSeconds) : nullptr;
}

GraphTask *BehaviorRef::StartSlot(BBSlot *input, bool reset, float timeoutSeconds) {
    int inputIndex = -1;
    std::string error;
    if (!input || !input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.Start requires an input BBSlot." : error);
        return nullptr;
    }
    return Start(inputIndex, reset, timeoutSeconds);
}

GraphTask *BehaviorRef::Watch(float timeoutSeconds) {
    CKBehavior *behavior = Get();
    if (!behavior) {
        SetScriptException("BehaviorRef is not valid.");
        return nullptr;
    }
    return m_Bridge ? m_Bridge->CreateGraphWatch(behavior, m_ComponentId, timeoutSeconds) : nullptr;
}

bool BehaviorRef::InputActive(int inputIndex) const {
    CKBehavior *behavior = Get();
    return behavior && inputIndex >= 0 && inputIndex < behavior->GetInputCount() && behavior->IsInputActive(inputIndex);
}

bool BehaviorRef::InputActiveSlot(BBSlot *input) const {
    int inputIndex = -1;
    std::string error;
    return input && input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error) && InputActive(inputIndex);
}

bool BehaviorRef::OutputActive(int outputIndex) const {
    CKBehavior *behavior = Get();
    return behavior && outputIndex >= 0 && outputIndex < behavior->GetOutputCount() && behavior->IsOutputActive(outputIndex);
}

bool BehaviorRef::OutputActiveSlot(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    return output && output->ResolveIndex(ScriptBridgeSlotKind::Output, outputIndex, error) && OutputActive(outputIndex);
}

ParamRef *BehaviorRef::Pin(int index) const {
    CKBehavior *behavior = Get();
    CKParameterIn *pin = behavior && index >= 0 && index < behavior->GetInputParameterCount() ? behavior->GetInputParameter(index) : nullptr;
    return m_Bridge && pin ? new ParamRef(m_Bridge, pin->GetID(), ScriptBridgeSlotKind::Pin, index, m_BehaviorId) : nullptr;
}

ParamRef *BehaviorRef::PinSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, index, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.Pin requires a pin BBSlot." : error);
        return nullptr;
    }
    return Pin(index);
}

ParamRef *BehaviorRef::Pout(int index) const {
    CKBehavior *behavior = Get();
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, m_BehaviorId) : nullptr;
}

ParamRef *BehaviorRef::PoutSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pout, index, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.Pout requires a pout BBSlot." : error);
        return nullptr;
    }
    return Pout(index);
}

ParamRef *BehaviorRef::Local(int index) const {
    CKBehavior *behavior = Get();
    CKParameterLocal *local = behavior && index >= 0 && index < behavior->GetLocalParameterCount() ? behavior->GetLocalParameter(index) : nullptr;
    return m_Bridge && local ? new ParamRef(m_Bridge, local->GetID(), ScriptBridgeSlotKind::Local, index, m_BehaviorId) : nullptr;
}

ParamRef *BehaviorRef::LocalSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Local, index, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.Local requires a local BBSlot." : error);
        return nullptr;
    }
    return Local(index);
}

ParamOperationRef *BehaviorRef::ConnectOperation(int pinIndex, ParamOp *operation) {
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

ParamOperationRef *BehaviorRef::ConnectOperationSlot(BBSlot *slot, ParamOp *operation) {
    int pinIndex = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetScriptException(error.empty() ? "BehaviorRef.ConnectOperation requires a pin BBSlot." : error);
        return nullptr;
    }
    return ConnectOperation(pinIndex, operation);
}

std::string BehaviorRef::Describe() const {
    BehaviorLayout *layout = Layout();
    std::string result = fmt::format("Behavior '{}' id={} prototype={}", GetName(), m_BehaviorId, GetPrototypeName());
    if (layout) {
        result += "\n" + layout->Describe();
        layout->Release();
    }
    return result;
}

CKObject *BehaviorRef::RawGet() const {
    CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    return GetCKObjectById(context, m_BehaviorId);
}

CKObject *BehaviorRef::RawGetStamped() const {
    CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    return GetStampedCKObjectById(context, m_BehaviorId, m_Stamp);
}

BehaviorBridge::BehaviorBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx)
    : m_Bridge(bridge), m_Context(ctx) {}

BehaviorGraph *BehaviorBridge::Graph() const {
    CKBehavior *root = nullptr;
    if (m_Context.Behavior) {
        root = ScriptBridgeBehaviorInternal::FindContainingOwnerScript(m_Context.Behavior);
    }
    if (!root && m_Context.Behavior) {
        root = ScriptBridgeBehaviorInternal::FindContainingBehaviorInContext(m_Context.Behavior);
    }
    if (!root && m_Context.Behavior) {
        root = m_Context.Behavior->GetOwnerScript();
    }
    CKBeObject *owner = m_Context.Behavior ? m_Context.Behavior->GetOwner() : nullptr;
    if (!root && owner && owner->GetScriptCount() > 0) {
        root = owner->GetScript(0);
    }
    if (!root && m_Context.Behavior) {
        root = m_Context.Behavior;
    }
    return root ? new BehaviorGraph(m_Bridge, m_Context, root->GetID()) : nullptr;
}

BehaviorRef *BehaviorBridge::Self() const {
    return m_Bridge && m_Context.Behavior ? m_Bridge->WrapBehavior(m_Context.Behavior, ComponentIdFromContext(m_Context)) : nullptr;
}

BehaviorRef *BehaviorBridge::OwnerScript() const {
    return m_Bridge && m_Context.Behavior ? m_Bridge->WrapBehavior(m_Context.Behavior->GetOwnerScript(), ComponentIdFromContext(m_Context)) : nullptr;
}

BehaviorRef *BehaviorBridge::Find(const std::string &name) const {
    if (!m_Bridge || name.empty()) return nullptr;
    BehaviorGraph *graph = Graph();
    if (graph) {
        BehaviorQuery *query = (new BehaviorQuery())->Name(name)->Recursive(true);
        BehaviorNode *node = graph->Find(query);
        BehaviorRef *ref = node ? node->Behavior() : nullptr;
        if (node) node->Release();
        query->Release();
        graph->Release();
        if (ref) {
            return ref;
        }
    }
    CKBeObject *owner = m_Context.Behavior ? m_Context.Behavior->GetOwner() : nullptr;
    if (CKBehavior *found = FindBehaviorOnOwner(owner, name)) {
        return m_Bridge->WrapBehavior(found, ComponentIdFromContext(m_Context));
    }
    CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    if (CKBehavior *found = FindBehaviorByNameInContext(context, name)) {
        return m_Bridge->WrapBehavior(found, ComponentIdFromContext(m_Context));
    }
    return nullptr;
}

BehaviorRef *BehaviorBridge::FindOn(CKBeObject *owner, const std::string &name) const {
    if (!m_Bridge || !owner || name.empty()) return nullptr;
    return m_Bridge->WrapBehavior(FindBehaviorOnOwner(owner, name), ComponentIdFromContext(m_Context));
}

BehaviorRef *BehaviorBridge::FindByID(CK_ID id) const {
    CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    return m_Bridge ? m_Bridge->WrapBehavior(CKBehavior::Cast(GetCKObjectById(context, id)), ComponentIdFromContext(m_Context)) : nullptr;
}

BehaviorRef *BehaviorBridge::FromParameter(const std::string &name) const {
    if (!m_Bridge || !m_Context.Behavior) return nullptr;
    CKParameter *param = FindReadableParameter(m_Context.Behavior, name);
    CKObject *obj = param ? param->GetValueObject(TRUE) : nullptr;
    return m_Bridge->WrapBehavior(CKBehavior::Cast(obj), ComponentIdFromContext(m_Context));
}
