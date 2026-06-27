#ifndef CK_SCRIPT_MODULE_MUTATION_POLICY_H
#define CK_SCRIPT_MODULE_MUTATION_POLICY_H

#include "CKAngelScript.h"

class ScriptApiDiagnostics;
class ScriptAsyncScheduler;
class ScriptHandleRegistry;
class ScriptModuleStateStore;

class ScriptModuleMutationPolicy {
public:
    static CKAS_STATUS CheckRuntimeHandlesReleased(const ScriptHandleRegistry &handles,
                                                   const ScriptAsyncScheduler *asyncScheduler,
                                                   ScriptApiDiagnostics &diagnostics,
                                                   const char *moduleName,
                                                   CKAngelScriptResult *result);
    static CKAS_STATUS CheckNoBoundImportConsumers(const ScriptModuleStateStore &stateStore,
                                                   ScriptApiDiagnostics &diagnostics,
                                                   const char *moduleName,
                                                   CKAngelScriptResult *result);
    static CKAS_STATUS CheckReplaceOrUnloadAllowed(const ScriptHandleRegistry &handles,
                                                   const ScriptAsyncScheduler *asyncScheduler,
                                                   const ScriptModuleStateStore &stateStore,
                                                   ScriptApiDiagnostics &diagnostics,
                                                   const char *moduleName,
                                                   CKAngelScriptResult *result);
    static CKAS_STATUS CheckMutationAllowed(ScriptApiDiagnostics &diagnostics,
                                            int publicCallbackDepth,
                                            const char *apiName,
                                            CKAngelScriptResult *result);
};

#endif // CK_SCRIPT_MODULE_MUTATION_POLICY_H
