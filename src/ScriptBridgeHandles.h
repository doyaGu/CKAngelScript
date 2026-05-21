#ifndef CK_SCRIPTBRIDGEHANDLES_H
#define CK_SCRIPTBRIDGEHANDLES_H

#include "ScriptBridgeCommon.h"
#include "ScriptBridgeParameterIO.h"
#include "ScriptNativeBuffer.h"
#include "ScriptParameterRegistry.h"

class CScriptArray;
class BBCallBuilder;
class BBTaskBuilder;
class BBConfig;
class BehaviorGraph;
class BehaviorGraphEdit;
class BehaviorLinkRef;
class BehaviorNode;
class BehaviorQuery;
class GraphEditLink;
class GraphEditNode;
class GraphEditResult;

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
    bool DestroyDetached();
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
    int SettingCount() const;
    int LocalCount() const;
    std::string InputName(int index) const;
    std::string OutputNameAt(int index) const;
    ParamInfo *Pin(int index) const;
    ParamInfo *Pout(int index) const;
    ParamInfo *Setting(int index) const;
    ParamInfo *Local(int index) const;
    int FindInput(const std::string &name, int occurrence) const;
    int FindOutput(const std::string &name, int occurrence) const;
    int FindPin(const std::string &name, int occurrence) const;
    int FindPout(const std::string &name, int occurrence) const;
    int FindSetting(const std::string &name, int occurrence) const;
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
    bool DestroyDetached();
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
    BehaviorGraph *AsGraph() const;
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

class BehaviorQuery final : public RefCounted {
public:
    BehaviorQuery();

    BehaviorQuery *Name(const std::string &name);
    BehaviorQuery *NameContains(const std::string &text);
    BehaviorQuery *PrototypeGuid(CKGUID guid);
    BehaviorQuery *PrototypeName(const std::string &name);
    BehaviorQuery *PrototypeQuery(const std::string &query);
    BehaviorQuery *Target(CKBeObject *target);
    BehaviorQuery *TargetName(const std::string &name);
    BehaviorQuery *TargetId(CK_ID id);
    BehaviorQuery *InputCount(int count);
    BehaviorQuery *OutputCount(int count);
    BehaviorQuery *PinCount(int count);
    BehaviorQuery *PoutCount(int count);
    BehaviorQuery *MaxDepth(int depth);
    BehaviorQuery *IncludeRoot(bool includeRoot);
    BehaviorQuery *Recursive(bool recursive);
    BehaviorQuery *Occurrence(int occurrence);
    std::string Describe() const;

    bool Matches(CKBehavior *behavior, int depth) const;
    int GetOccurrence() const;
    bool IsRecursive() const;
    bool IncludeRootNode() const;
    int GetMaxDepth() const;

private:
    bool CountMatches(int actual, int expected) const;

    std::string m_Name;
    std::string m_NameContains;
    CKGUID m_PrototypeGuid;
    std::string m_PrototypeName;
    CK_ID m_TargetId = 0;
    std::string m_TargetName;
    int m_InputCount = -1;
    int m_OutputCount = -1;
    int m_PinCount = -1;
    int m_PoutCount = -1;
    int m_MaxDepth = -1;
    int m_Occurrence = 0;
    bool m_Recursive = true;
    bool m_IncludeRoot = false;
};

class BehaviorGraph final : public RefCounted {
public:
    BehaviorGraph(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CK_ID rootBehaviorId);

    bool IsValid() const;
    BehaviorNode *Root() const;
    BehaviorGraphEdit *Edit() const;
    BehaviorNode *Find(BehaviorQuery *query) const;
    BehaviorNode *Require(BehaviorQuery *query) const;
    CScriptArray *FindAll(BehaviorQuery *query) const;
    std::string DescribeCandidates(BehaviorQuery *query) const;
    std::string Describe() const;

