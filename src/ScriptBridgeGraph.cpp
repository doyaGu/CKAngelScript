#include "ScriptBridgeHandles.h"

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

} // namespace ScriptBridgeGraphInternal

BehaviorQuery::BehaviorQuery() = default;

BehaviorQuery *BehaviorQuery::Name(const std::string &name) {
    m_Name = name;
    return this;
}

BehaviorQuery *BehaviorQuery::NameContains(const std::string &text) {
    m_NameContains = text;
    return this;
}

BehaviorQuery *BehaviorQuery::PrototypeGuid(CKGUID guid) {
    m_PrototypeGuid = guid;
    return this;
}

BehaviorQuery *BehaviorQuery::PrototypeName(const std::string &name) {
    m_PrototypeName = name;
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
    return this;
}

BehaviorQuery *BehaviorQuery::Target(CKBeObject *target) {
    m_TargetId = target ? target->GetID() : 0;
    return this;
}

BehaviorQuery *BehaviorQuery::TargetName(const std::string &name) {
    m_TargetName = name;
    return this;
}

BehaviorQuery *BehaviorQuery::TargetId(CK_ID id) {
    m_TargetId = id;
    return this;
}

BehaviorQuery *BehaviorQuery::InputCount(int count) { m_InputCount = count; return this; }
BehaviorQuery *BehaviorQuery::OutputCount(int count) { m_OutputCount = count; return this; }
BehaviorQuery *BehaviorQuery::PinCount(int count) { m_PinCount = count; return this; }
BehaviorQuery *BehaviorQuery::PoutCount(int count) { m_PoutCount = count; return this; }
BehaviorQuery *BehaviorQuery::MaxDepth(int depth) { m_MaxDepth = depth; return this; }
BehaviorQuery *BehaviorQuery::IncludeRoot(bool includeRoot) { m_IncludeRoot = includeRoot; return this; }
BehaviorQuery *BehaviorQuery::Recursive(bool recursive) { m_Recursive = recursive; return this; }
BehaviorQuery *BehaviorQuery::Occurrence(int occurrence) { m_Occurrence = occurrence < 0 ? 0 : occurrence; return this; }

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
    if (!m_PrototypeName.empty() && SafeString(behavior->GetPrototypeName()) != m_PrototypeName) {
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
    return m_Edit && m_Error.empty() && (m_SpecIndex >= 0 || m_BehaviorId != 0);
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
    return fmt::format("BehaviorGraphEdit(root={}, nodes={}, links={}, removes={}, moves={})",
                       ScriptBridgeGraphInternal::BehaviorLabel(RootBehavior()),
                       m_Nodes.size(),
                       m_Links.size(),
                       m_Removes.size(),
                       m_Moves.size());
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

    auto rollbackCreated = [&]() {
        for (CK_ID linkId : createdLinks) {
            if (CKBehaviorLink *link = CKBehaviorLink::Cast(GetCKObjectById(context, linkId))) {
                if (root) {
                    root->RemoveSubBehaviorLink(link);
                }
                context->DestroyObject(link);
            }
        }
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

    for (int i = 0; i < static_cast<int>(m_Nodes.size()); ++i) {
        NodeSpec &spec = m_Nodes[i];
        if (spec.Type != NodeSpec::Kind::Create) {
            continue;
        }
        std::vector<CK_ID> operationIds;
        CKBehavior *behavior = m_Bridge->CreatePersistentBehavior(spec.Request, ctx, spec.Name, error, &operationIds);
        if (!behavior) {
            rollbackCreated();
            SetError(error);
            return MakeResult(false, error, error);
        }
        createdOperations.insert(createdOperations.end(), operationIds.begin(), operationIds.end());
        CKERROR err = root->AddSubBehavior(behavior);
        if (err != CK_OK) {
            context->DestroyObject(behavior);
            rollbackCreated();
            error = fmt::format("Failed to add behavior '{}' to graph '{}' (CKERROR {}).",
                                SafeString(behavior->GetName()),
                                SafeString(root->GetName()),
                                err);
            SetError(error);
            return MakeResult(false, error, error);
        }
        spec.CreatedBehaviorId = behavior->GetID();
        createdNodes.push_back(behavior->GetID());
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
            rollbackCreated();
            error = "Failed to create behavior link: source or target IO is not valid.";
            SetError(error);
            return MakeResult(false, error, error);
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
            rollbackCreated();
            error = fmt::format("Failed to add behavior link {}#{} -> {}#{} (CKERROR {}).",
                                ScriptBridgeGraphInternal::BehaviorLabel(source),
                                spec.SourceOutputIndex,
                                ScriptBridgeGraphInternal::BehaviorLabel(target),
                                spec.TargetInputIndex,
                                err);
            SetError(error);
            return MakeResult(false, error, error);
        }
        spec.CreatedLinkId = link->GetID();
        createdLinks.push_back(link->GetID());
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
            SetError(error);
            return MakeResult(false, error, error, createdNodes, createdLinks);
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
            if (spec.SourceOutputIndex < 0 || spec.SourceOutputIndex >= source->GetOutputCount()) {
                error = fmt::format("Source output index #{} is out of range for {}.",
                                    spec.SourceOutputIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(source));
                return false;
            }
        } else {
            const NodeSpec &sourceSpec = m_Nodes[spec.SourceNodeIndex];
            const ScriptBridgeLayoutRecord *layout = m_Bridge->GetPrototypeLayout(ctx, sourceSpec.Request);
            if (!layout || spec.SourceOutputIndex < 0 || spec.SourceOutputIndex >= static_cast<int>(layout->Outputs.size())) {
                error = fmt::format("Source output index #{} is out of range for pending node #{}.",
                                    spec.SourceOutputIndex,
                                    spec.SourceNodeIndex);
                return false;
            }
        }

        if (target) {
            if (spec.TargetInputIndex < 0 || spec.TargetInputIndex >= target->GetInputCount()) {
                error = fmt::format("Target input index #{} is out of range for {}.",
                                    spec.TargetInputIndex,
                                    ScriptBridgeGraphInternal::BehaviorLabel(target));
                return false;
            }
        } else {
            const NodeSpec &targetSpec = m_Nodes[spec.TargetNodeIndex];
            const ScriptBridgeLayoutRecord *layout = m_Bridge->GetPrototypeLayout(ctx, targetSpec.Request);
            if (!layout || spec.TargetInputIndex < 0 || spec.TargetInputIndex >= static_cast<int>(layout->Inputs.size())) {
                error = fmt::format("Target input index #{} is out of range for pending node #{}.",
                                    spec.TargetInputIndex,
                                    spec.TargetNodeIndex);
                return false;
            }
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
    if (!node || node->EditOwner() != this || !node->IsValid()) {
        error = node ? node->Error() : "Graph edit node is not valid.";
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
        return spec.CreatedBehaviorId ? ScriptBridgeGraphInternal::BehaviorById(m_Bridge, spec.CreatedBehaviorId) : nullptr;
    }
    return ScriptBridgeGraphInternal::StampedBehaviorById(m_Bridge, spec.BehaviorId, spec.BehaviorStamp);
}

CKBehaviorLink *BehaviorGraphEdit::ResolveExistingLink(const LinkSpec &spec) const {
    return ScriptBridgeGraphInternal::StampedLinkById(m_Bridge, spec.ExistingLinkId, spec.ExistingLinkStamp);
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
                                 CK_ID componentId)
    : m_Bridge(bridge), m_RootBehaviorId(rootBehaviorId), m_LinkId(linkId), m_ComponentId(componentId) {
    m_LinkStamp = CaptureBridgeObjectStamp(Get());
}

bool BehaviorLinkRef::IsValid() const {
    return Get() != nullptr;
}

BehaviorRef *BehaviorLinkRef::SourceBehavior() const {
    CKBehavior *behavior = ScriptBridgeGraphInternal::SourceBehavior(Get());
    return m_Bridge ? m_Bridge->WrapBehavior(behavior, m_ComponentId) : nullptr;
}

int BehaviorLinkRef::SourceOutputIndex() const {
    return ScriptBridgeGraphInternal::SourceOutputIndex(Get());
}

BehaviorRef *BehaviorLinkRef::TargetBehavior() const {
    CKBehavior *behavior = ScriptBridgeGraphInternal::TargetBehavior(Get());
    return m_Bridge ? m_Bridge->WrapBehavior(behavior, m_ComponentId) : nullptr;
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
    return ScriptBridgeGraphInternal::StampedLinkById(m_Bridge, m_LinkId, m_LinkStamp);
}

CK_ID BehaviorLinkRef::RootId() const { return m_RootBehaviorId; }
CK_ID BehaviorLinkRef::LinkId() const { return m_LinkId; }
