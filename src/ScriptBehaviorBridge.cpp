#include "ScriptBridgeHandles.h"

#include <algorithm>
#include <functional>

#include <fmt/format.h>

#include "ScriptManager.h"
#include "ScriptParameterConversion.h"

#ifndef CKAS_BUILD_SELF_TESTS
#define CKAS_BUILD_SELF_TESTS 0
#endif

namespace {

#if CKAS_BUILD_SELF_TESTS
bool g_FailNextOwnedLocalSyncForSelfTest = false;
#endif

std::string BridgeObjectName(CKObject *object) {
    return SafeString(object ? object->GetName() : nullptr);
}

const ScriptBridgeFrameLocalBinding *FindFrameLocalBinding(
    const std::vector<ScriptBridgeFrameLocalBinding> &bindings,
    const std::string &name,
    std::size_t *index = nullptr) {
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i].Name == name) {
            if (index) {
                *index = i;
            }
            return &bindings[i];
        }
    }
    return nullptr;
}

bool ApplyFrameLocalDefault(CKParameterLocal *local,
                            const ScriptBridgeFrameLocalDecl &decl,
                            std::string &error) {
    if (!decl.HasDefault) {
        return true;
    }
    if (!SetParameterDefaultText(local, decl.DefaultText, error)) {
        error = "Failed to apply default value for frame local '" + decl.Name + "': " + error;
        return false;
    }
    return true;
}

struct OwnedFrameLocalSnapshot {
    std::string Name;
    CKGUID TypeGuid;
    ScriptParamValue Value;
};

bool CaptureOwnedFrameLocals(CKBehavior *behavior,
                             int ownedLocalStart,
                             std::vector<OwnedFrameLocalSnapshot> &snapshot,
                             std::string &error) {
    if (!behavior || ownedLocalStart < 0 || behavior->GetLocalParameterCount() < ownedLocalStart) {
        error = "Frame local interface cannot snapshot an invalid owned-local range.";
        return false;
    }
    for (int index = ownedLocalStart; index < behavior->GetLocalParameterCount(); ++index) {
        CKParameterLocal *local = behavior->GetLocalParameter(index);
        if (!local || behavior->IsLocalParameterSetting(index)) {
            error = "Frame local interface cannot snapshot an internal setting.";
            return false;
        }
        std::string readError;
        OwnedFrameLocalSnapshot entry;
        entry.Name = BridgeObjectName(local);
        entry.TypeGuid = local->GetGUID();
        entry.Value = ReadParameterValue(local, &readError);
        if (!readError.empty()) {
            error = "Failed to snapshot frame local '" + entry.Name + "': " + readError;
            return false;
        }
        snapshot.push_back(std::move(entry));
    }
    return true;
}

bool RestoreOwnedFrameLocals(CKBehavior *behavior,
                             int ownedLocalStart,
                             const std::vector<OwnedFrameLocalSnapshot> &snapshot,
                             std::string &error) {
    const int targetCount = ownedLocalStart + static_cast<int>(snapshot.size());
    while (behavior->GetLocalParameterCount() > targetCount) {
        const int index = behavior->GetLocalParameterCount() - 1;
        if (behavior->IsLocalParameterSetting(index)) {
            error = "Frame local rollback would remove an internal setting.";
            return false;
        }
        CKParameterLocal *removed = behavior->RemoveLocalParameter(index);
        if (!removed) {
            error = "Frame local rollback could not remove a generated local.";
            return false;
        }
        CKDestroyObject(removed);
    }
    for (int i = 0; i < static_cast<int>(snapshot.size()); ++i) {
        const OwnedFrameLocalSnapshot &entry = snapshot[i];
        const int index = ownedLocalStart + i;
        CKParameterLocal *local = index < behavior->GetLocalParameterCount()
            ? behavior->GetLocalParameter(index)
            : behavior->CreateLocalParameter(const_cast<CKSTRING>(entry.Name.c_str()), entry.TypeGuid);
        if (!local || behavior->IsLocalParameterSetting(index)) {
            error = "Frame local rollback could not restore slot '" + entry.Name + "'.";
            return false;
        }
        if (local->GetGUID() != entry.TypeGuid) {
            local->SetGUID(entry.TypeGuid);
        }
        if (BridgeObjectName(local) != entry.Name) {
            local->SetName(const_cast<CKSTRING>(entry.Name.c_str()));
        }
        std::string writeError;
        if (WriteParameterValue(local, entry.Value, writeError) != CK_OK) {
            error = "Frame local rollback could not restore '" + entry.Name + "': " + writeError;
            return false;
        }
    }
    return true;
}

