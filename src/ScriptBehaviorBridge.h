#ifndef CK_SCRIPTBEHAVIORBRIDGE_H
#define CK_SCRIPTBEHAVIORBRIDGE_H

#include <algorithm>
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
class ParamSourceLinkRef;
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

enum class ScriptBridgeInputBindingKind : CKBYTE {
    Empty,
    Value,
    Source
};

enum class ScriptBridgeTaskFlags : CKDWORD {
    None = 0,
    Alive = 1u << 0,
    Paused = 1u << 1,
    DeleteCallbackSent = 1u << 2,
    TimedOut = 1u << 3,
};

enum class ScriptBridgePrototypeKind : CKBYTE {
    Name,
    Guid
};

enum class ScriptBridgeObjectStampFlags : CKDWORD {
    None = 0,
    ClassId = 1u << 0,
    TypeGuid = 1u << 1,
    PrototypeGuid = 1u << 2,
    OperationGuid = 1u << 3,
    OwnerId = 1u << 4,
};

inline CKDWORD ScriptBridgeObjectStampFlagMask(ScriptBridgeObjectStampFlags flag) {
    return static_cast<CKDWORD>(flag);
}

inline bool HasScriptBridgeObjectStampFlag(CKDWORD flags, ScriptBridgeObjectStampFlags flag) {
    return (flags & ScriptBridgeObjectStampFlagMask(flag)) != 0;
}

inline void SetScriptBridgeObjectStampFlag(CKDWORD &flags, ScriptBridgeObjectStampFlags flag, bool enabled) {
    if (enabled) {
        flags |= ScriptBridgeObjectStampFlagMask(flag);
    } else {
        flags &= ~ScriptBridgeObjectStampFlagMask(flag);
    }
}

struct ScriptBridgeObjectStamp {
    CKDWORD Flags = 0;
    CK_CLASSID ClassId = 0;
    CK_ID OwnerId = 0;
    CKGUID TypeGuid;
    CKGUID PrototypeGuid;
    CKGUID OperationGuid;

    bool Has(ScriptBridgeObjectStampFlags flag) const {
        return HasScriptBridgeObjectStampFlag(Flags, flag);
    }

    void Set(ScriptBridgeObjectStampFlags flag, bool enabled) {
        SetScriptBridgeObjectStampFlag(Flags, flag, enabled);
    }
};

struct ScriptBridgeStampedObjectId {
    CK_ID Id = 0;
    ScriptBridgeObjectStamp Stamp;
};

inline CKDWORD ScriptBridgeTaskFlagMask(ScriptBridgeTaskFlags flag) {
    return static_cast<CKDWORD>(flag);
}

inline bool HasScriptBridgeTaskFlag(CKDWORD flags, ScriptBridgeTaskFlags flag) {
    return (flags & ScriptBridgeTaskFlagMask(flag)) != 0;
}

inline void SetScriptBridgeTaskFlag(CKDWORD &flags, ScriptBridgeTaskFlags flag, bool enabled) {
    if (enabled) {
        flags |= ScriptBridgeTaskFlagMask(flag);
    } else {
        flags &= ~ScriptBridgeTaskFlagMask(flag);
    }
}

struct ScriptBridgeIndexedValue {
    int PinIndex = -1;
    ScriptParamValue Value;
};

