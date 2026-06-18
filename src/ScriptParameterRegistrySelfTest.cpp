#include "ScriptSelfTests.h"

#include "angelscript.h"

#include "CKAttributeManager.h"
#include "CKParameterManager.h"
#include "ScriptParameterRegistry.h"

namespace {

bool ExecuteCKEnumStructProbe(asIScriptEngine *engine,
                              asIScriptFunction *function,
                              CKEnumStruct &input,
                              CKEnumStruct *other,
                              bool expectException,
                              const char *label,
                              std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0 && other) {
        r = scriptContext->SetArgObject(1, other);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKFlagsStructProbe(asIScriptEngine *engine,
                               asIScriptFunction *function,
                               CKFlagsStruct &input,
                               bool expectException,
                               const char *label,
                               std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKStructStructProbe(asIScriptEngine *engine,
                                asIScriptFunction *function,
                                CKStructStruct &input,
                                bool expectException,
                                const char *label,
                                std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &input);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKParameterTypeDescProbe(asIScriptEngine *engine,
                                     asIScriptFunction *function,
                                     CKContext *context,
                                     bool expectException,
                                     const char *label,
                                     std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, context);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool ExecuteCKAttributeDescProbe(asIScriptEngine *engine,
                                 asIScriptFunction *function,
                                 bool expectException,
                                 const char *label,
                                 std::string &error) {
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = std::string(label) + " could not create an execution context.";
        return false;
    }

    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (expectException) {
        ok = r == asEXECUTION_EXCEPTION;
        if (!ok) {
            error = std::string(label) + " expected a script exception, got code " + std::to_string(r) + ".";
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        ok = returnCode == 0;
        if (!ok) {
            error = std::string(label) + " returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string(label) + " exception: " + (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = std::string(label) + " failed with code " + std::to_string(r) + ".";
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    return ok;
}

bool RunCKEnumStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKEnumStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKEnumStructSelfTest";
    const char *source =
        "int ProbeEnumStruct(const CKEnumStruct &in input) {\n"
        "  if (input.GetNumEnums() != 2) return 1;\n"
        "  if (input.GetEnumValue(0) != 7 || input.GetEnumValue(1) != 9) return 2;\n"
        "  if (input.GetEnumDescription(0) != \"First\" || input.GetEnumDescription(1) != \"Second\") return 3;\n"
        "  CKEnumStruct copied(input);\n"
        "  if (copied.GetNumEnums() != 2 || copied.GetEnumValue(1) != 9) return 4;\n"
        "  CKEnumStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumEnums() != 2 || assigned.GetEnumDescription(0) != \"First\") return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeEnumStructOutOfRange(const CKEnumStruct &in input) {\n"
        "  return input.GetEnumValue(2);\n"
        "}\n"
        "\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKEnumStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-enum-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeEnumStruct(const CKEnumStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeEnumStructOutOfRange(const CKEnumStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKEnumStruct self-test functions were not found.";
        return false;
    }

    int values[2] = {7, 9};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("First"), const_cast<CKSTRING>("Second")};
    CKEnumStruct input;
    input.NbData = 2;
    input.Vals = values;
    input.Desc = descriptions;

    bool ok = ExecuteCKEnumStructProbe(engine, probe, input, nullptr, false, "CKEnumStruct copy probe", error) &&
              ExecuteCKEnumStructProbe(engine, outOfRange, input, nullptr, true, "CKEnumStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKFlagsStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKFlagsStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKFlagsStructSelfTest";
    const char *source =
        "int ProbeFlagsStruct(const CKFlagsStruct &in input) {\n"
        "  if (input.GetNumFlags() != 2) return 1;\n"
        "  if (input.GetFlagValue(0) != 1 || input.GetFlagValue(1) != 2) return 2;\n"
        "  if (input.GetFlagDescription(0) != \"Alpha\" || input.GetFlagDescription(1) != \"Beta\") return 3;\n"
        "  CKFlagsStruct copied(input);\n"
        "  if (copied.GetNumFlags() != 2 || copied.GetFlagValue(1) != 2) return 4;\n"
        "  CKFlagsStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumFlags() != 2 || assigned.GetFlagDescription(0) != \"Alpha\") return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeFlagsStructOutOfRange(const CKFlagsStruct &in input) {\n"
        "  return input.GetFlagValue(2);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKFlagsStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-flags-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeFlagsStruct(const CKFlagsStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeFlagsStructOutOfRange(const CKFlagsStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKFlagsStruct self-test functions were not found.";
        return false;
    }

    int values[2] = {1, 2};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("Alpha"), const_cast<CKSTRING>("Beta")};
    CKFlagsStruct input;
    input.NbData = 2;
    input.Vals = values;
    input.Desc = descriptions;

    bool ok = ExecuteCKFlagsStructProbe(engine, probe, input, false, "CKFlagsStruct copy probe", error) &&
              ExecuteCKFlagsStructProbe(engine, outOfRange, input, true, "CKFlagsStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKStructStructScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKStructStruct script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKStructStructSelfTest";
    const char *source =
        "bool SameGuid(const CKGUID &in lhs, const CKGUID &in rhs) {\n"
        "  return lhs.d1 == rhs.d1 && lhs.d2 == rhs.d2;\n"
        "}\n"
        "int ProbeStructStruct(const CKStructStruct &in input) {\n"
        "  if (input.GetNumSubParam() != 2) return 1;\n"
        "  CKGUID first(0x11111111, 0x22222222);\n"
        "  CKGUID second(0x33333333, 0x44444444);\n"
        "  if (!SameGuid(input.GetSubParamGuid(0), first) || !SameGuid(input.GetSubParamGuid(1), second)) return 2;\n"
        "  if (input.GetSubParamDescription(0) != \"First\" || input.GetSubParamDescription(1) != \"Second\") return 3;\n"
        "  CKStructStruct copied(input);\n"
        "  if (copied.GetNumSubParam() != 2 || !SameGuid(copied.GetSubParamGuid(1), second)) return 4;\n"
        "  CKStructStruct assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.GetNumSubParam() != 2 || assigned.GetSubParamDescription(0) != \"First\") return 5;\n"
        "  CKGUID guidValue = assigned.GetSubParamGuid(0);\n"
        "  guidValue.d1 = 0;\n"
        "  if (!SameGuid(assigned.GetSubParamGuid(0), first)) return 6;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeStructStructOutOfRange(const CKStructStruct &in input) {\n"
        "  CKGUID guid = input.GetSubParamGuid(2);\n"
        "  return int(guid.d1);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKStructStruct self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-struct-struct-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeStructStruct(const CKStructStruct &in)");
    asIScriptFunction *outOfRange = module->GetFunctionByDecl("int ProbeStructStructOutOfRange(const CKStructStruct &in)");
    if (!probe || !outOfRange) {
        engine->DiscardModule(moduleName);
        error = "CKStructStruct self-test functions were not found.";
        return false;
    }

    CKGUID guids[2] = {CKGUID(0x11111111, 0x22222222), CKGUID(0x33333333, 0x44444444)};
    CKSTRING descriptions[2] = {const_cast<CKSTRING>("First"), const_cast<CKSTRING>("Second")};
    CKStructStruct input;
    input.NbData = 2;
    input.Guids = guids;
    input.Desc = descriptions;

    bool ok = ExecuteCKStructStructProbe(engine, probe, input, false, "CKStructStruct copy probe", error) &&
              ExecuteCKStructStructProbe(engine, outOfRange, input, true, "CKStructStruct out-of-range probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGUIDScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKGUID script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *guidType = engine->GetTypeInfoByDecl("CKGUID");
    if (!guidType) {
        error = "CKGUID type is not registered.";
        return false;
    }
    if ((guidType->GetFlags() & asOBJ_APP_CLASS_ALLINTS) == 0) {
        error = "CKGUID type is missing asOBJ_APP_CLASS_ALLINTS.";
        return false;
    }
    if (!guidType->GetMethodByDecl("int opCmp(const CKGUID &in other) const")) {
        error = "CKGUID comparison method is not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKGUIDSelfTest";
    const char *source =
        "int ProbeCKGUID() {\n"
        "  CKGUID empty;\n"
        "  if (empty.IsValid()) return 1;\n"
        "  CKGUID direct(0x11111111, 0x22222222);\n"
        "  if (direct.d1 != 0x11111111 || direct.d2 != 0x22222222) return 2;\n"
        "  CKGUID listed = {0x33333333, 0x44444444};\n"
        "  if (listed.d1 != 0x33333333 || listed.d2 != 0x44444444) return 3;\n"
        "  listed.d1 = 0x55555555;\n"
        "  listed.d2 = 0x66666666;\n"
        "  if (listed.d1 != 0x55555555 || listed.d2 != 0x66666666) return 4;\n"
        "  CKGUID copied(listed);\n"
        "  if (copied.d1 != listed.d1 || copied.d2 != listed.d2) return 5;\n"
        "  CKGUID assigned;\n"
        "  assigned = direct;\n"
        "  if (assigned.d1 != direct.d1 || assigned.d2 != direct.d2) return 6;\n"
        "  if (direct.opCmp(assigned) != 0) return 7;\n"
        "  if (direct.opCmp(listed) >= 0) return 8;\n"
        "  if (!direct.IsValid()) return 9;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKGUID self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-guid-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKGUID()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKGUID self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKGUID value probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAttributeDesc script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeDescSelfTest";
    const char *source =
        "int ProbeAttributeDesc() {\n"
        "  CKAttributeDesc desc;\n"
        "  string longName = \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABCDEFGH\";\n"
        "  desc.Name = longName;\n"
        "  if (desc.Name.length() != 63) return 1;\n"
        "  if (desc.Name != longName.substr(0, 63)) return 2;\n"
        "  if (!desc.CallbackFct.IsNull() || !desc.CallbackArg.IsNull()) return 3;\n"
        "  if (!desc.DefaultValuePointer.IsNull() || desc.DefaultValue != \"\") return 4;\n"
        "  if (!desc.CreatorDll.IsNull()) return 5;\n"
        "  NativePointer empty;\n"
        "  desc.CallbackFct = empty;\n"
        "  desc.CallbackArg = empty;\n"
        "  desc.DefaultValuePointer = empty;\n"
        "  desc.CreatorDll = empty;\n"
        "  CKAttributeDesc copied(desc);\n"
        "  if (copied.Name != desc.Name || !copied.CallbackFct.IsNull()) return 6;\n"
        "  CKAttributeDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Name != desc.Name || !assigned.CreatorDll.IsNull()) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCallbackFctReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CallbackFct = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCallbackArgReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CallbackArg = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescDefaultValueReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.DefaultValuePointer = ptr;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeDescCreatorDllReject() {\n"
        "  CKAttributeDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.CreatorDll = ptr;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeDesc()");
    asIScriptFunction *callbackFctReject = module->GetFunctionByDecl("int ProbeAttributeDescCallbackFctReject()");
    asIScriptFunction *callbackArgReject = module->GetFunctionByDecl("int ProbeAttributeDescCallbackArgReject()");
    asIScriptFunction *defaultValueReject = module->GetFunctionByDecl("int ProbeAttributeDescDefaultValueReject()");
    asIScriptFunction *creatorDllReject = module->GetFunctionByDecl("int ProbeAttributeDescCreatorDllReject()");
    if (!probe || !callbackFctReject || !callbackArgReject || !defaultValueReject || !creatorDllReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKAttributeDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, callbackFctReject, true, "CKAttributeDesc CallbackFct rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, callbackArgReject, true, "CKAttributeDesc CallbackArg rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, defaultValueReject, true, "CKAttributeDesc DefaultValuePointer rejection probe", error) &&
              ExecuteCKAttributeDescProbe(engine, creatorDllReject, true, "CKAttributeDesc CreatorDll rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeCategoryDescScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAttributeCategoryDesc script self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeCategoryDescSelfTest";
    const char *source =
        "int ProbeAttributeCategoryDesc() {\n"
        "  CKAttributeCategoryDesc desc;\n"
        "  if (desc.Name != \"\" || !desc.NamePointer.IsNull() || desc.Flags != 0) return 1;\n"
        "  desc.Name = \"Gameplay\";\n"
        "  desc.Flags = 42;\n"
        "  if (desc.Name != \"Gameplay\" || desc.NamePointer.IsNull()) return 2;\n"
        "  CKAttributeCategoryDesc copied(desc);\n"
        "  if (copied.Name != \"Gameplay\" || copied.Flags != 42 || copied.NamePointer.IsNull()) return 3;\n"
        "  desc.Name = \"Changed\";\n"
        "  if (copied.Name != \"Gameplay\") return 4;\n"
        "  CKAttributeCategoryDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Name != \"Gameplay\" || assigned.Flags != 42) return 5;\n"
        "  copied.Name = \"Other\";\n"
        "  if (assigned.Name != \"Gameplay\") return 6;\n"
        "  NativePointer empty;\n"
        "  assigned.NamePointer = empty;\n"
        "  if (assigned.Name != \"\" || !assigned.NamePointer.IsNull()) return 7;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeCategoryDescNamePointerReject() {\n"
        "  CKAttributeCategoryDesc desc;\n"
        "  NativePointer ptr;\n"
        "  ptr += 1;\n"
        "  desc.NamePointer = ptr;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeCategoryDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-category-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeCategoryDesc()");
    asIScriptFunction *namePointerReject = module->GetFunctionByDecl("int ProbeAttributeCategoryDescNamePointerReject()");
    if (!probe || !namePointerReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeCategoryDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKAttributeCategoryDesc value probe", error) &&
              ExecuteCKAttributeDescProbe(engine, namePointerReject, true, "CKAttributeCategoryDesc NamePointer rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCK2dCurvePointScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CK2dCurvePoint script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *pointType = engine->GetTypeInfoByDecl("CK2dCurvePoint");
    if (!pointType) {
        error = "CK2dCurvePoint type is not registered.";
        return false;
    }
    if (!pointType->GetMethodByDecl("NativePointer GetCurve() const")) {
        error = "CK2dCurvePoint NativePointer GetCurve declaration is not registered.";
        return false;
    }
    if (pointType->GetMethodByDecl("CK2dCurve &GetCurve() const")) {
        error = "CK2dCurvePoint still exposes the old non-null GetCurve reference declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CK2dCurvePointSelfTest";
    const char *source =
        "int ProbeCurvePointDetachedOwner() {\n"
        "  CK2dCurvePoint point;\n"
        "  if (!point.GetCurve().IsNull()) return 1;\n"
        "  point.SetBias(0.25f);\n"
        "  point.SetTension(0.5f);\n"
        "  point.SetContinuity(0.75f);\n"
        "  point.SetLinear(true);\n"
        "  if (!point.IsLinear()) return 2;\n"
        "  point.SetLinear(false);\n"
        "  if (point.IsLinear()) return 3;\n"
        "  point.UseTCB(false);\n"
        "  if (point.IsTCB()) return 4;\n"
        "  point.UseTCB(true);\n"
        "  if (!point.IsTCB()) return 5;\n"
        "  point.SetPosition(Vx2DVector(1.0f, 2.0f));\n"
        "  point.SetInTangent(Vx2DVector(3.0f, 4.0f));\n"
        "  point.SetOutTangent(Vx2DVector(5.0f, 6.0f));\n"
        "  point.NotifyUpdate();\n"
        "  CK2dCurvePoint copied(point);\n"
        "  if (!copied.GetCurve().IsNull()) return 6;\n"
        "  CK2dCurvePoint assigned;\n"
        "  assigned = point;\n"
        "  if (!assigned.GetCurve().IsNull()) return 7;\n"
        "  if (assigned.GetBias() != point.GetBias()) return 8;\n"
        "  if (assigned.GetTension() != point.GetTension()) return 9;\n"
        "  if (assigned.GetContinuity() != point.GetContinuity()) return 10;\n"
        "  if (copied.GetPosition().x != 1.0f || copied.GetPosition().y != 2.0f) return 11;\n"
        "  if (assigned.GetInTangent().x != 3.0f || assigned.GetInTangent().y != 4.0f) return 12;\n"
        "  if (assigned.GetOutTangent().x != 5.0f || assigned.GetOutTangent().y != 6.0f) return 13;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CK2dCurvePoint self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-2d-curve-point-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCurvePointDetachedOwner()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CK2dCurvePoint self-test function was not found.";
        return false;
    }

    bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CK2dCurvePoint detached owner probe", error);
    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKDependenciesScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKDependencies script self-test requires an AngelScript engine.";
        return false;
    }

    if (!engine->GetGlobalFunctionByDecl("CKDependencies CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKDependencies value-returning CKGetDefaultClassDependencies declaration is not registered.";
        return false;
    }
    if (engine->GetGlobalFunctionByDecl("CKDependencies &CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKDependencies still exposes CKGetDefaultClassDependencies as a non-null reference.";
        return false;
    }
    if (!engine->GetGlobalFunctionByDecl("void CKCopyDefaultClassDependencies(CKDependencies &out d, CK_DEPENDENCIES_OPMODE mode)")) {
        error = "CKCopyDefaultClassDependencies declaration is not registered.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKDependenciesSelfTest";
    const char *source =
        "int ProbeDependenciesDefaultCopy() {\n"
        "  CKDependencies copied;\n"
        "  CKCopyDefaultClassDependencies(copied, CK_DEPENDENCIES_COPY);\n"
        "  if (copied.Size() < 0) return 1;\n"
        "  CKDependencies value = CKGetDefaultClassDependencies(CK_DEPENDENCIES_COPY);\n"
        "  if (value.Size() < 0) return 2;\n"
        "  CKDependencies unsupported = CKGetDefaultClassDependencies(CK_DEPENDENCIES_OPERATIONMODE);\n"
        "  if (unsupported.Size() < 0) return 3;\n"
        "  unsupported.ModifyOptions(CKCID_OBJECT, 0, 0);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDependencies self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-dependencies-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test script failed to build.";
        return false;
    }

    if (!module->GetFunctionByDecl("int ProbeDependenciesDefaultCopy()")) {
        engine->DiscardModule(moduleName);
        error = "CKDependencies self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKTimeProfilerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTimeProfiler script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTimeProfilerSelfTest";
    const char *source =
        "int ProbeCKTimeProfiler(CKContext@ ctx) {\n"
        "  CKTimeProfiler profiler(\"ckas-time-profiler\", ctx, -4);\n"
        "  string longName = \"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\";\n"
        "  profiler(longName);\n"
        "  profiler(\"second\");\n"
        "  string dump;\n"
        "  profiler.Dump(dump, \" / \");\n"
        "  if (dump.findFirst(\"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV\") < 0) return 1;\n"
        "  if (dump.findFirst(\"WXYZ0123456789\") >= 0) return 2;\n"
        "  if (dump.findFirst(\"second\") < 0) return 3;\n"
        "  if (dump.findFirst(\" / \") < 0) return 4;\n"
        "  profiler.Reset();\n"
        "  profiler.Dump(dump);\n"
        "  if (dump.length() != 0) return 5;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeCKTimeProfilerNullContext() {\n"
        "  CKTimeProfiler profiler(\"bad\", null);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTimeProfiler self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-time-profiler-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeCKTimeProfiler(CKContext@)");
    asIScriptFunction *nullContext = module->GetFunctionByDecl("int ProbeCKTimeProfilerNullContext()");
    if (!probe || !nullContext) {
        engine->DiscardModule(moduleName);
        error = "CKTimeProfiler self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKTimeProfiler probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, nullContext, true, "CKTimeProfiler null-context probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAttributeManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKAttributeManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAttributeManagerSelfTest";
    const char *source =
        "int ProbeAttributeManager(CKContext@ ctx) {\n"
        "  if (ctx is null) return 1;\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  if (am is null) return 2;\n"
        "  if (am.GetAttributeCount() < 0) return 3;\n"
        "  if (am.GetCategoriesCount() < 0) return 4;\n"
        "  am.GetName();\n"
        "  am.GetGuid();\n"
        "  am.IsAttributeIndexValid(-2147483647);\n"
        "  am.IsCategoryIndexValid(-2147483647);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeManagerCallbackReject(CKContext@ ctx) {\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  NativePointer ptr;\n"
        "  NativePointer empty;\n"
        "  ptr += 1;\n"
        "  am.SetAttributeCallbackFunction(-2147483647, ptr, empty);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeAttributeManagerCallbackArgReject(CKContext@ ctx) {\n"
        "  CKAttributeManager@ am = ctx.GetAttributeManager();\n"
        "  NativePointer ptr;\n"
        "  NativePointer empty;\n"
        "  ptr += 1;\n"
        "  am.SetAttributeCallbackFunction(-2147483647, empty, ptr);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAttributeManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-attribute-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeAttributeManager(CKContext@)");
    asIScriptFunction *callbackReject = module->GetFunctionByDecl("int ProbeAttributeManagerCallbackReject(CKContext@)");
    asIScriptFunction *callbackArgReject = module->GetFunctionByDecl("int ProbeAttributeManagerCallbackArgReject(CKContext@)");
    if (!probe || !callbackReject || !callbackArgReject) {
        engine->DiscardModule(moduleName);
        error = "CKAttributeManager self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKAttributeManager value probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, callbackReject, context, true, "CKAttributeManager callback rejection probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, callbackArgReject, context, true, "CKAttributeManager callback argument rejection probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBaseManagerCastScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBaseManager cast self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *baseManagerType = engine->GetTypeInfoByDecl("CKBaseManager");
    if (!baseManagerType) {
        error = "CKBaseManager self-test could not find the registered type.";
        return false;
    }
    if (baseManagerType->GetMethodByDecl("CKGUID GetGuid()") == nullptr ||
        baseManagerType->GetMethodByDecl("string GetName()") == nullptr ||
        baseManagerType->GetMethodByDecl("CKStateChunk@ SaveData(CKFile@ savedFile)") == nullptr ||
        baseManagerType->GetMethodByDecl("CKDWORD GetValidFunctionsMask()") == nullptr) {
        error = "CKBaseManager self-test could not find expected non-const manager methods.";
        return false;
    }
    if (baseManagerType->GetMethodByDecl("CKGUID GetGuid() const") != nullptr ||
        baseManagerType->GetMethodByDecl("string GetName() const") != nullptr ||
        baseManagerType->GetMethodByDecl("CKStateChunk@ SaveData(CKFile@ savedFile) const") != nullptr ||
        baseManagerType->GetMethodByDecl("CKDWORD GetValidFunctionsMask() const") != nullptr) {
        error = "CKBaseManager self-test found stale const manager method declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBaseManagerCastSelfTest";
    const char *source =
        "int ProbeBaseManagerCasts(CKContext@ ctx) {\n"
        "  if (ctx is null) return 1;\n"
        "  CKAttributeManager@ attr = ctx.GetAttributeManager();\n"
        "  CKTimeManager@ time = ctx.GetTimeManager();\n"
        "  if (attr is null || time is null) return 2;\n"
        "  CKBaseManager@ attrBase = attr;\n"
        "  CKBaseManager@ timeBase = time;\n"
        "  if (attrBase is null || timeBase is null) return 3;\n"
        "  CKAttributeManager@ attrAgain = cast<CKAttributeManager>(attrBase);\n"
        "  CKTimeManager@ timeAgain = cast<CKTimeManager>(timeBase);\n"
        "  if (attrAgain is null || timeAgain is null) return 4;\n"
        "  if (cast<CKTimeManager>(attrBase) !is null) return 5;\n"
        "  if (cast<CKAttributeManager>(timeBase) !is null) return 6;\n"
        "  if (cast<CKParameterManager>(attrBase) !is null) return 7;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBaseManager cast self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-base-manager-cast-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBaseManagerCasts(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBaseManager cast self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKBaseManager checked-cast probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTimeManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKTimeManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *timeManagerType = engine->GetTypeInfoByDecl("CKTimeManager");
    if (!timeManagerType) {
        error = "CKTimeManager self-test could not find the registered type.";
        return false;
    }
    if (timeManagerType->GetMethodByDecl("uint GetMainTickCount()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetLastDeltaTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetLastDeltaTimeFree()") == nullptr ||
        timeManagerType->GetMethodByDecl("float GetAbsoluteTime()") == nullptr ||
        timeManagerType->GetMethodByDecl("void GetTimeToWaitForLimits(float &out timeBeforeRender, float &out timeBeforeBeh)") == nullptr ||
        timeManagerType->GetMethodByDecl("void ResetChronos(bool renderChrono, bool behavioralChrono)") == nullptr) {
        error = "CKTimeManager self-test could not find expected time manager methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTimeManagerSelfTest";
    const char *source =
        "int ProbeTimeManager(CKContext@ ctx) {\n"
        "  CKTimeManager@ time = ctx.GetTimeManager();\n"
        "  if (time is null) return 1;\n"
        "  CKBaseManager@ base = time;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKTimeManager>(base) is null) return 3;\n"
        "  uint tick = time.GetMainTickCount();\n"
        "  float current = time.GetTime();\n"
        "  float absolute = time.GetAbsoluteTime();\n"
        "  float delta = time.GetLastDeltaTime();\n"
        "  float freeDelta = time.GetLastDeltaTimeFree();\n"
        "  float renderWait = 0.0f;\n"
        "  float behaviorWait = 0.0f;\n"
        "  time.GetTimeToWaitForLimits(renderWait, behaviorWait);\n"
        "  if (time.GetTimeScaleFactor() <= 0.0f) return 4;\n"
        "  if (time.GetMinimumDeltaTime() < 0.0f) return 5;\n"
        "  if (time.GetMaximumDeltaTime() < 0.0f) return 6;\n"
        "  if (time.GetFrameRateLimit() < 0.0f) return 7;\n"
        "  if (time.GetBehavioralRateLimit() < 0.0f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTimeManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-time-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTimeManager(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTimeManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKTimeManager accessors probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKBehaviorManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKBehaviorManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *behaviorManagerType = engine->GetTypeInfoByDecl("CKBehaviorManager");
    if (!behaviorManagerType) {
        error = "CKBehaviorManager self-test could not find the registered type.";
        return false;
    }
    if (behaviorManagerType->GetMethodByDecl("CKERROR Execute(float delta)") == nullptr ||
        behaviorManagerType->GetMethodByDecl("int GetObjectsCount()") == nullptr ||
        behaviorManagerType->GetMethodByDecl("CKBeObject@ GetObject(int pos)") == nullptr ||
        behaviorManagerType->GetMethodByDecl("int GetBehaviorMaxIteration()") == nullptr ||
        behaviorManagerType->GetMethodByDecl("void SetBehaviorMaxIteration(int n)") == nullptr) {
        error = "CKBehaviorManager self-test could not find expected behavior manager methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBehaviorManagerSelfTest";
    const char *source =
        "int ProbeBehaviorManager(CKContext@ ctx) {\n"
        "  CKBehaviorManager@ manager = ctx.GetBehaviorManager();\n"
        "  if (manager is null) return 1;\n"
        "  CKBaseManager@ base = manager;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKBehaviorManager>(base) is null) return 3;\n"
        "  int objectCount = manager.GetObjectsCount();\n"
        "  if (objectCount < 0) return 4;\n"
        "  int maxIteration = manager.GetBehaviorMaxIteration();\n"
        "  if (maxIteration < 0) return 5;\n"
        "  manager.SetBehaviorMaxIteration(maxIteration);\n"
        "  if (objectCount > 0) {\n"
        "    CKBeObject@ object = manager.GetObject(0);\n"
        "    if (object is null) return 6;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-behavior-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBehaviorManager(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKBehaviorManager accessors probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMessageManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKMessageManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *messageManagerType = engine->GetTypeInfoByDecl("CKMessageManager");
    if (!messageManagerType) {
        error = "CKMessageManager self-test could not find the registered type.";
        return false;
    }
    if (messageManagerType->GetMethodByDecl("CKMessageType AddMessageType(const string &in msgName)") == nullptr ||
        messageManagerType->GetMethodByDecl("string GetMessageTypeName(CKMessageType msgType)") == nullptr ||
        messageManagerType->GetMethodByDecl("int GetMessageTypeCount()") == nullptr ||
        messageManagerType->GetMethodByDecl("CKMessage@ SendMessageSingle(int MsgType, CKBeObject@ dest, CKBeObject@ sender = null)") == nullptr ||
        messageManagerType->GetMethodByDecl("CKERROR RegisterWait(CKMessageType msgType, CKBehavior@ beh, int outputToActivate, CKBeObject@ obj)") == nullptr ||
        messageManagerType->GetMethodByDecl("CKERROR UnRegisterWait(CKMessageType msgType, CKBehavior@ beh, int OutputToActivate)") == nullptr) {
        error = "CKMessageManager self-test could not find expected message manager methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMessageManagerSelfTest";
    const char *source =
        "int ProbeMessageManager(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  if (manager is null) return 1;\n"
        "  CKBaseManager@ base = manager;\n"
        "  if (base is null) return 2;\n"
        "  if (cast<CKMessageManager>(base) is null) return 3;\n"
        "  if (manager.GetMessageTypeCount() < 0) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullSingle(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.SendMessageSingle(0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullGroup(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.SendMessageGroup(0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullWait(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.RegisterWait(0, null, 0, null);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMessageManagerNullUnwait(CKContext@ ctx) {\n"
        "  CKMessageManager@ manager = ctx.GetMessageManager();\n"
        "  manager.UnRegisterWait(0, null, 0);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMessageManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-message-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMessageManager(CKContext@)");
    asIScriptFunction *nullSingle = module->GetFunctionByDecl("int ProbeMessageManagerNullSingle(CKContext@)");
    asIScriptFunction *nullGroup = module->GetFunctionByDecl("int ProbeMessageManagerNullGroup(CKContext@)");
    asIScriptFunction *nullWait = module->GetFunctionByDecl("int ProbeMessageManagerNullWait(CKContext@)");
    asIScriptFunction *nullUnwait = module->GetFunctionByDecl("int ProbeMessageManagerNullUnwait(CKContext@)");
    if (!probe || !nullSingle || !nullGroup || !nullWait || !nullUnwait) {
        engine->DiscardModule(moduleName);
        error = "CKMessageManager self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKMessageManager accessors probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullSingle, context, true, "CKMessageManager null single-message probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullGroup, context, true, "CKMessageManager null group-message probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullWait, context, true, "CKMessageManager null wait probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, nullUnwait, context, true, "CKMessageManager null unregister-wait probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKGridManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKGridManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *gridManagerType = engine->GetTypeInfoByDecl("CKGridManager");
    if (!gridManagerType) {
        error = "CKGridManager self-test could not find the registered type.";
        return false;
    }
    if (gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, int &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, float &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int layerType, const CKSquare &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, int &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, float &in fillVal)") == nullptr ||
        gridManagerType->GetMethodByDecl("void FillGridWithObjectShape(CK3dEntity@ ent, int solidLayerType, int shapeLayerType, const CKSquare &in fillVal)") == nullptr) {
        error = "CKGridManager self-test could not find expected typed FillGridWithObjectShape overloads.";
        return false;
    }

    return true;
}

bool RunCKFloorManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKFloorManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *floorManagerType = engine->GetTypeInfoByDecl("CKFloorManager");
    if (!floorManagerType) {
        error = "CKFloorManager self-test could not find the registered type.";
        return false;
    }
    if (floorManagerType->GetMethodByDecl("bool ReadAttributeValues(CK3dEntity@ ent, CKDWORD &out geo = void, bool &out moving = void, int &out type = void, bool &out hiera = void, bool &out first = void)") == nullptr) {
        error = "CKFloorManager self-test could not find guarded ReadAttributeValues declaration.";
        return false;
    }

    return true;
}

bool RunCKInterfaceManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKInterfaceManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *interfaceManagerType = engine->GetTypeInfoByDecl("CKInterfaceManager");
    if (!interfaceManagerType) {
        error = "CKInterfaceManager self-test could not find the registered type.";
        return false;
    }
    if (interfaceManagerType->GetMethodByDecl("int DoRenameDialog(string &inout name, CK_CLASSID cid)") == nullptr) {
        error = "CKInterfaceManager self-test could not find mutable DoRenameDialog declaration.";
        return false;
    }
    if (interfaceManagerType->GetMethodByDecl("int DoRenameDialog(const string &in name, CK_CLASSID cid)") != nullptr) {
        error = "CKInterfaceManager self-test found stale const DoRenameDialog declaration.";
        return false;
    }

    return true;
}

bool RunCKMidiManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKMidiManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *midiManagerType = engine->GetTypeInfoByDecl("CKMidiManager");
    if (!midiManagerType) {
        error = "CKMidiManager self-test could not find the registered type.";
        return false;
    }
    if (midiManagerType->GetMethodByDecl("NativePointer Create(NativePointer hwnd)") == nullptr ||
        midiManagerType->GetMethodByDecl("void Release(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR SetSoundFileName(NativePointer source, const string &in filename)") == nullptr ||
        midiManagerType->GetMethodByDecl("string GetSoundFileName(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Play(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Pause(NativePointer source, bool pause = true)") == nullptr ||
        midiManagerType->GetMethodByDecl("bool IsPlaying(NativePointer source)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKERROR Time(NativePointer source, CKDWORD &out ticks)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKDWORD MillisecsToTicks(NativePointer source, CKDWORD msOffset)") == nullptr ||
        midiManagerType->GetMethodByDecl("CKDWORD TicksToMillisecs(NativePointer source, CKDWORD tkOffset)") == nullptr) {
        error = "CKMidiManager self-test could not find expected guarded NativePointer methods.";
        return false;
    }

    return true;
}

bool RunCKSoundManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKSoundManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *soundManagerType = engine->GetTypeInfoByDecl("CKSoundManager");
    if (!soundManagerType) {
        error = "CKSoundManager self-test could not find the registered type.";
        return false;
    }
    if (soundManagerType->GetMethodByDecl("void Play(CKWaveSound@ sound, NativePointer source, bool loop)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Pause(CKWaveSound@ sound, NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Stop(CKWaveSound@ sound, NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("NativePointer CreateSource(CK_WAVESOUND_TYPE flags, CKWaveFormat &in wf, CKDWORD bytes, bool streamed)") == nullptr ||
        soundManagerType->GetMethodByDecl("NativePointer DuplicateSource(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void ReleaseSource(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void SetPlayPosition(NativePointer source, int pos)") == nullptr ||
        soundManagerType->GetMethodByDecl("int GetPlayPosition(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("bool IsPlaying(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR SetWaveFormat(NativePointer source, CKWaveFormat &in wf)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR GetWaveFormat(NativePointer source, CKWaveFormat &out wf)") == nullptr ||
        soundManagerType->GetMethodByDecl("int GetWaveSize(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR Lock(NativePointer source, CKDWORD writeCursor, CKDWORD numBytes, NativePointer &out audioPtr1, CKDWORD &out audioBytes1, NativePointer &out audioPtr2, CKDWORD &out audioBytes2, CK_WAVESOUND_LOCKMODE flags)") == nullptr ||
        soundManagerType->GetMethodByDecl("CKERROR Unlock(NativePointer source, NativePointer audioPtr1, CKDWORD numBytes1, NativePointer audioPtr2, CKDWORD audioBytes2)") == nullptr ||
        soundManagerType->GetMethodByDecl("void SetType(NativePointer source, CK_WAVESOUND_TYPE type)") == nullptr ||
        soundManagerType->GetMethodByDecl("CK_WAVESOUND_TYPE GetType(NativePointer source)") == nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSoundSettings &inout settings, bool set = true)") == nullptr ||
        soundManagerType->GetMethodByDecl("void Update3DSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSound3DSettings &inout settings, bool set = true)") == nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateListenerSettings(CK_SOUNDMANAGER_CAPS settingsoptions, CKListenerSettings &inout settings, bool set = true)") == nullptr) {
        error = "CKSoundManager self-test could not find expected guarded source/playback/settings declarations.";
        return false;
    }
    if (soundManagerType->GetMethodByDecl("void UpdateSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSoundSettings &out settings, bool set = true)") != nullptr ||
        soundManagerType->GetMethodByDecl("void Update3DSettings(NativePointer source, CK_SOUNDMANAGER_CAPS settingsoptions, CKWaveSound3DSettings &out settings, bool set = true)") != nullptr ||
        soundManagerType->GetMethodByDecl("void UpdateListenerSettings(CK_SOUNDMANAGER_CAPS settingsoptions, CKListenerSettings &out settings, bool set = true)") != nullptr) {
        error = "CKSoundManager self-test found stale out-only settings declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSoundManagerSelfTest";
    const char *source =
        "void ProbeCKSoundManagerSource(CKSoundManager@ manager, CKWaveFormat &in wf, CKWaveFormat &out outWf, CKWaveSoundSettings &inout settings, CKWaveSound3DSettings &inout settings3d, CKListenerSettings &inout listener) {\n"
        "  if (manager is null) return;\n"
        "  NativePointer source;\n"
        "  NativePointer created = manager.CreateSource(CK_WAVESOUND_BACKGROUND, wf, 0, false);\n"
        "  NativePointer duplicate = manager.DuplicateSource(source);\n"
        "  manager.ReleaseSource(source);\n"
        "  manager.SetPlayPosition(source, 0);\n"
        "  int pos = manager.GetPlayPosition(source);\n"
        "  bool playing = manager.IsPlaying(source);\n"
        "  manager.SetWaveFormat(source, wf);\n"
        "  manager.GetWaveFormat(source, outWf);\n"
        "  int waveSize = manager.GetWaveSize(source);\n"
        "  NativePointer audioPtr1;\n"
        "  NativePointer audioPtr2;\n"
        "  CKDWORD audioBytes1 = 0;\n"
        "  CKDWORD audioBytes2 = 0;\n"
        "  manager.Lock(source, 0, 0, audioPtr1, audioBytes1, audioPtr2, audioBytes2, CK_WAVESOUND_LOCKFROMWRITE);\n"
        "  manager.Unlock(source, audioPtr1, audioBytes1, audioPtr2, audioBytes2);\n"
        "  manager.SetType(source, CK_WAVESOUND_BACKGROUND);\n"
        "  CK_WAVESOUND_TYPE type = manager.GetType(source);\n"
        "  manager.UpdateSettings(source, CK_WAVESOUND_SETTINGS_GAIN, settings, false);\n"
        "  manager.Update3DSettings(source, CK_WAVESOUND_3DSETTINGS_POSITION, settings3d, false);\n"
        "  manager.UpdateListenerSettings(CK_WAVESOUND_3DSETTINGS_POSITION, listener, false);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSoundManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-sound-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKSoundManagerSource(CKSoundManager@, CKWaveFormat &in, CKWaveFormat &out, CKWaveSoundSettings &inout, CKWaveSound3DSettings &inout, CKListenerSettings &inout)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKSoundManager self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKWaveSoundScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKWaveSound script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *waveSoundType = engine->GetTypeInfoByDecl("CKWaveSound");
    if (!waveSoundType) {
        error = "CKWaveSound type is not registered.";
        return false;
    }

    if (waveSoundType->GetMethodByDecl("CKERROR WriteData(NativePointer buffer, int size)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKERROR Lock(CKDWORD writeCursor, CKDWORD numBytes, NativePointer &out ptr1, CKDWORD &out bytes1, NativePointer &out ptr2, CKDWORD &out bytes2, CK_WAVESOUND_LOCKMODE flags)") == nullptr ||
        waveSoundType->GetMethodByDecl("CKERROR Unlock(NativePointer ptr1, CKDWORD bytes1, NativePointer ptr2, CKDWORD bytes2)") == nullptr) {
        error = "CKWaveSound self-test could not find expected guarded audio-buffer methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKWaveSoundSelfTest";
    const char *source =
        "void ProbeCKWaveSoundAudio(CKWaveSound@ sound, NativePointer data) {\n"
        "  if (sound is null) return;\n"
        "  sound.WriteData(data, 0);\n"
        "  NativePointer ptr1;\n"
        "  NativePointer ptr2;\n"
        "  CKDWORD bytes1 = 0;\n"
        "  CKDWORD bytes2 = 0;\n"
        "  sound.Lock(0, 0, ptr1, bytes1, ptr2, bytes2, CK_WAVESOUND_LOCKFROMWRITE);\n"
        "  sound.Unlock(ptr1, bytes1, ptr2, bytes2);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKWaveSound self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-wave-sound-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKWaveSoundAudio(CKWaveSound@, NativePointer)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKWaveSound self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKObjectManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKObjectManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *objectManagerType = engine->GetTypeInfoByDecl("CKObjectManager");
    if (!objectManagerType) {
        error = "CKObjectManager self-test could not find the registered type.";
        return false;
    }
    if (objectManagerType->GetMethodByDecl("CKObject@ GetObject(CK_ID id)") != nullptr) {
        error = "CKObjectManager self-test found the removed GetObject(CK_ID) alias.";
        return false;
    }
    if (objectManagerType->GetMethodByDecl("CKObject@ CKGetObject(CK_ID id)") == nullptr ||
        objectManagerType->GetMethodByDecl("int GetObjectsCount()") == nullptr) {
        error = "CKObjectManager self-test could not find expected object manager methods.";
        return false;
    }

    return true;
}

bool RunCKPathManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKPathManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *pathManagerType = engine->GetTypeInfoByDecl("CKPathManager");
    if (!pathManagerType) {
        error = "CKPathManager self-test could not find the registered type.";
        return false;
    }
    if (pathManagerType->GetMethodByDecl("CKERROR RenameCategory(int catIdx, XString &in newName)") == nullptr ||
        pathManagerType->GetMethodByDecl("int GetPathIndex(int catIdx, XString &in path)") == nullptr ||
        pathManagerType->GetMethodByDecl("CKERROR RenamePath(int catIdx, int pathIdx, XString &in path)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsAbsolute(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsUNC(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsURL(XString &in file)") == nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsFile(XString &in file)") == nullptr) {
        error = "CKPathManager self-test could not find expected mutable XString methods.";
        return false;
    }
    if (pathManagerType->GetMethodByDecl("CKERROR RenameCategory(int catIdx, const XString &in newName)") != nullptr ||
        pathManagerType->GetMethodByDecl("int GetPathIndex(int catIdx, const XString &in path)") != nullptr ||
        pathManagerType->GetMethodByDecl("CKERROR RenamePath(int catIdx, int pathIdx, const XString &in path)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsAbsolute(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsUNC(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsURL(const XString &in file)") != nullptr ||
        pathManagerType->GetMethodByDecl("bool PathIsFile(const XString &in file)") != nullptr) {
        error = "CKPathManager self-test found stale const XString declarations.";
        return false;
    }

    return true;
}

bool RunCKInputManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKInputManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

#if CKVERSION == 0x13022002
    asITypeInfo *inputManagerType = engine->GetTypeInfoByDecl("CKInputManager");
    if (!inputManagerType) {
        error = "CKInputManager self-test could not find the registered type.";
        return false;
    }
    if (inputManagerType->GetMethodByDecl("int GetKeyName(CKDWORD key, string &out keyName)") == nullptr ||
        inputManagerType->GetMethodByDecl("CKDWORD GetKeyFromName(const string &in keyName)") == nullptr ||
        inputManagerType->GetMethodByDecl("void GetMouseButtonsState(CKBYTE &out left, CKBYTE &out right, CKBYTE &out middle, CKBYTE &out extra)") == nullptr) {
        error = "CKInputManager self-test could not find expected key-name or mouse-state methods.";
        return false;
    }
    if (inputManagerType->GetMethodByDecl("void GetMouseButtonsState(CKDWORD &out states)") != nullptr) {
        error = "CKInputManager self-test found stale packed mouse-state declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKInputManagerSelfTest";
    const char *source =
        "void ProbeCKInputManager(CKInputManager@ input) {\n"
        "  if (input is null) return;\n"
        "  string keyName;\n"
        "  input.GetKeyName(0, keyName);\n"
        "  input.GetKeyFromName(keyName);\n"
        "  CKBYTE left = 0;\n"
        "  CKBYTE right = 0;\n"
        "  CKBYTE middle = 0;\n"
        "  CKBYTE extra = 0;\n"
        "  input.GetMouseButtonsState(left, right, middle, extra);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKInputManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-input-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKInputManager(CKInputManager@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKInputManager self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
#else
    return true;
#endif

    return true;
}

bool RunCKRenderManagerScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKRenderManager script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    asITypeInfo *renderManagerType = engine->GetTypeInfoByDecl("CKRenderManager");
    if (!renderManagerType) {
        error = "CKRenderManager self-test could not find the registered type.";
        return false;
    }
    if (renderManagerType->GetMethodByDecl("VxDriverDesc &GetRenderDriverDescription(int driver)") == nullptr) {
        error = "CKRenderManager self-test could not find GetRenderDriverDescription.";
        return false;
    }
    if (renderManagerType->GetMethodByDecl("CKRenderContext@ CreateRenderContext(WIN_HANDLE window, int driver = 0, CKRECT &in rect = void, bool fullscreen = false, int bpp = -1, int zbpp = -1, int stencilBpp = -1, int refreshRate = 0)") == nullptr) {
        error = "CKRenderManager self-test could not find CreateRenderContext with optional rect.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRenderManagerSelfTest";
    const char *source =
        "int ProbeRenderManagerInvalidDriver(CKContext@ ctx) {\n"
        "  CKRenderManager@ rm = ctx.GetRenderManager();\n"
        "  if (rm is null) return 1;\n"
        "  rm.GetRenderDriverDescription(-1);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRenderManager self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-render-manager-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeRenderManagerInvalidDriver(CKContext@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKRenderManager self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, true, "CKRenderManager invalid driver probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPluginManagerScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPluginManager script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *pluginManagerType = engine->GetTypeInfoByDecl("CKPluginManager");
    if (!pluginManagerType) {
        error = "CKPluginManager self-test could not find the registered type.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("bool SetReaderOptionData(CKContext@ context, NativePointer data, CKParameterOut@ param, CKFileExtension ext, CKGUID &in guid = void)") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKParameterOut@ GetReaderOptionData(CKContext@ context, NativePointer data, CKFileExtension ext, CKGUID &in guid = void)") == nullptr) {
        error = "CKPluginManager self-test could not find expected reader option methods.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("int GetCategoryCount()") == nullptr ||
        pluginManagerType->GetMethodByDecl("int GetPluginDllCount()") == nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CKDWORD saveFlags, CKGUID &readerGuid = void)") == nullptr) {
        error = "CKPluginManager self-test could not find expected non-const/count/save declarations.";
        return false;
    }
    if (pluginManagerType->GetMethodByDecl("int GetCategoryCount() const") != nullptr ||
        pluginManagerType->GetMethodByDecl("int GetPluginDllCount() const") != nullptr ||
        pluginManagerType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in fileName, CKObjectArray@ objects, CK_LOAD_FLAGS saveFlags, CKGUID &readerGuid = void)") != nullptr) {
        error = "CKPluginManager self-test found stale count/save declarations.";
        return false;
    }

    return true;
}

bool RunCKBehaviorPrototypeScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBehaviorPrototype script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *prototypeType = engine->GetTypeInfoByDecl("CKBehaviorPrototype");
    if (!prototypeType) {
        error = "CKBehaviorPrototype self-test could not find the registered type.";
        return false;
    }
    if (prototypeType->GetMethodByDecl("void SetFunction(NativePointer fct)") == nullptr ||
        prototypeType->GetMethodByDecl("NativePointer GetFunction()") == nullptr ||
        prototypeType->GetMethodByDecl("void SetBehaviorCallbackFct(NativePointer fct, CKDWORD callbackMask, NativePointer param)") == nullptr ||
        prototypeType->GetMethodByDecl("NativePointer GetBehaviorCallbackFct()") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded pointer methods.";
        return false;
    }
    if (prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareInParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareOutParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareLocalParameter(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType, const string &in defaultVal)") == nullptr ||
        prototypeType->GetMethodByDecl("int DeclareSetting(const string &in name, CKGUID guidType, NativePointer defaultVal, int valSize)") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded parameter declaration overloads.";
        return false;
    }
#if CKVERSION == 0x13022002
    if (prototypeType->GetMethodByDecl("CKBEHAVIORIO_DESC &GetInIOList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKBEHAVIORIO_DESC &GetOutIOList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetInParameterList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetOutParameterList(int index)") == nullptr ||
        prototypeType->GetMethodByDecl("CKPARAMETER_DESC &GetLocalParameterList(int index)") == nullptr) {
        error = "CKBehaviorPrototype self-test could not find expected guarded list methods.";
        return false;
    }
#endif

    constexpr const char *moduleName = "__CKAS_CKBehaviorPrototypeSelfTest";
    const char *source =
        "void ProbeCKBehaviorPrototypePointers(CKBehaviorPrototype@ proto) {\n"
        "  if (proto is null) return;\n"
        "  NativePointer empty;\n"
        "  proto.SetFunction(empty);\n"
        "  NativePointer f = proto.GetFunction();\n"
        "  proto.SetBehaviorCallbackFct(empty, CKCB_BEHAVIORALL, empty);\n"
        "  NativePointer cb = proto.GetBehaviorCallbackFct();\n"
        "}\n";
    const char *parameterSource =
        "void ProbeCKBehaviorPrototypeParameterDeclarations(CKBehaviorPrototype@ proto) {\n"
        "  if (proto is null) return;\n"
        "  NativePointer empty;\n"
        "  proto.DeclareInParameter(\"In\", CKPGUID_INT);\n"
        "  proto.DeclareInParameter(\"InText\", CKPGUID_STRING, \"text\");\n"
        "  proto.DeclareInParameter(\"InRaw\", CKPGUID_INT, empty, 0);\n"
        "  proto.DeclareOutParameter(\"Out\", CKPGUID_INT);\n"
        "  proto.DeclareOutParameter(\"OutText\", CKPGUID_STRING, \"text\");\n"
        "  proto.DeclareOutParameter(\"OutRaw\", CKPGUID_INT, empty, 0);\n"
        "  proto.DeclareLocalParameter(\"Local\", CKPGUID_INT);\n"
        "  proto.DeclareLocalParameter(\"LocalText\", CKPGUID_STRING, \"text\");\n"
        "  proto.DeclareLocalParameter(\"LocalRaw\", CKPGUID_INT, empty, 0);\n"
        "  proto.DeclareSetting(\"Setting\", CKPGUID_BOOL);\n"
        "  proto.DeclareSetting(\"SettingText\", CKPGUID_STRING, \"text\");\n"
        "  proto.DeclareSetting(\"SettingRaw\", CKPGUID_BOOL, empty, 0);\n"
        "}\n";
#if CKVERSION == 0x13022002
    const char *listSource =
        "void ProbeCKBehaviorPrototypeLists(CKBehaviorPrototype@ proto) {\n"
        "  if (proto is null) return;\n"
        "  if (proto.GetInputCount() > 0) {\n"
        "    CKDWORD flags = proto.GetInIOList(0).Flags;\n"
        "  }\n"
        "  if (proto.GetOutputCount() > 0) {\n"
        "    CKDWORD flags = proto.GetOutIOList(0).Flags;\n"
        "  }\n"
        "  if (proto.GetInParameterCount() > 0) {\n"
        "    int owner = proto.GetInParameterList(0).Owner;\n"
        "  }\n"
        "  if (proto.GetOutParameterCount() > 0) {\n"
        "    int owner = proto.GetOutParameterList(0).Owner;\n"
        "  }\n"
        "  if (proto.GetLocalParameterCount() > 0) {\n"
        "    int owner = proto.GetLocalParameterList(0).Owner;\n"
        "  }\n"
        "}\n";
#endif

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBehaviorPrototype self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-behavior-prototype-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its script section.";
        return false;
    }
    r = module->AddScriptSection("ck-behavior-prototype-parameter-self-test", parameterSource);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its parameter script section.";
        return false;
    }
#if CKVERSION == 0x13022002
    r = module->AddScriptSection("ck-behavior-prototype-list-self-test", listSource);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test could not add its list script section.";
        return false;
    }
#endif
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKBehaviorPrototypePointers(CKBehaviorPrototype@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype self-test function was not found.";
        return false;
    }
    asIScriptFunction *parameterProbe = module->GetFunctionByDecl("void ProbeCKBehaviorPrototypeParameterDeclarations(CKBehaviorPrototype@)");
    if (!parameterProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype parameter self-test function was not found.";
        return false;
    }
#if CKVERSION == 0x13022002
    asIScriptFunction *listProbe = module->GetFunctionByDecl("void ProbeCKBehaviorPrototypeLists(CKBehaviorPrototype@)");
    if (!listProbe) {
        engine->DiscardModule(moduleName);
        error = "CKBehaviorPrototype list self-test function was not found.";
        return false;
    }
#endif

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKMaterialScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMaterial script self-test requires an AngelScript engine.";
        return false;
    }

#if CKVERSION != 0x26052005
    asITypeInfo *materialType = engine->GetTypeInfoByDecl("CKMaterial");
    if (!materialType) {
        error = "CKMaterial self-test could not find the registered type.";
        return false;
    }
    if (materialType->GetMethodByDecl("void SetCallback(NativePointer fct, NativePointer argument)") == nullptr ||
        materialType->GetMethodByDecl("NativePointer GetCallback(NativePointer &out argument = void)") == nullptr) {
        error = "CKMaterial self-test could not find expected guarded callback methods.";
        return false;
    }
    if (materialType->GetMethodByDecl("NativePointer GetCallback(NativePointer &out argument = void) const") != nullptr) {
        error = "CKMaterial self-test found stale const callback declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMaterialSelfTest";
    const char *source =
        "void ProbeCKMaterialCallback(CKMaterial@ material) {\n"
        "  if (material is null) return;\n"
        "  NativePointer argument;\n"
        "  NativePointer callback = material.GetCallback(argument);\n"
        "  NativePointer empty;\n"
        "  material.SetCallback(empty, empty);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMaterial self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-material-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("void ProbeCKMaterialCallback(CKMaterial@)");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKMaterial self-test function was not found.";
        return false;
    }

