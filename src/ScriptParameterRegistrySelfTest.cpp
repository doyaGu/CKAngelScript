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
        inputManagerType->GetMethodByDecl("CKDWORD GetKeyFromName(const string &in keyName)") == nullptr) {
        error = "CKInputManager self-test could not find expected key-name methods.";
        return false;
    }
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
    if (!RunCKAttributeDescScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKAttributeCategoryDescScriptSelfTest(engine, error)) {
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
    if (!RunCKPluginManagerScriptSelfTest(engine, error)) {
        return false;
    }
    if (!RunCKParameterTypeDescScriptSelfTest(context, engine, error)) {
        return false;
    }

    error.clear();
    return true;
}