inline void ScriptBridgeSetIndexedValue(std::vector<ScriptBridgeIndexedValue> &values,
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

struct ScriptBridgeInputSource {
    int PinIndex = -1;
    CK_ID SourceId = 0;
    ScriptBridgeObjectStamp SourceStamp;
};

struct ScriptBridgeInputSourceBinding {
    int PinIndex = -1;
    CK_ID SourceId = 0;
};

struct ScriptBridgeInputSourceBindings {
    std::vector<ScriptBridgeInputSourceBinding> Items;

    CK_ID Find(int pinIndex) const {
        const auto it = std::lower_bound(Items.begin(), Items.end(), pinIndex,
            [](const ScriptBridgeInputSourceBinding &entry, int index) {
                return entry.PinIndex < index;
            });
        return it != Items.end() && it->PinIndex == pinIndex ? it->SourceId : 0;
    }

    void Set(int pinIndex, CK_ID sourceId) {
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

    void Remove(int pinIndex) {
        const auto it = std::lower_bound(Items.begin(), Items.end(), pinIndex,
            [](const ScriptBridgeInputSourceBinding &entry, int index) {
                return entry.PinIndex < index;
            });
        if (it != Items.end() && it->PinIndex == pinIndex) {
            Items.erase(it);
        }
    }
};

struct ScriptBridgeOperationInput {
    ScriptBridgeInputBindingKind Kind = ScriptBridgeInputBindingKind::Empty;
    ScriptParamValue Value;
    CK_ID SourceId = 0;
    ScriptBridgeObjectStamp SourceStamp;
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
    struct IndexSet {
        CKDWORD InlineMask = 0;
        std::vector<int> Overflow;

        bool Empty() const;
        void Clear();
        bool Contains(int index) const;
        bool Insert(int index);
        void MergeFrom(const IndexSet &other);
        std::vector<int> ToVector() const;
    };

    bool Ok = true;
    int ReturnCode = 0;
    std::string Error;
    IndexSet ActiveOutputs;
    IndexSet SeenOutputs;
};

struct ScriptBridgeLayoutIoSlot {
    std::string Name;
};

struct ScriptBridgeLayoutParamSlot {
    ScriptBridgeSlotKind Kind = ScriptBridgeSlotKind::Standalone;
    int Index = -1;
    CK_ID ParameterId = 0;
    std::string Name;
    CKGUID TypeGuid;
    std::string TypeName;
    int DataSize = 0;
};

struct ScriptBridgeLayoutRecord {
    bool Prototype = false;
    CK_ID BehaviorId = 0;
    CKGUID PrototypeGuid;
    ScriptBridgeObjectStamp BehaviorStamp;
    std::string Signature;
    std::vector<ScriptBridgeLayoutIoSlot> Inputs;
    std::vector<ScriptBridgeLayoutIoSlot> Outputs;
    std::vector<ScriptBridgeLayoutParamSlot> Pins;
    std::vector<ScriptBridgeLayoutParamSlot> Pouts;
    std::vector<ScriptBridgeLayoutParamSlot> Locals;
};

struct ScriptBridgeBBInvocationSpec {
    ScriptBridgePrototypeKind PrototypeKind = ScriptBridgePrototypeKind::Name;
    CKGUID Guid;
    std::string PrototypeName;
    CK_ID ComponentId = 0;
    CK_ID OwnerId = 0;
    CK_ID TargetId = 0;
    std::vector<ScriptBridgeIndexedValue> IndexedParameters;
    std::vector<ScriptBridgeInputSource> SourceParameters;
    std::vector<ScriptBridgeOperationSpec> OperationParameters;
};

static_assert(sizeof(ScriptBridgeOperationInput) <= 160, "ScriptBridgeOperationInput must stay a thin binding record.");
static_assert(sizeof(ScriptBridgeIndexedValue) <= 160, "ScriptBridgeIndexedValue must stay compact.");
static_assert(sizeof(ScriptBridgeExecutionState) <= 160, "ScriptBridgeExecutionState must not become an output snapshot.");

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
    const ScriptBridgeLayoutRecord *GetBehaviorLayout(CK_ID behaviorId, const ScriptBridgeObjectStamp &stamp) const;
    const ScriptBridgeLayoutRecord *GetPrototypeLayout(const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request) const;
    ParamRef *WrapParameter(CKObject *parameter, ScriptBridgeSlotKind kind = ScriptBridgeSlotKind::Standalone, int index = -1);
    ParamOperationRef *WrapParameterOperation(CKParameterOperation *operation,
                                              CKParameterIn *targetInput = nullptr,
                                              CKParameter *previousSource = nullptr,
                                              const std::vector<CK_ID> &ownedLocalSourceIds = {});

    bool StepTask(CK_ID taskId, int generation, const CKBehaviorContext &ctx, int inputIndex);
    bool DestroyTask(CK_ID taskId, int generation);
    bool ResetTask(CK_ID taskId, int generation);
    bool IsTaskValid(CK_ID taskId, int generation) const;
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
    bool IsGraphWatchValid(CK_ID watchId, int generation) const;
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
        ScriptBridgeObjectStamp BehaviorStamp;
        CKDWORD Flags = 0;
        ScriptBridgeExecutionState LastState;
        ScriptBridgeInputSourceBindings InputSources;
        std::vector<CK_ID> OperationIds;

        bool HasFlag(ScriptBridgeTaskFlags flag) const { return HasScriptBridgeTaskFlag(Flags, flag); }
        void SetFlag(ScriptBridgeTaskFlags flag, bool enabled) { SetScriptBridgeTaskFlag(Flags, flag, enabled); }
    };

    struct ResultRecord {
        CK_ID ResultId = 0;
        int Generation = 0;
        CK_ID ComponentId = 0;
        CK_ID BehaviorId = 0;
        ScriptBridgeObjectStamp BehaviorStamp;
        CKDWORD Flags = 0;
        ScriptBridgeExecutionState State;
        std::vector<CK_ID> OperationIds;

        bool HasFlag(ScriptBridgeTaskFlags flag) const { return HasScriptBridgeTaskFlag(Flags, flag); }
        void SetFlag(ScriptBridgeTaskFlags flag, bool enabled) { SetScriptBridgeTaskFlag(Flags, flag, enabled); }
    };

    struct ScriptBridgeGraphWatch {
        CK_ID WatchId = 0;
        int Generation = 0;
        CK_ID ComponentId = 0;
        CK_ID BehaviorId = 0;
        ScriptBridgeObjectStamp BehaviorStamp;
        CKDWORD Flags = 0;
        float Elapsed = 0.0f;
        float TimeoutSeconds = 0.0f;
        std::string Error;
        ScriptBridgeExecutionState LastState;
        ScriptBridgeExecutionState::IndexSet SeenOutputs;

        bool HasFlag(ScriptBridgeTaskFlags flag) const { return HasScriptBridgeTaskFlag(Flags, flag); }
        void SetFlag(ScriptBridgeTaskFlags flag, bool enabled) { SetScriptBridgeTaskFlag(Flags, flag, enabled); }
    };

    CKBehavior *CreateRuntimeBehavior(const ScriptBridgeBBInvocationSpec &request,
                                      const CKBehaviorContext &ctx,
                                      ScriptBridgeExecutionState &state,
                                      ScriptBridgeInputSourceBindings *inputSources,
                                      std::vector<CK_ID> *operationIds);
    bool ApplyInputGraphRequests(CKBehavior *behavior,
                                 const ScriptBridgeBBInvocationSpec &request,
                                 std::string &error,
                                 ScriptBridgeInputSourceBindings *inputSources,
                                 std::vector<CK_ID> *operationIds);
    bool ResolvePrototype(const ScriptBridgeBBInvocationSpec &request, CKGUID &guid, std::string &error) const;
    int ExecuteRuntimeBehavior(CKBehavior *behavior, const CKBehaviorContext &ctx);
    ScriptBridgeExecutionState ExecuteOnce(CKBehavior *behavior,
                                     const CKBehaviorContext &ctx,
                                     int inputIndex,
                                     ScriptBridgeInputSourceBindings *inputSources,
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
    ScriptBridgeLayoutRecord BuildBehaviorLayout(CKBehavior *behavior) const;
    ScriptBridgeLayoutRecord BuildPrototypeLayout(CKBehaviorPrototype *prototype) const;
    std::string LayoutSignature(const ScriptBridgeLayoutRecord &layout) const;

    ScriptManager *m_Manager = nullptr;
    CK_ID m_NextRuntimeId = 1;
    CK_ID m_NextResultId = 1;
    CK_ID m_NextTaskId = 1;
    CK_ID m_NextGraphWatchId = 1;
    int m_NextGeneration = 1;
    std::unordered_map<CK_ID, ResultRecord> m_Results;
    std::unordered_map<CK_ID, TaskRecord> m_Tasks;
    std::unordered_map<CK_ID, ScriptBridgeGraphWatch> m_GraphWatches;
    mutable std::unordered_map<CK_ID, ScriptBridgeLayoutRecord> m_BehaviorLayouts;
    mutable std::unordered_map<std::string, ScriptBridgeLayoutRecord> m_PrototypeLayouts;
    std::vector<PendingDestroy> m_PendingDestroy;
};

void RegisterScriptBehaviorBridge(asIScriptEngine *engine);
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);

#endif // CK_SCRIPTBEHAVIORBRIDGE_H
