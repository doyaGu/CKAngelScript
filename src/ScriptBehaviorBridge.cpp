#include "ScriptBridgeHandles.h"

#include <algorithm>
#include <functional>

#include <fmt/format.h>

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

std::vector<int> ScriptBridgeExecutionState::IndexSet::ToVector() const {
    std::vector<int> result;
    for (int i = 0; i < 32; ++i) {
        if ((InlineMask & (1u << i)) != 0) {
            result.push_back(i);
        }
    }
    result.insert(result.end(), Overflow.begin(), Overflow.end());
    return result;
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

void ScriptBehaviorBridge::Clear() {
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
