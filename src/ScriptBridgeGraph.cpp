#include "ScriptBridgeHandles.h"

#include <algorithm>
#include <set>

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"
#include "ScriptParameterConversion.h"

namespace ScriptBridgeGraphInternal {

CKContext *GraphContext(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx) {
    if (ctx.Context) {
        return ctx.Context;
    }
    return bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr;
}

CKBehavior *BehaviorById(ScriptBehaviorBridge *bridge, CK_ID id) {
    CKContext *context = bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr;
    return CKBehavior::Cast(GetCKObjectById(context, id));
}

CKBehavior *StampedBehaviorById(ScriptBehaviorBridge *bridge, CK_ID id, const ScriptBridgeObjectStamp &stamp) {
    CKContext *context = bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr;
    return CKBehavior::Cast(GetStampedCKObjectById(context, id, stamp));
}

CKBehaviorLink *StampedLinkById(ScriptBehaviorBridge *bridge, CK_ID id, const ScriptBridgeObjectStamp &stamp) {
    CKContext *context = bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr;
    return CKBehaviorLink::Cast(GetStampedCKObjectById(context, id, stamp));
}

std::string PrototypeQualifiedName(CKObjectDeclaration *decl) {
    if (!decl) {
        return std::string();
    }
    const std::string category = SafeString(decl->GetCategory());
    const std::string name = SafeString(decl->GetName());
    return category.empty() ? name : category + "/" + name;
}

CKGUID PrototypeGuidFromQuery(const std::string &query) {
    CKGUID parsed;
    if (ParseScriptGuidString(query, parsed)) {
        return CKGetPrototypeFromGuid(parsed) ? parsed : CKGUID();
    }
    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (!decl) {
            continue;
        }
        if (SafeString(decl->GetName()) == query || PrototypeQualifiedName(decl) == query) {
            return decl->GetGuid();
        }
    }
    return CKGUID();
}

asITypeInfo *NodeArrayType(ScriptBehaviorBridge *bridge) {
    ScriptManager *manager = bridge ? bridge->GetManager() : nullptr;
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    return engine ? engine->GetTypeInfoByDecl("array<BehaviorNode@>") : nullptr;
}

asITypeInfo *LinkArrayType(ScriptBehaviorBridge *bridge) {
    ScriptManager *manager = bridge ? bridge->GetManager() : nullptr;
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    return engine ? engine->GetTypeInfoByDecl("array<BehaviorLinkRef@>") : nullptr;
}

CScriptArray *CreateNodeArray(ScriptBehaviorBridge *bridge) {
    asITypeInfo *arrayType = NodeArrayType(bridge);
    if (!arrayType) {
        SetScriptException("array<BehaviorNode@> is not registered.");
        return nullptr;
    }
    return CScriptArray::Create(arrayType, asUINT(0));
}

CScriptArray *CreateLinkArray(ScriptBehaviorBridge *bridge) {
    asITypeInfo *arrayType = LinkArrayType(bridge);
    if (!arrayType) {
        SetScriptException("array<BehaviorLinkRef@> is not registered.");
        return nullptr;
    }
    return CScriptArray::Create(arrayType, asUINT(0));
}

void AppendNode(CScriptArray *array, BehaviorNode *node) {
    if (!array || !node) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &node);
    node->Release();
}

void AppendLink(CScriptArray *array, BehaviorLinkRef *link) {
    if (!array || !link) {
        return;
    }
    const asUINT index = array->GetSize();
    array->Resize(index + 1);
    array->SetValue(index, &link);
    link->Release();
}

CKBehavior *IoOwner(CKBehaviorIO *io) {
    return io ? io->GetOwner() : nullptr;
}

int SourceOutputIndex(CKBehaviorLink *link) {
    CKBehaviorIO *io = link ? link->GetInBehaviorIO() : nullptr;
    CKBehavior *owner = IoOwner(io);
    if (!owner || !io) {
        return -1;
    }
    int index = owner->GetOutputPosition(io);
    return index >= 0 ? index : owner->GetInputPosition(io);
}

int TargetInputIndex(CKBehaviorLink *link) {
    CKBehaviorIO *io = link ? link->GetOutBehaviorIO() : nullptr;
    CKBehavior *owner = IoOwner(io);
    if (!owner || !io) {
        return -1;
    }
    int index = owner->GetInputPosition(io);
    return index >= 0 ? index : owner->GetOutputPosition(io);
}

CKBehavior *SourceBehavior(CKBehaviorLink *link) {
    return IoOwner(link ? link->GetInBehaviorIO() : nullptr);
}

CKBehavior *TargetBehavior(CKBehaviorLink *link) {
    return IoOwner(link ? link->GetOutBehaviorIO() : nullptr);
}

bool IsDirectChildOf(CKBehavior *container, CKBehavior *behavior) {
    if (!container || !behavior) {
        return false;
    }
    for (int i = 0; i < container->GetSubBehaviorCount(); ++i) {
        if (container->GetSubBehavior(i) == behavior) {
            return true;
        }
    }
    return false;
}

bool ContainsSubBehaviorRecursive(CKBehavior *container, CKBehavior *behavior, std::set<CK_ID> &visited) {
    if (!container || !behavior) {
        return false;
    }
    if (!visited.insert(container->GetID()).second) {
        return false;
    }
    for (int i = 0; i < container->GetSubBehaviorCount(); ++i) {
        CKBehavior *child = container->GetSubBehavior(i);
        if (child == behavior || ContainsSubBehaviorRecursive(child, behavior, visited)) {
            return true;
        }
    }
    return false;
}

bool ContainsSubBehaviorRecursive(CKBehavior *container, CKBehavior *behavior) {
    std::set<CK_ID> visited;
    return ContainsSubBehaviorRecursive(container, behavior, visited);
}

bool LinkTouches(CKBehaviorLink *link, CKBehavior *behavior) {
    return behavior && (SourceBehavior(link) == behavior || TargetBehavior(link) == behavior);
}

bool GraphContainsLink(CKBehavior *container, CKBehaviorLink *link) {
    if (!container || !link) {
        return false;
    }
    for (int i = 0; i < container->GetSubBehaviorLinkCount(); ++i) {
        if (container->GetSubBehaviorLink(i) == link) {
            return true;
        }
    }
    return false;
}

const ScriptBridgeLayoutParamSlot *FindLayoutSlot(const std::vector<ScriptBridgeLayoutParamSlot> &slots, int index) {
    const auto it = std::lower_bound(slots.begin(),
                                     slots.end(),
                                     index,
                                     [](const ScriptBridgeLayoutParamSlot &slot, int value) {
                                         return slot.Index < value;
                                     });
    return it != slots.end() && it->Index == index ? &*it : nullptr;
}

std::string BehaviorIoName(const std::string &prefix, int index, const char *fallback) {
    const char *actualPrefix = prefix.empty() ? fallback : prefix.c_str();
    return fmt::format("{} {}", actualPrefix, index);
}

int BehaviorIoCount(CKBehavior *behavior, bool input) {
    if (!behavior) {
        return 0;
    }
    return input ? behavior->GetInputCount() : behavior->GetOutputCount();
}

CKBehaviorIO *CreateBehaviorIo(CKBehavior *behavior, bool input, const std::string &name) {
    if (!behavior) {
        return nullptr;
    }
    return input ? behavior->CreateInput(const_cast<CKSTRING>(name.c_str()))
                 : behavior->CreateOutput(const_cast<CKSTRING>(name.c_str()));
}

CKBehaviorIO *RemoveLastBehaviorIo(CKBehavior *behavior, bool input) {
    if (!behavior) {
        return nullptr;
    }
    const int count = BehaviorIoCount(behavior, input);
    if (count <= 0) {
        return nullptr;
    }
    return input ? behavior->RemoveInput(count - 1)
                 : behavior->RemoveOutput(count - 1);
}

void NotifyBehaviorLayoutEdited(ScriptBehaviorBridge *bridge, CKBehavior *behavior) {
    if (!behavior) {
        return;
    }
    CallBridgeBehaviorCallback(behavior, CKM_BEHAVIOREDITED);
    if (bridge) {
        bridge->InvalidateBehaviorLayout(behavior->GetID());
    }
    behavior->NotifyEdition();
}

CKParameterLocal *CreateGraphEditInputSource(CKBehavior *behavior,
                                             int pinIndex,
                                             const ScriptParamValue &value,
                                             std::string &error) {
    if (!behavior || pinIndex < 0 || pinIndex >= behavior->GetInputParameterCount()) {
        error = fmt::format("Input parameter index #{} is out of range.", pinIndex);
        return nullptr;
    }

    CKParameterIn *pin = behavior->GetInputParameter(pinIndex);
    if (!pin) {
        error = fmt::format("Input parameter #{} is not valid.", pinIndex);
        return nullptr;
    }

    const std::string name = fmt::format("__CKAS_GraphEditInput_{}_{}", pinIndex, behavior->GetLocalParameterCount());
    CKParameterLocal *local = behavior->CreateLocalParameter(const_cast<CKSTRING>(name.c_str()), pin->GetGUID());
    if (!local) {
        error = fmt::format("Failed to create graph edit literal source for input parameter #{} '{}'.",
                            pinIndex,
                            SafeString(pin->GetName()));
        return nullptr;
    }

    CKERROR err = SetBridgeParamValue(local, value, error);
    if (err != CK_OK) {
        error = fmt::format("Failed to set graph edit literal source for input parameter #{} '{}' (expected {}, got {}, CKERROR {}).",
                            pinIndex,
                            SafeString(pin->GetName()),
                            ParameterTypeLabel(behavior->GetCKContext(), pin),
                            error.empty() ? ScriptParamValueKindName(value.Kind) : error,
                            err);
        const int position = behavior->GetLocalParameterPosition(local);
        if (position >= 0) {
            behavior->RemoveLocalParameter(position);
        }
        behavior->GetCKContext()->DestroyObject(local);
        return nullptr;
    }

    return local;
}

bool IsGraphEditInputSource(CKParameter *parameter) {
    CKParameterLocal *local = CKParameterLocal::Cast(parameter);
    return local && SafeString(local->GetName()).find("__CKAS_GraphEditInput_") == 0;
}

void DestroyGraphEditLocalSource(CKContext *context, CK_ID localId) {
    CKParameterLocal *local = CKParameterLocal::Cast(GetCKObjectById(context, localId));
    if (!local) {
        return;
    }
    if (CKBehavior *owner = CKBehavior::Cast(local->GetOwner())) {
        const int position = owner->GetLocalParameterPosition(local);
        if (position >= 0) {
            owner->RemoveLocalParameter(position);
        }
    }
    if (!local->IsToBeDeleted()) {
        context->DestroyObject(local);
    }
}

CKParameterOperation *OperationFromDirectSource(CKParameter *source, CKBehavior *expectedOwner) {
    CKParameterOperation *operation = source ? CKParameterOperation::Cast(source->GetOwner()) : nullptr;
    return operation && (!expectedOwner || operation->GetOwner() == expectedOwner) ? operation : nullptr;
}

void DestroyOperationLiteralLocal(CKBehavior *owner, CKParameter *source) {
    CKParameterLocal *local = CKParameterLocal::Cast(source);
    if (!owner || !local || CKBehavior::Cast(local->GetOwner()) != owner) {
        return;
    }
    const std::string name = SafeString(local->GetName());
    if (name.find("__CKAS_Op") != 0) {
        return;
    }
    const int position = owner->GetLocalParameterPosition(local);
    if (position >= 0) {
        owner->RemoveLocalParameter(position);
    }
    CKContext *context = owner->GetCKContext();
    if (context && !local->IsToBeDeleted()) {
        context->DestroyObject(local);
    }
}

void DestroyReplacedOperation(CKContext *context, CK_ID operationId) {
    CKParameterOperation *operation = CKParameterOperation::Cast(GetCKObjectById(context, operationId));
    if (!operation) {
        return;
    }
    CKBehavior *owner = operation->GetOwner();
    if (owner) {
        DestroyOperationLiteralLocal(owner, operation->GetInParameter1() ? operation->GetInParameter1()->GetDirectSource() : nullptr);
        DestroyOperationLiteralLocal(owner, operation->GetInParameter2() ? operation->GetInParameter2()->GetDirectSource() : nullptr);
        owner->RemoveParameterOperation(operation);
    }
    if (context && !operation->IsToBeDeleted()) {
        context->DestroyObject(operation);
    }
}

std::string BehaviorLabel(CKBehavior *behavior) {
    if (!behavior) {
        return "<null>";
    }
    CKBeObject *target = behavior->GetTarget();
    return fmt::format("'{}' id={} prototype='{}' target='{}' inputs={} outputs={} pins={} pouts={}",
                       SafeString(behavior->GetName()),
                       behavior->GetID(),
                       SafeString(behavior->GetPrototypeName()),
                       target ? SafeString(target->GetName()) : std::string("<none>"),
                       behavior->GetInputCount(),
                       behavior->GetOutputCount(),
                       behavior->GetInputParameterCount(),
                       behavior->GetOutputParameterCount());
}

