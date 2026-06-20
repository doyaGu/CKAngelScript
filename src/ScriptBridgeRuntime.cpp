#include "ScriptBridgeHandles.h"

#include <algorithm>

#include <fmt/format.h>

namespace ScriptBehaviorBridgeInternal {

bool SetLocalParameterValueByIndex(CKBehavior *behavior,
                                   int index,
                                   const ScriptParamValue &value,
                                   std::string &error) {
    if (!behavior) {
        error = "Behavior is not valid.";
        return false;
    }
    if (index < 0 || index >= behavior->GetLocalParameterCount()) {
        error = fmt::format("Local parameter #{} is out of range for Building Block '{}' (local count: {}).",
            index,
            SafeString(behavior->GetPrototypeName()),
            behavior->GetLocalParameterCount());
        return false;
    }
    CKParameterLocal *local = behavior->GetLocalParameter(index);
    if (!local) {
        error = fmt::format("Local parameter #{} is not available.", index);
        return false;
    }
    CKERROR err = SetBridgeParamValue(local, value, error);
    if (err != CK_OK) {
        error = fmt::format("Failed to set local parameter #{} '{}' (expected {}, CKERROR {}): {}",
            index,
            SafeString(local->GetName()),
            ParameterTypeLabel(behavior->GetCKContext(), local),
            err,
            error.empty() ? "conversion failed" : error);
        return false;
    }
    return true;
}

bool ApplyIndexedLocalParameters(CKBehavior *behavior,
                                 const std::vector<ScriptBridgeIndexedValue> &parameters,
                                 std::string &error) {
    for (const ScriptBridgeIndexedValue &entry : parameters) {
        if (!SetLocalParameterValueByIndex(behavior, entry.PinIndex, entry.Value, error)) {
            return false;
        }
    }
    return true;
}

bool NotifyIndexedSettingsEdited(CKBehavior *behavior,
                                  const std::vector<ScriptBridgeIndexedValue> &settings,
                                  const CKBehaviorContext &ctx,
                                  std::string &error) {
    if (settings.empty()) {
        return true;
    }

    const CKERROR err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED, &ctx);
    if (err != CK_OK) {
        error = fmt::format("Building Block SETTINGSEDITED callback failed (CKERROR {}).", err);
        return false;
    }
    return true;
}

CKBeObject *ResolveRequestBeObject(CKContext *context,
                                   CK_ID id,
                                   const ScriptBridgeObjectStamp &stamp,
                                   const char *role,
                                   std::string &error) {
    if (!id) {
        return nullptr;
    }

    CKObject *object = stamp.Flags != 0 ? GetStampedCKObjectById(context, id, stamp) : GetCKObjectById(context, id);
    if (!object) {
        error = fmt::format("{} object {} is no longer valid.", role ? role : "Building Block request", id);
        return nullptr;
    }

    CKBeObject *beObject = CKBeObject::Cast(object);
    if (!beObject) {
        error = fmt::format("{} object {} is no longer a CKBeObject.", role ? role : "Building Block request", id);
    }
    return beObject;
}

