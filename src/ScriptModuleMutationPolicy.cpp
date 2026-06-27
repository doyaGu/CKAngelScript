#include "ScriptModuleMutationPolicy.h"

#include <string>

#include <fmt/format.h>

#include "ScriptApiDiagnostics.h"
#include "ScriptAsync.h"
#include "ScriptHandleRegistry.h"
#include "ScriptModuleStateStore.h"

CKAS_STATUS ScriptModuleMutationPolicy::CheckRuntimeHandlesReleased(
    const ScriptHandleRegistry &handles,
    const ScriptAsyncScheduler *asyncScheduler,
    ScriptApiDiagnostics &diagnostics,
    const char *moduleName,
    CKAngelScriptResult *result) {
    if (handles.HasRuntimeHandleForModule(moduleName)) {
        return diagnostics.StoreResult(result,
                                       CKAS_INUSE,
                                       0,
                                       "Module has live object or execution handles.");
    }
    if (asyncScheduler && asyncScheduler->HasTaskForModule(moduleName)) {
        return diagnostics.StoreResult(result,
                                       CKAS_INUSE,
                                       0,
                                       "Module has live async tasks.");
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptModuleMutationPolicy::CheckNoBoundImportConsumers(
    const ScriptModuleStateStore &stateStore,
    ScriptApiDiagnostics &diagnostics,
    const char *moduleName,
    CKAngelScriptResult *result) {
    std::string importConsumer;
    if (stateStore.HasBoundImportConsumersForModule(moduleName, &importConsumer)) {
        return diagnostics.StoreResult(result,
                                       CKAS_INUSE,
                                       0,
                                       fmt::format("Module is imported by bound module '{}'.",
                                                   importConsumer));
    }
    return CKAS_OK;
}

CKAS_STATUS ScriptModuleMutationPolicy::CheckReplaceOrUnloadAllowed(
    const ScriptHandleRegistry &handles,
    const ScriptAsyncScheduler *asyncScheduler,
    const ScriptModuleStateStore &stateStore,
    ScriptApiDiagnostics &diagnostics,
    const char *moduleName,
    CKAngelScriptResult *result) {
    const CKAS_STATUS runtimeStatus =
        CheckRuntimeHandlesReleased(handles, asyncScheduler, diagnostics, moduleName, result);
    if (runtimeStatus != CKAS_OK) {
        return runtimeStatus;
    }
    return CheckNoBoundImportConsumers(stateStore, diagnostics, moduleName, result);
}

CKAS_STATUS ScriptModuleMutationPolicy::CheckMutationAllowed(ScriptApiDiagnostics &diagnostics,
                                                            int publicCallbackDepth,
                                                            const char *apiName,
                                                            CKAngelScriptResult *result) {
    if (publicCallbackDepth <= 0) {
        return CKAS_OK;
    }
    return diagnostics.StoreResult(result,
                                   CKAS_INVALIDSTATE,
                                   0,
                                   fmt::format("{} cannot mutate modules while a CKAngelScript callback is active.",
                                               apiName ? apiName : "CKAngelScript"));
}