bool ContainsText(const std::string &haystack, const std::string &needle) {
    return needle.empty() || haystack.find(needle) != std::string::npos;
}

bool BehaviorPrototypeNameMatches(CKBehavior *behavior, const std::string &name) {
    if (name.empty()) {
        return true;
    }
    if (!behavior) {
        return false;
    }
    if (SafeString(behavior->GetPrototypeName()) == name) {
        return true;
    }
    CKBehaviorPrototype *prototype = CKGetPrototypeFromGuid(behavior->GetPrototypeGuid());
    if (prototype && SafeString(prototype->GetName()) == name) {
        return true;
    }
    CKObjectDeclaration *decl = nullptr;
    const int count = CKGetPrototypeDeclarationCount();
    for (int i = 0; i < count; ++i) {
        CKObjectDeclaration *candidate = CKGetPrototypeDeclaration(i);
        if (candidate && candidate->GetGuid() == behavior->GetPrototypeGuid()) {
            decl = candidate;
            break;
        }
    }
    return decl && (SafeString(decl->GetName()) == name || PrototypeQualifiedName(decl) == name);
}

} // namespace ScriptBridgeGraphInternal

BehaviorQuery::BehaviorQuery() = default;

BehaviorQuery *BehaviorQuery::Name(const std::string &name) {
    m_Name = name;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::NameContains(const std::string &text) {
    m_NameContains = text;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::PrototypeGuid(CKGUID guid) {
    m_PrototypeGuid = guid;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::PrototypeName(const std::string &name) {
    m_PrototypeName = name;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::PrototypeQuery(const std::string &query) {
    CKGUID guid = ScriptBridgeGraphInternal::PrototypeGuidFromQuery(query);
    if (guid.IsValid()) {
        m_PrototypeGuid = guid;
        m_PrototypeName.clear();
    } else {
        m_PrototypeName = query;
    }
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::Target(CKBeObject *target) {
    m_TargetId = target ? target->GetID() : 0;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::TargetName(const std::string &name) {
    m_TargetName = name;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::TargetId(CK_ID id) {
    m_TargetId = id;
    AddRef();
    return this;
}

BehaviorQuery *BehaviorQuery::InputCount(int count) { m_InputCount = count; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::OutputCount(int count) { m_OutputCount = count; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::PinCount(int count) { m_PinCount = count; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::PoutCount(int count) { m_PoutCount = count; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::MaxDepth(int depth) { m_MaxDepth = depth; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::IncludeRoot(bool includeRoot) { m_IncludeRoot = includeRoot; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::Recursive(bool recursive) { m_Recursive = recursive; AddRef(); return this; }
BehaviorQuery *BehaviorQuery::Occurrence(int occurrence) { m_Occurrence = occurrence < 0 ? 0 : occurrence; AddRef(); return this; }

std::string BehaviorQuery::Describe() const {
    return fmt::format("BehaviorQuery(name='{}', contains='{}', prototypeGuid={}, prototypeName='{}', targetId={}, targetName='{}', counts={}/{}/{}/{}, recursive={}, includeRoot={}, maxDepth={}, occurrence={})",
                       m_Name,
                       m_NameContains,
                       GuidToString(m_PrototypeGuid),
                       m_PrototypeName,
                       m_TargetId,
                       m_TargetName,
                       m_InputCount,
                       m_OutputCount,
                       m_PinCount,
                       m_PoutCount,
                       m_Recursive ? "true" : "false",
                       m_IncludeRoot ? "true" : "false",
                       m_MaxDepth,
                       m_Occurrence);
}

bool BehaviorQuery::Matches(CKBehavior *behavior, int depth) const {
    if (!behavior) {
        return false;
    }
    if (m_MaxDepth >= 0 && depth > m_MaxDepth) {
        return false;
    }
    if (!m_Name.empty() && !NameEquals(behavior->GetName(), m_Name)) {
        return false;
    }
    if (!m_NameContains.empty() && !ScriptBridgeGraphInternal::ContainsText(SafeString(behavior->GetName()), m_NameContains)) {
        return false;
    }
    if (m_PrototypeGuid.IsValid() && behavior->GetPrototypeGuid() != m_PrototypeGuid) {
        return false;
    }
    if (!ScriptBridgeGraphInternal::BehaviorPrototypeNameMatches(behavior, m_PrototypeName)) {
        return false;
    }
    CKBeObject *target = behavior->GetTarget();
    if (m_TargetId != 0 && (!target || target->GetID() != m_TargetId)) {
        return false;
    }
    if (!m_TargetName.empty() && (!target || !NameEquals(target->GetName(), m_TargetName))) {
        return false;
    }
    return CountMatches(behavior->GetInputCount(), m_InputCount) &&
           CountMatches(behavior->GetOutputCount(), m_OutputCount) &&
           CountMatches(behavior->GetInputParameterCount(), m_PinCount) &&
           CountMatches(behavior->GetOutputParameterCount(), m_PoutCount);
}

int BehaviorQuery::GetOccurrence() const { return m_Occurrence; }
bool BehaviorQuery::IsRecursive() const { return m_Recursive; }
bool BehaviorQuery::IncludeRootNode() const { return m_IncludeRoot; }
int BehaviorQuery::GetMaxDepth() const { return m_MaxDepth; }

bool BehaviorQuery::CountMatches(int actual, int expected) const {
    return expected < 0 || actual == expected;
}

BehaviorGraph::BehaviorGraph(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CK_ID rootBehaviorId)
    : m_Bridge(bridge), m_Context(ctx), m_RootBehaviorId(rootBehaviorId) {
    m_RootStamp = CaptureBridgeObjectStamp(RootBehavior());
}

bool BehaviorGraph::IsValid() const {
    return RootBehavior() != nullptr;
}

BehaviorNode *BehaviorGraph::Root() const {
    CKBehavior *root = RootBehavior();
    return root ? WrapNode(root) : WrapNode(nullptr, "BehaviorGraph root is not valid.");
}

BehaviorGraphEdit *BehaviorGraph::Edit() const {
    return new BehaviorGraphEdit(m_Bridge, m_Context, m_RootBehaviorId);
}

BehaviorNode *BehaviorGraph::Find(BehaviorQuery *query) const {
    std::vector<CKBehavior *> matches = FindAllRaw(query);
    const int occurrence = query ? query->GetOccurrence() : 0;
    if (occurrence >= 0 && occurrence < static_cast<int>(matches.size())) {
        return WrapNode(matches[occurrence]);
    }
    return nullptr;
}

BehaviorNode *BehaviorGraph::Require(BehaviorQuery *query) const {
    std::vector<CKBehavior *> matches = FindAllRaw(query);
    if (matches.size() == 1) {
        return WrapNode(matches.front());
    }
    const std::string error = fmt::format("BehaviorGraph.Require expected exactly one match, found {}.\n{}",
                                          matches.size(),
                                          DescribeCandidates(query));
    return WrapNode(nullptr, error);
}

CScriptArray *BehaviorGraph::FindAll(BehaviorQuery *query) const {
    CScriptArray *array = ScriptBridgeGraphInternal::CreateNodeArray(m_Bridge);
    if (!array) {
        return nullptr;
    }
    for (CKBehavior *behavior : FindAllRaw(query)) {
        ScriptBridgeGraphInternal::AppendNode(array, WrapNode(behavior));
    }
    return array;
}

std::string BehaviorGraph::DescribeCandidates(BehaviorQuery *query) const {
    std::string text = query ? query->Describe() : std::string("BehaviorQuery(<all>)");
    const std::vector<CKBehavior *> matches = FindAllRaw(query);
    text += fmt::format("\nCandidates: {}", matches.size());
    for (CKBehavior *behavior : matches) {
        text += "\n  " + ScriptBridgeGraphInternal::BehaviorLabel(behavior);
    }
    return text;
}

std::string BehaviorGraph::Describe() const {
    return fmt::format("BehaviorGraph(root={})", ScriptBridgeGraphInternal::BehaviorLabel(RootBehavior()));
}

CKBehavior *BehaviorGraph::RootBehavior() const {
    return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, m_RootBehaviorId, m_RootStamp);
}

ScriptBehaviorBridge *BehaviorGraph::Bridge() const { return m_Bridge; }
CK_ID BehaviorGraph::ComponentId() const { return ComponentIdFromContext(m_Context); }
CK_ID BehaviorGraph::RootId() const { return m_RootBehaviorId; }
const CKBehaviorContext &BehaviorGraph::Context() const { return m_Context; }

std::vector<CKBehavior *> BehaviorGraph::FindAllRaw(BehaviorQuery *query) const {
    std::vector<CKBehavior *> matches;
    CKBehavior *root = RootBehavior();
    if (!root) {
        return matches;
    }

    struct ScanItem {
        CKBehavior *Behavior = nullptr;
        int Depth = 0;
    };
    std::vector<ScanItem> stack;
    if (query && query->IncludeRootNode()) {
        stack.push_back({root, 0});
    }
    const bool recursive = !query || query->IsRecursive();
    const int childDepth = 1;
    for (int i = root->GetSubBehaviorCount() - 1; i >= 0; --i) {
        stack.push_back({root->GetSubBehavior(i), childDepth});
    }

    std::set<CK_ID> visited;
    while (!stack.empty()) {
        const ScanItem item = stack.back();
        stack.pop_back();
        if (!item.Behavior || visited.find(item.Behavior->GetID()) != visited.end()) {
            continue;
        }
        visited.insert(item.Behavior->GetID());
        if (!query || query->Matches(item.Behavior, item.Depth)) {
            matches.push_back(item.Behavior);
        }
        if (!recursive) {
            continue;
        }
        if (query && query->GetMaxDepth() >= 0 && item.Depth >= query->GetMaxDepth()) {
            continue;
        }
        for (int i = item.Behavior->GetSubBehaviorCount() - 1; i >= 0; --i) {
            stack.push_back({item.Behavior->GetSubBehavior(i), item.Depth + 1});
        }
    }
    return matches;
}

BehaviorNode *BehaviorGraph::WrapNode(CKBehavior *behavior, const std::string &error) const {
    return new BehaviorNode(m_Bridge,
                            m_Context,
                            m_RootBehaviorId,
                            behavior ? behavior->GetID() : 0,
                            ComponentIdFromContext(m_Context),
                            error);
}

GraphEditResult::GraphEditResult(ScriptBehaviorBridge *bridge,
                                 const CKBehaviorContext &ctx,
                                 bool ok,
                                 const std::string &error,
                                 const std::string &description,
                                 const std::vector<CK_ID> &createdNodeIds,
                                 const std::vector<CK_ID> &createdLinkIds)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_Ok(ok),
      m_Error(error),
      m_Description(description),
      m_CreatedNodeIds(createdNodeIds),
      m_CreatedLinkIds(createdLinkIds) {}

bool GraphEditResult::Ok() const { return m_Ok; }
bool GraphEditResult::IsOk() const { return m_Ok; }
std::string GraphEditResult::Error() const { return m_Error; }

std::string GraphEditResult::Describe() const {
    if (!m_Description.empty()) {
        return m_Description;
    }
    return m_Ok ? "Graph edit succeeded." : m_Error;
}

CScriptArray *GraphEditResult::CreatedNodes() const {
    CScriptArray *array = ScriptBridgeGraphInternal::CreateNodeArray(m_Bridge);
    if (!array) {
        return nullptr;
    }
    CK_ID rootId = m_Context.Behavior ? m_Context.Behavior->GetID() : 0;
    for (CK_ID id : m_CreatedNodeIds) {
        ScriptBridgeGraphInternal::AppendNode(array, new BehaviorNode(m_Bridge, m_Context, rootId, id, ComponentIdFromContext(m_Context)));
    }
    return array;
}

CScriptArray *GraphEditResult::CreatedLinks() const {
    CScriptArray *array = ScriptBridgeGraphInternal::CreateLinkArray(m_Bridge);
    if (!array) {
        return nullptr;
    }
    CK_ID rootId = m_Context.Behavior ? m_Context.Behavior->GetID() : 0;
    for (CK_ID id : m_CreatedLinkIds) {
        ScriptBridgeGraphInternal::AppendLink(array, new BehaviorLinkRef(m_Bridge, rootId, id, ComponentIdFromContext(m_Context)));
    }
    return array;
}

bool GraphEditResult::Raise(const CKBehaviorContext &ctx) const {
    if (m_Ok) {
        return true;
    }
    ScriptBridgeExecutionState state;
    state.Ok = false;
    state.ReturnCode = CKBR_BEHAVIORERROR;
    state.Error = m_Error.empty() ? "Graph edit failed." : m_Error;
    return RaiseExecutionState(state, ctx);
}

GraphEditNode::GraphEditNode(BehaviorGraphEdit *edit, int specIndex, CK_ID behaviorId, const std::string &error)
    : m_Edit(edit), m_SpecIndex(specIndex), m_BehaviorId(behaviorId), m_Error(error) {
    if (m_Edit) {
        m_Edit->AddRef();
    }
}

GraphEditNode::~GraphEditNode() {
    if (m_Edit) {
        m_Edit->Release();
        m_Edit = nullptr;
    }
}

bool GraphEditNode::IsValid() const {
    return m_Edit && m_Error.empty() && m_Edit->IsNodeValid(this);
}

std::string GraphEditNode::Error() const { return m_Error; }

BehaviorRef *GraphEditNode::Behavior() const {
    return m_Edit ? m_Edit->Bridge()->WrapBehavior(m_Edit->ResolveNode(this), ComponentIdFromContext(m_Edit->Context())) : nullptr;
}

std::string GraphEditNode::Describe() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    CKBehavior *behavior = m_Edit ? m_Edit->ResolveNode(this) : nullptr;
    if (behavior) {
        return ScriptBridgeGraphInternal::BehaviorLabel(behavior);
    }
    return fmt::format("GraphEditNode(pending #{})", m_SpecIndex);
}

BehaviorGraphEdit *GraphEditNode::EditOwner() const { return m_Edit; }
int GraphEditNode::SpecIndex() const { return m_SpecIndex; }
CK_ID GraphEditNode::BehaviorId() const { return m_BehaviorId; }

GraphEditLink::GraphEditLink(BehaviorGraphEdit *edit, int specIndex, CK_ID linkId, const std::string &error)
    : m_Edit(edit), m_SpecIndex(specIndex), m_LinkId(linkId), m_Error(error) {
    if (m_Edit) {
        m_Edit->AddRef();
    }
}

GraphEditLink::~GraphEditLink() {
    if (m_Edit) {
        m_Edit->Release();
        m_Edit = nullptr;
    }
}

bool GraphEditLink::IsValid() const {
    return m_Edit && m_Error.empty() && (m_SpecIndex >= 0 || m_LinkId != 0);
}

std::string GraphEditLink::Error() const { return m_Error; }

BehaviorLinkRef *GraphEditLink::Link() const {
    return m_Edit ? m_Edit->ResolveLink(this) : nullptr;
}

std::string GraphEditLink::Describe() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    BehaviorLinkRef *link = Link();
    if (link) {
        const std::string text = link->Describe();
        link->Release();
        return text;
    }
    return fmt::format("GraphEditLink(pending #{})", m_SpecIndex);
}

BehaviorGraphEdit *GraphEditLink::EditOwner() const { return m_Edit; }
int GraphEditLink::SpecIndex() const { return m_SpecIndex; }
CK_ID GraphEditLink::LinkId() const { return m_LinkId; }

BehaviorGraphEdit::BehaviorGraphEdit(ScriptBehaviorBridge *bridge, const CKBehaviorContext &ctx, CK_ID rootBehaviorId)
    : m_Bridge(bridge), m_Context(ctx), m_RootBehaviorId(rootBehaviorId) {
    m_RootStamp = CaptureBridgeObjectStamp(RootBehavior());
}

BehaviorGraphEdit::~BehaviorGraphEdit() = default;

bool BehaviorGraphEdit::IsValid() const {
    return RootBehavior() != nullptr && m_Error.empty();
}

std::string BehaviorGraphEdit::Error() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    return RootBehavior() ? std::string() : "BehaviorGraphEdit root graph is not valid.";
}

std::string BehaviorGraphEdit::Describe() const {
    return fmt::format("BehaviorGraphEdit(root={}, nodes={}, links={}, removes={}, moves={}, layoutEdits={})",
                       ScriptBridgeGraphInternal::BehaviorLabel(RootBehavior()),
                       m_Nodes.size(),
                       m_Links.size(),
                       m_Removes.size(),
                       m_Moves.size(),
                       m_LayoutEdits.size());
}

GraphEditNode *BehaviorGraphEdit::Import(BehaviorNode *node) {
    if (!node || !node->IsValid()) {
        const std::string error = node ? node->Error() : "BehaviorGraphEdit.Import requires a valid BehaviorNode.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }
    if (node->RootId() != m_RootBehaviorId) {
        const std::string error = "BehaviorGraphEdit.Import requires a node from the same BehaviorGraph.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }

    NodeSpec spec;
    spec.Type = NodeSpec::Kind::Existing;
    spec.BehaviorId = node->BehaviorId();
    spec.BehaviorStamp = CaptureBridgeObjectStamp(node->Get());
    const int index = static_cast<int>(m_Nodes.size());
    m_Nodes.push_back(spec);
    return new GraphEditNode(this, index, spec.BehaviorId);
}

GraphEditNode *BehaviorGraphEdit::Clone(BehaviorNode *node, const std::string &name) {
    if (!node || !node->IsValid()) {
        const std::string error = node ? node->Error() : "BehaviorGraphEdit.Clone requires a valid BehaviorNode.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }
    if (node->RootId() != m_RootBehaviorId) {
        const std::string error = "BehaviorGraphEdit.Clone requires a node from the same BehaviorGraph.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }

    NodeSpec spec;
    spec.Type = NodeSpec::Kind::Clone;
    spec.BehaviorId = node->BehaviorId();
    spec.BehaviorStamp = CaptureBridgeObjectStamp(node->Get());
    CKBehavior *source = node->Get();
    spec.Name = name.empty() && source ? SafeString(source->GetName()) : name;
    const int index = static_cast<int>(m_Nodes.size());
    m_Nodes.push_back(spec);
    return new GraphEditNode(this, index, 0);
}

GraphEditNode *BehaviorGraphEdit::AddDecl(BBDecl *decl, const std::string &name) {
    if (!decl || !decl->IsValid()) {
        const std::string error = decl ? decl->Error() : "BehaviorGraphEdit.Add requires a valid BBDecl.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }

    NodeSpec spec;
    spec.Type = NodeSpec::Kind::Create;
    spec.Request = decl->Request();
    spec.Name = name.empty() ? decl->Name() : name;
    const int index = static_cast<int>(m_Nodes.size());
    m_Nodes.push_back(spec);
    return new GraphEditNode(this, index, 0);
}

GraphEditNode *BehaviorGraphEdit::AddConfig(BBConfig *config, const std::string &name) {
    if (!config || !config->IsValid()) {
        const std::string error = config ? config->Error() : "BehaviorGraphEdit.Add requires a valid BBConfig.";
        SetError(error);
        return new GraphEditNode(this, -1, 0, error);
    }

    NodeSpec spec;
    spec.Type = NodeSpec::Kind::Create;
    spec.Request = config->Request();
    BBDecl *decl = config->Decl();
    spec.Name = name.empty() && decl ? decl->Name() : name;
    if (decl) {
        decl->Release();
    }
    const int index = static_cast<int>(m_Nodes.size());
    m_Nodes.push_back(spec);
    return new GraphEditNode(this, index, 0);
}

BehaviorGraphEdit *BehaviorGraphEdit::Remove(BehaviorNode *node, bool removeIncidentLinks) {
    GraphEditNode *imported = Import(node);
    int index = -1;
    std::string error;
    if (!ResolveNodeIndex(imported, index, error)) {
        SetError(error);
        if (imported) imported->Release();
        AddRef();
        return this;
    }
    if (imported) {
        imported->Release();
    }
    m_Removes.push_back(RemoveSpec{index, removeIncidentLinks});
    AddRef();
    return this;
}

BehaviorGraphEdit *BehaviorGraphEdit::Move(BehaviorNode *node, BehaviorGraph *targetGraph) {
    GraphEditNode *imported = Import(node);
    int index = -1;
    std::string error;
    if (!ResolveNodeIndex(imported, index, error)) {
        SetError(error);
    } else if (!targetGraph || !targetGraph->IsValid()) {
        SetError("BehaviorGraphEdit.Move requires a valid target BehaviorGraph.");
    } else {
        CKBehavior *targetRoot = targetGraph->RootBehavior();
        m_Moves.push_back(MoveSpec{index, targetGraph->RootId(), CaptureBridgeObjectStamp(targetRoot)});
    }
    if (imported) {
        imported->Release();
    }
    AddRef();
    return this;
}

BehaviorGraphEdit *BehaviorGraphEdit::EnsureInputCount(GraphEditNode *node, int count, const std::string &prefix) {
    return EnsureIoCount(node, LayoutSpec::Kind::EnsureInputCount, count, prefix, "BehaviorGraphEdit.EnsureInputCount");
}

BehaviorGraphEdit *BehaviorGraphEdit::EnsureOutputCount(GraphEditNode *node, int count, const std::string &prefix) {
    return EnsureIoCount(node, LayoutSpec::Kind::EnsureOutputCount, count, prefix, "BehaviorGraphEdit.EnsureOutputCount");
}

BehaviorGraphEdit *BehaviorGraphEdit::Target(GraphEditNode *node, CKBeObject *target) {
    int nodeIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(node, nodeIndex, error)) {
        SetError(error);
    } else if (!target) {
        SetError("BehaviorGraphEdit.Target requires a valid target object.");
    } else {
        m_Targets.push_back(TargetSpec{nodeIndex, target->GetID(), CaptureBridgeObjectStamp(target)});
    }
    AddRef();
    return this;
}

GraphEditLink *BehaviorGraphEdit::Link(GraphEditNode *source,
                                       int sourceOutputIndex,
                                       GraphEditNode *target,
                                       int targetInputIndex,
                                       int delay) {
    int sourceIndex = -1;
    int targetIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(source, sourceIndex, error) || !ResolveNodeIndex(target, targetIndex, error)) {
        SetError(error);
        return new GraphEditLink(this, -1, 0, error);
    }

    LinkSpec spec;
    spec.Type = LinkSpec::Kind::Create;
    spec.SourceNodeIndex = sourceIndex;
    spec.TargetNodeIndex = targetIndex;
    spec.SourceOutputIndex = sourceOutputIndex;
    spec.TargetInputIndex = targetInputIndex;
    spec.Delay = delay < 0 ? 0 : delay;
    const int index = static_cast<int>(m_Links.size());
    m_Links.push_back(spec);
    return new GraphEditLink(this, index, 0);
}

GraphEditLink *BehaviorGraphEdit::LinkSlots(GraphEditNode *source,
                                            BBSlot *sourceOutput,
                                            GraphEditNode *target,
                                            BBSlot *targetInput,
                                            int delay) {
    int sourceIndex = -1;
    int targetIndex = -1;
    std::string error;
    if (!sourceOutput || !sourceOutput->ResolveIndex(ScriptBridgeSlotKind::Output, sourceIndex, error)) {
        SetError(error.empty() ? "BehaviorGraphEdit.Link requires an output slot." : error);
        return new GraphEditLink(this, -1, 0, Error());
    }
    if (!targetInput || !targetInput->ResolveIndex(ScriptBridgeSlotKind::Input, targetIndex, error)) {
        SetError(error.empty() ? "BehaviorGraphEdit.Link requires an input slot." : error);
        return new GraphEditLink(this, -1, 0, Error());
    }
    return Link(source, sourceIndex, target, targetIndex, delay);
}

BehaviorGraphEdit *BehaviorGraphEdit::Unlink(BehaviorLinkRef *link) {
    if (!link || !link->IsValid()) {
        SetError("BehaviorGraphEdit.Unlink requires a valid BehaviorLinkRef.");
        AddRef();
        return this;
    }
    if (link->RootId() != m_RootBehaviorId) {
        SetError("BehaviorGraphEdit.Unlink requires a link from the same BehaviorGraph.");
        AddRef();
        return this;
    }

    LinkSpec spec;
    spec.Type = LinkSpec::Kind::Unlink;
    spec.ExistingLinkId = link->LinkId();
    CKContext *context = m_Bridge && m_Bridge->GetManager() ? m_Bridge->GetManager()->GetCKContext() : nullptr;
    spec.ExistingLinkStamp = CaptureBridgeObjectStamp(GetCKObjectById(context, spec.ExistingLinkId));
    m_Links.push_back(spec);
    AddRef();
    return this;
}

GraphEditLink *BehaviorGraphEdit::Relink(BehaviorLinkRef *link,
                                         GraphEditNode *source,
                                         int sourceOutputIndex,
                                         GraphEditNode *target,
                                         int targetInputIndex,
                                         int delay) {
    Unlink(link)->Release();
    return Link(source, sourceOutputIndex, target, targetInputIndex, delay);
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlot(GraphEditNode *node, BBSlot *pin, ParamValue *value) {
    if (!value) {
        SetError("BehaviorGraphEdit.Set requires a valid ParamValue.");
        AddRef();
        return this;
    }
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, value->Value(), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlotInt(GraphEditNode *node, BBSlot *pin, int value) {
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, MakeScriptParamInt(value), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlotFloat(GraphEditNode *node, BBSlot *pin, float value) {
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, MakeScriptParamFloat(value), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlotBool(GraphEditNode *node, BBSlot *pin, bool value) {
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, MakeScriptParamBool(value), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlotString(GraphEditNode *node, BBSlot *pin, const std::string &value) {
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, MakeScriptParamString(value), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSlotObject(GraphEditNode *node, BBSlot *pin, CKObject *value) {
    return SetValue(node, pin, ScriptBridgeSlotKind::Pin, MakeScriptParamObject(value), "BehaviorGraphEdit.Set");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSetting(GraphEditNode *node, BBSlot *setting, ParamValue *value) {
    if (!value) {
        SetError("BehaviorGraphEdit.SetSetting requires a valid ParamValue.");
        AddRef();
        return this;
    }
    return SetValue(node, setting, ScriptBridgeSlotKind::Setting, value->Value(), "BehaviorGraphEdit.SetSetting");
}

BehaviorGraphEdit *BehaviorGraphEdit::SetSettingString(GraphEditNode *node, BBSlot *setting, const std::string &value) {
    return SetValue(node, setting, ScriptBridgeSlotKind::Setting, MakeScriptParamString(value), "BehaviorGraphEdit.SetSetting");
}

BehaviorGraphEdit *BehaviorGraphEdit::Source(GraphEditNode *node, BBSlot *pin, ParamRef *source) {
    int nodeIndex = -1;
    int pinIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(node, nodeIndex, error)) {
        SetError(error);
    } else if (!pin || !pin->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetError(error.empty() ? "BehaviorGraphEdit.Source requires a pin slot." : error);
    } else if (!source || !source->IsValid()) {
        SetError("BehaviorGraphEdit.Source requires a valid ParamRef source.");
    } else {
        RemoveNodeValue(nodeIndex, ScriptBridgeSlotKind::Pin, pinIndex);
        RemoveNodeOperation(nodeIndex, pinIndex);
        SourceSpec spec;
        spec.NodeIndex = nodeIndex;
        spec.PinIndex = pinIndex;
        spec.SourceId = source->GetID();
        spec.SourceStamp = source->Stamp();
        RemoveNodeSource(nodeIndex, pinIndex);
        if (m_Nodes[nodeIndex].Type == NodeSpec::Kind::Create) {
            ScriptBridgeInputSource request;
            request.PinIndex = pinIndex;
            request.SourceId = spec.SourceId;
            request.SourceStamp = spec.SourceStamp;
            SetRequestSource(m_Nodes[nodeIndex].Request, request);
        } else {
            m_Sources.push_back(spec);
        }
    }
    AddRef();
    return this;
}

BehaviorGraphEdit *BehaviorGraphEdit::Operation(GraphEditNode *node, BBSlot *pin, ParamOp *operation) {
    int nodeIndex = -1;
    int pinIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(node, nodeIndex, error)) {
        SetError(error);
    } else if (!pin || !pin->ResolveIndex(ScriptBridgeSlotKind::Pin, pinIndex, error)) {
        SetError(error.empty() ? "BehaviorGraphEdit.Operation requires a pin slot." : error);
    } else if (!operation) {
        SetError("BehaviorGraphEdit.Operation requires a valid ParamOp.");
    } else {
        RemoveNodeValue(nodeIndex, ScriptBridgeSlotKind::Pin, pinIndex);
        RemoveNodeSource(nodeIndex, pinIndex);
        OperationSpec spec;
        spec.NodeIndex = nodeIndex;
        spec.Operation = operation->RequestForPin(pinIndex);
        RemoveNodeOperation(nodeIndex, pinIndex);
        if (m_Nodes[nodeIndex].Type == NodeSpec::Kind::Create) {
            SetRequestOperation(m_Nodes[nodeIndex].Request, spec.Operation);
        } else {
            m_Operations.push_back(spec);
        }
    }
    AddRef();
    return this;
}

GraphEditResult *BehaviorGraphEdit::Validate(const CKBehaviorContext &ctx) const {
    std::string error;
    if (!ValidateInternal(ctx, error)) {
        return MakeResult(false, error, error);
    }
    return MakeResult(true, std::string(), "Graph edit validation succeeded.");
}

GraphEditResult *BehaviorGraphEdit::Apply(const CKBehaviorContext &ctx) {
    std::string error;
    if (!ValidateInternal(ctx, error)) {
        SetError(error);
        return MakeResult(false, error, error);
    }

    CKContext *context = ScriptBridgeGraphInternal::GraphContext(m_Bridge, ctx);
    CKBehavior *root = RootBehavior();
    std::vector<CK_ID> createdNodes;
    std::vector<CK_ID> createdLinks;
    std::vector<CK_ID> createdOperations;
    std::vector<ParamSourceLinkRef *> appliedSourceLinks;
    std::vector<ParamOperationRef *> appliedOperations;
    std::vector<CK_ID> createdValueLocalSources;
    std::vector<CK_ID> replacedValueLocalSources;
    std::vector<CK_ID> replacedOperations;
    struct AppliedLayoutEdit {
        CK_ID BehaviorId = 0;
        LayoutSpec::Kind Type = LayoutSpec::Kind::EnsureInputCount;
        int OriginalCount = 0;
    };
    struct AppliedTargetEdit {
        CK_ID BehaviorId = 0;
        CK_ID LocalSourceId = 0;
        CK_ID PreviousSourceId = 0;
        ScriptBridgeObjectStamp PreviousSourceStamp;
    };
    std::vector<AppliedLayoutEdit> appliedLayoutEdits;
    std::vector<AppliedTargetEdit> appliedTargetEdits;

    auto rollbackCreated = [&]() {
        for (ParamOperationRef *operation : appliedOperations) {
            if (operation) {
                operation->Destroy();
                operation->Release();
            }
        }
        appliedOperations.clear();
        for (ParamSourceLinkRef *link : appliedSourceLinks) {
            if (link) {
                link->Restore();
                link->Release();
            }
        }
        appliedSourceLinks.clear();
        for (CK_ID localId : createdValueLocalSources) {
            ScriptBridgeGraphInternal::DestroyGraphEditLocalSource(context, localId);
        }
        createdValueLocalSources.clear();
        for (CK_ID linkId : createdLinks) {
            if (CKBehaviorLink *link = CKBehaviorLink::Cast(GetCKObjectById(context, linkId))) {
                if (root) {
                    root->RemoveSubBehaviorLink(link);
                }
                context->DestroyObject(link);
            }
        }
        for (auto it = appliedLayoutEdits.rbegin(); it != appliedLayoutEdits.rend(); ++it) {
            CKBehavior *behavior = CKBehavior::Cast(GetCKObjectById(context, it->BehaviorId));
            if (!behavior) {
                continue;
            }
            const bool input = it->Type == LayoutSpec::Kind::EnsureInputCount;
            while (ScriptBridgeGraphInternal::BehaviorIoCount(behavior, input) > it->OriginalCount) {
                CKBehaviorIO *removed = ScriptBridgeGraphInternal::RemoveLastBehaviorIo(behavior, input);
                if (removed) {
                    context->DestroyObject(removed);
                } else {
                    break;
                }
            }
            ScriptBridgeGraphInternal::NotifyBehaviorLayoutEdited(m_Bridge, behavior);
        }
        appliedLayoutEdits.clear();
        for (auto it = appliedTargetEdits.rbegin(); it != appliedTargetEdits.rend(); ++it) {
            CKBehavior *behavior = CKBehavior::Cast(GetCKObjectById(context, it->BehaviorId));
            CKParameterIn *targetParam = behavior ? behavior->GetTargetParameter() : nullptr;
            if (targetParam) {
                CKParameter *previous = it->PreviousSourceId
                    ? CKParameter::Cast(GetStampedCKObjectById(context, it->PreviousSourceId, it->PreviousSourceStamp))
                    : nullptr;
                targetParam->SetDirectSource(previous);
            }
            if (CKObject *local = GetCKObjectById(context, it->LocalSourceId)) {
                context->DestroyObject(local);
            }
        }
        appliedTargetEdits.clear();
        for (CK_ID behaviorId : createdNodes) {
            if (CKBehavior *behavior = CKBehavior::Cast(GetCKObjectById(context, behaviorId))) {
                if (root) {
                    root->RemoveSubBehavior(behavior);
                }
                context->DestroyObject(behavior);
            }
        }
        for (CK_ID operationId : createdOperations) {
            if (CKObject *operation = GetCKObjectById(context, operationId)) {
                context->DestroyObject(operation);
            }
        }
    };

    auto failApply = [&](const std::string &message) -> GraphEditResult * {
        rollbackCreated();
        SetError(message);
        return MakeResult(false, message, message);
    };

    for (int i = 0; i < static_cast<int>(m_Nodes.size()); ++i) {
        NodeSpec &spec = m_Nodes[i];
        if (spec.Type != NodeSpec::Kind::Create && spec.Type != NodeSpec::Kind::Clone) {
            continue;
        }
        std::vector<CK_ID> operationIds;
        CKBehavior *behavior = nullptr;
        if (spec.Type == NodeSpec::Kind::Create) {
            behavior = m_Bridge->CreatePersistentBehavior(spec.Request, ctx, spec.Name, error, &operationIds);
        } else {
            CKBehavior *source = ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.BehaviorId, spec.BehaviorStamp);
            if (!source) {
                return failApply("BehaviorGraphEdit.Clone source behavior is no longer valid.");
            }
            behavior = CKBehavior::Cast(context->CopyObject(source, nullptr, nullptr, CK_OBJECTCREATION_DYNAMIC));
            if (!behavior) {
                return failApply(fmt::format("Failed to clone behavior '{}'.", SafeString(source->GetName())));
            }
            if (!spec.Name.empty()) {
                behavior->SetName(const_cast<CKSTRING>(spec.Name.c_str()), TRUE);
            }
        }
        if (!behavior) {
            return failApply(error);
        }
        createdOperations.insert(createdOperations.end(), operationIds.begin(), operationIds.end());
        CKERROR err = root->AddSubBehavior(behavior);
        if (err != CK_OK) {
            context->DestroyObject(behavior);
            error = fmt::format("Failed to add behavior '{}' to graph '{}' (CKERROR {}).",
                                SafeString(behavior->GetName()),
                                SafeString(root->GetName()),
                                err);
            return failApply(error);
        }
        spec.CreatedBehaviorId = behavior->GetID();
        spec.CreatedBehaviorStamp = CaptureBridgeObjectStamp(behavior);
        createdNodes.push_back(behavior->GetID());
    }

    for (const LayoutSpec &spec : m_LayoutEdits) {
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!behavior) {
            return failApply("BehaviorGraphEdit layout edit target behavior is not valid.");
        }
        const bool input = spec.Type == LayoutSpec::Kind::EnsureInputCount;
        const char *fallbackPrefix = input ? "In" : "Out";
        const int originalCount = ScriptBridgeGraphInternal::BehaviorIoCount(behavior, input);
        appliedLayoutEdits.push_back(AppliedLayoutEdit{behavior->GetID(), spec.Type, originalCount});
        for (int i = originalCount; i < spec.Count; ++i) {
            const std::string name = ScriptBridgeGraphInternal::BehaviorIoName(spec.Prefix, i, fallbackPrefix);
            if (!ScriptBridgeGraphInternal::CreateBehaviorIo(behavior, input, name)) {
                error = fmt::format("Failed to create {} #{} '{}' on {}.",
                                    input ? "input" : "output",
                                    i,
                                    name,
                                    ScriptBridgeGraphInternal::BehaviorLabel(behavior));
                return failApply(error);
            }
        }
        ScriptBridgeGraphInternal::NotifyBehaviorLayoutEdited(m_Bridge, behavior);
    }

    for (const TargetSpec &spec : m_Targets) {
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        CKBeObject *target = CKBeObject::Cast(GetStampedCKObjectById(context, spec.TargetId, spec.TargetStamp));
        if (!behavior || !target) {
            return failApply("BehaviorGraphEdit.Target behavior or target is no longer valid.");
        }
        if (!behavior->IsTargetable()) {
            return failApply(fmt::format("BehaviorGraphEdit.Target cannot target non-targetable behavior '{}'.",
                                         SafeString(behavior->GetName())));
        }
        if (!CKIsChildClassOf(target, behavior->GetCompatibleClassID())) {
            return failApply(fmt::format("BehaviorGraphEdit.Target target '{}' is not compatible with '{}'.",
                                         SafeString(target->GetName()),
                                         SafeString(behavior->GetName())));
        }
        CKERROR err = behavior->UseTarget(TRUE);
        if (err != CK_OK) {
            return failApply(fmt::format("BehaviorGraphEdit.Target failed to enable target on '{}' (CKERROR {}).",
                                         SafeString(behavior->GetName()),
                                         err));
        }
        CKParameterIn *targetParam = behavior->GetTargetParameter();
        if (!targetParam) {
            return failApply(fmt::format("BehaviorGraphEdit.Target behavior '{}' has no target parameter.",
                                         SafeString(behavior->GetName())));
        }
        CKParameter *previous = targetParam->GetDirectSource();
        CKParameterLocal *targetSource = behavior->CreateLocalParameter(const_cast<CKSTRING>("__CKAS_GraphEdit_Target"),
                                                                        targetParam->GetGUID());
        if (!targetSource) {
            return failApply("BehaviorGraphEdit.Target failed to create target source parameter.");
        }
        CK_ID targetId = target->GetID();
        if (targetSource->SetValue(&targetId, sizeof(targetId)) != CK_OK ||
            targetParam->SetDirectSource(targetSource) != CK_OK) {
            context->DestroyObject(targetSource);
            return failApply("BehaviorGraphEdit.Target failed to assign target parameter.");
        }
        appliedTargetEdits.push_back(AppliedTargetEdit{
            behavior->GetID(),
            targetSource->GetID(),
            previous ? previous->GetID() : 0,
            CaptureBridgeObjectStamp(previous)
        });
        if (m_Bridge) {
            m_Bridge->InvalidateBehaviorLayout(behavior->GetID());
        }
    }

    for (LinkSpec &spec : m_Links) {
        if (spec.Type != LinkSpec::Kind::Create) {
            continue;
        }
        CKBehavior *source = ResolveNodeBehavior(spec.SourceNodeIndex);
        CKBehavior *target = ResolveNodeBehavior(spec.TargetNodeIndex);
        CKBehaviorIO *sourceIo = source ? source->GetOutput(spec.SourceOutputIndex) : nullptr;
        CKBehaviorIO *targetIo = target ? target->GetInput(spec.TargetInputIndex) : nullptr;
        CKBehaviorLink *link = CKBehaviorLink::Cast(context->CreateObject(CKCID_BEHAVIORLINK,
                                                                          const_cast<CKSTRING>("__CKAS_GraphEdit_Link"),
                                                                          CK_OBJECTCREATION_NONAMECHECK));
        if (!link || !sourceIo || !targetIo) {
            if (link) {
                context->DestroyObject(link);
            }
            error = "Failed to create behavior link: source or target IO is not valid.";
            return failApply(error);
        }
        CKERROR err = link->SetInBehaviorIO(sourceIo);
        if (err == CK_OK) {
            err = link->SetOutBehaviorIO(targetIo);
        }
        if (err == CK_OK) {
            link->SetInitialActivationDelay(spec.Delay);
            link->SetActivationDelay(spec.Delay);
            err = root->AddSubBehaviorLink(link);
        }
        if (err != CK_OK) {
            context->DestroyObject(link);
            error = fmt::format("Failed to add behavior link {}#{} -> {}#{} (CKERROR {}).",
                                ScriptBridgeGraphInternal::BehaviorLabel(source),
                                spec.SourceOutputIndex,
                                ScriptBridgeGraphInternal::BehaviorLabel(target),
                                spec.TargetInputIndex,
                                err);
            return failApply(error);
        }
        spec.CreatedLinkId = link->GetID();
        createdLinks.push_back(link->GetID());
    }

    for (const ValueSpec &spec : m_Values) {
        if (spec.Kind == ScriptBridgeSlotKind::Setting) {
            continue;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!ApplyExistingValue(behavior, spec, error, appliedSourceLinks, createdValueLocalSources, replacedValueLocalSources, replacedOperations)) {
            return failApply(error);
        }
    }

    for (const SourceSpec &spec : m_Sources) {
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!ApplyExistingSource(behavior, spec, error, appliedSourceLinks, replacedValueLocalSources, replacedOperations)) {
            return failApply(error);
        }
    }

    for (const OperationSpec &spec : m_Operations) {
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!ApplyExistingOperation(behavior, spec, error, appliedOperations, replacedOperations)) {
            return failApply(error);
        }
    }

    for (const ValueSpec &spec : m_Values) {
        if (spec.Kind != ScriptBridgeSlotKind::Setting) {
            continue;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!ApplyExistingValue(behavior, spec, error, appliedSourceLinks, createdValueLocalSources, replacedValueLocalSources, replacedOperations)) {
            return failApply(error);
        }
    }

    for (const LinkSpec &spec : m_Links) {
        if (spec.Type != LinkSpec::Kind::Unlink) {
            continue;
        }
        CKBehaviorLink *link = ResolveExistingLink(spec);
        if (link) {
            root->RemoveSubBehaviorLink(link);
            context->DestroyObject(link);
        }
    }

    for (const MoveSpec &move : m_Moves) {
        CKBehavior *behavior = ResolveNodeBehavior(move.NodeIndex);
        CKBehavior *targetRoot = ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, move.TargetRootId, move.TargetRootStamp);
        if (!behavior || !targetRoot) {
            continue;
        }
        root->RemoveSubBehavior(behavior);
        CKERROR err = targetRoot->AddSubBehavior(behavior);
        if (err != CK_OK) {
            root->AddSubBehavior(behavior);
            error = fmt::format("Failed to move behavior '{}' into graph '{}' (CKERROR {}).",
                                SafeString(behavior->GetName()),
                                SafeString(targetRoot->GetName()),
                                err);
            return failApply(error);
        }
        if (m_Bridge) {
            m_Bridge->InvalidateBehaviorLayout(targetRoot->GetID());
        }
        targetRoot->NotifyEdition();
    }

    for (const RemoveSpec &remove : m_Removes) {
        CKBehavior *behavior = ResolveNodeBehavior(remove.NodeIndex);
        if (!behavior) {
            continue;
        }
        if (remove.RemoveIncidentLinks) {
            std::vector<CKBehaviorLink *> incident;
            for (int i = 0; i < root->GetSubBehaviorLinkCount(); ++i) {
                CKBehaviorLink *link = root->GetSubBehaviorLink(i);
                if (ScriptBridgeGraphInternal::LinkTouches(link, behavior)) {
                    incident.push_back(link);
                }
            }
            for (CKBehaviorLink *link : incident) {
                root->RemoveSubBehaviorLink(link);
                context->DestroyObject(link);
            }
        }
        root->RemoveSubBehavior(behavior);
        context->DestroyObject(behavior);
    }

    if (m_Bridge) {
        m_Bridge->InvalidateBehaviorLayout(root->GetID());
    }
    root->NotifyEdition();
    for (ParamSourceLinkRef *link : appliedSourceLinks) {
        if (link) {
            link->Commit();
            link->Release();
        }
    }
    appliedSourceLinks.clear();
    for (CK_ID localId : replacedValueLocalSources) {
        ScriptBridgeGraphInternal::DestroyGraphEditLocalSource(context, localId);
    }
    replacedValueLocalSources.clear();
    for (CK_ID operationId : replacedOperations) {
        ScriptBridgeGraphInternal::DestroyReplacedOperation(context, operationId);
    }
    replacedOperations.clear();
    for (ParamOperationRef *operation : appliedOperations) {
        if (operation) {
            operation->Release();
        }
    }
    appliedOperations.clear();
    createdValueLocalSources.clear();
    m_Applied = true;
    m_Error.clear();
    return MakeResult(true,
                      std::string(),
                      fmt::format("Graph edit applied: createdNodes={} createdLinks={} removedNodes={} moves={}.",
                                  createdNodes.size(),
                                  createdLinks.size(),
                                  m_Removes.size(),
                                  m_Moves.size()),
                      createdNodes,
                      createdLinks);
}

ScriptBehaviorBridge *BehaviorGraphEdit::Bridge() const { return m_Bridge; }
const CKBehaviorContext &BehaviorGraphEdit::Context() const { return m_Context; }
CK_ID BehaviorGraphEdit::RootId() const { return m_RootBehaviorId; }

CKBehavior *BehaviorGraphEdit::RootBehavior() const {
    return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, m_RootBehaviorId, m_RootStamp);
}

CKBehavior *BehaviorGraphEdit::ResolveNode(const GraphEditNode *node) const {
    int index = -1;
    std::string error;
    if (!ResolveNodeIndex(node, index, error)) {
        return nullptr;
    }
    return ResolveNodeBehavior(index);
}

bool BehaviorGraphEdit::IsNodeValid(const GraphEditNode *node) const {
    int index = -1;
    std::string error;
    if (!ResolveNodeIndex(node, index, error)) {
        return false;
    }
    const NodeSpec &spec = m_Nodes[index];
    if (spec.Type == NodeSpec::Kind::Create && !spec.CreatedBehaviorId) {
        return true;
    }
    if (spec.Type == NodeSpec::Kind::Clone && !spec.CreatedBehaviorId) {
        return true;
    }
    return ResolveNodeBehavior(index) != nullptr;
}

BehaviorLinkRef *BehaviorGraphEdit::ResolveLink(const GraphEditLink *link) const {
    if (!link || link->EditOwner() != this) {
        return nullptr;
    }
    if (link->LinkId()) {
        return new BehaviorLinkRef(m_Bridge, m_RootBehaviorId, link->LinkId(), ComponentIdFromContext(m_Context));
    }
    const int index = link->SpecIndex();
    if (index < 0 || index >= static_cast<int>(m_Links.size())) {
        return nullptr;
    }
    const LinkSpec &spec = m_Links[index];
    return spec.CreatedLinkId ? new BehaviorLinkRef(m_Bridge, m_RootBehaviorId, spec.CreatedLinkId, ComponentIdFromContext(m_Context)) : nullptr;
}

GraphEditResult *BehaviorGraphEdit::MakeResult(bool ok,
                                               const std::string &error,
                                               const std::string &description,
                                               const std::vector<CK_ID> &createdNodes,
                                               const std::vector<CK_ID> &createdLinks) const {
    CKBehavior *root = RootBehavior();
    CKBehaviorContext resultContext = m_Context;
    resultContext.Behavior = root;
    return new GraphEditResult(m_Bridge, resultContext, ok, error, description, createdNodes, createdLinks);
}

bool BehaviorGraphEdit::ValidateInternal(const CKBehaviorContext &ctx, std::string &error) const {
    CKContext *context = ScriptBridgeGraphInternal::GraphContext(m_Bridge, ctx);
    CKBehavior *root = RootBehavior();
    if (!context || !m_Bridge) {
        error = "BehaviorGraphEdit requires CKContext and ScriptBehaviorBridge.";
        return false;
    }
    if (!root) {
        error = "BehaviorGraphEdit root graph is not valid.";
        return false;
    }
    if (root->IsUsingFunction()) {
        error = fmt::format("Behavior '{}' is a function behavior, not a graph container.", SafeString(root->GetName()));
        return false;
    }
    if (!m_Error.empty()) {
        error = m_Error;
        return false;
    }
    if (m_Applied) {
        error = "BehaviorGraphEdit has already applied. Create a new edit transaction for additional graph mutations.";
        return false;
    }

    std::set<int> removedNodes;
    for (const RemoveSpec &remove : m_Removes) {
        if (remove.NodeIndex < 0 || remove.NodeIndex >= static_cast<int>(m_Nodes.size())) {
            error = "BehaviorGraphEdit has an invalid remove node reference.";
            return false;
        }
        if (!removedNodes.insert(remove.NodeIndex).second) {
            error = "BehaviorGraphEdit removes the same node more than once.";
            return false;
        }
    }

    for (int i = 0; i < static_cast<int>(m_Nodes.size()); ++i) {
        const NodeSpec &spec = m_Nodes[i];
        if (spec.Type == NodeSpec::Kind::Existing) {
            CKBehavior *behavior = ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.BehaviorId, spec.BehaviorStamp);
            if (!behavior) {
                error = fmt::format("Graph edit node #{} is stale or deleted.", i);
                return false;
            }
            if (behavior == root) {
                error = "BehaviorGraphEdit cannot mutate the root graph as a child node.";
                return false;
            }
            if (!ScriptBridgeGraphInternal::IsDirectChildOf(root, behavior)) {
                error = fmt::format("Behavior '{}' is not a direct child of graph '{}'.",
                                    SafeString(behavior->GetName()),
                                    SafeString(root->GetName()));
                return false;
            }
        } else if (spec.Type == NodeSpec::Kind::Clone) {
            CKBehavior *behavior = ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.BehaviorId, spec.BehaviorStamp);
            if (!behavior) {
                error = fmt::format("Graph edit clone source #{} is stale or deleted.", i);
                return false;
            }
            if (behavior == root) {
                error = "BehaviorGraphEdit cannot clone the root graph as a child node.";
                return false;
            }
            if (!ScriptBridgeGraphInternal::IsDirectChildOf(root, behavior)) {
                error = fmt::format("BehaviorGraphEdit clone source '{}' is not a direct child of graph '{}'.",
                                    SafeString(behavior->GetName()),
                                    SafeString(root->GetName()));
                return false;
            }
        } else {
            BBConfig probe(m_Bridge, ctx, spec.Request);
            if (!probe.Validate(ctx)) {
                error = probe.Error();
                return false;
            }
        }
    }

    auto nodeRemoved = [&](int index) {
        return removedNodes.find(index) != removedNodes.end();
    };

    for (const TargetSpec &spec : m_Targets) {
        if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(spec.NodeIndex)) {
            error = "BehaviorGraphEdit.Target references an invalid or removed node.";
            return false;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        CKBeObject *target = CKBeObject::Cast(GetStampedCKObjectById(context, spec.TargetId, spec.TargetStamp));
        if (!behavior || !target) {
            error = "BehaviorGraphEdit.Target behavior or target is not valid.";
            return false;
        }
        if (!behavior->IsTargetable()) {
            error = fmt::format("BehaviorGraphEdit.Target cannot target non-targetable behavior '{}'.",
                                SafeString(behavior->GetName()));
            return false;
        }
        if (!CKIsChildClassOf(target, behavior->GetCompatibleClassID())) {
            error = fmt::format("BehaviorGraphEdit.Target target '{}' is not compatible with '{}'.",
                                SafeString(target->GetName()),
                                SafeString(behavior->GetName()));
            return false;
        }
    }

    for (const LayoutSpec &spec : m_LayoutEdits) {
        if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(spec.NodeIndex)) {
            error = "BehaviorGraphEdit layout edit references an invalid or removed node.";
            return false;
        }
        const NodeSpec &node = m_Nodes[spec.NodeIndex];
        if (node.Type != NodeSpec::Kind::Existing) {
            error = "BehaviorGraphEdit layout edit can only target existing behavior nodes.";
            return false;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        if (!behavior) {
            error = "BehaviorGraphEdit layout edit target behavior is not valid.";
            return false;
        }
        const int currentCount = spec.Type == LayoutSpec::Kind::EnsureInputCount
            ? behavior->GetInputCount()
            : behavior->GetOutputCount();
        if (spec.Count < currentCount) {
            error = fmt::format("BehaviorGraphEdit layout edit cannot shrink {} from {} to {}.",
                                ScriptBridgeGraphInternal::BehaviorLabel(behavior),
                                currentCount,
                                spec.Count);
            return false;
        }
    }

    for (const LinkSpec &spec : m_Links) {
        if (spec.Type == LinkSpec::Kind::Unlink) {
            CKBehaviorLink *link = ResolveExistingLink(spec);
            if (!link || !ScriptBridgeGraphInternal::GraphContainsLink(root, link)) {
                error = "BehaviorGraphEdit.Unlink references a stale or foreign behavior link.";
                return false;
            }
            continue;
        }

        if (spec.SourceNodeIndex < 0 || spec.SourceNodeIndex >= static_cast<int>(m_Nodes.size()) ||
            spec.TargetNodeIndex < 0 || spec.TargetNodeIndex >= static_cast<int>(m_Nodes.size())) {
            error = "BehaviorGraphEdit.Link has an invalid node reference.";
            return false;
        }
        if (nodeRemoved(spec.SourceNodeIndex) || nodeRemoved(spec.TargetNodeIndex)) {
            error = "BehaviorGraphEdit.Link references a node scheduled for removal.";
            return false;
        }

        CKBehavior *source = ResolveNodeBehavior(spec.SourceNodeIndex);
        CKBehavior *target = ResolveNodeBehavior(spec.TargetNodeIndex);
        if (source) {
            if (spec.SourceOutputIndex < 0 || spec.SourceOutputIndex >= PlannedOutputCount(spec.SourceNodeIndex)) {
                error = fmt::format("Source output index #{} is out of range for {}.",
                                    spec.SourceOutputIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(source));
                return false;
            }
        } else {
            const NodeSpec &sourceSpec = m_Nodes[spec.SourceNodeIndex];
            const int outputCount = sourceSpec.Type == NodeSpec::Kind::Clone
                ? PlannedOutputCount(spec.SourceNodeIndex)
                : [&]() {
                    const ScriptBridgeLayoutRecord *layout = m_Bridge->GetPrototypeLayout(ctx, sourceSpec.Request);
                    return layout ? static_cast<int>(layout->Outputs.size()) : -1;
                }();
            if (spec.SourceOutputIndex < 0 || spec.SourceOutputIndex >= outputCount) {
                error = fmt::format("Source output index #{} is out of range for pending node #{}.",
                                    spec.SourceOutputIndex,
                                    spec.SourceNodeIndex);
                return false;
            }
        }

        if (target) {
            if (spec.TargetInputIndex < 0 || spec.TargetInputIndex >= PlannedInputCount(spec.TargetNodeIndex)) {
                error = fmt::format("Target input index #{} is out of range for {}.",
                                    spec.TargetInputIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(target));
                return false;
            }
        } else {
            const NodeSpec &targetSpec = m_Nodes[spec.TargetNodeIndex];
            const int inputCount = targetSpec.Type == NodeSpec::Kind::Clone
                ? PlannedInputCount(spec.TargetNodeIndex)
                : [&]() {
                    const ScriptBridgeLayoutRecord *layout = m_Bridge->GetPrototypeLayout(ctx, targetSpec.Request);
                    return layout ? static_cast<int>(layout->Inputs.size()) : -1;
                }();
            if (spec.TargetInputIndex < 0 || spec.TargetInputIndex >= inputCount) {
                error = fmt::format("Target input index #{} is out of range for pending node #{}.",
                                    spec.TargetInputIndex,
                                    spec.TargetNodeIndex);
                return false;
            }
        }
    }

    for (const ValueSpec &spec : m_Values) {
        if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(spec.NodeIndex)) {
            error = "BehaviorGraphEdit.Set references an invalid or removed node.";
            return false;
        }
        if (m_Nodes[spec.NodeIndex].Type != NodeSpec::Kind::Existing) {
            error = "BehaviorGraphEdit.Set has an unexpected pending-node value entry.";
            return false;
        }
        if (!ValidateValueSpec(context, spec, error)) {
            return false;
        }
    }

    CKParameterManager *pm = context->GetParameterManager();
    for (const SourceSpec &spec : m_Sources) {
        if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(spec.NodeIndex)) {
            error = "BehaviorGraphEdit.Source references an invalid or removed node.";
            return false;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        CKParameterIn *pin = behavior && spec.PinIndex >= 0 && spec.PinIndex < behavior->GetInputParameterCount()
            ? behavior->GetInputParameter(spec.PinIndex)
            : nullptr;
        if (!pin) {
            error = fmt::format("BehaviorGraphEdit.Source pin index #{} is out of range.", spec.PinIndex);
            return false;
        }
        CKParameter *source = ResolveStampedParameterSource(context, spec.SourceId, spec.SourceStamp, error);
        if (!source) {
            if (error.empty()) {
                error = "BehaviorGraphEdit.Source parameter is not valid.";
            }
            return false;
        }
        if (pm && pm->IsTypeCompatible(pin->GetGUID(), source->GetGUID()) == FALSE &&
            pm->IsTypeCompatible(source->GetGUID(), pin->GetGUID()) == FALSE) {
            error = fmt::format("BehaviorGraphEdit.Source type mismatch for pin #{} '{}' expected {}, got {}.",
                                spec.PinIndex,
                                SafeString(pin->GetName()),
                                ParameterTypeLabel(context, pin),
                                ParameterTypeLabel(context, source));
            return false;
        }
    }

    for (const OperationSpec &spec : m_Operations) {
        if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(spec.NodeIndex)) {
            error = "BehaviorGraphEdit.Operation references an invalid or removed node.";
            return false;
        }
        CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex);
        CKParameterIn *pin = behavior && spec.Operation.TargetPinIndex >= 0 &&
                spec.Operation.TargetPinIndex < behavior->GetInputParameterCount()
            ? behavior->GetInputParameter(spec.Operation.TargetPinIndex)
            : nullptr;
        if (!pin) {
            error = fmt::format("BehaviorGraphEdit.Operation pin index #{} is out of range.",
                                spec.Operation.TargetPinIndex);
            return false;
        }
        std::string operationError;
        if (!ValidateOperationSpec(context, pin->GetGUID(), spec.Operation, operationError)) {
            error = fmt::format("BehaviorGraphEdit.Operation failed for pin #{} '{}': {}",
                                spec.Operation.TargetPinIndex,
                                SafeString(pin->GetName()),
                                operationError);
            return false;
        }
    }

    for (const MoveSpec &move : m_Moves) {
        if (move.NodeIndex < 0 || move.NodeIndex >= static_cast<int>(m_Nodes.size()) || nodeRemoved(move.NodeIndex)) {
            error = "BehaviorGraphEdit.Move has an invalid or removed node reference.";
            return false;
        }
        CKBehavior *targetRoot = ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, move.TargetRootId, move.TargetRootStamp);
        if (!targetRoot || targetRoot->IsUsingFunction()) {
            error = "BehaviorGraphEdit.Move target graph is not valid.";
            return false;
        }
        CKBehavior *behavior = ResolveNodeBehavior(move.NodeIndex);
        if (!behavior) {
            error = "BehaviorGraphEdit.Move can only move existing nodes.";
            return false;
        }
        if (targetRoot == root) {
            error = "BehaviorGraphEdit.Move target graph is the current graph; Move requires another non-descendant graph.";
            return false;
        }
        if (targetRoot == behavior || ScriptBridgeGraphInternal::ContainsSubBehaviorRecursive(behavior, targetRoot)) {
            error = "BehaviorGraphEdit.Move cannot move a behavior into itself or one of its descendant graphs.";
            return false;
        }
        for (int i = 0; i < root->GetSubBehaviorLinkCount(); ++i) {
            if (ScriptBridgeGraphInternal::LinkTouches(root->GetSubBehaviorLink(i), behavior)) {
                error = "BehaviorGraphEdit.Move requires existing incident links to be unlinked first.";
                return false;
            }
        }
    }

    for (const RemoveSpec &remove : m_Removes) {
        CKBehavior *behavior = ResolveNodeBehavior(remove.NodeIndex);
        if (!behavior) {
            error = "BehaviorGraphEdit.Remove can only remove existing nodes.";
            return false;
        }
        if (!remove.RemoveIncidentLinks) {
            for (int i = 0; i < root->GetSubBehaviorLinkCount(); ++i) {
                if (ScriptBridgeGraphInternal::LinkTouches(root->GetSubBehaviorLink(i), behavior)) {
                    error = fmt::format("Cannot remove behavior '{}' while incident links remain. Pass removeIncidentLinks=true or unlink first.",
                                        SafeString(behavior->GetName()));
                    return false;
                }
            }
        }
    }

    return true;
}

bool BehaviorGraphEdit::ResolveNodeIndex(const GraphEditNode *node, int &index, std::string &error) const {
    if (!RootBehavior()) {
        error = "BehaviorGraphEdit root graph is not valid.";
        return false;
    }
    if (!node || node->EditOwner() != this || !node->Error().empty()) {
        error = node ? node->Error() : "Graph edit node is not valid.";
        if (error.empty()) {
            error = "Graph edit node is not valid.";
        }
        return false;
    }
    index = node->SpecIndex();
    if (index < 0 || index >= static_cast<int>(m_Nodes.size())) {
        error = "Graph edit node does not belong to this edit transaction.";
        return false;
    }
    return true;
}

CKBehavior *BehaviorGraphEdit::ResolveNodeBehavior(int index) const {
    if (index < 0 || index >= static_cast<int>(m_Nodes.size())) {
        return nullptr;
    }
    const NodeSpec &spec = m_Nodes[index];
    if (spec.Type == NodeSpec::Kind::Create) {
        return spec.CreatedBehaviorId
            ? ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.CreatedBehaviorId, spec.CreatedBehaviorStamp)
            : nullptr;
    }
    if (spec.Type == NodeSpec::Kind::Clone && spec.CreatedBehaviorId) {
        return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.CreatedBehaviorId, spec.CreatedBehaviorStamp);
    }
    return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.BehaviorId, spec.BehaviorStamp);
}

CKBehaviorLink *BehaviorGraphEdit::ResolveExistingLink(const LinkSpec &spec) const {
    return ScriptBridgeGraphInternal::StampedLinkById(m_Bridge, spec.ExistingLinkId, spec.ExistingLinkStamp);
}

int BehaviorGraphEdit::PlannedInputCount(int nodeIndex) const {
    CKBehavior *behavior = ResolveNodeBehavior(nodeIndex);
    int count = behavior ? behavior->GetInputCount() : 0;
    for (const LayoutSpec &spec : m_LayoutEdits) {
        if (spec.NodeIndex == nodeIndex && spec.Type == LayoutSpec::Kind::EnsureInputCount) {
            count = std::max(count, spec.Count);
        }
    }
    return count;
}

int BehaviorGraphEdit::PlannedOutputCount(int nodeIndex) const {
    CKBehavior *behavior = ResolveNodeBehavior(nodeIndex);
    int count = behavior ? behavior->GetOutputCount() : 0;
    for (const LayoutSpec &spec : m_LayoutEdits) {
        if (spec.NodeIndex == nodeIndex && spec.Type == LayoutSpec::Kind::EnsureOutputCount) {
            count = std::max(count, spec.Count);
        }
    }
    return count;
}

bool BehaviorGraphEdit::ValidateValueSpec(CKContext *context, const ValueSpec &spec, std::string &error) const {
    if (!context) {
        error = "BehaviorGraphEdit value validation requires CKContext.";
        return false;
    }
    if (spec.NodeIndex < 0 || spec.NodeIndex >= static_cast<int>(m_Nodes.size())) {
        error = "BehaviorGraphEdit value has an invalid node reference.";
        return false;
    }

    CKGUID targetGuid;
    std::string label;
    if (CKBehavior *behavior = ResolveNodeBehavior(spec.NodeIndex)) {
        if (spec.Kind == ScriptBridgeSlotKind::Pin) {
            CKParameterIn *pin = spec.SlotIndex >= 0 && spec.SlotIndex < behavior->GetInputParameterCount()
                ? behavior->GetInputParameter(spec.SlotIndex)
                : nullptr;
            if (!pin) {
                error = fmt::format("Pin index #{} is out of range for {}.",
                                    spec.SlotIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(behavior));
                return false;
            }
            targetGuid = pin->GetGUID();
            label = fmt::format("pin #{} '{}'", spec.SlotIndex, SafeString(pin->GetName()));
        } else if (spec.Kind == ScriptBridgeSlotKind::Setting) {
            CKParameterLocal *setting = spec.SlotIndex >= 0 && spec.SlotIndex < behavior->GetLocalParameterCount()
                ? behavior->GetLocalParameter(spec.SlotIndex)
                : nullptr;
            if (!setting || !behavior->IsLocalParameterSetting(spec.SlotIndex)) {
                error = fmt::format("Setting index #{} is out of range for {}.",
                                    spec.SlotIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(behavior));
                return false;
            }
            targetGuid = setting->GetGUID();
            label = fmt::format("setting #{} '{}'", spec.SlotIndex, SafeString(setting->GetName()));
        } else {
            error = "BehaviorGraphEdit value target must be a pin or setting slot.";
            return false;
        }
    } else {
        const NodeSpec &node = m_Nodes[spec.NodeIndex];
        const ScriptBridgeLayoutRecord *layout = m_Bridge ? m_Bridge->GetPrototypeLayout(m_Context, node.Request) : nullptr;
        const ScriptBridgeLayoutParamSlot *slot = nullptr;
        if (layout && spec.Kind == ScriptBridgeSlotKind::Pin) {
            slot = ScriptBridgeGraphInternal::FindLayoutSlot(layout->Pins, spec.SlotIndex);
            label = fmt::format("pending pin #{}", spec.SlotIndex);
        } else if (layout && spec.Kind == ScriptBridgeSlotKind::Setting) {
            slot = ScriptBridgeGraphInternal::FindLayoutSlot(layout->Settings, spec.SlotIndex);
            label = fmt::format("pending setting #{}", spec.SlotIndex);
        }
        if (!slot) {
            error = fmt::format("BehaviorGraphEdit value slot #{} is not valid for pending node #{}.",
                                spec.SlotIndex,
                                spec.NodeIndex);
            return false;
        }
        targetGuid = slot->TypeGuid;
        label = fmt::format("{} '{}'", label, slot->Name);
    }

    CKParameterLocal *probe = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_GraphEditValidate"), targetGuid, TRUE);
    if (!probe) {
        error = fmt::format("Failed to create validation parameter for {}.", label);
        return false;
    }

    std::string writeError;
    const CKERROR err = WriteParameterValue(probe, spec.Value, writeError);
    context->DestroyObject(probe);
    if (err != CK_OK) {
        error = fmt::format("BehaviorGraphEdit value validation failed for {} expected {}: {}",
                            label,
                            ParameterTypeLabel(context, targetGuid),
                            writeError.empty() ? fmt::format("CKERROR {}", err) : writeError);
        return false;
    }
    return true;
}

bool BehaviorGraphEdit::ApplyExistingValue(CKBehavior *behavior,
                                           const ValueSpec &spec,
                                           std::string &error,
                                           std::vector<ParamSourceLinkRef *> &sourceLinks,
                                           std::vector<CK_ID> &localSourceIds,
                                           std::vector<CK_ID> &replacedLocalSourceIds,
                                           std::vector<CK_ID> &replacedOperations) {
    if (!behavior) {
        error = "BehaviorGraphEdit value target behavior is not valid.";
        return false;
    }

    if (spec.Kind == ScriptBridgeSlotKind::Setting) {
        return m_Bridge && m_Bridge->SetBehaviorSetting(behavior, spec.SlotIndex, spec.Value, error);
    }
    if (spec.Kind != ScriptBridgeSlotKind::Pin) {
        error = "BehaviorGraphEdit.Set can only target pin slots.";
        return false;
    }

    CKParameterIn *pin = spec.SlotIndex >= 0 && spec.SlotIndex < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(spec.SlotIndex)
        : nullptr;
    if (!pin) {
        error = fmt::format("Input parameter index #{} is out of range for {}.",
                            spec.SlotIndex,
                            ScriptBridgeGraphInternal::BehaviorLabel(behavior));
        return false;
    }

    CKParameterLocal *local = ScriptBridgeGraphInternal::CreateGraphEditInputSource(behavior, spec.SlotIndex, spec.Value, error);
    if (!local) {
        return false;
    }

    CKParameter *previous = pin->GetDirectSource();
    const CKERROR err = pin->SetDirectSource(local);
    if (err != CK_OK) {
        error = fmt::format("Failed to connect graph edit literal source to input parameter #{} '{}' (CKERROR {}).",
                            spec.SlotIndex,
                            SafeString(pin->GetName()),
                            err);
        ScriptBridgeGraphInternal::DestroyGraphEditLocalSource(behavior->GetCKContext(), local->GetID());
        return false;
    }

    sourceLinks.push_back(new ParamSourceLinkRef(m_Bridge, pin, previous, local));
    localSourceIds.push_back(local->GetID());
    if (ScriptBridgeGraphInternal::IsGraphEditInputSource(previous)) {
        replacedLocalSourceIds.push_back(previous->GetID());
    } else if (CKParameterOperation *operation = ScriptBridgeGraphInternal::OperationFromDirectSource(previous, behavior)) {
        replacedOperations.push_back(operation->GetID());
    }
    return true;
}

bool BehaviorGraphEdit::ApplyExistingSource(CKBehavior *behavior,
                                            const SourceSpec &spec,
                                            std::string &error,
                                            std::vector<ParamSourceLinkRef *> &sourceLinks,
                                            std::vector<CK_ID> &replacedLocalSourceIds,
                                            std::vector<CK_ID> &replacedOperations) {
    if (!behavior) {
        error = "BehaviorGraphEdit source target behavior is not valid.";
        return false;
    }
    CKParameterIn *pin = spec.PinIndex >= 0 && spec.PinIndex < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(spec.PinIndex)
        : nullptr;
    if (!pin) {
        error = fmt::format("Input parameter index #{} is out of range for {}.",
                            spec.PinIndex,
                            ScriptBridgeGraphInternal::BehaviorLabel(behavior));
        return false;
    }

    CKParameter *source = ResolveStampedParameterSource(behavior->GetCKContext(), spec.SourceId, spec.SourceStamp, error);
    if (!source) {
        if (error.empty()) {
            error = "BehaviorGraphEdit source parameter is not valid.";
        }
        return false;
    }

    CKParameter *previous = pin->GetDirectSource();
    const CKERROR err = pin->SetDirectSource(source);
    if (err != CK_OK) {
        error = fmt::format("Failed to connect graph edit source to pin #{} '{}' (expected {}, got {}, CKERROR {}).",
                            spec.PinIndex,
                            SafeString(pin->GetName()),
                            ParameterTypeLabel(behavior->GetCKContext(), pin),
                            ParameterTypeLabel(behavior->GetCKContext(), source),
                            err);
        return false;
    }

    sourceLinks.push_back(new ParamSourceLinkRef(m_Bridge, pin, previous, source));
    if (ScriptBridgeGraphInternal::IsGraphEditInputSource(previous)) {
        replacedLocalSourceIds.push_back(previous->GetID());
    } else if (CKParameterOperation *operation = ScriptBridgeGraphInternal::OperationFromDirectSource(previous, behavior)) {
        replacedOperations.push_back(operation->GetID());
    }
    return true;
}

bool BehaviorGraphEdit::ApplyExistingOperation(CKBehavior *behavior,
                                               const OperationSpec &spec,
                                               std::string &error,
                                               std::vector<ParamOperationRef *> &operations,
                                               std::vector<CK_ID> &replacedOperations) {
    if (!behavior) {
        error = "BehaviorGraphEdit operation target behavior is not valid.";
        return false;
    }
    CKParameterIn *pin = spec.Operation.TargetPinIndex >= 0 && spec.Operation.TargetPinIndex < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(spec.Operation.TargetPinIndex)
        : nullptr;
    CKParameter *previous = pin ? pin->GetDirectSource() : nullptr;
    CKParameterOperation *previousOperation = ScriptBridgeGraphInternal::OperationFromDirectSource(previous, behavior);
    ParamOperationRef *operation = ConnectOperationToInput(m_Bridge, behavior, spec.Operation.TargetPinIndex, spec.Operation, error, true, nullptr);
    if (!operation) {
        return false;
    }
    if (previousOperation) {
        replacedOperations.push_back(previousOperation->GetID());
    }
    operations.push_back(operation);
    return true;
}

BehaviorGraphEdit *BehaviorGraphEdit::SetValue(GraphEditNode *node,
                                               BBSlot *slot,
                                               ScriptBridgeSlotKind kind,
                                               const ScriptParamValue &value,
                                               const char *method) {
    int nodeIndex = -1;
    int slotIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(node, nodeIndex, error)) {
        SetError(error);
    } else if (!slot || !slot->ResolveIndex(kind, slotIndex, error)) {
        SetError(fmt::format("{} requires a {} BBSlot.{}",
                             method ? method : "BehaviorGraphEdit.Set",
                             kind == ScriptBridgeSlotKind::Setting ? "setting" : "pin",
                             error.empty() ? "" : " " + error));
    } else {
        if (kind == ScriptBridgeSlotKind::Pin) {
            RemoveNodeSource(nodeIndex, slotIndex);
            RemoveNodeOperation(nodeIndex, slotIndex);
        }
        RemoveNodeValue(nodeIndex, kind, slotIndex);

        NodeSpec &nodeSpec = m_Nodes[nodeIndex];
        if (nodeSpec.Type == NodeSpec::Kind::Create) {
            if (kind == ScriptBridgeSlotKind::Pin) {
                nodeSpec.Request.SourceParameters.erase(
                    std::remove_if(nodeSpec.Request.SourceParameters.begin(),
                                   nodeSpec.Request.SourceParameters.end(),
                                   [slotIndex](const ScriptBridgeInputSource &entry) { return entry.PinIndex == slotIndex; }),
                    nodeSpec.Request.SourceParameters.end());
                nodeSpec.Request.OperationParameters.erase(
                    std::remove_if(nodeSpec.Request.OperationParameters.begin(),
                                   nodeSpec.Request.OperationParameters.end(),
                                   [slotIndex](const ScriptBridgeOperationSpec &entry) { return entry.TargetPinIndex == slotIndex; }),
                    nodeSpec.Request.OperationParameters.end());
                ScriptBridgeSetIndexedValue(nodeSpec.Request.IndexedParameters, slotIndex, value);
            } else {
                ScriptBridgeSetIndexedValue(nodeSpec.Request.IndexedSettings, slotIndex, value);
            }
        } else {
            ValueSpec spec;
            spec.NodeIndex = nodeIndex;
            spec.Kind = kind;
            spec.SlotIndex = slotIndex;
            spec.Value = value;
            m_Values.push_back(spec);
        }
    }
    AddRef();
    return this;
}

BehaviorGraphEdit *BehaviorGraphEdit::EnsureIoCount(GraphEditNode *node,
                                                    LayoutSpec::Kind kind,
                                                    int count,
                                                    const std::string &prefix,
                                                    const char *method) {
    int nodeIndex = -1;
    std::string error;
    if (!ResolveNodeIndex(node, nodeIndex, error)) {
        SetError(error);
    } else if (count < 0) {
        SetError(fmt::format("{} requires a non-negative count.", method ? method : "BehaviorGraphEdit.EnsureIoCount"));
    } else {
        LayoutSpec spec;
        spec.Type = kind;
        spec.NodeIndex = nodeIndex;
        spec.Count = count;
        spec.Prefix = TrimString(prefix);
        m_LayoutEdits.push_back(spec);
    }
    AddRef();
    return this;
}

void BehaviorGraphEdit::RemoveNodeValue(int nodeIndex, ScriptBridgeSlotKind kind, int slotIndex) {
    m_Values.erase(std::remove_if(m_Values.begin(),
                                  m_Values.end(),
                                  [nodeIndex, kind, slotIndex](const ValueSpec &spec) {
                                      return spec.NodeIndex == nodeIndex &&
                                             spec.Kind == kind &&
                                             spec.SlotIndex == slotIndex;
                                  }),
                   m_Values.end());
}

void BehaviorGraphEdit::RemoveNodeSource(int nodeIndex, int pinIndex) {
    m_Sources.erase(std::remove_if(m_Sources.begin(),
                                   m_Sources.end(),
                                   [nodeIndex, pinIndex](const SourceSpec &spec) {
                                       return spec.NodeIndex == nodeIndex && spec.PinIndex == pinIndex;
                                   }),
                    m_Sources.end());
    if (nodeIndex >= 0 && nodeIndex < static_cast<int>(m_Nodes.size())) {
        ScriptBridgeBBInvocationSpec &request = m_Nodes[nodeIndex].Request;
        request.SourceParameters.erase(std::remove_if(request.SourceParameters.begin(),
                                                      request.SourceParameters.end(),
                                                      [pinIndex](const ScriptBridgeInputSource &entry) {
                                                          return entry.PinIndex == pinIndex;
                                                      }),
                                       request.SourceParameters.end());
    }
}

void BehaviorGraphEdit::RemoveNodeOperation(int nodeIndex, int pinIndex) {
    m_Operations.erase(std::remove_if(m_Operations.begin(),
                                      m_Operations.end(),
                                      [nodeIndex, pinIndex](const OperationSpec &spec) {
                                          return spec.NodeIndex == nodeIndex &&
                                                 spec.Operation.TargetPinIndex == pinIndex;
                                      }),
                       m_Operations.end());
    if (nodeIndex >= 0 && nodeIndex < static_cast<int>(m_Nodes.size())) {
        ScriptBridgeBBInvocationSpec &request = m_Nodes[nodeIndex].Request;
        request.OperationParameters.erase(std::remove_if(request.OperationParameters.begin(),
                                                         request.OperationParameters.end(),
                                                         [pinIndex](const ScriptBridgeOperationSpec &entry) {
                                                             return entry.TargetPinIndex == pinIndex;
                                                         }),
                                          request.OperationParameters.end());
    }
}

void BehaviorGraphEdit::SetRequestSource(ScriptBridgeBBInvocationSpec &request, const ScriptBridgeInputSource &source) {
    const auto it = std::lower_bound(request.SourceParameters.begin(),
                                     request.SourceParameters.end(),
                                     source.PinIndex,
                                     [](const ScriptBridgeInputSource &entry, int index) {
                                         return entry.PinIndex < index;
                                     });
    if (it != request.SourceParameters.end() && it->PinIndex == source.PinIndex) {
        *it = source;
    } else {
        request.SourceParameters.insert(it, source);
    }
}

void BehaviorGraphEdit::SetRequestOperation(ScriptBridgeBBInvocationSpec &request, const ScriptBridgeOperationSpec &operation) {
    const auto it = std::lower_bound(request.OperationParameters.begin(),
                                     request.OperationParameters.end(),
                                     operation.TargetPinIndex,
                                     [](const ScriptBridgeOperationSpec &entry, int index) {
                                         return entry.TargetPinIndex < index;
                                     });
    if (it != request.OperationParameters.end() && it->TargetPinIndex == operation.TargetPinIndex) {
        *it = operation;
    } else {
        request.OperationParameters.insert(it, operation);
    }
}

void BehaviorGraphEdit::SetError(const std::string &error) const {
    m_Error = error;
    if (!error.empty()) {
        SetScriptException(error);
    }
}

BehaviorNode::BehaviorNode(ScriptBehaviorBridge *bridge,
                           const CKBehaviorContext &ctx,
                           CK_ID rootBehaviorId,
                           CK_ID behaviorId,
                           CK_ID componentId,
                           const std::string &error)
    : m_Bridge(bridge),
      m_Context(ctx),
      m_RootBehaviorId(rootBehaviorId),
      m_BehaviorId(behaviorId),
      m_ComponentId(componentId),
      m_Error(error) {
    m_BehaviorStamp = CaptureBridgeObjectStamp(Get());
}

bool BehaviorNode::IsValid() const {
    return Get() != nullptr && m_Error.empty();
}

std::string BehaviorNode::Error() const {
    return m_Error;
}

BehaviorRef *BehaviorNode::Behavior() const {
    return m_Bridge ? m_Bridge->WrapBehavior(Get(), m_ComponentId) : nullptr;
}

BehaviorGraph *BehaviorNode::AsGraph() const {
    CKBehavior *behavior = Get();
    return behavior ? new BehaviorGraph(m_Bridge, m_Context, behavior->GetID()) : nullptr;
}

BehaviorNode *BehaviorNode::Input(int index) const { return AdjacentFirst(false, index, nullptr); }
BehaviorNode *BehaviorNode::Output(int index) const { return AdjacentFirst(true, index, nullptr); }
BehaviorNode *BehaviorNode::Next(BehaviorQuery *query) const { return AdjacentFirst(true, -1, query); }
BehaviorNode *BehaviorNode::Prev(BehaviorQuery *query) const { return AdjacentFirst(false, -1, query); }
CScriptArray *BehaviorNode::NextAll(BehaviorQuery *query) const { return AdjacentAll(true, -1, query); }
CScriptArray *BehaviorNode::PrevAll(BehaviorQuery *query) const { return AdjacentAll(false, -1, query); }
BehaviorLinkRef *BehaviorNode::NextLink(BehaviorQuery *query) const { return AdjacentLinkFirst(true, -1, query); }
BehaviorLinkRef *BehaviorNode::PrevLink(BehaviorQuery *query) const { return AdjacentLinkFirst(false, -1, query); }

BehaviorNode *BehaviorNode::End(int maxSteps) const {
    CKBehavior *current = Get();
    if (!current) {
        return new BehaviorNode(m_Bridge, m_Context, m_RootBehaviorId, 0, m_ComponentId, "BehaviorNode is not valid.");
    }
    std::set<CK_ID> visited;
    int remaining = maxSteps < 0 ? 256 : maxSteps;
    while (remaining-- > 0 && current) {
        if (visited.find(current->GetID()) != visited.end()) {
            break;
        }
        visited.insert(current->GetID());
        BehaviorNode probe(m_Bridge, m_Context, m_RootBehaviorId, current->GetID(), m_ComponentId);
        BehaviorNode *next = probe.Next(nullptr);
        CKBehavior *nextBehavior = next ? next->Get() : nullptr;
        if (!nextBehavior) {
            if (next) next->Release();
            break;
        }
        current = nextBehavior;
        next->Release();
    }
    return new BehaviorNode(m_Bridge, m_Context, m_RootBehaviorId, current ? current->GetID() : 0, m_ComponentId);
}

std::string BehaviorNode::Describe() const {
    if (!m_Error.empty()) {
        return m_Error;
    }
    return ScriptBridgeGraphInternal::BehaviorLabel(Get());
}

CKBehavior *BehaviorNode::Get() const {
    return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, m_BehaviorId, m_BehaviorStamp);
}

