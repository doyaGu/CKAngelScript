#ifndef CK_ANGELSCRIPTEXECUTOR_H
#define CK_ANGELSCRIPTEXECUTOR_H

#include "CKDefines.h"

class CKBehaviorPrototype;
class CKObjectDeclaration;
struct CKBehaviorContext;

CKObjectDeclaration *FillBehaviorAngelScriptExecutorDecl();
CKERROR CreateAngelScriptExecutorProto(CKBehaviorPrototype **pproto);
int AngelScriptExecutor(const CKBehaviorContext &behcontext);
CKERROR AngelScriptExecutorCallback(const CKBehaviorContext &behcontext);

#if CKAS_BUILD_SELF_TESTS
void AngelScriptExecutorFailNextPhysicalInterfaceSyncForSelfTest();
#endif

#endif // CK_ANGELSCRIPTEXECUTOR_H
