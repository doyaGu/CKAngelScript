#ifndef CK_SCRIPTBRIDGEHANDLES_H
#define CK_SCRIPTBRIDGEHANDLES_H

#include "ScriptBridgeCommon.h"
#include "ScriptBridgeParameterIO.h"
#include "ScriptNativeBuffer.h"
#include "ScriptParameterRegistry.h"

class CScriptArray;
class BBCallBuilder;
class BBTaskBuilder;
class BBBinding;

class ParamInfo final : public RefCounted {
public:
    ParamInfo(ScriptBridgeSlotKind kind,
              int index,
              const std::string &name,
              CKGUID typeGuid,
              const std::string &typeName,
              int dataSize);

    int GetKind() const;
    int GetIndex() const;
    std::string GetName() const;
    CKGUID GetTypeGuid() const;
    std::string GetTypeName() const;
    int GetDataSize() const;
    std::string Describe() const;

private:
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    std::string m_Name;
    CKGUID m_TypeGuid;
    std::string m_TypeName;
    int m_DataSize = 0;
};

class ParamStructValue;

class ParamValue final : public RefCounted {
public:
    explicit ParamValue(const ScriptParamValue &value);

    const ScriptParamValue &Value() const;
    bool IsValid() const;
    CKGUID TypeGuid() const;
    std::string TypeName() const;

    int AsInt() const;
    float AsFloat() const;
    bool AsBool() const;
    std::string AsString() const;
    CKGUID AsGuid() const;
    VxVector AsVector() const;
    Vx2DVector AsVector2() const;
    VxColor AsColor() const;
    VxQuaternion AsQuaternion() const;
    VxMatrix AsMatrix() const;
    ParamStructValue *AsStruct() const;
    std::string AsText() const;
    NativeBuffer *AsRaw() const;

private:
    void ReportWrongKind(const char *method, const char *expected) const;

    ScriptParamValue m_Value;
};

class ParamStructValue final : public RefCounted {
public:
    explicit ParamStructValue(const ScriptParamValue &value);

    bool IsValid() const;
    CKGUID TypeGuid() const;
    std::string TypeName() const;
    ParamStructValue *Set(int index, ParamValue *value);
    ParamValue *Value() const;
    ParamValue *AsValue() const;
    std::string Describe() const;

private:
    ScriptParamValue m_Value;
};

class ParamStructRef final : public RefCounted {
public:
    ParamStructRef(ScriptBehaviorBridge *bridge, CK_ID parameterId);

    CKParameter *Get() const;
    bool IsValid() const;
    int Count() const;
    ParamStructInfo *Info() const;
    ParamRef *Member(int index) const;
    int FindMember(const std::string &name, int occurrence = 0) const;
    std::string Describe() const;

private:
    CKObject *RawGet() const;
    CKObject *RawGetStamped() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ParameterId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

class ParamSourceLinkRef final : public RefCounted {
public:
    ParamSourceLinkRef(ScriptBehaviorBridge *bridge,
                       CKParameterIn *target,
                       CKParameter *previousSource,
                       CKParameter *installedSource);
    ~ParamSourceLinkRef() override;

    bool IsValid() const;
    bool IsCommitted() const;
    bool IsRestored() const;
    bool Commit();
    bool Restore();
    std::string Describe() const;

private:
    CKContext *Context() const;
    CKParameterIn *Target() const;
    CKParameter *PreviousSource() const;
    CKParameter *InstalledSource() const;
    bool RestoreInternal(std::string &error, bool explicitCall);

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_TargetId = 0;
    CK_ID m_PreviousSourceId = 0;
    CK_ID m_InstalledSourceId = 0;
    ScriptBridgeObjectStamp m_TargetStamp;
    ScriptBridgeObjectStamp m_PreviousSourceStamp;
    ScriptBridgeObjectStamp m_InstalledSourceStamp;
    bool m_Committed = false;
    bool m_Restored = false;
};

class BehaviorLayout final : public RefCounted {
public:
    BehaviorLayout(ScriptBehaviorBridge *bridge, CK_ID behaviorId);
    BehaviorLayout(ScriptBehaviorBridge *bridge,
                   const CKBehaviorContext &ctx,
                   const ScriptBridgeBBInvocationSpec &request);

