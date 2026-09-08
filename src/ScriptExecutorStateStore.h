#ifndef CK_SCRIPT_EXECUTOR_STATE_STORE_H
#define CK_SCRIPT_EXECUTOR_STATE_STORE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "CKAll.h"

class CKBehavior;
class ScriptManager;
struct ScriptExecutorState;

class ScriptExecutorStateStore final {
public:
    using StateCallback = std::function<void(CK_ID, ScriptExecutorState *)>;

    ScriptExecutorStateStore();
    ~ScriptExecutorStateStore();

    ScriptExecutorStateStore(const ScriptExecutorStateStore &) = delete;
    ScriptExecutorStateStore &operator=(const ScriptExecutorStateStore &) = delete;

    ScriptExecutorState *GetOrCreate(CKBehavior *behavior,
                                     ScriptManager *manager,
                                     const StateCallback &beforeReplace);
    ScriptExecutorState *Get(CK_ID id, CKBehavior *expectedBehavior = nullptr) const;
    bool HasModuleConsumer(const std::string &moduleName) const;
    void CollectAsyncOwnerIds(std::unordered_set<std::uint64_t> &ownerIds) const;
    bool Remove(CK_ID id, CKBehavior *expectedBehavior, const StateCallback &beforeRemove);
    void ReleaseModule(const std::string &moduleName, const StateCallback &beforeRemove);
    void Clear(const StateCallback &beforeRemove);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

#endif // CK_SCRIPT_EXECUTOR_STATE_STORE_H
