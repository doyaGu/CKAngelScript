#include "ScriptBridgeHandles.h"

#include <fmt/format.h>

namespace ScriptBridgeParameterIOInternal {

CKGUID ResolveOperationInputGuid(CKContext *context,
                                 const ScriptBridgeOperationInput &input,
                                 std::string &error) {
    if (input.SourceId) {
        CKParameter *source = ResolveStampedParameterSource(context, input.SourceId, input.SourceStamp, error);
        if (!source) {
            if (error.empty()) {
                error = "Operation input source parameter is not valid.";
            }
            return CKGUID();
        }
        return source->GetGUID();
    }

    if (input.Kind == ScriptBridgeInputBindingKind::Value) {
        return ResolveBridgeValueType(context, input.Value, ScriptParameterGuidForValue(input.Value));
    }

    return CKPGUID_NONE;
}

CKParameterLocal *CreateOperationLiteralSource(CKBehavior *behavior,
                                               int operationIndex,
                                               int inputSlot,
                                               CKGUID guid,
                                               const ScriptParamValue &value,
                                               std::vector<CK_ID> &createdLocalIds,
                                               std::string &error) {
    if (!behavior) {
        error = "Behavior is null.";
        return nullptr;
    }

    const std::string name = fmt::format("__CKAS_Op{}_In{}", operationIndex, inputSlot);
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(name.c_str()), guid);
    if (!local) {
        error = fmt::format("Failed to create literal source for parameter operation input {}.", inputSlot);
        return nullptr;
    }

    createdLocalIds.push_back(local->GetID());
    CKERROR err = SetBridgeParamValue(local, value, error);
    if (err != CK_OK) {
        error = fmt::format("Failed to set literal source for parameter operation input {} (expected {}, got {}, CKERROR {}).",
            inputSlot,
            ParameterTypeLabel(behavior->GetCKContext(), local),
            error.empty() ? "value" : error,
            err);
        return nullptr;
    }

    return local;
}

bool BindOperationInput(CKBehavior *behavior,
                        CKParameterOperation *operation,
                        int operationIndex,
                        int inputSlot,
                        const ScriptBridgeOperationInput &request,
                        std::vector<CK_ID> &createdLocalIds,
                        std::string &error) {
    CKParameterIn *input = inputSlot == 1 ? operation->GetInParameter1() : operation->GetInParameter2();
    if (!input || input->GetGUID() == CKPGUID_NONE) {
        return true;
    }

    CKParameter *source = nullptr;
    if (request.SourceId) {
        source = ResolveStampedParameterSource(behavior ? behavior->GetCKContext() : nullptr,
                                               request.SourceId,
                                               request.SourceStamp,
                                               error);
        if (!source) {
            if (error.empty()) {
                error = fmt::format("Parameter operation input {} source is not valid.", inputSlot);
            } else {
                error = fmt::format("Parameter operation input {} source is not valid: {}", inputSlot, error);
            }
            return false;
        }
    } else if (request.Kind == ScriptBridgeInputBindingKind::Value) {
        source = CreateOperationLiteralSource(behavior, operationIndex, inputSlot, input->GetGUID(), request.Value, createdLocalIds, error);
        if (!source) {
            return false;
        }
    } else {
        error = fmt::format("Parameter operation input {} is required but was not provided.", inputSlot);
        return false;
    }

    CKERROR err = input->SetDirectSource(source);
    if (err != CK_OK) {
        error = fmt::format("Failed to bind parameter operation input {} (expected {}, got {}, CKERROR {}).",
            inputSlot,
            ParameterTypeLabel(behavior ? behavior->GetCKContext() : nullptr, input),
            ParameterTypeLabel(behavior ? behavior->GetCKContext() : nullptr, source),
            err);
        return false;
    }

    return true;
}