    int InputCount() const;
    int OutputCount() const;
    int PinCount() const;
    int PoutCount() const;
    int LocalCount() const;
    std::string InputName(int index) const;
    std::string OutputNameAt(int index) const;
    ParamInfo *Pin(int index) const;
    ParamInfo *Pout(int index) const;
    ParamInfo *Local(int index) const;
    int FindInput(const std::string &name, int occurrence) const;
    int FindOutput(const std::string &name, int occurrence) const;
    int FindPin(const std::string &name, int occurrence) const;
    int FindPout(const std::string &name, int occurrence) const;
    int FindLocal(const std::string &name, int occurrence) const;
    std::string Describe() const;

private:
    CKContext *Context() const;
    CKObject *RawBehavior() const;
    const ScriptBridgeLayoutRecord *LayoutRecord() const;
    ParamInfo *ParameterInfo(ScriptBridgeSlotKind kind, int index) const;
    int FindIo(bool input, const std::string &name, int occurrence) const;
    int FindParameter(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_BehaviorId = 0;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    ScriptBridgeObjectStamp m_BehaviorStamp;
    bool m_IsPrototype = false;
};

class ParamRef final : public RefCounted {
public:
    ParamRef(ScriptBehaviorBridge *bridge,
             CK_ID parameterId,
             ScriptBridgeSlotKind kind = ScriptBridgeSlotKind::Standalone,
             int index = -1,
             CK_ID ownerBehaviorId = 0);

    CKObject *Get() const;
    const ScriptBridgeObjectStamp &Stamp() const;
    CKParameter *Source() const;
    bool IsValid() const;
    CK_ID GetID() const;
    int GetIndex() const;
    int GetKind() const;
    std::string GetName() const;
    CKGUID TypeGuid() const;
    std::string TypeName() const;
    int DataSize() const;
    ParamRef *RealSource() const;
    ParamRef *DirectSource() const;
    ParamSourceLinkRef *SetSourceScoped(ParamRef *sourceRef);
    bool SetSource(ParamRef *sourceRef);
    bool Set(ParamValue *value);
    bool SetInt(int value);
    bool SetFloat(float value);
    bool SetBool(bool value);
    bool SetString(const std::string &value);
    bool SetObject(CKObject *value);
    bool SetEnum(const std::string &nameOrValue);
    bool SetEnumInt(int value);
    bool SetFlags(const std::string &namesOrMask);
    bool SetFlagsMask(CKDWORD value);
    bool SetStruct(ParamStructValue *value);
    ParamValue *GetValue() const;
    bool CopyFrom(ParamRef *sourceRef);
    std::string GetText() const;
    std::string GetEnumText() const;
    std::string GetFlagsText() const;
    ParamStructRef *Struct();
    bool SetText(const std::string &text);
    NativeBuffer *GetRaw() const;
    bool SetRaw(NativeBuffer *buffer);
    std::string Describe() const;

private:
    CKObject *RawGet() const;
    CKObject *RawGetStamped() const;
    CKBehavior *OwnerBehavior() const;
    CKParameter *WritableParameter(std::string &error) const;
    bool SetDirectValue(const ScriptParamValue &value);
    const char *KindName() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ParameterId = 0;
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    CK_ID m_OwnerBehaviorId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

class ParamOperationRef final : public RefCounted {
public:
    ParamOperationRef(ScriptBehaviorBridge *bridge,
                      CK_ID operationId,
                      CKParameterIn *targetInput = nullptr,
                      CKParameter *previousSource = nullptr,
                      const std::vector<CK_ID> &ownedLocalSourceIds = {});

