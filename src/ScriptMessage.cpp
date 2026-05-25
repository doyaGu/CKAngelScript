#include "ScriptMessage.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <utility>

#include <fmt/format.h>

#include "CKAll.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "add_on/scriptdictionary/scriptdictionary.h"

namespace {

std::string TrimTopic(const std::string &topic) {
    const auto first = std::find_if_not(topic.begin(), topic.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last = std::find_if_not(topic.rbegin(), topic.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return first < last ? std::string(first, last) : std::string();
}

ScriptManager *ManagerFromRuntimeContext(const ScriptContext &ctx) {
    return ctx.Context() ? ScriptManager::GetManager(ctx.Context()) : nullptr;
}

ScriptManager *ManagerFromBehaviorContext(const CKBehaviorContext &ctx) {
    return ctx.Context ? ScriptManager::GetManager(ctx.Context) : nullptr;
}

std::string SourceFromRuntimeContext(const ScriptContext &ctx) {
    return ctx.Target();
}

std::string SourceFromBehaviorContext(const CKBehaviorContext &ctx) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    ScriptComponentState *state = manager && ctx.Behavior ? manager->GetComponentState(ctx.Behavior->GetID()) : nullptr;
    return state ? state->MessageTarget : (ctx.Behavior ? ScriptMessageBus::ComponentTarget(ctx.Behavior->GetID()) : std::string());
}

void RaiseRuntimeError(const ScriptContext &ctx, const std::string &error) {
    if (!error.empty()) {
        ctx.Raise(error);
    }
}

void RaiseBehaviorError(const CKBehaviorContext &ctx, const std::string &error) {
    if (!error.empty() && ctx.Context) {
        ctx.Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Message] %s"), error.c_str());
    }
}

bool PublishRuntime(const ScriptContext &ctx, const std::string &topic, CScriptDictionary *payload, const std::string &target) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Publish(SourceFromRuntimeContext(ctx), topic, payload, target, error);
    if (!ok) {
        RaiseRuntimeError(ctx, error.empty() ? "Message::Publish failed." : error);
    }
    return ok;
}

bool PublishBehavior(const CKBehaviorContext &ctx, const std::string &topic, CScriptDictionary *payload, const std::string &target) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Publish(SourceFromBehaviorContext(ctx), topic, payload, target, error);
    if (!ok) {
        RaiseBehaviorError(ctx, error.empty() ? "Message::Publish failed." : error);
    }
    return ok;
}

bool SendRuntime(const ScriptContext &ctx, const std::string &target, const std::string &topic, CScriptDictionary *payload) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Send(SourceFromRuntimeContext(ctx), target, topic, payload, error);
    if (!ok) {
        RaiseRuntimeError(ctx, error.empty() ? "Message::Send failed." : error);
    }
    return ok;
}

bool SendBehavior(const CKBehaviorContext &ctx, const std::string &target, const std::string &topic, CScriptDictionary *payload) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Send(SourceFromBehaviorContext(ctx), target, topic, payload, error);
    if (!ok) {
        RaiseBehaviorError(ctx, error.empty() ? "Message::Send failed." : error);
    }
    return ok;
}

ScriptAsyncTaskBase *RequestRuntime(const ScriptContext &ctx,
                                    const std::string &target,
                                    const std::string &topic,
                                    CScriptDictionary *payload,
                                    int timeoutFrames) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    ScriptAsyncTaskBase *task = manager && manager->GetMessageBus()
        ? manager->GetMessageBus()->Request(SourceFromRuntimeContext(ctx), target, topic, payload, timeoutFrames, error)
        : nullptr;
    if (!task) {
        RaiseRuntimeError(ctx, error.empty() ? "Message::Request failed." : error);
    }
    return task;
}