CK_ID BehaviorNode::RootId() const { return m_RootBehaviorId; }
CK_ID BehaviorNode::BehaviorId() const { return m_BehaviorId; }

CScriptArray *BehaviorNode::AdjacentAll(bool next, int ioIndex, BehaviorQuery *query) const {
    CScriptArray *array = ScriptBridgeGraphInternal::CreateNodeArray(m_Bridge);
    if (!array) {
        return nullptr;
    }
    CKBehavior *container = ScriptBridgeGraphInternal::BehaviorById(m_Bridge, m_RootBehaviorId);
    CKBehavior *self = Get();
    if (!container || !self) {
        return array;
    }
    for (int i = 0; i < container->GetSubBehaviorLinkCount(); ++i) {
        CKBehaviorLink *link = container->GetSubBehaviorLink(i);
        CKBehavior *source = ScriptBridgeGraphInternal::SourceBehavior(link);
        CKBehavior *target = ScriptBridgeGraphInternal::TargetBehavior(link);
        CKBehavior *candidate = next ? target : source;
        CKBehavior *anchor = next ? source : target;
        const int linkIndex = next ? ScriptBridgeGraphInternal::SourceOutputIndex(link) : ScriptBridgeGraphInternal::TargetInputIndex(link);
        if (anchor != self || !candidate || (ioIndex >= 0 && linkIndex != ioIndex)) {
            continue;
        }
        if (query && !query->Matches(candidate, 0)) {
            continue;
        }
        ScriptBridgeGraphInternal::AppendNode(array, new BehaviorNode(m_Bridge, m_Context, m_RootBehaviorId, candidate->GetID(), m_ComponentId));
    }
    return array;
}

