#include "ScriptSelfTests.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <string>

#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"

static int CkasSelfTestExtensionValue() {
    return 77;
}

static int CkasDeferredExtensionValue() {
    return 88;
}

static int CkasPartialFailureExtensionValue() {
    return 99;
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

static int RegisterCkasDeferredSelfTestExtension(asIScriptEngine *engine,
                                                 CKAngelScript *,
                                                 void *,
                                                 const char **errorMessage) {
    if (!engine) {
        if (errorMessage) {
            *errorMessage = "Deferred self-test extension received a null engine.";
        }
        return -1;
    }

    int r = engine->SetDefaultNamespace("CKASDeferredExtensionSelfTest");
    if (r < 0) {
        return r;
    }
    return engine->RegisterGlobalFunction("int Value()", asFUNCTION(CkasDeferredExtensionValue), asCALL_CDECL);
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

static int RegisterCkasPartialFailureSelfTestExtension(asIScriptEngine *engine,
                                                       CKAngelScript *,
                                                       void *,
                                                       const char **errorMessage) {
    if (!engine) {
        return -1;
    }
    int r = engine->SetDefaultNamespace("CKASPartialFailureExtensionSelfTest");
    if (r < 0) {
        return r;
    }
    r = engine->RegisterGlobalFunction("int Value()", asFUNCTION(CkasPartialFailureExtensionValue), asCALL_CDECL);
    if (r < 0) {
        return r;
    }
    if (errorMessage) {
        *errorMessage = "Expected partial extension registration failure.";
    }
    return -88;
}

static bool HasGlobalFunctionInNamespace(asIScriptEngine *engine,
                                         const char *nameSpace,
                                         const char *decl) {
    if (!engine || !decl) {
        return false;
    }
    const char *currentNamespace = engine->GetDefaultNamespace();
    const std::string previousNamespace = currentNamespace ? currentNamespace : "";
    if (engine->SetDefaultNamespace(nameSpace ? nameSpace : "") < 0) {
        return false;
    }
    const bool found = engine->GetGlobalFunctionByDecl(decl) != nullptr;
    engine->SetDefaultNamespace(previousNamespace.c_str());
    return found;
}

static CKAngelScriptRawProc ResolveCkasSelfTestApiSymbol(void *, const char *name) {
    if (!name) {
        return nullptr;
    }
    if (std::strcmp(name, "CKGetAngelScript") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKGetAngelScript);
    }
    if (std::strcmp(name, "CKAngelScriptGetApiVersion") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptGetApiVersion);
    }
    if (std::strcmp(name, "CKAngelScriptHasFeature") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptHasFeature);
    }
    if (std::strcmp(name, "CKAngelScriptInitResult") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitResult);
    }
    if (std::strcmp(name, "CKAngelScriptInitEngineExtension") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitEngineExtension);
    }
    if (std::strcmp(name, "CKAngelScriptGetStatusName") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptGetStatusName);
    }
    if (std::strcmp(name, "CKAngelScriptGetStatusDescription") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptGetStatusDescription);
    }
    if (std::strcmp(name, "CKAngelScriptRegisterEngineExtension") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptRegisterEngineExtension);
    }
    if (std::strcmp(name, "CKAngelScriptUnregisterEngineExtension") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptUnregisterEngineExtension);
    }
    return nullptr;
}

static CKAngelScriptRawProc ResolveNoCkasSelfTestApiSymbol(void *, const char *) {
    return nullptr;
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
    void *ObjectInput = nullptr;
};

struct HostCallFilterProbeData {
    int CallCount = 0;
    const char *LastApiName = nullptr;
    CKDWORD LastFlags = 0;
};

struct MetadataProbe {
    bool Type = false;
    bool Method = false;
    bool Function = false;
    bool Global = false;
    bool TypeProperty = false;
    bool Declaration = false;
    bool SourceSectionType = false;
    int NamespacedTypes = 0;
    int NamespacedMethods = 0;
    CKDWORD CallbackCount = 0;
};

bool CkasStringEquals(const char *lhs, const char *rhs) {
    return lhs && rhs && std::strcmp(lhs, rhs) == 0;
}

bool CkasStringContains(const char *text, const char *needle) {
    return text && needle && std::strstr(text, needle) != nullptr;
}

CKAS_STATUS ConfigureIntArgument(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<IntExecutionData *>(userData);
    return ctx && ctx->SetArgDWord(0, static_cast<asDWORD>(data ? data->Input : 0)) >= 0
               ? CKAS_OK
               : CKAS_EXECUTIONFAILED;
}

CKAS_STATUS ReadIntReturn(asIScriptContext *ctx, void *userData) {
    auto *data = static_cast<IntExecutionData *>(userData);
    if (data) {
        data->Output = static_cast<int>(ctx->GetReturnDWord());
    }
    return CKAS_OK;
}

CKAS_STATUS MarkResumeHook(asIScriptContext *ctx, void *userData) {
    if (!ctx || ctx->GetState() != asEXECUTION_SUSPENDED) {
        return CKAS_INVALIDSTATE;
    }
    auto *data = static_cast<IntExecutionData *>(userData);
    if (data) {
        data->Input += 1;
    }
    return CKAS_OK;
}

CKAS_STATUS RejectExecutionConfigure(asIScriptContext *, void *) {
    return CKAS_INVALIDARGUMENT;
}

