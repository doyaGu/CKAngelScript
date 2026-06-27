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
};

ScriptComponentStateStore::ScriptComponentStateStore()
    : m_Impl(std::make_unique<Impl>()) {
}

ScriptComponentStateStore::~ScriptComponentStateStore() = default;

ScriptComponentState *ScriptComponentStateStore::GetOrCreate(CKBehavior *behavior, const std::string &messageTarget) {
    if (!behavior) {
        return nullptr;
    }

    const CK_ID id = behavior->GetID();
    auto it = m_Impl->States.find(id);
    if (it != m_Impl->States.end()) {
        it->second->Behavior = behavior;
        return it->second.get();
    }

    auto state = std::make_unique<ScriptComponentState>();
    state->BehaviorId = id;
    state->Behavior = behavior;
    state->MessageTarget = messageTarget;

    ScriptComponentState *raw = state.get();
    m_Impl->States[id] = std::move(state);
    return raw;
}

ScriptComponentState *ScriptComponentStateStore::Get(CK_ID id) const {
    auto it = m_Impl->States.find(id);
    if (it == m_Impl->States.end()) {
        return nullptr;
    }

    return it->second.get();
}

void ScriptComponentStateStore::Remove(CK_ID id, const StateCallback &beforeRemove) {
    auto it = m_Impl->States.find(id);
    if (it == m_Impl->States.end()) {
        return;
    }

    if (beforeRemove) {
        beforeRemove(id, it->second.get());
    }
    m_Impl->States.erase(it);
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

    return m_ComponentStates.GetOrCreate(behavior, ScriptMessageBus::ComponentTarget(behavior->GetID()));
}

ScriptComponentState *ScriptManager::GetComponentState(CK_ID id) const {
    return m_ComponentStates.Get(id);
}

void ScriptManager::ResetComponentStateRuntime(ScriptComponentState *state, bool unloadPrivateModule) {
    if (!state) {
        return;
    }

    if (m_BehaviorBridge && state->BehaviorId) {
        m_BehaviorBridge->DestroyComponentTasks(state->BehaviorId);
    }

    state->ReleaseCachedMethods();
    state->ReleaseObject();

    if (state->Invoker) {
        state->Invoker->Reset();
        delete state->Invoker;
        state->Invoker = nullptr;
    }

    if (unloadPrivateModule && state->PrivateModule && !state->RuntimeModuleName.empty()) {
        UnloadModule(state->RuntimeModuleName.c_str(), nullptr);
    }

    state->RuntimeModuleName.clear();
    state->Bindings.clear();
    if (m_MessageBus) {
        m_MessageBus->ClearTarget(state->MessageTarget, "Component runtime was reset.");
    }
    state->MessageTopics.clear();
    state->StaticMessageSubscriptionsRegistered = false;
    state->PrivateModule = false;
    state->Loaded = false;
    state->OnLoadCalled = false;
    state->AwakeCalled = false;
    state->StartCalled = false;
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

    ScriptComponentState *state = GetComponentState(behavior->GetID());
    if (state && state->Behavior) {
        ScriptComponentState *nullState = nullptr;
        state->Behavior->SetLocalParameterValue(0, &nullState);
    }

    ReleaseComponentState(behavior->GetID());
}

void ScriptManager::ReleaseComponentState(CK_ID id) {
    m_ComponentStates.Remove(id, [this](CK_ID stateId, ScriptComponentState *state) {
        if (m_BehaviorBridge) {
            m_BehaviorBridge->DestroyComponentTasks(stateId);
        }
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

bool ScriptManager::DeliverComponentMessage(CK_ID id, const ScriptMessage &message, bool immediate, std::string &error) {
    ScriptComponentState *state = GetComponentState(id);
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
    const ScriptInvocationStatus status = state->Invoker->ExecuteObjectMethodStatus(state->Object, state->OnMessage, message, ctx);
    if (status == ScriptInvocationStatus::Suspended) {
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

