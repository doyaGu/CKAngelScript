#ifndef CK_SCRIPTDYNCALL_H
#define CK_SCRIPTDYNCALL_H

#include <angelscript.h>

void RegisterScriptDynCall(asIScriptEngine *engine);
void RegisterScriptDynCallback(asIScriptEngine *engine);
void RegisterScriptDynLoad(asIScriptEngine *engine);

#endif // CK_SCRIPTDYNCALL_H
