#ifndef CK_SCRIPTSELFTESTS_H
#define CK_SCRIPTSELFTESTS_H

#include <string>

#include "CKDefines.h"

class CKContext;
class ScriptManager;
class asIScriptEngine;

CKERROR RunScriptStartupSelfTests(ScriptManager *manager);

bool RunScriptComponentMetadataSelfTest(std::string &error);
bool RunScriptParameterConversionSelfTest(std::string &error);
bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error);
bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptAsyncSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);

namespace ScriptRuntimeMetadata {
bool RunScriptRuntimeMetadataSelfTest(std::string &error);
}

namespace ScriptRuntimeDependencyResolver {
bool RunScriptRuntimeDependencySelfTest(std::string &error);
}

#endif // CK_SCRIPTSELFTESTS_H