void DestroyCreatedOperationPieces(CKContext *context,
                                   CKBehavior *behavior,
                                   CKParameterOperation *operation,
                                   const std::vector<CK_ID> &createdLocalIds) {
    if (!context) {
        return;
    }

    if (operation) {
        if (behavior) {
            behavior->RemoveParameterOperation(operation);
        }
        if (!operation->IsToBeDeleted()) {
            context->DestroyObject(operation);
        }
    }

    for (CK_ID localId : createdLocalIds) {
        if (CKObject *local = GetCKObjectById(context, localId)) {
            if (behavior) {
                if (CKParameterLocal *localParam = CKParameterLocal::Cast(local)) {
                    const int position = behavior->GetLocalParameterPosition(localParam);
                    if (position >= 0) {
                        behavior->RemoveLocalParameter(position);
                    }
                }
            }
            if (!local->IsToBeDeleted()) {
                context->DestroyObject(local);
            }
        }
    }
}

} // namespace ScriptBridgeParameterIOInternal

bool ValidateOperationSpec(CKContext *context,
                           CKGUID targetTypeGuid,
                           const ScriptBridgeOperationSpec &request,
                           std::string &error) {
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (!pm) {
        error = "CKParameterManager is not available.";
        return false;
    }

    const CKGUID operationGuid = ResolveOperationGuid(context, request.OperationGuid, request.OperationName, error);
    if (!CKGuidIsValid(operationGuid)) {
        return false;
    }

    CKGUID resultGuid = CKGuidIsValid(request.ResultTypeGuid)
        ? request.ResultTypeGuid
        : (!request.ResultTypeName.empty()
            ? ScriptResolveParameterGuid(context, request.ResultTypeName)
            : targetTypeGuid);
    if (!CKGuidIsValid(resultGuid) || resultGuid == CKPGUID_NONE) {
        resultGuid = targetTypeGuid;
    }

    const CKGUID in1Guid = ScriptBridgeParameterIOInternal::ResolveOperationInputGuid(context, request.In1, error);
    if (!error.empty()) {
        return false;
    }
    const CKGUID in2Guid = ScriptBridgeParameterIOInternal::ResolveOperationInputGuid(context, request.In2, error);
    if (!error.empty()) {
        return false;
    }

    const std::string opName = "__CKAS_ValidateOp";
    CKParameterOperation *operation = context->CreateCKParameterOperation(const_cast<CKSTRING>(opName.c_str()), operationGuid, resultGuid, in1Guid, in2Guid);
    if (!operation) {
        error = "Failed to create validation CKParameterOperation.";
        return false;
    }

    bool ok = true;
    if (!operation->GetOutParameter()) {
        error = "Parameter operation did not create an output parameter.";
        ok = false;
    } else if (!operation->GetOperationFunction()) {
        CKSTRING opText = pm->OperationGuidToName(operationGuid);
        error = fmt::format("No operation function registered for '{}' with result {}, input1 {}, input2 {}, target {}.",
            opText && opText[0] ? opText : GuidToString(operationGuid),
            ParameterTypeLabel(context, resultGuid),
            ParameterTypeLabel(context, in1Guid),
            ParameterTypeLabel(context, in2Guid),
            ParameterTypeLabel(context, targetTypeGuid));
        ok = false;
    }

    if (!operation->IsToBeDeleted()) {
        context->DestroyObject(operation);
    }
    return ok;
}

