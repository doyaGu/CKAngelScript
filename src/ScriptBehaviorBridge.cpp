#include "ScriptBridgeHandles.h"

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

    m_GraphWatches.clear();
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
            entry.second.Paused = paused;
        }
    }

    for (auto &entry : m_GraphWatches) {
        if (entry.second.ComponentId == componentId) {
            entry.second.Paused = paused;
        }
    }
}

BehaviorRef *ScriptBehaviorBridge::WrapBehavior(CKBehavior *behavior, CK_ID componentId) {
    return behavior ? new BehaviorRef(this, behavior->GetID(), componentId) : nullptr;
}

void ScriptBehaviorBridge::ReleaseBehaviorRef(BehaviorRef *ref) {
    if (ref) {
        ref->Release();
    }
}

ParamRef *ScriptBehaviorBridge::WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind, int index) {
    return parameter ? new ParamRef(this, parameter->GetID(), kind, index) : nullptr;
}

ParamOperationRef *ScriptBehaviorBridge::WrapParameterOperation(CKParameterOperation *operation) {
    return operation ? new ParamOperationRef(this, operation->GetID()) : nullptr;
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
    request.HasGuid = true;
    request.Guid = guid;
    return new BBPrototype(this, ctx, request);
}

void ScriptBehaviorBridge::ReleasePrototype(BBPrototype *prototype) {
    if (prototype) {
        prototype->Release();
    }
}

BBResult *ScriptBehaviorBridge::RunCall(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex) {
    ScriptBridgeExecutionState state;
    std::unordered_map<int, CK_ID> inputSources;
    std::vector<CK_ID> operationIds;
    CKBehavior *behavior = CreateRuntimeBehavior(request, ctx, state, &inputSources, &operationIds);

    if (behavior) {
        state = ExecuteOnce(behavior, ctx, inputIndex, &inputSources, true);
    }

    if (behavior && (state.ReturnCode & CKBR_ACTIVATENEXTFRAME)) {
        state.Ok = false;
        state.Error = "Building Block requested next-frame execution; use BB::Spawn(...).Start() instead of BB::Call(...).Run().";
        behavior->Activate(FALSE, TRUE);
    }

    ResultRecord record;
    record.ResultId = m_NextResultId++;
    record.Generation = m_NextGeneration++;
    record.ComponentId = request.ComponentId;
    record.BehaviorId = behavior ? behavior->GetID() : 0;
    record.State = state;
    record.OperationIds = std::move(operationIds);

    auto [it, inserted] = m_Results.emplace(record.ResultId, std::move(record));
    (void) inserted;
    return new BBResult(this, it->second.ResultId, it->second.Generation);
}

BBTask *ScriptBehaviorBridge::StartTask(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex) {
    ScriptBridgeExecutionState state;
    auto sources = std::unordered_map<int, CK_ID>();
    std::vector<CK_ID> operations;
    CKBehavior *behavior = CreateRuntimeBehavior(request, ctx, state, &sources, &operations);
    if (!behavior) {
        SetScriptException(state.Error.empty() ? "Failed to start BBTask." : state.Error);
        return nullptr;
    }

    TaskRecord record;
    record.TaskId = m_NextTaskId++;
    record.Generation = m_NextGeneration++;
    record.ComponentId = request.ComponentId;
    record.BehaviorId = behavior->GetID();
    record.Alive = true;
    record.InputSources = std::move(sources);
    record.OperationIds = std::move(operations);

    auto [it, inserted] = m_Tasks.emplace(record.TaskId, std::move(record));
    (void) inserted;

    it->second.LastState = ExecuteOnce(behavior, ctx, inputIndex, &it->second.InputSources, true);

    return new BBTask(this, it->second.TaskId, it->second.Generation);
}

