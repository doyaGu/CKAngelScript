#include "ScriptManager.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include <fmt/format.h>

#include "ScriptApiSupport.h"
#include "ScriptModuleBytecode.h"

bool ScriptManager::HasBoundImportConsumersForModule(const char *moduleName,
                                                     std::string *consumerModule) const {
    if (consumerModule) {
        consumerModule->clear();
    }
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    for (const auto &stateEntry : m_ModuleStates) {
        for (const ImportBindingEdge &edge : stateEntry.second.BoundImports) {
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

CKAS_STATUS ScriptManager::CheckModuleRuntimeHandlesReleased(const char *moduleName,
                                                             CKAngelScriptResult *result) {
    if (HasRuntimeHandleForModule(moduleName)) {
        return StoreResult(result,
                           CKAS_INUSE,
                           0,
                           "Module has live object or execution handles.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::CheckModuleHasNoBoundImportConsumers(const char *moduleName,
                                                                CKAngelScriptResult *result) {
    std::string importConsumer;
    if (HasBoundImportConsumersForModule(moduleName, &importConsumer)) {
        return StoreResult(result,
                           CKAS_INUSE,
                           0,
                           fmt::format("Module is imported by bound module '{}'.",
                                       importConsumer));
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptManager::CheckModuleReplaceOrUnloadAllowed(const char *moduleName,
                                                             CKAngelScriptResult *result) {
    const CKAS_STATUS runtimeStatus = CheckModuleRuntimeHandlesReleased(moduleName, result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    return CheckModuleHasNoBoundImportConsumers(moduleName, result);
}

bool ScriptManager::IsModuleMutationBlockedByCallback() const {
    return m_PublicCallbackDepth > 0;
}

CKAS_STATUS ScriptManager::RejectModuleMutationDuringCallback(const char *apiName,
                                                              CKAngelScriptResult *result) {
    return StoreResult(result,
                       CKAS_INVALIDSTATE,
                       0,
                       fmt::format("{} cannot mutate modules while a CKAngelScript callback is active.",
                                   apiName ? apiName : "CKAngelScript"));
}

ScriptManager::ModuleState *ScriptManager::FindModuleState(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return nullptr;
    }
    const auto it = m_ModuleStates.find(moduleName);
    return it == m_ModuleStates.end() ? nullptr : &it->second;
}

const ScriptManager::ModuleState *ScriptManager::FindModuleState(const char *moduleName) const {
    if (!moduleName || moduleName[0] == '\0') {
        return nullptr;
    }
    const auto it = m_ModuleStates.find(moduleName);
    return it == m_ModuleStates.end() ? nullptr : &it->second;
}

ScriptManager::ModuleState &ScriptManager::EnsureModuleState(const char *moduleName) {
    return m_ModuleStates[moduleName ? moduleName : ""];
}

void ScriptManager::SetModuleKind(const char *moduleName, ModuleKind kind) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    ModuleState &state = EnsureModuleState(moduleName);
    if (state.Kind != kind) {
        state.Kind = kind;
        state.FingerprintDirty = true;
    }
}

void ScriptManager::SetModuleIncludeEdges(const char *moduleName,
                                          const std::vector<ScriptIncludeEdge> &includeEdges) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    ModuleState &state = EnsureModuleState(moduleName);
    state.IncludeEdges = includeEdges;
    state.FingerprintDirty = true;
}

void ScriptManager::RefreshModuleIncludeEdgesFromCache(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    const std::shared_ptr<CachedScript> cached = m_ScriptCache.GetCachedScript(moduleName);
    SetModuleIncludeEdges(moduleName, cached ? cached->includeEdges : std::vector<ScriptIncludeEdge>());
}

void ScriptManager::ClearModuleIncludeEdges(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    ModuleState *state = FindModuleState(moduleName);
    if (!state || state->IncludeEdges.empty()) {
        return;
    }
    state->IncludeEdges.clear();
    state->FingerprintDirty = true;
}

CKAS_MODULEKIND ScriptManager::ToPublicModuleKind(ModuleKind kind) const {
    switch (kind) {
        case ModuleKind::Source:
            return CKAS_MODULEKIND_SOURCE;
        case ModuleKind::Bytecode:
            return CKAS_MODULEKIND_BYTECODE;
        case ModuleKind::RawUnknown:
        default:
            return CKAS_MODULEKIND_UNKNOWN;
    }
}

void ScriptManager::RebuildModuleFingerprint(const char *moduleName, ModuleState &state) {
    CKAngelScriptModuleFingerprint fingerprint;
    CKAngelScriptInitModuleFingerprint(&fingerprint);
    fingerprint.Kind = ToPublicModuleKind(state.Kind);
    fingerprint.Generation = state.Generation;
    fingerprint.ApiVersion = CKAS_API_VERSION;
    fingerprint.AngelScriptVersion = asGetLibraryVersion();
    fingerprint.AngelScriptOptions = asGetLibraryOptions();

    fingerprint.SourceHash = ScriptApiSupport::kFnvOffsetBasis;
    const std::shared_ptr<CachedScript> cached = m_ScriptCache.GetCachedScript(moduleName ? moduleName : "");
    if (cached) {
        ScriptApiSupport::HashBool(fingerprint.SourceHash, cached->sourceSnapshotSections);
        ScriptApiSupport::HashValue(fingerprint.SourceHash, static_cast<unsigned long long>(cached->sections.size()));
        for (const auto &section : cached->sections) {
            ScriptApiSupport::HashString(fingerprint.SourceHash, std::get<0>(section));
            ScriptApiSupport::HashString(fingerprint.SourceHash, std::get<1>(section));
        }
    }

    fingerprint.IncludeHash = ScriptApiSupport::kFnvOffsetBasis;
    ScriptApiSupport::HashValue(fingerprint.IncludeHash, static_cast<unsigned long long>(state.IncludeEdges.size()));
    for (const ScriptIncludeEdge &edge : state.IncludeEdges) {
        ScriptApiSupport::HashString(fingerprint.IncludeHash, edge.FromSection);
        ScriptApiSupport::HashString(fingerprint.IncludeHash, edge.ToSection);
        ScriptApiSupport::HashBool(fingerprint.IncludeHash, edge.ResolvedFromSnapshot);
    }

    fingerprint.DeclaredImportHash = ScriptApiSupport::kFnvOffsetBasis;
    asIScriptModule *module = GetModule(moduleName);
    if (module) {
        const asUINT importCount = module->GetImportedFunctionCount();
        ScriptApiSupport::HashValue(fingerprint.DeclaredImportHash, static_cast<unsigned long long>(importCount));
        for (asUINT i = 0; i < importCount; ++i) {
            ScriptApiSupport::HashValue(fingerprint.DeclaredImportHash, static_cast<CKDWORD>(i));
            ScriptApiSupport::HashString(fingerprint.DeclaredImportHash, module->GetImportedFunctionSourceModule(i));
            ScriptApiSupport::HashString(fingerprint.DeclaredImportHash, module->GetImportedFunctionDeclaration(i));
        }
    }

    fingerprint.BoundImportHash = ScriptApiSupport::kFnvOffsetBasis;
    ScriptApiSupport::HashValue(fingerprint.BoundImportHash, static_cast<unsigned long long>(state.BoundImports.size()));
    for (const ImportBindingEdge &edge : state.BoundImports) {
        ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.ImportModuleName);
        ScriptApiSupport::HashValue(fingerprint.BoundImportHash, edge.ImportIndex);
        ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.SourceModuleName);
        ScriptApiSupport::HashString(fingerprint.BoundImportHash, edge.FunctionDecl);
    }

    fingerprint.CombinedHash = ScriptApiSupport::kFnvOffsetBasis;
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.ApiVersion);
    ScriptApiSupport::HashString(fingerprint.CombinedHash, fingerprint.AngelScriptVersion);
    ScriptApiSupport::HashString(fingerprint.CombinedHash, fingerprint.AngelScriptOptions);
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, static_cast<CKDWORD>(fingerprint.Kind));
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.Generation);
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.SourceHash);
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.IncludeHash);
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.DeclaredImportHash);
    ScriptApiSupport::HashValue(fingerprint.CombinedHash, fingerprint.BoundImportHash);

    state.Fingerprint = fingerprint;
    state.FingerprintDirty = false;
}

