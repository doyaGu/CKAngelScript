//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Runner
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include "ScriptManager.h"
#include "ScriptRunner.h"

CKObjectDeclaration *FillBehaviorAngelScriptRunnerDecl();
CKERROR CreateAngelScriptRunnerProto(CKBehaviorPrototype **pproto);
int AngelScriptRunner(const CKBehaviorContext &behcontext);
CKERROR AngelScriptRunnerCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAngelScriptRunnerDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScript Runner");
    od->SetDescription("Run AngelScript code");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x53295cee,0x1a795bb8));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptRunnerProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptRunnerProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScript Runner");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("Out");
    proto->DeclareOutput("Error");

    proto->DeclareInParameter("Script", CKPGUID_STRING);
    proto->DeclareInParameter("Function", CKPGUID_STRING);

    proto->DeclareOutParameter("Error Message", CKPGUID_STRING);

    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptRunner);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_TARGETABLE |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS|
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptRunnerCallBack);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptRunner(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;

	int ret = CKBR_OK;
	bool success = true;

	ScriptRunner *runner = nullptr;
	beh->GetLocalParameterValue(0, &runner);
    if (runner) {
	    if (!runner->IsAttached()) {
		    runner->Attach(beh, true);
	    }

	    asIScriptFunction *func = nullptr;
	    beh->GetLocalParameterValue(1, &func);
	    if (func) {
		    if (func->GetParamCount() > 0) {
			    success = runner->ExecuteScript(
				    func,
				    [behcontext](asIScriptContext *ctx) {
					    ctx->SetArgObject(0, (void *) &behcontext);
				    },
				    [&ret](asIScriptContext *ctx) {
					    ret = static_cast<int>(ctx->GetReturnDWord());
				    });
		    } else {
			    success = runner->ExecuteScript(func);
		    }
	    }
    }

	beh->ActivateInput(0, FALSE);

	if (success) {
		beh->ActivateOutput(0);
	} else {
		beh->ActivateOutput(1);
		beh->SetOutputParameterValue(0, runner->GetErrorMessage().c_str());
	}

    return ret;
}

CKERROR AngelScriptRunnerCallBack(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
	CKContext *context = behcontext.Context;

    if (!beh)
        return CKBR_PARAMETERERROR;

	ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
	if (!man) {
		context->OutputToConsole("Can not get script manager");
		return CKERR_INVALIDPARAMETER;
	}

	ScriptRunner *runner = nullptr;

	switch (behcontext.CallbackMessage)
	{
	case CKM_BEHAVIORCREATE:
	case CKM_BEHAVIORLOAD: {
		runner = new ScriptRunner(man);
		beh->SetLocalParameterValue(0, &runner);
	}
		break;
	case CKM_BEHAVIORDELETE: {
		runner = nullptr;
		beh->GetLocalParameterValue(0, &runner);
		if (runner) {
			runner->Detach(beh, true);
			delete runner;
			runner = nullptr;
		}
		beh->SetLocalParameterValue(0, &runner);
	}
	break;
	case CKM_BEHAVIOREDITED: {
		runner = nullptr;
		beh->GetLocalParameterValue(0, &runner);
		if (runner) {
			runner->Detach(beh, true);
		}
	}
	break;
	case CKM_BEHAVIORRESET: {
		runner = nullptr;
		beh->GetLocalParameterValue(0, &runner);
		if (runner) {
			runner->Detach(beh, true);
			runner->Reset();
		}
	}
	break;
	case CKM_BEHAVIORDEACTIVATESCRIPT: {
		runner = nullptr;
		beh->GetLocalParameterValue(0, &runner);
		if (runner) {
			runner->Detach(beh, true);
		}
	}
	break;
	default:
		break;
	}

    return CKBR_OK;
}