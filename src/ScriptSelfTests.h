#ifndef CK_SCRIPTSELFTESTS_H
#define CK_SCRIPTSELFTESTS_H

#include <string>

#include "CKDefines.h"

#ifndef CKAS_ENABLE_API_EXPORT
#define CKAS_ENABLE_API_EXPORT 0
#endif

class CKContext;
class ScriptManager;
class asIScriptEngine;

CKERROR RunScriptStartupSelfTests(ScriptManager *manager);

bool RunScriptComponentMetadataSelfTest(std::string &error);
bool RunScriptParameterConversionSelfTest(std::string &error);
bool RunScriptParameterRegistrySelfTest(CKContext *context, std::string &error);
bool RunScriptApiSelfTest(CKContext *context, std::string &error);
#if CKAS_ENABLE_API_EXPORT
bool RunScriptInfoExportSelfTest(std::string &error);
#endif
bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptAsyncSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptSceneSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptNativeMemorySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptVxBindingSelfTest(std::string &error);

namespace ScriptRuntimeMetadata {
bool RunScriptRuntimeMetadataSelfTest(std::string &error);
}

namespace ScriptRuntimeDependencyResolver {
bool RunScriptRuntimeDependencySelfTest(std::string &error);
}

namespace ScriptMessageSelfTest {
bool RunScriptMessageSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
}

#endif // CK_SCRIPTSELFTESTS_H