ScriptAsyncTaskBase *RequestBehavior(const CKBehaviorContext &ctx,
                                     const std::string &target,
                                     const std::string &topic,
                                     CScriptDictionary *payload,
                                     int timeoutFrames) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    ScriptAsyncTaskBase *task = manager && manager->GetMessageBus()
        ? manager->GetMessageBus()->Request(SourceFromBehaviorContext(ctx), target, topic, payload, timeoutFrames, error)
        : nullptr;
    if (!task) {
        RaiseBehaviorError(ctx, error.empty() ? "Message::Request failed." : error);
    }
    return task;
}

bool ReplyRuntime(const ScriptContext &ctx, const ScriptMessage &request, CScriptDictionary *payload) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Reply(SourceFromRuntimeContext(ctx), request, payload, error);
    if (!ok) {
        RaiseRuntimeError(ctx, error.empty() ? "Message::Reply failed." : error);
    }
    return ok;
}

bool ReplyBehavior(const CKBehaviorContext &ctx, const ScriptMessage &request, CScriptDictionary *payload) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Reply(SourceFromBehaviorContext(ctx), request, payload, error);
    if (!ok) {
        RaiseBehaviorError(ctx, error.empty() ? "Message::Reply failed." : error);
    }
    return ok;
}

bool RejectRuntime(const ScriptContext &ctx, const ScriptMessage &request, const std::string &message) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Reject(SourceFromRuntimeContext(ctx), request, message, error);
    if (!ok) {
        RaiseRuntimeError(ctx, error.empty() ? "Message::Reject failed." : error);
    }
    return ok;
}

bool RejectBehavior(const CKBehaviorContext &ctx, const ScriptMessage &request, const std::string &message) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Reject(SourceFromBehaviorContext(ctx), request, message, error);
    if (!ok) {
        RaiseBehaviorError(ctx, error.empty() ? "Message::Reject failed." : error);
    }
    return ok;
}

bool SubscribeRuntime(const ScriptContext &ctx, const std::string &topic) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Subscribe(SourceFromRuntimeContext(ctx), topic, false, error);
    if (!ok) {
        RaiseRuntimeError(ctx, error);
    }
    return ok;
}

bool SubscribeBehavior(const CKBehaviorContext &ctx, const std::string &topic) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Subscribe(SourceFromBehaviorContext(ctx), topic, false, error);
    if (!ok) {
        RaiseBehaviorError(ctx, error);
    }
    return ok;
}

bool UnsubscribeRuntime(const ScriptContext &ctx, const std::string &topic) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Unsubscribe(SourceFromRuntimeContext(ctx), topic);
}

bool UnsubscribeBehavior(const CKBehaviorContext &ctx, const std::string &topic) {
    ScriptManager *manager = ManagerFromBehaviorContext(ctx);
    return manager && manager->GetMessageBus() &&
        manager->GetMessageBus()->Unsubscribe(SourceFromBehaviorContext(ctx), topic);
}

} // namespace

ScriptMessage::ScriptMessage() = default;

ScriptMessage::ScriptMessage(std::uint64_t id,
                             ScriptMessageKind kind,
                             std::string topic,
                             std::string source,
                             ScriptMessageTarget target,
                             std::uint64_t frameIndex,
                             bool requiresReply,
                             CScriptDictionary *payload)
    : m_Id(id),
      m_Kind(kind),
      m_Topic(std::move(topic)),
      m_Source(std::move(source)),
      m_Target(std::move(target)),
      m_FrameIndex(frameIndex),
      m_RequiresReply(requiresReply) {
    SetPayload(payload);
}

ScriptMessage::ScriptMessage(const ScriptMessage &other)
    : m_Id(other.m_Id),
      m_Kind(other.m_Kind),
      m_Topic(other.m_Topic),
      m_Source(other.m_Source),
      m_Target(other.m_Target),
      m_FrameIndex(other.m_FrameIndex),
      m_RequiresReply(other.m_RequiresReply) {
    SetPayload(other.m_Payload);
}

