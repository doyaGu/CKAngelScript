#ifndef CK_SCRIPT_MODULE_MUTATION_POLICY_H
#define CK_SCRIPT_MODULE_MUTATION_POLICY_H

#include "CKAngelScript.h"

class ScriptApiDiagnostics;
class ScriptHandleRegistry;
class ScriptModuleStateStore;

class ScriptModuleMutationPolicy {
public:
    CKAS_STATUS CheckRuntimeHandlesReleased(const ScriptHandleRegistry &handles,
                                            ScriptApiDiagnostics &diagnostics,
                                            const char *moduleName,
                                            CKAngelScriptResult *result) const;
    CKAS_STATUS CheckNoBoundImportConsumers(const ScriptModuleStateStore &stateStore,
                                            ScriptApiDiagnostics &diagnostics,
                                            const char *moduleName,
                                            CKAngelScriptResult *result) const;
    CKAS_STATUS CheckReplaceOrUnloadAllowed(const ScriptHandleRegistry &handles,
                                            const ScriptModuleStateStore &stateStore,
                                            ScriptApiDiagnostics &diagnostics,
                                            const char *moduleName,
                                            CKAngelScriptResult *result) const;
    CKAS_STATUS CheckMutationAllowed(ScriptApiDiagnostics &diagnostics,
                                     int publicCallbackDepth,
                                     const char *apiName,
                                     CKAngelScriptResult *result) const;
};

#endif // CK_SCRIPT_MODULE_MUTATION_POLICY_H
