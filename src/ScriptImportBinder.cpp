#include "ScriptImportBinder.h"

#include <fmt/format.h>

#include "ScriptApiSupport.h"
#include "ScriptManager.h"
#include "ScriptPublicOptions.h"

namespace {

struct ResolvedImportBinding {
    CKDWORD Index = 0;
    std::string SourceModuleName;
    std::string FunctionDecl;
    asIScriptFunction *TargetFunction = nullptr;
};

} // namespace

CKAS_STATUS ScriptImportBinder::GetImportedFunctionCount(ScriptManager &manager,
                                                         const char *moduleName,
                                                         CKDWORD *outCount,
                                                         CKAngelScriptResult *result) {
    if (outCount) {
        *outCount = 0;
    }
    if (!outCount) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import count out pointer is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    *outCount = static_cast<CKDWORD>(module->GetImportedFunctionCount());
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::EnumerateImportedFunctions(ScriptManager &manager,
                                                           const char *moduleName,
                                                           CKAngelScriptImportCallback callback,
                                                           void *userData,
                                                           CKAngelScriptResult *result) {
    if (!callback) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = manager.BorrowModule(moduleName, &module, result);
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
            ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchImport(entry, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return manager.StoreResult(result,
                                       callbackStatus,
                                       0,
                                       "Import enumeration stopped by callback.");
        }
    }
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::BindImportedFunction(ScriptManager &manager,
                                                     const CKAngelScriptImportBindOptions &options,
                                                     CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus = manager.CheckModuleMutationAllowed("BindImportedFunction", result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    ScriptPublicOptions::ImportBindRequest request;
    std::string errorMessage;
    CKAS_STATUS optionStatus = ScriptPublicOptions::DecodeImportBindOptions(options, request, errorMessage);
    if (optionStatus != CKAS_OK) {
        return manager.StoreResult(result, optionStatus, 0, errorMessage);
    }

    asIScriptModule *importModule = nullptr;
    CKAS_STATUS status = manager.BorrowModule(request.ImportModuleName, &importModule, result);
    if (status != CKAS_OK) {
        return status;
    }
    const asUINT importCount = importModule->GetImportedFunctionCount();
    if (request.ImportIndex >= importCount) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
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
        return manager.StoreResult(result,
                                   CKAS_INVALIDARGUMENT,
                                   0,
                                   "Import source module and function declaration are required.");
    }

    asIScriptModule *sourceModule = manager.GetModule(sourceModuleName.c_str());
    if (!sourceModule) {
        return manager.StoreResult(result,
                                   CKAS_NOTFOUND,
                                   0,
                                   fmt::format("Import source module '{}' was not found.", sourceModuleName));
    }
    asIScriptFunction *targetFunction = sourceModule->GetFunctionByDecl(functionDecl.c_str());
    if (!targetFunction) {
        return manager.StoreResult(result,
                                   CKAS_NOTFOUND,
                                   0,
                                   fmt::format("Import target function '{}' was not found in module '{}'.",
                                               functionDecl,
                                               sourceModuleName));
    }

    std::vector<ScriptImportBindingEdge> previousBinding =
        manager.m_ModuleStateStore.GetImportBindingForModuleIndex(request.ImportModuleName, request.ImportIndex);

    const int bindResult = importModule->BindImportedFunction(request.ImportIndex, targetFunction);
    if (bindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
        if (!previousBinding.empty()) {
            int rollbackCode = 0;
            std::string rollbackError;
            if (Rebind(manager, previousBinding, rollbackCode, rollbackError)) {
                return manager.StoreResult(result,
                                           status,
                                           bindResult,
                                           "Failed to bind imported function; previous binding was restored.");
            }
            manager.m_ModuleStateStore.RemoveImportBinding(request.ImportModuleName, request.ImportIndex);
            manager.m_ModuleStateStore.BumpGeneration(request.ImportModuleName);
            return manager.StoreResult(result,
                                       CKAS_EXECUTIONFAILED,
                                       rollbackCode,
                                       fmt::format("Failed to bind imported function; rollback also failed: {}",
                                                   rollbackError));
        }
        manager.m_ModuleStateStore.RemoveImportBinding(request.ImportModuleName, request.ImportIndex);
        manager.m_ModuleStateStore.BumpGeneration(request.ImportModuleName);
        return manager.StoreResult(result, status, bindResult, "Failed to bind imported function.");
    }
    manager.m_ModuleStateStore.RecordImportBinding(request.ImportModuleName,
                                                   request.ImportIndex,
                                                   sourceModuleName.c_str(),
                                                   functionDecl.c_str());
    manager.m_ModuleStateStore.BumpGeneration(request.ImportModuleName);
    return manager.StoreResult(result, CKAS_OK, bindResult);
}

CKAS_STATUS ScriptImportBinder::BindAllImportedFunctions(ScriptManager &manager,
                                                         const char *moduleName,
                                                         CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus = manager.CheckModuleMutationAllowed("BindAllImportedFunctions", result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = manager.BorrowModule(moduleName, &module, result);
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
            return manager.StoreResult(result,
                                       CKAS_INVALIDARGUMENT,
                                       0,
                                       fmt::format("Import {} is missing a source module or declaration.", i));
        }
        asIScriptModule *sourceModule = manager.GetModule(sourceModuleName.c_str());
        if (!sourceModule) {
            return manager.StoreResult(result,
                                       CKAS_NOTFOUND,
                                       0,
                                       fmt::format("Import {} source module '{}' was not found.",
                                                   i,
                                                   sourceModuleName));
        }
        asIScriptFunction *targetFunction = sourceModule->GetFunctionByDecl(functionDecl.c_str());
        if (!targetFunction) {
            return manager.StoreResult(result,
                                       CKAS_NOTFOUND,
                                       0,
                                       fmt::format("Import {} target function '{}' was not found in module '{}'.",
                                                   i,
                                                   functionDecl,
                                                   sourceModuleName));
        }
        const asEFuncType functionType = targetFunction->GetFuncType();
        if (functionType != asFUNC_SCRIPT && functionType != asFUNC_SYSTEM) {
            return manager.StoreResult(result,
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
        manager.m_ModuleStateStore.GetImportBindingsForModule(moduleName);
    for (const ResolvedImportBinding &binding : resolvedBindings) {
        manager.m_ModuleStateStore.RemoveImportBinding(moduleName, binding.Index);
        const int bindResult = module->BindImportedFunction(binding.Index, binding.TargetFunction);
        if (bindResult < 0) {
            const CKAS_STATUS status = ScriptApiSupport::StatusFromImportBindResult(bindResult);
            module->UnbindAllImportedFunctions();
            manager.m_ModuleStateStore.RemoveImportBindingsForModule(moduleName);

            int rollbackCode = 0;
            std::string rollbackError;
            const bool restored = Rebind(manager,
                                         previousBindings,
                                         rollbackCode,
                                         rollbackError);
            if (!restored) {
                manager.m_ModuleStateStore.BumpGeneration(moduleName);
                return manager.StoreResult(result,
                                           CKAS_EXECUTIONFAILED,
                                           rollbackCode,
                                           fmt::format("Failed to bind import {}; rollback also failed: {}",
                                                       binding.Index,
                                                       rollbackError));
            }
            manager.m_ModuleStateStore.RestoreImportBindingsForModule(moduleName, previousBindings);
            return manager.StoreResult(result,
                                       status,
                                       bindResult,
                                       fmt::format("Failed to bind import {}.", binding.Index));
        }
        manager.m_ModuleStateStore.RecordImportBinding(moduleName,
                                                       binding.Index,
                                                       binding.SourceModuleName.c_str(),
                                                       binding.FunctionDecl.c_str());
    }
    if (!resolvedBindings.empty()) {
        manager.m_ModuleStateStore.BumpGeneration(moduleName);
    }
    return manager.StoreResult(result, CKAS_OK);
}

CKAS_STATUS ScriptImportBinder::UnbindImportedFunction(ScriptManager &manager,
                                                       const char *moduleName,
                                                       CKDWORD importIndex,
                                                       CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus = manager.CheckModuleMutationAllowed("UnbindImportedFunction", result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    if (importIndex >= module->GetImportedFunctionCount()) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Import index is out of range.");
    }
    const int unbindResult = module->UnbindImportedFunction(importIndex);
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return manager.StoreResult(result, status, unbindResult, "Failed to unbind imported function.");
    }
    manager.m_ModuleStateStore.RemoveImportBinding(moduleName, importIndex);
    manager.m_ModuleStateStore.BumpGeneration(moduleName);
    return manager.StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptImportBinder::UnbindAllImportedFunctions(ScriptManager &manager,
                                                           const char *moduleName,
                                                           CKAngelScriptResult *result) {
    CKAS_STATUS callbackStatus = manager.CheckModuleMutationAllowed("UnbindAllImportedFunctions", result);
    if (callbackStatus != CKAS_OK) {
        return callbackStatus;
    }
    asIScriptModule *module = nullptr;
    CKAS_STATUS status = manager.BorrowModule(moduleName, &module, result);
    if (status != CKAS_OK) {
        return status;
    }
    const int unbindResult = module->UnbindAllImportedFunctions();
    if (unbindResult < 0) {
        status = ScriptApiSupport::StatusFromImportBindResult(unbindResult);
        return manager.StoreResult(result, status, unbindResult, "Failed to unbind imported functions.");
    }
    manager.m_ModuleStateStore.RemoveImportBindingsForModule(moduleName);
    if (module->GetImportedFunctionCount() > 0) {
        manager.m_ModuleStateStore.BumpGeneration(moduleName);
    }
    return manager.StoreResult(result, CKAS_OK, unbindResult);
}

CKAS_STATUS ScriptImportBinder::EnumerateBoundImportEdges(ScriptManager &manager,
                                                          const char *moduleName,
                                                          CKAngelScriptBoundImportEdgeCallback callback,
                                                          void *userData,
                                                          CKAngelScriptResult *result) {
    if (!callback) {
        return manager.StoreResult(result, CKAS_INVALIDARGUMENT, 0, "Bound import edge callback is required.");
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS borrowStatus = manager.BorrowModule(moduleName, &module, result);
    if (borrowStatus != CKAS_OK) {
        return borrowStatus;
    }
    (void)module;

    const std::vector<ScriptImportBindingEdge> edges =
        manager.m_ModuleStateStore.GetImportBindingsForModule(moduleName);
    for (const ScriptImportBindingEdge &edge : edges) {
        CKAngelScriptBoundImportEdge publicEdge = {};
        publicEdge.Size = sizeof(publicEdge);
        publicEdge.ImportModuleName = edge.ImportModuleName.c_str();
        publicEdge.ImportIndex = edge.ImportIndex;
        publicEdge.SourceModuleName = edge.SourceModuleName.c_str();
        publicEdge.FunctionDecl = edge.FunctionDecl.c_str();
        CKAS_STATUS callbackStatus = CKAS_OK;
        {
            ScriptApiSupport::CallbackDepthScope callbackScope(manager.m_PublicCallbackDepth);
            callbackStatus = ScriptApiSupport::DispatchBoundImportEdge(publicEdge, callback, userData);
        }
        if (callbackStatus != CKAS_OK) {
            return manager.StoreResult(result,
                                       callbackStatus,
                                       0,
                                       "Bound import edge enumeration stopped by callback.");
        }
    }
    return manager.StoreResult(result, CKAS_OK);
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