    CKBehavior *RootBehavior() const;
    ScriptBehaviorBridge *Bridge() const;
    CK_ID ComponentId() const;
    CK_ID RootId() const;
    const CKBehaviorContext &Context() const;

private:
    std::vector<CKBehavior *> FindAllRaw(BehaviorQuery *query) const;
    BehaviorNode *WrapNode(CKBehavior *behavior, const std::string &error = std::string()) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    CK_ID m_RootBehaviorId = 0;
    ScriptBridgeObjectStamp m_RootStamp;
};

class GraphEditResult final : public RefCounted {
public:
    GraphEditResult(ScriptBehaviorBridge *bridge,
                    const CKBehaviorContext &ctx,
                    bool ok,
                    const std::string &error,
                    const std::string &description,
                    const std::vector<CK_ID> &createdNodeIds = {},
                    const std::vector<CK_ID> &createdLinkIds = {});

    bool Ok() const;
    bool IsOk() const;
    std::string Error() const;
    std::string Describe() const;
    CScriptArray *CreatedNodes() const;
    CScriptArray *CreatedLinks() const;
    bool Raise(const CKBehaviorContext &ctx) const;

private:
    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    bool m_Ok = false;
    std::string m_Error;
    std::string m_Description;
    std::vector<CK_ID> m_CreatedNodeIds;
    std::vector<CK_ID> m_CreatedLinkIds;
};

class GraphEditNode final : public RefCounted {
public:
    GraphEditNode(BehaviorGraphEdit *edit, int specIndex, CK_ID behaviorId, const std::string &error = std::string());
    ~GraphEditNode() override;

    bool IsValid() const;
    std::string Error() const;
    BehaviorRef *Behavior() const;
    std::string Describe() const;

    BehaviorGraphEdit *EditOwner() const;
    int SpecIndex() const;
    CK_ID BehaviorId() const;

private:
    BehaviorGraphEdit *m_Edit = nullptr;
    int m_SpecIndex = -1;
    CK_ID m_BehaviorId = 0;
    std::string m_Error;
};

class GraphEditLink final : public RefCounted {
public:
    GraphEditLink(BehaviorGraphEdit *edit, int specIndex, CK_ID linkId, const std::string &error = std::string());
    ~GraphEditLink() override;

    bool IsValid() const;
    std::string Error() const;
    BehaviorLinkRef *Link() const;
    std::string Describe() const;

    BehaviorGraphEdit *EditOwner() const;
    int SpecIndex() const;
    CK_ID LinkId() const;

private:
    BehaviorGraphEdit *m_Edit = nullptr;
    int m_SpecIndex = -1;
    CK_ID m_LinkId = 0;
    std::string m_Error;
};

class BehaviorGraphEdit final : public RefCounted {
public:
    BehaviorGraphEdit(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CK_ID rootBehaviorId);
    ~BehaviorGraphEdit() override;

    bool IsValid() const;
    std::string Error() const;
    std::string Describe() const;
    GraphEditNode *Import(BehaviorNode *node);
    GraphEditNode *AddDecl(BBDecl *decl, const std::string &name = std::string());
    GraphEditNode *AddConfig(BBConfig *config, const std::string &name = std::string());
    BehaviorGraphEdit *Remove(BehaviorNode *node, bool removeIncidentLinks = false);
    BehaviorGraphEdit *Move(BehaviorNode *node, BehaviorGraph *targetGraph);
    GraphEditLink *Link(GraphEditNode *source, int sourceOutputIndex, GraphEditNode *target, int targetInputIndex, int delay = 1);
    GraphEditLink *LinkSlots(GraphEditNode *source, BBSlot *sourceOutput, GraphEditNode *target, BBSlot *targetInput, int delay = 1);
    BehaviorGraphEdit *Unlink(BehaviorLinkRef *link);
    GraphEditLink *Relink(BehaviorLinkRef *link, GraphEditNode *source, int sourceOutputIndex, GraphEditNode *target, int targetInputIndex, int delay = 1);
    BehaviorGraphEdit *SetSlot(GraphEditNode *node, BBSlot *pin, ParamValue *value);
    BehaviorGraphEdit *SetSlotInt(GraphEditNode *node, BBSlot *pin, int value);
    BehaviorGraphEdit *SetSlotFloat(GraphEditNode *node, BBSlot *pin, float value);
    BehaviorGraphEdit *SetSlotBool(GraphEditNode *node, BBSlot *pin, bool value);
    BehaviorGraphEdit *SetSlotString(GraphEditNode *node, BBSlot *pin, const std::string &value);
    BehaviorGraphEdit *SetSlotObject(GraphEditNode *node, BBSlot *pin, CKObject *value);
    BehaviorGraphEdit *SetSetting(GraphEditNode *node, BBSlot *setting, ParamValue *value);
    BehaviorGraphEdit *SetSettingString(GraphEditNode *node, BBSlot *setting, const std::string &value);
    BehaviorGraphEdit *Source(GraphEditNode *node, BBSlot *pin, ParamRef *source);
    BehaviorGraphEdit *Operation(GraphEditNode *node, BBSlot *pin, ParamOp *operation);
    GraphEditResult *Validate(const CKBehaviorContext &ctx) const;
    GraphEditResult *Apply(const CKBehaviorContext &ctx);

