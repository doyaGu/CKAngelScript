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

CScriptArray *CreateNodeArray(ScriptBehaviorBridge *bridge) {
    asITypeInfo *arrayType = NodeArrayType(bridge);
    if (!arrayType) {
        SetScriptException("array<BehaviorNode@> is not registered.");
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
