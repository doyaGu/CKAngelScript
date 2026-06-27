#include "ScriptPublicOptions.h"

#include <string>
#include <unordered_set>

#include "ScriptApiSupport.h"
#include "ScriptSourcePaths.h"

namespace ScriptPublicOptions {

namespace {

CKAS_STATUS Fail(std::string &errorMessage, const char *message) {
    errorMessage = message ? message : "";
    return CKAS_INVALIDARGUMENT;
}

CKAS_STATUS ValidateNoUnknownFlags(CKDWORD flags,
                                   CKDWORD knownFlags,
                                   const char *message,
                                   std::string &errorMessage) {
    if (ScriptApiSupport::HasUnknownPublicFlags(flags, knownFlags)) {
        return Fail(errorMessage, message);
    }
    return CKAS_OK;
}

} // namespace

CKAS_STATUS DecodeLoadOptions(const CKAngelScriptLoadOptions &options,
                              LoadModuleRequest &request,
                              std::string &errorMessage) {
    request = LoadModuleRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "LoadModule options size is invalid.");
    }
    request.ModuleName = ScriptApiSupport::PublicField(options,
                                                       &CKAngelScriptLoadOptions::ModuleName,
                                                       static_cast<const char *>(nullptr));
    request.Filename = ScriptApiSupport::PublicField(options,
                                                     &CKAngelScriptLoadOptions::Filename,
                                                     static_cast<const char *>(nullptr));
    request.Filenames = ScriptApiSupport::PublicField(options,
                                                      &CKAngelScriptLoadOptions::Filenames,
                                                      static_cast<const char **>(nullptr));
    request.FileCount = ScriptApiSupport::PublicField(options,
                                                      &CKAngelScriptLoadOptions::FileCount,
                                                      static_cast<size_t>(0));
    request.Code = ScriptApiSupport::PublicField(options,
                                                 &CKAngelScriptLoadOptions::Code,
                                                 static_cast<const char *>(nullptr));
    const CKAngelScriptSourceSection *sourceSections =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptLoadOptions::Sections,
                                      static_cast<const CKAngelScriptSourceSection *>(nullptr));
    const size_t sourceSectionCount =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptLoadOptions::SectionCount,
                                      static_cast<size_t>(0));
    request.Flags = ScriptApiSupport::PublicField(options,
                                                  &CKAngelScriptLoadOptions::Flags,
                                                  static_cast<CKDWORD>(CKAS_LOAD_DEFAULT));

    if (!ScriptApiSupport::IsNonEmpty(request.ModuleName)) {
        return Fail(errorMessage, "Module name is required.");
    }
    CKAS_STATUS status = ValidateNoUnknownFlags(request.Flags,
                                               CKAS_LOAD_REPLACEEXISTING,
                                               "Unknown LoadModule flags.",
                                               errorMessage);
    if (status != CKAS_OK) {
        return status;
    }

    const bool hasCode = request.Code != nullptr;
    const bool hasFile = ScriptApiSupport::IsNonEmpty(request.Filename);
    const bool hasFiles = request.FileCount > 0;
    const bool hasSourceSections = sourceSectionCount > 0;
    if (request.Filenames && request.FileCount == 0) {
        return Fail(errorMessage, "File list count is zero.");
    }
    if (sourceSections && sourceSectionCount == 0) {
        return Fail(errorMessage, "Source section count is zero.");
    }
    const int sourceCount = (hasCode ? 1 : 0) + (hasFile ? 1 : 0) +
                            (hasFiles ? 1 : 0) + (hasSourceSections ? 1 : 0);
    if (sourceCount > 1) {
        return Fail(errorMessage,
                    "LoadModule accepts only one source: Code, Filename, Filenames, or Sections.");
    }

    if (hasCode) {
        request.SourceKind = LoadSourceKind::Code;
        return CKAS_OK;
    }
    if (hasFile) {
        request.SourceKind = LoadSourceKind::File;
        return CKAS_OK;
    }
    if (hasFiles) {
        request.SourceKind = LoadSourceKind::Files;
        if (!request.Filenames) {
            return Fail(errorMessage, "File list is null.");
        }
        for (size_t i = 0; i < request.FileCount; ++i) {
            if (!ScriptApiSupport::IsNonEmpty(request.Filenames[i])) {
                return Fail(errorMessage, "File list contains an empty filename.");
            }
        }
        return CKAS_OK;
    }
    if (hasSourceSections) {
        request.SourceKind = LoadSourceKind::Sections;
        if (!sourceSections) {
            return Fail(errorMessage, "Source section list is null.");
        }
        request.Sections.reserve(sourceSectionCount);
        std::unordered_set<std::string> sectionNames;
        for (size_t i = 0; i < sourceSectionCount; ++i) {
            const CKAngelScriptSourceSection &sourceSection = sourceSections[i];
            if (!ScriptApiSupport::HasCompletePublicStruct(sourceSection)) {
                return Fail(errorMessage, "Source section options size is invalid.");
            }
            const char *sectionName =
                ScriptApiSupport::PublicField(sourceSection,
                                              &CKAngelScriptSourceSection::SectionName,
                                              static_cast<const char *>(nullptr));
            const char *sectionCode =
                ScriptApiSupport::PublicField(sourceSection,
                                              &CKAngelScriptSourceSection::Code,
                                              static_cast<const char *>(nullptr));
            const size_t sectionSize =
                ScriptApiSupport::PublicField(sourceSection,
                                              &CKAngelScriptSourceSection::CodeSize,
                                              static_cast<size_t>(0));
            if (!ScriptApiSupport::IsNonEmpty(sectionName)) {
                return Fail(errorMessage, "Source section list contains an empty section name.");
            }
            if (!sectionCode) {
                return Fail(errorMessage, "Source section list contains null code.");
            }
            const std::string sectionKey = ScriptSourcePaths::NormalizeSectionName(sectionName);
            if (!sectionNames.insert(sectionKey).second) {
                return Fail(errorMessage, "Source section list contains duplicate section names.");
            }
            request.Sections.emplace_back(sectionName,
                                          sectionSize == 0
                                              ? std::string(sectionCode)
                                              : std::string(sectionCode, sectionSize));
        }
    }
    return CKAS_OK;
}

