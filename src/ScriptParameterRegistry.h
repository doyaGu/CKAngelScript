#ifndef CK_SCRIPTPARAMETERREGISTRY_H
#define CK_SCRIPTPARAMETERREGISTRY_H

#include <string>

#include <angelscript.h>

class CKContext;

void RegisterScriptParameterRegistry(asIScriptEngine *engine);
bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error);

#endif // CK_SCRIPTPARAMETERREGISTRY_H