ScriptMessage &ScriptMessage::operator=(const ScriptMessage &other) {
    if (this == &other) {
        return *this;
    }
    m_Id = other.m_Id;
    m_Kind = other.m_Kind;
    m_Topic = other.m_Topic;
    m_Source = other.m_Source;
    m_Target = other.m_Target;
    m_FrameIndex = other.m_FrameIndex;
    m_RequiresReply = other.m_RequiresReply;
    SetPayload(other.m_Payload);
    return *this;
}

ScriptMessage::ScriptMessage(ScriptMessage &&other) noexcept
    : m_Id(other.m_Id),
      m_Kind(std::move(other.m_Kind)),
      m_Topic(std::move(other.m_Topic)),
      m_Source(std::move(other.m_Source)),
      m_Target(std::move(other.m_Target)),
      m_FrameIndex(other.m_FrameIndex),
      m_RequiresReply(other.m_RequiresReply),
      m_Payload(other.m_Payload) {
    other.m_Id = 0;
    other.m_FrameIndex = 0;
    other.m_RequiresReply = false;
    other.m_Payload = nullptr;
}

ScriptMessage &ScriptMessage::operator=(ScriptMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    SetPayload(nullptr);
    m_Id = other.m_Id;
    m_Kind = std::move(other.m_Kind);
    m_Topic = std::move(other.m_Topic);
    m_Source = std::move(other.m_Source);
    m_Target = std::move(other.m_Target);
    m_FrameIndex = other.m_FrameIndex;
    m_RequiresReply = other.m_RequiresReply;
    m_Payload = other.m_Payload;
    other.m_Id = 0;
    other.m_FrameIndex = 0;
    other.m_RequiresReply = false;
    other.m_Payload = nullptr;
    return *this;
}

ScriptMessage::~ScriptMessage() {
    SetPayload(nullptr);
}

std::uint64_t ScriptMessage::Id() const { return m_Id; }
std::string ScriptMessage::Kind() const {
    switch (m_Kind) {
    case ScriptMessageKind::Direct:
        return "direct";
    case ScriptMessageKind::Request:
        return "request";
    case ScriptMessageKind::Event:
    default:
        return "event";
    }
}
ScriptMessageKind ScriptMessage::KindValue() const { return m_Kind; }
std::string ScriptMessage::Topic() const { return m_Topic; }
std::string ScriptMessage::Source() const { return m_Source; }
std::string ScriptMessage::Target() const { return m_Target.Text; }
std::uint64_t ScriptMessage::FrameIndex() const { return m_FrameIndex; }
bool ScriptMessage::RequiresReply() const { return m_RequiresReply; }

CScriptDictionary *ScriptMessage::Payload() const {
    if (m_Payload) {
        m_Payload->AddRef();
    }
    return m_Payload;
}

void ScriptMessage::SetPayload(CScriptDictionary *payload) {
    if (payload) {
        payload->AddRef();
    }
    if (m_Payload) {
        m_Payload->Release();
    }
    m_Payload = payload;
}

ScriptMessageBus::ScriptMessageBus(ScriptManager *manager) : m_Manager(manager) {}

ScriptMessageBus::~ScriptMessageBus() {
    Clear();
}

bool ScriptMessageBus::Publish(const std::string &source,
                               const std::string &topic,
                               CScriptDictionary *payload,
                               const std::string &target,
                               std::string &error) {
    const std::string cleanTopic = TrimTopic(topic);
    if (cleanTopic.empty()) {
        error = "Message topic is empty.";
        return false;
    }
    ScriptMessageTarget parsedTarget = ParseTarget(target, true, error);
    if (parsedTarget.Kind == ScriptMessageTargetKind::Invalid) {
        return false;
    }
    m_Queue.push_back(MakeMessage(ScriptMessageKind::Event, cleanTopic, source, std::move(parsedTarget), false, payload));
    return true;
}

