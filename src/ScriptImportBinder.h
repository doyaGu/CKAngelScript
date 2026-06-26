#ifndef CK_SCRIPT_IMPORT_BINDER_H
#define CK_SCRIPT_IMPORT_BINDER_H

#include <string>
#include <vector>

#include "ScriptModuleStateStore.h"

class ScriptManager;

class ScriptImportBinder {
public:
    CKAS_STATUS GetImportedFunctionCount(ScriptManager &manager,
                                         const char *moduleName,
                                         CKDWORD *outCount,
                                         CKAngelScriptResult *result);
    CKAS_STATUS EnumerateImportedFunctions(ScriptManager &manager,
                                           const char *moduleName,
                                           CKAngelScriptImportCallback callback,
                                           void *userData,
                                           CKAngelScriptResult *result);
    CKAS_STATUS BindImportedFunction(ScriptManager &manager,
                                     const CKAngelScriptImportBindOptions &options,
                                     CKAngelScriptResult *result);
    CKAS_STATUS BindAllImportedFunctions(ScriptManager &manager,
                                         const char *moduleName,
                                         CKAngelScriptResult *result);
    CKAS_STATUS UnbindImportedFunction(ScriptManager &manager,
                                       const char *moduleName,
                                       CKDWORD importIndex,
                                       CKAngelScriptResult *result);
    CKAS_STATUS UnbindAllImportedFunctions(ScriptManager &manager,
                                           const char *moduleName,
                                           CKAngelScriptResult *result);
    CKAS_STATUS EnumerateBoundImportEdges(ScriptManager &manager,
                                          const char *moduleName,
                                          CKAngelScriptBoundImportEdgeCallback callback,
                                          void *userData,
                                          CKAngelScriptResult *result);

    bool Rebind(ScriptManager &manager,
                const std::vector<ScriptImportBindingEdge> &bindings,
                int &angelScriptCode,
                std::string &errorMessage);
};

#endif // CK_SCRIPT_IMPORT_BINDER_H
