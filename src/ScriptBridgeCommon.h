#ifndef CK_SCRIPTBRIDGECOMMON_H
#define CK_SCRIPTBRIDGECOMMON_H

#include "ScriptBehaviorBridge.h"

#include <cstdint>
#include <string>
#include <vector>

#include "CKAll.h"
#include "ScriptManager.h"

#undef GetObject

std::string SafeString(CKSTRING value);
CKObject *GetCKObjectById(CKContext *context, CK_ID id);
std::uintptr_t ScriptBridgeObjectAddress(CKObject *object);
ScriptBridgeObjectStamp CaptureBridgeObjectStamp(CKObject *object);
bool BridgeObjectStampMatches(CKObject *object, const ScriptBridgeObjectStamp &stamp);
CKObject *GetStampedCKObjectById(CKContext *context, CK_ID id, const ScriptBridgeObjectStamp &stamp);
CKObjectDeclaration *ResolvePrototypeDeclaration(CKBehaviorPrototype *prototype, bool requireManagerMetadata = false);
CKERROR CallBridgeBehaviorCallback(CKBehavior *behavior,
                                   CKDWORD message,
                                   const CKBehaviorContext *sourceContext = nullptr);

std::string IndexedName(const char *prefix, int index);
std::string GuidToString(CKGUID guid);
bool NameEquals(CKSTRING actual, const std::string &expected);
std::string TrimString(const std::string &value);
void SetScriptException(const std::string &message);
CKBehavior *FindBehaviorByNameInContext(CKContext *context, const std::string &name);

bool IsBehaviorErrorCode(int rc);
std::string ParameterTypeLabel(CKContext *context, CKGUID guid);
int ParameterDefaultSize(CKContext *context, CKGUID guid);
std::string ParameterTypeLabel(CKContext *context, CKParameter *parameter);
std::string ParameterTypeLabel(CKContext *context, CKParameterIn *parameter);
std::string DescribePrototypeParameter(CKContext *context, CKPARAMETER_DESC *parameter);

CKBEHAVIORIO_DESC *GetPrototypeInput(CKBehaviorPrototype *prototype, int index);
CKBEHAVIORIO_DESC *GetPrototypeOutput(CKBehaviorPrototype *prototype, int index);
CKPARAMETER_DESC *GetPrototypeInputParameter(CKBehaviorPrototype *prototype, int index);
CKPARAMETER_DESC *GetPrototypeOutputParameter(CKBehaviorPrototype *prototype, int index);
CKPARAMETER_DESC *GetPrototypeLocalParameter(CKBehaviorPrototype *prototype, int index);

int FindInputIndex(CKBehavior *behavior, const std::string &name);
int FindOutputIndex(CKBehavior *behavior, const std::string &name);
int FindInputParameterIndex(CKBehavior *behavior, const std::string &name);
std::vector<int> FindInputParameterIndices(CKBehavior *behavior, const std::string &name);
std::string DescribeInputParameterList(CKBehavior *behavior);
std::string DescribeInputParameterCandidates(CKBehavior *behavior, const std::vector<int> &indices);
bool ResolveInputParameterIndex(CKBehavior *behavior, int index, std::string &error);
int InputParameterIndexOrResolved(CKBehavior *behavior, int index, std::string &error);

CKParameter *FindReadableParameter(CKBehavior *behavior, const std::string &name);
std::string OutputName(CKBehavior *behavior, int index);
std::string OutputParamName(CKBehavior *behavior, int index);

CKGUID ResolveBridgeValueType(CKContext *context,
                              const ScriptParamValue &value,
                              CKGUID fallbackGuid = CKGUID());
CKERROR SetBridgeParamValue(CKParameter *param, const ScriptParamValue &value, std::string &error);
CKParameterLocal *EnsureInputSource(CKBehavior *behavior,
                                    int index,
                                    ScriptBridgeInputSourceBindings *inputSources);
std::string OutputSinkName(int index);
CKParameterLocal *FindLocalParameterByName(CKBehavior *behavior, const std::string &name);
CKParameterLocal *EnsureOutputSink(CKBehavior *behavior, int index);
void EnsureOutputSinks(CKBehavior *behavior);
bool SetInputParameterValueByIndex(CKBehavior *behavior,
                                   int index,
                                   const ScriptParamValue &value,
                                   std::string &error,
                                   ScriptBridgeInputSourceBindings *inputSources);
bool ApplyInputParameters(CKBehavior *behavior,
                          const std::vector<ScriptBridgeIndexedValue> &parameters,
                          std::string &error,
                          ScriptBridgeInputSourceBindings *inputSources);
bool ApplyIndexedInputParameters(CKBehavior *behavior,
                                 const std::vector<ScriptBridgeIndexedValue> &parameters,
                                 std::string &error,
                                 ScriptBridgeInputSourceBindings *inputSources);

bool PulseInput(CKBehavior *behavior, const std::string &input, std::string &error, bool reset = false);
bool PulseInputIndex(CKBehavior *behavior, int inputIndex, std::string &error, bool reset = false);
void ClearInputs(CKBehavior *behavior);
void ClearOutputs(CKBehavior *behavior);

bool ExecutionOutputActive(const ScriptBridgeExecutionState &state, int index);
bool ExecutionOutputSeen(const ScriptBridgeExecutionState &state, int index);
void MergeSeenOutputs(ScriptBridgeExecutionState &state);
ScriptBridgeExecutionState CaptureExecutionState(CKBehavior *behavior, int returnCode);
void ActivateParentChain(CKBehavior *behavior, bool reset);
bool ActivateOwnerScriptInCurrentScene(CKBehavior *behavior, bool reset);
bool RaiseExecutionState(const ScriptBridgeExecutionState &state, const CKBehaviorContext &ctx);

ScriptBridgeBBInvocationSpec MakeDefaultRequest(const CKBehaviorContext &ctx);
CK_ID ComponentIdFromContext(const CKBehaviorContext &ctx);
ScriptManager *ManagerFromContext(const CKBehaviorContext &ctx);
CKBehavior *FindBehaviorRecursive(CKBehavior *root, const std::string &name);
CKBehavior *FindBehaviorOnOwner(CKBeObject *owner, const std::string &name);

CKParameter *ParameterSourceForConnection(CKObject *parameter);
CKParameter *ResolveParameterSource(CKContext *context, CK_ID id);
CKParameter *ResolveStampedParameterSource(CKContext *context,
                                           CK_ID id,
                                           const ScriptBridgeObjectStamp &stamp,
                                           std::string &error);
CKGUID ResolveOperationGuid(CKContext *context, CKGUID guid, const std::string &name, std::string &error);

#endif // CK_SCRIPTBRIDGECOMMON_H