    ScriptBehaviorBridge *Bridge() const;
    const CKBehaviorContext &Context() const;
    CK_ID RootId() const;
    CKBehavior *RootBehavior() const;
    CKBehavior *ResolveNode(const GraphEditNode *node) const;
    BehaviorLinkRef *ResolveLink(const GraphEditLink *link) const;

private:
    struct NodeSpec {
        enum class Kind { Existing, Create };
        Kind Type = Kind::Existing;
        CK_ID BehaviorId = 0;
        ScriptBridgeObjectStamp BehaviorStamp;
        ScriptBridgeBBInvocationSpec Request;
        std::string Name;
        CK_ID CreatedBehaviorId = 0;
    };

    struct LinkSpec {
        enum class Kind { Create, Unlink };
        Kind Type = Kind::Create;
        int SourceNodeIndex = -1;
        int TargetNodeIndex = -1;
        int SourceOutputIndex = -1;
        int TargetInputIndex = -1;
        int Delay = 1;
        CK_ID ExistingLinkId = 0;
        ScriptBridgeObjectStamp ExistingLinkStamp;
        CK_ID CreatedLinkId = 0;
    };

    struct RemoveSpec {
        int NodeIndex = -1;
        bool RemoveIncidentLinks = false;
    };

    struct MoveSpec {
        int NodeIndex = -1;
        CK_ID TargetRootId = 0;
        ScriptBridgeObjectStamp TargetRootStamp;
    };

    struct ValueSpec {
        int NodeIndex = -1;
        ScriptBridgeSlotKind Kind = ScriptBridgeSlotKind::Standalone;
        int SlotIndex = -1;
        ScriptParamValue Value;
    };

    struct SourceSpec {
        int NodeIndex = -1;
        int PinIndex = -1;
        CK_ID SourceId = 0;
        ScriptBridgeObjectStamp SourceStamp;
    };

    struct OperationSpec {
        int NodeIndex = -1;
        ScriptBridgeOperationSpec Operation;
    };

