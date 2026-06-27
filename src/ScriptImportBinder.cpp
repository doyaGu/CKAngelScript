#include "ScriptImportBinder.h"

#include <fmt/format.h>

#include "ScriptApiDiagnostics.h"
#include "ScriptApiSupport.h"
#include "ScriptAngelScriptGc.h"
#include "ScriptManager.h"
#include "ScriptModuleBytecode.h"
#include "ScriptModuleMutationPolicy.h"
#include "ScriptPublicOptions.h"

namespace {

struct ResolvedImportBinding {
    CKDWORD Index = 0;
    std::string SourceModuleName;
    std::string FunctionDecl;
    asIScriptFunction *TargetFunction = nullptr;
};

bool CreateTransientImportModule(ScriptManager &manager,
                                 asIScriptModule *sourceModule,
                                 const char *moduleName,
                                 asIScriptModule **outModule,
                                 int &angelScriptCode,
                                 std::string &errorMessage) {
    if (outModule) {
        *outModule = nullptr;
    }
    angelScriptCode = 0;
    errorMessage.clear();
    if (!sourceModule || !manager.GetScriptEngine()) {
        angelScriptCode = -1;
        errorMessage = "Import binding preflight arguments are invalid.";
        return false;
    }

    std::vector<unsigned char> byteCode;
    if (!ScriptModuleBytecode::SaveModuleByteCode(sourceModule, byteCode, angelScriptCode)) {
        errorMessage = "Failed to snapshot importing module for import binding preflight.";
        return false;
    }

    const std::string transientName =
        ScriptModuleBytecode::MakeTransientModuleName(manager.GetScriptEngine(), moduleName);
    std::vector<unsigned char> loadByteCode = byteCode;
    asIScriptModule *transientModule = nullptr;
    if (!ScriptModuleBytecode::LoadModuleByteCode(manager.GetScriptEngine(),
                                                  transientName.c_str(),
                                                  loadByteCode,
                                                  &transientModule,
                                                  angelScriptCode)) {
        errorMessage = "Failed to load import binding preflight module.";
        return false;
    }

    if (outModule) {
        *outModule = transientModule;
    }
    return true;
}

void DiscardTransientImportModule(asIScriptModule *&module) {
    if (module) {
        ScriptDiscardModuleWithGarbageCollection(module);
        module = nullptr;
    }
}

} // namespace