bool SyncOwnedFrameLocalsUnsafe(CKBehavior *behavior,
                                const std::vector<ScriptBridgeFrameLocalDecl> &ownedDecls,
                                int ownedLocalStart,
                                bool injectFailure,
                                std::string &error) {
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }
    if (ownedLocalStart < 0 || behavior->GetLocalParameterCount() < ownedLocalStart) {
        error = "Frame local interface cannot sync owned locals because the behavior is missing its internal local/settings block.";
        return false;
    }

    const int targetCount = ownedLocalStart + static_cast<int>(ownedDecls.size());
    while (behavior->GetLocalParameterCount() < targetCount) {
        const ScriptBridgeFrameLocalDecl &decl = ownedDecls[behavior->GetLocalParameterCount() - ownedLocalStart];
        CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(decl.Name.c_str()), decl.TypeGuid);
        if (!local) {
            error = "Failed to create owned frame local '" + decl.Name + "'.";
            return false;
        }
        if (!ApplyFrameLocalDefault(local, decl, error)) {
            return false;
        }
    }

    for (int i = 0; i < static_cast<int>(ownedDecls.size()); ++i) {
        const int index = ownedLocalStart + i;
        CKParameterLocal *local = behavior->GetLocalParameter(index);
        const ScriptBridgeFrameLocalDecl &decl = ownedDecls[i];
        if (!local || behavior->IsLocalParameterSetting(index)) {
            error = "Owned frame local slot " + std::to_string(index) + " is unavailable.";
            return false;
        }

        bool typeChanged = false;
        bool nameChanged = false;
        if (local->GetGUID() != decl.TypeGuid) {
            local->SetGUID(decl.TypeGuid);
            typeChanged = true;
        }
        if (BridgeObjectName(local) != decl.Name) {
            local->SetName(const_cast<CKSTRING>(decl.Name.c_str()));
            nameChanged = true;
        }
        if ((typeChanged || nameChanged) && !ApplyFrameLocalDefault(local, decl, error)) {
            return false;
        }
    }

#if CKAS_BUILD_SELF_TESTS
    if (injectFailure) {
        error = "Injected owned frame-local synchronization failure.";
        return false;
    }
#else
    (void) injectFailure;
#endif
    while (behavior->GetLocalParameterCount() > targetCount) {
        const int index = behavior->GetLocalParameterCount() - 1;
        if (index < ownedLocalStart || behavior->IsLocalParameterSetting(index)) {
            error = "Frame local interface would remove an internal setting.";
            return false;
        }
        CKParameterLocal *removed = behavior->RemoveLocalParameter(index);
        if (!removed) {
            error = "Failed to remove an obsolete owned frame local.";
            return false;
        }
        CKDestroyObject(removed);
    }
    return true;
}

bool SyncOwnedFrameLocals(CKBehavior *behavior,
                          const std::vector<ScriptBridgeFrameLocalDecl> &ownedDecls,
                          int ownedLocalStart,
                          std::string &error) {
    std::vector<OwnedFrameLocalSnapshot> snapshot;
#if CKAS_BUILD_SELF_TESTS
    const bool injectFailure = g_FailNextOwnedLocalSyncForSelfTest;
    g_FailNextOwnedLocalSyncForSelfTest = false;
#else
    const bool injectFailure = false;
#endif
    if (!CaptureOwnedFrameLocals(behavior, ownedLocalStart, snapshot, error)) {
        return false;
    }
    std::string syncError;
    if (SyncOwnedFrameLocalsUnsafe(behavior, ownedDecls, ownedLocalStart, injectFailure, syncError)) {
        return true;
    }
    std::string restoreError;
    if (!RestoreOwnedFrameLocals(behavior, ownedLocalStart, snapshot, restoreError)) {
        error = syncError + " Failed to restore previous frame locals: " + restoreError;
    } else {
        error = syncError;
    }
    return false;
}

} // namespace

