#ifndef CK_SCRIPT_PUBLIC_OPTIONS_H
#define CK_SCRIPT_PUBLIC_OPTIONS_H

#include <string>
#include <tuple>
#include <vector>

#include "CKAngelScript.h"

namespace ScriptPublicOptions {

enum class LoadSourceKind {
    DefaultOrFile,
    Code,
    File,
    Files,
    Sections
};

struct LoadModuleRequest {
    const char *ModuleName = nullptr;
    const char *Filename = nullptr;
    const char **Filenames = nullptr;
    size_t FileCount = 0;
    const char *Code = nullptr;
    CKDWORD Flags = CKAS_LOAD_DEFAULT;
    LoadSourceKind SourceKind = LoadSourceKind::DefaultOrFile;
    std::vector<std::tuple<std::string, std::string>> Sections;
};

struct ImportBindRequest {
    const char *ImportModuleName = nullptr;
    CKDWORD ImportIndex = 0;
    const char *SourceModuleName = nullptr;
    const char *FunctionDecl = nullptr;
    CKDWORD Flags = 0;
};

struct BytecodeSaveRequest {
    const char *ModuleName = nullptr;
    CKAngelScriptBytecodeWriteCallback Write = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_BYTECODE_DEFAULT;
    bool StripDebugInfo = false;
};

struct BytecodeLoadRequest {
    const char *ModuleName = nullptr;
    CKAngelScriptBytecodeReadCallback Read = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_BYTECODE_DEFAULT;
};

struct FunctionRequest {
    const char *ModuleName = nullptr;
    const char *FunctionName = nullptr;
    const char *FunctionDecl = nullptr;
    CKDWORD Flags = 0;
    bool HasFunctionDecl = false;
};

struct ObjectRequest {
    const char *ModuleName = nullptr;
    const char *ClassName = nullptr;
    const char *ClassNamespace = nullptr;
    const char *TypeDecl = nullptr;
    bool HasTypeDecl = false;
};

struct MethodRequest {
    CKAngelScriptObject *Object = nullptr;
    const char *MethodName = nullptr;
    const char *MethodDecl = nullptr;
    bool HasMethodDecl = false;
};

struct ObjectMethodExecuteRequest {
    CKAngelScriptObject *Object = nullptr;
    CKAngelScriptMethod *Method = nullptr;
    CKAngelScriptWriteArgsCallback WriteArgs = nullptr;
    CKAngelScriptReadResultCallback ReadResult = nullptr;
    CKAngelScriptContextCallback ConfigureContext = nullptr;
    CKAngelScriptContextCallback ReadContextResult = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_CALL_DEFAULT;
};

struct FunctionExecutionRequest {
    CKAngelScriptFunction *Function = nullptr;
    const CKBehaviorContext *BehaviorContext = nullptr;
    CKDWORD Flags = CKAS_CALL_DEFAULT;
};

struct EngineExtensionRequest {
    const char *Name = nullptr;
    CKAngelScriptEngineExtensionCallback Register = nullptr;
    void *UserData = nullptr;
    CKDWORD Flags = CKAS_ENGINEEXTENSION_DEFAULT;
};

CKAS_STATUS DecodeLoadOptions(const CKAngelScriptLoadOptions &options,
                              LoadModuleRequest &request,
                              std::string &errorMessage);
CKAS_STATUS DecodeImportBindOptions(const CKAngelScriptImportBindOptions &options,
                                    ImportBindRequest &request,
                                    std::string &errorMessage);
CKAS_STATUS DecodeBytecodeSaveOptions(const CKAngelScriptBytecodeSaveOptions &options,
                                      BytecodeSaveRequest &request,
                                      std::string &errorMessage);
CKAS_STATUS DecodeBytecodeLoadOptions(const CKAngelScriptBytecodeLoadOptions &options,
                                      BytecodeLoadRequest &request,
                                      std::string &errorMessage);
CKAS_STATUS DecodeFunctionOptions(const CKAngelScriptFunctionOptions &options,
                                  FunctionRequest &request,
                                  std::string &errorMessage);
CKAS_STATUS DecodeObjectOptions(const CKAngelScriptObjectOptions &options,
                                ObjectRequest &request,
                                std::string &errorMessage);
CKAS_STATUS DecodeMethodOptions(const CKAngelScriptMethodOptions &options,
                                MethodRequest &request,
                                std::string &errorMessage);
CKAS_STATUS DecodeObjectMethodExecuteOptions(const CKAngelScriptObjectMethodExecuteOptions &options,
                                             ObjectMethodExecuteRequest &request,
                                             std::string &errorMessage);
CKAS_STATUS DecodeFunctionExecutionOptions(const CKAngelScriptFunctionExecutionOptions &options,
                                           FunctionExecutionRequest &request,
                                           std::string &errorMessage);
CKAS_STATUS ValidateExecutionStepOptions(const CKAngelScriptExecutionStepOptions *options,
                                         const char *apiName,
                                         std::string &errorMessage);
CKAS_STATUS DecodeEngineExtension(const CKAngelScriptEngineExtension &extension,
                                  EngineExtensionRequest &request,
                                  std::string &errorMessage);
CKAS_STATUS ValidateModuleFingerprintOutput(const CKAngelScriptModuleFingerprint *fingerprint,
                                            std::string &errorMessage);

} // namespace ScriptPublicOptions

#endif // CK_SCRIPT_PUBLIC_OPTIONS_H
