#include "ScriptCKObjectRetainer.h"

#include "ScriptUtils.h"

void *ScriptCKObjectRetainer::GetData(CK_ID id) const {
    const auto it = m_Data.find(id);
    if (it == m_Data.end()) {
        return nullptr;
    }
    return it->second;
}

void ScriptCKObjectRetainer::SetData(CK_ID id, void *data) {
    if (data) {
        m_Data[id] = data;
    } else {
        m_Data.erase(id);
    }
}

void ScriptCKObjectRetainer::ReleaseData(CK_ID id) {
    const auto it = m_Data.find(id);
    if (it == m_Data.end()) {
        return;
    }

    auto *func = static_cast<asIScriptFunction *>(it->second);
    m_Data.erase(it);
    if (func) {
        func->Release();
    }
}

void ScriptCKObjectRetainer::Clear() {
    for (const auto &entry : m_Data) {
        auto *func = static_cast<asIScriptFunction *>(entry.second);
        if (func) {
            func->Release();
        }
    }
    m_Data.clear();

    for (auto &entry : m_Callbacks) {
        ReleaseCallbackList(std::move(entry.second));
    }
    m_Callbacks.clear();
}

void ScriptCKObjectRetainer::TrackCallback(CK_ID id, asIScriptFunction *func) {
    if (func) {
        m_Callbacks[id].push_back(func);
    }
}

bool ScriptCKObjectRetainer::UntrackCallback(CK_ID id, asIScriptFunction *func) {
    auto it = m_Callbacks.find(id);
    if (it == m_Callbacks.end()) {
        return false;
    }

    auto &callbacks = it->second;
    bool removed = false;
    for (auto cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (*cb == func) {
            callbacks.erase(cb);
            removed = true;
            break;
        }
    }

    if (callbacks.empty()) {
        m_Callbacks.erase(it);
    }
    return removed;
}

void ScriptCKObjectRetainer::ReleaseCallbacks(CK_ID id) {
    auto it = m_Callbacks.find(id);
    if (it == m_Callbacks.end()) {
        return;
    }

    auto callbacks = std::move(it->second);
    m_Callbacks.erase(it);
    ReleaseCallbackList(std::move(callbacks));
}

void ScriptCKObjectRetainer::ReleaseCallbackList(std::vector<asIScriptFunction *> callbacks) {
    for (auto *func : callbacks) {
        if (!func) {
            continue;
        }
        if (IsMarkedAsReleasedOnce(func)) {
            ClearReleasedOnceMark(func);
        } else {
            if (IsMarkedAsTemporary(func)) {
                ClearTemporaryMark(func);
            }
            func->Release();
        }
    }
}