BehaviorNode *BehaviorNode::AdjacentFirst(bool next, int ioIndex, BehaviorQuery *query) const {
    CScriptArray *array = AdjacentAll(next, ioIndex, query);
    if (!array) {
        return nullptr;
    }
    BehaviorNode *result = nullptr;
    if (array->GetSize() > 0) {
        void *slot = array->At(0);
        result = slot ? *static_cast<BehaviorNode **>(slot) : nullptr;
        if (result) result->AddRef();
    }
    array->Release();
    return result;
}

BehaviorLinkRef *BehaviorNode::AdjacentLinkFirst(bool next, int ioIndex, BehaviorQuery *query) const {
    CKBehavior *container = ScriptBridgeGraphInternal::BehaviorById(m_Bridge, m_RootBehaviorId);
    CKBehavior *self = Get();
    if (!container || !self) {
        return nullptr;
    }
    for (int i = 0; i < container->GetSubBehaviorLinkCount(); ++i) {
        CKBehaviorLink *link = container->GetSubBehaviorLink(i);
        CKBehavior *source = ScriptBridgeGraphInternal::SourceBehavior(link);
        CKBehavior *target = ScriptBridgeGraphInternal::TargetBehavior(link);
        CKBehavior *candidate = next ? target : source;
        CKBehavior *anchor = next ? source : target;
        const int linkIndex = next ? ScriptBridgeGraphInternal::SourceOutputIndex(link) : ScriptBridgeGraphInternal::TargetInputIndex(link);
        if (anchor != self || !candidate || (ioIndex >= 0 && linkIndex != ioIndex)) {
            continue;
        }
        if (query && !query->Matches(candidate, 0)) {
            continue;
        }
        return new BehaviorLinkRef(m_Bridge, m_RootBehaviorId, link->GetID(), m_ComponentId);
    }
    return nullptr;
}