bool ConfigureBehaviorOwnerAndTarget(CKContext *context,
                                     CKBehavior *behavior,
                                     const ScriptBridgeBBInvocationSpec &request,
                                     std::string &error) {
    CKBeObject *owner = ResolveRequestBeObject(context, request.OwnerId, request.OwnerStamp, "Owner", error);
    if (request.OwnerId && !owner) {
        return false;
    }
    CKBeObject *target = ResolveRequestBeObject(context, request.TargetId, request.TargetStamp, "Target", error);
    if (request.TargetId && !target) {
        return false;
    }
    const CK_CLASSID compatibleClassId = behavior->GetCompatibleClassID();

    if (target) {
        if (!behavior->IsTargetable()) {
            error = fmt::format("Building Block '{}' is not targetable.", SafeString(behavior->GetPrototypeName()));
            return false;
        }

        if (!CKIsChildClassOf(target, compatibleClassId)) {
            error = fmt::format("Target '{}' is not compatible with Building Block '{}'.",
                                SafeString(target->GetName()),
                                SafeString(behavior->GetPrototypeName()));
            return false;
        }

        CKERROR err = behavior->UseTarget(TRUE);
        if (err != CK_OK) {
            error = fmt::format("Failed to enable target parameter for Building Block '{}' (CKERROR {}).",
                                SafeString(behavior->GetPrototypeName()),
                                err);
            return false;
        }

        CKParameterIn *targetParam = behavior->GetTargetParameter();
        if (!targetParam) {
            error = "Target parameter was not created.";
            return false;
        }

        CKParameterLocal *targetSource = behavior->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_Target"), targetParam->GetGUID());
        if (!targetSource) {
            error = "Failed to create target source parameter.";
            return false;
        }

        CK_ID targetId = target->GetID();
        if (targetSource->SetValue(&targetId, sizeof(targetId)) != CK_OK || targetParam->SetDirectSource(targetSource) != CK_OK) {
            error = "Failed to set target parameter.";
            return false;
        }
    } else {
        if (owner && !CKIsChildClassOf(owner, compatibleClassId)) {
            error = fmt::format("Owner '{}' is not compatible with Building Block '{}'.",
                                SafeString(owner->GetName()),
                                SafeString(behavior->GetPrototypeName()));
            return false;
        }

        if (!owner && compatibleClassId != CKCID_OBJECT && compatibleClassId != CKCID_BEOBJECT) {
            error = fmt::format("Building Block '{}' requires an owner or target.", SafeString(behavior->GetPrototypeName()));
            return false;
        }
    }

    CKERROR err = behavior->SetOwner(owner, FALSE);
    if (err != CK_OK) {
        error = fmt::format("Failed to set Building Block owner (CKERROR {}).", err);
        return false;
    }

    return true;
}

} // namespace ScriptBehaviorBridgeInternal

BBResult *ScriptBehaviorBridge::RunCall(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex) {
    ScriptBridgeExecutionState state;
    ScriptBridgeInputSourceBindings inputSources;
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
    record.BehaviorStamp = CaptureBridgeObjectStamp(behavior);
    record.State = state;
    record.OperationIds = std::move(operationIds);

    auto [it, inserted] = m_Results.emplace(record.ResultId, std::move(record));
    (void) inserted;
    return new BBResult(this, it->second.ResultId, it->second.Generation);
}

BBTask *ScriptBehaviorBridge::StartTask(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex) {
    ScriptBridgeExecutionState state;
    ScriptBridgeInputSourceBindings sources;
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
    record.BehaviorStamp = CaptureBridgeObjectStamp(behavior);
    record.SetFlag(ScriptBridgeTaskFlags::Alive, true);
    record.InputSources = std::move(sources);
    record.OperationIds = std::move(operations);

    auto [it, inserted] = m_Tasks.emplace(record.TaskId, std::move(record));
    (void) inserted;

    it->second.LastState = ExecuteOnce(behavior, ctx, inputIndex, &it->second.InputSources, true);

    return new BBTask(this, it->second.TaskId, it->second.Generation);
}

bool ScriptBehaviorBridge::SetTaskSetting(CK_ID taskId, int generation, int settingIndex, const ScriptParamValue &value, std::string &error) {
    CKBehavior *behavior = GetTaskBehavior(taskId, generation);
    if (!behavior) {
        error = "BBTask behavior is not available.";
        return false;
    }
    return SetBehaviorSetting(behavior, settingIndex, value, error);
}

CK_ID ScriptBehaviorBridge::CreateInstance(const ScriptBridgeBBInvocationSpec &request,
                                           const CKBehaviorContext &ctx,
                                           int &generation,
                                           std::string &error) {
    ScriptBridgeExecutionState state;
    ScriptBridgeInputSourceBindings sources;
    std::vector<CK_ID> operations;
    CKBehavior *behavior = CreateRuntimeBehavior(request, ctx, state, &sources, &operations);
    if (!behavior) {
        error = state.Error.empty() ? "Failed to create BBInstance." : state.Error;
        generation = 0;
        return 0;
    }

    InstanceRecord record;
    record.InstanceId = m_NextInstanceId++;
    record.Generation = m_NextGeneration++;
    record.ComponentId = request.ComponentId;
    record.BehaviorId = behavior->GetID();
    record.BehaviorStamp = CaptureBridgeObjectStamp(behavior);
    record.SetFlag(ScriptBridgeTaskFlags::Alive, true);
    record.LastState = CaptureExecutionState(behavior, CKBR_OK);
    record.InputSources = std::move(sources);
    record.OperationIds = std::move(operations);

    auto [it, inserted] = m_Instances.emplace(record.InstanceId, std::move(record));
    (void) inserted;
    generation = it->second.Generation;
    error.clear();
    return it->second.InstanceId;
}

