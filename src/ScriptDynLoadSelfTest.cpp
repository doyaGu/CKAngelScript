#include "ScriptSelfTests.h"

#include <angelscript.h>

bool RunScriptDynLoadSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "DynLoad self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_DynLoadSelfTest";
    const char *source =
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
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "DynLoad self-test could not create module.";
        return false;
    }

    int r = module->AddScriptSection("dynload-self-test", source);
    if (r < 0) {
        error = "DynLoad self-test could not add script section.";
        return false;
    }

    r = module->Build();
    if (r < 0) {
        error = "DynLoad self-test module build failed.";
        return false;
    }

    asIScriptFunction *function = module->GetFunctionByDecl("int RunDynSymbols()");
    if (!function) {
        error = "DynLoad self-test function was not compiled.";
        return false;
    }

    asIScriptContext *context = engine->RequestContext();
    if (!context) {
        error = "DynLoad self-test could not create execution context.";
        return false;
    }

    r = context->Prepare(function);
    if (r >= 0) {
        r = context->Execute();
    }

    bool ok = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(context->GetReturnDWord());
        if (returnCode == 0) {
            ok = true;
        } else {
            error = "DynLoad self-test returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = context->GetExceptionString();
        error = "DynLoad self-test exception: ";
        error += exception && exception[0] ? exception : "<empty>";
    } else {
        error = "DynLoad self-test execution failed with code " + std::to_string(r) + ".";
    }

    context->Unprepare();
    engine->ReturnContext(context);
    return ok;
}
