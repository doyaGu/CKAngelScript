//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Loader
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include "ScriptManager.h"
#include "ScriptRunner.h"

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

    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareLocalParameter(nullptr, CKPGUID_STATECHUNK);
    proto->DeclareLocalParameter(nullptr, CKPGUID_STRING);

    proto->DeclareSetting("Use File List", CKPGUID_BOOL, "FALSE");
    proto->DeclareSetting("Filename As Code", CKPGUID_BOOL, "FALSE");
    proto->DeclareSetting("No Script Cache", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptLoader);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_MESSAGESENDER |
                                                 CKBEHAVIOR_MESSAGERECEIVER |
                                                 CKBEHAVIOR_TARGETABLE |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptLoaderCallBack);

    *pproto = proto;
    return CK_OK;
}

#define USE_FILE_LIST 3
#define FILENAME_AS_CODE 4
#define NO_SCRIPT_CACHE 5

static void ActivateFailed(CKBehavior *beh,
                           CKContext *context,
                           const AngelScriptResult *result = nullptr,
                           const char *fallback = nullptr) {
    if (result && result->ErrorMessage && result->ErrorMessage[0] != '\0' && context) {
        context->OutputToConsoleEx(const_cast<char *>("[AngelScript Loader] %s"), result->ErrorMessage);
    } else if (fallback && fallback[0] != '\0' && context) {
        context->OutputToConsoleEx(const_cast<char *>("[AngelScript Loader] %s"), fallback);
    }
    if (beh) {
        beh->ActivateOutput(2);
        beh->SetOutputParameterValue(0, nullptr);
    }
}

static bool TriggerCallback(ScriptRunner *runner, const char *name, const CKBehaviorContext &behcontext) {
    if (!runner || !runner->IsAttached())
        return false;

    asIScriptFunction *func = runner->GetFunctionByName(name);
    if (!func)
        return false;

    bool success = true;
    if (func->GetParamCount() > 0) {
        success = runner->ExecuteScript(
            func,
            [behcontext](asIScriptContext *ctx) {
                ctx->SetArgObject(0, (void *) &behcontext);
            });
    } else {
        success = runner->ExecuteScript(func);
    }

    return success;
}

static int OnLoadModule(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return CKBR_OWNERERROR;

    beh->ActivateInput(0, FALSE);

    CKSTRING name = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
    if (!name || name[0] == '\0') {
        beh->ActivateOutput(2);
        return CKBR_OK;
    }

    ScriptRunner *runner = nullptr;
    beh->GetLocalParameterValue(0, &runner);

    CKSTRING previous = (CKSTRING) beh->GetLocalParameterReadDataPtr(2);
    if (previous && previous[0] != '\0') {
        if (runner) {
            TriggerCallback(runner, "OnUnload", behcontext);
            runner->Detach(beh);
        }

        man->UnloadModule(previous, nullptr);
        beh->SetLocalParameterValue(2, "");
    }

    AngelScriptResult result = {};

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

            if (!man->HasModule(name)) {
                AngelScriptLoadOptions options = {};
                options.ModuleName = name;
                options.Filename = filename.CStr();
                if (man->LoadModule(options, &result) != ANGELSCRIPT_STATUS_OK) {
                    ActivateFailed(beh, context, &result, "Failed to load script file.");
                    return CKBR_OK;
                }
            }
        } else {
            CKSTRING code = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
            if (!code) {
                ActivateFailed(beh, context, nullptr, "Script code is empty.");
                return CKBR_OK;
            }

            if (!man->HasModule(name) &&
                man->CompileModule(name, code, false, &result) != ANGELSCRIPT_STATUS_OK) {
                ActivateFailed(beh, context, &result, "Failed to compile script code.");
                return CKBR_OK;
            }
        }
    } else {
        CKDataArray *da = (CKDataArray *) beh->GetInputParameterReadDataPtr(1);
        if (!da) {
            ActivateFailed(beh, context, nullptr, "Script file list is missing.");
            return CKBR_OK;
        }

        int column = 0;
        beh->GetInputParameterValue(2, &column);

        if (column < 0 || column >= da->GetColumnCount()) {
            ActivateFailed(beh, context, nullptr, "Script file list column is out of range.");
            return CKBR_OK;
        }

        int count = da->GetRowCount();
        if (count == 0) {
            ActivateFailed(beh, context, nullptr, "Script file list is empty.");
            return CKBR_OK;
        }

        std::vector<std::string> filenameStore;
        filenameStore.reserve(count);
        const char **filenames = new const char *[count];
        for (int i = 0; i < count; i++) {
            char filename[4096] = {};
            if (!da->GetElementStringValue(i, column, filename) || filename[0] == '\0') {
                delete[] filenames;
                ActivateFailed(beh, context, nullptr, "Script file list contains an empty filename.");
                return CKBR_OK;
            }
            filenameStore.emplace_back(filename);
            filenames[i] = filenameStore.back().c_str();
        }

        if (!man->HasModule(name)) {
            AngelScriptLoadOptions options = {};
            options.ModuleName = name;
            options.Filenames = filenames;
            options.FileCount = static_cast<size_t>(count);
            const AngelScriptStatus status = man->LoadModule(options, &result);
            delete[] filenames;
            if (status != ANGELSCRIPT_STATUS_OK) {
                ActivateFailed(beh, context, &result, "Failed to load script files.");
                return CKBR_OK;
            }
        } else {
            delete[] filenames;
        }
    }

    asIScriptModule *module = man->GetModule(name);
    if (!module) {
        ActivateFailed(beh, context, nullptr, "Script module was not loaded.");
        return CKBR_OK;
    }

    beh->SetLocalParameterValue(2, name, strlen(name) + 1);

    if (runner) {
        runner->Attach(beh);
        TriggerCallback(runner, "OnLoad", behcontext);
    }

    beh->SetOutputParameterValue(0, nullptr);
    beh->ActivateOutput(0);
    return CKBR_OK;
}