bool ScriptMessageBus::Send(const std::string &source,
                            const std::string &target,
                            const std::string &topic,
                            CScriptDictionary *payload,
                            std::string &error) {
    const std::string cleanTopic = TrimTopic(topic);
    if (target.empty()) {
        error = "Message::Send requires a target.";
        return false;
    }
    if (cleanTopic.empty()) {
        error = "Message topic is empty.";
        return false;
    }
    ScriptMessageTarget parsedTarget = ParseTarget(target, false, error);
    if (parsedTarget.Kind == ScriptMessageTargetKind::Invalid) {
        return false;
    }
    ScriptMessage message = MakeMessage(ScriptMessageKind::Direct, cleanTopic, source, std::move(parsedTarget), false, payload);
    return Deliver(message, true, error);
}

ScriptAsyncTaskBase *ScriptMessageBus::Request(const std::string &source,
                                               const std::string &target,
                                               const std::string &topic,
                                               CScriptDictionary *payload,
                                               int timeoutFrames,
                                               std::string &error) {
    if (!m_Manager || !m_Manager->GetAsyncScheduler() || !m_Manager->GetScriptEngine()) {
        error = "Message bus requires an initialized script manager and async scheduler.";
        return nullptr;
    }
    const std::string cleanTopic = TrimTopic(topic);
    if (target.empty()) {
        error = "Message::Request requires a target.";
        return nullptr;
    }
    if (cleanTopic.empty()) {
        error = "Message topic is empty.";
        return nullptr;
    }
    ScriptMessageTarget parsedTarget = ParseTarget(target, false, error);
    if (parsedTarget.Kind == ScriptMessageTargetKind::Invalid) {
        return nullptr;
    }
    const int dictionaryType = DictionaryHandleTypeId();
    if (dictionaryType <= 0) {
        error = "Message bus could not resolve dictionary@.";
        return nullptr;
    }
    ScriptAsyncTaskBase *task = m_Manager->GetAsyncScheduler()->CreateManualTask(dictionaryType);
    ScriptMessage message = MakeMessage(ScriptMessageKind::Request, cleanTopic, source, std::move(parsedTarget), true, payload);
    PendingRequest pending;
    pending.Task = task;
    pending.Task->AddRef();
    pending.Source = source;
    pending.Target = message.Target();
    pending.StartedFrame = m_FrameIndex;
    pending.TimeoutFrames = timeoutFrames;
    pending.DueFrame = timeoutFrames > 0
        ? m_FrameIndex + static_cast<std::uint64_t>(timeoutFrames)
        : 0;
    m_PendingRequests[message.Id()] = pending;
    IndexPendingRequest(message.Id(), pending);
    if (pending.DueFrame > 0) {
        m_Timeouts.push(TimeoutEntry{pending.DueFrame, message.Id()});
    }
    if (!Deliver(message, true, error)) {
        auto it = m_PendingRequests.find(message.Id());
        if (it != m_PendingRequests.end()) {
            UnindexPendingRequest(it->first, it->second);
            ReleasePending(it->second);
            m_PendingRequests.erase(it);
        }
        task->Fail(error.empty() ? "Message request delivery failed." : error);
    }
    return task;
}

bool ScriptMessageBus::Reply(const std::string &source, const ScriptMessage &request, CScriptDictionary *payload, std::string &error) {
    auto it = m_PendingRequests.find(request.Id());
    if (it == m_PendingRequests.end()) {
        error = "Message request is no longer pending.";
        return false;
    }
    if (request.Target() != source) {
        error = "Only the request target can reply.";
        return false;
    }
    CScriptDictionary *copy = ClonePayload(payload);
    void *handle = copy;
    it->second.Task->CompleteFromAddress(&handle, it->second.Task->SubTypeId());
    if (copy) {
        copy->Release();
    }
    UnindexPendingRequest(it->first, it->second);
    ReleasePending(it->second);
    m_PendingRequests.erase(it);
    return true;
}