    GraphEditResult *MakeResult(bool ok,
                                const std::string &error,
                                const std::string &description,
                                const std::vector<CK_ID> &createdNodes = {},
                                const std::vector<CK_ID> &createdLinks = {}) const;
    bool ValidateInternal(const CKBehaviorContext &ctx, std::string &error) const;
    bool ResolveNodeIndex(const GraphEditNode *node, int &index, std::string &error) const;
    CKBehavior *ResolveNodeBehavior(int index) const;
    CKBehaviorLink *ResolveExistingLink(const LinkSpec &spec) const;
    bool ValidateValueSpec(CKContext *context, const ValueSpec &spec, std::string &error) const;
    bool ApplyExistingValue(CKBehavior *behavior,
                            const ValueSpec &spec,
                            std::string &error,
                            std::vector<ParamSourceLinkRef *> &sourceLinks,
                            std::vector<CK_ID> &localSourceIds,
                            std::vector<CK_ID> &replacedLocalSourceIds,
                            std::vector<CK_ID> &replacedOperations);
    bool ApplyExistingSource(CKBehavior *behavior,
                             const SourceSpec &spec,
                             std::string &error,
                             std::vector<ParamSourceLinkRef *> &sourceLinks,
                             std::vector<CK_ID> &replacedLocalSourceIds,
                             std::vector<CK_ID> &replacedOperations);
    bool ApplyExistingOperation(CKBehavior *behavior,
                                const OperationSpec &spec,
                                std::string &error,
                                std::vector<ParamOperationRef *> &operations,
                                std::vector<CK_ID> &replacedOperations);
    BehaviorGraphEdit *SetValue(GraphEditNode *node, BBSlot *slot, ScriptBridgeSlotKind kind, const ScriptParamValue &value, const char *method);
    void RemoveNodeValue(int nodeIndex, ScriptBridgeSlotKind kind, int slotIndex);
    void RemoveNodeSource(int nodeIndex, int pinIndex);
    void RemoveNodeOperation(int nodeIndex, int pinIndex);
    void SetRequestSource(ScriptBridgeBBInvocationSpec &request, const ScriptBridgeInputSource &source);
    void SetRequestOperation(ScriptBridgeBBInvocationSpec &request, const ScriptBridgeOperationSpec &operation);
    void SetError(const std::string &error) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    CK_ID m_RootBehaviorId = 0;
    ScriptBridgeObjectStamp m_RootStamp;
    mutable std::string m_Error;
    bool m_Applied = false;
    std::vector<NodeSpec> m_Nodes;
    std::vector<LinkSpec> m_Links;
    std::vector<RemoveSpec> m_Removes;
    std::vector<MoveSpec> m_Moves;
    std::vector<ValueSpec> m_Values;
    std::vector<SourceSpec> m_Sources;
    std::vector<OperationSpec> m_Operations;
};

class BehaviorNode final : public RefCounted {
public:
    BehaviorNode(ScriptBehaviorBridge *bridge,
                 const CKBehaviorContext &ctx,
                 CK_ID rootBehaviorId,
                 CK_ID behaviorId,
                 CK_ID componentId,
                 const std::string &error = std::string());

    bool IsValid() const;
    std::string Error() const;
    BehaviorRef *Behavior() const;
    BehaviorGraph *AsGraph() const;
    BehaviorNode *Input(int index) const;
    BehaviorNode *Output(int index) const;
    BehaviorNode *Next(BehaviorQuery *query = nullptr) const;
    BehaviorNode *Prev(BehaviorQuery *query = nullptr) const;
    CScriptArray *NextAll(BehaviorQuery *query = nullptr) const;
    CScriptArray *PrevAll(BehaviorQuery *query = nullptr) const;
    BehaviorLinkRef *NextLink(BehaviorQuery *query = nullptr) const;
    BehaviorLinkRef *PrevLink(BehaviorQuery *query = nullptr) const;
    BehaviorNode *End(int maxSteps = 256) const;
    std::string Describe() const;

    CKBehavior *Get() const;
    CK_ID RootId() const;
    CK_ID BehaviorId() const;

private:
    CScriptArray *AdjacentAll(bool next, int ioIndex, BehaviorQuery *query) const;
    BehaviorNode *AdjacentFirst(bool next, int ioIndex, BehaviorQuery *query) const;
    BehaviorLinkRef *AdjacentLinkFirst(bool next, int ioIndex, BehaviorQuery *query) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    CK_ID m_RootBehaviorId = 0;
    CK_ID m_BehaviorId = 0;
    CK_ID m_ComponentId = 0;
    ScriptBridgeObjectStamp m_BehaviorStamp;
    std::string m_Error;
};

class BehaviorLinkRef final : public RefCounted {
public:
    BehaviorLinkRef(ScriptBehaviorBridge *bridge,
                    CK_ID rootBehaviorId,
                    CK_ID linkId,
                    CK_ID componentId);

