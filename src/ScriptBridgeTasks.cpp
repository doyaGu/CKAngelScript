#include "ScriptBridgeHandles.h"

#include <fmt/format.h>

namespace {

int ResolveResultOutputName(ScriptBehaviorBridge *bridge,
                            CKBehavior *behavior,
                            ScriptBridgeSlotKind kind,
                            const std::string &name,
                            int occurrence,
                            std::string &error) {
    if (!behavior) {
        error = "BBResult behavior is not available.";
        return -1;
    }
    if (occurrence < 0) {
        error = "BBResult name occurrence must be non-negative.";
        return -1;
    }
    const ScriptBridgeLayoutRecord *layout = bridge
        ? bridge->GetBehaviorLayout(behavior->GetID(), CaptureBridgeObjectStamp(behavior))
        : nullptr;
    const int generation = layout ? layout->LayoutGeneration : -1;

    auto nameMatches = [&](const std::string &candidate) {
        return name.empty() ? true : NameEquals(const_cast<CKSTRING>(candidate.c_str()), name);
    };

    int seen = 0;
    std::string candidates;
    auto appendCandidate = [&](int index, const std::string &candidate, const std::string &typeName) {
        if (!candidates.empty()) {
            candidates += ", ";
        }
        candidates += fmt::format("#{} '{}'", index, candidate);
        if (!typeName.empty()) {
            candidates += fmt::format(": {}", typeName);
        }
    };

    if (layout) {
        if (kind == ScriptBridgeSlotKind::Output) {
            for (int i = 0; i < static_cast<int>(layout->Outputs.size()); ++i) {
                const std::string &candidate = layout->Outputs[i].Name;
                appendCandidate(i, candidate, std::string());
                if (nameMatches(candidate)) {
                    if (seen == occurrence) return i;
                    ++seen;
                }
            }
        } else {
            for (const ScriptBridgeLayoutParamSlot &slot : layout->Pouts) {
                appendCandidate(slot.Index, slot.Name, slot.TypeName);
                if (nameMatches(slot.Name)) {
                    if (seen == occurrence) return slot.Index;
                    ++seen;
                }
            }
        }
    } else if (kind == ScriptBridgeSlotKind::Output) {
        for (int i = 0; i < behavior->GetOutputCount(); ++i) {
            CKBehaviorIO *output = behavior->GetOutput(i);
            const std::string candidate = SafeString(output ? output->GetName() : nullptr);
            appendCandidate(i, candidate, std::string());
            if (nameMatches(candidate)) {
                if (seen == occurrence) return i;
                ++seen;
            }
        }
    } else {
        for (int i = 0; i < behavior->GetOutputParameterCount(); ++i) {
            CKParameterOut *pout = behavior->GetOutputParameter(i);
            const std::string candidate = SafeString(pout ? pout->GetName() : nullptr);
            appendCandidate(i, candidate, ParameterTypeLabel(behavior->GetCKContext(), pout));
            if (nameMatches(candidate)) {
                if (seen == occurrence) return i;
                ++seen;
            }
        }
    }

    error = fmt::format("BBResult could not resolve {} '{}' occurrence {} for Building Block '{}' at layout generation {}. Candidates: {}.",
        kind == ScriptBridgeSlotKind::Output ? "output" : "pout",
        name,
        occurrence,
        SafeString(behavior->GetPrototypeName()),
        generation,
        candidates.empty() ? "<none>" : candidates);
    return -1;
}

} // namespace

BBResult::BBResult(ScriptBehaviorBridge *bridge, CK_ID resultId, int generation)
    : m_Bridge(bridge), m_ResultId(resultId), m_Generation(generation) {}

BBResult::~BBResult() {
    if (m_Bridge && m_ResultId) {
        m_Bridge->DestroyResult(m_ResultId, m_Generation);
    }
}

ScriptBridgeExecutionState BBResult::State() const {
    if (m_Bridge) {
        return m_Bridge->GetResultState(m_ResultId, m_Generation);
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "BBResult is not valid.";
    return state;
}

bool BBResult::Ok() const { return State().Ok; }
int BBResult::ReturnCode() const { return State().ReturnCode; }
std::string BBResult::Error() const { return State().Error; }
bool BBResult::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }
bool BBResult::Raise(const CKBehaviorContext &ctx) const { return RaiseExecutionState(State(), ctx); }

bool BBResult::Raise(BehaviorFrame *frame) const {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "BBResult.Raise") && Raise(ctx);
}

bool BBResult::OutputActiveSlot(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    return output && output->ResolveIndex(ScriptBridgeSlotKind::Output, outputIndex, error) && OutputActive(outputIndex);
}

bool BBResult::OutputActiveName(const std::string &name, int occurrence) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetResultBehavior(m_ResultId, m_Generation) : nullptr;
    std::string error;
    const int index = ResolveResultOutputName(m_Bridge, behavior, ScriptBridgeSlotKind::Output, name, occurrence, error);
    if (index < 0) {
        SetScriptException(error);
        return false;
    }
    return OutputActive(index);
}

ParamRef *BBResult::Pout(int index) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetResultBehavior(m_ResultId, m_Generation) : nullptr;
    CKParameterOut *pout = behavior && index >= 0 && index < behavior->GetOutputParameterCount() ? behavior->GetOutputParameter(index) : nullptr;
    return m_Bridge && pout ? new ParamRef(m_Bridge, pout->GetID(), ScriptBridgeSlotKind::Pout, index, behavior->GetID()) : nullptr;
}

