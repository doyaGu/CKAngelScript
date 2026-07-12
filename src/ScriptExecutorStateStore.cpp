#include "ScriptExecutorStateStore.h"

#include <memory>
#include <unordered_map>

#include "ScriptExecutorState.h"

struct ScriptExecutorStateStore::Impl {
    std::unordered_map<CK_ID, std::unique_ptr<ScriptExecutorState>> States;
};

ScriptExecutorStateStore::ScriptExecutorStateStore()
    : m_Impl(std::make_unique<Impl>()) {
}

ScriptExecutorStateStore::~ScriptExecutorStateStore() = default;

ScriptExecutorState *ScriptExecutorStateStore::GetOrCreate(
    CKBehavior *behavior,
    ScriptManager *manager,
    const StateCallback &beforeReplace) {
    if (!behavior || !manager) {
        return nullptr;
    }

    const CK_ID id = behavior->GetID();
    auto it = m_Impl->States.find(id);
    if (it != m_Impl->States.end()) {
        if (it->second->Behavior == behavior) {
            it->second->Manager = manager;
            return it->second.get();
        }
        if (beforeReplace) {
            beforeReplace(id, it->second.get());
        }
        m_Impl->States.erase(it);
    }

    auto state = std::make_unique<ScriptExecutorState>();
    state->BehaviorId = id;
    state->Behavior = behavior;
    state->Manager = manager;
    ScriptExecutorState *raw = state.get();
    m_Impl->States[id] = std::move(state);
    return raw;
}

ScriptExecutorState *ScriptExecutorStateStore::Get(CK_ID id, CKBehavior *expectedBehavior) const {
    auto it = m_Impl->States.find(id);
    if (it == m_Impl->States.end() ||
        (expectedBehavior && it->second->Behavior != expectedBehavior)) {
        return nullptr;
    }
    return it->second.get();
}

bool ScriptExecutorStateStore::Remove(CK_ID id,
                                      CKBehavior *expectedBehavior,
                                      const StateCallback &beforeRemove) {
    auto it = m_Impl->States.find(id);
    if (it == m_Impl->States.end() ||
        (expectedBehavior && it->second->Behavior != expectedBehavior)) {
        return false;
    }
    if (beforeRemove) {
        beforeRemove(id, it->second.get());
    }
    m_Impl->States.erase(it);
    return true;
}

void ScriptExecutorStateStore::ReleaseModule(const std::string &moduleName,
                                             const StateCallback &beforeRemove) {
    if (moduleName.empty()) {
        return;
    }
    for (auto it = m_Impl->States.begin(); it != m_Impl->States.end();) {
        if (it->second->ModuleName != moduleName) {
            ++it;
            continue;
        }
        if (beforeRemove) {
            beforeRemove(it->first, it->second.get());
        }
        it = m_Impl->States.erase(it);
    }
}

void ScriptExecutorStateStore::Clear(const StateCallback &beforeRemove) {
    for (auto &entry : m_Impl->States) {
        if (beforeRemove) {
            beforeRemove(entry.first, entry.second.get());
        }
    }
    m_Impl->States.clear();
}