void ScriptManager::MarkModuleStateDirty(const char *moduleName) {
    ModuleState *state = FindModuleState(moduleName);
    if (state) {
        state->FingerprintDirty = true;
    }
}

std::vector<ScriptManager::ImportBindingEdge> ScriptManager::GetImportBindingsForModule(
    const char *moduleName) const {
    const ModuleState *state = FindModuleState(moduleName);
    return state ? state->BoundImports : std::vector<ImportBindingEdge>();
}

bool ScriptManager::RemoveImportBinding(const char *moduleName, CKDWORD importIndex) {
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    ModuleState *state = FindModuleState(moduleName);
    if (!state) {
        return false;
    }
    const auto oldSize = state->BoundImports.size();
    state->BoundImports.erase(std::remove_if(state->BoundImports.begin(),
                                             state->BoundImports.end(),
                                             [moduleName, importIndex](const ImportBindingEdge &edge) {
                                                 return edge.ImportModuleName == moduleName &&
                                                        edge.ImportIndex == importIndex;
                                             }),
                              state->BoundImports.end());
    const bool removed = state->BoundImports.size() != oldSize;
    if (removed) {
        state->FingerprintDirty = true;
    }
    return removed;
}

bool ScriptManager::RemoveImportBindingsForModule(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    ModuleState *state = FindModuleState(moduleName);
    if (!state || state->BoundImports.empty()) {
        return false;
    }
    state->BoundImports.clear();
    state->FingerprintDirty = true;
    return true;
}

bool ScriptManager::RebindImportBindings(const std::vector<ImportBindingEdge> &bindings,
                                         int &angelScriptCode,
                                         std::string &errorMessage) {
    angelScriptCode = 0;
    errorMessage.clear();
    struct ResolvedImportBinding {
        const ImportBindingEdge *Edge = nullptr;
        asIScriptModule *ImportModule = nullptr;
        asIScriptFunction *TargetFunction = nullptr;
    };
    std::vector<ResolvedImportBinding> resolvedBindings;
    resolvedBindings.reserve(bindings.size());
    for (const ImportBindingEdge &edge : bindings) {
        asIScriptModule *importModule = GetModule(edge.ImportModuleName.c_str());
        if (!importModule) {
            errorMessage = fmt::format("Failed to restore import binding: module '{}' was not found.",
                                       edge.ImportModuleName);
            return false;
        }
        if (edge.ImportIndex >= importModule->GetImportedFunctionCount()) {
            angelScriptCode = asINVALID_ARG;
            errorMessage = fmt::format("Failed to restore import binding: import {} is out of range in module '{}'.",
                                       edge.ImportIndex,
                                       edge.ImportModuleName);
            return false;
        }
        asIScriptModule *sourceModule = GetModule(edge.SourceModuleName.c_str());
        if (!sourceModule) {
            angelScriptCode = asNO_MODULE;
            errorMessage = fmt::format("Failed to restore import binding: source module '{}' was not found.",
                                       edge.SourceModuleName);
            return false;
        }
        asIScriptFunction *targetFunction = sourceModule->GetFunctionByDecl(edge.FunctionDecl.c_str());
        if (!targetFunction) {
            angelScriptCode = asNO_FUNCTION;
            errorMessage = fmt::format("Failed to restore import binding: function '{}' was not found in module '{}'.",
                                       edge.FunctionDecl,
                                       edge.SourceModuleName);
            return false;
        }
        ResolvedImportBinding resolved;
        resolved.Edge = &edge;
        resolved.ImportModule = importModule;
        resolved.TargetFunction = targetFunction;
        resolvedBindings.push_back(resolved);
    }
    std::vector<ResolvedImportBinding> appliedBindings;
    appliedBindings.reserve(resolvedBindings.size());
    for (const ResolvedImportBinding &resolved : resolvedBindings) {
        const ImportBindingEdge &edge = *resolved.Edge;
        angelScriptCode = resolved.ImportModule->BindImportedFunction(edge.ImportIndex, resolved.TargetFunction);
        if (angelScriptCode < 0) {
            errorMessage = fmt::format("Failed to restore import binding {} in module '{}'.",
                                       edge.ImportIndex,
                                       edge.ImportModuleName);
            for (const ResolvedImportBinding &applied : appliedBindings) {
                applied.ImportModule->UnbindImportedFunction(applied.Edge->ImportIndex);
            }
            return false;
        }
        appliedBindings.push_back(resolved);
    }
    return true;
}

void ScriptManager::RestoreImportBindingsForModule(
    const char *moduleName,
    const std::vector<ImportBindingEdge> &bindings) {
    RemoveImportBindingsForModule(moduleName);
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    ModuleState &state = EnsureModuleState(moduleName);
    for (const ImportBindingEdge &edge : bindings) {
        state.BoundImports.push_back(edge);
    }
    state.FingerprintDirty = true;
}

void ScriptManager::RecordImportBinding(const char *importModuleName,
                                        CKDWORD importIndex,
                                        const char *sourceModuleName,
                                        const char *functionDecl) {
    RemoveImportBinding(importModuleName, importIndex);
    if (!importModuleName || importModuleName[0] == '\0' ||
        !sourceModuleName || sourceModuleName[0] == '\0' ||
        !functionDecl || functionDecl[0] == '\0') {
        return;
    }
    ImportBindingEdge edge;
    edge.ImportModuleName = importModuleName;
    edge.ImportIndex = importIndex;
    edge.SourceModuleName = sourceModuleName;
    edge.FunctionDecl = functionDecl;
    ModuleState &state = EnsureModuleState(importModuleName);
    state.BoundImports.push_back(edge);
    state.FingerprintDirty = true;
}

void ScriptManager::BumpModuleGeneration(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }
    CKDWORD &generation = EnsureModuleState(moduleName).Generation;
    if (generation == 0) {
        generation = 1;
    } else {
        ++generation;
    }
    MarkModuleStateDirty(moduleName);
}

std::shared_ptr<CachedScript> ScriptManager::BuildTransientModule(
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    int &angelScriptCode,
    std::string &diagnostics,
    std::vector<CapturedScriptMessage> *messages) {
    angelScriptCode = 0;
    diagnostics.clear();
    if (!m_ScriptEngine || !moduleName || moduleName[0] == '\0' || sections.empty()) {
        angelScriptCode = -1;
        return nullptr;
    }

    auto script = std::make_shared<CachedScript>();
    script->name = moduleName;
    script->sourceSnapshotSections = sourceSnapshotSections;
    for (const auto &section : sections) {
        script->AddSection(std::get<0>(section), std::get<1>(section));
    }

    BeginScriptMessageCapture();
    const bool built = script->Build(m_ScriptEngine);
    diagnostics = EndScriptMessageCapture(messages);
    if (!built) {
        angelScriptCode = -3;
        if (script->module) {
            script->Discard();
        }
        return nullptr;
    }
    return script;
}

