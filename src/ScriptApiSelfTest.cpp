#include "ScriptSelfTests.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"

static int CkasSelfTestExtensionValue() {
    return 77;
}

static int RegisterCkasSelfTestExtension(asIScriptEngine *engine,
                                         CKAngelScript *,
                                         void *,
                                         const char **errorMessage) {
    if (!engine) {
        if (errorMessage) {
            *errorMessage = "Self-test extension received a null engine.";
        }
        return -1;
    }

    int r = engine->SetDefaultNamespace("CKASExtensionSelfTest");
    if (r < 0) {
        return r;
    }
    r = engine->RegisterGlobalFunction("int Value()", asFUNCTION(CkasSelfTestExtensionValue), asCALL_CDECL);
    const int reset = engine->SetDefaultNamespace("");
    if (r < 0) {
        return r;
    }
    return reset < 0 ? reset : 0;
}

static int RegisterCkasFailingSelfTestExtension(asIScriptEngine *,
                                                CKAngelScript *,
                                                void *,
                                                const char **errorMessage) {
    if (errorMessage) {
        *errorMessage = "Expected self-test extension failure.";
    }
    return -77;
}

namespace {

struct IntExecutionData {
    int Input = 0;
    int Output = 0;
};

struct ObjectExecutionData {
    CKBOOL BoolInput = FALSE;
    CKBOOL BoolOutput = FALSE;
    int IntInput = 0;
    int IntOutput = 0;
    float FloatInput = 0.0f;
    float FloatOutput = 0.0f;
    const char *StringInput = nullptr;
    char StringOutput[64] = {};
    size_t RequiredSize = 0;
};

void ConfigureIntArgument(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<IntExecutionData *>(userData);
    ctx->SetArgDWord(0, static_cast<asDWORD>(data ? data->Input : 0));
}

void ReadIntReturn(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<IntExecutionData *>(userData);
    if (data) {
        data->Output = static_cast<int>(ctx->GetReturnDWord());
    }
}

CKAS_STATUS WriteObjectInt(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptArgSetInt(writer, 0, data ? data->IntInput : 0);
}

CKAS_STATUS ReadObjectInt(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptResultGetInt(reader, data ? &data->IntOutput : nullptr);
}

CKAS_STATUS WriteObjectBool(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptArgSetBool(writer, 0, data ? data->BoolInput : FALSE);
}

CKAS_STATUS ReadObjectBool(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptResultGetBool(reader, data ? &data->BoolOutput : nullptr);
}

CKAS_STATUS WriteObjectFloat(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptArgSetFloat(writer, 0, data ? data->FloatInput : 0.0f);
}

CKAS_STATUS ReadObjectFloat(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptResultGetFloat(reader, data ? &data->FloatOutput : nullptr);
}

CKAS_STATUS WriteObjectString(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptArgSetString(writer, 0, data ? data->StringInput : "");
}

CKAS_STATUS ReadObjectString(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    if (!data) {
        return CKAS_INVALIDARGUMENT;
    }
    return CKAngelScriptResultGetString(reader, data->StringOutput, sizeof(data->StringOutput), &data->RequiredSize);
}

CKAS_STATUS ReadObjectStringTooSmall(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    if (!data) {
        return CKAS_INVALIDARGUMENT;
    }
    return CKAngelScriptResultGetString(reader, data->StringOutput, 4, &data->RequiredSize);
}

CKAS_STATUS WriteObjectIntAsBool(CKAngelScriptArgWriter *writer, void *) {
    return CKAngelScriptArgSetBool(writer, 0, TRUE);
}

bool WriteTextFile(const std::filesystem::path &path, const char *text, std::string &error) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        error = "CKAngelScript API self-test failed to create temporary script file.";
        return false;
    }
    out << (text ? text : "");
    return true;
}