ParamOperationRef *ConnectOperationToInput(ScriptBehaviorBridge *bridge,
                                                  CKBehavior *behavior,
                                                  int pinIndex,
                                                  const ScriptBridgeOperationSpec &request,
                                                  std::string &error,
                                                  bool allowOwnerOnly,
                                                  std::vector<CK_ID> *operationIds) {
    if (!bridge || !behavior) {
        error = "Behavior bridge or target behavior is not valid.";
        return nullptr;
    }

    CKContext *context = behavior->GetCKContext();
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (!pm) {
        error = "CKParameterManager is not available.";
        return nullptr;
    }

    const int targetIndex = InputParameterIndexOrResolved(behavior, pinIndex, error);
    if (targetIndex < 0) {
        return nullptr;
    }

    CKParameterIn *targetParam = behavior->GetInputParameter(targetIndex);
    if (!targetParam) {
        error = "Target input parameter is not valid.";
        return nullptr;
    }

    const CKGUID operationGuid = ResolveOperationGuid(context, request.OperationGuid, request.OperationName, error);
    if (!CKGuidIsValid(operationGuid)) {
        return nullptr;
    }

    CKGUID resultGuid = CKGuidIsValid(request.ResultTypeGuid)
        ? request.ResultTypeGuid
        : (!request.ResultTypeName.empty()
            ? ScriptResolveParameterGuid(context, request.ResultTypeName)
            : targetParam->GetGUID());
    if (!CKGuidIsValid(resultGuid) || resultGuid == CKPGUID_NONE) {
        resultGuid = targetParam->GetGUID();
    }

    const CKGUID in1Guid = ScriptBridgeParameterIOInternal::ResolveOperationInputGuid(context, request.In1, error);
    if (!error.empty()) {
        return nullptr;
    }
    const CKGUID in2Guid = ScriptBridgeParameterIOInternal::ResolveOperationInputGuid(context, request.In2, error);
    if (!error.empty()) {
        return nullptr;
    }

    const std::string opName = fmt::format("__CKAS_Op_{}_{}", behavior->GetID(), behavior->GetParameterOperationCount());
    CKParameterOperation *operation = context->CreateCKParameterOperation(const_cast<CKSTRING>(opName.c_str()), operationGuid, resultGuid, in1Guid, in2Guid);
    if (!operation) {
        error = "Failed to create CKParameterOperation.";
        return nullptr;
    }

    CKERROR addErr = behavior->AddParameterOperation(operation);
    if (addErr != CK_OK && allowOwnerOnly) {
        operation->SetOwner(behavior);
    } else if (addErr != CK_OK) {
        error = fmt::format("Failed to add CKParameterOperation to behavior '{}' (CKERROR {}).",
            SafeString(behavior->GetName()),
            addErr);
        ScriptBridgeParameterIOInternal::DestroyCreatedOperationPieces(context, behavior, operation, {});
        return nullptr;
    }

    std::vector<CK_ID> createdLocalIds;
    CKParameter *previousSource = targetParam->GetDirectSource();
    bool targetConnected = false;
    auto fail = [&](const std::string &message) -> ParamOperationRef * {
        error = message;
        if (targetConnected && targetParam->GetDirectSource() == operation->GetOutParameter()) {
            CKERROR restoreErr = targetParam->SetDirectSource(previousSource);
            if (restoreErr != CK_OK) {
                error += fmt::format(" Failed to restore previous input source (CKERROR {}).", restoreErr);
            }
        }
        ScriptBridgeParameterIOInternal::DestroyCreatedOperationPieces(context, behavior, operation, createdLocalIds);
        return nullptr;
    };

    if (!operation->GetOutParameter()) {
        return fail("Parameter operation did not create an output parameter.");
    }

    if (!operation->GetOperationFunction()) {
        CKSTRING opText = pm->OperationGuidToName(operationGuid);
        return fail(fmt::format("No operation function registered for '{}' with result {}, input1 {}, input2 {}.",
            opText && opText[0] ? opText : GuidToString(operationGuid),
            ParameterTypeLabel(context, resultGuid),
            ParameterTypeLabel(context, in1Guid),
            ParameterTypeLabel(context, in2Guid)));
    }

    if (!ScriptBridgeParameterIOInternal::BindOperationInput(behavior, operation, behavior->GetParameterOperationCount(), 1, request.In1, createdLocalIds, error)) {
        return fail(error);
    }
    if (!ScriptBridgeParameterIOInternal::BindOperationInput(behavior, operation, behavior->GetParameterOperationCount(), 2, request.In2, createdLocalIds, error)) {
        return fail(error);
    }

    CKERROR opErr = operation->DoOperation();
    if (opErr != CK_OK) {
        return fail(fmt::format("Parameter operation initialization failed (CKERROR {}).", opErr));
    }

    CKERROR setErr = targetParam->SetDirectSource(operation->GetOutParameter());
    if (setErr != CK_OK) {
        return fail(fmt::format("Failed to connect operation output to input parameter #{} '{}' (CKERROR {}).",
            targetIndex,
            SafeString(targetParam->GetName()),
            setErr));
    }
    targetConnected = true;

    if (operationIds) {
        operationIds->push_back(operation->GetID());
    }
    return bridge->WrapParameterOperation(operation, targetParam, previousSource, createdLocalIds);
}
