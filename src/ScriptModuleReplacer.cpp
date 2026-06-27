#include "ScriptModuleReplacer.h"

#include <fmt/format.h>

#include "ScriptApiDiagnostics.h"
#include "ScriptApiSupport.h"
#include "ScriptAngelScriptGc.h"
#include "ScriptCache.h"
#include "ScriptImportBinder.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
#include "ScriptModuleRegistry.h"
#include "ScriptModuleStateStore.h"

struct ScriptModuleReplacer::Snapshot {
    std::shared_ptr<CachedScript> Cache;
    std::vector<std::tuple<std::string, std::string>> Sections;
    ScriptMetadata Metadata;
    std::vector<ScriptImportBindingEdge> ImportBindings;
    std::vector<unsigned char> ByteCode;
    bool SourceSnapshotSections = false;
    bool HasModule = false;
};

std::shared_ptr<CachedScript> ScriptModuleReplacer::BuildTransientModule(
    ScriptManager &manager,
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    ScriptApiDiagnostics &diagnosticsStore,
    int &angelScriptCode,
    std::string &diagnostics,
    std::vector<CapturedScriptMessage> *messages) {
    angelScriptCode = 0;
    diagnostics.clear();
    if (!manager.GetScriptEngine() || !ScriptApiSupport::IsNonEmpty(moduleName) || sections.empty()) {
        angelScriptCode = -1;
        return nullptr;
    }

    auto script = std::make_shared<CachedScript>();
    script->name = moduleName;
    script->sourceSnapshotSections = sourceSnapshotSections;
    for (const auto &section : sections) {
        script->AddSection(std::get<0>(section), std::get<1>(section));
    }

    diagnosticsStore.BeginScriptMessageCapture();
    const bool built = script->Build(manager.GetScriptEngine());
    diagnostics = diagnosticsStore.EndScriptMessageCapture(messages);
    if (!built) {
        angelScriptCode = -3;
        if (script->module) {
            script->Discard();
        }
        return nullptr;
    }
    return script;
}

bool ScriptModuleReplacer::CaptureSnapshot(ScriptManager &manager,
                                           ScriptModuleRegistry &registry,
                                           const ScriptModuleStateStore &stateStore,
                                           const char *moduleName,
                                           Snapshot &snapshot,
                                           int &angelScriptCode,
                                           std::string &errorMessage) {
    snapshot = Snapshot();
    angelScriptCode = 0;
    errorMessage.clear();
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        angelScriptCode = -1;
        errorMessage = "Module name is required.";
        return false;
    }

    snapshot.Cache = registry.GetCachedScript(moduleName);
    snapshot.ImportBindings = stateStore.GetImportBindingsForModule(moduleName);
    if (snapshot.Cache) {
        snapshot.Sections = snapshot.Cache->sections;
        snapshot.Metadata = snapshot.Cache->metadata;
        snapshot.SourceSnapshotSections = snapshot.Cache->sourceSnapshotSections;
    }

    asIScriptModule *module = manager.GetModule(moduleName);
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

void ScriptModuleReplacer::RemoveForReplacement(ScriptManager &manager,
                                                ScriptModuleRegistry &registry,
                                                ScriptModuleStateStore &stateStore,
                                                const char *moduleName,
                                                Snapshot &snapshot) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }

    registry.Invalidate(moduleName);
    stateStore.RemoveImportBindingsForModule(moduleName);
    if (snapshot.Cache && snapshot.Cache->module) {
        snapshot.Cache->Discard();
        return;
    }

    asIScriptModule *module = manager.GetModule(moduleName);
    if (module) {
        ScriptDiscardModuleWithGarbageCollection(module);
    }
}

bool ScriptModuleReplacer::RestoreSnapshot(ScriptManager &manager,
                                           ScriptModuleRegistry &registry,
                                           ScriptModuleStateStore &stateStore,
                                           ScriptImportBinder &importBinder,
                                           const char *moduleName,
                                           Snapshot &snapshot,
                                           int &angelScriptCode,
                                           std::string &errorMessage) {
    angelScriptCode = 0;
    errorMessage.clear();
    if (!snapshot.HasModule) {
        return true;
    }

    asIScriptModule *restoredModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                  moduleName,
                                                  snapshot.ByteCode,
                                                  &restoredModule,
                                                  angelScriptCode)) {
        errorMessage = "Failed to restore previous script module bytecode.";
        return false;
    }

    std::shared_ptr<CachedScript> restoredCache =
        snapshot.Cache ? snapshot.Cache : std::make_shared<CachedScript>();
    restoredCache->name = moduleName ? moduleName : "";
    restoredCache->sections = snapshot.Sections;
    restoredCache->sourceSnapshotSections = snapshot.SourceSnapshotSections;
    restoredCache->metadata = snapshot.Metadata;
    restoredCache->module = restoredModule;
    registry.CacheScript(moduleName, restoredCache);
    if (!importBinder.Rebind(manager,
                             snapshot.ImportBindings,
                             angelScriptCode,
                             errorMessage)) {
        return false;
    }
    stateStore.RestoreImportBindingsForModule(moduleName, snapshot.ImportBindings);
    return true;
}

