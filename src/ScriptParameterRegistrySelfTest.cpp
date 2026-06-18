#include "ScriptSelfTests.h"

#include "angelscript.h"

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

    error.clear();
    return true;
}