bool ScriptBehaviorBridge::StepTask(CK_ID taskId, int generation, const CKBehaviorContext &ctx, int inputIndex) {
    TaskRecord *record = FindTask(taskId, generation);
    if (!record || !record->Alive) {
        SetScriptException("BBTask is not alive.");
        return false;
    }

    if (record->Paused) {
        record->LastState.Ok = false;
        record->LastState.Error = "BBTask is paused.";
        return false;
    }

    CKBehavior *behavior = GetTaskBehavior(taskId, generation);
    if (!behavior) {
        record->LastState.Ok = false;
        record->LastState.Error = "BBTask behavior no longer exists.";
        return false;
    }

    if (inputIndex < 0 && !behavior->IsActive()) {
        return record->LastState.Ok;
    }

    record->LastState = ExecuteOnce(behavior, ctx, inputIndex, &record->InputSources, false);
    return record->LastState.Ok;
}

bool ScriptBehaviorBridge::DestroyTask(CK_ID taskId, int generation) {
    auto it = m_Tasks.find(taskId);
    if (it == m_Tasks.end() || it->second.Generation != generation) {
        return false;
    }

    CKBehavior *behavior = GetTaskBehavior(taskId, generation);
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    for (CK_ID operationId : it->second.OperationIds) {
        if (context) {
            if (CKObject *op = GetCKObjectById(context, operationId)) {
                context->DestroyObject(op);
            }
        }
    }
    if (behavior) {
        QueueDestroy(behavior, true, it->second.DeleteCallbackSent);
    }
    m_Tasks.erase(it);
    return true;
}

bool ScriptBehaviorBridge::ResetTask(CK_ID taskId, int generation) {
    TaskRecord *record = FindTask(taskId, generation);
    if (!record) {
        return false;
    }

    CKBehavior *behavior = GetTaskBehavior(taskId, generation);
    if (!behavior) {
        record->LastState.Ok = false;
        record->LastState.Error = "BBTask behavior no longer exists.";
        return false;
    }

    ClearInputs(behavior);
    ClearOutputs(behavior);
    behavior->Activate(FALSE, TRUE);
    behavior->CallCallbackFunction(CKM_BEHAVIORRESET);
    record->LastState = ScriptBridgeExecutionState();
    return true;
}

bool ScriptBehaviorBridge::IsTaskAlive(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    return record && record->Alive;
}

bool ScriptBehaviorBridge::IsTaskPaused(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    return record && record->Paused;
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetTaskState(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    return record ? record->LastState : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "BBTask is not valid.", {}, {}};
}

CKBehavior *ScriptBehaviorBridge::GetTaskBehavior(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetCKObjectById(m_Manager->GetCKContext(), record->BehaviorId));
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetResultState(CK_ID resultId, int generation) const {
    const ResultRecord *record = FindResult(resultId, generation);
    return record ? record->State : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "BBResult is not valid.", {}, {}};
}

CKBehavior *ScriptBehaviorBridge::GetResultBehavior(CK_ID resultId, int generation) const {
    const ResultRecord *record = FindResult(resultId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetCKObjectById(m_Manager->GetCKContext(), record->BehaviorId));
}

bool ScriptBehaviorBridge::DestroyResult(CK_ID resultId, int generation) {
    auto it = m_Results.find(resultId);
    if (it == m_Results.end() || it->second.Generation != generation) {
        return false;
    }

    CKBehavior *behavior = GetResultBehavior(resultId, generation);
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    for (CK_ID operationId : it->second.OperationIds) {
        if (context) {
            if (CKObject *op = GetCKObjectById(context, operationId)) {
                context->DestroyObject(op);
            }
        }
    }
    if (behavior) {
        QueueDestroy(behavior, true, it->second.DeleteCallbackSent);
    }
    m_Results.erase(it);
    return true;
}

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
    record.Alive = true;
    record.TimeoutSeconds = timeoutSeconds > 0.0f ? timeoutSeconds : 0.0f;
    record.LastState = CaptureExecutionState(behavior, CKBR_OK);
    record.SeenOutputs = record.LastState.SeenOutputs;

    auto [it, inserted] = m_GraphWatches.emplace(record.WatchId, std::move(record));
    (void) inserted;
    return new GraphTask(this, it->second.WatchId, it->second.Generation);
}

