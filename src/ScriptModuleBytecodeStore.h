#ifndef CK_SCRIPT_MODULE_BYTECODE_STORE_H
#define CK_SCRIPT_MODULE_BYTECODE_STORE_H

#include "CKAngelScript.h"

class ScriptApiDiagnostics;
class ScriptHandleRegistry;
class ScriptManager;
class ScriptModuleRegistry;
class ScriptModuleStateStore;

class ScriptModuleBytecodeStore {
public:
    struct SaveContext {
        ScriptManager &Manager;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
        int &BytecodeCallbackDepth;
    };

    struct LoadContext {
        ScriptManager &Manager;
        ScriptModuleRegistry &ModuleRegistry;
        ScriptModuleStateStore &ModuleStateStore;
        ScriptHandleRegistry &HandleRegistry;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
        int &BytecodeCallbackDepth;
    };

    CKAS_STATUS Save(SaveContext &context,
                     const CKAngelScriptBytecodeSaveOptions &options,
                     CKAngelScriptResult *result);
    CKAS_STATUS Load(LoadContext &context,
                     const CKAngelScriptBytecodeLoadOptions &options,
                     CKAngelScriptResult *result);
};

#endif // CK_SCRIPT_MODULE_BYTECODE_STORE_H