bool ScriptManager::CaptureModuleReplacementSnapshot(const char *moduleName,
                                                     ModuleReplacementSnapshot &snapshot,
                                                     int &angelScriptCode,
                                                     std::string &errorMessage) {
    snapshot = ModuleReplacementSnapshot();
    angelScriptCode = 0;
    errorMessage.clear();
    if (!moduleName || moduleName[0] == '\0') {
        angelScriptCode = -1;
        errorMessage = "Module name is required.";
        return false;
    }

    snapshot.Cache = m_ScriptCache.GetCachedScript(moduleName);
    snapshot.ImportBindings = GetImportBindingsForModule(moduleName);
    if (snapshot.Cache) {
        snapshot.Sections = snapshot.Cache->sections;
        snapshot.Metadata = snapshot.Cache->metadata;
        snapshot.SourceSnapshotSections = snapshot.Cache->sourceSnapshotSections;
    }

    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return true;
    }

    snapshot.HasModule = true;
    if (!ScriptModuleBytecode::SaveModuleByteCode(module, snapshot.ByteCode, angelScriptCode)) {
        errorMessage = "Failed to snapshot existing script module bytecode.";
        return false;
    }
    return true;
}

void ScriptManager::RemoveModuleForReplacement(const char *moduleName,
                                               ModuleReplacementSnapshot &snapshot) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }

    m_ScriptCache.Invalidate(moduleName);
    RemoveImportBindingsForModule(moduleName);
    if (snapshot.Cache && snapshot.Cache->module) {
        snapshot.Cache->module->Discard();
        snapshot.Cache->module = nullptr;
        return;
    }

    asIScriptModule *module = GetModule(moduleName);
    if (module) {
        module->Discard();
    }
}

bool ScriptManager::RestoreModuleReplacementSnapshot(const char *moduleName,
                                                     ModuleReplacementSnapshot &snapshot,
                                                     int &angelScriptCode,
                                                     std::string &errorMessage) {
    angelScriptCode = 0;
    errorMessage.clear();
    if (!snapshot.HasModule) {
        return true;
    }

    asIScriptModule *restoredModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(m_ScriptEngine,
                                                                    moduleName,
                                                                    snapshot.ByteCode,
                                                                    &restoredModule,
                                                                    angelScriptCode)) {
        errorMessage = "Failed to restore previous script module bytecode.";
        return false;
    }

    std::shared_ptr<CachedScript> restoredCache = snapshot.Cache ? snapshot.Cache : std::make_shared<CachedScript>();
    restoredCache->name = moduleName ? moduleName : "";
    restoredCache->sections = snapshot.Sections;
    restoredCache->sourceSnapshotSections = snapshot.SourceSnapshotSections;
    restoredCache->metadata = snapshot.Metadata;
    restoredCache->module = restoredModule;
    m_ScriptCache.CacheScript(moduleName, restoredCache);
    if (!RebindImportBindings(snapshot.ImportBindings, angelScriptCode, errorMessage)) {
        return false;
    }
    RestoreImportBindingsForModule(moduleName, snapshot.ImportBindings);
    return true;
}