    bool IsValid() const;
    BehaviorRef *SourceBehavior() const;
    int SourceOutputIndex() const;
    BehaviorRef *TargetBehavior() const;
    int TargetInputIndex() const;
    int Delay() const;
    std::string Describe() const;
    CK_ID RootId() const;
    CK_ID LinkId() const;

private:
    CKBehaviorLink *Get() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CK_ID m_RootBehaviorId = 0;
    CK_ID m_LinkId = 0;
    CK_ID m_ComponentId = 0;
    ScriptBridgeObjectStamp m_LinkStamp;
};

class BehaviorBridge final : public RefCounted {
public:
    BehaviorBridge(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx);

    BehaviorGraph *Graph() const;
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
           CKDWORD caps,
           int layoutGeneration,
           const std::string &layoutSignature,
           const std::string &error = std::string(),
           CK_ID runtimeBehaviorId = 0,
           const ScriptBridgeObjectStamp &runtimeBehaviorStamp = ScriptBridgeObjectStamp());

    bool IsValid() const;
    std::string Error() const;
    int Kind() const;
    int Index() const;
    std::string Name() const;
    CKGUID TypeGuid() const;
    std::string TypeName() const;
    int DataSize() const;
    CKDWORD Caps() const;
    int LayoutGeneration() const;
    int Occurrence() const;
    bool IsSetting() const;
    bool IsRequired() const;
    bool IsStart() const;
    bool IsStop() const;
    bool HasDefault() const;
    std::string DefaultText() const;
    bool HasValue() const;
    std::string ValueText() const;
    std::string Describe() const;

    bool ResolveIndex(ScriptBridgeSlotKind expected, int &index, std::string &error) const;
    void SetMetadata(CKDWORD flags, const std::string &defaultText, const std::string &valueText);

private:
    const ScriptBridgeLayoutRecord *LayoutRecord() const;
    std::string KindName() const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    CK_ID m_RuntimeBehaviorId = 0;
    ScriptBridgeObjectStamp m_RuntimeBehaviorStamp;
    ScriptBridgeSlotKind m_Kind = ScriptBridgeSlotKind::Standalone;
    int m_Index = -1;
    std::string m_Name;
    CKGUID m_TypeGuid;
    std::string m_TypeName;
    int m_DataSize = 0;
    CKDWORD m_Caps = 0;
    CKDWORD m_MetadataFlags = 0;
    int m_LayoutGeneration = 0;
    std::string m_LayoutSignature;
    std::string m_DefaultText;
    std::string m_ValueText;
    std::string m_Error;
};

class BBDecl final : public RefCounted {
public:
    BBDecl(ScriptBehaviorBridge *bridge,
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
    BBConfig *Configure();
    BBSlot *In(const std::string &name, int occurrence = 0) const;
    BBSlot *Input(const std::string &name, int occurrence = 0) const;
    BBSlot *Out(const std::string &name, int occurrence = 0) const;
    BBSlot *Output(const std::string &name, int occurrence = 0) const;
    BBSlot *Pin(const std::string &name, int occurrence = 0) const;
    BBSlot *Pout(const std::string &name, int occurrence = 0) const;
    BBSlot *Setting(const std::string &name, int occurrence = 0) const;
    BBSlot *Local(const std::string &name, int occurrence = 0) const;
    CKGUID Guid() const;
    std::string Name() const;
    std::string Category() const;
    std::string QualifiedName() const;
    CKDWORD BehaviorFlags() const;
    CKDWORD PrototypeFlags() const;
    CK_CLASSID CompatibleClassId() const;
    int NeededManagerCount() const;
    CKGUID NeededManagerGuid(int index) const;
    std::string Describe() const;
    const ScriptBridgeBBInvocationSpec &Request() const;

private:
    CKBehaviorPrototype *PrototypeObject(std::string &error) const;
    BBSlot *ResolveSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    std::string m_Error;
};

class BBConfig final : public RefCounted {
public:
    BBConfig(ScriptBehaviorBridge *bridge,
              const CKBehaviorContext &ctx,
              const ScriptBridgeBBInvocationSpec &request,
              const std::string &error = std::string());
    ~BBConfig() override;