CKBehavior *ScriptBehaviorBridge::CreatePersistentBehavior(const ScriptBridgeBBInvocationSpec &request,
                                                           const CKBehaviorContext &ctx,
                                                           const std::string &name,
                                                           std::string &error,
                                                           std::vector<CK_ID> *operationIds) {
    if (!m_Manager || !m_Manager->GetCKContext()) {
        error = "Script manager or CKContext is not available.";
        return nullptr;
    }

    CKContext *context = m_Manager->GetCKContext();
    CKGUID guid;
    if (!ResolvePrototype(request, guid, error)) {
        return nullptr;
    }

    CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(guid);
    CKObjectDeclaration *declaration = ResolvePrototypeDeclaration(prototype, true);
    if (declaration) {
        for (int i = 0; i < declaration->GetManagerNeededCount(); ++i) {
            const CKGUID managerGuid = declaration->GetManagerNeeded(i);
            if (!context->GetManagerByGuid(managerGuid)) {
                error = fmt::format("Building Block '{}' requires manager {} which is not available.",
                                    SafeString(declaration->GetName()),
                                    GuidToString(managerGuid));
                return nullptr;
            }
        }
    }

    const std::string behaviorName = name.empty()
        ? fmt::format("__CKAS_GraphEdit_{}", m_NextRuntimeId++)
        : name;
    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR,
                                                                  const_cast<CKSTRING>(behaviorName.c_str()),
                                                                  CK_OBJECTCREATION_NONAMECHECK));
    if (!behavior) {
        error = "Failed to create persistent Building Block behavior.";
        return nullptr;
    }

    bool createCallbackSent = false;
    auto fail = [&](const std::string &message) -> CKBehavior * {
        error = message;
        if (createCallbackSent) {
            if (behavior->GetOwner()) {
                CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDETACH, &ctx);
            }
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE, &ctx);
        }
        context->DestroyObject(behavior);
        return nullptr;
    };

    behavior->UseFunction();
    CKERROR err = behavior->InitFromGuid(guid);
    if (err != CK_OK) {
        return fail(fmt::format("Failed to initialize Building Block from GUID ({}, {}) with CKERROR {}.",
                                guid.d[0],
                                guid.d[1],
                                err));
    }

    if (!ScriptBehaviorBridgeInternal::ApplyIndexedLocalParameters(behavior, request.IndexedSettings, error)) {
        return fail(error);
    }

    if (!ScriptBehaviorBridgeInternal::ConfigureBehaviorOwnerAndTarget(context, behavior, request, error)) {
        return fail(error);
    }

    err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &ctx);
    if (err != CK_OK) {
        return fail(fmt::format("Building Block CREATE callback failed (CKERROR {}).", err));
    }
    createCallbackSent = true;

    CKBeObject *owner = ScriptBehaviorBridgeInternal::ResolveRequestBeObject(context,
                                                                             request.OwnerId,
                                                                             request.OwnerStamp,
                                                                             "Owner",
                                                                             error);
    if (request.OwnerId && !owner) {
        return fail(error);
    }
    if (owner) {
        err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORATTACH, &ctx);
        if (err != CK_OK) {
            return fail(fmt::format("Building Block ATTACH callback failed (CKERROR {}).", err));
        }
    }

    if (!ScriptBehaviorBridgeInternal::NotifyIndexedSettingsEdited(behavior, request.IndexedSettings, ctx, error)) {
        return fail(error);
    }

    if (!ApplyIndexedInputParameters(behavior, request.IndexedParameters, error, nullptr)) {
        return fail(error);
    }

    if (!ApplyInputGraphRequests(behavior, request, error, nullptr, operationIds)) {
        return fail(error);
    }

    error.clear();
    return behavior;
}

