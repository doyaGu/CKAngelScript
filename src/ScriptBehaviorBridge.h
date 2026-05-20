#ifndef CK_SCRIPTBEHAVIORBRIDGE_H
#define CK_SCRIPTBEHAVIORBRIDGE_H

#include <string>
#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"
#include "VxMath.h"
#include "ScriptParameterConversion.h"

class CKBehavior;
class CKBehaviorPrototype;
class CKParameter;
class CKParameterOperation;
struct CKBehaviorContext;
class CKObject;
class ScriptManager;

class BehaviorRef;
class BehaviorBridge;
class BehaviorLayout;
class BBPrototype;
class BBResult;
class BBTask;
class GraphTask;
class ParamInfo;
class ParamRef;
class ParamValue;
class ParamOp;
class ParamOperationRef;
class NativeBuffer;

enum class ScriptBridgeSlotKind {
    Input,
    Output,
    Pin,
    Pout,
    Local,
    OperationIn,
    OperationOut,
    Standalone
};

struct ScriptBridgeParamValue {
    enum class Mode {
        Empty,
        Value,
        Text,
        Raw
    };

    Mode ModeKind = Mode::Empty;
    ScriptBridgeValue Value;
    std::string TextValue;
    CKGUID TypeGuid;
    std::string TypeName;
    std::vector<char> RawData;
};

struct ScriptBridgeInputSource {
    int PinIndex = -1;
    CK_ID SourceId = 0;
};

struct ScriptBridgeOperationInput {
    bool HasValue = false;
    ScriptBridgeParamValue Value;
    CK_ID SourceId = 0;
    CKGUID TypeGuid;
    std::string TypeName;
};

struct ScriptBridgeOperationSpec {
    int TargetPinIndex = -1;
    CKGUID OperationGuid;
    std::string OperationName;
    CKGUID ResultTypeGuid;
    std::string ResultTypeName;
    ScriptBridgeOperationInput In1;
    ScriptBridgeOperationInput In2;
};

struct ScriptBridgeExecutionState {
    bool Ok = true;
    int ReturnCode = 0;
    std::string Error;
    std::vector<int> ActiveOutputs;
    std::vector<int> SeenOutputs;
};

struct ScriptBridgeBBInvocationSpec {
    bool HasGuid = false;
    CKGUID Guid;
    std::string PrototypeName;
    CK_ID ComponentId = 0;
    CK_ID OwnerId = 0;
    CK_ID TargetId = 0;
    std::unordered_map<int, ScriptBridgeParamValue> IndexedParameters;
    std::vector<ScriptBridgeInputSource> SourceParameters;
    std::vector<ScriptBridgeOperationSpec> OperationParameters;
};

class ScriptBehaviorBridge {
public:
    explicit ScriptBehaviorBridge(ScriptManager *manager);
    ~ScriptBehaviorBridge();

    CKERROR PreProcess();
    CKERROR PostProcess();

    void Clear();
    void DestroyComponentTasks(CK_ID componentId);
    void ResetComponentTasks(CK_ID componentId);
    void PauseComponentTasks(CK_ID componentId, bool paused);

    BehaviorRef *WrapBehavior(CKBehavior *behavior, CK_ID componentId = 0);
    void ReleaseBehaviorRef(BehaviorRef *ref);
    BehaviorBridge *CreateBehaviorBridge(const CKBehaviorContext &ctx);
    BBPrototype *CreatePrototype(const CKBehaviorContext &ctx, const std::string &name);
    BBPrototype *CreatePrototype(const CKBehaviorContext &ctx, CKGUID guid);
    void ReleasePrototype(BBPrototype *prototype);
    BBResult *RunCall(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex);
    BBTask *StartTask(const ScriptBridgeBBInvocationSpec &request, const CKBehaviorContext &ctx, int inputIndex);
    CKBehaviorPrototype *ResolvePrototypeObject(const ScriptBridgeBBInvocationSpec &request, std::string &error) const;
    ParamRef *WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind = ScriptBridgeSlotKind::Standalone, int index = -1);
    ParamOperationRef *WrapParameterOperation(CKParameterOperation *operation);

    bool StepTask(CK_ID taskId, int generation, const CKBehaviorContext &ctx, int inputIndex);
    bool DestroyTask(CK_ID taskId, int generation);
    bool ResetTask(CK_ID taskId, int generation);
    bool IsTaskAlive(CK_ID taskId, int generation) const;
    bool IsTaskPaused(CK_ID taskId, int generation) const;
    ScriptBridgeExecutionState GetTaskState(CK_ID taskId, int generation) const;
    CKBehavior *GetTaskBehavior(CK_ID taskId, int generation) const;

    ScriptBridgeExecutionState GetResultState(CK_ID resultId, int generation) const;
    CKBehavior *GetResultBehavior(CK_ID resultId, int generation) const;
    bool DestroyResult(CK_ID resultId, int generation);

    GraphTask *CreateGraphWatch(CKBehavior *behavior, CK_ID componentId, float timeoutSeconds);
    bool StepGraphWatch(CK_ID watchId, int generation, const CKBehaviorContext &ctx);
    bool CancelGraphWatch(CK_ID watchId, int generation);
    bool ResetGraphWatch(CK_ID watchId, int generation);
    bool SetGraphWatchTimeout(CK_ID watchId, int generation, float timeoutSeconds);
    bool IsGraphWatchAlive(CK_ID watchId, int generation) const;
    bool IsGraphWatchPaused(CK_ID watchId, int generation) const;
    bool IsGraphWatchTimedOut(CK_ID watchId, int generation) const;
    float GetGraphWatchElapsed(CK_ID watchId, int generation) const;
    std::string GetGraphWatchError(CK_ID watchId, int generation) const;
    ScriptBridgeExecutionState GetGraphWatchState(CK_ID watchId, int generation) const;
    bool IsGraphWatchDone(CK_ID watchId, int generation, int outputIndex) const;
    CKBehavior *GetGraphWatchBehavior(CK_ID watchId, int generation) const;

    ScriptManager *GetManager() const { return m_Manager; }

