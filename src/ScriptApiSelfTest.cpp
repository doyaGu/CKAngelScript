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