bool ScriptBehaviorBridge::StartInstance(CK_ID instanceId, int generation, const CKBehaviorContext &ctx, int inputIndex) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !record->HasFlag(ScriptBridgeTaskFlags::Alive)) {
        SetScriptException("BBInstance is not alive.");
        return false;
    }
    if (record->HasFlag(ScriptBridgeTaskFlags::Paused)) {
        record->LastState.Ok = false;
        record->LastState.Error = "BBInstance is paused.";
        SetScriptException(record->LastState.Error);
        return false;
    }
    CKBehavior *behavior = GetInstanceBehavior(instanceId, generation);
    if (!behavior) {
        record->SetFlag(ScriptBridgeTaskFlags::Alive, false);
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = "BBInstance behavior is no longer available.";
        SetScriptException(record->LastState.Error);
        return false;
    }
    record->LastState = ExecuteOnce(behavior, ctx, inputIndex, &record->InputSources, true);
    return record->LastState.Ok;
}

bool ScriptBehaviorBridge::StepInstance(CK_ID instanceId, int generation, const CKBehaviorContext &ctx) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !record->HasFlag(ScriptBridgeTaskFlags::Alive)) {
        SetScriptException("BBInstance is not alive.");
        return false;
    }
    if (record->HasFlag(ScriptBridgeTaskFlags::Paused)) {
        record->LastState.Ok = false;
        record->LastState.Error = "BBInstance is paused.";
        SetScriptException(record->LastState.Error);
        return false;
    }
    CKBehavior *behavior = GetInstanceBehavior(instanceId, generation);
    if (!behavior) {
        record->SetFlag(ScriptBridgeTaskFlags::Alive, false);
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = "BBInstance behavior is no longer available.";
        SetScriptException(record->LastState.Error);
        return false;
    }
    record->LastState = ExecuteOnce(behavior, ctx, -1, &record->InputSources, false);
    return record->LastState.Ok;
}

bool ScriptBehaviorBridge::StopInstance(CK_ID instanceId, int generation, const CKBehaviorContext &ctx, int inputIndex) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !record->HasFlag(ScriptBridgeTaskFlags::Alive)) {
        SetScriptException("BBInstance is not alive.");
        return false;
    }

    CKBehavior *behavior = GetInstanceBehavior(instanceId, generation);
    if (!behavior) {
        record->SetFlag(ScriptBridgeTaskFlags::Alive, false);
        record->LastState.Ok = false;
        record->LastState.ReturnCode = CKBR_BEHAVIORERROR;
        record->LastState.Error = "BBInstance behavior is no longer available.";
        SetScriptException(record->LastState.Error);
        return false;
    }

    if (inputIndex >= 0) {
        record->LastState = ExecuteOnce(behavior, ctx, inputIndex, &record->InputSources, false);
        if (!record->LastState.Ok) {
            return false;
        }
    } else {
        ClearInputs(behavior);
        ClearOutputs(behavior);
        record->LastState = CaptureExecutionState(behavior, CKBR_OK);
    }

    behavior->Activate(FALSE, FALSE);
    return true;
}

void ScriptBehaviorBridge::RemoveInstanceSourceLink(CK_ID instanceId, int generation, int pinIndex, bool restoreTarget) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record) {
        return;
    }
    const auto it = std::lower_bound(record->SourceLinks.begin(),
                                     record->SourceLinks.end(),
                                     pinIndex,
                                     [](const InstanceRecord::SourceLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it == record->SourceLinks.end() || it->PinIndex != pinIndex) {
        return;
    }
    if (it->Link) {
        if (restoreTarget) {
            it->Link->Restore();
        } else {
            it->Link->DestroyDetached();
        }
        it->Link->Release();
    }
    record->SourceLinks.erase(it);
}

ParamSourceLinkRef *ScriptBehaviorBridge::TakeInstanceSourceLink(CK_ID instanceId, int generation, int pinIndex) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record) {
        return nullptr;
    }
    const auto it = std::lower_bound(record->SourceLinks.begin(),
                                     record->SourceLinks.end(),
                                     pinIndex,
                                     [](const InstanceRecord::SourceLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it == record->SourceLinks.end() || it->PinIndex != pinIndex) {
        return nullptr;
    }
    ParamSourceLinkRef *link = it->Link;
    record->SourceLinks.erase(it);
    return link;
}