    bool IsValid() const;
    std::string Error() const;
    std::string Describe() const;
    std::string Explain() const;
    BBDecl *Spec() const;
    BehaviorRef *Behavior() const;
    bool Raise(const CKBehaviorContext &ctx) const;

    BBSlot *In(const std::string &name, int occurrence = 0);
    BBSlot *Out(const std::string &name, int occurrence = 0);
    BBSlot *Pin(const std::string &name, int occurrence = 0);
    BBSlot *Pout(const std::string &name, int occurrence = 0);
    BBSlot *Setting(const std::string &name, int occurrence = 0);
    BBSlot *Local(const std::string &name, int occurrence = 0);
    bool RequireSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence = 0);

    BBConfig *Owner(CKBeObject *owner);
    BBConfig *Target(CKBeObject *target);
    BBConfig *SetSlot(BBSlot *pin, ParamValue *value);
    BBConfig *SetSlotInt(BBSlot *pin, int value);
    BBConfig *SetSlotFloat(BBSlot *pin, float value);
    BBConfig *SetSlotBool(BBSlot *pin, bool value);
    BBConfig *SetSlotString(BBSlot *pin, const std::string &value);
    BBConfig *SetSlotObject(BBSlot *pin, CKObject *value);
    BBConfig *SourceSlot(BBSlot *pin, ParamRef *source);
    BBConfig *OperationSlot(BBSlot *pin, ParamOp *operation);
    BBConfig *SetSetting(BBSlot *setting, ParamValue *value);
    BBConfig *SetSettingString(BBSlot *setting, const std::string &value);
    bool Validate(const CKBehaviorContext &ctx) const;
    BBDecl *Decl() const;
    BBInstance *Instance() const;
    BBInstance *EnsureSpawned(const CKBehaviorContext &ctx);
    BBInstance *EnsureStarted(const CKBehaviorContext &ctx);
    BBInstance *SpawnInstance(const CKBehaviorContext &ctx);
    BBInstance *SpawnStarted(const CKBehaviorContext &ctx);

    bool Stop(const CKBehaviorContext &ctx);
    bool Destroy();
    bool OutputActiveSlot(BBSlot *output);
    ParamRef *PinRefSlot(BBSlot *pin);
    ParamRef *PoutRefSlot(BBSlot *pout);

    void SetDefaultStart(const std::string &inputName);
    void SetDefaultStop(const std::string &inputName);
    void SetComponentLifetime(bool componentLifetime);
    bool UsesComponentLifetime() const;
    bool RegisterSlot(BBSlot *slot);
    const ScriptBridgeBBInvocationSpec &Request() const;

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
    bool SetValueForSetting(BBSlot *slot, const ScriptParamValue &value, const char *method);
    bool ApplySlotMetadata(BBSlot *slot);
    void RemovePendingValue(int pinIndex);
    void RemovePendingSource(int pinIndex);
    void RemovePendingOperation(int pinIndex);
    void ReplacePendingSource(const ScriptBridgeInputSource &source);
    void ReplacePendingOperation(const ScriptBridgeOperationSpec &operation);
    void SetError(const std::string &error) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    mutable std::string m_Error;
    std::vector<CachedSlot> m_Slots;
    std::vector<BBSlot *> m_RegisteredSlots;
    BBInstance *m_Instance = nullptr;
    std::string m_DefaultStartInput;
    int m_DefaultStartOccurrence = 0;
    std::string m_DefaultStopInput;
    int m_DefaultStopOccurrence = 0;
    bool m_ComponentLifetime = false;
};

class BBInstance final : public RefCounted {
public:
    BBInstance(ScriptBehaviorBridge *bridge,
               const CKBehaviorContext &ctx,
               const ScriptBridgeBBInvocationSpec &request,
               CK_ID instanceId,
               int generation,
               const std::string &defaultStartInput,
               int defaultStartOccurrence,
               const std::string &defaultStopInput,
               int defaultStopOccurrence,
               const std::string &error = std::string());
    ~BBInstance() override;