CKAS_STATUS DecodeImportBindOptions(const CKAngelScriptImportBindOptions &options,
                                    ImportBindRequest &request,
                                    std::string &errorMessage) {
    request = ImportBindRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "Import bind options size is invalid.");
    }
    request.ImportModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptImportBindOptions::ImportModuleName,
                                      static_cast<const char *>(nullptr));
    request.ImportIndex =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptImportBindOptions::ImportIndex,
                                      static_cast<CKDWORD>(0));
    request.SourceModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptImportBindOptions::SourceModuleName,
                                      static_cast<const char *>(nullptr));
    request.FunctionDecl =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptImportBindOptions::FunctionDecl,
                                      static_cast<const char *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptImportBindOptions::Flags,
                                      static_cast<CKDWORD>(0));
    return ValidateNoUnknownFlags(request.Flags, 0, "Unknown BindImportedFunction flags.", errorMessage);
}

CKAS_STATUS DecodeBytecodeSaveOptions(const CKAngelScriptBytecodeSaveOptions &options,
                                      BytecodeSaveRequest &request,
                                      std::string &errorMessage) {
    request = BytecodeSaveRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "Bytecode save options size is invalid.");
    }
    request.ModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeSaveOptions::ModuleName,
                                      static_cast<const char *>(nullptr));
    request.Write =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeSaveOptions::Write,
                                      static_cast<CKAngelScriptBytecodeWriteCallback>(nullptr));
    request.UserData =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeSaveOptions::UserData,
                                      static_cast<void *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeSaveOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_BYTECODE_DEFAULT));
    const CKAS_STATUS status = ValidateNoUnknownFlags(request.Flags,
                                                     CKAS_BYTECODE_STRIP_DEBUG_INFO,
                                                     "Unknown SaveModuleBytecode flags.",
                                                     errorMessage);
    request.StripDebugInfo = ScriptApiSupport::HasPublicFlag(request.Flags,
                                                             CKAS_BYTECODE_STRIP_DEBUG_INFO);
    return status;
}

CKAS_STATUS DecodeBytecodeLoadOptions(const CKAngelScriptBytecodeLoadOptions &options,
                                      BytecodeLoadRequest &request,
                                      std::string &errorMessage) {
    request = BytecodeLoadRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "Bytecode load options size is invalid.");
    }
    request.ModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeLoadOptions::ModuleName,
                                      static_cast<const char *>(nullptr));
    request.Read =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeLoadOptions::Read,
                                      static_cast<CKAngelScriptBytecodeReadCallback>(nullptr));
    request.UserData =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeLoadOptions::UserData,
                                      static_cast<void *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptBytecodeLoadOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_BYTECODE_DEFAULT));
    return ValidateNoUnknownFlags(request.Flags,
                                  CKAS_BYTECODE_REPLACEEXISTING,
                                  "Unknown LoadModuleBytecode flags.",
                                  errorMessage);
}