    CKParameterOperation *Get() const;
    bool IsValid() const;
    CK_ID GetID() const;
    ParamRef *Out() const;
    ParamRef *In1() const;
    ParamRef *In2() const;
    CKERROR Do() const;
    bool Restore();
    bool Destroy();
    std::string Describe() const;

private:
    CKObject *RawGet() const;
    CKObject *RawGetStamped() const;
    CKParameterIn *TargetInput() const;
    CKParameter *PreviousSource() const;
    CKParameter *InstalledSource() const;
    void DestroyOwnedLocalSources(CKBehavior *owner);
    bool RestoreTargetSource(std::string &error);

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_OperationId = 0;
    CK_ID m_TargetInputId = 0;
    CK_ID m_PreviousSourceId = 0;
    CK_ID m_InstalledSourceId = 0;
    ScriptBridgeObjectStamp m_Stamp;
    ScriptBridgeObjectStamp m_TargetInputStamp;
    ScriptBridgeObjectStamp m_PreviousSourceStamp;
    ScriptBridgeObjectStamp m_InstalledSourceStamp;
    std::vector<ScriptBridgeStampedObjectId> m_OwnedLocalSources;
    bool m_TargetRestored = false;
};

class ParamOp final : public RefCounted {
public:
    ParamOp(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const std::string &operationName);
    ParamOp(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CKGUID operationGuid);

    ParamOp *Result(CKGUID guid);
    ParamOp *ResultName(const std::string &typeName);
    ParamOp *InRef(int slot, ParamRef *source);
    ParamOp *InValue(int slot, ParamValue *value);
    ScriptBridgeOperationSpec RequestForPin(int pinIndex) const;
    std::string Describe() const;

private:
    ScriptBridgeOperationInput *Slot(int slot);

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeOperationSpec m_Request;
};

class BehaviorRef final : public RefCounted {
public:
    BehaviorRef(ScriptBehaviorBridge *bridge, CK_ID behaviorId, CK_ID componentId);

    CKBehavior *Get() const;
    bool IsValid() const;
    CK_ID GetID() const;
    std::string GetName() const;
    bool IsActive() const;
    CKGUID GetPrototypeGuid() const;
    std::string GetPrototypeName() const;
    BehaviorLayout *Layout() const;
    bool Trigger(int inputIndex, bool reset);
    bool TriggerSlot(BBSlot *input, bool reset);
    GraphTask *Start(int inputIndex, bool reset, float timeoutSeconds);
    GraphTask *StartSlot(BBSlot *input, bool reset, float timeoutSeconds);
    GraphTask *Watch(float timeoutSeconds);
    bool InputActive(int inputIndex) const;
    bool InputActiveSlot(BBSlot *input) const;
    bool OutputActive(int outputIndex) const;
    bool OutputActiveSlot(BBSlot *output) const;
    ParamRef *Pin(int index) const;
    ParamRef *PinSlot(BBSlot *slot) const;
    ParamRef *Pout(int index) const;
    ParamRef *PoutSlot(BBSlot *slot) const;
    ParamRef *Local(int index) const;
    ParamRef *LocalSlot(BBSlot *slot) const;
    ParamOperationRef *ConnectOperation(int pinIndex, ParamOp *operation);
    ParamOperationRef *ConnectOperationSlot(BBSlot *slot, ParamOp *operation);
    std::string Describe() const;

private:
    CKObject *RawGet() const;
    CKObject *RawGetStamped() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_BehaviorId = 0;
    CK_ID m_ComponentId = 0;
    ScriptBridgeObjectStamp m_Stamp;
};

class BehaviorBridge final : public RefCounted {
public:
    BehaviorBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx);

    BehaviorRef *Self() const;
    BehaviorRef *OwnerScript() const;
    BehaviorRef *Find(const std::string &name) const;
    BehaviorRef *FindOn(CKBeObject *owner, const std::string &name) const;
    BehaviorRef *FindByID(CK_ID id) const;
    BehaviorRef *FromParameter(const std::string &name) const;

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
};

class BBSlot final : public RefCounted {
public:
    BBSlot(ScriptBehaviorBridge *bridge,
           const CKBehaviorContext &ctx,
           const ScriptBridgeBBInvocationSpec &request,
           ScriptBridgeSlotKind kind,
           int index,
           const std::string &name,
           CKGUID typeGuid,
           const std::string &typeName,
           int dataSize,
           const std::string &layoutSignature,
           const std::string &error = std::string());

    bool IsValid() const;
    std::string Error() const;
    int Kind() const;
    int Index() const;
    std::string Name() const;
    CKGUID TypeGuid() const;
    std::string TypeName() const;
    int DataSize() const;
    std::string Describe() const;

