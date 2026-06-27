#include "ScriptModuleStateStore.h"

#include <algorithm>

#include "ScriptApiSupport.h"

namespace {

bool ImportBindingLess(const ScriptImportBindingEdge &lhs,
                       const ScriptImportBindingEdge &rhs) {
    if (lhs.ImportModuleName != rhs.ImportModuleName) {
        return lhs.ImportModuleName < rhs.ImportModuleName;
    }
    if (lhs.ImportIndex != rhs.ImportIndex) {
        return lhs.ImportIndex < rhs.ImportIndex;
    }
    if (lhs.SourceModuleName != rhs.SourceModuleName) {
        return lhs.SourceModuleName < rhs.SourceModuleName;
    }
    return lhs.FunctionDecl < rhs.FunctionDecl;
}

void SortImportBindings(std::vector<ScriptImportBindingEdge> &bindings) {
    std::sort(bindings.begin(), bindings.end(), ImportBindingLess);
}

} // namespace

void ScriptModuleStateStore::Clear() {
    m_States.clear();
    m_ModuleOrder.clear();
}

CKDWORD ScriptModuleStateStore::GetGeneration(const char *moduleName) const {
    const ModuleState *state = FindState(moduleName);
    return state ? state->Generation : 0;
}

void ScriptModuleStateStore::BumpGeneration(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    CKDWORD &generation = EnsureState(moduleName).Generation;
    ++generation;
    if (generation == 0) {
        generation = 1;
    }
    MarkDirty(moduleName);
}

void ScriptModuleStateStore::SetKind(const char *moduleName, ScriptModuleKind kind) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    ModuleState &state = EnsureState(moduleName);
    if (state.Kind != kind) {
        state.Kind = kind;
        state.FingerprintDirty = true;
    }
    if (kind == ScriptModuleKind::RawUnknown) {
        MarkModuleUnloaded(moduleName);
    } else {
        MarkModuleLoaded(moduleName);
    }
}

void ScriptModuleStateStore::SetIncludeEdges(const char *moduleName,
                                             const std::vector<ScriptIncludeEdge> &includeEdges) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    ModuleState &state = EnsureState(moduleName);
    state.IncludeEdges = includeEdges;
    state.FingerprintDirty = true;
}

void ScriptModuleStateStore::ClearIncludeEdges(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    ModuleState *state = FindState(moduleName);
    if (!state || state->IncludeEdges.empty()) {
        return;
    }
    state->IncludeEdges.clear();
    state->FingerprintDirty = true;
}

const std::vector<ScriptIncludeEdge> *ScriptModuleStateStore::FindIncludeEdges(
    const char *moduleName) const {
    const ModuleState *state = FindState(moduleName);
    return state ? &state->IncludeEdges : nullptr;
}

bool ScriptModuleStateStore::HasBoundImportConsumersForModule(const char *moduleName,
                                                              std::string *consumerModule) const {
    if (consumerModule) {
        consumerModule->clear();
    }
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    for (const std::string &stateModuleName : m_ModuleOrder) {
        const auto stateIt = m_States.find(stateModuleName);
        if (stateIt == m_States.end()) {
            continue;
        }
        for (const ScriptImportBindingEdge &edge : stateIt->second.BoundImports) {
            if (edge.SourceModuleName == moduleName && edge.ImportModuleName != moduleName) {
                if (consumerModule) {
                    *consumerModule = edge.ImportModuleName;
                }
                return true;
            }
        }
    }
    return false;
}

std::vector<ScriptImportBindingEdge> ScriptModuleStateStore::GetImportBindingsForModule(
    const char *moduleName) const {
    const ModuleState *state = FindState(moduleName);
    return state ? state->BoundImports : std::vector<ScriptImportBindingEdge>();
}

std::vector<ScriptImportBindingEdge> ScriptModuleStateStore::GetImportBindingForModuleIndex(
    const char *moduleName,
    CKDWORD importIndex) const {
    std::vector<ScriptImportBindingEdge> bindings;
    const ModuleState *state = FindState(moduleName);
    if (!state) {
        return bindings;
    }
    for (const ScriptImportBindingEdge &edge : state->BoundImports) {
        if (edge.ImportModuleName == moduleName && edge.ImportIndex == importIndex) {
            bindings.push_back(edge);
            break;
        }
    }
    return bindings;
}