static int OnUnloadModule(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return CKBR_OWNERERROR;

    beh->ActivateInput(1, FALSE);

    CKSTRING name = (CKSTRING) beh->GetInputParameterReadDataPtr(0);
    if (!name || name[0] == '\0') {
        beh->ActivateOutput(2);
        return CKBR_OK;
    }

    ScriptRunner *runner = nullptr;
    beh->GetLocalParameterValue(0, &runner);

    asIScriptModule *module = man->GetModule(name);
    if (!module) {
        ActivateFailed(beh, context, nullptr, "Script module was not loaded.");
        return CKBR_OK;
    }

    if (runner) {
        TriggerCallback(runner, "OnUnload", behcontext);
        runner->Detach(beh);
    }

    man->UnloadModule(name, nullptr);
    beh->SetLocalParameterValue(2, "");

    beh->SetOutputParameterValue(0, nullptr);
    beh->ActivateOutput(1);
    return CKBR_OK;
}

static void ReadScriptData(CKBehavior *beh, std::shared_ptr<CachedScript> &script) {
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
                if (fseek(fp, 0, SEEK_END) == 0) {
                    long size = ftell(fp);
                    if (size >= 0 && fseek(fp, 0, SEEK_SET) == 0) {
                        code.resize(size);
                        size_t read = fread(code.data(), 1, size, fp);
                        if (read != static_cast<size_t>(size)) {
                            code.clear();
                        }
                    }
                }
                fclose(fp);
            }

            script->AddSection(filename, code);
        } else {
            CKSTRING code = (CKSTRING) beh->GetInputParameterReadDataPtr(1);
            script->sections.emplace_back(name, code ? code : "");
        }
    } else {
        CKDataArray *da = (CKDataArray *) beh->GetInputParameterReadDataPtr(1);
        if (!da) {
            return;
        }

        int column = 0;
        beh->GetInputParameterValue(2, &column);

        if (column < 0 || column >= da->GetColumnCount()) {
            return;
        }

        int count = da->GetRowCount();
        for (int i = 0; i < count; i++) {
            char filename[4096] = {};
            if (!da->GetElementStringValue(i, column, filename) || filename[0] == '\0') {
                continue;
            }

            std::string code;

            XString resolvedFilename = filename;
            man->ResolveScriptFileName(resolvedFilename);
            FILE *fp = fopen(resolvedFilename.CStr(), "rb");
            if (fp) {
                if (fseek(fp, 0, SEEK_END) == 0) {
                    long size = ftell(fp);
                    if (size >= 0 && fseek(fp, 0, SEEK_SET) == 0) {
                        code.resize(size);
                        size_t read = fread(code.data(), 1, size, fp);
                        if (read != static_cast<size_t>(size)) {
                            code.clear();
                        }
                    }
                }
                fclose(fp);
            }

            script->AddSection(filename, code);
        }
    }
}

int AngelScriptLoader(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;

    if (beh->IsInputActive(0)) {
        return OnLoadModule(behcontext);
    } else if (beh->IsInputActive(1)) {
        return OnUnloadModule(behcontext);
    }

    return CKBR_OK;
}

