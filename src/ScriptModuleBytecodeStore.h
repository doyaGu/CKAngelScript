#ifndef CK_SCRIPT_MODULE_BYTECODE_STORE_H
#define CK_SCRIPT_MODULE_BYTECODE_STORE_H

#include "CKAngelScript.h"

class ScriptManager;

class ScriptModuleBytecodeStore {
public:
    CKAS_STATUS Save(ScriptManager &manager,
                     const CKAngelScriptBytecodeSaveOptions &options,
                     CKAngelScriptResult *result);
    CKAS_STATUS Load(ScriptManager &manager,
                     const CKAngelScriptBytecodeLoadOptions &options,
                     CKAngelScriptResult *result);
};

#endif // CK_SCRIPT_MODULE_BYTECODE_STORE_H
