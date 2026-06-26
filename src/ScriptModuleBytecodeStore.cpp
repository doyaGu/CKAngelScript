#include "ScriptModuleBytecodeStore.h"

#include <string>
#include <vector>

#include "ScriptApiSupport.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
#include "ScriptModuleMutationPolicy.h"
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
    CKAS_STATUS mutationStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(manager.m_Diagnostics,
                                                         manager.m_PublicCallbackDepth,
                                                         "LoadModuleBytecode",
                                                         result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!request.Read) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bytecode read callback is required.");
    }
    if (!manager.GetScriptEngine()) {
        return manager.StoreResult(result, CKAS_NOTINITIALIZED, 0, "AngelScript engine is not initialized.");
    }

    const bool replacingExisting = manager.m_ModuleRegistry.Has(manager, request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_BYTECODE_REPLACEEXISTING)) {
            return manager.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
    }
    const CKAS_STATUS runtimeStatus = ScriptModuleMutationPolicy::CheckRuntimeHandlesReleased(
        manager.m_HandleRegistry,
        manager.m_Diagnostics,
        request.ModuleName,
        result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    if (replacingExisting) {
        const CKAS_STATUS importStatus = ScriptModuleMutationPolicy::CheckNoBoundImportConsumers(
            manager.m_ModuleStateStore,
            manager.m_Diagnostics,
            request.ModuleName,
            result);
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

    return manager.m_ModuleReplacer.ReplaceFromBytecode(manager,
                                                        request.ModuleName,
                                                        candidateByteCode,
                                                        result);
}