CKAS_STATUS ScriptImportBinder::GetImportedFunctionCount(ReadContext &context,
                                                         const char *moduleName,
                                                         CKDWORD *outCount,
                                                         CKAngelScriptResult *result) {
    if (outCount) {
        *outCount = 0;
    }
    if (!outCount) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Import count out pointer is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = context.Manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    *outCount = static_cast<CKDWORD>(module->GetImportedFunctionCount());
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::EnumerateImportedFunctions(ReadContext &context,
                                                           const char *moduleName,
                                                           CKAngelScriptImportCallback callback,
                                                           void *userData,
                                                           CKAngelScriptResult *result) {
    if (!callback) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = context.Manager.BorrowModule(moduleName, &module, result);
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
            ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchImport(entry, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return context.Diagnostics.StoreResult(result,
                                                   callbackStatus,
                                                   0,
                                                   "Import enumeration stopped by callback.");
        }
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::BindImportedFunction(BindContext &context,
                                                     const CKAngelScriptImportBindOptions &options,
                                                     CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "BindImportedFunction",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    ScriptPublicOptions::ImportBindRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeImportBindOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return context.Diagnostics.StoreResult(result, optionStatus, 0, errorMessage);
    }

    asIScriptModule *importModule = nullptr;
    CKAS_STATUS status = context.Manager.BorrowModule(request.ImportModuleName, &importModule, result);
    if (status != CKAS_OK) {
        return status;
    }
    const asUINT importCount = importModule->GetImportedFunctionCount();
    if (request.ImportIndex >= importCount) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
    }

    const char *defaultSourceModuleName = importModule->GetImportedFunctionSourceModule(request.ImportIndex);
    const char *defaultFunctionDecl = importModule->GetImportedFunctionDeclaration(request.ImportIndex);
    const std::string sourceModuleName =
        ScriptApiSupport::IsNonEmpty(request.SourceModuleName)
            ? request.SourceModuleName
            : (defaultSourceModuleName ? defaultSourceModuleName : "");
    const std::string functionDecl =
        ScriptApiSupport::IsNonEmpty(request.FunctionDecl)
            ? request.FunctionDecl
            : (defaultFunctionDecl ? defaultFunctionDecl : "");
    if (sourceModuleName.empty() || functionDecl.empty()) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_INVALIDARGUMENT,
            0,
            "Import source module and function declaration are required.");
    }

    asIScriptModule *sourceModule = context.Manager.GetModule(sourceModuleName.c_str());
    if (!sourceModule) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_NOTFOUND,
            0,
            fmt::format("Import source module '{}' was not found.", sourceModuleName));
    }
    asIScriptFunction *targetFunction =
        ScriptApiSupport::FindFunctionByDecl(sourceModule, functionDecl.c_str());
    if (!targetFunction) {
        return context.Diagnostics.StoreResult(
            result,
            CKAS_NOTFOUND,
            0,
            fmt::format("Import target function '{}' was not found in module '{}'.",
                        functionDecl,
                        sourceModuleName));
    }

    std::vector<ScriptImportBindingEdge> previousBinding =
        context.StateStore.GetImportBindingForModuleIndex(request.ImportModuleName, request.ImportIndex);

    asIScriptModule *preflightModule = nullptr;
    int preflightCode = 0;
    std::string preflightError;
    if (!CreateTransientImportModule(context.Manager,
                                     importModule,
                                     request.ImportModuleName,
                                     &preflightModule,
                                     preflightCode,
                                     preflightError)) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_EXECUTIONFAILED,
                                               preflightCode,
                                               preflightError);
    }
    const int preflightBindResult =
        preflightModule->BindImportedFunction(request.ImportIndex, targetFunction);
    DiscardTransientImportModule(preflightModule);
    if (preflightBindResult < 0) {
        return context.Diagnostics.StoreResult(
            result,
            ScriptApiSupport::StatusFromImportBindResult(preflightBindResult),
            preflightBindResult,
            "Failed to bind imported function.");
    }

    const int bindResult = importModule->BindImportedFunction(request.ImportIndex, targetFunction);
    if (bindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
        if (!previousBinding.empty()) {
            int rollbackCode = 0;
            std::string rollbackError;
            if (Rebind(context.Manager, previousBinding, rollbackCode, rollbackError)) {
                return context.Diagnostics.StoreResult(
                    result,
                    status,
                    bindResult,
                    "Failed to bind imported function; previous binding was restored.");
            }
            context.StateStore.RemoveImportBinding(request.ImportModuleName, request.ImportIndex);
            context.StateStore.BumpGeneration(request.ImportModuleName);
            return context.Diagnostics.StoreResult(
                result,
                CKAS_EXECUTIONFAILED,
                rollbackCode,
                fmt::format("Failed to bind imported function; rollback also failed: {}",
                            rollbackError));
        }
        if (context.StateStore.RemoveImportBinding(request.ImportModuleName, request.ImportIndex)) {
            context.StateStore.BumpGeneration(request.ImportModuleName);
        }
        return context.Diagnostics.StoreResult(result, status, bindResult, "Failed to bind imported function.");
    }
    context.StateStore.RecordImportBinding(request.ImportModuleName,
                                           request.ImportIndex,
                                           sourceModuleName.c_str(),
                                           functionDecl.c_str());
    context.StateStore.BumpGeneration(request.ImportModuleName);
    return context.Diagnostics.StoreResult(result, CKAS_OK, bindResult);
}