bool ScriptModuleStateStore::RemoveImportBinding(const char *moduleName, CKDWORD importIndex) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    ModuleState *state = FindState(moduleName);
    if (!state) {
        return false;
    }
    const auto oldSize = state->BoundImports.size();
    state->BoundImports.erase(std::remove_if(state->BoundImports.begin(),
                                             state->BoundImports.end(),
                                             [moduleName, importIndex](const ScriptImportBindingEdge &edge) {
                                                 return edge.ImportModuleName == moduleName &&
                                                        edge.ImportIndex == importIndex;
                                             }),
                              state->BoundImports.end());
    const bool removed = state->BoundImports.size() != oldSize;
    if (removed) {
        state->FingerprintDirty = true;
        if (state->Kind == ScriptModuleKind::RawUnknown && state->BoundImports.empty()) {
            MarkModuleUnloaded(moduleName);
        }
    }
    return removed;
}

bool ScriptModuleStateStore::RemoveImportBindingsForModule(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return false;
    }
    ModuleState *state = FindState(moduleName);
    if (!state || state->BoundImports.empty()) {
        return false;
    }
    state->BoundImports.clear();
    state->FingerprintDirty = true;
    if (state->Kind == ScriptModuleKind::RawUnknown) {
        MarkModuleUnloaded(moduleName);
    }
    return true;
}

void ScriptModuleStateStore::RestoreImportBindingsForModule(
    const char *moduleName,
    const std::vector<ScriptImportBindingEdge> &bindings) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    ModuleState &state = EnsureState(moduleName);
    state.BoundImports.clear();
    for (const ScriptImportBindingEdge &edge : bindings) {
        if (edge.ImportModuleName == moduleName) {
            state.BoundImports.push_back(edge);
        }
    }
    SortImportBindings(state.BoundImports);
    state.FingerprintDirty = true;
    if (state.BoundImports.empty()) {
        if (state.Kind == ScriptModuleKind::RawUnknown) {
            MarkModuleUnloaded(moduleName);
        }
    } else {
        EnsureModuleOrdered(moduleName);
    }
}

void ScriptModuleStateStore::RecordImportBinding(const char *importModuleName,
                                                 CKDWORD importIndex,
                                                 const char *sourceModuleName,
                                                 const char *functionDecl) {
    if (!ScriptApiSupport::IsNonEmpty(importModuleName)) {
        return;
    }
    ScriptImportBindingEdge edge;
    edge.ImportModuleName = importModuleName;
    edge.ImportIndex = importIndex;
    edge.SourceModuleName = sourceModuleName ? sourceModuleName : "";
    edge.FunctionDecl = functionDecl ? functionDecl : "";
    ModuleState &state = EnsureState(importModuleName);
    RemoveImportBinding(importModuleName, importIndex);
    state.BoundImports.push_back(edge);
    SortImportBindings(state.BoundImports);
    state.FingerprintDirty = true;
    EnsureModuleOrdered(importModuleName);
}

void ScriptModuleStateStore::MarkDirty(const char *moduleName) {
    ModuleState *state = FindState(moduleName);
    if (state) {
        state->FingerprintDirty = true;
    }
}