    bool ResolveIndex(ScriptBridgeSlotKind expected, int &index, std::string &error) const;

private:
    const ScriptBridgeLayoutRecord *LayoutRecord() const;
    std::string KindName() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    std::string m_Name;
    CKGUID m_TypeGuid;
    std::string m_TypeName;
    int m_DataSize = 0;
    std::string m_LayoutSignature;
    std::string m_Error;
};

class BBSpec final : public RefCounted {
public:
    BBSpec(ScriptBehaviorBridge *bridge,
           const CKBehaviorContext &ctx,
           const ScriptBridgeBBInvocationSpec &request,
           const std::string &error = std::string());

    bool IsValid() const;
    std::string Error() const;
    CKGUID GetGuid() const;
    std::string GetName() const;
    std::string GetCategory() const;
    std::string GetQualifiedName() const;
    BBPrototype *Prototype() const;
    BehaviorLayout *Layout() const;
    BBCallBuilder *Call();
    BBTaskBuilder *Spawn();
    BBBinding *Bind();
    BBSlot *In(const std::string &name, int occurrence = 0) const;
    BBSlot *Out(const std::string &name, int occurrence = 0) const;
    BBSlot *Pin(const std::string &name, int occurrence = 0) const;
    BBSlot *Pout(const std::string &name, int occurrence = 0) const;
    BBSlot *Local(const std::string &name, int occurrence = 0) const;
    std::string Describe() const;

private:
    CKBehaviorPrototype *PrototypeObject(std::string &error) const;
    BBSlot *ResolveSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    std::string m_Error;
};

class BBBinding final : public RefCounted {
public:
    BBBinding(ScriptBehaviorBridge *bridge,
              const CKBehaviorContext &ctx,
              const ScriptBridgeBBInvocationSpec &request,
              const std::string &error = std::string());
    ~BBBinding() override;

    bool IsValid() const;
    std::string Error() const;
    std::string Describe() const;
    BBSpec *Spec() const;
    BBTask *Task() const;
    BehaviorRef *Behavior() const;
    bool Raise(const CKBehaviorContext &ctx) const;

    BBSlot *In(const std::string &name, int occurrence = 0);
    BBSlot *Out(const std::string &name, int occurrence = 0);
    BBSlot *Pin(const std::string &name, int occurrence = 0);
    BBSlot *Pout(const std::string &name, int occurrence = 0);
    BBSlot *Local(const std::string &name, int occurrence = 0);
    bool RequireSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence = 0);

    BBBinding *Owner(CKBeObject *owner);
    BBBinding *Target(CKBeObject *target);
    BBBinding *Set(const std::string &pinName, ParamValue *value);
    BBBinding *SetInt(const std::string &pinName, int value);
    BBBinding *SetFloat(const std::string &pinName, float value);
    BBBinding *SetBool(const std::string &pinName, bool value);
    BBBinding *SetString(const std::string &pinName, const std::string &value);
    BBBinding *SetObject(const std::string &pinName, CKObject *value);
    BBBinding *SetSlot(BBSlot *pin, ParamValue *value);
    BBBinding *SetSlotInt(BBSlot *pin, int value);
    BBBinding *SetSlotFloat(BBSlot *pin, float value);
    BBBinding *SetSlotBool(BBSlot *pin, bool value);
    BBBinding *SetSlotString(BBSlot *pin, const std::string &value);
    BBBinding *SetSlotObject(BBSlot *pin, CKObject *value);
    BBBinding *Source(const std::string &pinName, ParamRef *source);
    BBBinding *SourceSlot(BBSlot *pin, ParamRef *source);
    BBBinding *Operation(const std::string &pinName, ParamOp *operation);
    BBBinding *OperationSlot(BBSlot *pin, ParamOp *operation);

