#include "ScriptModuleBytecodeStore.h"

#include <memory>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "ScriptApiSupport.h"
#include "ScriptCache.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
#include "ScriptModuleReplacer.h"
#include "ScriptPublicOptions.h"

CKAS_STATUS ScriptModuleBytecodeStore::Save(ScriptManager &manager,
                                            const CKAngelScriptBytecodeSaveOptions &options,
                                            CKAngelScriptResult *result) {
    ScriptPublicOptions::BytecodeSaveRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeBytecodeSaveOptions(options,
                                                                              request,
                                                                              errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (manager.m_BytecodeCallbackDepth > 0) {
        return manager.StoreResult(result,
                                   CKAS_INVALIDSTATE,
                                   0,
                                   "SaveModuleBytecode cannot be called from a CKAngelScript bytecode callback.");
    }
    if (!request.Write) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode write callback is required.");
    }

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = manager.BorrowModule(request.ModuleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    bool saved = false;
    {
        ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
        ScriptApiSupport::CallbackDepthScope bytecodeCallbackScope(manager.m_BytecodeCallbackDepth);
        saved = ScriptModuleBytecode::SaveModuleByteCode(module,
                                                         request.Write,
                                                         request.UserData,
                                                         request.StripDebugInfo,
                                                         angelScriptCode,
                                                         callbackStatus);
    }
    if (!saved) {
        if (callbackStatus != CKAS_OK) {
            return manager.StoreResult(result, callbackStatus, angelScriptCode, "Bytecode write callback failed.");
        }
        return manager.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, "Failed to save module bytecode.");
    }
    return manager.StoreResult(result, CKAS_OK, angelScriptCode);
}

CKAS_STATUS ScriptModuleBytecodeStore::Load(ScriptManager &manager,
                                            const CKAngelScriptBytecodeLoadOptions &options,
                                            CKAngelScriptResult *result) {
    ScriptPublicOptions::BytecodeLoadRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeBytecodeLoadOptions(options,
                                                                              request,
                                                                              errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (!ScriptApiSupport::IsNonEmpty(request.ModuleName)) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    if (manager.IsModuleMutationBlockedByCallback()) {
        return manager.RejectModuleMutationDuringCallback("LoadModuleBytecode", result);
    }
    if (!request.Read) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode read callback is required.");
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    const bool replacingExisting = manager.HasModule(request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_BYTECODE_REPLACEEXISTING)) {
            return manager.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
    }
    const CKAS_STATUS runtimeStatus = manager.CheckModuleRuntimeHandlesReleased(request.ModuleName, result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    if (replacingExisting) {
        const CKAS_STATUS importStatus = manager.CheckModuleHasNoBoundImportConsumers(request.ModuleName, result);
        if (importStatus != CKAS_OK) {
            return importStatus;
        }
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    const std::string transientName =
        ScriptModuleBytecode::MakeTransientModuleName(manager.GetScriptEngine(), request.ModuleName);
    asIScriptModule *candidateModule = nullptr;
    bool loaded = false;
    {
        ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
        ScriptApiSupport::CallbackDepthScope bytecodeCallbackScope(manager.m_BytecodeCallbackDepth);
        loaded = ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                          transientName.c_str(),
                                                          request.Read,
                                                          request.UserData,
                                                          &candidateModule,
                                                          angelScriptCode,
                                                          callbackStatus);
    }
    if (!loaded) {
        if (callbackStatus != CKAS_OK) {
            return manager.StoreResult(result, callbackStatus, angelScriptCode, "Bytecode read callback failed.");
        }
        return manager.StoreResult(result, CKAS_COMPILEERROR, angelScriptCode, "Failed to load module bytecode.");
    }

    std::vector<unsigned char> candidateByteCode;
    if (!ScriptModuleBytecode::SaveModuleByteCode(candidateModule,
                                                  candidateByteCode,
                                                  angelScriptCode)) {
        candidateModule->Discard();
        return manager.StoreResult(result,
                                   CKAS_EXECUTIONFAILED,
                                   angelScriptCode,
                                   "Failed to snapshot loaded module bytecode.");
    }
    candidateModule->Discard();
    candidateModule = nullptr;

    ScriptModuleReplacer::Snapshot snapshot;
    std::string snapshotError;
    if (!manager.m_ModuleReplacer.CaptureSnapshot(manager,
                                                  request.ModuleName,
                                                  snapshot,
                                                  angelScriptCode,
                                                  snapshotError)) {
        return manager.StoreResult(result, CKAS_EXECUTIONFAILED, angelScriptCode, snapshotError);
    }

    manager.m_ModuleReplacer.RemoveForReplacement(manager, request.ModuleName, snapshot);

    asIScriptModule *committedModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                  request.ModuleName,
                                                  candidateByteCode,
                                                  &committedModule,
                                                  angelScriptCode)) {
        int restoreCode = 0;
        std::string restoreError;
        const bool restored = manager.m_ModuleReplacer.RestoreSnapshot(manager,
                                                                       request.ModuleName,
                                                                       snapshot,
                                                                       restoreCode,
                                                                       restoreError);
        return manager.StoreResult(result,
                                   CKAS_EXECUTIONFAILED,
                                   angelScriptCode,
                                   restored
                                       ? "Failed to commit loaded module bytecode."
                                       : fmt::format("Failed to commit loaded module bytecode; rollback also failed: {}",
                                                     restoreError));
    }

    auto committedCache = std::make_shared<CachedScript>();
    committedCache->name = request.ModuleName;
    committedCache->module = committedModule;
    manager.m_ScriptCache.CacheScript(request.ModuleName, committedCache);
    manager.ClearModuleIncludeEdges(request.ModuleName);
    manager.SetModuleKind(request.ModuleName, ScriptModuleKind::Bytecode);
    manager.BumpModuleGeneration(request.ModuleName);
    return manager.StoreResult(result, CKAS_OK, angelScriptCode);
}