#if CKAS_BUILD_SELF_TESTS
void ScriptBehaviorBridgeFailNextOwnedLocalSyncForSelfTest() {
    g_FailNextOwnedLocalSyncForSelfTest = true;
}
#endif

void ScriptBridgeSetIndexedValue(std::vector<ScriptBridgeIndexedValue> &values,
                                 int pinIndex,
                                 const ScriptParamValue &value) {
    const auto it = std::lower_bound(values.begin(), values.end(), pinIndex,
        [](const ScriptBridgeIndexedValue &entry, int index) {
            return entry.PinIndex < index;
        });
    if (it != values.end() && it->PinIndex == pinIndex) {
        it->Value = value;
    } else {
        values.insert(it, ScriptBridgeIndexedValue{pinIndex, value});
    }
}

CK_ID ScriptBridgeInputSourceBindings::Find(int pinIndex) const {
    const auto it = std::lower_bound(Items.begin(), Items.end(), pinIndex,
        [](const ScriptBridgeInputSourceBinding &entry, int index) {
            return entry.PinIndex < index;
        });
    return it != Items.end() && it->PinIndex == pinIndex ? it->SourceId : 0;
}

void ScriptBridgeInputSourceBindings::Set(int pinIndex, CK_ID sourceId) {
    const auto it = std::lower_bound(Items.begin(), Items.end(), pinIndex,
        [](const ScriptBridgeInputSourceBinding &entry, int index) {
            return entry.PinIndex < index;
        });
    if (it != Items.end() && it->PinIndex == pinIndex) {
        it->SourceId = sourceId;
        return;
    }
    Items.insert(it, ScriptBridgeInputSourceBinding{pinIndex, sourceId});
}

void ScriptBridgeInputSourceBindings::Remove(int pinIndex) {
    const auto it = std::lower_bound(Items.begin(), Items.end(), pinIndex,
        [](const ScriptBridgeInputSourceBinding &entry, int index) {
            return entry.PinIndex < index;
        });
    if (it != Items.end() && it->PinIndex == pinIndex) {
        Items.erase(it);
    }
}

bool ScriptBridgeExecutionState::IndexSet::Empty() const {
    return InlineMask == 0 && Overflow.empty();
}

void ScriptBridgeExecutionState::IndexSet::Clear() {
    InlineMask = 0;
    Overflow.clear();
}

bool ScriptBridgeExecutionState::IndexSet::Contains(int index) const {
    if (index < 0) {
        return !Empty();
    }
    if (index < 32) {
        return (InlineMask & (1u << index)) != 0;
    }
    return std::find(Overflow.begin(), Overflow.end(), index) != Overflow.end();
}

bool ScriptBridgeExecutionState::IndexSet::Insert(int index) {
    if (index < 0) {
        return false;
    }
    if (index < 32) {
        const CKDWORD bit = 1u << index;
        const bool inserted = (InlineMask & bit) == 0;
        InlineMask |= bit;
        return inserted;
    }
    if (std::find(Overflow.begin(), Overflow.end(), index) != Overflow.end()) {
        return false;
    }
    Overflow.push_back(index);
    return true;
}

void ScriptBridgeExecutionState::IndexSet::MergeFrom(const IndexSet &other) {
    InlineMask |= other.InlineMask;
    for (int index : other.Overflow) {
        Insert(index);
    }
}

ScriptBehaviorBridge::ScriptBehaviorBridge(ScriptManager *manager)
    : m_Manager(manager) {}

ScriptBehaviorBridge::~ScriptBehaviorBridge() {
    Clear();
}

CKERROR ScriptBehaviorBridge::PreProcess() {
    for (auto &entry : m_PendingDestroy) {
        if (entry.FramesToWait > 0) {
            --entry.FramesToWait;
        }
    }
    return CK_OK;
}

CKERROR ScriptBehaviorBridge::PostProcess() {
    DestroyQueuedReady();
    return CK_OK;
}

