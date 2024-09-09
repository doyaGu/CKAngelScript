//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Loader
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include "ScriptManager.h"

CKObjectDeclaration *FillBehaviorAngelScriptLoaderDecl();
CKERROR CreateAngelScriptLoaderProto(CKBehaviorPrototype **pproto);
int AngelScriptLoader(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAngelScriptLoaderDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScriptLoader");
    od->SetDescription("Load angel script");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x599924bb, 0x12ec072a));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptLoaderProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptLoaderProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScriptLoader");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Load");
    proto->DeclareInput("Unload");

    proto->DeclareOutput("Loaded");
    proto->DeclareOutput("Unloaded");
    proto->DeclareOutput("Failed");

    proto->DeclareInParameter("Name", CKPGUID_STRING);
    proto->DeclareInParameter("Path", CKPGUID_STRING);
    proto->DeclareInParameter("Callback", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptLoader);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptLoader(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return CKBR_OWNERERROR;

    CKSTRING name = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
    if (!name || name[0] == '\0') {
        beh->ActivateOutput(2);
        return CKBR_OK;
    }

    XString filename;

    CKSTRING path = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
    if (path && path[0] != '\0') {
        filename = path;
    } else {
        filename = name;
    }

    CKPathManager *pm = context->GetPathManager();
    XString category = "Script Paths";
    int catIdx = pm->GetCategoryIndex(category);
    pm->ResolveFileName(filename, catIdx);

    CKBOOL callback = TRUE;
    beh->GetInputParameterValue(2, &callback);

    if (beh->IsInputActive(0)) {
        // Load
        beh->ActivateInput(0, FALSE);

        int r = man->LoadScript(name, filename.CStr());
        if (r < 0) {
            beh->ActivateOutput(2);
            return CKBR_OK;
        }

        asIScriptModule *module = man->GetScript(filename.CStr());
        if (!module) {
            beh->ActivateOutput(2);
            return CKBR_OK;
        }

        if (callback) {
            asIScriptEngine *scriptEngine = man->GetScriptEngine();
            asIScriptContext *scriptContext = man->GetScriptContext();

            asIScriptFunction *func = module->GetFunctionByDecl("void OnLoad()");
            if (func) {
                r = module->ResetGlobalVars();
                if (r < 0) {
                    scriptEngine->WriteMessage(filename.CStr(), 0, 0, asMSGTYPE_ERROR,
                                               "Failed while initializing global variables");
                } else {
                    r = scriptContext->Prepare(func);
                    if (r < 0) {
                        scriptEngine->WriteMessage(filename.CStr(), 0, 0, asMSGTYPE_ERROR,
                                                   "Failed while preparing the context for execution");
                    } else {
                        scriptContext->Execute();
                    }
                }
            }
        }

        beh->ActivateOutput(0);
    } else if (beh->IsInputActive(1)) {
        // Unload
        beh->ActivateInput(1, FALSE);

        asIScriptModule *module = man->GetScript(filename.CStr());
        if (!module) {
            beh->ActivateOutput(2);
            return CKBR_OK;
        }

        if (callback) {
            asIScriptEngine *scriptEngine = man->GetScriptEngine();
            asIScriptContext *scriptContext = man->GetScriptContext();

            asIScriptFunction *func = module->GetFunctionByDecl("void OnUnload()");
            if (func) {
                if (scriptContext->Prepare(func) < 0) {
                    scriptEngine->WriteMessage(filename.CStr(), 0, 0, asMSGTYPE_ERROR,
                                               "Failed while preparing the context for execution");
                } else {
                    scriptContext->Execute();
                }
            }
        }

        module->Discard();

        beh->ActivateOutput(1);
    }

    return CKBR_OK;
}
