#ifndef CK_ANGELSCRIPTEVENTHOOK_H
#define CK_ANGELSCRIPTEVENTHOOK_H

#include "CKDefines.h"

class CKBehaviorPrototype;
class CKObjectDeclaration;
struct CKBehaviorContext;

CKObjectDeclaration *FillBehaviorAngelScriptEventHookDecl();
CKERROR CreateAngelScriptEventHookProto(CKBehaviorPrototype **pproto);
int AngelScriptEventHook(const CKBehaviorContext &behcontext);

#endif // CK_ANGELSCRIPTEVENTHOOK_H