    BBTask *Start(const CKBehaviorContext &ctx);
    BBTask *StartName(const CKBehaviorContext &ctx, const std::string &inputName);
    BBTask *StartSlot(const CKBehaviorContext &ctx, BBSlot *input);
    bool Step(const CKBehaviorContext &ctx);
    bool StepName(const CKBehaviorContext &ctx, const std::string &inputName);
    bool StepSlot(const CKBehaviorContext &ctx, BBSlot *input);
    bool Stop(const CKBehaviorContext &ctx);
    bool StopName(const CKBehaviorContext &ctx, const std::string &inputName);
    bool StopSlot(const CKBehaviorContext &ctx, BBSlot *input);
    BBTask *Restart(const CKBehaviorContext &ctx);
    bool Destroy();
    bool OutputActive(const std::string &outputName);
    bool OutputActiveSlot(BBSlot *output);
    ParamRef *PinRef(const std::string &pinName);
    ParamRef *PinRefSlot(BBSlot *pin);
    ParamRef *PoutRef(const std::string &poutName);
    ParamRef *PoutRefSlot(BBSlot *pout);

    void SetDefaultStart(const std::string &inputName);
    void SetDefaultStop(const std::string &inputName);
    void SetManaged(bool managed);
    bool IsManaged() const;

private:
    struct CachedSlot {
        ScriptBridgeSlotKind Kind = ScriptBridgeSlotKind::Standalone;
        int Occurrence = 0;
        std::string Name;
        BBSlot *Slot = nullptr;
    };

    BBSlot *Slot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence);
    BBSlot *FindCachedSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const;
    BBSlot *DefaultInputSlot();
    BBSlot *DefaultStopSlot();
    bool ResolvePin(BBSlot *slot, int &pinIndex, const char *method);
    bool SetValueForPin(BBSlot *slot, const ScriptParamValue &value, const char *method);
    bool SetLiveValue(BBSlot *slot, const ScriptParamValue &value, const char *method);
    bool SourceForPin(BBSlot *slot, ParamRef *source, const char *method);
    bool OperationForPin(BBSlot *slot, ParamOp *operation, const char *method);
    void ClearOwnedGraphLinks();
    void SetError(const std::string &error) const;
    BBTask *ReturnTask() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    mutable std::string m_Error;
    std::vector<CachedSlot> m_Slots;
    std::vector<ParamSourceLinkRef *> m_SourceLinks;
    std::vector<ParamOperationRef *> m_Operations;
    BBTask *m_Task = nullptr;
    std::string m_DefaultStartInput;
    std::string m_DefaultStopInput;
    bool m_Managed = false;
};

class BBCallBuilder final : public RefCounted {
public:
    BBCallBuilder(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request);