    engine->DiscardModule(moduleName);
#endif
    return true;
}

bool RunCKBitmapReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBitmapReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *bitmapReaderType = engine->GetTypeInfoByDecl("CKBitmapReader");
    if (!bitmapReaderType) {
        error = "CKBitmapReader self-test could not find the registered type.";
        return false;
    }
    if (bitmapReaderType->GetMethodByDecl("int SaveMemory(NativePointer &out memory, CKBitmapProperties@ bp)") == nullptr ||
        bitmapReaderType->GetMethodByDecl("void ReleaseMemory(NativePointer memory)") == nullptr) {
        error = "CKBitmapReader self-test could not find expected memory methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBitmapReaderSelfTest";
    const char *source =
        "void ProbeBitmapReaderSaveMemory(CKBitmapReader@ reader, CKBitmapProperties@ bp) {\n"
        "  NativePointer memory;\n"
        "  if (reader is null) return;\n"
        "  int result = reader.SaveMemory(memory, bp);\n"
        "  if (result == 0) reader.ReleaseMemory(memory);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBitmapReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bitmap-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBitmapReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKSoundReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKSoundReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *soundReaderType = engine->GetTypeInfoByDecl("CKSoundReader");
    if (!soundReaderType) {
        error = "CKSoundReader self-test could not find the registered type.";
        return false;
    }
    if (soundReaderType->GetMethodByDecl("CKERROR GetDataBuffer(NativePointer &out buf, int &out size)") == nullptr ||
        soundReaderType->GetMethodByDecl("CKERROR ReadMemory(NativePointer memory, int size)") == nullptr) {
        error = "CKSoundReader self-test could not find expected memory methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKSoundReaderSelfTest";
    const char *source =
        "void ProbeSoundReaderMemory(CKSoundReader@ reader, NativePointer memory, int size) {\n"
        "  NativePointer buffer;\n"
        "  int bufferSize = 0;\n"
        "  if (reader is null) return;\n"
        "  reader.GetDataBuffer(buffer, bufferSize);\n"
        "  reader.ReadMemory(memory, size);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKSoundReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-sound-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKSoundReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKMovieReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMovieReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *movieReaderType = engine->GetTypeInfoByDecl("CKMovieReader");
    if (!movieReaderType) {
        error = "CKMovieReader self-test could not find the registered type.";
        return false;
    }
    if (movieReaderType->GetMethodByDecl("int GetMovieFrameCount()") == nullptr ||
        movieReaderType->GetMethodByDecl("int GetMovieLength()") == nullptr ||
        movieReaderType->GetMethodByDecl("CKERROR ReadFrame(int f, CKMovieProperties@ &out prop)") == nullptr) {
        error = "CKMovieReader self-test could not find expected reader methods.";
        return false;
    }
    if (movieReaderType->GetMethodByDecl("int GetMovieFrameCount() const") != nullptr ||
        movieReaderType->GetMethodByDecl("int GetMovieLength() const") != nullptr) {
        error = "CKMovieReader self-test found stale const frame-count declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMovieReaderSelfTest";
    const char *source =
        "void ProbeMovieReaderSurface(CKMovieReader@ reader) {\n"
        "  CKMovieProperties@ prop;\n"
        "  if (reader is null) return;\n"
        "  reader.GetMovieFrameCount();\n"
        "  reader.GetMovieLength();\n"
        "  reader.OpenFile(\"\");\n"
        "  reader.OpenMemory(\"\");\n"
        "  reader.OpenAsynchronousFile(\"\");\n"
        "  reader.ReadFrame(0, prop);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMovieReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-movie-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMovieReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKModelReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKModelReader script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *modelReaderType = engine->GetTypeInfoByDecl("CKModelReader");
    if (!modelReaderType) {
        error = "CKModelReader self-test could not find the registered type.";
        return false;
    }
    if (modelReaderType->GetMethodByDecl("CKERROR Load(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD loadFlags, CKCharacter@ carac = null)") == nullptr ||
        modelReaderType->GetMethodByDecl("CKERROR Save(CKContext@ context, const string &in filename, CKObjectArray@ objArray, CKDWORD saveFlags)") == nullptr) {
        error = "CKModelReader self-test could not find expected load/save methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKModelReaderSelfTest";
    const char *source =
        "void ProbeModelReaderSurface(CKModelReader@ reader, CKContext@ ctx, CKObjectArray@ objects, CKCharacter@ carac) {\n"
        "  if (reader is null) return;\n"
        "  reader.Load(ctx, \"\", objects, 0);\n"
        "  reader.Load(ctx, \"\", objects, 0, carac);\n"
        "  reader.Save(ctx, \"\", objects, 0);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKModelReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-model-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKModelReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKModelReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKDataReaderScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKDataReader script self-test requires an AngelScript engine.";
        return false;
    }

    const char *readerTypes[] = {
        "CKDataReader",
        "CKModelReader",
        "CKBitmapReader",
        "CKSoundReader",
        "CKMovieReader",
    };
    for (const char *readerTypeName : readerTypes) {
        asITypeInfo *readerType = engine->GetTypeInfoByDecl(readerTypeName);
        if (!readerType) {
            error = std::string("CKDataReader self-test could not find registered type ") + readerTypeName + ".";
            return false;
        }
        if (readerType->GetMethodByDecl("CKPluginInfo GetReaderInfo()") == nullptr) {
            error = std::string("CKDataReader self-test could not find value GetReaderInfo on ") + readerTypeName + ".";
            return false;
        }
        if (readerType->GetMethodByDecl("CKPluginInfo &GetReaderInfo()") != nullptr) {
            error = std::string("CKDataReader self-test found stale reference GetReaderInfo on ") + readerTypeName + ".";
            return false;
        }
    }

    asITypeInfo *dataReaderType = engine->GetTypeInfoByDecl("CKDataReader");
    if (!dataReaderType ||
        dataReaderType->GetMethodByDecl("CKBitmapReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKModelReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKSoundReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("CKMovieReader@ opCast()") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKBitmapReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKModelReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKSoundReader@ opCast() const") == nullptr ||
        dataReaderType->GetMethodByDecl("const CKMovieReader@ opCast() const") == nullptr) {
        error = "CKDataReader self-test could not find expected checked derived cast declarations.";
        return false;
    }

    const char *derivedReaderTypes[] = {
        "CKModelReader",
        "CKBitmapReader",
        "CKSoundReader",
        "CKMovieReader",
    };
    for (const char *readerTypeName : derivedReaderTypes) {
        asITypeInfo *readerType = engine->GetTypeInfoByDecl(readerTypeName);
        if (!readerType ||
            readerType->GetMethodByDecl("CKDataReader@ opImplCast()") == nullptr ||
            readerType->GetMethodByDecl("const CKDataReader@ opImplCast() const") == nullptr) {
            error = std::string("CKDataReader self-test could not find base cast declarations on ") + readerTypeName + ".";
            return false;
        }
    }

    constexpr const char *moduleName = "__CKAS_CKDataReaderSelfTest";
    const char *source =
        "void ProbeDataReaderInfo(CKDataReader@ data, CKModelReader@ model) {\n"
        "  if (data !is null) {\n"
        "    CKPluginInfo info = data.GetReaderInfo();\n"
        "    info.m_Version = info.m_Version;\n"
        "  }\n"
        "  if (model !is null) {\n"
        "    CKPluginInfo modelInfo = model.GetReaderInfo();\n"
        "    modelInfo.m_Version = modelInfo.m_Version;\n"
        "    CKDataReader@ modelBase = model;\n"
        "    if (modelBase is null) return;\n"
        "  }\n"
        "  if (data !is null) {\n"
        "    CKBitmapReader@ bitmap = cast<CKBitmapReader>(data);\n"
        "    CKModelReader@ modelReader = cast<CKModelReader>(data);\n"
        "    CKSoundReader@ sound = cast<CKSoundReader>(data);\n"
        "    CKMovieReader@ movie = cast<CKMovieReader>(data);\n"
        "    if (bitmap !is null) bitmap.GetReaderInfo();\n"
        "    if (modelReader !is null) modelReader.GetReaderInfo();\n"
        "    if (sound !is null) sound.GetReaderInfo();\n"
        "    if (movie !is null) movie.GetReaderInfo();\n"
        "  }\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKDataReader self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-data-reader-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataReader self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKDataReader self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    if (!keyType) {
        error = "CKKey self-test could not find the registered type.";
        return false;
    }
    if (keyType->GetMethodByDecl("CKRotationKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKTCBRotationKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKTCBPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr ||
        keyType->GetMethodByDecl("CKMorphKey opConv() const") != nullptr) {
        error = "CKKey self-test found stale unsafe base-to-derived value conversion.";
        return false;
    }
    if (keyType->GetMethodByDecl("float GetTime()") == nullptr) {
        error = "CKKey self-test could not find expected non-const GetTime declaration.";
        return false;
    }
    if (keyType->GetMethodByDecl("float GetTime() const") != nullptr) {
        error = "CKKey self-test found stale const GetTime declaration.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKKeySelfTest";
    const char *source =
        "int ProbeKey() {\n"
        "  CKKey key;\n"
        "  if (key.GetTime() != 0.0f) return 1;\n"
        "  key.SetTime(1.25f);\n"
        "  if (key.GetTime() != 1.25f) return 2;\n"
        "  CKKey copy(key);\n"
        "  if (copy.GetTime() != 1.25f) return 3;\n"
        "  CKKey assigned;\n"
        "  assigned = key;\n"
        "  if (assigned.GetTime() != 1.25f) return 4;\n"
        "  assigned.TimeStep = 2.5f;\n"
        "  if (assigned.GetTime() != 2.5f) return 5;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *positionKeyType = engine->GetTypeInfoByDecl("CKPositionKey");
    if (!keyType || !positionKeyType) {
        error = "CKPositionKey self-test could not find key types.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("CKTCBPositionKey opConv() const") != nullptr ||
        positionKeyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr) {
        error = "CKPositionKey self-test found stale unsafe position-key down-conversion.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKPositionKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        positionKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        positionKeyType->GetMethodByDecl("bool Compare(CKPositionKey&in key, float threshold)") == nullptr) {
        error = "CKPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (positionKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        positionKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        positionKeyType->GetMethodByDecl("bool Compare(CKPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKPositionKeySelfTest";
    const char *source =
        "int ProbePositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  CKPositionKey key(1.25f, pos);\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 2;\n"
        "  CKPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 3;\n"
        "  copy.Pos.x = 4.0f;\n"
        "  if (key.Compare(copy, 0.0f)) return 4;\n"
        "  CKPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 5;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 6;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 7;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbePositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKRotationKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKRotationKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *rotationKeyType = engine->GetTypeInfoByDecl("CKRotationKey");
    if (!keyType || !rotationKeyType) {
        error = "CKRotationKey self-test could not find key types.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("CKTCBRotationKey opConv() const") != nullptr) {
        error = "CKRotationKey self-test found stale unsafe rotation-key down-conversion.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKRotationKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        rotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation()") == nullptr ||
        rotationKeyType->GetMethodByDecl("bool Compare(CKRotationKey&in key, float threshold)") == nullptr) {
        error = "CKRotationKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (rotationKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        rotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation() const") != nullptr ||
        rotationKeyType->GetMethodByDecl("bool Compare(CKRotationKey&in key, float threshold) const") != nullptr) {
        error = "CKRotationKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKRotationKeySelfTest";
    const char *source =
        "int ProbeRotationKey() {\n"
        "  VxQuaternion rot(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "  CKRotationKey key(1.25f, rot);\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  VxQuaternion got = key.GetRotation();\n"
        "  if (got.x != 0.0f || got.y != 0.0f || got.z != 0.0f || got.w != 1.0f) return 2;\n"
        "  CKRotationKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 3;\n"
        "  copy.Rot.w = 0.5f;\n"
        "  if (key.Compare(copy, 0.0f)) return 4;\n"
        "  CKRotationKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 5;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 6;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 7;\n"
        "  VxQuaternion moved(0.1f, 0.2f, 0.3f, 0.4f);\n"
        "  key.SetRotation(moved);\n"
        "  got = key.GetRotation();\n"
        "  if (got.x != 0.1f || got.y != 0.2f || got.z != 0.3f || got.w != 0.4f) return 8;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKRotationKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-rotation-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeRotationKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKRotationKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKRotationKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTCBPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKTCBPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tcbPositionKeyType = engine->GetTypeInfoByDecl("CKTCBPositionKey");
    if (!tcbPositionKeyType) {
        error = "CKTCBPositionKey self-test could not find the registered type.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("CKPositionKey opImplConv() const") == nullptr) {
        error = "CKTCBPositionKey self-test could not find safe base value conversions.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        tcbPositionKeyType->GetMethodByDecl("bool Compare(CKTCBPositionKey&in key, float threshold)") == nullptr) {
        error = "CKTCBPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (tcbPositionKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        tcbPositionKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        tcbPositionKeyType->GetMethodByDecl("bool Compare(CKTCBPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKTCBPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTCBPositionKeySelfTest";
    const char *source =
        "int ProbeTCBPositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  CKTCBPositionKey key(1.25f, pos, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f);\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  if (key.tension != 0.1f || key.continuity != 0.2f || key.bias != 0.3f) return 2;\n"
        "  if (key.easeto != 0.4f || key.easefrom != 0.5f) return 3;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 4;\n"
        "  CKTCBPositionKey defaults(2.0f, pos);\n"
        "  if (defaults.tension != 0.0f || defaults.continuity != 0.0f || defaults.bias != 0.0f) return 5;\n"
        "  if (defaults.easeto != 0.0f || defaults.easefrom != 0.0f) return 6;\n"
        "  CKTCBPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 7;\n"
        "  copy.tension = 0.9f;\n"
        "  if (key.Compare(copy, 0.0f)) return 8;\n"
        "  CKTCBPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 9;\n"
        "  CKPositionKey positionBase = key;\n"
        "  if (positionBase.GetTime() != 1.25f) return 10;\n"
        "  VxVector basePos = positionBase.GetPosition();\n"
        "  if (basePos.x != 1.0f || basePos.y != 2.0f || basePos.z != 3.0f) return 11;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 12;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 13;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 14;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTCBPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-tcb-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTCBPositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTCBPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKTCBPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKTCBRotationKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKTCBRotationKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *tcbRotationKeyType = engine->GetTypeInfoByDecl("CKTCBRotationKey");
    if (!tcbRotationKeyType) {
        error = "CKTCBRotationKey self-test could not find the registered type.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("CKRotationKey opImplConv() const") == nullptr) {
        error = "CKTCBRotationKey self-test could not find safe base value conversions.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation()") == nullptr ||
        tcbRotationKeyType->GetMethodByDecl("bool Compare(CKTCBRotationKey&in key, float threshold)") == nullptr) {
        error = "CKTCBRotationKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (tcbRotationKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        tcbRotationKeyType->GetMethodByDecl("const VxQuaternion& GetRotation() const") != nullptr ||
        tcbRotationKeyType->GetMethodByDecl("bool Compare(CKTCBRotationKey&in key, float threshold) const") != nullptr) {
        error = "CKTCBRotationKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKTCBRotationKeySelfTest";
    const char *source =
        "int ProbeTCBRotationKey() {\n"
        "  VxQuaternion rot(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "  CKTCBRotationKey key(1.25f, rot, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f);\n"
        "  if (key.GetTime() != 1.25f) return 1;\n"
        "  if (key.tension != 0.1f || key.continuity != 0.2f || key.bias != 0.3f) return 2;\n"
        "  if (key.easeto != 0.4f || key.easefrom != 0.5f) return 3;\n"
        "  VxQuaternion got = key.GetRotation();\n"
        "  if (got.x != 0.0f || got.y != 0.0f || got.z != 0.0f || got.w != 1.0f) return 4;\n"
        "  CKTCBRotationKey defaults(2.0f, rot);\n"
        "  if (defaults.tension != 0.0f || defaults.continuity != 0.0f || defaults.bias != 0.0f) return 5;\n"
        "  if (defaults.easeto != 0.0f || defaults.easefrom != 0.0f) return 6;\n"
        "  CKTCBRotationKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 7;\n"
        "  copy.tension = 0.9f;\n"
        "  if (key.Compare(copy, 0.0f)) return 8;\n"
        "  CKTCBRotationKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 9;\n"
        "  CKRotationKey rotationBase = key;\n"
        "  if (rotationBase.GetTime() != 1.25f) return 10;\n"
        "  VxQuaternion baseRot = rotationBase.GetRotation();\n"
        "  if (baseRot.x != 0.0f || baseRot.y != 0.0f || baseRot.z != 0.0f || baseRot.w != 1.0f) return 11;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 12;\n"
        "  key.SetTime(2.5f);\n"
        "  if (key.GetTime() != 2.5f) return 13;\n"
        "  VxQuaternion moved(0.1f, 0.2f, 0.3f, 0.4f);\n"
        "  key.SetRotation(moved);\n"
        "  got = key.GetRotation();\n"
        "  if (got.x != 0.1f || got.y != 0.2f || got.z != 0.3f || got.w != 0.4f) return 14;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKTCBRotationKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-tcb-rotation-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeTCBRotationKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKTCBRotationKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKTCBRotationKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKMorphKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMorphKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *morphKeyType = engine->GetTypeInfoByDecl("CKMorphKey");
    if (!morphKeyType) {
        error = "CKMorphKey self-test could not find the registered type.";
        return false;
    }
    for (asUINT i = 0; i < morphKeyType->GetPropertyCount(); ++i) {
        const char *decl = morphKeyType->GetPropertyDeclaration(i, true);
        const std::string propertyDecl = decl ? decl : "";
        if (propertyDecl == "VxVector& PosArray" ||
            propertyDecl == "VxCompressedVector& NormArray") {
            error = "CKMorphKey self-test found stale raw pointer reference property.";
            return false;
        }
    }
    if (morphKeyType->GetMethodByDecl("NativePointer GetPosArray() const") == nullptr ||
        morphKeyType->GetMethodByDecl("void SetPosArray(NativePointer pointer)") == nullptr ||
        morphKeyType->GetMethodByDecl("NativePointer GetNormArray() const") == nullptr ||
        morphKeyType->GetMethodByDecl("void SetNormArray(NativePointer pointer)") == nullptr) {
        error = "CKMorphKey self-test could not find expected NativePointer array accessors.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr) {
        error = "CKMorphKey self-test could not find safe CKKey value conversion.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        morphKeyType->GetMethodByDecl("bool Compare(CKMorphKey&in key, int nbVertex, float threshold)") == nullptr) {
        error = "CKMorphKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (morphKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        morphKeyType->GetMethodByDecl("bool Compare(CKMorphKey&in key, int nbVertex, float threshold) const") != nullptr) {
        error = "CKMorphKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMorphKeySelfTest";
    const char *source =
        "int ProbeMorphKey() {\n"
        "  CKMorphKey key;\n"
        "  if (key.GetTime() != 0.0f) return 1;\n"
        "  if (!key.GetPosArray().IsNull()) return 2;\n"
        "  if (!key.GetNormArray().IsNull()) return 3;\n"
        "  NativePointer empty;\n"
        "  key.SetPosArray(empty);\n"
        "  key.SetNormArray(empty);\n"
        "  if (!key.GetPosArray().IsNull() || !key.GetNormArray().IsNull()) return 4;\n"
        "  key.SetTime(1.25f);\n"
        "  CKMorphKey copy(key);\n"
        "  if (copy.GetTime() != 1.25f) return 5;\n"
        "  CKMorphKey assigned;\n"
        "  assigned = key;\n"
        "  if (assigned.GetTime() != 1.25f) return 6;\n"
        "  if (!key.Compare(copy, 0, 0.0f)) return 7;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 1.25f) return 8;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMorphKeyNegativeCount() {\n"
        "  CKMorphKey left;\n"
        "  CKMorphKey right;\n"
        "  left.Compare(right, -1, 0.0f);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMorphKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-morph-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeMorphKey()");
    asIScriptFunction *negativeCount = module->GetFunctionByDecl("int ProbeMorphKeyNegativeCount()");
    if (!probe || !negativeCount) {
        engine->DiscardModule(moduleName);
        error = "CKMorphKey self-test functions were not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKMorphKey probe", error) &&
                    ExecuteCKAttributeDescProbe(engine, negativeCount, true, "CKMorphKey negative-count probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKAnimControllerScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAnimController script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *controllerType = engine->GetTypeInfoByDecl("CKAnimController");
    if (!controllerType) {
        error = "CKAnimController self-test could not find the registered type.";
        return false;
    }
    if (controllerType->GetMethodByDecl("CKKey& GetKey(int index)") == nullptr ||
        controllerType->GetMethodByDecl("bool Compare(CKAnimController@ control, float threshold = 0.0)") == nullptr ||
        controllerType->GetMethodByDecl("bool Clone(CKAnimController@ control)") == nullptr) {
        error = "CKAnimController self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAnimControllerSelfTest";
    const char *source =
        "void ProbeAnimControllerSurface(CKAnimController@ controller, CKKey &in key, NativePointer result) {\n"
        "  if (controller is null) return;\n"
        "  controller.AddKey(key);\n"
        "  CKKey got = controller.GetKey(0);\n"
        "  controller.Compare(controller);\n"
        "  controller.Clone(controller);\n"
        "  controller.Evaluate(0.0f, result);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAnimController self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-anim-controller-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimController self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKMorphControllerScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKMorphController script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *morphControllerType = engine->GetTypeInfoByDecl("CKMorphController");
    if (!morphControllerType) {
        error = "CKMorphController self-test could not find the registered type.";
        return false;
    }
    if (morphControllerType->GetMethodByDecl("bool Evaluate(float timeStep, NativePointer res)") != nullptr ||
        morphControllerType->GetMethodByDecl("int AddKey(CKKey&in key)") != nullptr ||
        morphControllerType->GetMethodByDecl("int AddKey(CKKey&in key, bool allocateNormals)") != nullptr ||
        morphControllerType->GetMethodByDecl("CKKey& GetKey(int index)") != nullptr) {
        error = "CKMorphController self-test found stale inherited base-key methods.";
        return false;
    }
    if (morphControllerType->GetMethodByDecl("int AddMorphKey(CKMorphKey&in key, bool allocateNormals = true)") == nullptr ||
        morphControllerType->GetMethodByDecl("CKMorphKey& GetMorphKey(int index)") == nullptr ||
        morphControllerType->GetMethodByDecl("bool Evaluate(float timeStep, int vertexCount, NativePointer vertexPtr, CKDWORD vertexStride, NativePointer normalPtr)") == nullptr) {
        error = "CKMorphController self-test could not find expected morph-specific methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKMorphControllerSelfTest";
    const char *source =
        "void ProbeMorphControllerSurface(CKMorphController@ controller, CKMorphKey &in key, NativePointer vertexPtr, NativePointer normalPtr) {\n"
        "  if (controller is null) return;\n"
        "  controller.AddMorphKey(key);\n"
        "  controller.AddMorphKey(key, false);\n"
        "  CKMorphKey got = controller.GetMorphKey(0);\n"
        "  controller.Evaluate(0.0f, 0, vertexPtr, 12, normalPtr);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKMorphController self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-morph-controller-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKMorphController self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKObjectAnimationScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKObjectAnimation script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKObjectAnimation");
    if (!animationType) {
        error = "CKObjectAnimation self-test could not find the registered type.";
        return false;
    }
    if (animationType->GetMethodByDecl("bool EvaluateMorphTarget(float time, int vertexCount, NativePointer vertices, CKDWORD vStride, NativePointer normals)") == nullptr ||
        animationType->GetMethodByDecl("bool Compare(CKObjectAnimation@ anim, float threshold = 0.0)") == nullptr ||
        animationType->GetMethodByDecl("bool ShareDataFrom(CKObjectAnimation@ anim)") == nullptr ||
        animationType->GetMethodByDecl("CKObjectAnimation@ CreateMergedAnimation(CKObjectAnimation@ subAnim2, bool dynamic = false)") == nullptr ||
        animationType->GetMethodByDecl("void CreateTransition(float length, CKObjectAnimation@ animIn, float stepFrom, CKObjectAnimation@ animOut, float stepTo, bool veloc, bool dontTurn, CKAnimKey&in startingSet = void)") == nullptr ||
        animationType->GetMethodByDecl("void Clone(CKObjectAnimation@ anim)") == nullptr) {
        error = "CKObjectAnimation self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKObjectAnimationSelfTest";
    const char *source =
        "void ProbeObjectAnimationSurface(CKObjectAnimation@ anim, CKObjectAnimation@ other, NativePointer vertices, NativePointer normals) {\n"
        "  if (anim is null || other is null) return;\n"
        "  anim.EvaluateMorphTarget(0.0f, 0, vertices, 12, normals);\n"
        "  anim.Compare(other);\n"
        "  anim.ShareDataFrom(other);\n"
        "  CKObjectAnimation@ merged = anim.CreateMergedAnimation(other);\n"
        "  anim.Clone(other);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKObjectAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-object-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKObjectAnimation self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKAnimationScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKAnimation script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKAnimation");
    if (!animationType) {
        error = "CKAnimation self-test could not find the registered type.";
        return false;
    }
    if (animationType->GetMethodByDecl("CKAnimation@ CreateMergedAnimation(CKAnimation@ anim2, bool dynamic = false)") == nullptr ||
        animationType->GetMethodByDecl("float CreateTransition(CKAnimation@ input, CKAnimation@ output, CKDWORD outTransitionMode, float length = 6.0, float frameTo = 0)") == nullptr) {
        error = "CKAnimation self-test could not find expected guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKAnimationSelfTest";
    const char *source =
        "void ProbeAnimationSurface(CKAnimation@ anim, CKAnimation@ other) {\n"
        "  if (anim is null || other is null) return;\n"
        "  CKAnimation@ merged = anim.CreateMergedAnimation(other);\n"
        "  anim.CreateTransition(anim, other, 0);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKAnimation self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKKeyedAnimationScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKKeyedAnimation script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *objectType = engine->GetTypeInfoByDecl("CKObject");
    asITypeInfo *sceneObjectType = engine->GetTypeInfoByDecl("CKSceneObject");
    asITypeInfo *animationType = engine->GetTypeInfoByDecl("CKAnimation");
    asITypeInfo *keyedType = engine->GetTypeInfoByDecl("CKKeyedAnimation");
    if (!objectType || !sceneObjectType || !animationType || !keyedType) {
        error = "CKKeyedAnimation self-test could not find required object hierarchy types.";
        return false;
    }
    if (objectType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        sceneObjectType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        animationType->GetMethodByDecl("CKKeyedAnimation@ opCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKObject@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKSceneObject@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKAnimation@ opImplCast()") == nullptr ||
        keyedType->GetMethodByDecl("CKERROR AddAnimation(CKObjectAnimation@ anim)") == nullptr ||
        keyedType->GetMethodByDecl("CKERROR RemoveAnimation(CKObjectAnimation@ anim)") == nullptr) {
        error = "CKKeyedAnimation self-test could not find expected casts or guarded methods.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKKeyedAnimationSelfTest";
    const char *source =
        "void ProbeKeyedAnimationSurface(CKObject@ object, CKSceneObject@ sceneObject, CKAnimation@ animation, CKKeyedAnimation@ keyed, CKObjectAnimation@ objectAnimation) {\n"
        "  CKKeyedAnimation@ fromObject = cast<CKKeyedAnimation>(object);\n"
        "  CKKeyedAnimation@ fromSceneObject = cast<CKKeyedAnimation>(sceneObject);\n"
        "  CKKeyedAnimation@ fromAnimation = cast<CKKeyedAnimation>(animation);\n"
        "  if (keyed is null) return;\n"
        "  CKObject@ asObject = keyed;\n"
        "  CKSceneObject@ asSceneObject = keyed;\n"
        "  CKAnimation@ asAnimation = keyed;\n"
        "  keyed.AddAnimation(objectAnimation);\n"
        "  keyed.RemoveAnimation(objectAnimation);\n"
        "  CKObjectAnimation@ byIndex = keyed.GetAnimation(0);\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKKeyedAnimation self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-keyed-animation-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKeyedAnimation self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKKeyedAnimation self-test script failed to build.";
        return false;
    }

    engine->DiscardModule(moduleName);
    return true;
}

bool RunCKBezierPositionKeyScriptSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "CKBezierPositionKey script self-test requires an AngelScript engine.";
        return false;
    }

    asITypeInfo *keyType = engine->GetTypeInfoByDecl("CKKey");
    asITypeInfo *positionKeyType = engine->GetTypeInfoByDecl("CKPositionKey");
    asITypeInfo *bezierKeyType = engine->GetTypeInfoByDecl("CKBezierPositionKey");
    if (!keyType || !positionKeyType || !bezierKeyType) {
        error = "CKBezierPositionKey self-test could not find key types.";
        return false;
    }
    if (keyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr ||
        positionKeyType->GetMethodByDecl("CKBezierPositionKey opConv() const") != nullptr) {
        error = "CKBezierPositionKey self-test found stale unsafe base-to-derived value conversion.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("CKKey opImplConv() const") == nullptr ||
        bezierKeyType->GetMethodByDecl("CKPositionKey opImplConv() const") == nullptr) {
        error = "CKBezierPositionKey self-test could not find safe derived-to-base value conversions.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("float GetTime()") == nullptr ||
        bezierKeyType->GetMethodByDecl("const VxVector& GetPosition()") == nullptr ||
        bezierKeyType->GetMethodByDecl("bool Compare(CKBezierPositionKey&in key, float threshold)") == nullptr) {
        error = "CKBezierPositionKey self-test could not find expected non-const SDK declarations.";
        return false;
    }
    if (bezierKeyType->GetMethodByDecl("float GetTime() const") != nullptr ||
        bezierKeyType->GetMethodByDecl("const VxVector& GetPosition() const") != nullptr ||
        bezierKeyType->GetMethodByDecl("bool Compare(CKBezierPositionKey&in key, float threshold) const") != nullptr) {
        error = "CKBezierPositionKey self-test found stale const SDK declarations.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKBezierPositionKeySelfTest";
    const char *source =
        "int ProbeBezierPositionKey() {\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  VxVector input(0.1f, 0.2f, 0.3f);\n"
        "  VxVector output(0.4f, 0.5f, 0.6f);\n"
        "  CKBezierKeyFlags flags;\n"
        "  flags.SetInTangentMode(BEZIER_KEY_TANGENTS);\n"
        "  flags.SetOutTangentMode(BEZIER_KEY_TANGENTS);\n"
        "  CKBezierPositionKey key(2.5f, pos, flags, input, output);\n"
        "  if (key.GetTime() != 2.5f) return 1;\n"
        "  VxVector got = key.GetPosition();\n"
        "  if (got.x != 1.0f || got.y != 2.0f || got.z != 3.0f) return 2;\n"
        "  if (key.Flags.GetInTangentMode() != BEZIER_KEY_TANGENTS) return 3;\n"
        "  CKBezierPositionKey copy(key);\n"
        "  if (!key.Compare(copy, 0.0f)) return 4;\n"
        "  copy.Out.x = 9.0f;\n"
        "  if (key.Compare(copy, 0.0f)) return 5;\n"
        "  CKBezierPositionKey assigned;\n"
        "  assigned = key;\n"
        "  if (!key.Compare(assigned, 0.0f)) return 6;\n"
        "  CKPositionKey positionBase = key;\n"
        "  if (positionBase.GetTime() != 2.5f) return 7;\n"
        "  VxVector basePos = positionBase.GetPosition();\n"
        "  if (basePos.x != 1.0f || basePos.y != 2.0f || basePos.z != 3.0f) return 8;\n"
        "  CKKey baseKey = key;\n"
        "  if (baseKey.GetTime() != 2.5f) return 9;\n"
        "  CKKey baseFromPosition = positionBase;\n"
        "  if (baseFromPosition.GetTime() != 2.5f) return 10;\n"
        "  key.SetTime(3.5f);\n"
        "  if (key.GetTime() != 3.5f) return 11;\n"
        "  VxVector moved(7.0f, 8.0f, 9.0f);\n"
        "  key.SetPosition(moved);\n"
        "  got = key.GetPosition();\n"
        "  if (got.x != 7.0f || got.y != 8.0f || got.z != 9.0f) return 12;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKBezierPositionKey self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-bezier-position-key-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeBezierPositionKey()");
    if (!probe) {
        engine->DiscardModule(moduleName);
        error = "CKBezierPositionKey self-test function was not found.";
        return false;
    }

    const bool ok = ExecuteCKAttributeDescProbe(engine, probe, false, "CKBezierPositionKey probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

bool RunCKParameterTypeDescScriptSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "CKParameterTypeDesc script self-test requires CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_CKParameterTypeDescSelfTest";
    const char *source =
        "int ProbeParameterTypeDesc(CKContext@ ctx) {\n"
        "  CKParameterTypeDesc desc;\n"
        "  if (!desc.CreatorDll.IsNull()) return 1;\n"
        "  NativePointer empty;\n"
        "  desc.CreatorDll = empty;\n"
        "  if (!desc.CreatorDll.IsNull()) return 2;\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  if (pm is null) return 3;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMissingParameterType(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  pm.GetParameterTypeDescription(-2147483647);\n"
        "  return 0;\n"
        "}\n"
        "int ProbeMissingParameterGuid(CKContext@ ctx) {\n"
        "  CKParameterManager@ pm = ctx.GetParameterManager();\n"
        "  CKGUID missing(0x7badc0de, 0x13572468);\n"
        "  pm.GetParameterTypeDescription(missing);\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "CKParameterTypeDesc self-test could not create a script module.";
        return false;
    }

    int r = module->AddScriptSection("ck-parameter-type-desc-self-test", source);
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test could not add its script section.";
        return false;
    }
    r = module->Build();
    if (r < 0) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test script failed to build.";
        return false;
    }

    asIScriptFunction *probe = module->GetFunctionByDecl("int ProbeParameterTypeDesc(CKContext@)");
    asIScriptFunction *missingType = module->GetFunctionByDecl("int ProbeMissingParameterType(CKContext@)");
    asIScriptFunction *missingGuid = module->GetFunctionByDecl("int ProbeMissingParameterGuid(CKContext@)");
    if (!probe || !missingType || !missingGuid) {
        engine->DiscardModule(moduleName);
        error = "CKParameterTypeDesc self-test functions were not found.";
        return false;
    }

    bool ok = ExecuteCKParameterTypeDescProbe(engine, probe, context, false, "CKParameterTypeDesc CreatorDll probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, missingType, context, true, "CKParameterTypeDesc missing type probe", error) &&
              ExecuteCKParameterTypeDescProbe(engine, missingGuid, context, true, "CKParameterTypeDesc missing guid probe", error);

    engine->DiscardModule(moduleName);
    return ok;
}

} // namespace

bool RunScriptParameterRegistrySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(context);
    if (!registry) {
        error = "ScriptParameterRegistry is not available.";
        return false;
    }

    bool sawEnum = false;
    bool sawFlags = false;
    bool sawStruct = false;
    CKParameterManager *pm = registry->GetParameterManager();
    if (!pm) {
        error = "CKParameterManager is not available.";
        return false;
    }

    for (int i = 0; i < pm->GetParameterTypesCount(); ++i) {
        const ScriptParamTypeRecord *record = registry->GetType(i);
        if (!record) {
            continue;
        }

        if (!sawEnum && record->Has(ScriptParamTypeCaps::EnumLike) && !record->EnumEntries.empty()) {
            int value = 0;
            std::string parseError;
            if (!registry->ParseEnumValue(record->Guid, record->EnumEntries[0].Name, value, parseError) ||
                value != record->EnumEntries[0].Value) {
                error = "Enum registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseEnumValue(record->Guid, "__missing_enum__", value, parseError)) {
                error = "Enum registry accepted an unknown token.";
                return false;
            }
            sawEnum = true;
        }

        if (!sawFlags && record->Has(ScriptParamTypeCaps::FlagsLike) && !record->FlagEntries.empty()) {
            CKDWORD value = 0;
            std::string parseError;
            if (!registry->ParseFlagsValue(record->Guid, record->FlagEntries[0].Name, value, parseError) ||
                value != static_cast<CKDWORD>(record->FlagEntries[0].Value)) {
                error = "Flags registry lookup failed: " + parseError;
                return false;
            }
            if (registry->ParseFlagsValue(record->Guid, "__missing_flag__", value, parseError)) {
                error = "Flags registry accepted an unknown token.";
                return false;
            }
            sawFlags = true;
        }

        if (!sawStruct && record->Has(ScriptParamTypeCaps::StructLike) && !record->StructMembers.empty()) {
            if (record->StructMembers[0].Guid == CKGUID() || record->StructMembers[0].Name.empty()) {
                error = "Struct registry member metadata is incomplete.";
                return false;
            }
            sawStruct = true;
        }
    }

    if (!sawEnum || !sawFlags || !sawStruct) {
        error = "Registry did not find enum, flags, and struct parameter metadata.";
        return false;
    }

    const CKGUID enumGuid(0x6a33ce51, 0x22705501);
    CKERROR err = registry->RegisterEnum(enumGuid, "__CKAS_TestEnum", "First=7,Second=9");
    if (err == CK_OK) {
        int value = 0;
        std::string parseError;
        if (!registry->ParseEnumValue(enumGuid, "Second", value, parseError) || value != 9) {
            error = "Registered enum did not refresh registry cache.";
            return false;
        }
    }

    if (!RunCKEnumStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKFlagsStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKStructStructScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKGUIDScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeCategoryDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCK2dCurvePointScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKDependenciesScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTimeProfilerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKRotationKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTCBPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKTCBRotationKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMorphKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAnimControllerScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMorphControllerScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKObjectAnimationScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAnimationScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKKeyedAnimationScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKBezierPositionKeyScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBaseManagerCastScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKTimeManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBehaviorManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMessageManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKGridManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKFloorManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKInterfaceManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKMidiManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKSoundManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKWaveSoundScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKObjectManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKPathManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKInputManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKRenderManagerScriptSelfTest(context, engine, error)) {
        return false;
    }
    if (!RunCKBitmapReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKSoundReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMovieReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKModelReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKDataReaderScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKPluginManagerScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKBehaviorPrototypeScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKMaterialScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKParameterTypeDescScriptSelfTest(context, engine, error)) {
        return false;
    }

    error.clear();
    return true;
}