bool ScriptMessageBus::Reject(const std::string &source, const ScriptMessage &request, const std::string &message, std::string &error) {
    auto it = m_PendingRequests.find(request.Id());
    if (it == m_PendingRequests.end()) {
        error = "Message request is no longer pending.";
        return false;
    }
    if (request.Target() != source) {
        error = "Only the request target can reject.";
        return false;
    }
    it->second.Task->Fail(message.empty() ? "Message request rejected." : message);
    UnindexPendingRequest(it->first, it->second);
    ReleasePending(it->second);
    m_PendingRequests.erase(it);
    return true;
}

bool ScriptMessageBus::Subscribe(const std::string &target, const std::string &topic, bool isStatic, std::string &error) {
    const std::string cleanTopic = TrimTopic(topic);
    if (target.empty()) {
        error = "Message subscription target is empty.";
        return false;
    }
    if (cleanTopic.empty()) {
        error = "Message topic is empty.";
        return false;
    }
    ScriptMessageTarget parsedTarget = ParseTarget(target, false, error);
    if (parsedTarget.Kind == ScriptMessageTargetKind::Invalid) {
        return false;
    }
    SubscriptionSet &set = m_Subscriptions[target];
    (isStatic ? set.StaticTopics : set.DynamicTopics).insert(cleanTopic);
    TopicTargets &topicTargets = m_TopicSubscriptions[cleanTopic];
    const auto exists = std::find_if(topicTargets.Targets.begin(), topicTargets.Targets.end(), [&](const ScriptMessageTarget &candidate) {
        return candidate.Text == target;
    });
    if (exists == topicTargets.Targets.end()) {
        topicTargets.Targets.push_back(std::move(parsedTarget));
        topicTargets.Dirty = true;
    }
    return true;
}

bool ScriptMessageBus::Unsubscribe(const std::string &target, const std::string &topic) {
    const std::string cleanTopic = TrimTopic(topic);
    auto it = m_Subscriptions.find(target);
    if (it == m_Subscriptions.end()) {
        return false;
    }
    const bool removed = it->second.DynamicTopics.erase(cleanTopic) > 0;
    if (removed && it->second.StaticTopics.find(cleanTopic) == it->second.StaticTopics.end()) {
        RemoveTopicTarget(cleanTopic, target);
    }
    return removed;
}

void ScriptMessageBus::ClearDynamicSubscriptions(const std::string &target) {
    auto it = m_Subscriptions.find(target);
    if (it != m_Subscriptions.end()) {
        for (const std::string &topic : it->second.DynamicTopics) {
            if (it->second.StaticTopics.find(topic) == it->second.StaticTopics.end()) {
                RemoveTopicTarget(topic, target);
            }
        }
        it->second.DynamicTopics.clear();
    }
}

void ScriptMessageBus::ClearTarget(const std::string &target, const std::string &error) {
    auto it = m_Subscriptions.find(target);
    if (it != m_Subscriptions.end()) {
        for (const std::string &topic : it->second.StaticTopics) {
            RemoveTopicTarget(topic, target);
        }
        for (const std::string &topic : it->second.DynamicTopics) {
            RemoveTopicTarget(topic, target);
        }
        m_Subscriptions.erase(it);
    }
    FailPendingForTarget(target, error.empty() ? "Message target was removed." : error);
}

void ScriptMessageBus::Clear() {
    m_Queue.clear();
    m_DrainQueue.clear();
    for (auto &entry : m_PendingRequests) {
        entry.second.Task->Fail("Message bus was cleared.");
        ReleasePending(entry.second);
    }
    m_PendingRequests.clear();
    m_PendingByTarget.clear();
    m_Timeouts = {};
    m_Subscriptions.clear();
    m_TopicSubscriptions.clear();
}

