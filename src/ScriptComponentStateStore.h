#ifndef CK_SCRIPT_COMPONENT_STATE_STORE_H
#define CK_SCRIPT_COMPONENT_STATE_STORE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "CKAll.h"

class CKBehavior;
struct ScriptComponentState;

class ScriptComponentStateStore {
public:
    using StateCallback = std::function<void(CK_ID, ScriptComponentState *)>;

    ScriptComponentStateStore();
    ~ScriptComponentStateStore();

    ScriptComponentStateStore(const ScriptComponentStateStore &) = delete;
    ScriptComponentStateStore &operator=(const ScriptComponentStateStore &) = delete;

    ScriptComponentState *GetOrCreate(CKBehavior *behavior,
                                      const std::string &messageTarget,
                                      const StateCallback &beforeReplace);
    ScriptComponentState *Get(CK_ID id, CKBehavior *expectedBehavior = nullptr) const;
    void AdvanceMessageGeneration(ScriptComponentState *state);
    bool HasModuleConsumer(const std::string &moduleName) const;
    void CollectAsyncOwnerIds(std::unordered_set<std::uint64_t> &ownerIds) const;
    bool Remove(CK_ID id, CKBehavior *expectedBehavior, const StateCallback &beforeRemove);
    void ReleaseModule(const std::string &moduleName, const StateCallback &beforeRemove);
    void Clear(const StateCallback &beforeRemove);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

#endif // CK_SCRIPT_COMPONENT_STATE_STORE_H
