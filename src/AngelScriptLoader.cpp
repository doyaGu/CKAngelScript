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
CKERROR AngelScriptLoaderCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAngelScriptLoaderDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScript Loader");
    od->SetDescription("Load AngelScript modules");
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
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScript Loader");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Load");
    proto->DeclareInput("Unload");

    proto->DeclareOutput("Loaded");
    proto->DeclareOutput("Unloaded");
    proto->DeclareOutput("Failed");

    proto->DeclareInParameter("Name", CKPGUID_STRING);
    proto->DeclareInParameter("Filename", CKPGUID_STRING);

    proto->DeclareLocalParameter(nullptr, CKPGUID_STATECHUNK);

    proto->DeclareSetting("Use File List", CKPGUID_BOOL, "FALSE");
    proto->DeclareSetting("Filename As Code", CKPGUID_BOOL, "FALSE");
    proto->DeclareSetting("Trigger Callbacks", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptLoader);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_TARGETABLE |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptLoaderCallBack);

    *pproto = proto;
    return CK_OK;
}

#define USE_FILE_LIST 1
#define FILENAME_AS_CODE 2
#define TRIGGER_CALLBACKS 3

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

    CKBOOL callback = TRUE;
    beh->GetLocalParameterValue(TRIGGER_CALLBACKS, &callback);

    if (beh->IsInputActive(0)) {
        // Load
        beh->ActivateInput(0, FALSE);

        int r = 0;

        CKBOOL useFileList = FALSE;
        beh->GetLocalParameterValue(USE_FILE_LIST, &useFileList);

        if (!useFileList) {
            CKBOOL filenameAsCode = FALSE;
            beh->GetLocalParameterValue(FILENAME_AS_CODE, &filenameAsCode);

            if (!filenameAsCode) {
                XString filename;

                CKSTRING path = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
                if (path && path[0] != '\0') {
                    filename = path;
                } else {
                    filename = name;
                }

                r = man->LoadScript(name, filename.CStr());
                if (r < 0) {
                    beh->ActivateOutput(2);
                    beh->SetOutputParameterValue(0, nullptr);
                    return CKBR_OK;
                }
            } else {
                CKSTRING code = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
                r = man->CompileScript(name, code);
                if (r < 0) {
                    beh->ActivateOutput(2);
                    beh->SetOutputParameterValue(0, nullptr);
                    return CKBR_OK;
                }
            }
        } else {
            CKDataArray *da = (CKDataArray *) beh->GetInputParameterReadDataPtr(1);
            if (!da) {
                beh->ActivateOutput(2);
                beh->SetOutputParameterValue(0, nullptr);
                return CKBR_OK;
            }

            int column = 0;
            beh->GetInputParameterValue(2, &column);

            int count = da->GetRowCount();
            if (count == 0) {
                beh->ActivateOutput(2);
                beh->SetOutputParameterValue(0, nullptr);
                return CKBR_OK;
            }

            const char **filenames = new const char *[count];
            for (int i = 0; i < count; i++) {
                filenames[i] = (CKSTRING) da->GetElement(i, column);
            }

            r = man->LoadScripts(name, filenames, count);
            delete[] filenames;

            if (r < 0) {
                beh->ActivateOutput(2);
                beh->SetOutputParameterValue(0, nullptr);
                return CKBR_OK;
            }
        }

        asIScriptModule *module = man->GetScript(name);
        if (!module) {
            beh->ActivateOutput(2);
            beh->SetOutputParameterValue(0, nullptr);
            return CKBR_OK;
        }

        if (callback) {
            asIScriptEngine *scriptEngine = man->GetScriptEngine();
            asIScriptContext *scriptContext = scriptEngine->RequestContext();

            asIScriptFunction *func = module->GetFunctionByDecl("void OnLoad()");
            if (func) {
                r = module->ResetGlobalVars();
                if (r < 0) {
                    scriptEngine->WriteMessage(name, 0, 0, asMSGTYPE_ERROR,
                                               "Failed while initializing global variables");
                } else {
                    r = scriptContext->Prepare(func);
                    if (r < 0) {
                        scriptEngine->WriteMessage(name, 0, 0, asMSGTYPE_ERROR,
                                                   "Failed while preparing the context for execution");
                    } else {
                        scriptContext->Execute();
                    }
                }
            }
        }

        beh->SetOutputParameterValue(0, nullptr);
        beh->ActivateOutput(0);
    } else if (beh->IsInputActive(1)) {
        // Unload
        beh->ActivateInput(1, FALSE);

        asIScriptModule *module = man->GetScript(name);
        if (!module) {
            beh->ActivateOutput(2);
            beh->SetOutputParameterValue(0, nullptr);
            return CKBR_OK;
        }

        if (callback) {
            asIScriptEngine *scriptEngine = man->GetScriptEngine();
            asIScriptContext *scriptContext = scriptEngine->RequestContext();

            asIScriptFunction *func = module->GetFunctionByDecl("void OnUnload()");
            if (func) {
                if (scriptContext->Prepare(func) < 0) {
                    scriptEngine->WriteMessage(name, 0, 0, asMSGTYPE_ERROR,
                                               "Failed while preparing the context for execution");
                } else {
                    scriptContext->Execute();
                }
            }
        }

        man->UnloadScript(name);

        beh->SetOutputParameterValue(0, nullptr);
        beh->ActivateOutput(1);
    }

    return CKBR_OK;
}