CKAS_STATUS DecodeFunctionOptions(const CKAngelScriptFunctionOptions &options,
                                  FunctionRequest &request,
                                  std::string &errorMessage) {
    request = FunctionRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "FindFunction options size is invalid.");
    }
    request.ModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionOptions::ModuleName,
                                      static_cast<const char *>(nullptr));
    request.FunctionName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionOptions::FunctionName,
                                      static_cast<const char *>(nullptr));
    request.FunctionDecl =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionOptions::FunctionDecl,
                                      static_cast<const char *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionOptions::Flags,
                                      static_cast<CKDWORD>(0));
    CKAS_STATUS status = ValidateNoUnknownFlags(request.Flags,
                                               0,
                                               "Unknown FindFunction flags.",
                                               errorMessage);
    if (status != CKAS_OK) {
        return status;
    }
    if (!ScriptApiSupport::HasExactlyOneNonEmpty(request.FunctionName, request.FunctionDecl)) {
        return Fail(errorMessage, "Exactly one of FunctionName or FunctionDecl is required.");
    }
    request.HasFunctionDecl = ScriptApiSupport::IsNonEmpty(request.FunctionDecl);
    return CKAS_OK;
}

CKAS_STATUS DecodeObjectOptions(const CKAngelScriptObjectOptions &options,
                                ObjectRequest &request,
                                std::string &errorMessage) {
    request = ObjectRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "CreateObject options size is invalid.");
    }
    request.ModuleName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectOptions::ModuleName,
                                      static_cast<const char *>(nullptr));
    request.ClassName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectOptions::ClassName,
                                      static_cast<const char *>(nullptr));
    request.ClassNamespace =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectOptions::ClassNamespace,
                                      static_cast<const char *>(nullptr));
    request.TypeDecl =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectOptions::TypeDecl,
                                      static_cast<const char *>(nullptr));
    request.HasTypeDecl = ScriptApiSupport::IsNonEmpty(request.TypeDecl);
    if (!ScriptApiSupport::IsNonEmpty(request.ModuleName)) {
        return Fail(errorMessage, "Module name is required.");
    }
    if (!ScriptApiSupport::HasExactlyOneNonEmpty(request.ClassName, request.TypeDecl)) {
        return Fail(errorMessage, "Exactly one of ClassName or TypeDecl is required.");
    }
    if (request.HasTypeDecl && ScriptApiSupport::IsNonEmpty(request.ClassNamespace)) {
        return Fail(errorMessage, "ClassNamespace cannot be used with TypeDecl.");
    }
    return CKAS_OK;
}

CKAS_STATUS DecodeMethodOptions(const CKAngelScriptMethodOptions &options,
                                MethodRequest &request,
                                std::string &errorMessage) {
    request = MethodRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "FindObjectMethod options size is invalid.");
    }
    request.Object =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptMethodOptions::Object,
                                      static_cast<CKAngelScriptObject *>(nullptr));
    request.MethodName =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptMethodOptions::MethodName,
                                      static_cast<const char *>(nullptr));
    request.MethodDecl =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptMethodOptions::MethodDecl,
                                      static_cast<const char *>(nullptr));
    request.HasMethodDecl = ScriptApiSupport::IsNonEmpty(request.MethodDecl);
    if (!request.Object) {
        return Fail(errorMessage, "Object handle is required.");
    }
    if (!ScriptApiSupport::HasExactlyOneNonEmpty(request.MethodName, request.MethodDecl)) {
        return Fail(errorMessage, "Exactly one of MethodName or MethodDecl is required.");
    }
    return CKAS_OK;
}

CKAS_STATUS DecodeObjectMethodExecuteOptions(const CKAngelScriptObjectMethodExecuteOptions &options,
                                             ObjectMethodExecuteRequest &request,
                                             std::string &errorMessage) {
    request = ObjectMethodExecuteRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "CallObjectMethod options size is invalid.");
    }
    request.Object =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::Object,
                                      static_cast<CKAngelScriptObject *>(nullptr));
    request.Method =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::Method,
                                      static_cast<CKAngelScriptMethod *>(nullptr));
    request.WriteArgs =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::WriteArgs,
                                      static_cast<CKAngelScriptWriteArgsCallback>(nullptr));
    request.ReadResult =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::ReadResult,
                                      static_cast<CKAngelScriptReadResultCallback>(nullptr));
    request.ConfigureContext =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::ConfigureContext,
                                      static_cast<CKAngelScriptContextCallback>(nullptr));
    request.ReadContextResult =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::ReadContextResult,
                                      static_cast<CKAngelScriptContextCallback>(nullptr));
    request.UserData =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::UserData,
                                      static_cast<void *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptObjectMethodExecuteOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    return ValidateNoUnknownFlags(request.Flags,
                                  CKAS_CALL_NO_SUSPEND,
                                  "Unknown object method call flags.",
                                  errorMessage);
}