CKAS_STATUS ScriptManager::ReplaceModuleFromSections(
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    CKAngelScriptResult *result) {
    int angelScriptCode = 0;
    std::string diagnostics;
    std::vector<CapturedScriptMessage> diagnosticMessages;
    const std::string transientName = ScriptModuleBytecode::MakeTransientModuleName(m_ScriptEngine,
                                                                                                      moduleName);
    std::shared_ptr<CachedScript> candidate =
        BuildTransientModule(transientName.c_str(),
                             sections,
                             sourceSnapshotSections,
                             angelScriptCode,
                             diagnostics,
                             &diagnosticMessages);
    if (!candidate || !candidate->module) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           angelScriptCode,
                           diagnostics.empty() ? "Failed to compile replacement script module." : diagnostics,
                           std::string(),
                           &diagnosticMessages);
    }

    std::vector<unsigned char> candidateByteCode;
    if (!ScriptModuleBytecode::SaveModuleByteCode(candidate->module,
                                                                    candidateByteCode,
                                                                    angelScriptCode)) {
        candidate->Discard();
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           "Failed to snapshot replacement script module bytecode.");
    }

    ModuleReplacementSnapshot snapshot;
    std::string snapshotError;
    if (!CaptureModuleReplacementSnapshot(moduleName, snapshot, angelScriptCode, snapshotError)) {
        candidate->Discard();
        return StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, snapshotError);
    }

    RemoveModuleForReplacement(moduleName, snapshot);

    asIScriptModule *committedModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(m_ScriptEngine,
                                                                    moduleName,
                                                                    candidateByteCode,
                                                                    &committedModule,
                                                                    angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored = RestoreModuleReplacementSnapshot(moduleName, snapshot, restoreCode, restoreError);
        candidate->Discard();
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           restored
                               ? "Failed to commit replacement script module bytecode."
                               : fmt::format("Failed to commit replacement script module bytecode; rollback also failed: {}",
                                             restoreError));
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = moduleName ? moduleName : "";
    committedCache->sections = candidate->sections;
    committedCache->includeEdges = candidate->includeEdges;
    committedCache->sourceSnapshotSections = candidate->sourceSnapshotSections;
    ScriptMetadata::RemapForModule(candidate->module,
                                   committedModule,
                                   candidate->metadata,
                                   committedCache->metadata);
    committedCache->module = committedModule;
    m_ScriptCache.CacheScript(moduleName, committedCache);
    candidate->Discard();
    SetModuleIncludeEdges(moduleName, committedCache->includeEdges);
    SetModuleKind(moduleName, ModuleKind::Source);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptManager::LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result) {
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "LoadModule options size is invalid.");
    }
    const char *moduleName = ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::ModuleName, static_cast<const char *>(nullptr));
    const char *filename = ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::Filename, static_cast<const char *>(nullptr));
    const char **filenames = ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::Filenames, static_cast<const char **>(nullptr));
    const size_t fileCount = ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::FileCount, static_cast<size_t>(0));
    const char *code = ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::Code, static_cast<const char *>(nullptr));
    const CKAngelScriptSourceSection *sourceSections =
        ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::Sections, static_cast<const CKAngelScriptSourceSection *>(nullptr));
    const size_t sourceSectionCount =
        ScriptApiSupport::PublicField(options, &CKAngelScriptLoadOptions::SectionCount, static_cast<size_t>(0));
    const CKDWORD flags = ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptLoadOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_LOAD_DEFAULT));

    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("LoadModule", result);
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_LOAD_REPLACEEXISTING)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown LoadModule flags.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool hasCode = code != nullptr;
    const bool hasFile = filename && filename[0] != '\0';
    const bool hasFiles = fileCount > 0;
    const bool hasSourceSections = sourceSectionCount > 0;
    const int sourceCount = (hasCode ? 1 : 0) + (hasFile ? 1 : 0) + (hasFiles ? 1 : 0) + (hasSourceSections ? 1 : 0);
    if (sourceCount > 1) {
        return StoreResult(result,
                           CKAS_INVALIDARGUMENT,
                           0,
                           "LoadModule accepts only one source: Code, Filename, Filenames, or Sections.");
    }
    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_LOAD_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(moduleName, result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
    }
    if (hasCode) {
        return CompileModule(moduleName, code, CKAS_COMPILE_REPLACEEXISTING, result);
    }
    if (hasSourceSections) {
        if (!sourceSections) {
            return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Source section list is null.");
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.reserve(sourceSectionCount);
        for (size_t i = 0; i < sourceSectionCount; ++i) {
            const CKAngelScriptSourceSection &sourceSection = sourceSections[i];
            if (!ScriptApiSupport::HasCompletePublicStruct(sourceSection)) {
                return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Source section options size is invalid.");
            }
            const char *sectionName = ScriptApiSupport::PublicField(sourceSection,
                                                  &CKAngelScriptSourceSection::SectionName,
                                                  static_cast<const char *>(nullptr));
            const char *sectionCode = ScriptApiSupport::PublicField(sourceSection,
                                                  &CKAngelScriptSourceSection::Code,
                                                  static_cast<const char *>(nullptr));
            const size_t sectionSize = ScriptApiSupport::PublicField(sourceSection,
                                                   &CKAngelScriptSourceSection::CodeSize,
                                                   static_cast<size_t>(0));
            if (!sectionName || sectionName[0] == '\0') {
                return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Source section list contains an empty section name.");
            }
            if (!sectionCode) {
                return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Source section list contains null code.");
            }
            sections.emplace_back(sectionName,
                                  sectionSize == 0 ? std::string(sectionCode)
                                                   : std::string(sectionCode, sectionSize));
        }
        return ReplaceModuleFromSections(moduleName, sections, true, result);
    }
    if (hasFiles) {
        if (!filenames) {
            return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "File list is null.");
        }
        for (size_t i = 0; i < fileCount; ++i) {
            if (!filenames[i] || filenames[i][0] == '\0') {
                return StoreResult(result,
                                   CKAS_INVALIDARGUMENT,
                                   0,
                                   "File list contains an empty filename.");
            }
        }
        if (replacingExisting) {
            std::vector<std::tuple<std::string, std::string>> sections;
            sections.reserve(fileCount);
            for (size_t i = 0; i < fileCount; ++i) {
                XString scriptFilename = filenames[i];
                if (scriptFilename.Find(".as") == XString::NOTFOUND) {
                    scriptFilename += ".as";
                }
                ResolveScriptFileName(scriptFilename);
                sections.emplace_back(scriptFilename.CStr(), std::string());
            }
            return ReplaceModuleFromSections(moduleName, sections, result);
        }
        std::vector<CapturedScriptMessage> diagnosticMessages;
        BeginScriptMessageCapture();
        const int loadResult = LoadModuleFromFiles(moduleName, filenames, fileCount);
        const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
        if (loadResult < 0) {
            return StoreResult(result,
                               CKAS_COMPILEERROR,
                               loadResult,
                               diagnostics.empty() ? "Failed to load script files." : diagnostics,
                               std::string(),
                               &diagnosticMessages);
        }
        RefreshModuleIncludeEdgesFromCache(moduleName);
        SetModuleKind(moduleName, ModuleKind::Source);
        BumpModuleGeneration(moduleName);
        return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
    }

    if (replacingExisting) {
        std::string scriptFilename;
        if (filename) {
            scriptFilename = filename;
        } else {
            scriptFilename = moduleName;
            scriptFilename += ".as";
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(std::move(scriptFilename), std::string());
        return ReplaceModuleFromSections(moduleName, sections, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    BeginScriptMessageCapture();
    const int loadResult = LoadModuleFromDefaultOrFile(moduleName, filename);
    const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
    if (loadResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           loadResult,
                           diagnostics.empty() ? "Failed to load script file." : diagnostics,
                           std::string(),
                           &diagnosticMessages);
    }
    RefreshModuleIncludeEdgesFromCache(moduleName);
    SetModuleKind(moduleName, ModuleKind::Source);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptManager::CompileModule(const char *moduleName,
                                               const char *scriptCode,
                                               CKDWORD flags,
                                               CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0' || !scriptCode) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name and script code are required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("CompileModule", result);
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_COMPILE_REPLACEEXISTING)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown CompileModule flags.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_COMPILE_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
        const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(moduleName, result);
        if (mutationStatus != CKAS_OK) {
            return mutationStatus;
        }
        std::vector<std::tuple<std::string, std::string>> sections;
        sections.emplace_back(moduleName, scriptCode);
        return ReplaceModuleFromSections(moduleName, sections, result);
    }

    std::vector<CapturedScriptMessage> diagnosticMessages;
    BeginScriptMessageCapture();
    const int compileResult = CompileModuleFromMemory(moduleName, scriptCode);
    const std::string diagnostics = EndScriptMessageCapture(&diagnosticMessages);
    if (compileResult < 0) {
        return StoreResult(result,
                           CKAS_COMPILEERROR,
                           compileResult,
                           diagnostics.empty() ? "Failed to compile script module." : diagnostics,
                           std::string(),
                           &diagnosticMessages);
    }
    RefreshModuleIncludeEdgesFromCache(moduleName);
    SetModuleKind(moduleName, ModuleKind::Source);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptManager::UnloadModule(const char *moduleName, CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("UnloadModule", result);
    }
    const CKAS_STATUS mutationStatus = CheckModuleReplaceOrUnloadAllowed(moduleName, result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!DiscardModule(moduleName)) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not loaded.");
    }
    RemoveImportBindingsForModule(moduleName);
    ClearModuleIncludeEdges(moduleName);
    SetModuleKind(moduleName, ModuleKind::RawUnknown);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK);
}

bool ScriptManager::HasModule(const char *moduleName) {
    return GetModule(moduleName) != nullptr;
}

CKDWORD ScriptManager::GetModuleGeneration(const char *moduleName) const {
    if (!moduleName || moduleName[0] == '\0') {
        return 0;
    }
    const ModuleState *state = FindModuleState(moduleName);
    return state ? state->Generation : 0;
}

asIScriptModule *ScriptManager::GetModule(const char *moduleName) {
    return GetScript(moduleName);
}

