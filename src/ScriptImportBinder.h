#ifndef CK_SCRIPT_IMPORT_BINDER_H
#define CK_SCRIPT_IMPORT_BINDER_H

#include <string>
#include <vector>

#include "ScriptModuleStateStore.h"

class ScriptApiDiagnostics;
class ScriptManager;

class ScriptImportBinder {
public:
    struct ReadContext {
        ScriptManager &Manager;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
    };

    struct BindContext {
        ScriptManager &Manager;
        ScriptModuleStateStore &StateStore;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
    };

    CKAS_STATUS GetImportedFunctionCount(ReadContext &context,
                                         const char *moduleName,
                                         CKDWORD *outCount,
                                         CKAngelScriptResult *result);
    CKAS_STATUS EnumerateImportedFunctions(ReadContext &context,
                                           const char *moduleName,
                                           CKAngelScriptImportCallback callback,
                                           void *userData,
                                           CKAngelScriptResult *result);
    CKAS_STATUS BindImportedFunction(BindContext &context,
                                     const CKAngelScriptImportBindOptions &options,
                                     CKAngelScriptResult *result);
    CKAS_STATUS BindAllImportedFunctions(BindContext &context,
                                         const char *moduleName,
                                         CKAngelScriptResult *result);
    CKAS_STATUS UnbindImportedFunction(BindContext &context,
                                       const char *moduleName,
                                       CKDWORD importIndex,
                                       CKAngelScriptResult *result);
    CKAS_STATUS UnbindAllImportedFunctions(BindContext &context,
                                           const char *moduleName,
                                           CKAngelScriptResult *result);
    CKAS_STATUS EnumerateBoundImportEdges(BindContext &context,
                                          const char *moduleName,
                                          CKAngelScriptBoundImportEdgeCallback callback,
                                          void *userData,
                                          CKAngelScriptResult *result);

    unsigned long long BuildDeclaredImportHash(ScriptManager &manager, const char *moduleName) const;

    bool Rebind(ScriptManager &manager,
                const std::vector<ScriptImportBindingEdge> &bindings,
                int &angelScriptCode,
                std::string &errorMessage);
};

#endif // CK_SCRIPT_IMPORT_BINDER_H