CKERROR AngelScriptLoaderCallBack(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh)
        return CKBR_PARAMETERERROR;

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man)
        return CKBR_OWNERERROR;

    ScriptRunner *runner = nullptr;

    switch (behcontext.CallbackMessage) {
        case CKM_BEHAVIORCREATE: {
            runner = new ScriptRunner(man);
            beh->SetLocalParameterValue(0, &runner);
        }
        break;
        case CKM_BEHAVIORLOAD: {
            runner = nullptr;
            beh->GetLocalParameterValue(0, &runner);
            if (!runner) {
                runner = new ScriptRunner(man);
                beh->SetLocalParameterValue(0, &runner);
            }

            CKBOOL noScriptCache = FALSE;
            beh->GetLocalParameterValue(NO_SCRIPT_CACHE, &noScriptCache);
            if (!noScriptCache) {
                const std::string scriptName = (CKSTRING) beh->GetLocalParameterReadDataPtr(2);
                if (!scriptName.empty()) {
                    CKStateChunk *chunk = nullptr;
                    beh->GetLocalParameterValue(1, &chunk);
                    if (chunk) {
                        man->RestoreCachedScriptFromChunk(scriptName.c_str(), chunk);
                    } else {
                        man->NewCachedScript(scriptName.c_str());
                    }
                }
            }
        }
        break;
        case CKM_BEHAVIORDELETE: {
            runner = nullptr;
            beh->GetLocalParameterValue(0, &runner);

            CKSTRING scriptName = (CKSTRING) beh->GetLocalParameterReadDataPtr(2);
            if (scriptName && scriptName[0] != '\0') {
                if (runner) {
                    TriggerCallback(runner, "OnUnload", behcontext);
                    runner->Detach(beh);
                }

                man->UnloadModule(scriptName, nullptr);
                beh->SetLocalParameterValue(2, "");
            }

            if (runner) {
                delete runner;
                runner = nullptr;
            }
            beh->SetLocalParameterValue(0, &runner);
        }
        break;
        case CKM_BEHAVIORPRESAVE: {
            CKBOOL noScriptCache = FALSE;
            beh->GetLocalParameterValue(NO_SCRIPT_CACHE, &noScriptCache);
            if (!noScriptCache) {
                const std::string scriptName = (CKSTRING) beh->GetLocalParameterReadDataPtr(2);
                if (!scriptName.empty()) {
                    auto script = man->GetCachedScript(scriptName.c_str());
                    if (script) {
                        ReadScriptData(beh, script);
                        CKStateChunk *chunk = nullptr;
                        beh->GetLocalParameterValue(1, &chunk);
                        if (chunk) {
                            man->SaveCachedScriptToChunk(scriptName.c_str(), chunk);
                        }
                    }
                }
            }
        }
        break;
        case CKM_BEHAVIORSETTINGSEDITED: {
            CKParameterIn *pin = beh->GetInputParameter(1);

            CKBOOL useFileList = FALSE;
            beh->GetLocalParameterValue(USE_FILE_LIST, &useFileList);
            if (useFileList) {
                if (pin) {
                    pin->SetGUID(CKPGUID_DATAARRAY, TRUE, "File List");
                }

                if (beh->GetInputParameterCount() < 3) {
                    beh->CreateInputParameter("List Column", CKPGUID_INT);
                }
            } else {
                if (beh->GetInputParameterCount() > 2) {
                    CKParameterIn *removed = beh->RemoveInputParameter(2);
                    if (removed) {
                        CKDestroyObject(removed);
                    }
                }
                CKBOOL filenameAsCode = FALSE;
                beh->GetLocalParameterValue(FILENAME_AS_CODE, &filenameAsCode);
                if (pin) {
                    if (filenameAsCode) {
                        pin->SetGUID(CKPGUID_STRING, TRUE, "Code");
                    } else {
                        pin->SetGUID(CKPGUID_STRING, TRUE, "Filename");
                    }
                }
            }

            CKBOOL noScriptCache = FALSE;
            beh->GetLocalParameterValue(NO_SCRIPT_CACHE, &noScriptCache);
            if (noScriptCache) {
                const std::string scriptName = (CKSTRING) beh->GetLocalParameterReadDataPtr(2);
                if (!scriptName.empty()) {
                    man->ClearCachedScriptCode(scriptName.c_str());
                }

                CKStateChunk *chunk = nullptr;
                beh->GetLocalParameterValue(1, &chunk);
                if (chunk) {
                    chunk->Clear();
                }
            }
        }
        break;
        case CKM_BEHAVIORPAUSE: {
            runner = nullptr;
            beh->GetLocalParameterValue(0, &runner);
            TriggerCallback(runner, "OnPause", behcontext);
        }
        break;
        case CKM_BEHAVIORRESUME: {
            runner = nullptr;
            beh->GetLocalParameterValue(0, &runner);
            TriggerCallback(runner, "OnResume", behcontext);
        }
        break;
        case CKM_BEHAVIORRESET: {
            runner = nullptr;
            beh->GetLocalParameterValue(0, &runner);
            TriggerCallback(runner, "OnReset", behcontext);
        }
        break;
        default:
            break;
    }

    return CKBR_OK;
}
