#include "ScriptComponentStateStore.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "ScriptBehaviorBridge.h"
#include "ScriptComponentState.h"
#include "ScriptInvoker.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"

struct ScriptComponentStateStore::Impl {
    std::unordered_map<CK_ID, std::unique_ptr<ScriptComponentState> > States;
    std::uint64_t NextMessageGeneration = 1;
};

ScriptComponentStateStore::ScriptComponentStateStore()
    : m_Impl(std::make_unique<Impl>()) {
}

ScriptComponentStateStore::~ScriptComponentStateStore() = default;

ScriptComponentState *ScriptComponentStateStore::GetOrCreate(
    CKBehavior *behavior,
    const std::string &messageTarget,
    const StateCallback &beforeReplace) {
    if (!behavior) {
        return nullptr;
    }

    const CK_ID id = behavior->GetID();
    auto it = m_Impl->States.find(id);
    if (it != m_Impl->States.end()) {
        if (it->second->Behavior == behavior) {
            return it->second.get();
        }
        if (beforeReplace) {
            beforeReplace(id, it->second.get());
        }
        m_Impl->States.erase(it);
    }

    auto state = std::make_unique<ScriptComponentState>();
    state->BehaviorId = id;
    state->MessageGeneration = m_Impl->NextMessageGeneration++;
    if (m_Impl->NextMessageGeneration == 0) {
        m_Impl->NextMessageGeneration = 1;
    }
    state->Behavior = behavior;
    state->MessageTarget = messageTarget;

    ScriptComponentState *raw = state.get();
    m_Impl->States[id] = std::move(state);
    return raw;
}

ScriptComponentState *ScriptComponentStateStore::Get(CK_ID id, CKBehavior *expectedBehavior) const {
    auto it = m_Impl->States.find(id);
    if (it == m_Impl->States.end() ||
        (expectedBehavior && it->second->Behavior != expectedBehavior)) {
        return nullptr;
    }

    return it->second.get();
}

bool ScriptComponentStateStore::Remove(CK_ID id,
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

void ScriptComponentStateStore::Clear(const StateCallback &beforeRemove) {
    for (auto &entry : m_Impl->States) {
        if (beforeRemove) {
            beforeRemove(entry.first, entry.second.get());
        }
    }
    m_Impl->States.clear();
}

ScriptComponentState *ScriptManager::GetOrCreateComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return nullptr;
    }

    return m_ComponentStates.GetOrCreate(
        behavior,
        ScriptMessageBus::ComponentTarget(behavior->GetID()),
        [this](CK_ID, ScriptComponentState *replaced) {
            ResetComponentStateRuntime(replaced, true);
        });
}

ScriptComponentState *ScriptManager::GetComponentState(CK_ID id, CKBehavior *expectedBehavior) const {
    return m_ComponentStates.Get(id, expectedBehavior);
}

void ScriptManager::ResetComponentStateRuntime(ScriptComponentState *state, bool unloadOwnedModule) {
    if (!state) {
        return;
    }

    if (m_BehaviorBridge && state->BehaviorId) {
        m_BehaviorBridge->DestroyComponentTasks(state->BehaviorId);
    }

    if (state->Invoker) {
        state->Invoker->Reset();
        delete state->Invoker;
        state->Invoker = nullptr;
    }

    state->ReleaseCachedMethods();
    state->ReleaseObject();

    if (unloadOwnedModule && state->OwnsModule() && !state->ModuleName.empty()) {
        UnloadModule(state->ModuleName.c_str(), nullptr);
    }

    state->ModuleName.clear();
    state->ScriptEntryId.clear();
    state->Bindings.clear();
    if (m_MessageBus) {
        m_MessageBus->ClearTarget(state->MessageTarget, "Component runtime was reset.");
    }
    state->MessageTopics.clear();
    state->StaticMessageSubscriptionsRegistered = false;
    state->ModuleKind = ScriptComponentModuleKind::None;
    state->Loaded = false;
    state->InitialLifecycle.Clear();
    state->InstanceEnabled = false;
    state->Failed = false;
    state->PendingDestroy = false;
    state->PendingDisableOutput = false;
    state->PendingResetRuntime = false;
}

void ScriptManager::ReleaseComponentState(CKBehavior *behavior) {
    if (!behavior) {
        return;
    }

    ScriptComponentState *nullState = nullptr;
    behavior->SetLocalParameterValue(0, &nullState);
    m_ComponentStates.Remove(
        behavior->GetID(),
        behavior,
        [this](CK_ID, ScriptComponentState *state) {
            ResetComponentStateRuntime(state, true);
        });
}

void ScriptManager::ReleaseComponentState(CK_ID id) {
    m_ComponentStates.Remove(id, nullptr, [this](CK_ID, ScriptComponentState *state) {
        ResetComponentStateRuntime(state, true);
    });
}

void ScriptManager::ClearComponentStates() {
    m_ComponentStates.Clear([this](CK_ID id, ScriptComponentState *state) {
        if (m_BehaviorBridge) {
            m_BehaviorBridge->DestroyComponentTasks(id);
        }
        ResetComponentStateRuntime(state, true);
    });
}

bool ScriptManager::DeliverComponentMessage(CK_ID id,
                                            std::uint64_t generation,
                                            const ScriptMessage &message,
                                            bool immediate,
                                            std::string &error) {
    ScriptComponentState *state = GetComponentState(id);
    if (state && generation != state->MessageGeneration) {
        error = "Component message target is stale.";
        return false;
    }
    if (!state || !state->Loaded || state->Failed || !state->InstanceEnabled || !state->Object || !state->Invoker || !state->OnMessage) {
        error = "Component message target is not ready.";
        return false;
    }
    if (state->Invoker->IsContextSuspended()) {
        error = "Component message target is busy.";
        return false;
    }
    CKBehaviorContext ctx = {};
    ctx.Context = m_Context;
    ctx.Behavior = state->Behavior;
    const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(
        state->Object,
        state->OnMessage,
        message,
        ctx,
        state->OnMessageArgument);
    if (status == ScriptInvocationStatus::Suspended) {
        state->ActiveLifecycle = state->OnMessage;
        state->ActiveLifecycleName = "OnMessage";
        if (state->Behavior) {
            state->Behavior->Activate(TRUE);
        }
        return true;
    }
    if (status == ScriptInvocationStatus::Failed) {
        error = state->Invoker->GetErrorMessage();
        if (error.empty()) {
            error = "Component message handler failed.";
        }
        state->Failed = true;
        return false;
    }
    return true;
}

