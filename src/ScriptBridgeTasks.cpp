#include "ScriptBridgeHandles.h"

BBResult::BBResult(ScriptBehaviorBridge *bridge, CK_ID resultId, int generation)
    : m_Bridge(bridge), m_ResultId(resultId), m_Generation(generation) {}

BBResult::~BBResult() {
    if (m_Bridge && m_ResultId) {
        m_Bridge->DestroyResult(m_ResultId, m_Generation);
    }
}

ScriptBridgeExecutionState BBResult::State() const {
    return m_Bridge ? m_Bridge->GetResultState(m_ResultId, m_Generation)
                    : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "BBResult is not valid.", {}, {}};
}

bool BBResult::Ok() const { return State().Ok; }
int BBResult::ReturnCode() const { return State().ReturnCode; }
std::string BBResult::Error() const { return State().Error; }
bool BBResult::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }
bool BBResult::Raise(const CKBehaviorContext &ctx) const { return RaiseExecutionState(State(), ctx); }

ParamRef *BBResult::Pout(int index) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetResultBehavior(m_ResultId, m_Generation) : nullptr;
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

BBTask::BBTask(ScriptBehaviorBridge *bridge, CK_ID taskId, int generation)
    : m_Bridge(bridge), m_TaskId(taskId), m_Generation(generation) {}

BBTask::~BBTask() { Destroy(); }

ScriptBridgeExecutionState BBTask::State() const {
    return m_Bridge ? m_Bridge->GetTaskState(m_TaskId, m_Generation)
                    : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "BBTask is not valid.", {}, {}};
}

bool BBTask::IsValid() const { return m_Bridge && m_TaskId != 0; }
bool BBTask::IsAlive() const { return m_Bridge && m_Bridge->IsTaskAlive(m_TaskId, m_Generation); }
bool BBTask::IsPaused() const { return m_Bridge && m_Bridge->IsTaskPaused(m_TaskId, m_Generation); }
int BBTask::ReturnCode() const { return State().ReturnCode; }
std::string BBTask::Error() const { return State().Error; }
bool BBTask::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }
bool BBTask::Step(const CKBehaviorContext &ctx, int inputIndex) { return m_Bridge && m_Bridge->StepTask(m_TaskId, m_Generation, ctx, inputIndex); }
bool BBTask::Reset() { return m_Bridge && m_Bridge->ResetTask(m_TaskId, m_Generation); }

bool BBTask::Destroy() {
    if (!m_Bridge || m_TaskId == 0) {
        return false;
    }
    const bool result = m_Bridge->DestroyTask(m_TaskId, m_Generation);
    m_TaskId = 0;
    return result;
}

BehaviorRef *BBTask::Behavior() const {
    return m_Bridge ? m_Bridge->WrapBehavior(m_Bridge->GetTaskBehavior(m_TaskId, m_Generation)) : nullptr;
}

ParamRef *BBTask::Pout(int index) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetTaskBehavior(m_TaskId, m_Generation) : nullptr;
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

bool BBTask::Raise(const CKBehaviorContext &ctx) const {
    return RaiseExecutionState(State(), ctx);
}

GraphTask::GraphTask(ScriptBehaviorBridge *bridge, CK_ID watchId, int generation)
    : m_Bridge(bridge), m_WatchId(watchId), m_Generation(generation) {}

GraphTask::~GraphTask() { Cancel(); }

ScriptBridgeExecutionState GraphTask::State() const {
    return m_Bridge ? m_Bridge->GetGraphWatchState(m_WatchId, m_Generation)
                    : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "GraphTask is not valid.", {}, {}};
}

bool GraphTask::IsValid() const { return m_Bridge && m_WatchId != 0; }
bool GraphTask::IsAlive() const { return m_Bridge && m_Bridge->IsGraphWatchAlive(m_WatchId, m_Generation); }
bool GraphTask::IsPaused() const { return m_Bridge && m_Bridge->IsGraphWatchPaused(m_WatchId, m_Generation); }
bool GraphTask::TimedOut() const { return m_Bridge && m_Bridge->IsGraphWatchTimedOut(m_WatchId, m_Generation); }
float GraphTask::Elapsed() const { return m_Bridge ? m_Bridge->GetGraphWatchElapsed(m_WatchId, m_Generation) : 0.0f; }
std::string GraphTask::Error() const { return m_Bridge ? m_Bridge->GetGraphWatchError(m_WatchId, m_Generation) : "GraphTask is not valid."; }

GraphTask *GraphTask::Timeout(float seconds) {
    if (m_Bridge && m_Bridge->SetGraphWatchTimeout(m_WatchId, m_Generation, seconds)) {
        AddRef();
        return this;
    }
    return nullptr;
}

bool GraphTask::Step(const CKBehaviorContext &ctx) { return m_Bridge && m_Bridge->StepGraphWatch(m_WatchId, m_Generation, ctx); }
bool GraphTask::Done(int outputIndex) const { return m_Bridge && m_Bridge->IsGraphWatchDone(m_WatchId, m_Generation, outputIndex); }
bool GraphTask::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }

bool GraphTask::Cancel() {
    if (!m_Bridge || m_WatchId == 0) {
        return false;
    }
    const bool result = m_Bridge->CancelGraphWatch(m_WatchId, m_Generation);
    m_WatchId = 0;
    return result;
}

bool GraphTask::Reset() { return m_Bridge && m_Bridge->ResetGraphWatch(m_WatchId, m_Generation); }

BehaviorRef *GraphTask::Behavior() const {
    return m_Bridge ? m_Bridge->WrapBehavior(m_Bridge->GetGraphWatchBehavior(m_WatchId, m_Generation)) : nullptr;
}

ParamRef *GraphTask::Pout(int index) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetGraphWatchBehavior(m_WatchId, m_Generation) : nullptr;
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

bool GraphTask::Raise(const CKBehaviorContext &ctx) const {
    return RaiseExecutionState(State(), ctx);
}
