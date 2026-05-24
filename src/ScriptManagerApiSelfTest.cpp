#include "ScriptSelfTests.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "AngelScriptManager.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"

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
        error = "AngelScriptManager API self-test failed to create temporary script file.";
        return false;
    }
    out << (text ? text : "");
    return true;
}

void RemoveTextFile(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool ContainsCompileLocation(const AngelScriptResult &result) {
    if (!result.ErrorMessage || result.ErrorMessage[0] == '\0') {
        return false;
    }
    const std::string message = result.ErrorMessage;
    return message.find('(') != std::string::npos &&
           message.find(')') != std::string::npos &&
           message.find("ERROR") != std::string::npos;
}

bool ExpectStatus(AngelScriptStatus actual,
                  AngelScriptStatus expected,
                  const char *label,
                  const AngelScriptResult *result,
                  std::string &error) {
    if (actual == expected) {
        return true;
    }
    error = std::string(label ? label : "AngelScriptManager API") + " returned unexpected status.";
    if (result && result->ErrorMessage && result->ErrorMessage[0] != '\0') {
        error += " ";
        error += result->ErrorMessage;
    }
    return false;
}

} // namespace

bool RunScriptManagerApiSelfTest(CKContext *context, std::string &error) {
    if (!context) {
        error = "AngelScriptManager API self-test requires a CKContext.";
        return false;
    }

    AngelScriptManager *manager = AngelScriptManager::GetManager(context);
    if (!manager) {
        error = "AngelScriptManager API self-test could not retrieve the public manager.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_ManagerApiSelfTest";
    const char *source =
        "int __ckas_public_add(int value) { return value + 5; }\n"
        "int __ckas_public_async() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 9; }\n"
        "int __ckas_public_exception() { array<int> values; return values[1]; }\n";

    AngelScriptResult result = {};
    if (!ExpectStatus(manager->CompileModule(moduleName, source, true, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "CompileModule",
                      &result,
                      error)) {
        return false;
    }

    if (!manager->HasModule(moduleName) || !manager->GetModule(moduleName)) {
        error = "AngelScriptManager API self-test could not query the compiled module.";
        return false;
    }
    if (!manager->FindFunctionByName(moduleName, "__ckas_public_add") ||
        !manager->FindFunctionByDecl(moduleName, "int __ckas_public_add(int)")) {
        error = "AngelScriptManager API self-test could not query compiled functions.";
        return false;
    }

    if (manager->CompileModule(moduleName, source, false, &result) == ANGELSCRIPT_STATUS_OK) {
        error = "AngelScriptManager API self-test expected duplicate CompileModule without replace to fail.";
        return false;
    }
    if (!manager->GetLastResult() || manager->GetLastResult()->Status == ANGELSCRIPT_STATUS_OK) {
        error = "AngelScriptManager API self-test expected duplicate CompileModule to update LastResult.";
        return false;
    }

    constexpr const char *loadModuleName = "__CKAS_ManagerApiLoadSelfTest";
    AngelScriptLoadOptions loadOptions = {};
    loadOptions.ModuleName = loadModuleName;
    loadOptions.Code = "int __ckas_public_loaded() { return 3; }\n";
    loadOptions.ReplaceExisting = true;
    if (!ExpectStatus(manager->LoadModule(loadOptions, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "LoadModule code",
                      &result,
                      error)) {
        return false;
    }
    if (!manager->FindFunctionByName(loadModuleName, "__ckas_public_loaded")) {
        error = "AngelScriptManager API self-test could not query LoadModule code output.";
        return false;
    }
    manager->UnloadModule(loadModuleName, nullptr);

    const std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    const std::filesystem::path singleFile = tempDir / "__ckas_manager_api_single.as";
    const std::filesystem::path multiFileA = tempDir / "__ckas_manager_api_multi_a.as";
    const std::filesystem::path multiFileB = tempDir / "__ckas_manager_api_multi_b.as";
    const std::filesystem::path defaultFile = std::filesystem::current_path() / "__CKAS_ManagerApiDefaultFileLoadSelfTest.as";
    if (!WriteTextFile(singleFile, "int __ckas_public_file_loaded() { return 11; }\n", error) ||
        !WriteTextFile(multiFileA, "int __ckas_public_multi_a() { return 12; }\n", error) ||
        !WriteTextFile(multiFileB, "int __ckas_public_multi_b() { return __ckas_public_multi_a() + 1; }\n", error) ||
        !WriteTextFile(defaultFile, "int __ckas_public_default_file_loaded() { return 14; }\n", error)) {
        return false;
    }

    constexpr const char *singleFileModuleName = "__CKAS_ManagerApiSingleFileLoadSelfTest";
    AngelScriptLoadOptions singleFileOptions = {};
    singleFileOptions.ModuleName = singleFileModuleName;
    const std::string singleFilePath = singleFile.string();
    singleFileOptions.Filename = singleFilePath.c_str();
    singleFileOptions.ReplaceExisting = true;
    if (!ExpectStatus(manager->LoadModule(singleFileOptions, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "LoadModule single file",
                      &result,
                      error)) {
        return false;
    }
    if (!manager->FindFunctionByName(singleFileModuleName, "__ckas_public_file_loaded")) {
        error = "AngelScriptManager API self-test could not query LoadModule single-file output.";
        return false;
    }
    manager->UnloadModule(singleFileModuleName, nullptr);

    constexpr const char *multiFileModuleName = "__CKAS_ManagerApiMultiFileLoadSelfTest";
    const std::string multiFilePathA = multiFileA.string();
    const std::string multiFilePathB = multiFileB.string();
    const char *multiFiles[] = { multiFilePathA.c_str(), multiFilePathB.c_str() };
    AngelScriptLoadOptions multiFileOptions = {};
    multiFileOptions.ModuleName = multiFileModuleName;
    multiFileOptions.Filenames = multiFiles;
    multiFileOptions.FileCount = 2;
    multiFileOptions.ReplaceExisting = true;
    if (!ExpectStatus(manager->LoadModule(multiFileOptions, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "LoadModule multi file",
                      &result,
                      error)) {
        return false;
    }
    if (!manager->FindFunctionByName(multiFileModuleName, "__ckas_public_multi_b")) {
        error = "AngelScriptManager API self-test could not query LoadModule multi-file output.";
        return false;
    }
    manager->UnloadModule(multiFileModuleName, nullptr);

    constexpr const char *defaultFileModuleName = "__CKAS_ManagerApiDefaultFileLoadSelfTest";
    AngelScriptLoadOptions defaultFileOptions = {};
    defaultFileOptions.ModuleName = defaultFileModuleName;
    defaultFileOptions.ReplaceExisting = true;
    if (!ExpectStatus(manager->LoadModule(defaultFileOptions, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "LoadModule default file",
                      &result,
                      error)) {
        RemoveTextFile(defaultFile);
        return false;
    }
    if (!manager->FindFunctionByName(defaultFileModuleName, "__ckas_public_default_file_loaded")) {
        error = "AngelScriptManager API self-test could not query LoadModule default-file output.";
        manager->UnloadModule(defaultFileModuleName, nullptr);
        RemoveTextFile(defaultFile);
        return false;
    }
    manager->UnloadModule(defaultFileModuleName, nullptr);
    RemoveTextFile(defaultFile);

    AngelScriptLoadOptions missingFileOptions = {};
    missingFileOptions.ModuleName = "__CKAS_ManagerApiMissingFileLoadSelfTest";
    const std::string missingFilePath = (tempDir / "__ckas_manager_api_missing_file.as").string();
    missingFileOptions.Filename = missingFilePath.c_str();
    missingFileOptions.ReplaceExisting = true;
    if (manager->LoadModule(missingFileOptions, &result) != ANGELSCRIPT_STATUS_COMPILE_ERROR ||
        !result.ErrorMessage ||
        result.ErrorMessage[0] == '\0') {
        error = "AngelScriptManager API self-test expected missing file LoadModule to return diagnostics.";
        manager->UnloadModule(missingFileOptions.ModuleName, nullptr);
        return false;
    }

    AngelScriptLoadOptions conflictingOptions = {};
    conflictingOptions.ModuleName = "__CKAS_ManagerApiConflictingLoadSelfTest";
    conflictingOptions.Code = "int __ckas_public_conflict() { return 1; }\n";
    conflictingOptions.Filename = singleFilePath.c_str();
    conflictingOptions.ReplaceExisting = true;
    if (manager->LoadModule(conflictingOptions, &result) != ANGELSCRIPT_STATUS_INVALID_ARGUMENT) {
        error = "AngelScriptManager API self-test expected LoadModule with multiple sources to fail.";
        manager->UnloadModule(conflictingOptions.ModuleName, nullptr);
        return false;
    }
    RemoveTextFile(singleFile);
    RemoveTextFile(multiFileA);
    RemoveTextFile(multiFileB);

    constexpr const char *badModuleName = "__CKAS_ManagerApiBadCompileSelfTest";
    if (manager->CompileModule(badModuleName, "int __ckas_bad_compile( {", true, &result) !=
        ANGELSCRIPT_STATUS_COMPILE_ERROR) {
        error = "AngelScriptManager API self-test expected invalid code to return CompileError.";
        manager->UnloadModule(badModuleName, nullptr);
        return false;
    }
    if (!ContainsCompileLocation(result)) {
        error = "AngelScriptManager API self-test expected compile errors to include AngelScript diagnostics.";
        manager->UnloadModule(badModuleName, nullptr);
        return false;
    }
    manager->UnloadModule(badModuleName, nullptr);

    IntExecutionData data;
    data.Input = 37;
    AngelScriptExecuteOptions executeOptions = {};
    executeOptions.ModuleName = moduleName;
    executeOptions.FunctionDecl = "int __ckas_public_add(int)";
    executeOptions.ConfigureContext = ConfigureIntArgument;
    executeOptions.ReadResult = ReadIntReturn;
    executeOptions.UserData = &data;

    AngelScriptExecution *execution = manager->CreateExecution(executeOptions, &result);
    if (!execution) {
        error = "AngelScriptManager API self-test failed to create an execution handle.";
        return false;
    }
    if (!ExpectStatus(manager->StartExecution(execution),
                      ANGELSCRIPT_STATUS_OK,
                      "StartExecution",
                      manager->GetExecutionResult(execution),
                      error)) {
        manager->ReleaseExecution(execution);
        return false;
    }
    if (manager->GetExecutionState(execution) != ANGELSCRIPT_EXECUTION_FINISHED || data.Output != 42) {
        error = "AngelScriptManager API self-test returned the wrong synchronous execution result.";
        manager->ReleaseExecution(execution);
        return false;
    }
    if (!manager->GetExecutionResult(execution) ||
        manager->GetExecutionResult(execution)->AngelScriptCode != asEXECUTION_FINISHED) {
        error = "AngelScriptManager API self-test expected finished execution to expose the raw AngelScript code.";
        manager->ReleaseExecution(execution);
        return false;
    }
    manager->ReleaseExecution(execution);

    AngelScriptExecuteOptions exceptionOptions = {};
    exceptionOptions.ModuleName = moduleName;
    exceptionOptions.FunctionName = "__ckas_public_exception";
    execution = manager->CreateExecution(exceptionOptions, &result);
    if (!execution) {
        error = "AngelScriptManager API self-test failed to create an exception execution handle.";
        return false;
    }
    if (manager->StartExecution(execution) != ANGELSCRIPT_STATUS_EXECUTION_FAILED) {
        error = "AngelScriptManager API self-test expected script exception to fail execution.";
        manager->ReleaseExecution(execution);
        return false;
    }
    const AngelScriptResult *exceptionResult = manager->GetExecutionResult(execution);
    if (!exceptionResult ||
        !exceptionResult->ErrorMessage ||
        !exceptionResult->StackTrace ||
        exceptionResult->AngelScriptCode != asEXECUTION_EXCEPTION) {
        error = "AngelScriptManager API self-test expected script exception result details.";
        manager->ReleaseExecution(execution);
        return false;
    }
    manager->ReleaseExecution(execution);

    AngelScriptExecuteOptions asyncOptions = {};
    asyncOptions.ModuleName = moduleName;
    asyncOptions.FunctionName = "__ckas_public_async";
    asyncOptions.ReadResult = ReadIntReturn;
    asyncOptions.UserData = &data;

    data.Output = 0;
    execution = manager->CreateExecution(asyncOptions, &result);
    if (!execution) {
        error = "AngelScriptManager API self-test failed to create an async execution handle.";
        return false;
    }
    if (!ExpectStatus(manager->StartExecution(execution),
                      ANGELSCRIPT_STATUS_SUSPENDED,
                      "StartExecution async",
                      manager->GetExecutionResult(execution),
                      error)) {
        manager->ReleaseExecution(execution);
        return false;
    }
    if (manager->GetExecutionState(execution) != ANGELSCRIPT_EXECUTION_SUSPENDED) {
        error = "AngelScriptManager API self-test expected async execution to be suspended.";
        manager->ReleaseExecution(execution);
        return false;
    }
    if (!manager->GetExecutionResult(execution) ||
        manager->GetExecutionResult(execution)->AngelScriptCode != asEXECUTION_SUSPENDED) {
        error = "AngelScriptManager API self-test expected suspended execution to expose the raw AngelScript code.";
        manager->ReleaseExecution(execution);
        return false;
    }
    if (!ExpectStatus(manager->CancelExecution(execution),
                      ANGELSCRIPT_STATUS_CANCELLED,
                      "CancelExecution",
                      manager->GetExecutionResult(execution),
                      error)) {
        manager->ReleaseExecution(execution);
        return false;
    }
    if (manager->GetExecutionState(execution) != ANGELSCRIPT_EXECUTION_CANCELLED) {
        error = "AngelScriptManager API self-test expected cancelled execution state.";
        manager->ReleaseExecution(execution);
        return false;
    }
    manager->ReleaseExecution(execution);

    data.Output = 0;
    execution = manager->CreateExecution(asyncOptions, &result);
    if (!execution) {
        error = "AngelScriptManager API self-test failed to create a resumable async execution handle.";
        return false;
    }
    if (!ExpectStatus(manager->StartExecution(execution),
                      ANGELSCRIPT_STATUS_SUSPENDED,
                      "StartExecution resumable async",
                      manager->GetExecutionResult(execution),
                      error)) {
        manager->ReleaseExecution(execution);
        return false;
    }
    ScriptAsyncScheduler *scheduler = ScriptManager::GetManager(context)->GetAsyncScheduler();
    if (scheduler) {
        scheduler->Tick();
    }
    if (!ExpectStatus(manager->ResumeExecution(execution),
                      ANGELSCRIPT_STATUS_OK,
                      "ResumeExecution",
                      manager->GetExecutionResult(execution),
                      error)) {
        manager->ReleaseExecution(execution);
        return false;
    }
    if (manager->GetExecutionState(execution) != ANGELSCRIPT_EXECUTION_FINISHED || data.Output != 9) {
        error = "AngelScriptManager API self-test expected resumed async execution to finish with the script result.";
        manager->ReleaseExecution(execution);
        return false;
    }
    manager->ReleaseExecution(execution);

    AngelScriptExecuteOptions missingOptions = {};
    missingOptions.ModuleName = moduleName;
    missingOptions.FunctionName = "__ckas_missing_function";
    execution = manager->CreateExecution(missingOptions, &result);
    if (execution || result.Status != ANGELSCRIPT_STATUS_NOT_FOUND) {
        error = "AngelScriptManager API self-test expected missing function execution creation to fail.";
        if (execution) {
            manager->ReleaseExecution(execution);
        }
        return false;
    }

    if (!ExpectStatus(manager->UnloadModule(moduleName, &result),
                      ANGELSCRIPT_STATUS_OK,
                      "UnloadModule",
                      &result,
                      error)) {
        return false;
    }
    if (manager->HasModule(moduleName)) {
        error = "AngelScriptManager API self-test expected unloaded module to be absent.";
        return false;
    }
    return true;
}