BehaviorLinkRef::BehaviorLinkRef(ScriptBehaviorBridge *bridge,
                                 CK_ID rootBehaviorId,
                                 CK_ID linkId,
                                 CK_ID componentId,
                                 CKContext *context)
    : ObjectRef(context ? context : (bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr), linkId),
      m_Bridge(bridge),
      m_RootBehaviorId(rootBehaviorId),
      m_ComponentId(componentId) {}

bool BehaviorLinkRef::IsValid() const {
    return Get() != nullptr;
}

BehaviorRef *BehaviorLinkRef::SourceBehavior() const {
    CKBehavior *behavior = ScriptBridgeGraphInternal::SourceBehavior(Get());
    if (!behavior) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapBehavior(behavior, m_ComponentId)
                    : new BehaviorRef(nullptr, behavior->GetID(), m_ComponentId, behavior->GetCKContext());
}

int BehaviorLinkRef::SourceOutputIndex() const {
    return ScriptBridgeGraphInternal::SourceOutputIndex(Get());
}

BehaviorRef *BehaviorLinkRef::TargetBehavior() const {
    CKBehavior *behavior = ScriptBridgeGraphInternal::TargetBehavior(Get());
    if (!behavior) {
        return nullptr;
    }
    return m_Bridge ? m_Bridge->WrapBehavior(behavior, m_ComponentId)
                    : new BehaviorRef(nullptr, behavior->GetID(), m_ComponentId, behavior->GetCKContext());
}

int BehaviorLinkRef::TargetInputIndex() const {
    return ScriptBridgeGraphInternal::TargetInputIndex(Get());
}

int BehaviorLinkRef::Delay() const {
    CKBehaviorLink *link = Get();
    return link ? link->GetActivationDelay() : 0;
}

std::string BehaviorLinkRef::Describe() const {
    CKBehaviorLink *link = Get();
    if (!link) {
        return "BehaviorLinkRef is not valid.";
    }
    return fmt::format("BehaviorLink id={} {}#{} -> {}#{} delay={}",
                       link->GetID(),
                       ScriptBridgeGraphInternal::BehaviorLabel(ScriptBridgeGraphInternal::SourceBehavior(link)),
                       SourceOutputIndex(),
                       ScriptBridgeGraphInternal::BehaviorLabel(ScriptBridgeGraphInternal::TargetBehavior(link)),
                       TargetInputIndex(),
                       Delay());
}

CKBehaviorLink *BehaviorLinkRef::Get() const {
    return CKBehaviorLink::Cast(Object());
}

CK_ID BehaviorLinkRef::RootId() const { return m_RootBehaviorId; }
CK_ID BehaviorLinkRef::LinkId() const { return Id(); }