void ScriptMessageBus::Tick() {
    ++m_FrameIndex;
    while (!m_Timeouts.empty() && m_Timeouts.top().DueFrame <= m_FrameIndex) {
        const TimeoutEntry entry = m_Timeouts.top();
        m_Timeouts.pop();
        m_PerfStats.PendingTimeoutChecks++;
        auto it = m_PendingRequests.find(entry.Id);
        if (it != m_PendingRequests.end() && it->second.DueFrame == entry.DueFrame) {
            it->second.Task->Fail("Message request timed out.");
            UnindexPendingRequest(it->first, it->second);
            ReleasePending(it->second);
            m_PendingRequests.erase(it);
        }
    }

    if (m_Draining || m_Queue.empty()) {
        return;
    }
    m_Draining = true;
    m_DrainQueue.clear();
    m_DrainQueue.swap(m_Queue);
    for (const ScriptMessage &message : m_DrainQueue) {
        std::string error;
        Deliver(message, false, error);
    }
    m_DrainQueue.clear();
    m_Draining = false;
}

std::string ScriptMessageBus::RuntimeTarget(const std::string &scriptId) {
    return "runtime:" + scriptId;
}

std::string ScriptMessageBus::ComponentTarget(CK_ID id) {
    return "component:" + std::to_string(id);
}

CScriptDictionary *ScriptMessageBus::ClonePayload(CScriptDictionary *payload) const {
    if (!payload || !m_Manager || !m_Manager->GetScriptEngine()) {
        return nullptr;
    }
    CScriptDictionary *copy = CScriptDictionary::Create(m_Manager->GetScriptEngine());
    if (copy) {
        *copy = *payload;
    }
    return copy;
}

ScriptMessageTarget ScriptMessageBus::ParseTarget(const std::string &target, bool allowEmpty, std::string &error) {
    m_PerfStats.TargetParses++;
    ScriptMessageTarget parsed;
    parsed.Text = target;
    if (target.empty()) {
        if (allowEmpty) {
            return parsed;
        }
        parsed.Kind = ScriptMessageTargetKind::Invalid;
        error = "Message target is empty.";
        return parsed;
    }
    if (target.rfind("runtime:", 0) == 0) {
        parsed.RuntimeId = target.substr(8);
        if (parsed.RuntimeId.empty()) {
            parsed.Kind = ScriptMessageTargetKind::Invalid;
            error = "Runtime message target is missing a script id.";
            return parsed;
        }
        parsed.Kind = ScriptMessageTargetKind::Runtime;
        return parsed;
    }
    if (target.rfind("component:", 0) == 0) {
        const char *begin = target.c_str() + 10;
        char *end = nullptr;
        const unsigned long id = std::strtoul(begin, &end, 10);
        if (begin == end || (end && *end != '\0')) {
            parsed.Kind = ScriptMessageTargetKind::Invalid;
            error = "Component message target has an invalid CK_ID.";
            return parsed;
        }
        parsed.ComponentId = static_cast<CK_ID>(id);
        parsed.Kind = ScriptMessageTargetKind::Component;
        return parsed;
    }
    parsed.Kind = ScriptMessageTargetKind::Invalid;
    error = "Unsupported message target: " + target;
    return parsed;
}

int ScriptMessageBus::DictionaryHandleTypeId() {
    if (m_DictionaryHandleTypeId == 0 && m_Manager && m_Manager->GetScriptEngine()) {
        m_DictionaryHandleTypeId = m_Manager->GetScriptEngine()->GetTypeIdByDecl("dictionary@");
    }
    return m_DictionaryHandleTypeId;
}

ScriptMessage ScriptMessageBus::MakeMessage(ScriptMessageKind kind,
                                            const std::string &topic,
                                            const std::string &source,
                                            ScriptMessageTarget target,
                                            bool requiresReply,
                                            CScriptDictionary *payload) {
    CScriptDictionary *copy = ClonePayload(payload);
    ScriptMessage message(m_NextId++, kind, topic, source, std::move(target), m_FrameIndex, requiresReply, copy);
    if (copy) {
        copy->Release();
    }
    return message;
}

