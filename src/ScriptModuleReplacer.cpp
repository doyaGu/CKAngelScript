#include "ScriptModuleReplacer.h"

#include <fmt/format.h>

#include "ScriptApiSupport.h"
#include "ScriptCache.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
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

    manager.BeginScriptMessageCapture();
    const bool built = script->Build(manager.GetScriptEngine());
    diagnostics = manager.EndScriptMessageCapture(messages);
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

    snapshot.Cache = manager.m_ModuleRegistry.GetCachedScript(moduleName);
    snapshot.ImportBindings = manager.m_ModuleStateStore.GetImportBindingsForModule(moduleName);
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
                                                const char *moduleName,
                                                Snapshot &snapshot) {
    if (!ScriptApiSupport::IsNonEmpty(moduleName)) {
        return;
    }

    manager.m_ModuleRegistry.Invalidate(moduleName);
    manager.m_ModuleStateStore.RemoveImportBindingsForModule(moduleName);
    if (snapshot.Cache && snapshot.Cache->module) {
        snapshot.Cache->module->Discard();
        snapshot.Cache->module = nullptr;
        return;
    }

    asIScriptModule *module = manager.GetModule(moduleName);
    if (module) {
        module->Discard();
    }
}

bool ScriptModuleReplacer::RestoreSnapshot(ScriptManager &manager,
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
    manager.m_ModuleRegistry.CacheScript(moduleName, restoredCache);
    if (!manager.m_ImportBinder.Rebind(manager,
                                       snapshot.ImportBindings,
                                       angelScriptCode,
                                       errorMessage)) {
        return false;
    }
    manager.m_ModuleStateStore.RestoreImportBindingsForModule(moduleName, snapshot.ImportBindings);
    return true;
}

bool ScriptModuleReplacer::CommitBytecodeCandidate(
    ScriptManager &manager,
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
    if (!CaptureSnapshot(manager, moduleName, snapshot, angelScriptCode, snapshotError)) {
        errorMessage = snapshotError;
        return false;
    }

    RemoveForReplacement(manager, moduleName, snapshot);

    std::vector<unsigned char> loadByteCode = candidateByteCode;
    asIScriptModule *committedModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                  moduleName,
                                                  loadByteCode,
                                                  &committedModule,
                                                  angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored = RestoreSnapshot(manager, moduleName, snapshot, restoreCode, restoreError);
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
                             angelScriptCode,
                             diagnostics,
                             &diagnosticMessages);
    if (!candidate || !candidate->module) {
        return manager.StoreResult(result,
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
        return manager.StoreResult(result,
                                   CKAS_EXECUTIONFAILED,
                                   angelScriptCode,
                                   "Failed to snapshot replacement script module bytecode.");
    }

    asIScriptModule *committedModule = nullptr;
    std::string commitError;
    if (!CommitBytecodeCandidate(manager,
                                 moduleName,
                                 candidateByteCode,
                                 "Failed to commit replacement script module bytecode.",
                                 "Failed to commit replacement script module bytecode",
                                 &committedModule,
                                 angelScriptCode,
                                 commitError)) {
        candidate->Discard();
        return manager.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, commitError);
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
    manager.m_ModuleRegistry.CacheScript(moduleName, committedCache);
    candidate->Discard();
    manager.m_ModuleStateStore.SetIncludeEdges(moduleName, committedCache->includeEdges);
    manager.m_ModuleStateStore.SetKind(moduleName, ScriptModuleKind::Source);
    manager.m_ModuleStateStore.BumpGeneration(moduleName);
    return manager.StoreResult(result, CKAS_OK, 0, std::string(), std::string(), &diagnosticMessages);
}

CKAS_STATUS ScriptModuleReplacer::ReplaceFromBytecode(
    ScriptManager &manager,
    const char *moduleName,
    const std::vector<unsigned char> &byteCode,
    CKAngelScriptResult *result) {
    int angelScriptCode = 0;
    asIScriptModule *committedModule = nullptr;
    std::string commitError;
    if (!CommitBytecodeCandidate(manager,
                                 moduleName,
                                 byteCode,
                                 "Failed to commit loaded module bytecode.",
                                 "Failed to commit loaded module bytecode",
                                 &committedModule,
                                 angelScriptCode,
                                 commitError)) {
        return manager.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, commitError);
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = moduleName ? moduleName : "";
    committedCache->module = committedModule;
    manager.m_ModuleRegistry.CacheScript(moduleName, committedCache);
    manager.m_ModuleStateStore.ClearIncludeEdges(moduleName);
    manager.m_ModuleStateStore.SetKind(moduleName, ScriptModuleKind::Bytecode);
    manager.m_ModuleStateStore.BumpGeneration(moduleName);
    return manager.StoreResult(result, CKAS_OK, angelScriptCode);
}