CKAS_STATUS ScriptManager::BorrowModule(const char *moduleName,
                                        asIScriptModule **outModule,
                                        CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    if (!outModule) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module out pointer is required.");
    }
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module was not found.");
    }
    *outModule = module;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByName(const char *moduleName,
                                                const char *functionName,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!functionName || functionName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function name is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *match = nullptr;
    asUINT matchCount = 0;
    const asUINT count = module->GetFunctionCount();
    for (asUINT i = 0; i < count; ++i) {
        asIScriptFunction *function = module->GetFunctionByIndex(i);
        const char *name = function ? function->GetName() : nullptr;
        if (name && std::strcmp(name, functionName) == 0) {
            match = function;
            ++matchCount;
        }
    }
    if (matchCount == 0) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    if (matchCount > 1) {
        return StoreResult(result, CKAS_AMBIGUOUS, 0, "Function name matched multiple overloads.");
    }
    *outFunction = match;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BorrowFunctionByDecl(const char *moduleName,
                                                const char *functionDecl,
                                                asIScriptFunction **outFunction,
                                                CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    if (!outFunction) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function out pointer is required.");
    }
    if (!functionDecl || functionDecl[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Function declaration is required.");
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    asIScriptFunction *function = module->GetFunctionByDecl(functionDecl);
    if (!function) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Function was not found.");
    }
    *outFunction = function;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateMetadata(const char *moduleName,
                                             CKAngelScriptMetadataCallback callback,
                                             void *userData,
                                             CKAngelScriptResult *result) {
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Metadata callback is required.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    std::shared_ptr<CachedScript> cached = GetCachedScript(moduleName);
    if (!cached || !cached->GetScriptModule()) {
        return StoreResult(result, CKAS_NOTFOUND, 0, "Module metadata was not found.");
    }

    asIScriptModule *module = cached->GetScriptModule();
    auto finish = [this, result](CKAS_STATUS status) {
        return status == CKAS_OK
                   ? StoreResult(result, CKAS_OK)
                   : StoreResult(result, status, 0, "Metadata enumeration stopped by callback.");
    };
    auto dispatchMetadata = [this, callback, userData](
                                const CKAngelScriptMetadataEntry &entry,
                                CKDWORD metadataCount,
                                const std::function<const char *(CKDWORD)> &metadataAt) {
        ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
        return ScriptApiSupport::DispatchMetadata(entry, metadataCount, metadataAt, callback, userData);
    };

    const asUINT typeCount = module->GetObjectTypeCount();
    for (asUINT typeIndex = 0; typeIndex < typeCount; ++typeIndex) {
        asITypeInfo *type = module->GetObjectTypeByIndex(typeIndex);
        if (!type) {
            continue;
        }
        const int typeId = type->GetTypeId();
        const char *typeName = type->GetName();
        const char *typeNamespace = type->GetNamespace();
        const std::string typeDeclaration = typeName ? typeName : "";
        const int rawTypeMetadataCount = cached->GetTypeMetadataCount(typeId);
        const CKDWORD typeMetadataCount =
            static_cast<CKDWORD>(XMax(0, rawTypeMetadataCount));
        if (typeMetadataCount > 0) {
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE;
            entry.Name = typeName;
            entry.Namespace = typeNamespace;
            entry.Declaration = typeDeclaration.c_str();
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                typeMetadataCount,
                [cached, typeId](CKDWORD index) {
                    return cached->GetTypeMetadata(typeId, static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }

        const asUINT methodCount = type->GetMethodCount();
        for (asUINT methodIndex = 0; methodIndex < methodCount; ++methodIndex) {
            asIScriptFunction *method = type->GetMethodByIndex(methodIndex);
            const int rawMetadataCount = cached->GetClassMethodMetadataCount(typeId, method);
            const CKDWORD metadataCount =
                static_cast<CKDWORD>(XMax(0, rawMetadataCount));
            if (!method || metadataCount == 0) {
                continue;
            }
            const std::string declaration = method->GetDeclaration(false, false, true);
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE_METHOD;
            entry.Name = method->GetName();
            entry.Namespace = typeNamespace;
            entry.Declaration = declaration.c_str();
            entry.ParentTypeName = typeName;
            entry.ParentTypeNamespace = typeNamespace;
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                metadataCount,
                [cached, typeId, method](CKDWORD index) {
                    return cached->GetClassMethodMetadata(typeId, method, static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }

        const asUINT propertyCount = type->GetPropertyCount();
        for (asUINT propertyIndex = 0; propertyIndex < propertyCount; ++propertyIndex) {
            const int rawMetadataCount = cached->GetClassVarMetadataCount(typeId, static_cast<int>(propertyIndex));
            const CKDWORD metadataCount =
                static_cast<CKDWORD>(XMax(0, rawMetadataCount));
            if (metadataCount == 0) {
                continue;
            }
            const char *propertyName = nullptr;
            type->GetProperty(propertyIndex, &propertyName);
            const char *declaration = type->GetPropertyDeclaration(propertyIndex, false);
            CKAngelScriptMetadataEntry entry = {};
            entry.Size = sizeof(entry);
            entry.Target = CKAS_METADATA_TYPE_PROPERTY;
            entry.Name = propertyName;
            entry.Namespace = typeNamespace;
            entry.Declaration = declaration;
            entry.ParentTypeName = typeName;
            entry.ParentTypeNamespace = typeNamespace;
            const CKAS_STATUS status = dispatchMetadata(
                entry,
                metadataCount,
                [cached, typeId, propertyIndex](CKDWORD index) {
                    return cached->GetClassVarMetadata(typeId,
                                                       static_cast<int>(propertyIndex),
                                                       static_cast<int>(index));
                });
            if (status != CKAS_OK) {
                return finish(status);
            }
        }
    }

    const asUINT functionCount = module->GetFunctionCount();
    for (asUINT functionIndex = 0; functionIndex < functionCount; ++functionIndex) {
        asIScriptFunction *function = module->GetFunctionByIndex(functionIndex);
        const int rawMetadataCount = cached->GetFuncMetadataCount(function);
        const CKDWORD metadataCount =
            static_cast<CKDWORD>(XMax(0, rawMetadataCount));
        if (!function || metadataCount == 0) {
            continue;
        }
        const std::string declaration = function->GetDeclaration(false, true, true);
        CKAngelScriptMetadataEntry entry = {};
        entry.Size = sizeof(entry);
        entry.Target = CKAS_METADATA_GLOBAL_FUNCTION;
        entry.Name = function->GetName();
        entry.Namespace = function->GetNamespace();
        entry.Declaration = declaration.c_str();
        const CKAS_STATUS status = dispatchMetadata(
            entry,
            metadataCount,
            [cached, function](CKDWORD index) {
                return cached->GetFuncMetadata(function, static_cast<int>(index));
            });
        if (status != CKAS_OK) {
            return finish(status);
        }
    }

    const asUINT globalCount = module->GetGlobalVarCount();
    for (asUINT globalIndex = 0; globalIndex < globalCount; ++globalIndex) {
        const int rawMetadataCount = cached->GetVarMetadataCount(static_cast<int>(globalIndex));
        const CKDWORD metadataCount =
            static_cast<CKDWORD>(XMax(0, rawMetadataCount));
        if (metadataCount == 0) {
            continue;
        }
        const char *name = nullptr;
        const char *nameSpace = nullptr;
        module->GetGlobalVar(globalIndex, &name, &nameSpace);
        const char *declaration = module->GetGlobalVarDeclaration(globalIndex, true);
        CKAngelScriptMetadataEntry entry = {};
        entry.Size = sizeof(entry);
        entry.Target = CKAS_METADATA_GLOBAL_VARIABLE;
        entry.Name = name;
        entry.Namespace = nameSpace;
        entry.Declaration = declaration;
        const CKAS_STATUS status = dispatchMetadata(
            entry,
            metadataCount,
            [cached, globalIndex](CKDWORD index) {
                return cached->GetVarMetadata(static_cast<int>(globalIndex), static_cast<int>(index));
            });
        if (status != CKAS_OK) {
            return finish(status);
        }
    }

    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetImportedFunctionCount(const char *moduleName,
                                                    CKDWORD *outCount,
                                                    CKAngelScriptResult *result) {
    if (outCount) {
        *outCount = 0;
    }
    if (!outCount) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import count out pointer is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    *outCount = static_cast<CKDWORD>(module->GetImportedFunctionCount());
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateImportedFunctions(const char *moduleName,
                                                      CKAngelScriptImportCallback callback,
                                                      void *userData,
                                                      CKAngelScriptResult *result) {
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }

    const asUINT count = module->GetImportedFunctionCount();
    for (asUINT i = 0; i < count; ++i) {
        CKAngelScriptImportEntry entry = {};
        entry.Size = sizeof(entry);
        entry.Index = static_cast<CKDWORD>(i);
        entry.Declaration = module->GetImportedFunctionDeclaration(i);
        entry.SourceModuleName = module->GetImportedFunctionSourceModule(i);
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchImport(entry, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result,
                               callbackStatus,
                               0,
                               "Import enumeration stopped by callback.");
        }
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::BindImportedFunction(const CKAngelScriptImportBindOptions &options,
                                                CKAngelScriptResult *result) {
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("BindImportedFunction", result);
    }
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import bind options size is invalid.");
    }
    const char *importModuleName =
        ScriptApiSupport::PublicField(options, &CKAngelScriptImportBindOptions::ImportModuleName, static_cast<const char *>(nullptr));
    const CKDWORD importIndex =
        ScriptApiSupport::PublicField(options, &CKAngelScriptImportBindOptions::ImportIndex, static_cast<CKDWORD>(0));
    const char *sourceModuleOverride =
        ScriptApiSupport::PublicField(options, &CKAngelScriptImportBindOptions::SourceModuleName, static_cast<const char *>(nullptr));
    const char *functionDeclOverride =
        ScriptApiSupport::PublicField(options, &CKAngelScriptImportBindOptions::FunctionDecl, static_cast<const char *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options, &CKAngelScriptImportBindOptions::Flags, static_cast<CKDWORD>(0));
    if (flags != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown BindImportedFunction flags.");
    }

    asIScriptModule *importModule = nullptr;
    CKAS_STATUS status = BorrowModule(importModuleName, &importModule, result);
    if (status != CKAS_OK) {
        return status;
    }
    const asUINT importCount = importModule->GetImportedFunctionCount();
    if (importIndex >= importCount) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
    }

    const char *defaultSourceModuleName = importModule->GetImportedFunctionSourceModule(importIndex);
    const char *defaultFunctionDecl = importModule->GetImportedFunctionDeclaration(importIndex);
    const std::string sourceModuleName =
        sourceModuleOverride && sourceModuleOverride[0] != '\0'
            ? sourceModuleOverride
            : (defaultSourceModuleName ? defaultSourceModuleName : "");
    const std::string functionDecl =
        functionDeclOverride && functionDeclOverride[0] != '\0'
            ? functionDeclOverride
            : (defaultFunctionDecl ? defaultFunctionDecl : "");
    if (sourceModuleName.empty() || functionDecl.empty()) {
        return StoreResult(result,
                           CKAS_INVALIDARGUMENT,
                           0,
                           "Import source module and function declaration are required.");
    }

    asIScriptModule *sourceModule = GetModule(sourceModuleName.c_str());
    if (!sourceModule) {
        return StoreResult(result,
                           CKAS_NOTFOUND,
                           0,
                           fmt::format("Import source module '{}' was not found.", sourceModuleName));
    }
    asIScriptFunction *targetFunction = sourceModule->GetFunctionByDecl(functionDecl.c_str());
    if (!targetFunction) {
        return StoreResult(result,
                           CKAS_NOTFOUND,
                           0,
                           fmt::format("Import target function '{}' was not found in module '{}'.",
                                       functionDecl,
                                       sourceModuleName));
    }

    std::vector<ImportBindingEdge> previousBinding;
    const ModuleState *previousState = FindModuleState(importModuleName);
    if (previousState) {
        for (const ImportBindingEdge &edge : previousState->BoundImports) {
            if (edge.ImportModuleName == importModuleName && edge.ImportIndex == importIndex) {
                previousBinding.push_back(edge);
                break;
            }
        }
    }

    const int bindResult = importModule->BindImportedFunction(importIndex, targetFunction);
    if (bindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
        if (!previousBinding.empty()) {
            int rollbackCode = 0;
            std::string rollbackError;
            if (RebindImportBindings(previousBinding, rollbackCode, rollbackError)) {
                return StoreResult(result,
                                   status,
                                   bindResult,
                                   "Failed to bind imported function; previous binding was restored.");
            }
            RemoveImportBinding(importModuleName, importIndex);
            BumpModuleGeneration(importModuleName);
            return StoreResult(result,
                               CKAS_EXECUTIONFAILED,
                               rollbackCode,
                               fmt::format("Failed to bind imported function; rollback also failed: {}",
                                           rollbackError));
        }
        RemoveImportBinding(importModuleName, importIndex);
        BumpModuleGeneration(importModuleName);
        return StoreResult(result, status, bindResult, "Failed to bind imported function.");
    }
    RecordImportBinding(importModuleName, importIndex, sourceModuleName.c_str(), functionDecl.c_str());
    BumpModuleGeneration(importModuleName);
    return StoreResult(result, CKAS_OK, bindResult);
}

CKAS_STATUS ScriptManager::BindAllImportedFunctions(const char *moduleName,
                                                    CKAngelScriptResult *result) {
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("BindAllImportedFunctions", result);
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }

    const asUINT count = module->GetImportedFunctionCount();
    struct ResolvedImportBinding {
        CKDWORD Index = 0;
        std::string SourceModuleName;
        std::string FunctionDecl;
        asIScriptFunction *TargetFunction = nullptr;
    };
    std::vector<ResolvedImportBinding> resolvedBindings;
    resolvedBindings.reserve(count);
    for (asUINT i = 0; i < count; ++i) {
        const char *sourceModuleNameView = module->GetImportedFunctionSourceModule(i);
        const char *functionDeclView = module->GetImportedFunctionDeclaration(i);
        const std::string sourceModuleName = sourceModuleNameView ? sourceModuleNameView : "";
        const std::string functionDecl = functionDeclView ? functionDeclView : "";
        if (sourceModuleName.empty() || functionDecl.empty()) {
            return StoreResult(result,
                               CKAS_INVALIDARGUMENT,
                               0,
                               fmt::format("Import {} is missing a source module or declaration.", i));
        }
        asIScriptModule *sourceModule = GetModule(sourceModuleName.c_str());
        if (!sourceModule) {
            return StoreResult(result,
                               CKAS_NOTFOUND,
                               0,
                               fmt::format("Import {} source module '{}' was not found.", i, sourceModuleName));
        }
        asIScriptFunction *targetFunction = sourceModule->GetFunctionByDecl(functionDecl.c_str());
        if (!targetFunction) {
            return StoreResult(result,
                               CKAS_NOTFOUND,
                               0,
                               fmt::format("Import {} target function '{}' was not found in module '{}'.",
                                           i,
                                           functionDecl,
                                           sourceModuleName));
        }
        const asEFuncType functionType = targetFunction->GetFuncType();
        if (functionType != asFUNC_SCRIPT && functionType != asFUNC_SYSTEM) {
            return StoreResult(result,
                               CKAS_UNSUPPORTED,
                               asNOT_SUPPORTED,
                               fmt::format("Import {} target function type is not supported.", i));
        }
        ResolvedImportBinding binding;
        binding.Index = static_cast<CKDWORD>(i);
        binding.SourceModuleName = sourceModuleName;
        binding.FunctionDecl = functionDecl;
        binding.TargetFunction = targetFunction;
        resolvedBindings.push_back(binding);
    }

    const std::vector<ImportBindingEdge> previousBindings = GetImportBindingsForModule(moduleName);
    for (const ResolvedImportBinding &binding : resolvedBindings) {
        RemoveImportBinding(moduleName, binding.Index);
        const int bindResult = module->BindImportedFunction(binding.Index, binding.TargetFunction);
        if (bindResult < 0) {
            const CKAS_STATUS status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
            module->UnbindAllImportedFunctions();
            RemoveImportBindingsForModule(moduleName);

            int rollbackCode = 0;
            std::string rollbackError;
            const bool restored = RebindImportBindings(previousBindings, rollbackCode, rollbackError);
            if (!restored) {
                BumpModuleGeneration(moduleName);
                return StoreResult(result,
                                   CKAS_EXECUTIONFAILED,
                                   rollbackCode,
                                   fmt::format("Failed to bind import {}; rollback also failed: {}",
                                               binding.Index,
                                               rollbackError));
            }
            RestoreImportBindingsForModule(moduleName, previousBindings);
            return StoreResult(result,
                               status,
                               bindResult,
                               fmt::format("Failed to bind import {}.", binding.Index));
        }
        RecordImportBinding(moduleName,
                            binding.Index,
                            binding.SourceModuleName.c_str(),
                            binding.FunctionDecl.c_str());
    }
    if (!resolvedBindings.empty()) {
        BumpModuleGeneration(moduleName);
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::UnbindImportedFunction(const char *moduleName,
                                                  CKDWORD importIndex,
                                                  CKAngelScriptResult *result) {
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("UnbindImportedFunction", result);
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    if (importIndex >= module->GetImportedFunctionCount()) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
    }
    const int unbindResult = module->UnbindImportedFunction(importIndex);
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return StoreResult(result, status, unbindResult, "Failed to unbind imported function.");
    }
    RemoveImportBinding(moduleName, importIndex);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptManager::UnbindAllImportedFunctions(const char *moduleName,
                                                      CKAngelScriptResult *result) {
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("UnbindAllImportedFunctions", result);
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    const int unbindResult = module->UnbindAllImportedFunctions();
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return StoreResult(result, status, unbindResult, "Failed to unbind imported functions.");
    }
    RemoveImportBindingsForModule(moduleName);
    if (module->GetImportedFunctionCount() > 0) {
        BumpModuleGeneration(moduleName);
    }
    return StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptManager::EnumerateBoundImportEdges(const char *moduleName,
                                                     CKAngelScriptBoundImportEdgeCallback callback,
                                                     void *userData,
                                                     CKAngelScriptResult *result) {
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bound import edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const ModuleState *state = FindModuleState(moduleName);
    if (!state) {
        return StoreResult(result, CKAS_OK);
    }

    for (const ImportBindingEdge &edge : state->BoundImports) {
        CKAngelScriptBoundImportEdge publicEdge = {};
        publicEdge.Size = sizeof(publicEdge);
        publicEdge.ImportModuleName = edge.ImportModuleName.c_str();
        publicEdge.ImportIndex = edge.ImportIndex;
        publicEdge.SourceModuleName = edge.SourceModuleName.c_str();
        publicEdge.FunctionDecl = edge.FunctionDecl.c_str();
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchBoundImportEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result,
                               callbackStatus,
                               0,
                               "Bound import edge enumeration stopped by callback.");
        }
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::EnumerateModuleIncludeEdges(const char *moduleName,
                                                       CKAngelScriptIncludeEdgeCallback callback,
                                                       void *userData,
                                                       CKAngelScriptResult *result) {
    if (!callback) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Include edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const ModuleState *state = FindModuleState(moduleName);
    if (!state) {
        return StoreResult(result, CKAS_OK);
    }

    for (const ScriptIncludeEdge &edge : state->IncludeEdges) {
        CKAngelScriptIncludeEdge publicEdge = {};
        publicEdge.Size = sizeof(publicEdge);
        publicEdge.ModuleName = moduleName;
        publicEdge.FromSection = edge.FromSection.c_str();
        publicEdge.ToSection = edge.ToSection.c_str();
        publicEdge.ResolvedFromSnapshot = edge.ResolvedFromSnapshot ? TRUE : FALSE;
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchIncludeEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result,
                               callbackStatus,
                               0,
                               "Include edge enumeration stopped by callback.");
        }
    }
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::GetModuleFingerprint(const char *moduleName,
                                                CKAngelScriptModuleFingerprint *outFingerprint,
                                                CKAngelScriptResult *result) {
    if (!outFingerprint) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module fingerprint out pointer is required.");
    }
    if (!ScriptApiSupport::HasCompletePublicStruct(*outFingerprint)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module fingerprint size is invalid.");
    }
    CKAngelScriptInitModuleFingerprint(outFingerprint);

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    ModuleState &state = EnsureModuleState(moduleName);
    if (state.FingerprintDirty) {
        RebuildModuleFingerprint(moduleName, state);
    }
    *outFingerprint = state.Fingerprint;
    return StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptManager::SaveModuleBytecode(const CKAngelScriptBytecodeSaveOptions &options,
                                              CKAngelScriptResult *result) {
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode save options size is invalid.");
    }
    const char *moduleName =
        ScriptApiSupport::PublicField(options, &CKAngelScriptBytecodeSaveOptions::ModuleName, static_cast<const char *>(nullptr));
    CKAngelScriptBytecodeWriteCallback write =
        ScriptApiSupport::PublicField(options,
                    &CKAngelScriptBytecodeSaveOptions::Write,
                    static_cast<CKAngelScriptBytecodeWriteCallback>(nullptr));
    void *userData = ScriptApiSupport::PublicField(options, &CKAngelScriptBytecodeSaveOptions::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeSaveOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_BYTECODE_DEFAULT));
    if ((flags & ~static_cast<CKDWORD>(CKAS_BYTECODE_STRIP_DEBUG_INFO)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown SaveModuleBytecode flags.");
    }
    if (m_BytecodeCallbackDepth > 0) {
        return StoreResult(result,
                           CKAS_INVALIDSTATE,
                           0,
                           "SaveModuleBytecode cannot be called from a CKAngelScript bytecode callback.");
    }
    if (!write) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode write callback is required.");
    }

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    const bool stripDebugInfo = ScriptApiSupport::HasPublicFlag(flags, CKAS_BYTECODE_STRIP_DEBUG_INFO);
    bool saved = false;
    {
        ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
        ScriptModuleBytecode::PublicCallbackScope bytecodeCallbackScope(m_BytecodeCallbackDepth);
        saved = ScriptModuleBytecode::SaveModuleByteCode(module,
                                                                           write,
                                                                           userData,
                                                                           stripDebugInfo,
                                                                           angelScriptCode,
                                                                           callbackStatus);
    }
    if (!saved) {
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result, callbackStatus, angelScriptCode, "Bytecode write callback failed.");
        }
        return StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, "Failed to save module bytecode.");
    }
    return StoreResult(result, CKAS_OK, angelScriptCode);
}