CKAS_STATUS DecodeFunctionExecutionOptions(const CKAngelScriptFunctionExecutionOptions &options,
                                           FunctionExecutionRequest &request,
                                           std::string &errorMessage) {
    request = FunctionExecutionRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(options)) {
        return Fail(errorMessage, "CreateFunctionExecution options size is invalid.");
    }
    request.Function =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionExecutionOptions::Function,
                                      static_cast<CKAngelScriptFunction *>(nullptr));
    request.BehaviorContext =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionExecutionOptions::BehaviorContext,
                                      static_cast<const CKBehaviorContext *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(options,
                                      &CKAngelScriptFunctionExecutionOptions::Flags,
                                      static_cast<CKDWORD>(CKAS_CALL_DEFAULT));
    CKAS_STATUS status = ValidateNoUnknownFlags(request.Flags,
                                               CKAS_CALL_NO_SUSPEND,
                                               "Unknown function execution flags.",
                                               errorMessage);
    if (status != CKAS_OK) {
        return status;
    }
    if (!request.Function) {
        return Fail(errorMessage, "Function handle is required.");
    }
    return CKAS_OK;
}

CKAS_STATUS ValidateExecutionStepOptions(const CKAngelScriptExecutionStepOptions *options,
                                         const char *apiName,
                                         std::string &errorMessage) {
    errorMessage.clear();
    if (options && !ScriptApiSupport::HasCompletePublicStruct(*options)) {
        errorMessage = apiName ? apiName : "Execution";
        errorMessage += " step options size is invalid.";
        return CKAS_INVALIDARGUMENT;
    }
    return CKAS_OK;
}

CKAS_STATUS DecodeEngineExtension(const CKAngelScriptEngineExtension &extension,
                                  EngineExtensionRequest &request,
                                  std::string &errorMessage) {
    request = EngineExtensionRequest();
    errorMessage.clear();
    if (!ScriptApiSupport::HasCompletePublicStruct(extension)) {
        return Fail(errorMessage, "Engine extension size is invalid.");
    }
    request.Name =
        ScriptApiSupport::PublicField(extension,
                                      &CKAngelScriptEngineExtension::Name,
                                      static_cast<const char *>(nullptr));
    request.Register =
        ScriptApiSupport::PublicField(extension,
                                      &CKAngelScriptEngineExtension::Register,
                                      static_cast<CKAngelScriptEngineExtensionCallback>(nullptr));
    request.UserData =
        ScriptApiSupport::PublicField(extension,
                                      &CKAngelScriptEngineExtension::UserData,
                                      static_cast<void *>(nullptr));
    request.Flags =
        ScriptApiSupport::PublicField(extension,
                                      &CKAngelScriptEngineExtension::Flags,
                                      static_cast<CKDWORD>(CKAS_ENGINEEXTENSION_DEFAULT));
    CKAS_STATUS status = ValidateNoUnknownFlags(request.Flags,
                                               CKAS_ENGINEEXTENSION_DEFERRED,
                                               "Unknown engine extension flags.",
                                               errorMessage);
    if (status != CKAS_OK) {
        return status;
    }
    if (!ScriptApiSupport::IsNonEmpty(request.Name)) {
        return Fail(errorMessage, "Engine extension name is required.");
    }
    if (!request.Register) {
        return Fail(errorMessage, "Engine extension callback is required.");
    }
    return CKAS_OK;
}

CKAS_STATUS ValidateModuleFingerprintOutput(const CKAngelScriptModuleFingerprint *fingerprint,
                                            std::string &errorMessage) {
    errorMessage.clear();
    if (!fingerprint) {
        return Fail(errorMessage, "Module fingerprint out pointer is required.");
    }
    if (!ScriptApiSupport::HasCompletePublicStruct(*fingerprint)) {
        return Fail(errorMessage, "Module fingerprint size is invalid.");
    }
    return CKAS_OK;
}

} // namespace ScriptPublicOptions
