#include "ScriptModuleBytecodeStore.h"

#include <string>
#include <vector>

#include "ScriptApiDiagnostics.h"
#include "ScriptApiSupport.h"
#include "ScriptAngelScriptGc.h"
#include "ScriptHandleRegistry.h"
#include "ScriptImportBinder.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
#include "ScriptModuleMutationPolicy.h"
#include "ScriptModuleRegistry.h"
#include "ScriptModuleStateStore.h"
#include "ScriptPublicOptions.h"

CKAS_STATUS ScriptModuleBytecodeStore::Save(SaveContext &context,
                                            const CKAngelScriptBytecodeSaveOptions &options,
                                            CKAngelScriptResult *result) {
    ScriptPublicOptions::BytecodeSaveRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeBytecodeSaveOptions(options,
                                                                              request,
                                                                              errorMessage);
    if (optionStatus != CKAS_OK) {
        return context.Diagnostics.StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (context.BytecodeCallbackDepth > 0) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_INVALIDSTATE,
            0,
            "SaveModuleBytecode cannot be called from a CKAngelScript bytecode callback.");
    }
    if (!request.Write) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Bytecode write callback is required.");
    }

    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = context.Manager.BorrowModule(request.ModuleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    bool saved = false;
    {
        ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
        ScriptApiSupport::CallbackDepthScope bytecodeCallbackScope(context.BytecodeCallbackDepth);
        saved = ScriptModuleBytecode::SaveModuleByteCode(module,
                                                         request.Write,
                                                         request.UserData,
                                                         request.StripDebugInfo,
                                                         angelScriptCode,
                                                         callbackStatus);
    }
    if (!saved) {
        if (callbackStatus != CKAS_OK) {
            return context.Diagnostics.StoreResult(result,
                                                   callbackStatus,
                                                   angelScriptCode,
                                                   "Bytecode write callback failed.");
        }
        return context.Diagnostics.StoreResult(result,
                                               CKAS_EXECUTIONFAILED,
                                               angelScriptCode,
                                               "Failed to save module bytecode.");
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK, angelScriptCode);
}

CKAS_STATUS ScriptModuleBytecodeStore::Load(LoadContext &context,
                                            const CKAngelScriptBytecodeLoadOptions &options,
                                            CKAngelScriptResult *result) {
    ScriptPublicOptions::BytecodeLoadRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeBytecodeLoadOptions(options,
                                                                              request,
                                                                              errorMessage);
    if (optionStatus != CKAS_OK) {
        return context.Diagnostics.StoreResult(result, optionStatus, 0, errorMessage);
    }
    if (context.BytecodeCallbackDepth > 0) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_INVALIDSTATE,
            0,
            "LoadModuleBytecode cannot be called from a CKAngelScript bytecode callback.");
    }
    if (!ScriptApiSupport::IsNonEmpty(request.ModuleName)) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Module name is required.");
    }
    CKAS_STATUS mutationStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "LoadModuleBytecode",
                                                         result);
    if (mutationStatus != CKAS_OK) {
        return mutationStatus;
    }
    if (!request.Read) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Bytecode read callback is required.");
    }
    if (!context.Manager.GetScriptEngine()) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_NOTINITIALIZED,
                                               0,
                                               "AngelScript engine is not initialized.");
    }

    const bool replacingExisting = context.ModuleRegistry.Has(context.Manager, request.ModuleName);
    if (replacingExisting) {
        if (!ScriptApiSupport::HasPublicFlag(request.Flags, CKAS_BYTECODE_REPLACEEXISTING)) {
            return context.Diagnostics.StoreResult(result, CKAS_ALREADYEXISTS, 0, "Module already exists.");
        }
    }
    const CKAS_STATUS runtimeStatus = ScriptModuleMutationPolicy::CheckRuntimeHandlesReleased(
        context.HandleRegistry,
        context.AsyncScheduler,
        context.Diagnostics,
        request.ModuleName,
        result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    if (replacingExisting) {
        const CKAS_STATUS importStatus = ScriptModuleMutationPolicy::CheckNoBoundImportConsumers(
            context.ModuleStateStore,
            context.Diagnostics,
            request.ModuleName,
            result);
        if (importStatus != CKAS_OK) {
            return importStatus;
        }
    }

    int angelScriptCode = 0;
    CKAS_STATUS callbackStatus = CKAS_OK;
    const std::string transientName =
        ScriptModuleBytecode::MakeTransientModuleName(context.Manager.GetScriptEngine(), request.ModuleName);
    asIScriptModule *candidateModule = nullptr;
    bool loaded = false;
    {
        ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
        ScriptApiSupport::CallbackDepthScope bytecodeCallbackScope(context.BytecodeCallbackDepth);
        loaded = ScriptModuleBytecode::LoadModuleByteCode(context.Manager.GetScriptEngine(),
                                                          transientName.c_str(),
                                                          request.Read,
                                                          request.UserData,
                                                          &candidateModule,
                                                          angelScriptCode,
                                                          callbackStatus);
    }
    if (!loaded) {
        if (callbackStatus != CKAS_OK) {
            return context.Diagnostics.StoreResult(result,
                                                   callbackStatus,
                                                   angelScriptCode,
                                                   "Bytecode read callback failed.");
        }
        return context.Diagnostics.StoreResult(result,
                                               CKAS_COMPILEERROR,
                                               angelScriptCode,
                                               "Failed to load module bytecode.");
    }

    std::vector<unsigned char> candidateByteCode;
    if (!ScriptModuleBytecode::SaveModuleByteCode(candidateModule,
                                                  candidateByteCode,
                                                  angelScriptCode)) {
        ScriptDiscardModuleWithGarbageCollection(candidateModule);
        return context.Diagnostics.StoreResult(result,
                                               CKAS_EXECUTIONFAILED,
                                               angelScriptCode,
                                               "Failed to snapshot loaded module bytecode.");
    }
    ScriptDiscardModuleWithGarbageCollection(candidateModule);
    candidateModule = nullptr;

    ScriptModuleRegistry::MutationContext mutationContext = {
        context.Manager,
        context.ModuleStateStore,
        context.HandleRegistry,
        context.AsyncScheduler,
        context.ImportBinder,
        context.Diagnostics,
        context.PublicCallbackDepth};
    return context.ModuleRegistry.ReplaceFromBytecode(mutationContext,
                                                      request.ModuleName,
                                                      candidateByteCode,
                                                      result);
}