void ReadScriptData(CKBehavior *beh, std::shared_ptr<CachedScript> &script) {
    if (!beh)
        return;

    CKContext *context = beh->GetCKContext();
    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return;

    CKSTRING name = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
    if (!name || name[0] == '\0') {
        return;
    }

    script->name = name;

    CKBOOL useFileList = FALSE;
    beh->GetLocalParameterValue(USE_FILE_LIST, &useFileList);

    if (!useFileList) {
        CKBOOL filenameAsCode = FALSE;
        beh->GetLocalParameterValue(FILENAME_AS_CODE, &filenameAsCode);

        if (!filenameAsCode) {
            std::string filename;
            CKSTRING path = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
            if (path && path[0] != '\0') {
                filename = path;
            } else {
                filename = name;
                filename += ".as";
            }

            std::string code;

            XString resolvedFilename = filename.c_str();
            man->ResolveScriptFileName(resolvedFilename);
            FILE *fp = fopen(resolvedFilename.CStr(), "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                size_t size = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                code.resize(size);
                fread(code.data(), 1, size, fp);
                fclose(fp);
            }

            script->AddSection(filename, code);
        } else {
            CKSTRING code = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
            script->sections.emplace_back(name, code);
        }
    } else {
        CKDataArray *da = (CKDataArray *) beh->GetInputParameterReadDataPtr(1);
        if (!da) {
            return;
        }

        int column = 0;
        beh->GetInputParameterValue(2, &column);

        int count = da->GetRowCount();
        for (int i = 0; i < count; i++) {
            auto filename = (CKSTRING) da->GetElement(i, column);

            std::string code;

            XString resolvedFilename = filename;
            man->ResolveScriptFileName(resolvedFilename);
            FILE *fp = fopen(filename, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                size_t size = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                code.resize(size);
                fread(code.data(), 1, size, fp);
                fclose(fp);
            }

            script->AddSection(filename, code);
        }
    }
}

CKERROR AngelScriptLoaderCallBack(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh)
        return CKBR_PARAMETERERROR;

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return CKBR_OWNERERROR;

    auto &cache = man->GetScriptCache();

    switch (behcontext.CallbackMessage) {
        case CKM_BEHAVIORLOAD: {
            const std::string scriptName = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
            auto script = cache.NewCachedScript(scriptName);
            if (!script->module) {
                CKStateChunk *chunk = nullptr;
                beh->GetLocalParameterValue(0, &chunk);
                if (chunk) {
                    script->LoadFromChunk(chunk);
                }
            }
        }
        break;
        case CKM_BEHAVIORDELETE: {
            const std::string scriptName = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
            cache.Invalidate(scriptName);
        }
        break;
        case CKM_BEHAVIORPRESAVE: {
            const std::string scriptName = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
            auto script = cache.GetCachedScript(scriptName);
            if (script) {
                ReadScriptData(beh, script);
                CKStateChunk *chunk = nullptr;
                beh->GetLocalParameterValue(0, &chunk);
                if (chunk) {
                    script->SaveToChunk(chunk);
                }
            }
        }
        break;
        case CKM_BEHAVIORSETTINGSEDITED: {
            CKParameterIn *pin = beh->GetInputParameter(1);

            CKBOOL useFileList = FALSE;
            beh->GetLocalParameterValue(USE_FILE_LIST, &useFileList);
            if (useFileList) {
                pin->SetGUID(CKPGUID_DATAARRAY, TRUE, "File List");

                beh->CreateInputParameter("List Column", CKPGUID_INT);
            } else {
                CKDestroyObject(beh->RemoveInputParameter(2));
                CKBOOL filenameAsCode = FALSE;
                beh->GetLocalParameterValue(FILENAME_AS_CODE, &filenameAsCode);
                if (filenameAsCode) {
                    pin->SetGUID(CKPGUID_STRING, TRUE, "Code");
                } else {
                    pin->SetGUID(CKPGUID_STRING, TRUE, "Filename");
                }
            }
        }
        break;
        default:
            break;
    }

    return CKBR_OK;
}