    BBCallBuilder *Owner(CKBeObject *owner);
    BBCallBuilder *Target(CKBeObject *target);
    BBCallBuilder *Set(int pinIndex, ParamValue *value);
    BBCallBuilder *SetSlot(BBSlot *slot, ParamValue *value);
    BBCallBuilder *SetSlotInt(BBSlot *slot, int value);
    BBCallBuilder *SetSlotFloat(BBSlot *slot, float value);
    BBCallBuilder *SetSlotBool(BBSlot *slot, bool value);
    BBCallBuilder *SetSlotString(BBSlot *slot, const std::string &value);
    BBCallBuilder *SetSlotObject(BBSlot *slot, CKObject *value);
    BBCallBuilder *SetSource(int pinIndex, ParamRef *source);
    BBCallBuilder *Source(BBSlot *slot, ParamRef *source);
    BBCallBuilder *SetOperation(int pinIndex, ParamOp *operation);
    BBCallBuilder *Operation(BBSlot *slot, ParamOp *operation);
    BBResult *Run(int inputIndex);
    BBResult *RunSlot(BBSlot *input);

private:
    BBCallBuilder *SetValueForPin(int pinIndex, const ScriptParamValue &value);
    bool ResolvePinSlot(BBSlot *slot, int &pinIndex, const char *method);

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBTaskBuilder final : public RefCounted {
public:
    BBTaskBuilder(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request);

    BBTaskBuilder *Owner(CKBeObject *owner);
    BBTaskBuilder *Target(CKBeObject *target);
    BBTaskBuilder *Set(int pinIndex, ParamValue *value);
    BBTaskBuilder *SetSlot(BBSlot *slot, ParamValue *value);
    BBTaskBuilder *SetSlotInt(BBSlot *slot, int value);
    BBTaskBuilder *SetSlotFloat(BBSlot *slot, float value);
    BBTaskBuilder *SetSlotBool(BBSlot *slot, bool value);
    BBTaskBuilder *SetSlotString(BBSlot *slot, const std::string &value);
    BBTaskBuilder *SetSlotObject(BBSlot *slot, CKObject *value);
    BBTaskBuilder *SetSource(int pinIndex, ParamRef *source);
    BBTaskBuilder *Source(BBSlot *slot, ParamRef *source);
    BBTaskBuilder *SetOperation(int pinIndex, ParamOp *operation);
    BBTaskBuilder *Operation(BBSlot *slot, ParamOp *operation);
    BBTask *Start(int inputIndex);
    BBTask *StartSlot(BBSlot *input);

private:
    BBTaskBuilder *SetValueForPin(int pinIndex, const ScriptParamValue &value);
    bool ResolvePinSlot(BBSlot *slot, int &pinIndex, const char *method);

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBPrototype final : public RefCounted {
public:
    BBPrototype(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, const ScriptBridgeBBInvocationSpec &request);

    BBCallBuilder *Call();
    BBTaskBuilder *Spawn();
    BehaviorLayout *Layout() const;
    bool IsValid() const;
    CKGUID GetGuid() const;
    std::string GetName() const;
    std::string GetCategory() const;
    std::string GetQualifiedName() const;
    std::string Describe() const;

private:
    CKBehaviorPrototype *Prototype(std::string &error) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
};

class BBBridge final : public RefCounted {
public:
    BBBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx);

    BBPrototype *PrototypeByName(const std::string &name) const;
    BBPrototype *PrototypeByGuid(CKGUID guid) const;
    int Count() const;
    BBPrototype *At(int index) const;
    BBPrototype *Find(const std::string &query, int occurrence = 0) const;
    CScriptArray *FindAll(const std::string &query) const;
    BBSpec *Require(const std::string &query) const;
    BBSpec *RequireGuid(CKGUID guid) const;
    BBBinding *Bind(const std::string &query) const;
    BBBinding *BindGuid(CKGUID guid) const;

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
};

class BBResult final : public RefCounted {
public:
    BBResult(ScriptBehaviorBridge *bridge, CK_ID resultId, int generation);
    ~BBResult() override;

    bool Ok() const;
    int ReturnCode() const;
    std::string Error() const;
    bool OutputActive(int outputIndex) const;
    bool OutputActiveSlot(BBSlot *output) const;
    ParamRef *Pout(int index) const;
    ParamRef *PoutSlot(BBSlot *slot) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_ResultId = 0;
    int m_Generation = 0;
};

class BBTask final : public RefCounted {
public:
    BBTask(ScriptBehaviorBridge *bridge, CK_ID taskId, int generation);
    ~BBTask() override;

    bool IsValid() const;
    bool IsAlive() const;
    bool IsPaused() const;
    int ReturnCode() const;
    std::string Error() const;
    bool OutputActive(int outputIndex) const;
    bool OutputActiveSlot(BBSlot *output) const;
    bool Step(const CKBehaviorContext &ctx, int inputIndex);
    bool StepSlot(const CKBehaviorContext &ctx, BBSlot *input);
    bool Reset();
    bool Destroy();
    BehaviorRef *Behavior() const;
    ParamRef *Pout(int index) const;
    ParamRef *PoutSlot(BBSlot *slot) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_TaskId = 0;
    int m_Generation = 0;
};

class GraphTask final : public RefCounted {
public:
    GraphTask(ScriptBehaviorBridge *bridge, CK_ID watchId, int generation);
    ~GraphTask() override;

    bool IsValid() const;
    bool IsAlive() const;
    bool IsPaused() const;
    bool TimedOut() const;
    float Elapsed() const;
    std::string Error() const;
    GraphTask *Timeout(float seconds);
    bool Step(const CKBehaviorContext &ctx);
    bool Done(int outputIndex) const;
    bool DoneSlot(BBSlot *output) const;
    bool OutputActive(int outputIndex) const;
    bool OutputActiveSlot(BBSlot *output) const;
    bool Cancel();
    bool Reset();
    BehaviorRef *Behavior() const;
    ParamRef *Pout(int index) const;
    ParamRef *PoutSlot(BBSlot *slot) const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBridgeExecutionState State() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_WatchId = 0;
    int m_Generation = 0;
};

#endif // CK_SCRIPTBRIDGEHANDLES_H
