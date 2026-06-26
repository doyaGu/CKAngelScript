#ifndef CK_SCRIPT_MODULE_BYTECODE_H
#define CK_SCRIPT_MODULE_BYTECODE_H

#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAngelScript.h"

namespace ScriptModuleBytecode {

bool SaveModuleByteCode(asIScriptModule *module,
                        std::vector<unsigned char> &byteCode,
                        int &angelScriptCode,
                        bool stripDebugInfo = false);
bool SaveModuleByteCode(asIScriptModule *module,
                        CKAngelScriptBytecodeWriteCallback callback,
                        void *userData,
                        bool stripDebugInfo,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus);
bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        std::vector<unsigned char> &byteCode,
                        asIScriptModule **outModule,
                        int &angelScriptCode);
bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        CKAngelScriptBytecodeReadCallback callback,
                        void *userData,
                        asIScriptModule **outModule,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus);
std::string MakeTransientModuleName(asIScriptEngine *engine, const char *moduleName);

} // namespace ScriptModuleBytecode

#endif // CK_SCRIPT_MODULE_BYTECODE_H