void ScriptBehaviorBridge::Clear(bool clearFrameLocalBindings) {
    std::vector<CK_ID> resultIds;
    resultIds.reserve(m_Results.size());
    for (const auto &entry : m_Results) {
        resultIds.push_back(entry.first);
    }

    for (CK_ID resultId : resultIds) {
        auto it = m_Results.find(resultId);
        if (it != m_Results.end()) {
            DestroyResult(it->second.ResultId, it->second.Generation);
        }
    }

    std::vector<CK_ID> taskIds;
    taskIds.reserve(m_Tasks.size());
    for (const auto &entry : m_Tasks) {
        taskIds.push_back(entry.first);
    }

    for (CK_ID taskId : taskIds) {
        auto it = m_Tasks.find(taskId);
        if (it != m_Tasks.end()) {
            DestroyTask(it->second.TaskId, it->second.Generation);
        }
    }

    std::vector<CK_ID> instanceIds;
    instanceIds.reserve(m_Instances.size());
    for (const auto &entry : m_Instances) {
        instanceIds.push_back(entry.first);
    }

    for (CK_ID instanceId : instanceIds) {
        auto it = m_Instances.find(instanceId);
        if (it != m_Instances.end()) {
            DestroyInstance(it->second.InstanceId, it->second.Generation);
        }
    }

    m_GraphWatches.clear();
    if (clearFrameLocalBindings) {
        m_FrameLocalBindings.clear();
    }
    m_FrameLocalInterfaces.clear();
    m_BehaviorLayouts.clear();
    m_PrototypeLayouts.clear();
    ForceDestroyQueued();
}

void ScriptBehaviorBridge::DestroyComponentTasks(CK_ID componentId) {
    std::vector<CK_ID> taskIds;
    for (const auto &entry : m_Tasks) {
        if (entry.second.ComponentId == componentId) {
            taskIds.push_back(entry.first);
        }
    }

    for (CK_ID taskId : taskIds) {
        auto it = m_Tasks.find(taskId);
        if (it != m_Tasks.end()) {
            DestroyTask(it->second.TaskId, it->second.Generation);
        }
    }

    std::vector<CK_ID> instanceIds;
    for (const auto &entry : m_Instances) {
        if (entry.second.ComponentId == componentId) {
            instanceIds.push_back(entry.first);
        }
    }
    for (CK_ID instanceId : instanceIds) {
        auto it = m_Instances.find(instanceId);
        if (it != m_Instances.end()) {
            DestroyInstance(it->second.InstanceId, it->second.Generation);
        }
    }

    std::vector<CK_ID> watchIds;
    for (const auto &entry : m_GraphWatches) {
        if (entry.second.ComponentId == componentId) {
            watchIds.push_back(entry.first);
        }
    }
    for (CK_ID watchId : watchIds) {
        auto it = m_GraphWatches.find(watchId);
        if (it != m_GraphWatches.end()) {
            CancelGraphWatch(it->second.WatchId, it->second.Generation);
        }
    }
}

void ScriptBehaviorBridge::ResetComponentTasks(CK_ID componentId) {
    for (auto &entry : m_Tasks) {
        if (entry.second.ComponentId == componentId) {
            ResetTask(entry.second.TaskId, entry.second.Generation);
        }
    }

    std::vector<CK_ID> instanceIds;
    for (const auto &entry : m_Instances) {
        if (entry.second.ComponentId == componentId) {
            instanceIds.push_back(entry.first);
        }
    }
    for (CK_ID instanceId : instanceIds) {
        auto it = m_Instances.find(instanceId);
        if (it != m_Instances.end()) {
            DestroyInstance(it->second.InstanceId, it->second.Generation);
        }
    }

    std::vector<CK_ID> watchIds;
    for (const auto &entry : m_GraphWatches) {
        if (entry.second.ComponentId == componentId) {
            watchIds.push_back(entry.first);
        }
    }
    for (CK_ID watchId : watchIds) {
        auto it = m_GraphWatches.find(watchId);
        if (it != m_GraphWatches.end()) {
            CancelGraphWatch(it->second.WatchId, it->second.Generation);
        }
    }
}

void ScriptBehaviorBridge::PauseComponentTasks(CK_ID componentId, bool paused) {
    for (auto &entry : m_Tasks) {
        if (entry.second.ComponentId == componentId) {
            entry.second.SetFlag(ScriptBridgeTaskFlags::Paused, paused);
        }
    }

    for (auto &entry : m_Instances) {
        if (entry.second.ComponentId == componentId) {
            entry.second.SetFlag(ScriptBridgeTaskFlags::Paused, paused);
        }
    }

    for (auto &entry : m_GraphWatches) {
        if (entry.second.ComponentId == componentId) {
            entry.second.SetFlag(ScriptBridgeTaskFlags::Paused, paused);
        }
    }
}