bool ScriptBehaviorBridge::StepGraphWatch(CK_ID watchId, int generation, const CKBehaviorContext &ctx) {
    ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record || !record->Alive) {
        SetScriptException("GraphTask is not alive.");
        return false;
    }

    if (record->Paused) {
        record->LastState.Ok = false;
        record->LastState.Error = "GraphTask is paused.";
        record->Error = record->LastState.Error;
        return false;
    }

    CKBehavior *behavior = GetGraphWatchBehavior(watchId, generation);
    if (!behavior) {
        record->Alive = false;
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = fmt::format("GraphTask target behavior id={} no longer exists.", record->BehaviorId);
        record->Error = record->LastState.Error;
        return false;
    }

    record->Elapsed += ctx.DeltaTime > 0.0f ? ctx.DeltaTime : 0.0f;
    record->LastState = CaptureExecutionState(behavior, CKBR_OK);
    for (int output : record->LastState.ActiveOutputs) {
        if (std::find(record->SeenOutputs.begin(), record->SeenOutputs.end(), output) == record->SeenOutputs.end()) {
            record->SeenOutputs.push_back(output);
        }
    }
    record->LastState.SeenOutputs = record->SeenOutputs;

    if (record->TimeoutSeconds > 0.0f && record->Elapsed >= record->TimeoutSeconds) {
        record->Alive = false;
        record->TimedOut = true;
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

    record->Alive = true;
    record->Paused = false;
    record->TimedOut = false;
    record->Elapsed = 0.0f;
    record->Error.clear();
    record->LastState = ScriptBridgeExecutionState();
    record->SeenOutputs.clear();
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
    return record && record->Alive;
}

bool ScriptBehaviorBridge::IsGraphWatchPaused(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record && record->Paused;
}

bool ScriptBehaviorBridge::IsGraphWatchTimedOut(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    return record && record->TimedOut;
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
    return record ? record->LastState : ScriptBridgeExecutionState{false, CKBR_BEHAVIORERROR, "GraphTask is not valid.", {}, {}};
}

bool ScriptBehaviorBridge::IsGraphWatchDone(CK_ID watchId, int generation, int outputIndex) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record) {
        return false;
    }
    if (outputIndex < 0) {
        return !record->SeenOutputs.empty();
    }
    return std::find(record->SeenOutputs.begin(), record->SeenOutputs.end(), outputIndex) != record->SeenOutputs.end();
}

CKBehavior *ScriptBehaviorBridge::GetGraphWatchBehavior(CK_ID watchId, int generation) const {
    const ScriptBridgeGraphWatch *record = FindGraphWatch(watchId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetCKObjectById(m_Manager->GetCKContext(), record->BehaviorId));
}

CKBehaviorPrototype *ScriptBehaviorBridge::ResolvePrototypeObject(const ScriptBridgeBBInvocationSpec &request, std::string &error) const {
    CKGUID guid;
    if (!ResolvePrototype(request, guid, error)) {
        return nullptr;
    }

    CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(guid);
    if (!prototype) {
        error = fmt::format("Building Block prototype {} was not found.", GuidToString(guid));
    }
    return prototype;
}