private:
    struct PendingDestroy {
        CK_ID BehaviorId = 0;
        int FramesToWait = 2;
    };

    struct TaskRecord {
        CK_ID TaskId = 0;
        int Generation = 0;
        CK_ID ComponentId = 0;
        CK_ID BehaviorId = 0;
        bool Alive = false;
        bool Paused = false;
        bool DeleteCallbackSent = false;
        ScriptBridgeExecutionState LastState;
        std::unordered_map<int, CK_ID> InputSources;
        std::vector<CK_ID> OperationIds;
    };

    struct ResultRecord {
        CK_ID ResultId = 0;
        int Generation = 0;
        CK_ID ComponentId = 0;
        CK_ID BehaviorId = 0;
        bool DeleteCallbackSent = false;
        ScriptBridgeExecutionState State;
        std::vector<CK_ID> OperationIds;
    };

    struct ScriptBridgeGraphWatch {
        CK_ID WatchId = 0;
        int Generation = 0;
        CK_ID ComponentId = 0;
        CK_ID BehaviorId = 0;
        bool Alive = false;
        bool Paused = false;
        bool TimedOut = false;
        float Elapsed = 0.0f;
        float TimeoutSeconds = 0.0f;
        std::string Error;
        ScriptBridgeExecutionState LastState;
        std::vector<int> SeenOutputs;
    };

    CKBehavior *CreateRuntimeBehavior(const ScriptBridgeBBInvocationSpec &request,
                                      const CKBehaviorContext &ctx,
                                      ScriptBridgeExecutionState &state,
                                      std::unordered_map<int, CK_ID> *inputSources,
                                      std::vector<CK_ID> *operationIds);
    bool ApplyInputGraphRequests(CKBehavior *behavior,
                                 const ScriptBridgeBBInvocationSpec &request,
                                 std::string &error,
                                 std::unordered_map<int, CK_ID> *inputSources,
                                 std::vector<CK_ID> *operationIds);
    bool ResolvePrototype(const ScriptBridgeBBInvocationSpec &request, CKGUID &guid, std::string &error) const;
    int ExecuteRuntimeBehavior(CKBehavior *behavior, const CKBehaviorContext &ctx);
    ScriptBridgeExecutionState ExecuteOnce(CKBehavior *behavior,
                                     const CKBehaviorContext &ctx,
                                     int inputIndex,
                                     std::unordered_map<int, CK_ID> *inputSources,
                                     bool pulseDefaultInput);
    void QueueDestroy(CKBehavior *behavior, bool sendCallbacks, bool deleteCallbackAlreadySent = false);
    void DestroyQueuedReady();
    void ForceDestroyQueued();
    TaskRecord *FindTask(CK_ID taskId, int generation);
    const TaskRecord *FindTask(CK_ID taskId, int generation) const;
    ResultRecord *FindResult(CK_ID resultId, int generation);
    const ResultRecord *FindResult(CK_ID resultId, int generation) const;
    ScriptBridgeGraphWatch *FindGraphWatch(CK_ID watchId, int generation);
    const ScriptBridgeGraphWatch *FindGraphWatch(CK_ID watchId, int generation) const;

    ScriptManager *m_Manager = nullptr;
    CK_ID m_NextRuntimeId = 1;
    CK_ID m_NextResultId = 1;
    CK_ID m_NextTaskId = 1;
    CK_ID m_NextGraphWatchId = 1;
    int m_NextGeneration = 1;
    std::unordered_map<CK_ID, ResultRecord> m_Results;
    std::unordered_map<CK_ID, TaskRecord> m_Tasks;
    std::unordered_map<CK_ID, ScriptBridgeGraphWatch> m_GraphWatches;
    std::vector<PendingDestroy> m_PendingDestroy;
};

void RegisterScriptBehaviorBridge(asIScriptEngine *engine);
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);

#endif // CK_SCRIPTBEHAVIORBRIDGE_H
