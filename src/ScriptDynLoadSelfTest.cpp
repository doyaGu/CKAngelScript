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

    constexpr const char *moduleName = "__CKAS_DynLoadSelfTest";
    const std::string source =
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
        "  NativePointer version = lib.FindSymbol(\"CKAngelScriptGetApiVersion\");\n"
        "  if (version.IsNull()) version = lib.FindSymbol(\"_CKAngelScriptGetApiVersion\");\n"
        "  if (version.IsNull()) return 6;\n"
        "  @lib = null;\n"
        "  DynCall@ call = DynCall();\n"
        "  if (call is null) return 7;\n"
        "  int apiVersion = call.CallInt(version);\n"
        "  if (call.GetError() != DC_ERROR_NONE) return 8;\n"
        "  return apiVersion == " + std::to_string(CKAS_API_VERSION) + " ? 0 : 9;\n"
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

    asIScriptFunction *function = module->GetFunctionByDecl("int RunDynSymbols()");
    if (!function) {
        error = "DynLoad DynSymbols self-test function was not compiled.";
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
    if (!ok) {
        return false;
    }

    function = module->GetFunctionByDecl("int RunDynLibrary()");
    if (!function) {
        error = "DynLoad DynLibrary self-test function was not compiled.";
        return false;
    }

    context = engine->RequestContext();
    if (!context) {
        error = "DynLoad self-test could not create DynLibrary execution context.";
        return false;
    }

    r = context->Prepare(function);
    if (r >= 0) {
        r = context->Execute();
    }

    ok = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(context->GetReturnDWord());
        if (returnCode == 0) {
            ok = true;
        } else {
            error = "DynLibrary self-test returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = context->GetExceptionString();
        error = "DynLibrary self-test exception: ";
        error += exception && exception[0] ? exception : "<empty>";
    } else {
        error = "DynLibrary self-test execution failed with code " + std::to_string(r) + ".";
    }

    context->Unprepare();
    engine->ReturnContext(context);
    return ok;
}