bool ScriptBehaviorBridge::ApplyInputGraphRequests(CKBehavior *behavior,
                                                   const ScriptBridgeBBInvocationSpec &request,
                                                   std::string &error,
                                                   std::unordered_map<int, CK_ID> *inputSources,
                                                   std::vector<CK_ID> *operationIds) {
    (void) inputSources;
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    CKContext *context = behavior->GetCKContext();
    for (const ScriptBridgeInputSource &sourceRequest : request.SourceParameters) {
        CKParameter *source = ResolveParameterSource(context, sourceRequest.SourceId);
        if (!source) {
            error = "Input source parameter is not valid.";
            return false;
        }
        if (sourceRequest.PinIndex < 0 || sourceRequest.PinIndex >= behavior->GetInputParameterCount()) {
            error = fmt::format("Input parameter index #{} is out of range for Building Block '{}' (input parameter count: {}).",
                sourceRequest.PinIndex,
                SafeString(behavior->GetPrototypeName()),
                behavior->GetInputParameterCount());
            return false;
        }
        CKParameterIn *input = behavior->GetInputParameter(sourceRequest.PinIndex);
        CKERROR err = input ? input->SetDirectSource(source) : CKERR_INVALIDPARAMETER;
        if (err != CK_OK) {
            error = fmt::format("Failed to set source for input parameter #{} '{}' (expected {}, got {}, CKERROR {}).",
                sourceRequest.PinIndex,
                SafeString(input ? input->GetName() : nullptr),
                ParameterTypeLabel(behavior->GetCKContext(), input),
                ParameterTypeLabel(behavior->GetCKContext(), source),
                err);
            return false;
        }
    }

    for (const ScriptBridgeOperationSpec &operationRequest : request.OperationParameters) {
        ParamOperationRef *operation = ConnectOperationToInput(this, behavior, operationRequest.TargetPinIndex, operationRequest, error, true, operationIds);
        if (!operation) {
            return false;
        }
        operation->Release();
    }

    return true;
}

CKBehavior *ScriptBehaviorBridge::CreateRuntimeBehavior(const ScriptBridgeBBInvocationSpec &request,
                                                        const CKBehaviorContext &ctx,
                                                        ScriptBridgeExecutionState &state,
                                                        std::unordered_map<int, CK_ID> *inputSources,
                                                        std::vector<CK_ID> *operationIds) {
    if (!m_Manager || !m_Manager->GetCKContext()) {
        state.Ok = false;
        state.Error = "Script manager or CKContext is not available.";
        return nullptr;
    }

    CKContext *context = m_Manager->GetCKContext();
    CKGUID guid;
    std::string error;
    if (!ResolvePrototype(request, guid, error)) {
        state.Ok = false;
        state.Error = error;
        return nullptr;
    }

    const std::string name = fmt::format("__CKAS_BB_{}_{}", request.ComponentId, m_NextRuntimeId++);
    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, const_cast<CKSTRING>(name.c_str()), CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        state.Ok = false;
        state.Error = "Failed to create runtime behavior.";
        return nullptr;
    }

    bool createCallbackSent = false;
    auto fail = [&](const std::string &message) -> CKBehavior * {
        state.Ok = false;
        state.Error = message;
        QueueDestroy(behavior, createCallbackSent);
        return nullptr;
    };

    behavior->UseFunction();
    CKERROR err = behavior->InitFromGuid(guid);
    if (err != CK_OK) {
        return fail(fmt::format("Failed to initialize Building Block from GUID ({}, {}) with CKERROR {}.", guid.d[0], guid.d[1], err));
    }

    CKBeObject *owner = request.OwnerId ? CKBeObject::Cast(GetCKObjectById(context, request.OwnerId)) : nullptr;
    CKBeObject *target = request.TargetId ? CKBeObject::Cast(GetCKObjectById(context, request.TargetId)) : nullptr;
    const CK_CLASSID compatibleClassId = behavior->GetCompatibleClassID();

    if (target) {
        if (!behavior->IsTargetable()) {
            return fail(fmt::format("Building Block '{}' is not targetable.", SafeString(behavior->GetPrototypeName())));
        }

        if (!CKIsChildClassOf(target, compatibleClassId)) {
            return fail(fmt::format("Target '{}' is not compatible with Building Block '{}'.",
                SafeString(target->GetName()),
                SafeString(behavior->GetPrototypeName())));
        }

        err = behavior->UseTarget(TRUE);
        if (err != CK_OK) {
            return fail(fmt::format("Failed to enable target parameter for Building Block '{}' (CKERROR {}).",
                SafeString(behavior->GetPrototypeName()),
                err));
        }

        CKParameterIn *targetParam = behavior->GetTargetParameter();
        if (!targetParam) {
            return fail("Target parameter was not created.");
        }

        CKParameterLocal *targetSource = behavior->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_Target"), targetParam->GetGUID());
        if (!targetSource) {
            return fail("Failed to create target source parameter.");
        }

        CK_ID targetId = target->GetID();
        if (targetSource->SetValue(&targetId, sizeof(targetId)) != CK_OK || targetParam->SetDirectSource(targetSource) != CK_OK) {
            return fail("Failed to set target parameter.");
        }
    } else {
        if (owner && !CKIsChildClassOf(owner, compatibleClassId)) {
            return fail(fmt::format("Owner '{}' is not compatible with Building Block '{}'.",
                SafeString(owner->GetName()),
                SafeString(behavior->GetPrototypeName())));
        }

        if (!owner && compatibleClassId != CKCID_OBJECT && compatibleClassId != CKCID_BEOBJECT) {
            return fail(fmt::format("Building Block '{}' requires an owner or target.", SafeString(behavior->GetPrototypeName())));
        }
    }

    err = behavior->SetOwner(owner, FALSE);
    if (err != CK_OK) {
        return fail(fmt::format("Failed to set Building Block owner (CKERROR {}).", err));
    }

    err = behavior->CallCallbackFunction(CKM_BEHAVIORCREATE);
    if (err != CK_OK) {
        return fail(fmt::format("Building Block CREATE callback failed (CKERROR {}).", err));
    }
    createCallbackSent = true;

    if (owner) {
        err = behavior->CallCallbackFunction(CKM_BEHAVIORATTACH);
        if (err != CK_OK) {
            return fail(fmt::format("Building Block ATTACH callback failed (CKERROR {}).", err));
        }
    }

    if (!ApplyIndexedInputParameters(behavior, request.IndexedParameters, error, inputSources)) {
        return fail(error);
    }

    if (!ApplyInputGraphRequests(behavior, request, error, inputSources, operationIds)) {
        return fail(error);
    }

    return behavior;
}

