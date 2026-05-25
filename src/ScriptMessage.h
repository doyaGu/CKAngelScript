#ifndef CK_SCRIPTMESSAGE_H
#define CK_SCRIPTMESSAGE_H

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <angelscript.h>

#include "CKTypes.h"

struct CKBehaviorContext;
class CScriptDictionary;
class ScriptAsyncTaskBase;
class ScriptManager;
class ScriptContext;

class ScriptMessage {
public:
    ScriptMessage();
    ScriptMessage(std::uint64_t id,
                  std::string kind,
                  std::string topic,
                  std::string source,
                  std::string target,
                  std::uint64_t frameIndex,
                  bool requiresReply,
                  CScriptDictionary *payload);
    ScriptMessage(const ScriptMessage &other);
    ScriptMessage &operator=(const ScriptMessage &other);
    ScriptMessage(ScriptMessage &&other) noexcept;
    ScriptMessage &operator=(ScriptMessage &&other) noexcept;
    ~ScriptMessage();

    std::uint64_t Id() const;
    std::string Kind() const;
    std::string Topic() const;
    std::string Source() const;
    std::string Target() const;
    std::uint64_t FrameIndex() const;
    bool RequiresReply() const;
    CScriptDictionary *Payload() const;

private:
    void SetPayload(CScriptDictionary *payload);

    std::uint64_t m_Id = 0;
    std::string m_Kind;
    std::string m_Topic;
    std::string m_Source;
    std::string m_Target;
    std::uint64_t m_FrameIndex = 0;
    bool m_RequiresReply = false;
    CScriptDictionary *m_Payload = nullptr;
};

class ScriptMessageBus {
public:
    explicit ScriptMessageBus(ScriptManager *manager);
    ~ScriptMessageBus();

    bool Publish(const std::string &source,
                 const std::string &topic,
                 CScriptDictionary *payload,
                 const std::string &target,
                 std::string &error);
    bool Send(const std::string &source,
              const std::string &target,
              const std::string &topic,
              CScriptDictionary *payload,
              std::string &error);
    ScriptAsyncTaskBase *Request(const std::string &source,
                                 const std::string &target,
                                 const std::string &topic,
                                 CScriptDictionary *payload,
                                 int timeoutFrames,
                                 std::string &error);
    bool Reply(const std::string &source, const ScriptMessage &request, CScriptDictionary *payload, std::string &error);
    bool Reject(const std::string &source, const ScriptMessage &request, const std::string &message, std::string &error);

    bool Subscribe(const std::string &target, const std::string &topic, bool isStatic, std::string &error);
    bool Unsubscribe(const std::string &target, const std::string &topic);
    void ClearDynamicSubscriptions(const std::string &target);
    void FailPendingForTarget(const std::string &target, const std::string &error);
    void ClearTarget(const std::string &target, const std::string &error = std::string());
    void Clear();
    void Tick();

    static std::string RuntimeTarget(const std::string &scriptId);
    static std::string ComponentTarget(CK_ID id);

private:
    struct SubscriptionSet {
        std::unordered_set<std::string> StaticTopics;
        std::unordered_set<std::string> DynamicTopics;
    };
    struct PendingRequest {
        ScriptAsyncTaskBase *Task = nullptr;
        std::string Source;
        std::string Target;
        std::uint64_t StartedFrame = 0;
        int TimeoutFrames = 0;
    };

    CScriptDictionary *ClonePayload(CScriptDictionary *payload) const;
    ScriptMessage MakeMessage(const std::string &kind,
                              const std::string &topic,
                              const std::string &source,
                              const std::string &target,
                              bool requiresReply,
                              CScriptDictionary *payload);
    bool Deliver(const ScriptMessage &message, bool immediate, std::string &error);
    bool DeliverTarget(const std::string &target, const ScriptMessage &message, bool immediate, std::string &error);
    void RemoveTopicTarget(const std::string &topic, const std::string &target);
    void ReleasePending(PendingRequest &pending);

    ScriptManager *m_Manager = nullptr;
    std::uint64_t m_FrameIndex = 0;
    std::uint64_t m_NextId = 1;
    bool m_Draining = false;
    std::vector<ScriptMessage> m_Queue;
    std::vector<ScriptMessage> m_DrainQueue;
    std::unordered_map<std::string, SubscriptionSet> m_Subscriptions;
    std::unordered_map<std::string, std::set<std::string>> m_TopicSubscriptions;
    std::unordered_map<std::uint64_t, PendingRequest> m_PendingRequests;
};

void RegisterScriptMessage(asIScriptEngine *engine);

#endif // CK_SCRIPTMESSAGE_H