bool ScriptMessageBus::Deliver(const ScriptMessage &message, bool immediate, std::string &error) {
    if (!message.Target().empty()) {
        return DeliverTarget(message.m_Target, message, immediate, error);
    }
    auto it = m_TopicSubscriptions.find(message.Topic());
    if (it == m_TopicSubscriptions.end() || it->second.Targets.empty()) {
        error = message.Target().empty()
            ? fmt::format("No subscribers for message topic '{}'.", message.Topic())
            : fmt::format("Message target '{}' is not available.", message.Target());
        return message.KindValue() == ScriptMessageKind::Event;
    }
    TopicTargets &topicTargets = it->second;
    if (topicTargets.Dirty || !topicTargets.Snapshot) {
        topicTargets.Snapshot = std::make_shared<std::vector<ScriptMessageTarget>>(topicTargets.Targets);
        topicTargets.Dirty = false;
        m_PerfStats.BroadcastSnapshotBuilds++;
    }
    std::shared_ptr<std::vector<ScriptMessageTarget>> snapshot = topicTargets.Snapshot;
    bool ok = true;
    for (const ScriptMessageTarget &target : *snapshot) {
        std::string targetError;
        if (!DeliverTarget(target, message, immediate, targetError)) {
            ok = false;
            if (error.empty()) {
                error = targetError;
            }
        }
    }
    return ok;
}

bool ScriptMessageBus::DeliverTarget(const ScriptMessageTarget &target, const ScriptMessage &message, bool immediate, std::string &error) {
    if (target.Kind == ScriptMessageTargetKind::Runtime) {
        return m_Manager && m_Manager->GetRuntime() &&
            m_Manager->GetRuntime()->DeliverMessage(target.RuntimeId, message, immediate, error);
    }
    if (target.Kind == ScriptMessageTargetKind::Component) {
        return m_Manager && m_Manager->DeliverComponentMessage(target.ComponentId, message, immediate, error);
    }
    error = "Unsupported message target: " + target.Text;
    return false;
}

void ScriptMessageBus::RemoveTopicTarget(const std::string &topic, const std::string &target) {
    auto it = m_TopicSubscriptions.find(topic);
    if (it == m_TopicSubscriptions.end()) {
        return;
    }
    TopicTargets &topicTargets = it->second;
    const auto oldSize = topicTargets.Targets.size();
    topicTargets.Targets.erase(std::remove_if(topicTargets.Targets.begin(), topicTargets.Targets.end(), [&](const ScriptMessageTarget &candidate) {
        return candidate.Text == target;
    }), topicTargets.Targets.end());
    if (topicTargets.Targets.size() != oldSize) {
        topicTargets.Dirty = true;
    }
    if (topicTargets.Targets.empty()) {
        m_TopicSubscriptions.erase(it);
    }
}

void ScriptMessageBus::FailPendingForTarget(const std::string &target, const std::string &error) {
    auto targetIt = m_PendingByTarget.find(target);
    if (targetIt == m_PendingByTarget.end()) {
        return;
    }
    std::vector<std::uint64_t> ids(targetIt->second.begin(), targetIt->second.end());
    for (std::uint64_t id : ids) {
        auto it = m_PendingRequests.find(id);
        if (it != m_PendingRequests.end()) {
            it->second.Task->Fail(error);
            UnindexPendingRequest(it->first, it->second);
            ReleasePending(it->second);
            m_PendingRequests.erase(it);
        }
    }
}

ScriptMessageBusPerfStats ScriptMessageBus::PerfStats() const {
    return m_PerfStats;
}

void ScriptMessageBus::ResetPerfStats() {
    m_PerfStats = {};
}

void ScriptMessageBus::IndexPendingRequest(std::uint64_t id, const PendingRequest &pending) {
    if (!pending.Source.empty()) {
        m_PendingByTarget[pending.Source].insert(id);
    }
    if (!pending.Target.empty()) {
        m_PendingByTarget[pending.Target].insert(id);
    }
}

