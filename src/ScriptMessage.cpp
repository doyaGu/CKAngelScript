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
    return ScriptMessageBus::RuntimeTarget(ctx.ScriptId());
}

std::string SourceFromBehaviorContext(const CKBehaviorContext &ctx) {
    return ctx.Behavior ? ScriptMessageBus::ComponentTarget(ctx.Behavior->GetID()) : std::string();
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
                             std::string kind,
                             std::string topic,
                             std::string source,
                             std::string target,
                             std::uint64_t frameIndex,
                             bool requiresReply,
                             CScriptDictionary *payload)
    : m_Id(id),
      m_Kind(std::move(kind)),
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
std::string ScriptMessage::Kind() const { return m_Kind; }
std::string ScriptMessage::Topic() const { return m_Topic; }
std::string ScriptMessage::Source() const { return m_Source; }
std::string ScriptMessage::Target() const { return m_Target; }
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
    m_Queue.push_back(MakeMessage("event", cleanTopic, source, target, false, payload));
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
    ScriptMessage message = MakeMessage("direct", cleanTopic, source, target, false, payload);
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
    const int dictionaryType = m_Manager->GetScriptEngine()->GetTypeIdByDecl("dictionary@");
    ScriptAsyncTaskBase *task = m_Manager->GetAsyncScheduler()->CreateManualTask(dictionaryType);
    ScriptMessage message = MakeMessage("request", cleanTopic, source, target, true, payload);
    PendingRequest pending;
    pending.Task = task;
    pending.Task->AddRef();
    pending.Source = source;
    pending.Target = target;
    pending.StartedFrame = m_FrameIndex;
    pending.TimeoutFrames = timeoutFrames;
    m_PendingRequests[message.Id()] = pending;
    if (!Deliver(message, true, error)) {
        auto it = m_PendingRequests.find(message.Id());
        if (it != m_PendingRequests.end()) {
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
    SubscriptionSet &set = m_Subscriptions[target];
    (isStatic ? set.StaticTopics : set.DynamicTopics).insert(cleanTopic);
    m_TopicSubscriptions[cleanTopic].insert(target);
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
    m_Subscriptions.clear();
    m_TopicSubscriptions.clear();
}

void ScriptMessageBus::Tick() {
    ++m_FrameIndex;
    std::vector<std::uint64_t> timedOut;
    for (const auto &entry : m_PendingRequests) {
        const PendingRequest &pending = entry.second;
        if (pending.TimeoutFrames > 0 &&
            m_FrameIndex >= pending.StartedFrame + static_cast<std::uint64_t>(pending.TimeoutFrames)) {
            timedOut.push_back(entry.first);
        }
    }
    for (std::uint64_t id : timedOut) {
        auto it = m_PendingRequests.find(id);
        if (it != m_PendingRequests.end()) {
            it->second.Task->Fail("Message request timed out.");
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

ScriptMessage ScriptMessageBus::MakeMessage(const std::string &kind,
                                            const std::string &topic,
                                            const std::string &source,
                                            const std::string &target,
                                            bool requiresReply,
                                            CScriptDictionary *payload) {
    CScriptDictionary *copy = ClonePayload(payload);
    ScriptMessage message(m_NextId++, kind, topic, source, target, m_FrameIndex, requiresReply, copy);
    if (copy) {
        copy->Release();
    }
    return message;
}

bool ScriptMessageBus::Deliver(const ScriptMessage &message, bool immediate, std::string &error) {
    if (!message.Target().empty()) {
        return DeliverTarget(message.Target(), message, immediate, error);
    }
    auto it = m_TopicSubscriptions.find(message.Topic());
    if (it == m_TopicSubscriptions.end() || it->second.empty()) {
        error = message.Target().empty()
            ? fmt::format("No subscribers for message topic '{}'.", message.Topic())
            : fmt::format("Message target '{}' is not available.", message.Target());
        return message.Kind() == "event";
    }
    bool ok = true;
    for (const std::string &target : it->second) {
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

bool ScriptMessageBus::DeliverTarget(const std::string &target, const ScriptMessage &message, bool immediate, std::string &error) {
    if (target.rfind("runtime:", 0) == 0) {
        return m_Manager && m_Manager->GetRuntime() &&
            m_Manager->GetRuntime()->DeliverMessage(target.substr(8), message, immediate, error);
    }
    if (target.rfind("component:", 0) == 0) {
        CK_ID id = static_cast<CK_ID>(std::strtoul(target.c_str() + 10, nullptr, 10));
        return m_Manager && m_Manager->DeliverComponentMessage(id, message, immediate, error);
    }
    error = "Unsupported message target: " + target;
    return false;
}

void ScriptMessageBus::RemoveTopicTarget(const std::string &topic, const std::string &target) {
    auto it = m_TopicSubscriptions.find(topic);
    if (it == m_TopicSubscriptions.end()) {
        return;
    }
    it->second.erase(target);
    if (it->second.empty()) {
        m_TopicSubscriptions.erase(it);
    }
}

void ScriptMessageBus::FailPendingForTarget(const std::string &target, const std::string &error) {
    std::vector<std::uint64_t> ids;
    for (const auto &entry : m_PendingRequests) {
        if (entry.second.Source == target || entry.second.Target == target) {
            ids.push_back(entry.first);
        }
    }
    for (std::uint64_t id : ids) {
        auto it = m_PendingRequests.find(id);
        if (it != m_PendingRequests.end()) {
            it->second.Task->Fail(error);
            ReleasePending(it->second);
            m_PendingRequests.erase(it);
        }
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