CKAngelScriptModuleFingerprint ScriptModuleStateStore::GetFingerprint(
    const char *moduleName,
    CKDWORD apiVersion,
    const char *angelScriptVersion,
    const char *angelScriptOptions,
    unsigned long long sourceHash,
    unsigned long long declaredImportHash) {
    ModuleState &state = EnsureState(moduleName);
    const bool fingerprintInputsChanged =
        !state.FingerprintInputsValid ||
        state.FingerprintSourceHash != sourceHash ||
        state.FingerprintDeclaredImportHash != declaredImportHash;
    if (state.FingerprintDirty || fingerprintInputsChanged) {
        CKAngelScriptModuleFingerprint fingerprint;
        CKAngelScriptInitModuleFingerprint(&fingerprint);
        fingerprint.Kind = ToPublicKind(state.Kind);
        fingerprint.Generation = state.Generation;
        fingerprint.ApiVersion = apiVersion;
        fingerprint.AngelScriptVersion = angelScriptVersion;
        fingerprint.AngelScriptOptions = angelScriptOptions;
        fingerprint.SourceHash = sourceHash;
        fingerprint.DeclaredImportHash = declaredImportHash;

        fingerprint.IncludeHash = ScriptApiSupport::kFnvOffsetBasis;
        ScriptApiSupport::HashValue(fingerprint.IncludeHash,
                                    static_cast<unsigned long long>(state.IncludeEdges.size()));
        for (const ScriptIncludeEdge &edge : state.IncludeEdges) {
            ScriptApiSupport::HashString(fingerprint.IncludeHash, edge.FromSection);
            ScriptApiSupport::HashString(fingerprint.IncludeHash, edge.ToSection);
            ScriptApiSupport::HashBool(fingerprint.IncludeHash, edge.ResolvedFromSnapshot);
        }

        fingerprint.BoundImportHash = ScriptApiSupport::kFnvOffsetBasis;
        ScriptApiSupport::HashValue(fingerprint.BoundImportHash,
                                    static_cast<unsigned long long>(state.BoundImports.size()));
        for (const ScriptImportBindingEdge &edge : state.BoundImports) {
            ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.ImportModuleName);
            ScriptApiSupport::HashValue(fingerprint.BoundImportHash, edge.ImportIndex);
            ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.SourceModuleName);
            ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.FunctionDecl);
        }

        fingerprint.CombinedHash = ScriptApiSupport::kFnvOffsetBasis;
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.ApiVersion);
        ScriptApiSupport::HashString(fingerprint.CombinedHash, fingerprint.AngelScriptVersion);
        ScriptApiSupport::HashString(fingerprint.CombinedHash, fingerprint.AngelScriptOptions);
        ScriptApiSupport::HashValue(fingerprint.CombinedHash,
                                    static_cast<CKDWORD>(fingerprint.Kind));
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.Generation);
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.SourceHash);
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.IncludeHash);
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.DeclaredImportHash);
        ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.BoundImportHash);

        state.Fingerprint = fingerprint;
        state.FingerprintSourceHash = sourceHash;
        state.FingerprintDeclaredImportHash = declaredImportHash;
        state.FingerprintInputsValid = true;
        state.FingerprintDirty = false;
    }
    return state.Fingerprint;
}

CKAS_MODULEKIND ScriptModuleStateStore::ToPublicKind(ScriptModuleKind kind) {
    switch (kind) {
        case ScriptModuleKind::Source:
            return CKAS_MODULEKIND_SOURCE;
        case ScriptModuleKind::Bytecode:
            return CKAS_MODULEKIND_BYTECODE;
        case ScriptModuleKind::RawUnknown:
        default:
            return CKAS_MODULEKIND_UNKNOWN;
    }
}

ScriptModuleStateStore::ModuleState *ScriptModuleStateStore::FindState(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return nullptr;
    }
    const auto it = m_States.find(moduleName);
    return it == m_States.end() ? nullptr : &it->second;
}

const ScriptModuleStateStore::ModuleState *ScriptModuleStateStore::FindState(
    const char *moduleName) const {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return nullptr;
    }
    const auto it = m_States.find(moduleName);
    return it == m_States.end() ? nullptr : &it->second;
}

ScriptModuleStateStore::ModuleState &ScriptModuleStateStore::EnsureState(const char *moduleName) {
    const std::string key = moduleName ? moduleName : "";
    return m_States.emplace(key, ModuleState()).first->second;
}

void ScriptModuleStateStore::EnsureModuleOrdered(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    if (std::find(m_ModuleOrder.begin(), m_ModuleOrder.end(), moduleName) == m_ModuleOrder.end()) {
        m_ModuleOrder.emplace_back(moduleName);
    }
}

void ScriptModuleStateStore::MarkModuleLoaded(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    m_ModuleOrder.erase(std::remove(m_ModuleOrder.begin(), m_ModuleOrder.end(), moduleName),
                        m_ModuleOrder.end());
    m_ModuleOrder.emplace_back(moduleName);
}

void ScriptModuleStateStore::MarkModuleUnloaded(const char *moduleName) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }
    m_ModuleOrder.erase(std::remove(m_ModuleOrder.begin(), m_ModuleOrder.end(), moduleName),
                        m_ModuleOrder.end());
}