void ScriptMessageBus::UnindexPendingRequest(std::uint64_t id, const PendingRequest &pending) {
    auto remove = [&](const std::string &target) {
        auto it = m_PendingByTarget.find(target);
        if (it == m_PendingByTarget.end()) {
            return;
        }
        it->second.erase(id);
        if (it->second.empty()) {
            m_PendingByTarget.erase(it);
        }
    };
    if (!pending.Source.empty()) {
        remove(pending.Source);
    }
    if (!pending.Target.empty()) {
        remove(pending.Target);
    }
}

void ScriptMessageBus::ReleasePending(PendingRequest &pending) {
    if (pending.Task) {
        pending.Task->Release();
        pending.Task = nullptr;
    }
}

void RegisterScriptMessage(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;
    r = engine->RegisterObjectType("ScriptMessage", sizeof(ScriptMessage), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptMessage", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptMessage *self) { new(self) ScriptMessage(); }, (ScriptMessage *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptMessage", asBEHAVE_CONSTRUCT, "void f(const ScriptMessage &in other)",
                                        asFUNCTIONPR([](const ScriptMessage &other, ScriptMessage *self) { new(self) ScriptMessage(other); },
                                                     (const ScriptMessage &, ScriptMessage *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptMessage", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptMessage *self) { self->~ScriptMessage(); }, (ScriptMessage *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "ScriptMessage &opAssign(const ScriptMessage &in other)",
                                     asMETHODPR(ScriptMessage, operator=, (const ScriptMessage &), ScriptMessage &),
                                     asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "uint64 Id() const", asMETHOD(ScriptMessage, Id), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "string Kind() const", asMETHOD(ScriptMessage, Kind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "string Topic() const", asMETHOD(ScriptMessage, Topic), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "string Source() const", asMETHOD(ScriptMessage, Source), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "string Target() const", asMETHOD(ScriptMessage, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "uint64 FrameIndex() const", asMETHOD(ScriptMessage, FrameIndex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "bool RequiresReply() const", asMETHOD(ScriptMessage, RequiresReply), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptMessage", "dictionary@ Payload() const", asMETHOD(ScriptMessage, Payload), asCALL_THISCALL); assert(r >= 0);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";
    r = engine->SetDefaultNamespace("Message"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Publish(const ScriptContext &in ctx, const string &in topic, dictionary@ payload = null, const string &in target = \"\")", asFUNCTION(PublishRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Publish(const CKBehaviorContext &in ctx, const string &in topic, dictionary@ payload = null, const string &in target = \"\")", asFUNCTION(PublishBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Send(const ScriptContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null)", asFUNCTION(SendRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Send(const CKBehaviorContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null)", asFUNCTION(SendBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("AsyncTask<dictionary@>@ Request(const ScriptContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null, int timeoutFrames = 300)", asFUNCTION(RequestRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("AsyncTask<dictionary@>@ Request(const CKBehaviorContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null, int timeoutFrames = 300)", asFUNCTION(RequestBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Reply(const ScriptContext &in ctx, const ScriptMessage &in request, dictionary@ payload = null)", asFUNCTION(ReplyRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Reply(const CKBehaviorContext &in ctx, const ScriptMessage &in request, dictionary@ payload = null)", asFUNCTION(ReplyBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Reject(const ScriptContext &in ctx, const ScriptMessage &in request, const string &in error)", asFUNCTION(RejectRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Reject(const CKBehaviorContext &in ctx, const ScriptMessage &in request, const string &in error)", asFUNCTION(RejectBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Subscribe(const ScriptContext &in ctx, const string &in topic)", asFUNCTION(SubscribeRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Subscribe(const CKBehaviorContext &in ctx, const string &in topic)", asFUNCTION(SubscribeBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Unsubscribe(const ScriptContext &in ctx, const string &in topic)", asFUNCTION(UnsubscribeRuntime), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Unsubscribe(const CKBehaviorContext &in ctx, const string &in topic)", asFUNCTION(UnsubscribeBehavior), asCALL_CDECL); assert(r >= 0);
    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}
