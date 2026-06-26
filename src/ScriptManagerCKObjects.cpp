#include "ScriptManager.h"

void *ScriptManager::GetCKObjectData(CK_ID id) const {
    return m_CKObjectRetainer.GetData(id);
}

void ScriptManager::SetCKObjectData(CK_ID id, void *data) {
    m_CKObjectRetainer.SetData(id, data);
}

void ScriptManager::ReleaseCKObjectData(CK_ID id) {
    m_CKObjectRetainer.ReleaseData(id);
}

void ScriptManager::ClearCKObjectData() {
    m_CKObjectRetainer.Clear();
}

void ScriptManager::TrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    m_CKObjectRetainer.TrackCallback(id, func);
}

bool ScriptManager::UntrackCKObjectCallback(CK_ID id, asIScriptFunction *func) {
    return m_CKObjectRetainer.UntrackCallback(id, func);
}

void ScriptManager::ReleaseCKObjectCallbacks(CK_ID id) {
    m_CKObjectRetainer.ReleaseCallbacks(id);
}