CKAS_STATUS ScriptImportBinder::BindAllImportedFunctions(BindContext &context,
                                                         const char *moduleName,
                                                         CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "BindAllImportedFunctions",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = context.Manager.BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }

    const asUINT count = module->GetImportedFunctionCount();
    std::vector<ResolvedImportBinding> resolvedBindings;
    resolvedBindings.reserve(count);
    for (asUINT i = 0; i < count; ++i) {
        const char *sourceModuleNameView = module->GetImportedFunctionSourceModule(i);
        const char *functionDeclView = module->GetImportedFunctionDeclaration(i);
        const std::string sourceModuleName = sourceModuleNameView ? sourceModuleNameView : "";
        const std::string functionDecl = functionDeclView ? functionDeclView : "";
        if (sourceModuleName.empty() || functionDecl.empty()) {
            return context.Diagnostics.StoreResult(
                result,
                CKAS_INVALIDARGUMENT,
                0,
                fmt::format("Import {} is missing a source module or declaration.", i));
        }
        asIScriptModule *sourceModule = context.Manager.GetModule(sourceModuleName.c_str());
        if (!sourceModule) {
            return context.Diagnostics.StoreResult(
                result,
                CKAS_NOTFOUND,
                0,
                fmt::format("Import {} source module '{}' was not found.",
                            i,
                            sourceModuleName));
        }
        asIScriptFunction *targetFunction =
            ScriptApiSupport::FindFunctionByDecl(sourceModule, functionDecl.c_str());
        if (!targetFunction) {
            return context.Diagnostics.StoreResult(
                result,
                CKAS_NOTFOUND,
                0,
                fmt::format("Import {} target function '{}' was not found in module '{}'.",
                            i,
                            functionDecl,
                            sourceModuleName));
        }
        const asEFuncType functionType = targetFunction->GetFuncType();
        if (functionType != asFUNC_SCRIPT && functionType != asFUNC_SYSTEM) {
            return context.Diagnostics.StoreResult(
                result,
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

    const std::vector<ScriptImportBindingEdge> previousBindings =
        context.StateStore.GetImportBindingsForModule(moduleName);
    if (resolvedBindings.empty()) {
        return context.Diagnostics.StoreResult(result, CKAS_OK);
    }

    asIScriptModule *preflightModule = nullptr;
    int preflightCode = 0;
    std::string preflightError;
    if (!CreateTransientImportModule(context.Manager,
                                     module,
                                     moduleName,
                                     &preflightModule,
                                     preflightCode,
                                     preflightError)) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_EXECUTIONFAILED,
                                               preflightCode,
                                               preflightError);
    }
    for (const ResolvedImportBinding &binding : resolvedBindings) {
        preflightCode = preflightModule->BindImportedFunction(binding.Index, binding.TargetFunction);
        if (preflightCode < 0) {
            DiscardTransientImportModule(preflightModule);
            return context.Diagnostics.StoreResult(result,
                                                   ScriptApiSupport::StatusFromImportBindResult(preflightCode),
                                                   preflightCode,
                                                   fmt::format("Failed to bind import {}.", binding.Index));
        }
    }
    DiscardTransientImportModule(preflightModule);

    std::vector<ResolvedImportBinding> appliedBindings;
    appliedBindings.reserve(resolvedBindings.size());
    for (const ResolvedImportBinding &binding : resolvedBindings) {
        context.StateStore.RemoveImportBinding(moduleName, binding.Index);
        const int bindResult = module->BindImportedFunction(binding.Index, binding.TargetFunction);
        if (bindResult < 0) {
            const CKAS_STATUS status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
            module->UnbindImportedFunction(binding.Index);
            for (auto it = appliedBindings.rbegin(); it != appliedBindings.rend(); ++it) {
                module->UnbindImportedFunction(it->Index);
            }
            context.StateStore.RemoveImportBindingsForModule(moduleName);

            int rollbackCode = 0;
            std::string rollbackError;
            const bool restored = Rebind(context.Manager,
                                         previousBindings,
                                         rollbackCode,
                                         rollbackError);
            if (!restored) {
                context.StateStore.BumpGeneration(moduleName);
                return context.Diagnostics.StoreResult(
                    result,
                    CKAS_EXECUTIONFAILED,
                    rollbackCode,
                    fmt::format("Failed to bind import {}; rollback also failed: {}",
                                binding.Index,
                                rollbackError));
            }
            context.StateStore.RestoreImportBindingsForModule(moduleName, previousBindings);
            return context.Diagnostics.StoreResult(result,
                                                   status,
                                                   bindResult,
                                                   fmt::format("Failed to bind import {}.", binding.Index));
        }
        context.StateStore.RecordImportBinding(moduleName,
                                               binding.Index,
                                               binding.SourceModuleName.c_str(),
                                               binding.FunctionDecl.c_str());
        appliedBindings.push_back(binding);
    }
    if (!resolvedBindings.empty()) {
        context.StateStore.BumpGeneration(moduleName);
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::UnbindImportedFunction(BindContext &context,
                                                       const char *moduleName,
                                                       CKDWORD importIndex,
                                                       CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "UnbindImportedFunction",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = context.Manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    if (importIndex >= module->GetImportedFunctionCount()) {
        return context.Diagnostics.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
    }
    const int unbindResult = module->UnbindImportedFunction(importIndex);
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return context.Diagnostics.StoreResult(result, status, unbindResult, "Failed to unbind imported function.");
    }
    context.StateStore.RemoveImportBinding(moduleName, importIndex);
    context.StateStore.BumpGeneration(moduleName);
    return context.Diagnostics.StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptImportBinder::UnbindAllImportedFunctions(BindContext &context,
                                                           const char *moduleName,
                                                           CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus =
        ScriptModuleMutationPolicy::CheckMutationAllowed(context.Diagnostics,
                                                         context.PublicCallbackDepth,
                                                         "UnbindAllImportedFunctions",
                                                         result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = context.Manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    const int unbindResult = module->UnbindAllImportedFunctions();
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return context.Diagnostics.StoreResult(result, status, unbindResult, "Failed to unbind imported functions.");
    }
    context.StateStore.RemoveImportBindingsForModule(moduleName);
    if (module->GetImportedFunctionCount() > 0) {
        context.StateStore.BumpGeneration(moduleName);
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptImportBinder::EnumerateBoundImportEdges(BindContext &context,
                                                          const char *moduleName,
                                                          CKAngelScriptBoundImportEdgeCallback callback,
                                                          void *userData,
                                                          CKAngelScriptResult *result) {
    if (!callback) {
        return context.Diagnostics.StoreResult(result,
                                               CKAS_INVALIDARGUMENT,
                                               0,
                                               "Bound import edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = context.Manager.BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const std::vector<ScriptImportBindingEdge> edges =
        context.StateStore.GetImportBindingsForModule(moduleName);
    for (const ScriptImportBindingEdge &edge : edges) {
        CKAngelScriptBoundImportEdge publicEdge = {};
        publicEdge.Size = sizeof(publicEdge);
        publicEdge.ImportModuleName = edge.ImportModuleName.c_str();
        publicEdge.ImportIndex = edge.ImportIndex;
        publicEdge.SourceModuleName = edge.SourceModuleName.c_str();
        publicEdge.FunctionDecl = edge.FunctionDecl.c_str();
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptApiSupport::CallbackDepthScope callbackScope(context.PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchBoundImportEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return context.Diagnostics.StoreResult(
                result,
                callbackStatus,
                0,
                "Bound import edge enumeration stopped by callback.");
        }
    }
    return context.Diagnostics.StoreResult(result, CKAS_OK);
}

unsigned long long ScriptImportBinder::BuildDeclaredImportHash(ScriptManager &manager,
                                                               const char *moduleName) const {
    unsigned long long declaredImportHash = ScriptApiSupport::kFnvOffsetBasis;
    asIScriptModule *module = manager.GetModule(moduleName);
    if (module) {
        const asUINT importCount = module->GetImportedFunctionCount();
        ScriptApiSupport::HashValue(declaredImportHash, static_cast<unsigned long long>(importCount));
        for (asUINT i = 0; i < importCount; ++i) {
            ScriptApiSupport::HashValue(declaredImportHash, static_cast<CKDWORD>(i));
            ScriptApiSupport::HashString(declaredImportHash, module->GetImportedFunctionSourceModule(i));
            ScriptApiSupport::HashString(declaredImportHash, module->GetImportedFunctionDeclaration(i));
        }
    }
    return declaredImportHash;
}

bool ScriptImportBinder::Rebind(ScriptManager &manager,
                                const std::vector<ScriptImportBindingEdge> &bindings,
                                int &angelScriptCode,
                                std::string &errorMessage) {
    angelScriptCode = 0;
    errorMessage.clear();
    struct ResolvedImportBinding {
        const ScriptImportBindingEdge *Edge = nullptr;
        asIScriptModule *ImportModule = nullptr;
        asIScriptFunction *TargetFunction = nullptr;
    };
    struct PreflightImportModule {
        std::string Name;
        asIScriptModule *Source = nullptr;
        asIScriptModule *Transient = nullptr;
    };
    std::vector<ResolvedImportBinding> resolvedBindings;
    resolvedBindings.reserve(bindings.size());
    for (const ScriptImportBindingEdge &edge : bindings) {
        asIScriptModule *importModule = manager.GetModule(edge.ImportModuleName.c_str());
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
        asIScriptModule *sourceModule = manager.GetModule(edge.SourceModuleName.c_str());
        if (!sourceModule) {
            angelScriptCode = asNO_MODULE;
            errorMessage = fmt::format("Failed to restore import binding: source module '{}' was not found.",
                                       edge.SourceModuleName);
            return false;
        }
        asIScriptFunction *targetFunction =
            ScriptApiSupport::FindFunctionByDecl(sourceModule, edge.FunctionDecl.c_str());
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

    std::vector<PreflightImportModule> preflightModules;
    auto discardPreflightModules = [&preflightModules]() {
        for (PreflightImportModule &module : preflightModules) {
            DiscardTransientImportModule(module.Transient);
        }
        preflightModules.clear();
    };
    for (const ResolvedImportBinding &resolved : resolvedBindings) {
        const ScriptImportBindingEdge &edge = *resolved.Edge;
        PreflightImportModule *preflight = nullptr;
        for (PreflightImportModule &candidate : preflightModules) {
            if (candidate.Source == resolved.ImportModule &&
                candidate.Name == edge.ImportModuleName) {
                preflight = &candidate;
                break;
            }
        }
        if (!preflight) {
            PreflightImportModule module;
            module.Name = edge.ImportModuleName;
            module.Source = resolved.ImportModule;
            if (!CreateTransientImportModule(manager,
                                             resolved.ImportModule,
                                             edge.ImportModuleName.c_str(),
                                             &module.Transient,
                                             angelScriptCode,
                                             errorMessage)) {
                discardPreflightModules();
                if (errorMessage.empty()) {
                    errorMessage = "Failed to preflight import binding restore.";
                }
                return false;
            }
            preflightModules.push_back(module);
            preflight = &preflightModules.back();
        }

        angelScriptCode =
            preflight->Transient->BindImportedFunction(edge.ImportIndex, resolved.TargetFunction);
        if (angelScriptCode < 0) {
            errorMessage = fmt::format("Failed to preflight import binding {} in module '{}'.",
                                       edge.ImportIndex,
                                       edge.ImportModuleName);
            discardPreflightModules();
            return false;
        }
    }
    discardPreflightModules();

    std::vector<ResolvedImportBinding> appliedBindings;
    appliedBindings.reserve(resolvedBindings.size());
    for (const ResolvedImportBinding &resolved : resolvedBindings) {
        const ScriptImportBindingEdge &edge = *resolved.Edge;
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