CKAS_STATUS ScriptManager::LoadModuleBytecode(const CKAngelScriptBytecodeLoadOptions &options,
                                              CKAngelScriptResult *result) {
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode load options size is invalid.");
    }
    const char *moduleName =
        ScriptApiSupport::PublicField(options, &CKAngelScriptBytecodeLoadOptions::ModuleName, static_cast<const char *>(nullptr));
    CKAngelScriptBytecodeReadCallback read =
        ScriptApiSupport::PublicField(options,
                    &CKAngelScriptBytecodeLoadOptions::Read,
                    static_cast<CKAngelScriptBytecodeReadCallback>(nullptr));
    void *userData = ScriptApiSupport::PublicField(options, &CKAngelScriptBytecodeLoadOptions::UserData, static_cast<void *>(nullptr));
    const CKDWORD flags = ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeLoadOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_BYTECODE_DEFAULT));
    if (!moduleName || moduleName[0] == '\0') {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (IsModuleMutationBlockedByCallback()) {
        return RejectModuleMutationDuringCallback("LoadModuleBytecode", result);
    }
    if (!read) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode read callback is required.");
    }
    if ((flags & ~static_cast<CKDWORD>(CKAS_BYTECODE_REPLACEEXISTING)) != 0) {
        return StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Unknown LoadModuleBytecode flags.");
    }
    if (!m_ScriptEngine) {
        return StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    const bool replacingExisting = HasModule(moduleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(flags, CKAS_BYTECODE_REPLACEEXISTING)) {
            return StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
    }
    const CKAS_STATUS runtimeStatus = CheckModuleRuntimeHandlesReleased(moduleName, result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    if (replacingExisting) {
        const CKAS_STATUS importStatus = CheckModuleHasNoBoundImportConsumers(moduleName, result);
        if (importStatus != CKAS_OK) {
            return importStatus;
        }
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    const std::string transientName = ScriptModuleBytecode::MakeTransientModuleName(m_ScriptEngine,
                                                                                                      moduleName);
    asIScriptModule *candidateModule = nullptr;
    bool loaded = false;
    {
        ScriptModuleBytecode::PublicCallbackScope callbackScope(m_PublicCallbackDepth);
        ScriptModuleBytecode::PublicCallbackScope bytecodeCallbackScope(m_BytecodeCallbackDepth);
        loaded = ScriptModuleBytecode::LoadModuleByteCode(m_ScriptEngine,
                                                                            transientName.c_str(),
                                                                            read,
                                                                            userData,
                                                                            &candidateModule,
                                                                            angelScriptCode,
                                                                            callbackStatus);
    }
    if (!loaded) {
        if (callbackStatus != CKAS_OK) {
            return StoreResult(result, callbackStatus, angelScriptCode, "Bytecode read callback failed.");
        }
        return StoreResult(result, CKAS_COMPILEERROR, angelScriptCode, "Failed to load module bytecode.");
    }

    std::vector<unsigned char> candidateByteCode;
    if (!ScriptModuleBytecode::SaveModuleByteCode(candidateModule,
                                                                    candidateByteCode,
                                                                    angelScriptCode)) {
        candidateModule->Discard();
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           "Failed to snapshot loaded module bytecode.");
    }
    candidateModule->Discard();
    candidateModule = nullptr;

    ModuleReplacementSnapshot snapshot;
    std::string snapshotError;
    if (!CaptureModuleReplacementSnapshot(moduleName, snapshot, angelScriptCode, snapshotError)) {
        return StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, snapshotError);
    }

    RemoveModuleForReplacement(moduleName, snapshot);

    asIScriptModule *committedModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(m_ScriptEngine,
                                                                    moduleName,
                                                                    candidateByteCode,
                                                                    &committedModule,
                                                                    angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored = RestoreModuleReplacementSnapshot(moduleName, snapshot, restoreCode, restoreError);
        return StoreResult(result,
                           CKAS_EXECUTIONFAILED,
                           angelScriptCode,
                           restored
                               ? "Failed to commit loaded module bytecode."
                               : fmt::format("Failed to commit loaded module bytecode; rollback also failed: {}",
                                             restoreError));
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = moduleName;
    committedCache->module = committedModule;
    m_ScriptCache.CacheScript(moduleName, committedCache);
    ClearModuleIncludeEdges(moduleName);
    SetModuleKind(moduleName, ModuleKind::Bytecode);
    BumpModuleGeneration(moduleName);
    return StoreResult(result, CKAS_OK, angelScriptCode);
}

int ScriptManager::LoadModuleFromDefaultOrFile(const char *moduleName, const char *filename) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    XString scriptFilename;
    if (filename) {
        scriptFilename = filename;
    } else {
        scriptFilename = moduleName;
        scriptFilename += ".as";
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, moduleName, scriptFilename.CStr());
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::LoadModuleFromFiles(const char *moduleName, const char **filenames, size_t count) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!m_ScriptEngine)
        return -2;

    std::vector<std::string> files;
    for (size_t i = 0; i < count; i++) {
        XString scriptFilename = filenames[i];
        if (scriptFilename.Find(".as") == XString::NOTFOUND)
            scriptFilename += ".as";
        ResolveScriptFileName(scriptFilename);
        files.emplace_back(scriptFilename.CStr());
    }

    auto cache = m_ScriptCache.LoadScript(m_ScriptEngine, moduleName, files);
    if (!cache)
        return -3;
    return 0;
}

