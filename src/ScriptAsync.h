#ifndef CK_SCRIPTASYNC_H
#define CK_SCRIPTASYNC_H

#include <string>
#include <unordered_map>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"
#include "ScriptRefCounted.h"

class BBTask;
class GraphTask;
class ScriptManager;

enum class ScriptAsyncTaskState {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

enum class ScriptAsyncTaskKind {
    Delay,
    Script,
    All,
    Race,
    Any,
    BBTask,
    GraphTask
};

class ScriptAsyncScheduler;

class ScriptAsyncStoredValue final {
public:
    ScriptAsyncStoredValue() = default;
    ~ScriptAsyncStoredValue();

    ScriptAsyncStoredValue(const ScriptAsyncStoredValue &) = delete;
    ScriptAsyncStoredValue &operator=(const ScriptAsyncStoredValue &) = delete;

    bool HasValue() const { return m_HasValue; }
    int TypeId() const { return m_TypeId; }

    void Clear();
    void Store(asIScriptEngine *engine, void *address, int typeId);
    bool CopyTo(void *address, int typeId, std::string &error) const;
    bool CopyFrom(const ScriptAsyncStoredValue &other, std::string &error);

private:
    asIScriptEngine *m_Engine = nullptr;
    int m_TypeId = asTYPEID_VOID;
    bool m_HasValue = false;
    std::vector<unsigned char> m_Primitive;
    void *m_Object = nullptr;
};

class ScriptAsyncTaskBase final : public RefCounted {
public:
    ScriptAsyncTaskBase(ScriptAsyncScheduler *scheduler,
                        asIScriptEngine *engine,
                        ScriptAsyncTaskKind kind,
                        int subtypeId);
    ~ScriptAsyncTaskBase() override;

    ScriptAsyncTaskBase(const ScriptAsyncTaskBase &) = delete;
    ScriptAsyncTaskBase &operator=(const ScriptAsyncTaskBase &) = delete;

    bool IsPending() const;
    bool IsRunning() const;
    bool IsCompleted() const;
    bool IsFailed() const;
    bool IsCancelled() const;
    bool IsDone() const;
    std::string Error() const;
    bool Cancel();

    ScriptAsyncTaskState State() const { return m_State; }
    ScriptAsyncTaskKind Kind() const { return m_Kind; }
    int SubTypeId() const { return m_SubTypeId; }

    void SetDelayFrames(int frames);
    void SetFunction(asIScriptFunction *function);
    void SetChildren(const std::vector<ScriptAsyncTaskBase *> &children);
    void SetBBTask(BBTask *task, const CKBehaviorContext *context = nullptr, int inputIndex = -1);
    void SetGraphTask(GraphTask *task, const CKBehaviorContext *context = nullptr);

    void Advance();
    bool CopyResultTo(void *address, int typeId, std::string &error) const;
    bool CopyResultFrom(const ScriptAsyncTaskBase *other, std::string &error);
    void CompleteVoid();
    void CompleteFromAddress(void *address, int typeId);
    void CompleteWithArray(int arrayTypeId, void *arrayHandle);
    void Fail(const std::string &error);

private:
    void SetState(ScriptAsyncTaskState state);
    void ReleaseResult();
    void ReleaseContext();
    void ReleaseChildren();
    void ReleaseBridgeTasks();
    void AdvanceDelay();
    void AdvanceScript();
    void AdvanceAll();
    void AdvanceRace();
    void AdvanceAny();
    void AdvanceBBTask();
    void AdvanceGraphTask();
    bool CompleteAllArray(std::string &error);
    std::string TaskErrorForAwait() const;

    ScriptAsyncScheduler *m_Scheduler = nullptr;
    asIScriptEngine *m_Engine = nullptr;
    ScriptAsyncTaskKind m_Kind = ScriptAsyncTaskKind::Delay;
    ScriptAsyncTaskState m_State = ScriptAsyncTaskState::Pending;
    int m_SubTypeId = asTYPEID_VOID;
    ScriptAsyncStoredValue m_Result;
    std::string m_Error;

    int m_DelayFrames = 0;
    asIScriptFunction *m_Function = nullptr;
    asIScriptContext *m_Context = nullptr;
    bool m_Started = false;
    std::vector<ScriptAsyncTaskBase *> m_Children;
    BBTask *m_BBTask = nullptr;
    GraphTask *m_GraphTask = nullptr;
    bool m_DriveBridgeTask = false;
    CKBehaviorContext m_BridgeTaskContext;
    int m_BBTaskInputIndex = -1;
};

class ScriptAsyncScheduler final {
public:
    enum class ResumeState {
        Ready,
        Pending,
        Failed
    };

    explicit ScriptAsyncScheduler(ScriptManager *manager);
    ~ScriptAsyncScheduler();

    ScriptAsyncScheduler(const ScriptAsyncScheduler &) = delete;
    ScriptAsyncScheduler &operator=(const ScriptAsyncScheduler &) = delete;

    ScriptManager *GetManager() const { return m_Manager; }
    asIScriptEngine *GetEngine() const;

    ScriptAsyncTaskBase *CreateDelay(int frames);
    ScriptAsyncTaskBase *CreateScriptTask(asIScriptFunction *function, int subtypeId);
    ScriptAsyncTaskBase *CreateAggregate(ScriptAsyncTaskKind kind,
                                         int subtypeId,
                                         const std::vector<ScriptAsyncTaskBase *> &children);
    ScriptAsyncTaskBase *CreateBBTask(BBTask *task, const CKBehaviorContext *context = nullptr, int inputIndex = -1);
    ScriptAsyncTaskBase *CreateGraphTask(GraphTask *task, const CKBehaviorContext *context = nullptr);

    void Track(ScriptAsyncTaskBase *task);
    void Tick();
    bool WaitForTask(asIScriptContext *context,
                     ScriptAsyncTaskBase *task,
                     void *outAddress,
                     int outTypeId,
                     std::string &error);
    ResumeState PrepareContextResume(asIScriptContext *context, std::string &error);
    void ForgetContext(asIScriptContext *context);
    void Clear();

private:
    struct WaitRecord {
        ScriptAsyncTaskBase *Task = nullptr;
        void *OutAddress = nullptr;
        int OutTypeId = asTYPEID_VOID;
    };

    void ReleaseWaitRecord(WaitRecord &record);
    void RemoveFinishedTrackedTasks();

    ScriptManager *m_Manager = nullptr;
    std::vector<ScriptAsyncTaskBase *> m_Tasks;
    std::unordered_map<asIScriptContext *, WaitRecord> m_Waits;
};

void RegisterScriptAsync(asIScriptEngine *engine);

#if CKAS_BUILD_SELF_TESTS
bool RunScriptAsyncSelfTest(class CKContext *context, asIScriptEngine *engine, std::string &error);
#endif

#endif // CK_SCRIPTASYNC_H