    bool IsValid() const;
    bool IsAlive() const;
    std::string Error() const;
    std::string Explain() const;
    BBDecl *Decl() const;
    BehaviorRef *Behavior() const;
    BehaviorLayout *Layout() const;
    BBSlot *Input(const std::string &name, int occurrence = 0) const;
    BBSlot *Output(const std::string &name, int occurrence = 0) const;
    BBSlot *PinSlot(const std::string &name, int occurrence = 0) const;
    BBSlot *PoutSlot(const std::string &name, int occurrence = 0) const;
    BBSlot *Setting(const std::string &name, int occurrence = 0) const;
    BBSlot *Local(const std::string &name, int occurrence = 0) const;
    bool Start();
    bool Start(BBSlot *input);
    bool StartWithContext(const CKBehaviorContext &ctx);
    bool StartSlotWithContext(const CKBehaviorContext &ctx, BBSlot *input);
    bool Step(const CKBehaviorContext &ctx);
    bool Stop();
    bool StopWithContext(const CKBehaviorContext &ctx);
    bool OutputActive(BBSlot *output) const;
    ParamRef *Pin(BBSlot *pin) const;
    ParamRef *Pout(BBSlot *pout) const;
    bool SetSlot(BBSlot *pin, ParamValue *value);
    bool SetSlotInt(BBSlot *pin, int value);
    bool SetSlotFloat(BBSlot *pin, float value);
    bool SetSlotBool(BBSlot *pin, bool value);
    bool SetSlotString(BBSlot *pin, const std::string &value);
    bool SetSlotObject(BBSlot *pin, CKObject *value);
    bool SourceSlot(BBSlot *pin, ParamRef *source);
    bool OperationSlot(BBSlot *pin, ParamOp *operation);
    bool StepSetSlot(const CKBehaviorContext &ctx, BBSlot *pin, ParamValue *value);
    bool StepSetSlotInt(const CKBehaviorContext &ctx, BBSlot *pin, int value);
    bool StepSetSlotFloat(const CKBehaviorContext &ctx, BBSlot *pin, float value);
    bool StepSetSlotBool(const CKBehaviorContext &ctx, BBSlot *pin, bool value);
    bool StepSetSlotString(const CKBehaviorContext &ctx, BBSlot *pin, const std::string &value);
    bool StepSetSlotObject(const CKBehaviorContext &ctx, BBSlot *pin, CKObject *value);
    bool SetSettingValue(BBSlot *setting, ParamValue *value);
    bool SetSetting(BBSlot *setting, const std::string &value);
    bool Destroy();
    bool Raise(const CKBehaviorContext &ctx) const;
    CK_ID BridgeInstanceId() const;
    int BridgeGeneration() const;

private:
    BBSlot *RuntimeSlot(ScriptBridgeSlotKind kind, const std::string &name, int occurrence) const;
    bool SetValueForPin(BBSlot *pin, const ScriptParamValue &value, const char *method);
    bool SourceForPin(BBSlot *pin, ParamRef *source, const char *method);
    bool OperationForPin(BBSlot *pin, ParamOp *operation, const char *method);
    void SetError(const std::string &error) const;

    ScriptBehaviorBridge *m_Bridge = nullptr;
    CKBehaviorContext m_Context;
    ScriptBridgeBBInvocationSpec m_Request;
    mutable std::string m_Error;
    CK_ID m_InstanceId = 0;
    int m_Generation = 0;
    std::string m_DefaultStartInput;
    int m_DefaultStartOccurrence = 0;
    std::string m_DefaultStopInput;
    int m_DefaultStopOccurrence = 0;
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
    BBDecl *Require(const std::string &query) const;
    BBDecl *RequireGuid(CKGUID guid) const;

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
    CK_ID BridgeTaskId() const;
    int BridgeGeneration() const;

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