CKAS_STATUS HostCallFilterProbe(const char *apiName, CKDWORD flags, void *userData) {
    auto *data = static_cast<HostCallFilterProbeData *>(userData);
    if (!data || !apiName || apiName[0] == '\0') {
        return CKAS_INVALIDARGUMENT;
    }
    ++data->CallCount;
    data->LastApiName = apiName;
    data->LastFlags = flags;
    return (flags & CKAS_HOSTCALL_MUTATES_HOST_STATE) != 0 ? CKAS_INVALIDSTATE : CKAS_OK;
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

CKAS_STATUS WriteObjectHandle(CKAngelScriptArgWriter *writer, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptArgSetObjectHandle(writer, 0, data ? data->ObjectInput : nullptr);
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

CKAS_STATUS ProbeMetadata(const CKAngelScriptMetadataEntry *entry,
                          CKDWORD metadataIndex,
                          const char *metadata,
                          void *userData) {
    auto *probe = static_cast<MetadataProbe *>(userData);
    if (!probe || !entry || entry->Size < sizeof(*entry) || metadataIndex >= entry->MetadataCount || !metadata) {
        return CKAS_INVALIDARGUMENT;
    }

    ++probe->CallbackCount;
    if (CkasStringEquals(metadata, "ckas_selftest_type") &&
        entry->Target == CKAS_METADATA_TYPE &&
        CkasStringEquals(entry->Name, "__CKAS_PublicMetadataType")) {
        probe->Type = true;
    } else if (CkasStringEquals(metadata, "ckas_selftest_method") &&
               entry->Target == CKAS_METADATA_TYPE_METHOD &&
               CkasStringEquals(entry->ParentTypeName, "__CKAS_PublicMetadataType") &&
               CkasStringEquals(entry->Name, "Add")) {
        probe->Method = true;
        probe->Declaration = probe->Declaration ||
                             (CkasStringContains(entry->Declaration, "int") &&
                              CkasStringContains(entry->Declaration, "Add"));
    } else if (CkasStringEquals(metadata, "ckas_selftest_function") &&
               entry->Target == CKAS_METADATA_GLOBAL_FUNCTION &&
               CkasStringEquals(entry->Name, "__ckas_public_metadata_global")) {
        probe->Function = true;
        probe->Declaration = probe->Declaration ||
                             CkasStringContains(entry->Declaration, "__ckas_public_metadata_global");
    } else if (CkasStringEquals(metadata, "ckas_selftest_global") &&
               entry->Target == CKAS_METADATA_GLOBAL_VARIABLE &&
               CkasStringEquals(entry->Name, "__ckas_public_metadata_value")) {
        probe->Global = true;
    } else if (CkasStringEquals(metadata, "ckas_selftest_property") &&
               entry->Target == CKAS_METADATA_TYPE_PROPERTY &&
               CkasStringEquals(entry->ParentTypeName, "__CKAS_PublicMetadataType") &&
               CkasStringEquals(entry->Name, "Value")) {
        probe->TypeProperty = true;
    } else if (CkasStringEquals(metadata, "ckas_selftest_type_namespace") &&
               entry->Target == CKAS_METADATA_TYPE &&
               CkasStringEquals(entry->Name, "Duplicate") &&
               CkasStringContains(entry->Namespace, "CKASMetadata")) {
        ++probe->NamespacedTypes;
    } else if (CkasStringEquals(metadata, "ckas_selftest_method_namespace") &&
               entry->Target == CKAS_METADATA_TYPE_METHOD &&
               CkasStringEquals(entry->ParentTypeName, "Duplicate") &&
               CkasStringContains(entry->ParentTypeNamespace, "CKASMetadata") &&
               CkasStringEquals(entry->Name, "Mark")) {
        ++probe->NamespacedMethods;
    } else if (CkasStringEquals(metadata, "ckas_selftest_source_section_type") &&
               entry->Target == CKAS_METADATA_TYPE &&
               CkasStringEquals(entry->Name, "__CKAS_SourceSectionMetadataType")) {
        probe->SourceSectionType = true;
    }

    return CKAS_OK;
}

CKAS_STATUS StopMetadata(const CKAngelScriptMetadataEntry *, CKDWORD, const char *, void *) {
    return CKAS_CANCELLED;
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

bool ContainsStructuredCompileDiagnostic(const CKAngelScriptResult &result) {
    if (!result.CompilerMessages || result.CompilerMessageCount == 0) {
        return false;
    }
    for (size_t i = 0; i < result.CompilerMessageCount; ++i) {
        const CKAngelScriptCompilerMessage &message = result.CompilerMessages[i];
        if (message.Size >= sizeof(CKAngelScriptCompilerMessage) &&
            message.Type == CKAS_MESSAGE_ERROR &&
            message.Row > 0 &&
            message.Column > 0 &&
            message.Message &&
            message.Message[0] != '\0') {
            return true;
        }
    }
    return false;
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

    CKAngelScriptResult result = CKAngelScriptApi::Result();
    if (result.Size != sizeof(result) ||
        result.Status != CKAS_OK ||
        result.AngelScriptCode != 0 ||
        result.ErrorMessage ||
        result.StackTrace ||
        result.CompilerMessages ||
        result.CompilerMessageCount != 0) {
        error = "CKAngelScript API self-test expected Result() to initialize a clean result.";
        return false;
    }

    CKAngelScriptResult initResult;
    std::memset(&initResult, 0x7f, sizeof(initResult));
    CKAngelScriptInitResult(&initResult);
    if (initResult.Size != sizeof(initResult) ||
        initResult.Status != CKAS_OK ||
        initResult.AngelScriptCode != 0 ||
        initResult.ErrorMessage ||
        initResult.StackTrace ||
        initResult.CompilerMessages ||
        initResult.CompilerMessageCount != 0) {
        error = "CKAngelScript API self-test expected Result initializer defaults.";
        return false;
    }

    CKAngelScriptLoadOptions initLoadOptions;
    std::memset(&initLoadOptions, 0x7f, sizeof(initLoadOptions));
    CKAngelScriptInitLoadOptions(&initLoadOptions);
    if (initLoadOptions.Size != sizeof(initLoadOptions) ||
        initLoadOptions.ModuleName ||
        initLoadOptions.Filename ||
        initLoadOptions.Filenames ||
        initLoadOptions.FileCount != 0 ||
        initLoadOptions.Code ||
        initLoadOptions.Sections ||
        initLoadOptions.SectionCount != 0 ||
        initLoadOptions.Flags != CKAS_LOAD_DEFAULT) {
        error = "CKAngelScript API self-test expected LoadOptions initializer defaults.";
        return false;
    }

    CKAngelScriptFunctionOptions initFunctionOptions;
    std::memset(&initFunctionOptions, 0x7f, sizeof(initFunctionOptions));
    CKAngelScriptInitFunctionOptions(&initFunctionOptions);
    if (initFunctionOptions.Size != sizeof(initFunctionOptions) ||
        initFunctionOptions.ModuleName ||
        initFunctionOptions.FunctionName ||
        initFunctionOptions.FunctionDecl ||
        initFunctionOptions.Flags != 0) {
        error = "CKAngelScript API self-test expected FunctionOptions initializer defaults.";
        return false;
    }

    CKAngelScriptFunctionExecutionOptions initExecutionOptions;
    std::memset(&initExecutionOptions, 0x7f, sizeof(initExecutionOptions));
    CKAngelScriptInitFunctionExecutionOptions(&initExecutionOptions);
    if (initExecutionOptions.Size != sizeof(initExecutionOptions) ||
        initExecutionOptions.Function ||
        initExecutionOptions.BehaviorContext ||
        initExecutionOptions.Flags != CKAS_CALL_DEFAULT) {
        error = "CKAngelScript API self-test expected FunctionExecutionOptions initializer defaults.";
        return false;
    }

    CKAngelScriptExecutionStepOptions initStepOptions;
    std::memset(&initStepOptions, 0x7f, sizeof(initStepOptions));
    CKAngelScriptInitExecutionStepOptions(&initStepOptions);
    if (initStepOptions.Size != sizeof(initStepOptions) ||
        initStepOptions.ConfigureContext ||
        initStepOptions.ReadResult ||
        initStepOptions.UserData) {
        error = "CKAngelScript API self-test expected ExecutionStepOptions initializer defaults.";
        return false;
    }

    CKAngelScriptObjectOptions initObjectOptions;
    std::memset(&initObjectOptions, 0x7f, sizeof(initObjectOptions));
    CKAngelScriptInitObjectOptions(&initObjectOptions);
    if (initObjectOptions.Size != sizeof(initObjectOptions) ||
        initObjectOptions.ModuleName ||
        initObjectOptions.ClassName ||
        initObjectOptions.ClassNamespace ||
        initObjectOptions.TypeDecl) {
        error = "CKAngelScript API self-test expected ObjectOptions initializer defaults.";
        return false;
    }

    CKAngelScriptMethodOptions initMethodOptions;
    std::memset(&initMethodOptions, 0x7f, sizeof(initMethodOptions));
    CKAngelScriptInitMethodOptions(&initMethodOptions);
    if (initMethodOptions.Size != sizeof(initMethodOptions) ||
        initMethodOptions.Object ||
        initMethodOptions.MethodName ||
        initMethodOptions.MethodDecl) {
        error = "CKAngelScript API self-test expected MethodOptions initializer defaults.";
        return false;
    }

    CKAngelScriptObjectMethodExecuteOptions initObjectCallOptions;
    std::memset(&initObjectCallOptions, 0x7f, sizeof(initObjectCallOptions));
    CKAngelScriptInitObjectMethodExecuteOptions(&initObjectCallOptions);
    if (initObjectCallOptions.Size != sizeof(initObjectCallOptions) ||
        initObjectCallOptions.Object ||
        initObjectCallOptions.Method ||
        initObjectCallOptions.WriteArgs ||
        initObjectCallOptions.ReadResult ||
        initObjectCallOptions.UserData ||
        initObjectCallOptions.Flags != CKAS_CALL_DEFAULT) {
        error = "CKAngelScript API self-test expected ObjectMethodExecuteOptions initializer defaults.";
        return false;
    }

    CKAngelScriptEngineExtension initExtension;
    std::memset(&initExtension, 0x7f, sizeof(initExtension));
    CKAngelScriptInitEngineExtension(&initExtension);
    if (initExtension.Size != sizeof(initExtension) ||
        initExtension.Name ||
        initExtension.Register ||
        initExtension.UserData ||
        initExtension.Flags != CKAS_ENGINEEXTENSION_DEFAULT) {
        error = "CKAngelScript API self-test expected EngineExtension initializer defaults.";
        return false;
    }

    CKAngelScriptInitResult(nullptr);
    CKAngelScriptInitLoadOptions(nullptr);
    CKAngelScriptInitFunctionOptions(nullptr);
    CKAngelScriptInitFunctionExecutionOptions(nullptr);
    CKAngelScriptInitExecutionStepOptions(nullptr);
    CKAngelScriptInitObjectOptions(nullptr);
    CKAngelScriptInitMethodOptions(nullptr);
    CKAngelScriptInitObjectMethodExecuteOptions(nullptr);
    CKAngelScriptInitEngineExtension(nullptr);

    const CKAS_STATUS knownStatuses[] = {
        CKAS_OK,
        CKAS_INVALIDARGUMENT,
        CKAS_NOTINITIALIZED,
        CKAS_NOTFOUND,
        CKAS_COMPILEERROR,
        CKAS_EXECUTIONFAILED,
        CKAS_SUSPENDED,
        CKAS_CANCELLED,
        CKAS_STALEHANDLE,
        CKAS_UNSUPPORTED,
        CKAS_TYPEMISMATCH,
        CKAS_BUFFERTOOSMALL,
        CKAS_INVALIDSTATE,
        CKAS_INUSE,
        CKAS_ALREADYEXISTS,
        CKAS_AMBIGUOUS,
        CKAS_FOREIGNHANDLE
    };
    for (CKAS_STATUS status : knownStatuses) {
        const char *name = CKAngelScriptApi::StatusName(status);
        const char *description = CKAngelScriptApi::StatusDescription(status);
        if (!name || name[0] == '\0' || !description || description[0] == '\0') {
            error = "CKAngelScript API self-test expected status text for every known status.";
            return false;
        }
    }
    if (std::string(CKAngelScriptApi::StatusName(static_cast<CKAS_STATUS>(9999))) != "CKAS_UNKNOWN" ||
        !CKAngelScriptApi::StatusDescription(static_cast<CKAS_STATUS>(9999)) ||
        CKAngelScriptApi::StatusDescription(static_cast<CKAS_STATUS>(9999))[0] == '\0') {
        error = "CKAngelScript API self-test expected stable fallback status text.";
        return false;
    }
    if (CKAS_FOREIGNHANDLE != 16 ||
        CKAS_FEATURE_METADATA_REFLECTION != 13 ||
        CKAS_FEATURE_OBJECT_TYPE_NAMESPACE != 14 ||
        CKAS_FEATURE_OBJECT_METHOD_CONTEXT_ACCESS != 15 ||
        CKAS_FEATURE_SCRIPT_ARRAY_ACCESS != 16 ||
        CKAS_FEATURE_ACTIVE_CONTEXT_EXCEPTION != 17 ||
        CKAS_FEATURE_SOURCE_SECTIONS != 18 ||
        CKAS_FEATURE_OBJECT_HANDLE_ARGS != 19 ||
        CKAS_FEATURE_HOST_CALL_FILTER != 20 ||
        CKAS_EXECUTION_CANCELLED != 5 ||
        CKAS_LOAD_REPLACEEXISTING != 0x00000001 ||
        CKAS_COMPILE_REPLACEEXISTING != 0x00000001 ||
        CKAS_ENGINEEXTENSION_DEFERRED != 0x00000001 ||
        CKAS_CALL_NO_SUSPEND != 0x00000001 ||
        CKAS_HOSTCALL_MUTATES_HOST_STATE != 0x00000001 ||
        CKAS_METADATA_TYPE_PROPERTY != 5) {
        error = "CKAngelScript API self-test expected stable explicit public enum values.";
        return false;
    }

    if (api->GetApiVersion() != CKAS_API_VERSION ||
        !api->HasFeature(CKAS_FEATURE_MODULE_LIFECYCLE) ||
        !api->HasFeature(CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS) ||
        !api->HasFeature(CKAS_FEATURE_FUNCTION_HANDLE) ||
        !api->HasFeature(CKAS_FEATURE_FUNCTION_EXECUTION) ||
        !api->HasFeature(CKAS_FEATURE_FUNCTION_EXECUTION_RESUME) ||
        !api->HasFeature(CKAS_FEATURE_OBJECT_HANDLE) ||
        !api->HasFeature(CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL) ||
        !api->HasFeature(CKAS_FEATURE_TYPED_ARG_READER_WRITER) ||
        !api->HasFeature(CKAS_FEATURE_STACK_TRACE) ||
        !api->HasFeature(CKAS_FEATURE_ENGINE_EXTENSION) ||
        !api->HasFeature(CKAS_FEATURE_PUBLIC_STRUCT_INITIALIZERS) ||
        !api->HasFeature(CKAS_FEATURE_STATUS_TEXT) ||
        !api->HasFeature(CKAS_FEATURE_METADATA_REFLECTION) ||
        !api->HasFeature(CKAS_FEATURE_OBJECT_TYPE_NAMESPACE) ||
        !api->HasFeature(CKAS_FEATURE_OBJECT_METHOD_CONTEXT_ACCESS) ||
        !api->HasFeature(CKAS_FEATURE_SCRIPT_ARRAY_ACCESS) ||
        !api->HasFeature(CKAS_FEATURE_ACTIVE_CONTEXT_EXCEPTION) ||
        !api->HasFeature(CKAS_FEATURE_SOURCE_SECTIONS) ||
        !api->HasFeature(CKAS_FEATURE_OBJECT_HANDLE_ARGS) ||
        !api->HasFeature(CKAS_FEATURE_HOST_CALL_FILTER)) {
        error = "CKAngelScript API self-test found an unexpected v8 feature set.";
        return false;
    }

    HostCallFilterProbeData hostCallFilterProbe;
    CKAngelScriptResult hostCallFilterResult = CKAngelScriptApi::Result();
    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "CKAngelScript API self-test could not resolve ScriptManager for host-call filter.";
        return false;
    }
    if (api->SetHostCallFilter(HostCallFilterProbe, &hostCallFilterProbe, &hostCallFilterResult) != CKAS_OK ||
        hostCallFilterResult.Status != CKAS_OK) {
        error = "CKAngelScript API self-test failed to install a host-call filter.";
        return false;
    }
    if (manager->RejectHostCall("SelfTest::ReadOnly", CKAS_HOSTCALL_DEFAULT) ||
        hostCallFilterProbe.CallCount != 1 ||
        !CkasStringEquals(hostCallFilterProbe.LastApiName, "SelfTest::ReadOnly") ||
        hostCallFilterProbe.LastFlags != CKAS_HOSTCALL_DEFAULT) {
        error = "CKAngelScript API self-test expected read-only host calls to pass through the filter.";
        return false;
    }
    if (!manager->RejectHostCall("SelfTest::Mutating", CKAS_HOSTCALL_MUTATES_HOST_STATE) ||
        hostCallFilterProbe.CallCount != 2 ||
        !CkasStringEquals(hostCallFilterProbe.LastApiName, "SelfTest::Mutating") ||
        hostCallFilterProbe.LastFlags != CKAS_HOSTCALL_MUTATES_HOST_STATE) {
        error = "CKAngelScript API self-test expected mutating host calls to be rejected by the filter.";
        return false;
    }
    if (api->SetHostCallFilter(nullptr, nullptr, &hostCallFilterResult) != CKAS_OK ||
        hostCallFilterResult.Status != CKAS_OK ||
        manager->RejectHostCall("SelfTest::AfterClear", CKAS_HOSTCALL_MUTATES_HOST_STATE)) {
        error = "CKAngelScript API self-test failed to clear the host-call filter.";
        return false;
    }

    CKAngelScriptExtensionApi softApi;
    CKAngelScriptInitExtensionApi(&softApi);
    if (softApi.Size != sizeof(softApi) ||
        CKAngelScriptExtensionApiIsLoaded(&softApi)) {
        error = "CKAngelScript API self-test expected an initialized extension API table to be empty.";
        return false;
    }
    CKAngelScriptInitExtensionApi(nullptr);
    if (CKAngelScriptLoadExtensionApi(nullptr, ResolveCkasSelfTestApiSymbol, nullptr) != FALSE ||
        CKAngelScriptLoadExtensionApi(&softApi, nullptr, nullptr) != FALSE ||
        CKAngelScriptLoadExtensionApi(&softApi, ResolveNoCkasSelfTestApiSymbol, nullptr) != FALSE ||
        CKAngelScriptExtensionApiIsLoaded(&softApi)) {
        error = "CKAngelScript API self-test expected extension API soft-load failures to clear the table.";
        return false;
    }
    if (CKAngelScriptLoadExtensionApi(&softApi, ResolveCkasSelfTestApiSymbol, nullptr) != TRUE ||
        !CKAngelScriptExtensionApiIsLoaded(&softApi) ||
        softApi.GetApiVersion() != CKAS_API_VERSION) {
        error = "CKAngelScript API self-test failed to soft-load the extension API table.";
        return false;
    }

    CKAngelScriptResult softResult = CKAngelScriptApi::Result();
    if (CKAngelScriptRegisterEngineExtensionWithApi(nullptr,
                                                    context,
                                                    "__ckas_soft_api_extension",
                                                    RegisterCkasDeferredSelfTestExtension,
                                                    nullptr,
                                                    CKAS_ENGINEEXTENSION_DEFERRED,
                                                    &softResult) != CKAS_INVALIDARGUMENT ||
        softResult.Status != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected an unloaded extension API table to fail registration.";
        return false;
    }
    if (CKAngelScriptRegisterEngineExtensionWithApi(&softApi,
                                                    context,
                                                    "__ckas_soft_api_extension",
                                                    RegisterCkasDeferredSelfTestExtension,
                                                    nullptr,
                                                    CKAS_ENGINEEXTENSION_DEFERRED,
                                                    &softResult) != CKAS_OK ||
        CKAngelScriptUnregisterEngineExtensionWithApi(&softApi,
                                                      context,
                                                      "__ckas_soft_api_extension",
                                                      &softResult) != CKAS_OK) {
        error = "CKAngelScript API self-test expected the soft-loaded extension API table to register and unregister.";
        return false;
    }

    CKBOOL clearedBool = TRUE;
    int clearedInt = 123;
    float clearedFloat = 4.0f;
    const char *clearedStringView = reinterpret_cast<const char *>(static_cast<uintptr_t>(1));
    size_t clearedStringViewSize = 99;
    char clearedString[8] = "dirty";
    size_t clearedRequiredSize = 99;
    if (CKAngelScriptResultGetBool(nullptr, &clearedBool) != CKAS_INVALIDARGUMENT || clearedBool != FALSE ||
        CKAngelScriptResultGetInt(nullptr, &clearedInt) != CKAS_INVALIDARGUMENT || clearedInt != 0 ||
        CKAngelScriptResultGetFloat(nullptr, &clearedFloat) != CKAS_INVALIDARGUMENT || clearedFloat != 0.0f ||
        CKAngelScriptResultGetStringView(nullptr, &clearedStringView, &clearedStringViewSize) != CKAS_INVALIDARGUMENT ||
        clearedStringView != nullptr ||
        clearedStringViewSize != 0 ||
        CKAngelScriptResultGetString(nullptr, clearedString, sizeof(clearedString), &clearedRequiredSize) != CKAS_INVALIDARGUMENT ||
        clearedString[0] != '\0' ||
        clearedRequiredSize != 0) {
        error = "CKAngelScript API self-test expected result helpers to clear outputs on failure.";
        return false;
    }

    asIScriptEngine *invalidEngine = reinterpret_cast<asIScriptEngine *>(static_cast<uintptr_t>(1));
    result = {};
    if (CKAngelScriptBorrowEngine(nullptr, &invalidEngine, &result) != CKAS_INVALIDARGUMENT ||
        invalidEngine != nullptr ||
        result.Status != CKAS_INVALIDARGUMENT ||
        !result.ErrorMessage) {
        error = "CKAngelScript API self-test expected invalid public handles to fill result diagnostics.";
        return false;
    }

    CKAngelScriptEngineExtension invalidSizeExtension = CKAngelScriptApi::EngineExtension();
    invalidSizeExtension.Size = 0;
    invalidSizeExtension.Name = "__ckas_invalid_size_extension";
    invalidSizeExtension.Register = RegisterCkasSelfTestExtension;
    if (api->RegisterEngineExtension(invalidSizeExtension, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected zero-sized extension options to fail.";
        return false;
    }

    CKAngelScriptEngineExtension failingExtension = CKAngelScriptApi::EngineExtension();
    failingExtension.Name = "__ckas_failing_extension";
    failingExtension.Register = RegisterCkasFailingSelfTestExtension;
    if (api->RegisterEngineExtension(failingExtension, &result) != CKAS_EXECUTIONFAILED ||
        !result.ErrorMessage ||
        std::string(result.ErrorMessage).find("Expected self-test extension failure") == std::string::npos) {
        error = "CKAngelScript API self-test expected failing extension diagnostics.";
        return false;
    }
    if (api->UnregisterEngineExtension("__ckas_failing_extension", &result) != CKAS_NOTFOUND) {
        error = "CKAngelScript API self-test expected failing extension registration to avoid retaining the extension.";
        return false;
    }

    asIScriptEngine *engine = nullptr;
    if (!ExpectStatus(api->BorrowEngine(&engine, &result), CKAS_OK, "BorrowEngine", &result, error) || !engine) {
        return false;
    }
    if (!engine->GetGlobalFunctionByDecl("string CKStrupr(const string &in str)") ||
        !engine->GetGlobalFunctionByDecl("string CKStrlwr(const string &in str)") ||
        engine->GetGlobalFunctionByDecl("NativePointer CKStrupr(const string &in str)") ||
        engine->GetGlobalFunctionByDecl("NativePointer CKStrlwr(const string &in str)")) {
        error = "CKAngelScript API self-test found unsafe CKStrupr/CKStrlwr string overload declarations.";
        return false;
    }

    {
        void *intArray = nullptr;
        if (api->CreateArray("array<int>", 2, &intArray) != CKAS_OK || !intArray) {
            error = "CKAngelScript API self-test failed to create array<int> through public API.";
            return false;
        }

        int refCount = 0;
        asITypeInfo *arrayType = nullptr;
        int arrayTypeId = 0;
        int elementTypeId = 0;
        CKDWORD size = 0;
        int value0 = 11;
        int value1 = 22;
        int value2 = 33;
        const void *constElement = nullptr;
        void *element = nullptr;
        if (CKAngelScriptApi::ArrayGetRefCount(intArray, &refCount) != CKAS_OK ||
            refCount != 1 ||
            CKAngelScriptApi::ArrayGetArrayType(intArray, &arrayType) != CKAS_OK ||
            !arrayType ||
            CKAngelScriptApi::ArrayGetArrayTypeId(intArray, &arrayTypeId) != CKAS_OK ||
            arrayTypeId != arrayType->GetTypeId() ||
            CKAngelScriptApi::ArrayGetElementTypeId(intArray, &elementTypeId) != CKAS_OK ||
            elementTypeId != asTYPEID_INT32 ||
            CKAngelScriptApi::ArrayGetSize(intArray, &size) != CKAS_OK ||
            size != 2 ||
            CKAngelScriptApi::ArraySetElementValue(intArray, 0, &value0) != CKAS_OK ||
            CKAngelScriptApi::ArraySetElementValue(intArray, 1, &value1) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetConstElementAddress(intArray, 0, &constElement) != CKAS_OK ||
            !constElement ||
            *static_cast<const int *>(constElement) != value0 ||
            CKAngelScriptApi::ArrayInsertLast(intArray, &value2) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetSize(intArray, &size) != CKAS_OK ||
            size != 3 ||
            CKAngelScriptApi::ArrayRemoveAt(intArray, 1) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetElementAddress(intArray, 1, &element) != CKAS_OK ||
            !element ||
            *static_cast<int *>(element) != value2 ||
            CKAngelScriptApi::ArrayReserve(intArray, 8) != CKAS_OK ||
            CKAngelScriptApi::ArrayResize(intArray, 4) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetSize(intArray, &size) != CKAS_OK ||
            size != 4 ||
            CKAngelScriptApi::ArrayClear(intArray) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetSize(intArray, &size) != CKAS_OK ||
            size != 0 ||
            CKAngelScriptApi::ArrayAddRef(intArray) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetRefCount(intArray, &refCount) != CKAS_OK ||
            refCount != 2 ||
            CKAngelScriptApi::ArrayRelease(intArray) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetRefCount(intArray, &refCount) != CKAS_OK ||
            refCount != 1) {
            CKAngelScriptApi::ArrayRelease(intArray);
            error = "CKAngelScript API self-test found invalid array<int> public API behavior.";
            return false;
        }
        void *handleSlot = nullptr;
        if (CKAngelScriptApi::AssignObjectHandle(&handleSlot, intArray, arrayType) != CKAS_OK ||
            handleSlot != intArray ||
            CKAngelScriptApi::ArrayGetRefCount(intArray, &refCount) != CKAS_OK ||
            refCount != 2 ||
            CKAngelScriptApi::AssignObjectHandle(&handleSlot, nullptr, arrayType) != CKAS_OK ||
            handleSlot != nullptr ||
            CKAngelScriptApi::ArrayGetRefCount(intArray, &refCount) != CKAS_OK ||
            refCount != 1) {
            if (handleSlot) {
                CKAngelScriptApi::ArrayRelease(handleSlot);
            }
            CKAngelScriptApi::ArrayRelease(intArray);
            error = "CKAngelScript API self-test found invalid object handle assignment behavior.";
            return false;
        }
        CKAngelScriptApi::ArrayRelease(intArray);

        asITypeInfo *byteArrayType = engine->GetTypeInfoByDecl("array<uint8>");
        void *byteArray = nullptr;
        unsigned char byteValue = 42;
        if (!byteArrayType ||
            CKAngelScriptApi::CreateArrayByType(byteArrayType, 1, &byteArray) != CKAS_OK ||
            !byteArray ||
            CKAngelScriptApi::ArrayGetElementTypeId(byteArray, &elementTypeId) != CKAS_OK ||
            elementTypeId != asTYPEID_UINT8 ||
            CKAngelScriptApi::ArraySetElementValue(byteArray, 0, &byteValue) != CKAS_OK ||
            CKAngelScriptApi::ArrayGetConstElementAddress(byteArray, 0, &constElement) != CKAS_OK ||
            !constElement ||
            *static_cast<const unsigned char *>(constElement) != byteValue) {
            if (byteArray) {
                CKAngelScriptApi::ArrayRelease(byteArray);
            }
            error = "CKAngelScript API self-test found invalid array<uint8> public API behavior.";
            return false;
        }
        CKAngelScriptApi::ArrayRelease(byteArray);

        void *missingArray = reinterpret_cast<void *>(static_cast<uintptr_t>(1));
        if (api->CreateArray("__CKAS_MissingArray", 0, &missingArray) != CKAS_NOTFOUND ||
            missingArray != nullptr ||
            CKAngelScriptApi::ArrayGetSize(nullptr, &size) != CKAS_INVALIDARGUMENT ||
            size != 0) {
            error = "CKAngelScript API self-test expected array API failures to clear outputs.";
            return false;
        }
    }

    CKAngelScriptEngineExtension partialFailureExtension = CKAngelScriptApi::EngineExtension();
    partialFailureExtension.Name = "__ckas_partial_failure_extension";
    partialFailureExtension.Register = RegisterCkasPartialFailureSelfTestExtension;
    if (api->RegisterEngineExtension(partialFailureExtension, &result) != CKAS_EXECUTIONFAILED ||
        HasGlobalFunctionInNamespace(engine, "CKASPartialFailureExtensionSelfTest", "int Value()")) {
        error = "CKAngelScript API self-test expected partial extension failure to roll back registered symbols.";
        return false;
    }
    partialFailureExtension.Register = RegisterCkasDeferredSelfTestExtension;
    if (api->RegisterEngineExtension(partialFailureExtension, &result) != CKAS_OK ||
        !HasGlobalFunctionInNamespace(engine, "CKASDeferredExtensionSelfTest", "int Value()") ||
        api->UnregisterEngineExtension("__ckas_partial_failure_extension", &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected failed extension names to be reusable.";
        return false;
    }

    CKAngelScriptEngineExtension deferredExtension = CKAngelScriptApi::EngineExtension();
    deferredExtension.Name = "__ckas_deferred_extension";
    deferredExtension.Register = RegisterCkasDeferredSelfTestExtension;
    deferredExtension.Flags = CKAS_ENGINEEXTENSION_DEFERRED;
    if (api->RegisterEngineExtension(deferredExtension, &result) != CKAS_OK ||
        HasGlobalFunctionInNamespace(engine, "CKASDeferredExtensionSelfTest", "int Value()") ||
        api->UnregisterEngineExtension("__ckas_deferred_extension", &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected deferred extension unregister before rebuild to stay inactive.";
        return false;
    }

    CKAngelScriptEngineExtension extension =
        CKAngelScriptApi::EngineExtension("__ckas_public_api_extension", RegisterCkasSelfTestExtension);
    if (!ExpectStatus(api->RegisterEngineExtension(extension, &result),
                      CKAS_OK,
                      "RegisterEngineExtension",
                      &result,
                      error)) {
        return false;
    }
    if (api->RegisterEngineExtension(extension, &result) != CKAS_ALREADYEXISTS) {
        error = "CKAngelScript API self-test expected duplicate extension registration to return ALREADYEXISTS.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_ManagerApiSelfTest";
    const char *source =
        "int __ckas_public_add(int value) { return value + 5; }\n"
        "int __ckas_public_extension() { return CKASExtensionSelfTest::Value(); }\n"
        "int __ckas_public_overload() { return 1; }\n"
        "int __ckas_public_overload(int value) { return value; }\n"
        "int __ckas_public_side_effect_value = 0;\n"
        "int __ckas_public_side_effect() { __ckas_public_side_effect_value = 1; return 0; }\n"
        "int __ckas_public_get_side_effect() { return __ckas_public_side_effect_value; }\n"
        "int __ckas_public_async() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 9; }\n"
        "int __ckas_public_exception() { array<int> values; return values[1]; }\n"
        "int __ckas_public_ckui_callback_struct() {\n"
        "    CKUICallbackStruct first;\n"
        "    first.Reason = CKUIM_OUTTOCONSOLE;\n"
        "    first.ConsoleString = \"alpha\";\n"
        "    first.ConsoleString = \"beta\";\n"
        "    if (first.ConsoleString != \"beta\") return 0;\n"
        "    CKUICallbackStruct second(first);\n"
        "    if (second.ConsoleString != \"beta\") return 0;\n"
        "    second.ConsoleString = \"gamma\";\n"
        "    if (first.ConsoleString != \"beta\" || second.ConsoleString != \"gamma\") return 0;\n"
        "    CKUICallbackStruct third;\n"
        "    third = second;\n"
        "    if (third.ConsoleString != \"gamma\") return 0;\n"
        "    third.ConsoleString = \"delta\";\n"
        "    if (third.ConsoleString != \"delta\") return 0;\n"
        "    CKUICallbackStruct defaultReason;\n"
        "    defaultReason.ConsoleString = \"owned-default\";\n"
        "    {\n"
        "        CKUICallbackStruct scopedCopy(defaultReason);\n"
        "        if (scopedCopy.ConsoleString != \"owned-default\") return 0;\n"
        "    }\n"
        "    if (defaultReason.ConsoleString != \"owned-default\") return 0;\n"
        "    CKUICallbackStruct assignedDefault;\n"
        "    assignedDefault = defaultReason;\n"
        "    if (assignedDefault.ConsoleString != \"owned-default\") return 0;\n"
        "    assignedDefault.ConsoleString = \"assigned-default\";\n"
        "    if (defaultReason.ConsoleString != \"owned-default\") return 0;\n"
        "    return assignedDefault.ConsoleString == \"assigned-default\" ? 1 : 0;\n"
        "}\n"
        "int __ckas_public_ckstr_string_overloads() {\n"
        "    if (CKStrupr(\"BaLl\") != \"BALL\") return 0;\n"
        "    if (CKStrlwr(\"BaLl\") != \"ball\") return 0;\n"
        "    if (CKStrupr(\"\") != \"\") return 0;\n"
        "    if (CKStrlwr(\"\") != \"\") return 0;\n"
        "    return 1;\n"
        "}\n"
        "[ckas_selftest_type]\n"
        "class __CKAS_PublicMetadataType {\n"
        "    [ckas_selftest_property]\n"
        "    int Value;\n"
        "    [ckas_selftest_method]\n"
        "    int Add(int value) { return value + 1; }\n"
        "}\n"
        "[ckas_selftest_function]\n"
        "int __ckas_public_metadata_global() { return 1; }\n"
        "[ckas_selftest_global]\n"
        "int __ckas_public_metadata_value = 2;\n"
        "namespace CKASMetadataA { [ckas_selftest_type_namespace] class Duplicate { [ckas_selftest_method_namespace] void Mark() {} } }\n"
        "namespace CKASMetadataB { [ckas_selftest_type_namespace] class Duplicate { [ckas_selftest_method_namespace] void Mark() {} } }\n";

    CKAngelScriptFunctionOptions zeroSizeFunctionOptions = CKAngelScriptApi::FunctionOptions();
    zeroSizeFunctionOptions.Size = 0;
    zeroSizeFunctionOptions.ModuleName = moduleName;
    zeroSizeFunctionOptions.FunctionDecl = "int __ckas_public_add(int)";
    CKAngelScriptFunction *invalidSizedFunction = reinterpret_cast<CKAngelScriptFunction *>(static_cast<uintptr_t>(1));
    if (api->FindFunction(zeroSizeFunctionOptions, &invalidSizedFunction, &result) != CKAS_INVALIDARGUMENT ||
        invalidSizedFunction != nullptr) {
        error = "CKAngelScript API self-test expected zero-sized function options to fail and clear out pointer.";
        return false;
    }

    CKAngelScriptLoadOptions truncatedLoadOptions = CKAngelScriptApi::LoadOptions();
    truncatedLoadOptions.Size = sizeof(CKAngelScriptLoadOptions) - 1;
    truncatedLoadOptions.ModuleName = "__CKAS_ManagerApiTruncatedLoadSelfTest";
    truncatedLoadOptions.Code = "int __ckas_truncated_load() { return 1; }\n";
    truncatedLoadOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(truncatedLoadOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected truncated LoadModule options to fail.";
        return false;
    }

    if (!ExpectStatus(api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result),
                      CKAS_OK,
                      "CompileModule",
                      &result,
                      error)) {
        return false;
    }
    if (api->UnregisterEngineExtension("__ckas_public_api_extension", &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected modules referencing an extension to block unregister.";
        return false;
    }
    if (api->CompileModule(moduleName, source, CKAS_COMPILE_DEFAULT, &result) != CKAS_ALREADYEXISTS) {
        error = "CKAngelScript API self-test expected duplicate CompileModule without replace to return ALREADYEXISTS.";
        return false;
    }
    if (api->CompileModule(moduleName, source, 0x80000000u, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected unknown CompileModule flags to fail.";
        return false;
    }
    const CKDWORD generationBeforeFailedReplace = api->GetModuleGeneration(moduleName);
    asIScriptFunction *stillBorrowedFunction = reinterpret_cast<asIScriptFunction *>(static_cast<uintptr_t>(1));
    if (api->CompileModule(moduleName,
                           "int __ckas_public_add(int value) { return value + ; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_COMPILEERROR ||
        api->GetModuleGeneration(moduleName) != generationBeforeFailedReplace ||
        api->BorrowFunctionByDecl(moduleName, "int __ckas_public_add(int)", &stillBorrowedFunction, &result) != CKAS_OK ||
        !stillBorrowedFunction) {
        error = "CKAngelScript API self-test expected failed module replacement to keep the old module and generation.";
        return false;
    }

    asIScriptModule *module = nullptr;
    if (!ExpectStatus(api->BorrowModule(moduleName, &module, &result), CKAS_OK, "BorrowModule", &result, error) ||
        !module ||
        !api->HasModule(moduleName)) {
        return false;
    }

    asIScriptFunction *borrowedFunction = nullptr;
    if (!ExpectStatus(api->BorrowFunctionByDecl(moduleName, "int __ckas_public_add(int)", &borrowedFunction, &result),
                      CKAS_OK,
                      "BorrowFunctionByDecl",
                      &result,
                      error) ||
        !borrowedFunction) {
        return false;
    }
    borrowedFunction = reinterpret_cast<asIScriptFunction *>(static_cast<uintptr_t>(1));
    if (api->BorrowFunctionByDecl(moduleName, "int __ckas_missing()", &borrowedFunction, &result) != CKAS_NOTFOUND ||
        borrowedFunction != nullptr) {
        error = "CKAngelScript API self-test expected missing borrowed function lookup to clear the out pointer.";
        return false;
    }
    if (api->BorrowFunctionByName(moduleName, "__ckas_public_overload", &borrowedFunction, &result) != CKAS_AMBIGUOUS ||
        borrowedFunction != nullptr) {
        error = "CKAngelScript API self-test expected overloaded borrowed function lookup to be ambiguous.";
        return false;
    }

    MetadataProbe metadataProbe;
    if (api->EnumerateMetadata(moduleName, ProbeMetadata, &metadataProbe, &result) != CKAS_OK ||
        metadataProbe.CallbackCount < 5 ||
        !metadataProbe.Type ||
        !metadataProbe.Method ||
        !metadataProbe.Function ||
        !metadataProbe.Global ||
        !metadataProbe.TypeProperty ||
        !metadataProbe.Declaration ||
        metadataProbe.NamespacedTypes != 2 ||
        metadataProbe.NamespacedMethods != 2) {
        error = "CKAngelScript API self-test expected metadata enumeration for type/method/function/global/property targets.";
        return false;
    }
    if (api->EnumerateMetadata("__CKAS_MissingMetadataModule", ProbeMetadata, &metadataProbe, &result) != CKAS_NOTFOUND ||
        api->EnumerateMetadata(moduleName, nullptr, nullptr, &result) != CKAS_INVALIDARGUMENT ||
        api->EnumerateMetadata(moduleName, StopMetadata, nullptr, &result) != CKAS_CANCELLED) {
        error = "CKAngelScript API self-test expected metadata enumeration failure statuses.";
        return false;
    }

    constexpr const char *loadModuleName = "__CKAS_ManagerApiLoadSelfTest";
    CKAngelScriptLoadOptions loadOptions = CKAngelScriptApi::LoadCodeOptions(
        loadModuleName,
        "int __ckas_public_loaded() { return 3; }\n",
        CKAS_LOAD_REPLACEEXISTING);
    loadOptions.Size = sizeof(loadOptions) + 16;
    if (!ExpectStatus(api->LoadModule(loadOptions, &result),
                      CKAS_OK,
                      "LoadModule code",
                      &result,
                      error) ||
        api->BorrowFunctionByDecl(loadModuleName, "int __ckas_public_loaded()", &borrowedFunction, &result) != CKAS_OK) {
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
    const std::string singleFilePath = singleFile.string();
    CKAngelScriptLoadOptions singleFileOptions =
        CKAngelScriptApi::LoadFileOptions(singleFileModuleName, singleFilePath.c_str(), CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(singleFileOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(singleFileModuleName, "int __ckas_public_file_loaded()", &borrowedFunction, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected single-file LoadModule to expose its function.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    api->UnloadModule(singleFileModuleName, nullptr);

    constexpr const char *multiFileModuleName = "__CKAS_ManagerApiMultiFileLoadSelfTest";
    const std::string multiFilePathA = multiFileA.string();
    const std::string multiFilePathB = multiFileB.string();
    const char *multiFiles[] = { multiFilePathA.c_str(), multiFilePathB.c_str() };
    CKAngelScriptLoadOptions multiFileOptions =
        CKAngelScriptApi::LoadFilesOptions(multiFileModuleName, multiFiles, 2, CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(multiFileOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(multiFileModuleName, "int __ckas_public_multi_b()", &borrowedFunction, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected multi-file LoadModule to expose its function.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    api->UnloadModule(multiFileModuleName, nullptr);

    constexpr const char *sourceSectionsModuleName = "__CKAS_ManagerApiSourceSectionsLoadSelfTest";
    const char *sourceSectionEntry =
        "#include \"lib/helper.as\"\n"
        "#include \"lib/helper.as\"\n"
        "int __ckas_public_source_sections_loaded() { return __ckas_snapshot_helper() + 1; }\n";
    const char *sourceSectionHelper =
        "[ckas_selftest_source_section_type]\n"
        "class __CKAS_SourceSectionMetadataType {}\n"
        "int __ckas_snapshot_helper() { return 20; }\n";
    CKAngelScriptSourceSection sourceSections[2] = {};
    sourceSections[0].Size = sizeof(sourceSections[0]);
    sourceSections[0].SectionName = "entry/main.as";
    sourceSections[0].Code = sourceSectionEntry;
    sourceSections[0].CodeSize = std::strlen(sourceSectionEntry);
    sourceSections[1].Size = sizeof(sourceSections[1]);
    sourceSections[1].SectionName = "entry/lib/helper.as";
    sourceSections[1].Code = sourceSectionHelper;
    sourceSections[1].CodeSize = std::strlen(sourceSectionHelper);
    CKAngelScriptLoadOptions sourceSectionOptions =
        CKAngelScriptApi::LoadSectionsOptions(sourceSectionsModuleName,
                                              sourceSections,
                                              2,
                                              CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(sourceSectionOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(sourceSectionsModuleName,
                                  "int __ckas_public_source_sections_loaded()",
                                  &borrowedFunction,
                                  &result) != CKAS_OK ||
        !borrowedFunction) {
        error = "CKAngelScript API self-test expected source-section LoadModule to resolve in-memory includes.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    MetadataProbe sourceSectionMetadataProbe;
    if (api->EnumerateMetadata(sourceSectionsModuleName, ProbeMetadata, &sourceSectionMetadataProbe, &result) != CKAS_OK ||
        !sourceSectionMetadataProbe.SourceSectionType) {
        error = "CKAngelScript API self-test expected source-section LoadModule to preserve metadata from included sections.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    api->UnloadModule(sourceSectionsModuleName, nullptr);

    CKAngelScriptSourceSection invalidSourceSection = {};
    invalidSourceSection.Size = sizeof(invalidSourceSection);
    invalidSourceSection.SectionName = "";
    invalidSourceSection.Code = sourceSectionEntry;
    CKAngelScriptLoadOptions invalidSourceSectionOptions =
        CKAngelScriptApi::LoadSectionsOptions("__CKAS_ManagerApiInvalidSourceSectionsSelfTest",
                                              &invalidSourceSection,
                                              1,
                                              CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(invalidSourceSectionOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with an invalid source section to fail.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }

    constexpr const char *defaultFileModuleName = "__CKAS_ManagerApiDefaultFileLoadSelfTest";
    CKAngelScriptLoadOptions defaultFileOptions = CKAngelScriptApi::LoadOptions();
    defaultFileOptions.ModuleName = defaultFileModuleName;
    defaultFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(defaultFileOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(defaultFileModuleName, "int __ckas_public_default_file_loaded()", &borrowedFunction, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected default-file LoadModule to expose its function.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    api->UnloadModule(defaultFileModuleName, nullptr);

    const char *invalidFiles[] = { multiFilePathA.c_str(), nullptr };
    CKAngelScriptLoadOptions invalidFileListOptions = CKAngelScriptApi::LoadOptions();
    invalidFileListOptions.ModuleName = "__CKAS_ManagerApiInvalidFileListSelfTest";
    invalidFileListOptions.Filenames = invalidFiles;
    invalidFileListOptions.FileCount = 2;
    invalidFileListOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(invalidFileListOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with an invalid file list entry to fail.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }

    CKAngelScriptLoadOptions conflictingOptions = CKAngelScriptApi::LoadOptions();
    conflictingOptions.ModuleName = "__CKAS_ManagerApiConflictingLoadSelfTest";
    conflictingOptions.Code = "int __ckas_public_conflict() { return 1; }\n";
    conflictingOptions.Filename = singleFilePath.c_str();
    conflictingOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(conflictingOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with multiple sources to fail.";
        RemoveTextFile(singleFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
        return false;
    }
    RemoveTextFile(singleFile);
    RemoveTextFile(multiFileA);
    RemoveTextFile(multiFileB);
    RemoveTextFile(defaultFile);

    CKAngelScriptFunctionOptions invalidFunctionOptions = CKAngelScriptApi::FunctionOptions();
    invalidFunctionOptions.ModuleName = moduleName;
    CKAngelScriptFunction *function = reinterpret_cast<CKAngelScriptFunction *>(static_cast<uintptr_t>(1));
    if (api->FindFunction(invalidFunctionOptions, &function, &result) != CKAS_INVALIDARGUMENT || function != nullptr) {
        error = "CKAngelScript API self-test expected missing function lookup key to fail and clear out pointer.";
        return false;
    }
    invalidFunctionOptions.FunctionName = "__ckas_public_add";
    invalidFunctionOptions.FunctionDecl = "int __ckas_public_add(int)";
    if (api->FindFunction(invalidFunctionOptions, &function, &result) != CKAS_INVALIDARGUMENT || function != nullptr) {
        error = "CKAngelScript API self-test expected dual function lookup keys to fail.";
        return false;
    }
    invalidFunctionOptions.FunctionDecl = nullptr;
    invalidFunctionOptions.Flags = 1;
    if (api->FindFunction(invalidFunctionOptions, &function, &result) != CKAS_INVALIDARGUMENT || function != nullptr) {
        error = "CKAngelScript API self-test expected unknown FindFunction flags to fail.";
        return false;
    }
    function = reinterpret_cast<CKAngelScriptFunction *>(static_cast<uintptr_t>(1));
    if (api->FindFunction(CKAngelScriptApi::FunctionByNameOptions(moduleName, "__ckas_public_overload"),
                          &function,
                          &result) != CKAS_AMBIGUOUS ||
        function != nullptr) {
        error = "CKAngelScript API self-test expected FunctionByNameOptions to preserve ambiguous overload semantics.";
        return false;
    }

    CKAngelScriptFunctionOptions addFunctionOptions =
        CKAngelScriptApi::FunctionByDeclOptions(moduleName, "int __ckas_public_add(int)");
    CKAngelScriptFunction *addFunction = nullptr;
    if (!ExpectStatus(api->FindFunction(addFunctionOptions, &addFunction, &result),
                      CKAS_OK,
                      "FindFunction add",
                      &result,
                      error) ||
        !addFunction) {
        return false;
    }

    {
        CKAngelScriptFunctionHandle addFunctionHandle;
        if (api->FindFunction(addFunctionOptions, addFunctionHandle, &result) != CKAS_OK ||
            !addFunctionHandle ||
            addFunctionHandle.Owner() != api.Handle()) {
            error = "CKAngelScript API self-test expected RAII function lookup to produce an owned handle.";
            api->ReleaseFunction(addFunction);
            return false;
        }
        if (api->FindFunctionByDecl(moduleName,
                                    "int __ckas_missing()",
                                    addFunctionHandle,
                                    &result) != CKAS_NOTFOUND ||
            addFunctionHandle) {
            error = "CKAngelScript API self-test expected failed RAII function lookup to clear the handle.";
            api->ReleaseFunction(addFunction);
            return false;
        }
    }

    IntExecutionData data;
    data.Input = 37;
    CKAngelScriptFunctionExecutionOptions executeOptions =
        CKAngelScriptApi::FunctionExecutionOptions(addFunction);
    CKAngelScriptExecutionStepOptions stepOptions =
        CKAngelScriptApi::ExecutionStepOptions(ConfigureIntArgument, ReadIntReturn, &data);
    CKAngelScriptExecution *execution = nullptr;
    executeOptions.Flags = 0x00000002u;
    execution = reinterpret_cast<CKAngelScriptExecution *>(static_cast<uintptr_t>(1));
    if (api->CreateFunctionExecution(executeOptions, &execution, &result) != CKAS_INVALIDARGUMENT ||
        execution != nullptr) {
        error = "CKAngelScript API self-test expected deleted function execution flags to be invalid.";
        api->ReleaseFunction(addFunction);
        return false;
    }
    executeOptions.Flags = CKAS_CALL_DEFAULT;

    {
        CKAngelScriptExecutionHandle executionHandle;
        if (api->CreateFunctionExecution(executeOptions, executionHandle, &result) != CKAS_OK ||
            !executionHandle ||
            executionHandle.Owner() != api.Handle()) {
            error = "CKAngelScript API self-test expected RAII execution creation to produce an owned handle.";
            api->ReleaseFunction(addFunction);
            return false;
        }
        executeOptions.Flags = 0x00000002u;
        if (api->CreateFunctionExecution(executeOptions, executionHandle, &result) != CKAS_INVALIDARGUMENT ||
            executionHandle) {
            error = "CKAngelScript API self-test expected failed RAII execution creation to clear the handle.";
            api->ReleaseFunction(addFunction);
            return false;
        }
        executeOptions.Flags = CKAS_CALL_DEFAULT;
    }

    if (!ExpectStatus(api->CreateFunctionExecution(executeOptions, &execution, &result),
                      CKAS_OK,
                      "CreateFunctionExecution",
                      &result,
                      error) ||
        !execution) {
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptExecutionStepOptions invalidStepOptions = stepOptions;
    invalidStepOptions.Size = 0;
    if (api->StartExecution(execution, invalidStepOptions, &result) != CKAS_INVALIDARGUMENT ||
        result.Status != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected zero-sized execution step options to fail.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (!ExpectStatus(api->StartExecution(execution, stepOptions, &result),
                      CKAS_OK,
                      "StartExecution",
                      &result,
                      error)) {
        api->ReleaseExecution(execution);
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAS_EXECUTIONSTATE state = CKAS_EXECUTION_FAILED;
    const CKAngelScriptResult *executionResult = nullptr;
    if (api->GetExecutionState(execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_FINISHED ||
        api->BorrowExecutionResult(execution, &executionResult, &result) != CKAS_OK ||
        !executionResult ||
        executionResult->AngelScriptCode != asEXECUTION_FINISHED ||
        data.Output != 42) {
        error = "CKAngelScript API self-test returned the wrong synchronous execution result.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (api->StartExecution(execution, nullptr, &result) != CKAS_INVALIDSTATE ||
        result.Status != CKAS_INVALIDSTATE ||
        api->ResumeExecution(execution, nullptr, &result) != CKAS_INVALIDSTATE ||
        result.Status != CKAS_INVALIDSTATE) {
        error = "CKAngelScript API self-test expected invalid execution transitions to return INVALIDSTATE.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);

    CKAngelScriptFunctionOptions ckuiFunctionOptions = CKAngelScriptApi::FunctionOptions();
    ckuiFunctionOptions.ModuleName = moduleName;
    ckuiFunctionOptions.FunctionDecl = "int __ckas_public_ckui_callback_struct()";
    CKAngelScriptFunction *ckuiFunction = nullptr;
    if (api->FindFunction(ckuiFunctionOptions, &ckuiFunction, &result) != CKAS_OK || !ckuiFunction) {
        error = "CKAngelScript API self-test could not find the CKUICallbackStruct smoke function.";
        api->ReleaseFunction(addFunction);
        return false;
    }

    CKAngelScriptFunctionExecutionOptions ckuiExecuteOptions =
        CKAngelScriptApi::FunctionExecutionOptions(ckuiFunction);
    IntExecutionData ckuiData;
    CKAngelScriptExecutionStepOptions ckuiStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &ckuiData);
    CKAngelScriptExecution *ckuiExecution = nullptr;
    if (api->CreateFunctionExecution(ckuiExecuteOptions, &ckuiExecution, &result) != CKAS_OK ||
        !ckuiExecution ||
        api->StartExecution(ckuiExecution, ckuiStepOptions, &result) != CKAS_OK ||
        ckuiData.Output != 1) {
        error = "CKAngelScript API self-test found invalid CKUICallbackStruct string ownership behavior.";
        if (ckuiExecution) {
            api->ReleaseExecution(ckuiExecution);
        }
        api->ReleaseFunction(ckuiFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(ckuiExecution);
    api->ReleaseFunction(ckuiFunction);

    CKAngelScriptFunctionOptions ckstrFunctionOptions = CKAngelScriptApi::FunctionOptions();
    ckstrFunctionOptions.ModuleName = moduleName;
    ckstrFunctionOptions.FunctionDecl = "int __ckas_public_ckstr_string_overloads()";
    CKAngelScriptFunction *ckstrFunction = nullptr;
    if (api->FindFunction(ckstrFunctionOptions, &ckstrFunction, &result) != CKAS_OK || !ckstrFunction) {
        error = "CKAngelScript API self-test could not find the CKStrupr/CKStrlwr string overload smoke function.";
        api->ReleaseFunction(addFunction);
        return false;
    }

    CKAngelScriptFunctionExecutionOptions ckstrExecuteOptions =
        CKAngelScriptApi::FunctionExecutionOptions(ckstrFunction);
    IntExecutionData ckstrData;
    CKAngelScriptExecutionStepOptions ckstrStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &ckstrData);
    CKAngelScriptExecution *ckstrExecution = nullptr;
    if (api->CreateFunctionExecution(ckstrExecuteOptions, &ckstrExecution, &result) != CKAS_OK ||
        !ckstrExecution ||
        api->StartExecution(ckstrExecution, ckstrStepOptions, &result) != CKAS_OK ||
        ckstrData.Output != 1) {
        error = "CKAngelScript API self-test found invalid CKStrupr/CKStrlwr string overload behavior.";
        if (ckstrExecution) {
            api->ReleaseExecution(ckstrExecution);
        }
        api->ReleaseFunction(ckstrFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(ckstrExecution);
    api->ReleaseFunction(ckstrFunction);

    CKAngelScriptFunctionOptions sideEffectFunctionOptions = CKAngelScriptApi::FunctionOptions();
    sideEffectFunctionOptions.ModuleName = moduleName;
    sideEffectFunctionOptions.FunctionDecl = "int __ckas_public_side_effect()";
    CKAngelScriptFunction *sideEffectFunction = nullptr;
    CKAngelScriptFunctionOptions sideEffectGetterOptions = CKAngelScriptApi::FunctionOptions();
    sideEffectGetterOptions.ModuleName = moduleName;
    sideEffectGetterOptions.FunctionDecl = "int __ckas_public_get_side_effect()";
    CKAngelScriptFunction *sideEffectGetter = nullptr;
    if (api->FindFunction(sideEffectFunctionOptions, &sideEffectFunction, &result) != CKAS_OK ||
        api->FindFunction(sideEffectGetterOptions, &sideEffectGetter, &result) != CKAS_OK ||
        !sideEffectFunction ||
        !sideEffectGetter) {
        error = "CKAngelScript API self-test failed to find side-effect probe functions.";
        if (sideEffectFunction) {
            api->ReleaseFunction(sideEffectFunction);
        }
        if (sideEffectGetter) {
            api->ReleaseFunction(sideEffectGetter);
        }
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptFunctionExecutionOptions rejectedOptions = CKAngelScriptApi::FunctionExecutionOptions(sideEffectFunction);
    CKAngelScriptExecutionStepOptions rejectedStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(RejectExecutionConfigure);
    execution = nullptr;
    if (api->CreateFunctionExecution(rejectedOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, rejectedStepOptions, &result) != CKAS_INVALIDARGUMENT ||
        result.Status != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected rejected ConfigureContext to fail before script execution.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(sideEffectFunction);
        api->ReleaseFunction(sideEffectGetter);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);
    CKAngelScriptFunctionExecutionOptions getterOptions = CKAngelScriptApi::FunctionExecutionOptions(sideEffectGetter);
    CKAngelScriptExecutionStepOptions getterStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &data);
    data.Output = -1;
    execution = nullptr;
    if (api->CreateFunctionExecution(getterOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, getterStepOptions, &result) != CKAS_OK ||
        data.Output != 0) {
        error = "CKAngelScript API self-test expected rejected ConfigureContext to prevent script side effects.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(sideEffectFunction);
        api->ReleaseFunction(sideEffectGetter);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);
    api->ReleaseFunction(sideEffectFunction);
    api->ReleaseFunction(sideEffectGetter);

    CKAngelScriptFunctionOptions extensionFunctionOptions = CKAngelScriptApi::FunctionOptions();
    extensionFunctionOptions.ModuleName = moduleName;
    extensionFunctionOptions.FunctionDecl = "int __ckas_public_extension()";
    CKAngelScriptFunction *extensionFunction = nullptr;
    if (!ExpectStatus(api->FindFunction(extensionFunctionOptions, &extensionFunction, &result),
                      CKAS_OK,
                      "FindFunction extension",
                      &result,
                      error) ||
        !extensionFunction) {
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptFunctionExecutionOptions extensionExecutionOptions =
        CKAngelScriptApi::FunctionExecutionOptions(extensionFunction);
    CKAngelScriptExecutionStepOptions extensionStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &data);
    data.Output = 0;
    execution = nullptr;
    if (api->CreateFunctionExecution(extensionExecutionOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, extensionStepOptions, &result) != CKAS_OK ||
        data.Output != 77) {
        error = "CKAngelScript API self-test returned the wrong extension result.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(extensionFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);
    api->ReleaseFunction(extensionFunction);

    CKAngelScriptFunctionOptions exceptionFunctionOptions = CKAngelScriptApi::FunctionOptions();
    exceptionFunctionOptions.ModuleName = moduleName;
    exceptionFunctionOptions.FunctionDecl = "int __ckas_public_exception()";
    CKAngelScriptFunction *exceptionFunction = nullptr;
    api->FindFunction(exceptionFunctionOptions, &exceptionFunction, &result);
    CKAngelScriptFunctionExecutionOptions exceptionExecutionOptions =
        CKAngelScriptApi::FunctionExecutionOptions(exceptionFunction);
    execution = nullptr;
    if (!exceptionFunction ||
        api->CreateFunctionExecution(exceptionExecutionOptions, &execution, &result) != CKAS_OK ||
        api->StartExecution(execution, nullptr, &result) != CKAS_EXECUTIONFAILED ||
        api->BorrowExecutionResult(execution, &executionResult, &result) != CKAS_OK ||
        !executionResult ||
        !executionResult->ErrorMessage ||
        !executionResult->StackTrace ||
        executionResult->AngelScriptCode != asEXECUTION_EXCEPTION) {
        error = "CKAngelScript API self-test expected script exception result details.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        if (exceptionFunction) {
            api->ReleaseFunction(exceptionFunction);
        }
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);
    api->ReleaseFunction(exceptionFunction);

    CKAngelScriptFunctionOptions asyncFunctionOptions = CKAngelScriptApi::FunctionOptions();
    asyncFunctionOptions.ModuleName = moduleName;
    asyncFunctionOptions.FunctionDecl = "int __ckas_public_async()";
    CKAngelScriptFunction *asyncFunction = nullptr;
    if (!ExpectStatus(api->FindFunction(asyncFunctionOptions, &asyncFunction, &result),
                      CKAS_OK,
                      "FindFunction async",
                      &result,
                      error) ||
        !asyncFunction) {
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptFunctionExecutionOptions asyncOptions = CKAngelScriptApi::FunctionExecutionOptions(asyncFunction);
    CKAngelScriptExecutionStepOptions asyncStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &data);
    data.Output = 0;
    execution = nullptr;
    if (api->CreateFunctionExecution(asyncOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, asyncStepOptions, &result) != CKAS_SUSPENDED ||
        data.Output != 0 ||
        api->GetExecutionState(execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_SUSPENDED) {
        error = "CKAngelScript API self-test expected async execution to suspend.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (api->UnloadModule(moduleName, &result) != CKAS_INUSE ||
        api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected active execution handles to block unload/replace.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (api->CancelExecution(execution, &result) != CKAS_OK ||
        result.Status != CKAS_OK ||
        api->BorrowExecutionResult(execution, &executionResult, &result) != CKAS_OK ||
        !executionResult ||
        executionResult->Status != CKAS_CANCELLED ||
        api->GetExecutionState(execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_CANCELLED) {
        error = "CKAngelScript API self-test expected cancellation to return OK and store a CANCELLED execution result.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);

    asyncOptions.Flags = CKAS_CALL_NO_SUSPEND;
    execution = nullptr;
    if (api->CreateFunctionExecution(asyncOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, asyncStepOptions, &result) != CKAS_UNSUPPORTED) {
        error = "CKAngelScript API self-test expected CKAS_CALL_NO_SUSPEND to reject suspended scripts.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);

    constexpr const char *releaseSuspendedModuleName = "__CKAS_ReleaseSuspendedExecutionSelfTest";
    const char *releaseSuspendedSource =
        "int __ckas_release_suspended_async() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 11; }\n";
    if (!ExpectStatus(api->CompileModule(releaseSuspendedModuleName,
                                         releaseSuspendedSource,
                                         CKAS_COMPILE_DEFAULT,
                                         &result),
                      CKAS_OK,
                      "CompileModule release suspended execution self-test",
                      &result,
                      error)) {
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptFunctionOptions releaseSuspendedFunctionOptions = CKAngelScriptApi::FunctionOptions();
    releaseSuspendedFunctionOptions.ModuleName = releaseSuspendedModuleName;
    releaseSuspendedFunctionOptions.FunctionDecl = "int __ckas_release_suspended_async()";
    CKAngelScriptFunction *releaseSuspendedFunction = nullptr;
    if (!ExpectStatus(api->FindFunction(releaseSuspendedFunctionOptions,
                                        &releaseSuspendedFunction,
                                        &result),
                      CKAS_OK,
                      "FindFunction release suspended execution",
                      &result,
                      error) ||
        !releaseSuspendedFunction) {
        api->UnloadModule(releaseSuspendedModuleName);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptFunctionExecutionOptions releaseSuspendedOptions =
        CKAngelScriptApi::FunctionExecutionOptions(releaseSuspendedFunction);
    execution = nullptr;
    if (api->CreateFunctionExecution(releaseSuspendedOptions, &execution, &result) != CKAS_OK ||
        !execution ||
        api->StartExecution(execution, asyncStepOptions, &result) != CKAS_SUSPENDED) {
        error = "CKAngelScript API self-test failed to create suspended execution release case.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(releaseSuspendedFunction);
        api->UnloadModule(releaseSuspendedModuleName);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (api->ReleaseExecution(execution, &result) != CKAS_OK ||
        result.Status != CKAS_OK ||
        api->UnloadModule(releaseSuspendedModuleName, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected releasing a suspended execution to abort and unblock unload.";
        api->ReleaseFunction(releaseSuspendedFunction);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseFunction(releaseSuspendedFunction);

    asyncOptions.Flags = CKAS_CALL_DEFAULT;
    data.Output = 0;
    execution = nullptr;
    api->CreateFunctionExecution(asyncOptions, &execution, &result);
    if (!execution || api->StartExecution(execution, asyncStepOptions, &result) != CKAS_SUSPENDED ||
        data.Output != 0) {
        error = "CKAngelScript API self-test failed to create a resumable async execution.";
        if (execution) {
            api->ReleaseExecution(execution);
        }
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (ScriptAsyncScheduler *scheduler = ScriptManager::GetManager(context)->GetAsyncScheduler()) {
        scheduler->Tick();
    }
    IntExecutionData resumeData;
    resumeData.Input = 0;
    resumeData.Output = 0;
    CKAngelScriptExecutionStepOptions resumeStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(MarkResumeHook, ReadIntReturn, &resumeData);
    if (api->ResumeExecution(execution, resumeStepOptions, &result) != CKAS_OK ||
        api->GetExecutionState(execution, &state, &result) != CKAS_OK ||
        state != CKAS_EXECUTION_FINISHED ||
        data.Output != 0 ||
        resumeData.Input != 1 ||
        resumeData.Output != 9) {
        error = "CKAngelScript API self-test expected resumed async execution to finish.";
        api->ReleaseExecution(execution);
        api->ReleaseFunction(asyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(execution);
    api->ReleaseFunction(asyncFunction);

    if (!ExpectStatus(api->UnloadModule(moduleName, &result),
                      CKAS_OK,
                      "UnloadModule with function symbols only",
                      &result,
                      error)) {
        api->ReleaseFunction(addFunction);
        return false;
    }
    execution = reinterpret_cast<CKAngelScriptExecution *>(static_cast<uintptr_t>(1));
    if (api->CreateFunctionExecution(executeOptions, &execution, &result) != CKAS_STALEHANDLE ||
        execution != nullptr) {
        error = "CKAngelScript API self-test expected function symbol handles to become stale after unload.";
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseFunction(addFunction);

    constexpr const char *badModuleName = "__CKAS_ManagerApiBadCompileSelfTest";
    if (api->CompileModule(badModuleName, "int __ckas_bad_compile( {", CKAS_COMPILE_REPLACEEXISTING, &result) !=
        CKAS_COMPILEERROR ||
        !ContainsStructuredCompileDiagnostic(result)) {
        error = "CKAngelScript API self-test expected compile errors to include structured AngelScript diagnostics.";
        api->UnloadModule(badModuleName, nullptr);
        return false;
    }
    api->UnloadModule(badModuleName, nullptr);

    constexpr const char *objectModuleName = "__CKAS_ManagerApiObjectSelfTest";
    const char *objectSource =
        "class __CKAS_PublicObject {\n"
        "  int base;\n"
        "  __CKAS_PublicObject() { base = 10; }\n"
        "  int Add(int value) { return base + value; }\n"
        "  int Over(int value) { return value; }\n"
        "  int Over(float value) { return int(value); }\n"
        "  bool Flip(bool value) { return !value; }\n"
        "  float Half(float value) { return value * 0.5f; }\n"
        "  string Echo(const string &in value) { return \"echo:\" + value; }\n"
        "  int UseHandle(__CKAS_PublicObject@ other) { return other is null ? -1 : other.Add(base); }\n"
        "  int Wait() { AsyncTask<void>@ delay = Async::Delay(1); Await(delay); return 5; }\n"
        "  void Boom() { array<int> values; values[1] = 1; }\n"
        "}\n"
        "namespace __CKAS_TestNS {\n"
        "  class __CKAS_NamespacedObject {\n"
        "    int Value() { return 7; }\n"
        "  }\n"
        "}\n"
        "namespace __CKAS_OtherNS {\n"
        "  class __CKAS_NamespacedObject {\n"
        "    int Value() { return 9; }\n"
        "  }\n"
        "}\n";
    if (!ExpectStatus(api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result),
                      CKAS_OK,
                      "CompileModule object ABI",
                      &result,
                      error)) {
        return false;
    }
    const CKDWORD objectGeneration = api->GetModuleGeneration(objectModuleName);
    CKAngelScriptObjectOptions objectOptions =
        CKAngelScriptApi::ObjectOptions(objectModuleName, "__CKAS_PublicObject");

    {
        CKAngelScriptObjectHandle objectHandle;
        if (api->CreateObject(objectOptions, objectHandle, &result) != CKAS_OK ||
            !objectHandle ||
            objectHandle.Owner() != api.Handle()) {
            error = "CKAngelScript API self-test expected RAII object creation to produce an owned handle.";
            return false;
        }

        CKAngelScriptMethodHandle methodHandle;
        if (api->FindObjectMethod(CKAngelScriptApi::MethodByDeclOptions(objectHandle.Get(), "int Add(int)"),
                                  methodHandle,
                                  &result) != CKAS_OK ||
            !methodHandle ||
            methodHandle.Owner() != api.Handle()) {
            error = "CKAngelScript API self-test expected RAII method lookup to produce an owned handle.";
            return false;
        }
        if (api->FindObjectMethodByDecl(objectHandle.Get(),
                                        "void Missing()",
                                        methodHandle,
                                        &result) != CKAS_NOTFOUND ||
            methodHandle) {
            error = "CKAngelScript API self-test expected failed RAII method lookup to clear the handle.";
            return false;
        }
        if (api->CreateObject(CKAngelScriptApi::ObjectOptions(objectModuleName, "__CKAS_MissingObject"),
                              objectHandle,
                              &result) != CKAS_NOTFOUND ||
            objectHandle) {
            error = "CKAngelScript API self-test expected failed RAII object creation to clear the handle.";
            return false;
        }
        if (api->CreateObject(CKAngelScriptApi::ObjectOptionsByNamespace(objectModuleName,
                                                                         "__CKAS_TestNS",
                                                                         "__CKAS_NamespacedObject"),
                              objectHandle,
                              &result) != CKAS_OK ||
            !objectHandle) {
            error = "CKAngelScript API self-test failed to create a namespaced object by namespace.";
            return false;
        }
        if (api->CreateObject(CKAngelScriptApi::ObjectOptionsByDecl(objectModuleName,
                                                                    "__CKAS_TestNS::__CKAS_NamespacedObject"),
                              objectHandle,
                              &result) != CKAS_OK ||
            !objectHandle) {
            error = "CKAngelScript API self-test failed to create a namespaced object by type declaration.";
            return false;
        }
        CKAngelScriptObjectOptions invalidNamespacedDecl =
            CKAngelScriptApi::ObjectOptionsByDecl(objectModuleName,
                                                  "__CKAS_TestNS::__CKAS_NamespacedObject");
        invalidNamespacedDecl.ClassNamespace = "__CKAS_TestNS";
        if (api->CreateObject(invalidNamespacedDecl, objectHandle, &result) != CKAS_INVALIDARGUMENT ||
            objectHandle) {
            error = "CKAngelScript API self-test expected ClassNamespace with TypeDecl to fail.";
            return false;
        }
    }

    ObjectExecutionData namespaceObjectData;
    CKAngelScriptObject *namespaceObject = nullptr;
    CKAngelScriptObject *otherNamespaceObject = nullptr;
    CKAngelScriptMethod *namespaceValueMethod = nullptr;
    CKAngelScriptMethod *otherNamespaceValueMethod = nullptr;
    if (api->CreateObject(CKAngelScriptApi::ObjectOptionsByNamespace(objectModuleName,
                                                                     "__CKAS_TestNS",
                                                                     "__CKAS_NamespacedObject"),
                          &namespaceObject,
                          &result) != CKAS_OK ||
        !namespaceObject ||
        api->CreateObject(CKAngelScriptApi::ObjectOptionsByNamespace(objectModuleName,
                                                                     "__CKAS_OtherNS",
                                                                     "__CKAS_NamespacedObject"),
                          &otherNamespaceObject,
                          &result) != CKAS_OK ||
        !otherNamespaceObject) {
        error = "CKAngelScript API self-test failed to create same-name namespaced objects.";
        if (namespaceObject)
            api->ReleaseObject(namespaceObject);
        if (otherNamespaceObject)
            api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    if (api->FindObjectMethod(CKAngelScriptApi::MethodByDeclOptions(namespaceObject, "int Value()"),
                              &namespaceValueMethod,
                              &result) != CKAS_OK ||
        !namespaceValueMethod ||
        api->FindObjectMethod(CKAngelScriptApi::MethodByDeclOptions(otherNamespaceObject, "int Value()"),
                              &otherNamespaceValueMethod,
                              &result) != CKAS_OK ||
        !otherNamespaceValueMethod) {
        error = "CKAngelScript API self-test failed to find same-name namespaced object methods.";
        if (namespaceValueMethod)
            api->ReleaseMethod(namespaceValueMethod);
        if (otherNamespaceValueMethod)
            api->ReleaseMethod(otherNamespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    CKAngelScriptObjectMethodExecuteOptions namespaceCall =
        CKAngelScriptApi::ObjectMethodExecuteOptions(namespaceObject,
                                                    namespaceValueMethod,
                                                    nullptr,
                                                    ReadObjectInt,
                                                    &namespaceObjectData,
                                                    CKAS_CALL_NO_SUSPEND);
    namespaceObjectData.IntOutput = 0;
    if (api->CallObjectMethod(namespaceCall, &result) != CKAS_OK || namespaceObjectData.IntOutput != 7) {
        error = "CKAngelScript API self-test expected first namespace object method result.";
        api->ReleaseMethod(namespaceValueMethod);
        api->ReleaseMethod(otherNamespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    namespaceCall.Object = otherNamespaceObject;
    namespaceCall.Method = otherNamespaceValueMethod;
    namespaceObjectData.IntOutput = 0;
    if (api->CallObjectMethod(namespaceCall, &result) != CKAS_OK || namespaceObjectData.IntOutput != 9) {
        error = "CKAngelScript API self-test expected second namespace object method result.";
        api->ReleaseMethod(namespaceValueMethod);
        api->ReleaseMethod(otherNamespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    namespaceCall.Object = otherNamespaceObject;
    namespaceCall.Method = namespaceValueMethod;
    if (api->CallObjectMethod(namespaceCall, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected namespace-mismatched object/method handles to fail.";
        api->ReleaseMethod(namespaceValueMethod);
        api->ReleaseMethod(otherNamespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    if (api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected namespaced object handles to block module replacement.";
        api->ReleaseMethod(namespaceValueMethod);
        api->ReleaseMethod(otherNamespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        api->ReleaseObject(otherNamespaceObject);
        return false;
    }
    api->ReleaseMethod(otherNamespaceValueMethod);
    api->ReleaseObject(namespaceObject);
    api->ReleaseObject(otherNamespaceObject);
    if (api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_OK ||
        api->CreateObject(CKAngelScriptApi::ObjectOptionsByNamespace(objectModuleName,
                                                                     "__CKAS_TestNS",
                                                                     "__CKAS_NamespacedObject"),
                          &namespaceObject,
                          &result) != CKAS_OK ||
        !namespaceObject) {
        error = "CKAngelScript API self-test failed to replace object module after releasing namespaced objects.";
        api->ReleaseMethod(namespaceValueMethod);
        return false;
    }
    namespaceCall.Object = namespaceObject;
    namespaceCall.Method = namespaceValueMethod;
    if (api->CallObjectMethod(namespaceCall, &result) != CKAS_STALEHANDLE) {
        error = "CKAngelScript API self-test expected namespaced method handles to become stale after module replacement.";
        api->ReleaseMethod(namespaceValueMethod);
        api->ReleaseObject(namespaceObject);
        return false;
    }
    api->ReleaseMethod(namespaceValueMethod);
    api->ReleaseObject(namespaceObject);

    CKAngelScriptObject *object = nullptr;
    if (api->CreateObject(objectOptions, &object, &result) != CKAS_OK || !object) {
        error = "CKAngelScript API self-test failed to create an object handle.";
        return false;
    }

    CKAngelScriptMethodOptions methodOptions = CKAngelScriptApi::MethodOptions();
    methodOptions.Object = object;
    CKAngelScriptMethod *method = reinterpret_cast<CKAngelScriptMethod *>(static_cast<uintptr_t>(1));
    if (api->FindObjectMethod(methodOptions, &method, &result) != CKAS_INVALIDARGUMENT || method != nullptr) {
        error = "CKAngelScript API self-test expected missing method lookup key to fail and clear out pointer.";
        api->ReleaseObject(object);
        return false;
    }
    methodOptions.MethodName = "Add";
    methodOptions.MethodDecl = "int Add(int)";
    if (api->FindObjectMethod(methodOptions, &method, &result) != CKAS_INVALIDARGUMENT || method != nullptr) {
        error = "CKAngelScript API self-test expected dual method lookup keys to fail.";
        api->ReleaseObject(object);
        return false;
    }
    methodOptions = CKAngelScriptApi::MethodByNameOptions(object, "Over");
    if (api->FindObjectMethod(methodOptions, &method, &result) != CKAS_AMBIGUOUS || method != nullptr) {
        error = "CKAngelScript API self-test expected overloaded method lookup to be ambiguous.";
        api->ReleaseObject(object);
        return false;
    }
    methodOptions = CKAngelScriptApi::MethodByDeclOptions(object, "void Missing()");
    if (api->FindObjectMethod(methodOptions, &method, &result) != CKAS_NOTFOUND || method != nullptr) {
        error = "CKAngelScript API self-test expected missing method lookup to return NOTFOUND.";
        api->ReleaseObject(object);
        return false;
    }
    methodOptions = CKAngelScriptApi::MethodByDeclOptions(object, "int Add(int)");
    CKAngelScriptMethod *addMethod = nullptr;
    if (api->FindObjectMethod(methodOptions, &addMethod, &result) != CKAS_OK || !addMethod) {
        error = "CKAngelScript API self-test failed to find Add object method.";
        api->ReleaseObject(object);
        return false;
    }

    ObjectExecutionData objectData;
    objectData.IntInput = 32;
    CKAngelScriptObjectMethodExecuteOptions objectCall =
        CKAngelScriptApi::ObjectMethodExecuteOptions(
            object,
            addMethod,
            WriteObjectInt,
            ReadObjectInt,
            &objectData,
            CKAS_CALL_NO_SUSPEND);
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.IntOutput != 42) {
        error = "CKAngelScript API self-test expected object int result to be 42.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Flags = 0x00000004u;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected deleted object method flags to be invalid.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Flags = CKAS_CALL_NO_SUSPEND;
    objectCall.WriteArgs = WriteObjectIntAsBool;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_TYPEMISMATCH) {
        error = "CKAngelScript API self-test expected object arg writer type mismatch.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.WriteArgs = WriteObjectInt;
    objectData.BoolOutput = TRUE;
    objectCall.ReadResult = ReadObjectBool;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_TYPEMISMATCH ||
        objectData.BoolOutput != FALSE) {
        error = "CKAngelScript API self-test expected result reader type mismatch to clear output.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.ReadResult = ReadObjectInt;

    methodOptions.MethodDecl = "int UseHandle(__CKAS_PublicObject@)";
    CKAngelScriptMethod *handleMethod = nullptr;
    objectData.ObjectInput = object;
    objectData.IntOutput = 0;
    if (api->FindObjectMethod(methodOptions, &handleMethod, &result) != CKAS_OK || !handleMethod) {
        error = "CKAngelScript API self-test failed to find object-handle arg method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = handleMethod;
    objectCall.WriteArgs = WriteObjectHandle;
    objectCall.ReadResult = ReadObjectInt;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.IntOutput != 20) {
        error = "CKAngelScript API self-test expected object handle arg round-trip.";
        api->ReleaseMethod(handleMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(handleMethod);

    methodOptions.MethodDecl = "bool Flip(bool)";
    CKAngelScriptMethod *boolMethod = nullptr;
    objectData.BoolInput = TRUE;
    objectData.BoolOutput = TRUE;
    if (api->FindObjectMethod(methodOptions, &boolMethod, &result) != CKAS_OK || !boolMethod) {
        error = "CKAngelScript API self-test failed to find Flip object method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = boolMethod;
    objectCall.WriteArgs = WriteObjectBool;
    objectCall.ReadResult = ReadObjectBool;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.BoolOutput != FALSE) {
        error = "CKAngelScript API self-test expected bool object method round-trip.";
        api->ReleaseMethod(boolMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(boolMethod);

    methodOptions.MethodDecl = "float Half(float)";
    CKAngelScriptMethod *floatMethod = nullptr;
    objectData.FloatInput = 5.0f;
    objectData.FloatOutput = 0.0f;
    if (api->FindObjectMethod(methodOptions, &floatMethod, &result) != CKAS_OK || !floatMethod) {
        error = "CKAngelScript API self-test failed to find Half object method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = floatMethod;
    objectCall.WriteArgs = WriteObjectFloat;
    objectCall.ReadResult = ReadObjectFloat;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK || objectData.FloatOutput != 2.5f) {
        error = "CKAngelScript API self-test expected float object method round-trip.";
        api->ReleaseMethod(floatMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(floatMethod);

    methodOptions.MethodDecl = "string Echo(const string &in)";
    CKAngelScriptMethod *echoMethod = nullptr;
    objectData.StringInput = "hello";
    objectData.StringOutput[0] = '\0';
    objectData.RequiredSize = 0;
    if (api->FindObjectMethod(methodOptions, &echoMethod, &result) != CKAS_OK || !echoMethod) {
        error = "CKAngelScript API self-test failed to find Echo object method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = echoMethod;
    objectCall.WriteArgs = WriteObjectString;
    objectCall.ReadResult = ReadObjectStringTooSmall;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_BUFFERTOOSMALL || objectData.RequiredSize == 0) {
        error = "CKAngelScript API self-test expected string result buffer-too-small diagnostics.";
        api->ReleaseMethod(echoMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.ReadResult = ReadObjectString;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK ||
        std::string(objectData.StringOutput) != "echo:hello") {
        error = "CKAngelScript API self-test expected string object method round-trip.";
        api->ReleaseMethod(echoMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(echoMethod);

    methodOptions.MethodDecl = "void Boom()";
    CKAngelScriptMethod *boomMethod = nullptr;
    if (api->FindObjectMethod(methodOptions, &boomMethod, &result) != CKAS_OK || !boomMethod) {
        error = "CKAngelScript API self-test failed to find Boom object method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = boomMethod;
    objectCall.WriteArgs = nullptr;
    objectCall.ReadResult = nullptr;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_EXECUTIONFAILED ||
        !result.ErrorMessage ||
        !result.StackTrace) {
        error = "CKAngelScript API self-test expected object method exceptions to include diagnostics.";
        api->ReleaseMethod(boomMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(boomMethod);

    methodOptions.MethodDecl = "int Wait()";
    CKAngelScriptMethod *waitMethod = nullptr;
    if (api->FindObjectMethod(methodOptions, &waitMethod, &result) != CKAS_OK || !waitMethod) {
        error = "CKAngelScript API self-test failed to find Wait object method.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.Method = waitMethod;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_UNSUPPORTED) {
        error = "CKAngelScript API self-test expected suspended synchronous object method calls to be unsupported.";
        api->ReleaseMethod(waitMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(waitMethod);

    objectCall.Method = addMethod;
    objectCall.WriteArgs = WriteObjectInt;
    objectCall.ReadResult = ReadObjectInt;

    if (api->UnloadModule(objectModuleName, &result) != CKAS_INUSE ||
        api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected object handles to block module unload/replace.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseObject(object);
    if (!ExpectStatus(api->UnloadModule(objectModuleName, &result),
                      CKAS_OK,
                      "UnloadModule object with method symbol only",
                      &result,
                      error) ||
        api->GetModuleGeneration(objectModuleName) == objectGeneration) {
        api->ReleaseMethod(addMethod);
        return false;
    }
    const CKDWORD objectGenerationBeforeReplace = api->GetModuleGeneration(objectModuleName);
    if (api->CompileModule(objectModuleName, objectSource, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_OK ||
        api->GetModuleGeneration(objectModuleName) != objectGenerationBeforeReplace + 1 ||
        api->CreateObject(objectOptions, &object, &result) != CKAS_OK ||
        !object) {
        error = "CKAngelScript API self-test failed to recreate object module.";
        api->ReleaseMethod(addMethod);
        return false;
    }
    objectCall.Object = object;
    objectCall.Method = addMethod;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_STALEHANDLE) {
        error = "CKAngelScript API self-test expected method symbol handle to become stale after module replacement.";
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    api->ReleaseMethod(addMethod);
    api->ReleaseObject(object);
    api->UnloadModule(objectModuleName, nullptr);

    if (api->UnregisterEngineExtension("__ckas_missing_extension", &result) != CKAS_NOTFOUND) {
        error = "CKAngelScript API self-test expected missing extension unregister to return NOTFOUND.";
        return false;
    }
    if (!ExpectStatus(api->UnregisterEngineExtension("__ckas_public_api_extension", &result),
                      CKAS_OK,
                      "UnregisterEngineExtension",
                      &result,
                      error)) {
        return false;
    }
    if (HasGlobalFunctionInNamespace(engine, "CKASExtensionSelfTest", "int Value()")) {
        error = "CKAngelScript API self-test expected unregister to remove current engine extension symbols.";
        return false;
    }
    return true;
}