void RemoveTextFile(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool ContainsCompileLocation(const CKAngelScriptResult &result) {
    if (!result.ErrorMessage || result.ErrorMessage[0] == '\0') {
        return false;
    }
    const std::string message = result.ErrorMessage;
    return message.find('(') != std::string::npos &&
           message.find(')') != std::string::npos &&
           message.find("ERROR") != std::string::npos;
}

bool ExpectStatus(CKAS_STATUS actual,
                  CKAS_STATUS expected,
                  const char *label,
                  const CKAngelScriptResult *result,
                  std::string &error) {
    if (actual == expected) {
        return true;
    }
    error = std::string(label ? label : "CKAngelScript API") + " returned unexpected status.";
    if (result && result->ErrorMessage && result->ErrorMessage[0] != '\0') {
        error += " ";
        error += result->ErrorMessage;
    }
    return false;
}

} // namespace

bool RunScriptApiSelfTest(CKContext *context, std::string &error) {
    if (!context) {
        error = "CKAngelScript API self-test requires a CKContext.";
        return false;
    }

    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "CKAngelScript API self-test could not retrieve the public API.";
        return false;
    }

    CKAngelScriptResult result = {};
    CKAngelScriptEngineExtension failingExtension = CKAngelScriptApi::EngineExtension();
    failingExtension.Name = "__ckas_failing_extension";
    failingExtension.Register = RegisterCkasFailingSelfTestExtension;
    if (api->RegisterEngineExtension(failingExtension, &result) != CKAS_EXECUTIONFAILED) {
        error = "CKAngelScript API self-test expected failing extension registration to fail.";
        return false;
    }
    if (!result.ErrorMessage || std::string(result.ErrorMessage).find("Expected self-test extension failure") == std::string::npos) {
        error = "CKAngelScript API self-test expected extension failure diagnostics.";
        return false;
    }
    // A failing extension is retained (it replays on the next rebuild), so drop
    // it here to avoid polluting later engine rebuilds with a known failure.
    if (!ExpectStatus(api->UnregisterEngineExtension("__ckas_failing_extension", nullptr, &result),
                      CKAS_OK,
                      "UnregisterEngineExtension failing",
                      &result,
                      error)) {
        return false;
    }

    CKAngelScriptEngineExtension extension = CKAngelScriptApi::EngineExtension();
    extension.Name = "__ckas_public_api_extension";
    extension.Register = RegisterCkasSelfTestExtension;
    if (!ExpectStatus(api->RegisterEngineExtension(extension, &result),
                      CKAS_OK,
                      "RegisterEngineExtension",
                      &result,
                      error)) {
        return false;
    }
    if (api->RegisterEngineExtension(extension, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected duplicate extension registration to fail.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_ManagerApiSelfTest";
    const char *source =
        "int __ckas_public_add(int value) { return value + 5; }\n"
        "int __ckas_public_extension() { return CKASExtensionSelfTest::Value(); }\n"
        "int __ckas_public_async() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 9; }\n"
        "int __ckas_public_exception() { array<int> values; return values[1]; }\n";

    if (!ExpectStatus(api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result),
                      CKAS_OK,
                      "CompileModule",
                      &result,
                      error)) {
        return false;
    }

    if (!api->HasModule(moduleName) || !api->GetModule(moduleName)) {
        error = "CKAngelScript API self-test could not query the compiled module.";
        return false;
    }
    if (!api->FindFunctionByName(moduleName, "__ckas_public_add") ||
        !api->FindFunctionByName(moduleName, "__ckas_public_extension") ||
        !api->FindFunctionByDecl(moduleName, "int __ckas_public_add(int)")) {
        error = "CKAngelScript API self-test could not query compiled functions.";
        return false;
    }

    if (api->CompileModule(moduleName, source, CKAS_COMPILE_DEFAULT, &result) == CKAS_OK) {
        error = "CKAngelScript API self-test expected duplicate CompileModule without replace to fail.";
        return false;
    }
    if (!api->GetLastResult() || api->GetLastResult()->Status == CKAS_OK) {
        error = "CKAngelScript API self-test expected duplicate CompileModule to update LastResult.";
        return false;
    }

    constexpr const char *loadModuleName = "__CKAS_ManagerApiLoadSelfTest";
    CKAngelScriptLoadOptions loadOptions = CKAngelScriptApi::LoadOptions();
    loadOptions.ModuleName = loadModuleName;
    loadOptions.Code = "int __ckas_public_loaded() { return 3; }\n";
    loadOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (!ExpectStatus(api->LoadModule(loadOptions, &result),
                      CKAS_OK,
                      "LoadModule code",
                      &result,
                      error)) {
        return false;
    }
    if (!api->FindFunctionByName(loadModuleName, "__ckas_public_loaded")) {
        error = "CKAngelScript API self-test could not query LoadModule code output.";
        return false;
    }
    api->UnloadModule(loadModuleName, nullptr);

    const std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    const std::filesystem::path singleFile = tempDir / "__ckas_public_api_single.as";
    const std::filesystem::path multiFileA = tempDir / "__ckas_public_api_multi_a.as";
    const std::filesystem::path multiFileB = tempDir / "__ckas_public_api_multi_b.as";
    const std::filesystem::path defaultFile = std::filesystem::current_path() / "__CKAS_ManagerApiDefaultFileLoadSelfTest.as";
    if (!WriteTextFile(singleFile, "int __ckas_public_file_loaded() { return 11; }\n", error) ||
        !WriteTextFile(multiFileA, "int __ckas_public_multi_a() { return 12; }\n", error) ||
        !WriteTextFile(multiFileB, "int __ckas_public_multi_b() { return __ckas_public_multi_a() + 1; }\n", error) ||
        !WriteTextFile(defaultFile, "int __ckas_public_default_file_loaded() { return 14; }\n", error)) {
        return false;
    }

    constexpr const char *singleFileModuleName = "__CKAS_ManagerApiSingleFileLoadSelfTest";
    CKAngelScriptLoadOptions singleFileOptions = CKAngelScriptApi::LoadOptions();
    singleFileOptions.ModuleName = singleFileModuleName;
    const std::string singleFilePath = singleFile.string();
    singleFileOptions.Filename = singleFilePath.c_str();
    singleFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (!ExpectStatus(api->LoadModule(singleFileOptions, &result),
                      CKAS_OK,
                      "LoadModule single file",
                      &result,
                      error)) {
        return false;
    }
    if (!api->FindFunctionByName(singleFileModuleName, "__ckas_public_file_loaded")) {
        error = "CKAngelScript API self-test could not query LoadModule single-file output.";
        return false;
    }
    api->UnloadModule(singleFileModuleName, nullptr);

    constexpr const char *multiFileModuleName = "__CKAS_ManagerApiMultiFileLoadSelfTest";
    const std::string multiFilePathA = multiFileA.string();
    const std::string multiFilePathB = multiFileB.string();
    const char *multiFiles[] = { multiFilePathA.c_str(), multiFilePathB.c_str() };
    CKAngelScriptLoadOptions multiFileOptions = CKAngelScriptApi::LoadOptions();
    multiFileOptions.ModuleName = multiFileModuleName;
    multiFileOptions.Filenames = multiFiles;
    multiFileOptions.FileCount = 2;
    multiFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (!ExpectStatus(api->LoadModule(multiFileOptions, &result),
                      CKAS_OK,
                      "LoadModule multi file",
                      &result,
                      error)) {
        return false;
    }
    if (!api->FindFunctionByName(multiFileModuleName, "__ckas_public_multi_b")) {
        error = "CKAngelScript API self-test could not query LoadModule multi-file output.";
        return false;
    }
    api->UnloadModule(multiFileModuleName, nullptr);

    const char *invalidFiles[] = { multiFilePathA.c_str(), nullptr };
    CKAngelScriptLoadOptions invalidFileListOptions = CKAngelScriptApi::LoadOptions();
    invalidFileListOptions.ModuleName = "__CKAS_ManagerApiInvalidFileListSelfTest";
    invalidFileListOptions.Filenames = invalidFiles;
    invalidFileListOptions.FileCount = 2;
    invalidFileListOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(invalidFileListOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with an invalid file list entry to fail.";
        api->UnloadModule(invalidFileListOptions.ModuleName, nullptr);
        return false;
    }

    constexpr const char *defaultFileModuleName = "__CKAS_ManagerApiDefaultFileLoadSelfTest";
    CKAngelScriptLoadOptions defaultFileOptions = CKAngelScriptApi::LoadOptions();
    defaultFileOptions.ModuleName = defaultFileModuleName;
    defaultFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (!ExpectStatus(api->LoadModule(defaultFileOptions, &result),
                      CKAS_OK,
                      "LoadModule default file",
                      &result,
                      error)) {
        RemoveTextFile(defaultFile);
        return false;
    }
    if (!api->FindFunctionByName(defaultFileModuleName, "__ckas_public_default_file_loaded")) {
        error = "CKAngelScript API self-test could not query LoadModule default-file output.";
        api->UnloadModule(defaultFileModuleName, nullptr);
        RemoveTextFile(defaultFile);
        return false;
    }
    api->UnloadModule(defaultFileModuleName, nullptr);
    RemoveTextFile(defaultFile);

    CKAngelScriptLoadOptions missingFileOptions = CKAngelScriptApi::LoadOptions();
    missingFileOptions.ModuleName = "__CKAS_ManagerApiMissingFileLoadSelfTest";
    const std::string missingFilePath = (tempDir / "__ckas_public_api_missing_file.as").string();
    missingFileOptions.Filename = missingFilePath.c_str();
    missingFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(missingFileOptions, &result) != CKAS_COMPILEERROR ||
        !result.ErrorMessage ||
        result.ErrorMessage[0] == '\0') {
        error = "CKAngelScript API self-test expected missing file LoadModule to return diagnostics.";
        api->UnloadModule(missingFileOptions.ModuleName, nullptr);
        return false;
    }

    CKAngelScriptLoadOptions conflictingOptions = CKAngelScriptApi::LoadOptions();
    conflictingOptions.ModuleName = "__CKAS_ManagerApiConflictingLoadSelfTest";
    conflictingOptions.Code = "int __ckas_public_conflict() { return 1; }\n";
    conflictingOptions.Filename = singleFilePath.c_str();
    conflictingOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(conflictingOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with multiple sources to fail.";
        api->UnloadModule(conflictingOptions.ModuleName, nullptr);
        return false;
    }
    RemoveTextFile(singleFile);
    RemoveTextFile(multiFileA);
    RemoveTextFile(multiFileB);

    constexpr const char *badModuleName = "__CKAS_ManagerApiBadCompileSelfTest";
    if (api->CompileModule(badModuleName, "int __ckas_bad_compile( {", CKAS_COMPILE_REPLACEEXISTING, &result) !=
        CKAS_COMPILEERROR) {
        error = "CKAngelScript API self-test expected invalid code to return CompileError.";
        api->UnloadModule(badModuleName, nullptr);
        return false;
    }
    if (!ContainsCompileLocation(result)) {
        error = "CKAngelScript API self-test expected compile errors to include AngelScript diagnostics.";
        api->UnloadModule(badModuleName, nullptr);
        return false;
    }
    api->UnloadModule(badModuleName, nullptr);

    IntExecutionData data;
    data.Input = 37;
    CKAngelScriptExecuteOptions executeOptions = CKAngelScriptApi::ExecuteOptions();
    executeOptions.ModuleName = moduleName;
    executeOptions.FunctionDecl = "int __ckas_public_add(int)";
    executeOptions.ConfigureContext = ConfigureIntArgument;
    executeOptions.ReadResult = ReadIntReturn;
    executeOptions.UserData = &data;

    CKAngelScriptExecution *execution = api->CreateExecution(executeOptions, &result);
    if (!execution) {
        error = "CKAngelScript API self-test failed to create an execution handle.";
        return false;
    }
    if (!ExpectStatus(api->StartExecution(execution),
                      CKAS_OK,
                      "StartExecution",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->GetExecutionState(execution) != CKAS_EXECUTION_FINISHED || data.Output != 42) {
        error = "CKAngelScript API self-test returned the wrong synchronous execution result.";
        api->ReleaseExecution(execution);
        return false;
    }
    if (!api->GetExecutionResult(execution) ||
        api->GetExecutionResult(execution)->AngelScriptCode != asEXECUTION_FINISHED) {
        error = "CKAngelScript API self-test expected finished execution to expose the raw AngelScript code.";
        api->ReleaseExecution(execution);
        return false;
    }
    api->ReleaseExecution(execution);

    data.Output = 0;
    CKAngelScriptExecuteOptions extensionOptions = CKAngelScriptApi::ExecuteOptions();
    extensionOptions.ModuleName = moduleName;
    extensionOptions.FunctionName = "__ckas_public_extension";
    extensionOptions.ReadResult = ReadIntReturn;
    extensionOptions.UserData = &data;
    execution = api->CreateExecution(extensionOptions, &result);
    if (!execution) {
        error = "CKAngelScript API self-test failed to create an extension execution handle.";
        return false;
    }
    if (!ExpectStatus(api->StartExecution(execution),
                      CKAS_OK,
                      "StartExecution extension",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->GetExecutionState(execution) != CKAS_EXECUTION_FINISHED || data.Output != 77) {
        error = "CKAngelScript API self-test returned the wrong extension result.";
        api->ReleaseExecution(execution);
        return false;
    }
    api->ReleaseExecution(execution);

    CKAngelScriptExecuteOptions exceptionOptions = CKAngelScriptApi::ExecuteOptions();
    exceptionOptions.ModuleName = moduleName;
    exceptionOptions.FunctionName = "__ckas_public_exception";
    execution = api->CreateExecution(exceptionOptions, &result);
    if (!execution) {
        error = "CKAngelScript API self-test failed to create an exception execution handle.";
        return false;
    }
    if (api->StartExecution(execution) != CKAS_EXECUTIONFAILED) {
        error = "CKAngelScript API self-test expected script exception to fail execution.";
        api->ReleaseExecution(execution);
        return false;
    }
    const CKAngelScriptResult *exceptionResult = api->GetExecutionResult(execution);
    if (!exceptionResult ||
        !exceptionResult->ErrorMessage ||
        !exceptionResult->StackTrace ||
        exceptionResult->AngelScriptCode != asEXECUTION_EXCEPTION) {
        error = "CKAngelScript API self-test expected script exception result details.";
        api->ReleaseExecution(execution);
        return false;
    }
    api->ReleaseExecution(execution);

    CKAngelScriptExecuteOptions asyncOptions = CKAngelScriptApi::ExecuteOptions();
    asyncOptions.ModuleName = moduleName;
    asyncOptions.FunctionName = "__ckas_public_async";
    asyncOptions.ReadResult = ReadIntReturn;
    asyncOptions.UserData = &data;

    data.Output = 0;
    execution = api->CreateExecution(asyncOptions, &result);
    if (!execution) {
        error = "CKAngelScript API self-test failed to create an async execution handle.";
        return false;
    }
    if (!ExpectStatus(api->StartExecution(execution),
                      CKAS_SUSPENDED,
                      "StartExecution async",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->GetExecutionState(execution) != CKAS_EXECUTION_SUSPENDED) {
        error = "CKAngelScript API self-test expected async execution to be suspended.";
        api->ReleaseExecution(execution);
        return false;
    }
    if (!api->GetExecutionResult(execution) ||
        api->GetExecutionResult(execution)->AngelScriptCode != asEXECUTION_SUSPENDED) {
        error = "CKAngelScript API self-test expected suspended execution to expose the raw AngelScript code.";
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->UnloadModule(moduleName, &result) != CKAS_EXECUTIONFAILED) {
        error = "CKAngelScript API self-test expected UnloadModule with an active execution handle to fail.";
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_EXECUTIONFAILED) {
        error = "CKAngelScript API self-test expected replace CompileModule with an active execution handle to fail.";
        api->ReleaseExecution(execution);
        return false;
    }
    if (!ExpectStatus(api->CancelExecution(execution),
                      CKAS_CANCELLED,
                      "CancelExecution",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->GetExecutionState(execution) != CKAS_EXECUTION_CANCELLED) {
        error = "CKAngelScript API self-test expected cancelled execution state.";
        api->ReleaseExecution(execution);
        return false;
    }
    api->ReleaseExecution(execution);

    data.Output = 0;
    execution = api->CreateExecution(asyncOptions, &result);
    if (!execution) {
        error = "CKAngelScript API self-test failed to create a resumable async execution handle.";
        return false;
    }
    if (!ExpectStatus(api->StartExecution(execution),
                      CKAS_SUSPENDED,
                      "StartExecution resumable async",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    ScriptAsyncScheduler *scheduler = ScriptManager::GetManager(context)->GetAsyncScheduler();
    if (scheduler) {
        scheduler->Tick();
    }
    if (!ExpectStatus(api->ResumeExecution(execution),
                      CKAS_OK,
                      "ResumeExecution",
                      api->GetExecutionResult(execution),
                      error)) {
        api->ReleaseExecution(execution);
        return false;
    }
    if (api->GetExecutionState(execution) != CKAS_EXECUTION_FINISHED || data.Output != 9) {
        error = "CKAngelScript API self-test expected resumed async execution to finish with the script result.";
        api->ReleaseExecution(execution);
        return false;
    }
    api->ReleaseExecution(execution);

    CKAngelScriptExecuteOptions missingOptions = CKAngelScriptApi::ExecuteOptions();
    missingOptions.ModuleName = moduleName;
    missingOptions.FunctionName = "__ckas_missing_function";
    execution = api->CreateExecution(missingOptions, &result);
    if (execution || result.Status != CKAS_NOTFOUND) {
        error = "CKAngelScript API self-test expected missing function execution creation to fail.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        return false;
    }

    if (api->GetApiVersion() < CKAS_API_VERSION ||
        !api->HasCapability(CKAS_CAP_MODULE_COMPILE) ||
        !api->HasCapability(CKAS_CAP_OBJECT_CREATE) ||
        !api->HasCapability(CKAS_CAP_OBJECT_METHOD_EXECUTION) ||
        !api->HasCapability(CKAS_CAP_ARG_WRITER) ||
        !api->HasCapability(CKAS_CAP_RESULT_READER) ||
        !api->HasCapability(CKAS_CAP_STACKTRACE)) {
        error = "CKAngelScript API self-test expected the object/method ABI capabilities to be available.";
        return false;
    }
    if (api->HasCapability(CKAS_CAP_ASYNC_RESUME)) {
        error = "CKAngelScript API self-test does not expect object-method async resume capability yet.";
        return false;
    }

    constexpr const char *objectModuleName = "__CKAS_ManagerApiObjectSelfTest";
    const char *objectSource =
        "class __CKAS_PublicObject {\n"
        "  int base;\n"
        "  __CKAS_PublicObject() { base = 10; }\n"
        "  int Add(int value) { return base + value; }\n"
        "  bool Flip(bool value) { return !value; }\n"
        "  float Half(float value) { return value * 0.5f; }\n"
        "  string Echo(const string &in value) { return \"echo:\" + value; }\n"
        "  int Wait() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 5; }\n"
        "  void Nothing() {}\n"
        "  void Boom() { array<int> values; values[1] = 1; }\n"
        "}\n";
    if (!ExpectStatus(api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result),
                      CKAS_OK,
                      "CompileModule object ABI",
                      &result,
                      error)) {
        return false;
    }
    const CKDWORD objectGeneration = api->GetModuleGeneration(objectModuleName);
    if (objectGeneration == 0) {
        error = "CKAngelScript API self-test expected compiled object module to have a generation.";
        return false;
    }

    CKAngelScriptObjectOptions objectOptions = CKAngelScriptApi::ObjectOptions();
    objectOptions.ModuleName = objectModuleName;
    objectOptions.ClassName = "__CKAS_PublicObject";
    CKAngelScriptObject *object = api->CreateObject(objectOptions, &result);
    if (!object) {
        error = "CKAngelScript API self-test failed to create an object handle.";
        return false;
    }

    CKAngelScriptMethodOptions missingOptionalOptions = CKAngelScriptApi::MethodOptions();
    missingOptionalOptions.Object = object;
    missingOptionalOptions.MethodDecl = "void Missing()";
    missingOptionalOptions.Optional = TRUE;
    CKAngelScriptMethod *missingOptional = api->FindObjectMethod(missingOptionalOptions, &result);
    if (missingOptional || result.Status != CKAS_OK) {
        error = "CKAngelScript API self-test expected optional missing object method lookup to return null with OK.";
        if (missingOptional) {
            api->ReleaseMethod(missingOptional);
        }
        api->ReleaseObject(object);
        return false;
    }

    CKAngelScriptMethodOptions missingRequiredOptions = CKAngelScriptApi::MethodOptions();
    missingRequiredOptions.Object = object;
    missingRequiredOptions.MethodDecl = "void Missing()";
    if (api->FindObjectMethod(missingRequiredOptions, &result) || result.Status != CKAS_NOTFOUND) {
        error = "CKAngelScript API self-test expected required missing object method lookup to fail.";
        api->ReleaseObject(object);
        return false;
    }

    CKAngelScriptMethodOptions addOptions = CKAngelScriptApi::MethodOptions();
    addOptions.Object = object;
    addOptions.MethodDecl = "int Add(int)";
    CKAngelScriptMethod *addMethod = api->FindObjectMethod(addOptions, &result);
    if (!addMethod) {
        error = "CKAngelScript API self-test failed to find Add object method.";
        api->ReleaseObject(object);
        return false;
    }

    ObjectExecutionData objectData;
    objectData.IntInput = 32;
    CKAngelScriptObjectMethodExecuteOptions objectCall = CKAngelScriptApi::ObjectMethodExecuteOptions();
    objectCall.Object = object;
    objectCall.Method = addMethod;
    objectCall.WriteArgs = WriteObjectInt;
    objectCall.ReadResult = ReadObjectInt;
    objectCall.UserData = &objectData;
    objectCall.Flags = CKAS_CALL_NO_SUSPEND | CKAS_CALL_HOT_PATH;
    if (!ExpectStatus(api->CallObjectMethod(objectCall, &result),
                      CKAS_OK,
                      "CallObjectMethod int",
                      &result,
                      error)) {
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    if (objectData.IntOutput != 42) {
        error = "CKAngelScript API self-test expected object int result to be 42.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }

    objectCall.WriteArgs = WriteObjectIntAsBool;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_TYPEMISMATCH) {
        error = "CKAngelScript API self-test expected object arg writer type mismatch.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.WriteArgs = WriteObjectInt;

    CKAngelScriptExecution *objectExecution = api->CreateObjectMethodExecution(objectCall, &result);
    if (!objectExecution) {
        error = "CKAngelScript API self-test failed to create an object method execution handle.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectData.IntOutput = 0;
    if (!ExpectStatus(api->StartExecution(objectExecution),
                      CKAS_OK,
                      "StartExecution object method",
                      api->GetExecutionResult(objectExecution),
                      error)) {
        api->ReleaseExecution(objectExecution);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    if (objectData.IntOutput != 42) {
        error = "CKAngelScript API self-test expected object method execution result to be 42.";
        api->ReleaseExecution(objectExecution);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseExecution(objectExecution);
    api->ReleaseMethod(addMethod);

    CKAngelScriptMethodOptions waitOptions = CKAngelScriptApi::MethodOptions();
    waitOptions.Object = object;
    waitOptions.MethodDecl = "int Wait()";
    CKAngelScriptMethod *waitMethod = api->FindObjectMethod(waitOptions, &result);
    objectCall.Method = waitMethod;
    objectCall.WriteArgs = nullptr;
    objectCall.ReadResult = nullptr;
    objectExecution = waitMethod ? api->CreateObjectMethodExecution(objectCall, &result) : nullptr;
    if (!waitMethod || !objectExecution || api->StartExecution(objectExecution) != CKAS_UNSUPPORTED) {
        error = "CKAngelScript API self-test expected suspended object method execution to be unsupported.";
        if (objectExecution) {
            api->ReleaseExecution(objectExecution);
        }
        if (waitMethod) {
            api->ReleaseMethod(waitMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseExecution(objectExecution);
    api->ReleaseMethod(waitMethod);

    CKAngelScriptMethodOptions boolOptions = CKAngelScriptApi::MethodOptions();
    boolOptions.Object = object;
    boolOptions.MethodDecl = "bool Flip(bool)";
    CKAngelScriptMethod *boolMethod = api->FindObjectMethod(boolOptions, &result);
    objectData.BoolInput = TRUE;
    objectData.BoolOutput = TRUE;
    objectCall.Method = boolMethod;
    objectCall.WriteArgs = WriteObjectBool;
    objectCall.ReadResult = ReadObjectBool;
    if (!boolMethod || api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.BoolOutput != FALSE) {
        error = "CKAngelScript API self-test expected bool object method round-trip.";
        if (boolMethod) {
            api->ReleaseMethod(boolMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(boolMethod);

    CKAngelScriptMethodOptions floatOptions = CKAngelScriptApi::MethodOptions();
    floatOptions.Object = object;
    floatOptions.MethodDecl = "float Half(float)";
    CKAngelScriptMethod *floatMethod = api->FindObjectMethod(floatOptions, &result);
    objectData.FloatInput = 5.0f;
    objectData.FloatOutput = 0.0f;
    objectCall.Method = floatMethod;
    objectCall.WriteArgs = WriteObjectFloat;
    objectCall.ReadResult = ReadObjectFloat;
    if (!floatMethod || api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.FloatOutput != 2.5f) {
        error = "CKAngelScript API self-test expected float object method round-trip.";
        if (floatMethod) {
            api->ReleaseMethod(floatMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(floatMethod);

    CKAngelScriptMethodOptions echoOptions = CKAngelScriptApi::MethodOptions();
    echoOptions.Object = object;
    echoOptions.MethodDecl = "string Echo(const string &in)";
    CKAngelScriptMethod *echoMethod = api->FindObjectMethod(echoOptions, &result);
    objectData.StringInput = "hello";
    objectData.StringOutput[0] = '\0';
    objectData.RequiredSize = 0;
    objectCall.Method = echoMethod;
    objectCall.WriteArgs = WriteObjectString;
    objectCall.ReadResult = ReadObjectStringTooSmall;
    if (!echoMethod || api->CallObjectMethod(objectCall, &result) != CKAS_BUFFERTOOSMALL || objectData.RequiredSize == 0) {
        error = "CKAngelScript API self-test expected string result buffer-too-small diagnostics.";
        if (echoMethod) {
            api->ReleaseMethod(echoMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    objectCall.ReadResult = ReadObjectString;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK || std::string(objectData.StringOutput) != "echo:hello") {
        error = "CKAngelScript API self-test expected string object method round-trip.";
        api->ReleaseMethod(echoMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(echoMethod);

    CKAngelScriptMethodOptions boomOptions = CKAngelScriptApi::MethodOptions();
    boomOptions.Object = object;
    boomOptions.MethodDecl = "void Boom()";
    CKAngelScriptMethod *boomMethod = api->FindObjectMethod(boomOptions, &result);
    objectCall.Method = boomMethod;
    objectCall.WriteArgs = nullptr;
    objectCall.ReadResult = nullptr;
    if (!boomMethod || api->CallObjectMethod(objectCall, &result) != CKAS_EXECUTIONFAILED ||
        !result.ErrorMessage || !result.StackTrace) {
        error = "CKAngelScript API self-test expected object method exceptions to include diagnostics.";
        if (boomMethod) {
            api->ReleaseMethod(boomMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(boomMethod);

    CKAngelScriptMethodOptions staleOptions = CKAngelScriptApi::MethodOptions();
    staleOptions.Object = object;
    staleOptions.MethodDecl = "int Add(int)";
    CKAngelScriptMethod *staleMethod = api->FindObjectMethod(staleOptions, &result);
    objectCall.Method = staleMethod;
    objectCall.WriteArgs = WriteObjectInt;
    objectCall.ReadResult = ReadObjectInt;
    if (!staleMethod ||
        !ExpectStatus(api->UnloadModule(objectModuleName, &result),
                      CKAS_OK,
                      "UnloadModule object ABI",
                      &result,
                      error)) {
        if (staleMethod) {
            api->ReleaseMethod(staleMethod);
        }
        api->ReleaseObject(object);
        return false;
    }
    if (api->CallObjectMethod(objectCall, &result) != CKAS_STALEHANDLE ||
        api->GetModuleGeneration(objectModuleName) == objectGeneration) {
        error = "CKAngelScript API self-test expected object/method handles to become stale after unload.";
        api->ReleaseMethod(staleMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(staleMethod);
    api->ReleaseObject(object);

    if (!ExpectStatus(api->UnloadModule(moduleName, &result),
                      CKAS_OK,
                      "UnloadModule",
                      &result,
                      error)) {
        return false;
    }
    if (api->HasModule(moduleName)) {
        error = "CKAngelScript API self-test expected unloaded module to be absent.";
        return false;
    }
    if (!ExpectStatus(api->UnregisterEngineExtension("__ckas_public_api_extension", nullptr, &result),
                      CKAS_OK,
                      "UnregisterEngineExtension",
                      &result,
                      error)) {
        return false;
    }
    return true;
}
