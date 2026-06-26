#ifndef CK_SCRIPT_COMPONENT_STATE_STORE_H
#define CK_SCRIPT_COMPONENT_STATE_STORE_H

#include <functional>
#include <memory>
#include <string>

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

    ScriptComponentState *GetOrCreate(CKBehavior *behavior, const std::string &messageTarget);
    ScriptComponentState *Get(CK_ID id) const;
    void Remove(CK_ID id, const StateCallback &beforeRemove);
    void Clear(const StateCallback &beforeRemove);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

#endif // CK_SCRIPT_COMPONENT_STATE_STORE_H