void ScriptBehaviorBridge::RemoveInstanceOperation(CK_ID instanceId, int generation, int pinIndex, bool restoreTarget) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record) {
        return;
    }
    const auto it = std::lower_bound(record->Operations.begin(),
                                     record->Operations.end(),
                                     pinIndex,
                                     [](const InstanceRecord::OperationLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it == record->Operations.end() || it->PinIndex != pinIndex) {
        return;
    }
    if (it->Operation) {
        if (restoreTarget) {
            it->Operation->Destroy();
        } else {
            it->Operation->DestroyDetached();
        }
        it->Operation->Release();
    }
    record->Operations.erase(it);
}

ParamOperationRef *ScriptBehaviorBridge::TakeInstanceOperation(CK_ID instanceId, int generation, int pinIndex) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record) {
        return nullptr;
    }
    const auto it = std::lower_bound(record->Operations.begin(),
                                     record->Operations.end(),
                                     pinIndex,
                                     [](const InstanceRecord::OperationLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it == record->Operations.end() || it->PinIndex != pinIndex) {
        return nullptr;
    }
    ParamOperationRef *operation = it->Operation;
    record->Operations.erase(it);
    return operation;
}

bool ScriptBehaviorBridge::StoreInstanceSourceLink(CK_ID instanceId, int generation, int pinIndex, ParamSourceLinkRef *link) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !link) {
        return false;
    }
    const auto it = std::lower_bound(record->SourceLinks.begin(),
                                     record->SourceLinks.end(),
                                     pinIndex,
                                     [](const InstanceRecord::SourceLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != record->SourceLinks.end() && it->PinIndex == pinIndex) {
        if (it->Link) {
            it->Link->Release();
        }
        it->Link = link;
    } else {
        record->SourceLinks.insert(it, InstanceRecord::SourceLink{pinIndex, link});
    }
    return true;
}

bool ScriptBehaviorBridge::StoreInstanceOperation(CK_ID instanceId, int generation, int pinIndex, ParamOperationRef *operation) {
    InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !operation) {
        return false;
    }
    const auto it = std::lower_bound(record->Operations.begin(),
                                     record->Operations.end(),
                                     pinIndex,
                                     [](const InstanceRecord::OperationLink &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != record->Operations.end() && it->PinIndex == pinIndex) {
        if (it->Operation) {
            it->Operation->Release();
        }
        it->Operation = operation;
    } else {
        record->Operations.insert(it, InstanceRecord::OperationLink{pinIndex, operation});
    }
    return true;
}

bool ScriptBehaviorBridge::DestroyInstance(CK_ID instanceId, int generation) {
    auto it = m_Instances.find(instanceId);
    if (it == m_Instances.end() || it->second.Generation != generation) {
        return false;
    }

    ClearInstanceGraphLinks(it->second);
    CKBehavior *behavior = GetInstanceBehavior(instanceId, generation);
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    for (CK_ID operationId : it->second.OperationIds) {
        if (context) {
            if (CKObject *op = GetCKObjectById(context, operationId)) {
                context->DestroyObject(op);
            }
        }
    }
    if (behavior) {
        QueueDestroy(behavior, true, it->second.HasFlag(ScriptBridgeTaskFlags::DeleteCallbackSent));
    }
    m_Instances.erase(it);
    return true;
}

bool ScriptBehaviorBridge::SetInstanceSetting(CK_ID instanceId,
                                              int generation,
                                              int settingIndex,
                                              const ScriptParamValue &value,
                                              std::string &error) {
    CKBehavior *behavior = GetInstanceBehavior(instanceId, generation);
    if (!behavior) {
        error = "BBInstance behavior is not available.";
        return false;
    }
    return SetBehaviorSetting(behavior, settingIndex, value, error);
}

bool ScriptBehaviorBridge::IsInstanceValid(CK_ID instanceId, int generation) const {
    return FindInstance(instanceId, generation) != nullptr;
}

