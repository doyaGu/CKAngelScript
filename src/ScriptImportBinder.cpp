#include "ScriptImportBinder.h"

#include <fmt/format.h>

#include "ScriptManager.h"

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