int ScriptManager::CompileModuleFromMemory(const char *moduleName, const char *scriptCode) {
    if (!moduleName || moduleName[0] == '\0')
        return -1;

    if (!scriptCode)
        return -1;

    if (!m_ScriptEngine)
        return -2;

    auto cache = m_ScriptCache.CompileScript(m_ScriptEngine, moduleName, scriptCode);
    if (!cache)
        return -3;
    return 0;
}

bool ScriptManager::DiscardCachedModule(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0')
        return false;
    return m_ScriptCache.UnloadScript(moduleName);
}

bool ScriptManager::DiscardModule(const char *moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return false;
    }
    if (DiscardCachedModule(moduleName)) {
        return true;
    }
    asIScriptModule *module = GetModule(moduleName);
    if (!module) {
        return false;
    }
    module->Discard();
    return true;
}

asIScriptModule *ScriptManager::GetScript(const char *scriptName) {
    if (!m_ScriptEngine)
        return nullptr;
    return m_ScriptEngine->GetModule(scriptName, asGM_ONLY_IF_EXISTS);
}

std::shared_ptr<CachedScript> ScriptManager::GetCachedScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0') {
        return nullptr;
    }
    return m_ScriptCache.GetCachedScript(scriptName);
}

std::shared_ptr<CachedScript> ScriptManager::NewCachedScript(const char *scriptName) {
    if (!scriptName || scriptName[0] == '\0') {
        return nullptr;
    }
    return m_ScriptCache.NewCachedScript(scriptName);
}

bool ScriptManager::RestoreCachedScriptFromChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = NewCachedScript(scriptName);
    if (!script) {
        return false;
    }
    if (script->module) {
        return true;
    }
    return script->LoadFromChunk(chunk);
}

bool ScriptManager::SaveCachedScriptToChunk(const char *scriptName, CKStateChunk *chunk) {
    if (!chunk) {
        return false;
    }
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    return script ? script->SaveToChunk(chunk) : false;
}

bool ScriptManager::ClearCachedScriptCode(const char *scriptName) {
    std::shared_ptr<CachedScript> script = GetCachedScript(scriptName);
    if (!script) {
        return false;
    }
    script->ClearCodeCache();
    return true;
}