bool ScriptBehaviorBridge::IsInstanceAlive(CK_ID instanceId, int generation) const {
    const InstanceRecord *record = FindInstance(instanceId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::Alive);
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetInstanceState(CK_ID instanceId, int generation) const {
    const InstanceRecord *record = FindInstance(instanceId, generation);
    if (record) {
        return record->LastState;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "BBInstance is not valid.";
    return state;
}

CKBehavior *ScriptBehaviorBridge::GetInstanceBehavior(CK_ID instanceId, int generation) const {
    const InstanceRecord *record = FindInstance(instanceId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetStampedCKObjectById(m_Manager->GetCKContext(), record->BehaviorId, record->BehaviorStamp));
}

bool ScriptBehaviorBridge::StepTask(CK_ID taskId, int generation, const CKBehaviorContext &ctx, int inputIndex) {
    TaskRecord *record = FindTask(taskId, generation);
    if (!record || !record->HasFlag(ScriptBridgeTaskFlags::Alive)) {
        SetScriptException("BBTask is not alive.");
        return false;
    }

    if (record->HasFlag(ScriptBridgeTaskFlags::Paused)) {
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
        record->LastState.ActiveOutputs.Clear();
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
        QueueDestroy(behavior, true, it->second.HasFlag(ScriptBridgeTaskFlags::DeleteCallbackSent));
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
    CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORRESET);
    record->LastState = ScriptBridgeExecutionState();
    return true;
}

bool ScriptBehaviorBridge::IsTaskAlive(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::Alive);
}

bool ScriptBehaviorBridge::IsTaskValid(CK_ID taskId, int generation) const {
    return FindTask(taskId, generation) != nullptr;
}

bool ScriptBehaviorBridge::IsTaskPaused(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    return record && record->HasFlag(ScriptBridgeTaskFlags::Paused);
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetTaskState(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    if (record) {
        return record->LastState;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "BBTask is not valid.";
    return state;
}

CKBehavior *ScriptBehaviorBridge::GetTaskBehavior(CK_ID taskId, int generation) const {
    const TaskRecord *record = FindTask(taskId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetStampedCKObjectById(m_Manager->GetCKContext(), record->BehaviorId, record->BehaviorStamp));
}

ScriptBridgeExecutionState ScriptBehaviorBridge::GetResultState(CK_ID resultId, int generation) const {
    const ResultRecord *record = FindResult(resultId, generation);
    if (record) {
        return record->State;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = "BBResult is not valid.";
    return state;
}

CKBehavior *ScriptBehaviorBridge::GetResultBehavior(CK_ID resultId, int generation) const {
    const ResultRecord *record = FindResult(resultId, generation);
    if (!record || !m_Manager || !m_Manager->GetCKContext()) {
        return nullptr;
    }
    return CKBehavior::Cast(GetStampedCKObjectById(m_Manager->GetCKContext(), record->BehaviorId, record->BehaviorStamp));
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
        QueueDestroy(behavior, true, it->second.HasFlag(ScriptBridgeTaskFlags::DeleteCallbackSent));
    }
    m_Results.erase(it);
    return true;
}
bool ScriptBehaviorBridge::ApplyInputGraphRequests(CKBehavior *behavior,
                                                   const ScriptBridgeBBInvocationSpec &request,
                                                   std::string &error,
                                                   ScriptBridgeInputSourceBindings *inputSources,
                                                   std::vector<CK_ID> *operationIds) {
    (void) inputSources;
    if (!behavior) {
        error = "Behavior is null.";
        return false;
    }

    CKContext *context = behavior->GetCKContext();
    for (const ScriptBridgeInputSource &sourceRequest : request.SourceParameters) {
        CKParameter *source = ResolveStampedParameterSource(context,
                                                            sourceRequest.SourceId,
                                                            sourceRequest.SourceStamp,
                                                            error);
        if (!source) {
            if (error.empty()) {
                error = "Input source parameter is not valid.";
            }
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
                                                        ScriptBridgeInputSourceBindings *inputSources,
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
    CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(guid);
    CKObjectDeclaration *declaration = ResolvePrototypeDeclaration(prototype, true);
    if (declaration) {
        for (int i = 0; i < declaration->GetManagerNeededCount(); ++i) {
            const CKGUID managerGuid = declaration->GetManagerNeeded(i);
            if (!context->GetManagerByGuid(managerGuid)) {
                state.Ok = false;
                state.Error = fmt::format("Building Block '{}' requires manager {} which is not available.",
                    SafeString(declaration->GetName()),
                    GuidToString(managerGuid));
                return nullptr;
            }
        }
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

    if (!ScriptBehaviorBridgeInternal::ApplyIndexedLocalParameters(behavior, request.IndexedSettings, error)) {
        return fail(error);
    }

    if (!ScriptBehaviorBridgeInternal::ConfigureBehaviorOwnerAndTarget(context, behavior, request, error)) {
        return fail(error);
    }

    err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE, &ctx);
    if (err != CK_OK) {
        return fail(fmt::format("Building Block CREATE callback failed (CKERROR {}).", err));
    }
    createCallbackSent = true;

    CKBeObject *owner = ScriptBehaviorBridgeInternal::ResolveRequestBeObject(context,
                                                                             request.OwnerId,
                                                                             request.OwnerStamp,
                                                                             "Owner",
                                                                             error);
    if (request.OwnerId && !owner) {
        return fail(error);
    }
    if (owner) {
        err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORATTACH, &ctx);
        if (err != CK_OK) {
            return fail(fmt::format("Building Block ATTACH callback failed (CKERROR {}).", err));
        }
    }

    if (!ScriptBehaviorBridgeInternal::NotifyIndexedSettingsEdited(behavior, request.IndexedSettings, ctx, state.Error)) {
        return fail(state.Error);
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
    if (request.PrototypeKind == ScriptBridgePrototypeKind::Guid) {
        if (!CKGuidIsValid(request.Guid)) {
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
#if CKVERSION == 0x13022002
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
#else
    return behavior->Execute(ctx.DeltaTime);
#endif
}

ScriptBridgeExecutionState ScriptBehaviorBridge::ExecuteOnce(CKBehavior *behavior,
                                                       const CKBehaviorContext &ctx,
                                                       int inputIndex,
                                                       ScriptBridgeInputSourceBindings *inputSources,
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
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDETACH);
        }
        CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE);
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

void ScriptBehaviorBridge::ClearInstanceGraphLinks(InstanceRecord &record) {
    for (InstanceRecord::SourceLink &link : record.SourceLinks) {
        if (link.Link) {
            link.Link->Restore();
            link.Link->Release();
            link.Link = nullptr;
        }
    }
    record.SourceLinks.clear();

    for (InstanceRecord::OperationLink &operation : record.Operations) {
        if (operation.Operation) {
            operation.Operation->Destroy();
            operation.Operation->Release();
            operation.Operation = nullptr;
        }
    }
    record.Operations.clear();
}

bool ScriptBehaviorBridge::SetBehaviorSetting(CKBehavior *behavior,
                                              int settingIndex,
                                              const ScriptParamValue &value,
                                              std::string &error) {
    if (!behavior) {
        error = "Building Block behavior is not available.";
        return false;
    }
    if (!ScriptBehaviorBridgeInternal::SetLocalParameterValueByIndex(behavior, settingIndex, value, error)) {
        return false;
    }
    CKERROR err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORSETTINGSEDITED);
    if (err != CK_OK) {
        error = fmt::format("Building Block SETTINGSEDITED callback failed for '{}' (CKERROR {}).",
            SafeString(behavior->GetPrototypeName()),
            err);
        return false;
    }
    m_BehaviorLayouts.erase(behavior->GetID());
    return true;
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

ScriptBehaviorBridge::InstanceRecord *ScriptBehaviorBridge::FindInstance(CK_ID instanceId, int generation) {
    auto it = m_Instances.find(instanceId);
    if (it == m_Instances.end() || it->second.Generation != generation) {
        return nullptr;
    }
    return &it->second;
}

const ScriptBehaviorBridge::InstanceRecord *ScriptBehaviorBridge::FindInstance(CK_ID instanceId, int generation) const {
    auto it = m_Instances.find(instanceId);
    if (it == m_Instances.end() || it->second.Generation != generation) {
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
