#ifndef CK_SCRIPT_API_HANDLES_H
#define CK_SCRIPT_API_HANDLES_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAll.h"
#include "CKAngelScript.h"
#include "ScriptInvoker.h"

class ScriptManager;

struct CKAngelScriptExecution {
    explicit CKAngelScriptExecution(ScriptManager *manager)
        : Manager(manager), Invoker(manager) {}

    ~CKAngelScriptExecution() {
        if (Function) {
            Function->Release();
            Function = nullptr;
        }
    }

    ScriptManager *Manager = nullptr;
    ScriptInvoker Invoker;
    asIScriptFunction *Function = nullptr;
    CKAS_EXECUTIONSTATE State = CKAS_EXECUTION_READY;
    CKAngelScriptResult Result = {sizeof(CKAngelScriptResult), CKAS_OK, 0, nullptr, nullptr, nullptr, 0};
    std::string ErrorMessage;
    std::string StackTrace;
    std::string ModuleName;
    std::string FunctionName;
    std::string FunctionDecl;
    CKDWORD ModuleGeneration = 0;
    CKBehaviorContext BehaviorContextStorage;
    bool HasBehaviorContext = false;
    CKDWORD Flags = CKAS_CALL_DEFAULT;
};

struct CKAngelScriptFunction {
    ScriptManager *Manager = nullptr;
    std::string ModuleName;
    std::string FunctionName;
    std::string FunctionDecl;
    CKDWORD ModuleGeneration = 0;
};

struct CKAngelScriptObject {
    ScriptManager *Manager = nullptr;
    asIScriptObject *Object = nullptr;
    std::string ModuleName;
    std::string ClassName;
    std::string ClassNamespace;
    CKDWORD ModuleGeneration = 0;
};

struct CKAngelScriptMethod {
    ScriptManager *Manager = nullptr;
    std::string ModuleName;
    std::string ClassName;
    std::string ClassNamespace;
    std::string MethodName;
    std::string MethodDecl;
    CKDWORD ModuleGeneration = 0;
    std::vector<int> ParamTypes;
    std::vector<asDWORD> ParamFlags;
    int ReturnType = asTYPEID_VOID;
    asDWORD ReturnFlags = 0;
};

struct CKAngelScriptStringArg {
    CKDWORD Index = 0;
    std::string Value;
};

struct CKAngelScriptArgWriter {
    asIScriptContext *Context = nullptr;
    const CKAngelScriptMethod *Method = nullptr;
    std::vector<CKAngelScriptStringArg> StringArgs;
};

struct CKAngelScriptResultReader {
    asIScriptContext *Context = nullptr;
    const CKAngelScriptMethod *Method = nullptr;
};

#endif // CK_SCRIPT_API_HANDLES_H