bool ScriptModuleReplacer::CommitBytecodeCandidate(
    ScriptManager &manager,
    ScriptModuleRegistry &registry,
    ScriptModuleStateStore &stateStore,
    ScriptImportBinder &importBinder,
    const char *moduleName,
    const std::vector<unsigned char> &candidateByteCode,
    const char *commitFailedMessage,
    const char *rollbackFailedPrefix,
    asIScriptModule **outCommittedModule,
    int &angelScriptCode,
    std::string &errorMessage) {
    if (outCommittedModule) {
        *outCommittedModule = nullptr;
    }
    angelScriptCode = 0;
    errorMessage.clear();

    Snapshot snapshot;
    std::string snapshotError;
    if (!CaptureSnapshot(manager, registry, stateStore, moduleName, snapshot, angelScriptCode, snapshotError)) {
        errorMessage = snapshotError;
        return false;
    }

    RemoveForReplacement(manager, registry, stateStore, moduleName, snapshot);

    std::vector<unsigned char> loadByteCode = candidateByteCode;
    asIScriptModule *committedModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                  moduleName,
                                                  loadByteCode,
                                                  &committedModule,
                                                  angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored =
            RestoreSnapshot(manager, registry, stateStore, importBinder, moduleName, snapshot, restoreCode, restoreError);
        if (restored) {
            errorMessage = commitFailedMessage ? commitFailedMessage : "Failed to commit module bytecode.";
        } else {
            errorMessage = fmt::format("{}; rollback also failed: {}",
                                       rollbackFailedPrefix ? rollbackFailedPrefix : "Failed to commit module bytecode",
                                       restoreError);
        }
        return false;
    }

    if (outCommittedModule) {
        *outCommittedModule = committedModule;
    }
    return true;
}

CKAS_STATUS ScriptModuleReplacer::ReplaceFromSections(
    ScriptManager &manager,
    ScriptModuleRegistry &registry,
    ScriptModuleStateStore &stateStore,
    ScriptImportBinder &importBinder,
    ScriptApiDiagnostics &diagnosticsStore,
    const char *moduleName,
    const std::vector<std::tuple<std::string, std::string>> &sections,
    bool sourceSnapshotSections,
    CKAngelScriptResult *result) {
    int angelScriptCode = 0;
    std::string diagnostics;
    std::vector<CapturedScriptMessage> diagnosticMessages;
    const std::string transientName =
        ScriptModuleBytecode::MakeTransientModuleName(manager.GetScriptEngine(), moduleName);
    std::shared_ptr<CachedScript> candidate =
        BuildTransientModule(manager,
                             transientName.c_str(),
                             sections,
                             sourceSnapshotSections,
                             diagnosticsStore,
                             angelScriptCode,
                             diagnostics,
                             &diagnosticMessages);
    if (!candidate || !candidate->module) {
        return diagnosticsStore.StoreResult(
            result,
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
        return diagnosticsStore.StoreResult(result,
                                            CKAS_EXECUTIONFAILED,
                                            angelScriptCode,
                                            "Failed to snapshot replacement script module bytecode.");
    }

    asIScriptModule *committedModule = nullptr;
    std::string commitError;
    if (!CommitBytecodeCandidate(manager,
                                 registry,
                                 stateStore,
                                 importBinder,
                                 moduleName,
                                 candidateByteCode,
                                 "Failed to commit replacement script module bytecode.",
                                 "Failed to commit replacement script module bytecode",
                                 &committedModule,
                                 angelScriptCode,
                                 commitError)) {
        candidate->Discard();
        return diagnosticsStore.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, commitError);
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
    registry.CacheScript(moduleName, committedCache);
    candidate->Discard();
    stateStore.SetIncludeEdges(moduleName, committedCache->includeEdges);
    stateStore.SetKind(moduleName, ScriptModuleKind::Source);
    stateStore.BumpGeneration(moduleName);
    return diagnosticsStore.StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptModuleReplacer::ReplaceFromBytecode(
    ScriptManager &manager,
    ScriptModuleRegistry &registry,
    ScriptModuleStateStore &stateStore,
    ScriptImportBinder &importBinder,
    ScriptApiDiagnostics &diagnosticsStore,
    const char *moduleName,
    const std::vector<unsigned char> &byteCode,
    CKAngelScriptResult *result) {
    int angelScriptCode = 0;
    asIScriptModule *committedModule = nullptr;
    std::string commitError;
    if (!CommitBytecodeCandidate(manager,
                                 registry,
                                 stateStore,
                                 importBinder,
                                 moduleName,
                                 byteCode,
                                 "Failed to commit loaded module bytecode.",
                                 "Failed to commit loaded module bytecode",
                                 &committedModule,
                                 angelScriptCode,
                                 commitError)) {
        return diagnosticsStore.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, commitError);
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = moduleName ? moduleName : "";
    committedCache->module = committedModule;
    registry.CacheScript(moduleName, committedCache);
    stateStore.ClearIncludeEdges(moduleName);
    stateStore.SetKind(moduleName, ScriptModuleKind::Bytecode);
    stateStore.BumpGeneration(moduleName);
    return diagnosticsStore.StoreResult(result, CKAS_OK, angelScriptCode);
}