BehaviorRef *ScriptBehaviorBridge::WrapBehavior(CKBehavior *behavior, CK_ID componentId) {
    return behavior ? new BehaviorRef(this, behavior->GetID(), componentId, behavior->GetCKContext()) : nullptr;
}

void ScriptBehaviorBridge::ReleaseBehaviorRef(BehaviorRef *ref) {
    if (ref) {
        ref->Release();
    }
}

ParamRef *ScriptBehaviorBridge::WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind, int index) {
    if (!parameter) {
        return nullptr;
    }
    CKContext *context = parameter->GetCKContext();
    if (CKParameterIn::Cast(parameter)) {
        return new ParamInRef(this, parameter->GetID(), kind, index, 0, context);
    }
    if (CKParameterOut::Cast(parameter)) {
        return new ParamOutRef(this, parameter->GetID(), kind, index, 0, context);
    }
    if (CKParameterLocal::Cast(parameter)) {
        return new ParamLocalRef(this, parameter->GetID(), kind, index, 0, context);
    }
    return new ParamRef(this, parameter->GetID(), kind, index, 0, context);
}

ParamOperationRef *ScriptBehaviorBridge::WrapParameterOperation(CKParameterOperation *operation,
                                                                CKParameterIn *targetInput,
                                                                CKParameter *previousSource,
                                                                const std::vector<CK_ID> &ownedLocalSourceIds) {
    return operation ? new ParamOperationRef(this, operation->GetID(), targetInput, previousSource, ownedLocalSourceIds, operation->GetCKContext()) : nullptr;
}

BehaviorBridge *ScriptBehaviorBridge::CreateBehaviorBridge(const CKBehaviorContext &ctx) {
    return new BehaviorBridge(this, ctx);
}

BBPrototype *ScriptBehaviorBridge::CreatePrototype(const CKBehaviorContext &ctx, const std::string &name) {
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(ctx);
    request.PrototypeName = name;
    return new BBPrototype(this, ctx, request);
}

BBPrototype *ScriptBehaviorBridge::CreatePrototype(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(ctx);
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = guid;
    return new BBPrototype(this, ctx, request);
}

void ScriptBehaviorBridge::ReleasePrototype(BBPrototype *prototype) {
    if (prototype) {
        prototype->Release();
    }
}

bool ScriptBehaviorBridge::BindFrameLocal(CKBehavior *behavior,
                                          const std::string &name,
                                          CKGUID guid,
                                          ParamRef *sourceRef,
                                          const std::string &label,
                                          std::string &error) {
    if (!behavior) {
        error = "BindLocal requires a behavior.";
        return false;
    }
    if (name.empty()) {
        error = "BindLocal requires a non-empty logical local name.";
        return false;
    }
    if (!CKGuidIsValid(guid)) {
        error = "BindLocal '" + name + "' requires a valid declared parameter guid.";
        return false;
    }
    CKParameter *source = sourceRef ? sourceRef->Source() : nullptr;
    if (!source) {
        error = "BindLocal '" + name + "' requires a parameter with a writable real source.";
        return false;
    }
    if (source->GetGUID() != guid) {
        CKContext *context = behavior->GetCKContext();
        error = "BindLocal '" + name + "' type mismatch. Expected " +
            DescribeScriptParamType(context, guid) + ", got " +
            DescribeScriptParamType(context, source->GetGUID()) + ".";
        return false;
    }

    const CK_ID behaviorId = behavior->GetID();
    std::vector<ScriptBridgeFrameLocalBinding> &bindings = m_FrameLocalBindings[behaviorId];

    std::size_t bindingIndex = 0;
    ScriptBridgeFrameLocalBinding oldBinding;
    bool hadExisting = false;
    if (ScriptBridgeFrameLocalBinding *existing = const_cast<ScriptBridgeFrameLocalBinding *>(
            FindFrameLocalBinding(bindings, name, &bindingIndex))) {
        if (existing->TypeGuid != guid || existing->SourceId != source->GetID()) {
            error = "BindLocal '" + name + "' conflicts with an existing binding.";
            return false;
        }
        oldBinding = *existing;
        hadExisting = true;
        existing->SourceStamp = CaptureBridgeObjectStamp(source);
        existing->Label = label;
    } else {
        ScriptBridgeFrameLocalBinding binding;
        binding.Name = name;
        binding.TypeGuid = guid;
        binding.SourceId = source->GetID();
        binding.SourceStamp = CaptureBridgeObjectStamp(source);
        binding.Label = label;
        bindings.push_back(std::move(binding));
        bindingIndex = bindings.size() - 1;
    }

    const auto interfaceIt = m_FrameLocalInterfaces.find(behaviorId);
    if (interfaceIt != m_FrameLocalInterfaces.end()) {
        const std::vector<ScriptBridgeFrameLocalDecl> declarations = interfaceIt->second.Declarations;
        const int ownedLocalStart = interfaceIt->second.OwnedLocalStart;
        const bool strict = interfaceIt->second.Strict;

        std::string applyError;
        if (!ApplyFrameLocalInterface(behavior, declarations, ownedLocalStart, strict, applyError)) {
            if (hadExisting) {
                bindings[bindingIndex] = std::move(oldBinding);
            } else if (bindingIndex < bindings.size()) {
                bindings.erase(bindings.begin() + static_cast<std::ptrdiff_t>(bindingIndex));
            }

            std::string restoreError;
            if (!ApplyFrameLocalInterface(behavior, declarations, ownedLocalStart, strict, restoreError)) {
                error = "BindLocal '" + name + "' could not refresh the logical local interface: " +
                    applyError + " Failed to restore previous interface: " + restoreError;
            } else {
                error = "BindLocal '" + name + "' could not refresh the logical local interface: " + applyError;
            }
            return false;
        }
    }
    return true;
}