bool ScriptBehaviorBridge::ResolvePrototype(const ScriptBridgeBBInvocationSpec &request, CKGUID &guid, std::string &error) const {
    if (request.HasGuid) {
        if (!request.Guid.IsValid()) {
            error = "Building Block GUID is invalid.";
            return false;
        }
        if (!CKGetPrototypeFromGuid(request.Guid)) {
            error = fmt::format("Building Block prototype {} was not found.", GuidToString(request.Guid));
            return false;
        }
        guid = request.Guid;
        return true;
    }

    if (request.PrototypeName.empty()) {
        error = "Building Block name is empty.";
        return false;
    }

    CKGUID found;
    int matches = 0;
    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (!decl) {
            continue;
        }

        const std::string name = SafeString(decl->GetName());
        const std::string category = SafeString(decl->GetCategory());
        const std::string qualified = category.empty() ? name : category + "/" + name;

        if (name == request.PrototypeName || qualified == request.PrototypeName) {
            found = decl->GetGuid();
            ++matches;
        }
    }

    if (matches == 0) {
        error = fmt::format("Building Block prototype '{}' was not found.", request.PrototypeName);
        return false;
    }

    if (matches > 1) {
        error = fmt::format("Building Block prototype '{}' is ambiguous; use Category/Name or CKGUID.", request.PrototypeName);
        return false;
    }

    guid = found;
    return true;
}

