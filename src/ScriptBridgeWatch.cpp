#include "ScriptBridgeHandles.h"

#include <algorithm>

#include <fmt/format.h>

GraphTask *ScriptBehaviorBridge::CreateGraphWatch(CKBehavior *behavior, CK_ID componentId, float timeoutSeconds) {
    if (!behavior) {
        SetScriptException("GraphTask target behavior is not valid.");
        return nullptr;
    }

    ScriptBridgeGraphWatch record;
    record.WatchId = m_NextGraphWatchId++;
    record.Generation = m_NextGeneration++;
    record.ComponentId = componentId;
    record.BehaviorId = behavior->GetID();
    record.BehaviorStamp = CaptureBridgeObjectStamp(behavior);
    record.SetFlag(ScriptBridgeTaskFlags::Alive, true);
    record.TimeoutSeconds = timeoutSeconds > 0.0f ? timeoutSeconds : 0.0f;
    record.LastState = CaptureExecutionState(behavior, CKBR_OK);
    record.SeenOutputs = record.LastState.SeenOutputs;

    auto [it, inserted] = m_GraphWatches.emplace(record.WatchId, std::move(record));
    (void) inserted;
    return new GraphTask(this, it->second.WatchId, it->second.Generation);
}

bool ScriptBehaviorBridge::StepGraphWatch(CK_ID watchId, int generation, const CKBehaviorContext &ctx) {
    ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record || !record->HasFlag(ScriptBridgeTaskFlags::Alive)) {
        SetScriptException("GraphTask is not alive.");
        return false;
    }

    if (record->HasFlag(ScriptBridgeTaskFlags::Paused)) {
        record->LastState.Ok = false;
        record->LastState.Error = "GraphTask is paused.";
        record->Error = record->LastState.Error;
        return false;
    }

    CKBehavior *behavior = GetGraphWatchBehavior(watchId, generation);
    if (!behavior) {
        record->SetFlag(ScriptBridgeTaskFlags::Alive, false);
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = fmt::format("GraphTask target behavior id={} no longer exists.", record->BehaviorId);
        record->Error = record->LastState.Error;
        return false;
    }

    record->Elapsed += ctx.DeltaTime > 0.0f ? ctx.DeltaTime : 0.0f;
    record->LastState = CaptureExecutionState(behavior, CKBR_OK);
    record->SeenOutputs.MergeFrom(record->LastState.ActiveOutputs);
    record->LastState.SeenOutputs = record->SeenOutputs;

    if (record->TimeoutSeconds > 0.0f && record->Elapsed >= record->TimeoutSeconds) {
        record->SetFlag(ScriptBridgeTaskFlags::Alive, false);
        record->SetFlag(ScriptBridgeTaskFlags::TimedOut, true);
        record->Error = fmt::format("GraphTask timed out after {:.3f}s while watching behavior '{}' id={}.",
            record->TimeoutSeconds,
            SafeString(behavior->GetName()),
            behavior->GetID());
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = record->Error;
        return false;
    }

    record->Error.clear();
    return true;
}

bool ScriptBehaviorBridge::CancelGraphWatch(CK_ID watchId, int generation) {
    auto it = m_GraphWatches.find(watchId);
    if (it == m_GraphWatches.end() || it->second.Generation != generation) {
        return false;
    }
    m_GraphWatches.erase(it);
    return true;
}

bool ScriptBehaviorBridge::ResetGraphWatch(CK_ID watchId, int generation) {
    ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record) {
        return false;
    }

    record->SetFlag(ScriptBridgeTaskFlags::Alive, true);
    record->SetFlag(ScriptBridgeTaskFlags::Paused, false);
    record->SetFlag(ScriptBridgeTaskFlags::TimedOut, false);
    record->Elapsed = 0.0f;
    record->Error.clear();
    record->LastState = ScriptBridgeExecutionState();
    record->SeenOutputs.Clear();
    if (CKBehavior *behavior = GetGraphWatchBehavior(watchId, generation)) {
        record->LastState = CaptureExecutionState(behavior, CKBR_OK);
        record->SeenOutputs = record->LastState.SeenOutputs;
    }
    return true;
}

bool ScriptBehaviorBridge::SetGraphWatchTimeout(CK_ID watchId, int generation, float timeoutSeconds) {
    ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record) {
        return false;
    }
    record->TimeoutSeconds = timeoutSeconds > 0.0f ? timeoutSeconds : 0.0f;
    return true;
}

bool ScriptBehaviorBridge::IsGraphWatchAlive(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::Alive) && GetGraphWatchBehavior(watchId, generation);
}

bool ScriptBehaviorBridge::IsGraphWatchValid(CK_ID watchId, int generation) const {
    return GetGraphWatchBehavior(watchId, generation) != nullptr;
}

bool ScriptBehaviorBridge::IsGraphWatchPaused(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::Paused);
}

bool ScriptBehaviorBridge::IsGraphWatchTimedOut(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::TimedOut);
}

float ScriptBehaviorBridge::GetGraphWatchElapsed(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record ? record->Elapsed : 0.0f;
}

std::string ScriptBehaviorBridge::GetGraphWatchError(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record) {
        return "GraphTask is not valid.";
    }
    return record->Error.empty() ? record->LastState.Error : record->Error;
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetGraphWatchState(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (record) {
        return record->LastState;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "GraphTask is not valid.";
    return state;
}

bool ScriptBehaviorBridge::IsGraphWatchDone(CK_ID watchId, int generation, int outputIndex) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record) {
        return false;
    }
    if (outputIndex < 0) {
        return !record->SeenOutputs.Empty();
    }
    return record->SeenOutputs.Contains(outputIndex);
}

CKBehavior *ScriptBehaviorBridge::GetGraphWatchBehavior(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetStampedCKObjectById(m_Manager->GetCKContext(), record->BehaviorId, record->BehaviorStamp));
}
ScriptBehaviorBridge::ScriptBridgeGraphWatch *ScriptBehaviorBridge::FindGraphWatch(CK_ID watchId, int generation) {
    auto it = m_GraphWatches.find(watchId);
    if (it == m_GraphWatches.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}

const ScriptBehaviorBridge::ScriptBridgeGraphWatch *ScriptBehaviorBridge::FindGraphWatch(CK_ID watchId, int generation) const {
    auto it = m_GraphWatches.find(watchId);
    if (it == m_GraphWatches.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}