bool ScriptBehaviorBridge::ApplyFrameLocalInterface(CKBehavior *behavior,
                                                    const std::vector<ScriptBridgeFrameLocalDecl> &locals,
                                                    int ownedLocalStart,
                                                    bool strict,
                                                    std::string &error) {
    if (!behavior) {
        error = "Frame local interface requires a behavior.";
        return false;
    }

    const CK_ID behaviorId = behavior->GetID();
    const auto bindingsIt = m_FrameLocalBindings.find(behaviorId);
    const std::vector<ScriptBridgeFrameLocalBinding> *bindings =
        bindingsIt == m_FrameLocalBindings.end() ? nullptr : &bindingsIt->second;
    std::vector<bool> matched(bindings ? bindings->size() : 0, false);

    std::vector<ScriptBridgeFrameLocalSlot> slots;
    slots.reserve(locals.size());
    std::vector<ScriptBridgeFrameLocalDecl> ownedDecls;
    ownedDecls.reserve(locals.size());

    for (const ScriptBridgeFrameLocalDecl &decl : locals) {
        if (decl.Name.empty()) {
            error = "Frame local interface contains an empty local name.";
            return false;
        }
        if (!CKGuidIsValid(decl.TypeGuid)) {
            error = "Frame local '" + decl.Name + "' has no valid declared guid.";
            return false;
        }
        for (const ScriptBridgeFrameLocalSlot &slot : slots) {
            if (slot.Name == decl.Name) {
                error = "Frame local interface declares duplicate local '" + decl.Name + "'.";
                return false;
            }
        }

        ScriptBridgeFrameLocalSlot slot;
        slot.Name = decl.Name;
        slot.TypeGuid = decl.TypeGuid;

        std::size_t bindingIndex = 0;
        const ScriptBridgeFrameLocalBinding *binding =
            bindings ? FindFrameLocalBinding(*bindings, decl.Name, &bindingIndex) : nullptr;
        if (binding) {
            matched[bindingIndex] = true;
            if (binding->TypeGuid != decl.TypeGuid) {
                CKContext *context = behavior->GetCKContext();
                error = "Frame local binding '" + decl.Name + "' type mismatch. Interface declares " +
                    DescribeScriptParamType(context, decl.TypeGuid) + ", binding declares " +
                    DescribeScriptParamType(context, binding->TypeGuid) + ".";
                return false;
            }
            std::string sourceError;
            CKParameter *source = ResolveStampedParameterSource(behavior->GetCKContext(),
                                                                binding->SourceId,
                                                                binding->SourceStamp,
                                                                sourceError);
            if (!source) {
                error = "Frame local binding '" + decl.Name + "' source is invalid: " + sourceError;
                return false;
            }
            if (source->GetGUID() != decl.TypeGuid) {
                CKContext *context = behavior->GetCKContext();
                error = "Frame local binding '" + decl.Name + "' source type mismatch. Expected " +
                    DescribeScriptParamType(context, decl.TypeGuid) + ", got " +
                    DescribeScriptParamType(context, source->GetGUID()) + ".";
                return false;
            }
            slot.SourceId = source->GetID();
            slot.SourceStamp = CaptureBridgeObjectStamp(source);
            slot.Label = binding->Label;
        } else {
            slot.OwnedLocalIndex = ownedLocalStart + static_cast<int>(ownedDecls.size());
            ownedDecls.push_back(decl);
        }

        slots.push_back(std::move(slot));
    }

    if (strict && bindings) {
        for (std::size_t i = 0; i < bindings->size(); ++i) {
            if (!matched[i]) {
                error = "Frame local binding '" + (*bindings)[i].Name + "' has no matching interface local declaration.";
                return false;
            }
        }
    }

    if (!SyncOwnedFrameLocals(behavior, ownedDecls, ownedLocalStart, error)) {
        return false;
    }

    if (slots.empty()) {
        m_FrameLocalInterfaces.erase(behaviorId);
    } else {
        ScriptBridgeFrameLocalInterface interfaceLayout;
        interfaceLayout.BehaviorId = behaviorId;
        interfaceLayout.BehaviorStamp = CaptureBridgeObjectStamp(behavior);
        interfaceLayout.Declarations = locals;
        interfaceLayout.OwnedLocalStart = ownedLocalStart;
        interfaceLayout.Strict = strict;
        interfaceLayout.Locals = std::move(slots);
        m_FrameLocalInterfaces.insert_or_assign(behaviorId, std::move(interfaceLayout));
    }
    return true;
}