ParamRef *BBResult::PoutName(const std::string &name, int occurrence) const {
    CKBehavior *behavior = m_Bridge ? m_Bridge->GetResultBehavior(m_ResultId, m_Generation) : nullptr;
    std::string error;
    const int index = ResolveResultOutputName(m_Bridge, behavior, ScriptBridgeSlotKind::Pout, name, occurrence, error);
    if (index < 0) {
        SetScriptException(error);
        return nullptr;
    }
    return Pout(index);
}

ParamRef *BBResult::PoutSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pout, index, error)) {
        SetScriptException(error.empty() ? "BBResult.Pout requires a pout BBSlot." : error);
        return nullptr;
    }
    return Pout(index);
}

BBTask::BBTask(ScriptBehaviorBridge *bridge, CK_ID taskId, int generation)
    : m_Bridge(bridge), m_TaskId(taskId), m_Generation(generation) {}

BBTask::~BBTask() { Destroy(); }

ScriptBridgeExecutionState BBTask::State() const {
    if (m_Bridge) {
        return m_Bridge->GetTaskState(m_TaskId, m_Generation);
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "BBTask is not valid.";
    return state;
}

bool BBTask::IsValid() const { return m_Bridge && m_TaskId != 0 && m_Bridge->IsTaskValid(m_TaskId, m_Generation); }
bool BBTask::IsAlive() const { return m_Bridge && m_Bridge->IsTaskAlive(m_TaskId, m_Generation); }
bool BBTask::IsPaused() const { return m_Bridge && m_Bridge->IsTaskPaused(m_TaskId, m_Generation); }
int BBTask::ReturnCode() const { return State().ReturnCode; }
std::string BBTask::Error() const { return State().Error; }
bool BBTask::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }
bool BBTask::Step(const CKBehaviorContext &ctx, int inputIndex) { return m_Bridge && m_Bridge->StepTask(m_TaskId, m_Generation, ctx, inputIndex); }

bool BBTask::Step(BehaviorFrame *frame, int inputIndex) {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "BBTask.Step") && Step(ctx, inputIndex);
}
bool BBTask::Reset() { return m_Bridge && m_Bridge->ResetTask(m_TaskId, m_Generation); }

bool BBTask::OutputActiveSlot(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    return output && output->ResolveIndex(ScriptBridgeSlotKind::Output, outputIndex, error) && OutputActive(outputIndex);
}

bool BBTask::StepSlot(const CKBehaviorContext &ctx, BBSlot *input) {
    int inputIndex = -1;
    std::string error;
    if (!input || !input->ResolveIndex(ScriptBridgeSlotKind::Input, inputIndex, error)) {
        SetScriptException(error.empty() ? "BBTask.Step requires an input BBSlot." : error);
        return false;
    }
    return Step(ctx, inputIndex);
}

bool BBTask::StepSlot(BehaviorFrame *frame, BBSlot *input) {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "BBTask.Step") && StepSlot(ctx, input);
}

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

ParamRef *BBTask::PoutSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pout, index, error)) {
        SetScriptException(error.empty() ? "BBTask.Pout requires a pout BBSlot." : error);
        return nullptr;
    }
    return Pout(index);
}

bool BBTask::Raise(const CKBehaviorContext &ctx) const {
    return RaiseExecutionState(State(), ctx);
}

bool BBTask::Raise(BehaviorFrame *frame) const {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "BBTask.Raise") && Raise(ctx);
}

int BBTask::BridgeGeneration() const { return m_Generation; }

GraphTask::GraphTask(ScriptBehaviorBridge *bridge, CK_ID watchId, int generation)
    : m_Bridge(bridge), m_WatchId(watchId), m_Generation(generation) {}

GraphTask::~GraphTask() { Cancel(); }

ScriptBridgeExecutionState GraphTask::State() const {
    if (m_Bridge) {
        return m_Bridge->GetGraphWatchState(m_WatchId, m_Generation);
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "GraphTask is not valid.";
    return state;
}

bool GraphTask::IsValid() const { return m_Bridge && m_WatchId != 0 && m_Bridge->IsGraphWatchValid(m_WatchId, m_Generation); }
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

bool GraphTask::Step(BehaviorFrame *frame) {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "GraphTask.Step") && Step(ctx);
}
bool GraphTask::Done(int outputIndex) const { return m_Bridge && m_Bridge->IsGraphWatchDone(m_WatchId, m_Generation, outputIndex); }
bool GraphTask::OutputActive(int outputIndex) const { return ExecutionOutputActive(State(), outputIndex); }

bool GraphTask::DoneSlot(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    return output && output->ResolveIndex(ScriptBridgeSlotKind::Output, outputIndex, error) && Done(outputIndex);
}

bool GraphTask::OutputActiveSlot(BBSlot *output) const {
    int outputIndex = -1;
    std::string error;
    return output && output->ResolveIndex(ScriptBridgeSlotKind::Output, outputIndex, error) && OutputActive(outputIndex);
}

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

ParamRef *GraphTask::PoutSlot(BBSlot *slot) const {
    int index = -1;
    std::string error;
    if (!slot || !slot->ResolveIndex(ScriptBridgeSlotKind::Pout, index, error)) {
        SetScriptException(error.empty() ? "GraphTask.Pout requires a pout BBSlot." : error);
        return nullptr;
    }
    return Pout(index);
}

bool GraphTask::Raise(const CKBehaviorContext &ctx) const {
    return RaiseExecutionState(State(), ctx);
}

bool GraphTask::Raise(BehaviorFrame *frame) const {
    CKBehaviorContext ctx = {};
    return MakeContextFromFrame(frame, ctx, "GraphTask.Raise") && Raise(ctx);
}
