//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Runner
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include "ScriptManager.h"

typedef int (*CKBehaviorCallback)(const CKBehaviorContext *behcontext, void *arg);

CKObjectDeclaration *FillBehaviorAngelScriptRunnerDecl();
CKERROR CreateAngelScriptRunnerProto(CKBehaviorPrototype **pproto);
int AngelScriptRunner(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAngelScriptRunnerDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScriptRunner");
    od->SetDescription("Run angel script");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x599924bb,0x12ec072a));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptRunnerProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptRunnerProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScriptRunner");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("Success");
    proto->DeclareOutput("Failed");

    proto->DeclareInParameter("Name", CKPGUID_STRING);

    proto->DeclareOutParameter("Result", CKPGUID_INT);

    proto->DeclareLocalParameter("Data", CKPGUID_VOIDBUF);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptRunner);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptRunner(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;

    ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
    if (!man)
        return CKBR_OWNERERROR;

    XString name = (CKSTRING)beh->GetInputParameterReadDataPtr(0);
    if (name.Empty()) {
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    return CKBR_OK;
}