int ScriptBehaviorBridge::ExecuteRuntimeBehavior(CKBehavior *behavior, const CKBehaviorContext &ctx) {
    if (!behavior || !m_Manager || !m_Manager->GetCKContext()) {
        return CKBR_BEHAVIORERROR;
    }

    CKContext *context = m_Manager->GetCKContext();
    CKBehaviorContext savedContext = context->m_BehaviorContext;
    CKBehaviorManager *behaviorManager = context->GetBehaviorManager();
    CKBehavior *savedCurrent = behaviorManager ? behaviorManager->m_CurrentBehavior : nullptr;

    context->m_BehaviorContext = ctx;
    const int rc = behavior->Execute(ctx.DeltaTime);

    context->m_BehaviorContext = savedContext;
    if (behaviorManager) {
        behaviorManager->m_CurrentBehavior = savedCurrent;
    }

    return rc;
}

ScriptBridgeExecutionState ScriptBehaviorBridge::ExecuteOnce(CKBehavior *behavior,
                                                       const CKBehaviorContext &ctx,
                                                       int inputIndex,
                                                       std::unordered_map<int, CK_ID> *inputSources,
                                                       bool pulseDefaultInput) {
    ScriptBridgeExecutionState state;
    std::string error;
    const bool shouldPulse = inputIndex >= 0 || pulseDefaultInput;
    const int effectiveInput = inputIndex >= 0 ? inputIndex : 0;
    if (shouldPulse && !PulseInputIndex(behavior, effectiveInput, error)) {
        state.Ok = false;
        state.ReturnCode = CKBR_PARAMETERERROR;
        state.Error = error;
        return state;
    }

    const int rc = ExecuteRuntimeBehavior(behavior, ctx);
    state = CaptureExecutionState(behavior, rc);
    ClearInputs(behavior);
    ClearOutputs(behavior);

    (void) inputSources;
    return state;
}

void ScriptBehaviorBridge::QueueDestroy(CKBehavior *behavior, bool sendCallbacks, bool deleteCallbackAlreadySent) {
    if (!behavior || !m_Manager || !m_Manager->GetCKContext()) {
        return;
    }

    if (sendCallbacks && !deleteCallbackAlreadySent) {
        if (behavior->GetOwner()) {
            behavior->CallCallbackFunction(CKM_BEHAVIORDETACH);
        }
        behavior->CallCallbackFunction(CKM_BEHAVIORDELETE);
    }

    behavior->SetOwner(nullptr, FALSE);
    behavior->Activate(FALSE, TRUE);
    m_PendingDestroy.push_back(PendingDestroy{behavior->GetID(), 2});
}

void ScriptBehaviorBridge::DestroyQueuedReady() {
    if (!m_Manager || !m_Manager->GetCKContext()) {
        m_PendingDestroy.clear();
        return;
    }

    CKContext *context = m_Manager->GetCKContext();
    auto it = m_PendingDestroy.begin();
    while (it != m_PendingDestroy.end()) {
        if (it->FramesToWait > 0) {
            ++it;
            continue;
        }

        if (CKObject *obj = GetCKObjectById(context, it->BehaviorId)) {
            if (!obj->IsToBeDeleted()) {
                context->DestroyObject(obj);
            }
        }
        it = m_PendingDestroy.erase(it);
    }
}

void ScriptBehaviorBridge::ForceDestroyQueued() {
    for (auto &entry : m_PendingDestroy) {
        entry.FramesToWait = 0;
    }
    DestroyQueuedReady();
}

ScriptBehaviorBridge::TaskRecord *ScriptBehaviorBridge::FindTask(CK_ID taskId, int generation) {
    auto it = m_Tasks.find(taskId);
    if (it == m_Tasks.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}

const ScriptBehaviorBridge::TaskRecord *ScriptBehaviorBridge::FindTask(CK_ID taskId, int generation) const {
    auto it = m_Tasks.find(taskId);
    if (it == m_Tasks.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}

ScriptBehaviorBridge::ResultRecord *ScriptBehaviorBridge::FindResult(CK_ID resultId, int generation) {
    auto it = m_Results.find(resultId);
    if (it == m_Results.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}

const ScriptBehaviorBridge::ResultRecord *ScriptBehaviorBridge::FindResult(CK_ID resultId, int generation) const {
    auto it = m_Results.find(resultId);
    if (it == m_Results.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
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
