#ifndef CK_ANGELSCRIPTEXECUTOR_H
#define CK_ANGELSCRIPTEXECUTOR_H

#include "CKDefines.h"

class CKBehaviorPrototype;
class CKObjectDeclaration;
class ScriptManager;
struct CKBehaviorContext;

CKObjectDeclaration *FillBehaviorAngelScriptExecutorDecl();
CKERROR CreateAngelScriptExecutorProto(CKBehaviorPrototype **pproto);
int AngelScriptExecutor(const CKBehaviorContext &behcontext);
CKERROR AngelScriptExecutorCallback(const CKBehaviorContext &behcontext);
void ReleaseAngelScriptExecutorStatesForManager(ScriptManager *manager);
void ReleaseAngelScriptExecutorStatesForModule(ScriptManager *manager, const char *moduleName);

#endif // CK_ANGELSCRIPTEXECUTOR_H
