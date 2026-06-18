#include "ScriptSelfTests.h"

#include "CKAngelScript.h"

#include <angelscript.h>
#include <windows.h>

namespace {

std::string EscapeScriptString(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        if (ch == '\\' || ch == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

bool GetCurrentModulePath(std::string &path) {
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&RunScriptDynLoadSelfTest),
                            &module)) {
        return false;
    }

    char buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (length == 0 || length >= sizeof(buffer)) {
        return false;
    }

    path.assign(buffer, length);
    return true;
}

} // namespace

bool RunScriptDynLoadSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "DynLoad self-test requires an AngelScript engine.";
        return false;
    }

    std::string modulePath;
    if (!GetCurrentModulePath(modulePath)) {
        error = "DynLoad self-test could not resolve the current module path.";
        return false;
    }

    asITypeInfo *dynCallType = engine->GetTypeInfoByDecl("DynCall");
    if (!dynCallType) {
        error = "DynLoad self-test could not resolve DynCall type.";
        return false;
    }
    if (!dynCallType->GetMethodByDecl("DynCall& BeginCallAggr(const DynAggregate&in)") ||
        dynCallType->GetMethodByDecl("DynCall& BeginCallAggr(const DynAggregate&inout)")) {
        error = "DynCall BeginCallAggr declaration direction is not const &in.";
        return false;
    }
    if (!dynCallType->GetMethodByDecl("NativePointer CallAggregate(NativePointer, const DynAggregate&in, NativePointer)") ||
        dynCallType->GetMethodByDecl("NativePointer CallAggregate(NativePointer, const DynAggregate&inout, NativePointer)")) {
        error = "DynCall CallAggregate declaration direction is not const &in.";
        return false;
    }

    asITypeInfo *dynCallbackType = engine->GetTypeInfoByDecl("DynCallback");
    if (!dynCallbackType) {
        error = "DynLoad self-test could not resolve DynCallback type.";
        return false;
    }
    if ((dynCallbackType->GetFlags() & asOBJ_GC) == 0) {
        error = "DynLoad self-test found DynCallback missing GC type flag.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_DynLoadSelfTest";
    const std::string source =
        "NativePointer FindExport(DynLibrary@ lib, const string &in name) {\n"
        "  NativePointer symbol = lib.FindSymbol(name);\n"
        "  if (symbol.IsNull()) symbol = lib.FindSymbol(\"_\" + name);\n"
        "  return symbol;\n"
        "}\n"
        "int RunDynSymbols() {\n"
        "  DynSymbols@ symbols = DynSymbols();\n"
        "  if (symbols is null) return 1;\n"
        "  if (symbols.IsInited()) return 2;\n"
        "  if (symbols.GetCount() != 0) return 3;\n"
        "  if (symbols.GetName(-1) != \"\") return 4;\n"
        "  NativePointer nullValue;\n"
        "  if (symbols.GetName(nullValue) != \"\") return 5;\n"
        "  NativePointer unknownValue(uintptr_t(1));\n"
        "  if (symbols.GetName(unknownValue) != \"\") return 6;\n"
        "  return 0;\n"
        "}\n"
        "int RunDynLibrary() {\n"
        "  DynLibrary@ lib = DynLibrary();\n"
        "  if (lib is null) return 1;\n"
        "  if (lib.IsLoaded()) return 2;\n"
        "  if (!lib.Load(\"" + EscapeScriptString(modulePath) + "\")) return 3;\n"
        "  if (!lib.IsLoaded()) return 4;\n"
        "  if (lib.GetLibraryPath() == \"\") return 5;\n"
        "  NativePointer version = FindExport(lib, \"CKAngelScriptGetApiVersion\");\n"
        "  if (version.IsNull()) return 6;\n"
        "  @lib = null;\n"
        "  DynCall@ call = DynCall();\n"
        "  if (call is null) return 7;\n"
        "  int apiVersion = call.CallInt(version);\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 8;\n"
        "  return apiVersion == " + std::to_string(CKAS_API_VERSION) + " ? 0 : 9;\n"
        "}\n"
        "int RunDynCall() {\n"
        "  DynLibrary@ lib = DynLibrary();\n"
        "  if (lib is null) return 1;\n"
        "  if (!lib.Load(\"" + EscapeScriptString(modulePath) + "\")) return 2;\n"
        "  NativePointer hasFeature = FindExport(lib, \"CKAngelScriptHasFeature\");\n"
        "  if (hasFeature.IsNull()) return 3;\n"
        "  NativePointer statusName = FindExport(lib, \"CKAngelScriptGetStatusName\");\n"
        "  if (statusName.IsNull()) return 4;\n"
        "  DynCall@ call = DynCall(4096);\n"
        "  if (call is null) return 5;\n"
        "  call.Mode(DC_CALL_C_DEFAULT).Reset().ArgInt(12);\n"
        "  if (call.CallInt(hasFeature) == 0) return 6;\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 7;\n"
        "  string name = call.Reset().ArgInt(0).CallString(statusName);\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 8;\n"
        "  if (name != \"CKAS_OK\") return 9;\n"
        "  return 0;\n"
        "}\n"
        "int RunDynAggregate() {\n"
        "  DynAggregate@ child = DynAggregate(1, 4);\n"
        "  if (child is null) return 1;\n"
        "  child.Field(DC_SIGCHAR_INT, 0);\n"
        "  child.Close();\n"
        "  DynAggregate@ parent = DynAggregate(2, 8);\n"
        "  if (parent is null) return 2;\n"
        "  parent.AggregateField(child, 0);\n"
        "  @child = null;\n"
        "  parent.Field(DC_SIGCHAR_INT, 4);\n"
        "  parent.Close();\n"
        "  @parent = null;\n"
        "  return 0;\n"
        "}\n"
        "int RunDynAggregateOverflow() {\n"
        "  DynAggregate@ aggr = DynAggregate(1, 8);\n"
        "  aggr.Field(DC_SIGCHAR_INT, 0);\n"
        "  aggr.Field(DC_SIGCHAR_INT, 4);\n"
        "  return 1;\n"
        "}\n"
        "int RunDynAggregateAfterClose() {\n"
        "  DynAggregate@ aggr = DynAggregate(1, 4);\n"
        "  aggr.Close();\n"
        "  aggr.Field(DC_SIGCHAR_INT, 0);\n"
        "  return 1;\n"
        "}\n"
        "int RunDynAggregateCycle() {\n"
        "  DynAggregate@ aggr = DynAggregate(1, 4);\n"
        "  aggr.AggregateField(aggr, 0);\n"
        "  return 1;\n"
        "}\n"
        "int8 DynArgsScalarHandler(NativePointer pcb, DynArgs &args, DynValue &result) {\n"
        "  if (pcb.IsNull()) { result.SetInt(101); return DC_SIGCHAR_INT; }\n"
        "  result.SetInt(args.ArgInt() + 7);\n"
        "  return DC_SIGCHAR_INT;\n"
        "}\n"
        "int8 DynCallbackInitHandler(NativePointer pcb, DynArgs &args, DynValue &result) {\n"
        "  if (pcb.IsNull()) { result.SetInt(102); return DC_SIGCHAR_INT; }\n"
        "  result.SetInt(args.ArgInt() + 11);\n"
        "  return DC_SIGCHAR_INT;\n"
        "}\n"
        "int RunDynCallback() {\n"
        "  DynCallback@ scalarCb = DynCallback(\"i)i\", DynArgsScalarHandler);\n"
        "  if (scalarCb is null) return 1;\n"
        "  if (scalarCb.GetHandler() is null) return 2;\n"
        "  DynCall@ call = DynCall();\n"
        "  if (call is null) return 3;\n"
        "  int scalar = call.Reset().ArgInt(35).CallInt(scalarCb.GetCallback());\n"
        "  if (scalar != 42) return scalar;\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 4;\n"
        "  scalarCb.Init(\"i)i\", DynCallbackInitHandler);\n"
        "  if (scalarCb.GetHandler() is null) return 5;\n"
        "  int rebound = call.Reset().ArgInt(31).CallInt(scalarCb.GetCallback());\n"
        "  if (rebound != 42) return rebound;\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 6;\n"
        "  return 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "DynLoad self-test could not create module.";
        return false;
    }

    int r = module->AddScriptSection("dynload-self-test", source.c_str(), source.size());
    if (r < 0) {
        error = "DynLoad self-test could not add script section.";
        return false;
    }

    r = module->Build();
    if (r < 0) {
        error = "DynLoad self-test module build failed.";
        return false;
    }

    auto executeIntFunction = [&](const char *decl, const char *label) -> bool {
        asIScriptFunction *function = module->GetFunctionByDecl(decl);
        if (!function) {
            error = std::string("DynLoad ") + label + " self-test function was not compiled.";
            return false;
        }

        asIScriptContext *context = engine->RequestContext();
        if (!context) {
            error = std::string("DynLoad self-test could not create ") + label + " execution context.";
            return false;
        }

        int exec = context->Prepare(function);
        if (exec >= 0) {
            exec = context->Execute();
        }

        bool ok = false;
        if (exec == asEXECUTION_FINISHED) {
            const int returnCode = static_cast<int>(context->GetReturnDWord());
            if (returnCode == 0) {
                ok = true;
            } else {
                error = std::string(label) + " self-test returned " + std::to_string(returnCode) + ".";
            }
        } else if (exec == asEXECUTION_EXCEPTION) {
            const char *exception = context->GetExceptionString();
            error = std::string(label) + " self-test exception: ";
            error += exception && exception[0] ? exception : "<empty>";
        } else {
            error = std::string(label) + " self-test execution failed with code " + std::to_string(exec) + ".";
        }

        context->Unprepare();
        engine->ReturnContext(context);
        return ok;
    };

    auto executeExpectedException = [&](const char *decl, const char *label, const char *expected) -> bool {
        asIScriptFunction *function = module->GetFunctionByDecl(decl);
        if (!function) {
            error = std::string("DynLoad ") + label + " self-test function was not compiled.";
            return false;
        }

        asIScriptContext *context = engine->RequestContext();
        if (!context) {
            error = std::string("DynLoad self-test could not create ") + label + " execution context.";
            return false;
        }

        int exec = context->Prepare(function);
        if (exec >= 0) {
            exec = context->Execute();
        }

        bool ok = false;
        if (exec == asEXECUTION_EXCEPTION) {
            const char *exception = context->GetExceptionString();
            const std::string message = exception ? exception : "";
            if (message.find(expected) != std::string::npos) {
                ok = true;
            } else {
                error = std::string(label) + " self-test raised unexpected exception: " + message;
            }
        } else if (exec == asEXECUTION_FINISHED) {
            error = std::string(label) + " self-test finished without expected exception.";
        } else {
            error = std::string(label) + " self-test execution failed with code " + std::to_string(exec) + ".";
        }

        context->Unprepare();
        engine->ReturnContext(context);
        return ok;
    };

    return executeIntFunction("int RunDynSymbols()", "DynSymbols") &&
           executeIntFunction("int RunDynLibrary()", "DynLibrary") &&
           executeIntFunction("int RunDynCall()", "DynCall") &&
           executeIntFunction("int RunDynAggregate()", "DynAggregate") &&
           executeExpectedException("int RunDynAggregateOverflow()", "DynAggregate overflow", "field capacity exceeded") &&
           executeExpectedException("int RunDynAggregateAfterClose()", "DynAggregate after close", "closed DynAggregate") &&
           executeExpectedException("int RunDynAggregateCycle()", "DynAggregate cycle", "nesting cycle") &&
           executeIntFunction("int RunDynCallback()", "DynCallback");
}