bool ScriptBehaviorBridge::ApplyFrameLocalInterfaceTransaction(
    CKBehavior *behavior,
    const std::vector<ScriptBridgeFrameLocalDecl> &locals,
    int ownedLocalStart,
    bool strict,
    const std::function<bool(std::string &)> &commit,
    std::string &error) {
    if (!behavior || !commit) {
        error = "Frame local interface transaction requires a behavior and commit action.";
        return false;
    }

    std::vector<OwnedFrameLocalSnapshot> localSnapshot;
    if (!CaptureOwnedFrameLocals(behavior, ownedLocalStart, localSnapshot, error)) {
        return false;
    }

    const CK_ID behaviorId = behavior->GetID();
    const auto oldInterfaceIt = m_FrameLocalInterfaces.find(behaviorId);
    const bool hadOldInterface = oldInterfaceIt != m_FrameLocalInterfaces.end();
    const ScriptBridgeFrameLocalInterface oldInterface = hadOldInterface
        ? oldInterfaceIt->second
        : ScriptBridgeFrameLocalInterface();

    if (!ApplyFrameLocalInterface(behavior, locals, ownedLocalStart, strict, error)) {
        return false;
    }

    std::string commitError;
    if (commit(commitError)) {
        return true;
    }

    std::string restoreError;
    if (!RestoreOwnedFrameLocals(behavior, ownedLocalStart, localSnapshot, restoreError)) {
        error = commitError + " Failed to restore previous frame locals: " + restoreError;
        return false;
    }
    if (hadOldInterface) {
        m_FrameLocalInterfaces.insert_or_assign(behaviorId, oldInterface);
    } else {
        m_FrameLocalInterfaces.erase(behaviorId);
    }
    error = commitError;
    return false;
}

void ScriptBehaviorBridge::ClearFrameLocalInterface(CK_ID behaviorId, bool clearBindings) {
    if (behaviorId == 0) {
        return;
    }
    m_FrameLocalInterfaces.erase(behaviorId);
    if (clearBindings) {
        m_FrameLocalBindings.erase(behaviorId);
    }
}

bool ScriptBehaviorBridge::HasFrameLocalInterface(CK_ID behaviorId) const {
    return m_FrameLocalInterfaces.find(behaviorId) != m_FrameLocalInterfaces.end();
}

int ScriptBehaviorBridge::FrameLocalCount(CK_ID behaviorId) const {
    const auto it = m_FrameLocalInterfaces.find(behaviorId);
    return it == m_FrameLocalInterfaces.end() ? 0 : static_cast<int>(it->second.Locals.size());
}

