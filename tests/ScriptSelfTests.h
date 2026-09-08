#ifndef CK_SCRIPTSELFTESTS_H
#define CK_SCRIPTSELFTESTS_H

#include <string>

#include "CKDefines.h"
#include "ScriptCoreSelfTests.h"

#ifndef CKAS_ENABLE_API_EXPORT
#define CKAS_ENABLE_API_EXPORT 0
#endif

#ifndef CKAS_ENABLE_DYNCALL
#define CKAS_ENABLE_DYNCALL 0
#endif

class CKContext;
class ScriptManager;
class asIScriptEngine;

CKERROR RunScriptStartupSelfTests(ScriptManager *manager);
void RecordScriptManagerLifecycleSelfTestEvent(const char *event);
void PrepareScriptManagerLifecycleEndSelfTest(ScriptManager *manager);
CKERROR CompleteScriptManagerLifecycleSelfTest(ScriptManager *manager);

bool RunScriptComponentMetadataSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptInvocationSelfTest(ScriptManager *manager, std::string &error);
bool RunScriptParameterConversionSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptParameterRegistrySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptApiSelfTest(CKContext *context, std::string &error);
#if CKAS_ENABLE_API_EXPORT
bool RunScriptInfoExportSelfTest(asIScriptEngine *engine, std::string &error);
#endif
bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptAsyncSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptExecutorSelfTest(CKContext *context,
                               asIScriptEngine *engine,
                               std::string &error,
                               std::string *pauseResumeTrace = nullptr);
bool RunScriptSceneSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptNativeMemorySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
bool RunScriptVxBindingSelfTest(asIScriptEngine *engine, std::string &error);
#if CKAS_ENABLE_DYNCALL
bool RunScriptDynLoadSelfTest(asIScriptEngine *engine, std::string &error);
#endif

namespace ScriptMessageSelfTest {
bool RunScriptMessageSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error);
}

#endif // CK_SCRIPTSELFTESTS_H