int ScriptBehaviorBridge::FindFrameLocal(CK_ID behaviorId, const std::string &name, int occurrence) const {
    const auto it = m_FrameLocalInterfaces.find(behaviorId);
    if (it == m_FrameLocalInterfaces.end() || occurrence < 0) {
        return -1;
    }
    if (name.empty()) {
        return occurrence == 0 && !it->second.Locals.empty() ? 0 : -1;
    }

    int seen = 0;
    for (int i = 0; i < static_cast<int>(it->second.Locals.size()); ++i) {
        if (it->second.Locals[i].Name != name) {
            continue;
        }
        if (seen == occurrence) {
            return i;
        }
        ++seen;
    }
    return -1;
}

CKParameter *ScriptBehaviorBridge::ResolveFrameLocal(CKContext *context,
                                                     CK_ID behaviorId,
                                                     int index,
                                                     std::string &error) const {
    const auto it = m_FrameLocalInterfaces.find(behaviorId);
    if (it == m_FrameLocalInterfaces.end()) {
        error = "Behavior has no logical frame local interface.";
        return nullptr;
    }
    const ScriptBridgeFrameLocalInterface &interfaceLayout = it->second;
    if (index < 0 || index >= static_cast<int>(interfaceLayout.Locals.size())) {
        error = fmt::format("Logical frame local index {} is outside range 0..{}.",
                            index,
                            interfaceLayout.Locals.empty() ? -1 : static_cast<int>(interfaceLayout.Locals.size()) - 1);
        return nullptr;
    }

    const ScriptBridgeFrameLocalSlot &slot = interfaceLayout.Locals[index];
    CKContext *resolvedContext = context ? context : (m_Manager ? m_Manager->GetCKContext() : nullptr);
    if (slot.Mapped()) {
        std::string sourceError;
        CKParameter *source = ResolveStampedParameterSource(resolvedContext,
                                                            slot.SourceId,
                                                            slot.SourceStamp,
                                                            sourceError);
        if (!source) {
            error = "Logical frame local '" + slot.Name + "' source is invalid: " + sourceError;
            return nullptr;
        }
        if (source->GetGUID() != slot.TypeGuid) {
            error = "Logical frame local '" + slot.Name + "' source type changed. Expected " +
                DescribeScriptParamType(resolvedContext, slot.TypeGuid) + ", got " +
                DescribeScriptParamType(resolvedContext, source->GetGUID()) + ".";
            return nullptr;
        }
        return source;
    }

    CKBehavior *behavior = CKBehavior::Cast(GetStampedCKObjectById(resolvedContext,
                                                                   behaviorId,
                                                                   interfaceLayout.BehaviorStamp));
    CKParameterLocal *local = behavior &&
                              slot.OwnedLocalIndex >= 0 &&
                              slot.OwnedLocalIndex < behavior->GetLocalParameterCount()
        ? behavior->GetLocalParameter(slot.OwnedLocalIndex)
        : nullptr;
    if (!local) {
        error = "Owned logical frame local '" + slot.Name + "' is missing from the behavior.";
        return nullptr;
    }
    if (local->GetGUID() != slot.TypeGuid) {
        error = "Owned logical frame local '" + slot.Name + "' type changed. Expected " +
            DescribeScriptParamType(resolvedContext, slot.TypeGuid) + ", got " +
            DescribeScriptParamType(resolvedContext, local->GetGUID()) + ".";
        return nullptr;
    }
    return local;
}

std::string ScriptBehaviorBridge::DescribeFrameLocals(CK_ID behaviorId) const {
    const auto it = m_FrameLocalInterfaces.find(behaviorId);
    if (it == m_FrameLocalInterfaces.end()) {
        return "<none>";
    }
    std::string result;
    for (int i = 0; i < static_cast<int>(it->second.Locals.size()); ++i) {
        const ScriptBridgeFrameLocalSlot &slot = it->second.Locals[i];
        if (!result.empty()) {
            result += ", ";
        }
        result += fmt::format("#{} '{}'{}",
                              i,
                              slot.Name,
                              slot.Mapped() ? " (mapped)" : " (owned)");
    }
    return result.empty() ? "<none>" : result;
}
