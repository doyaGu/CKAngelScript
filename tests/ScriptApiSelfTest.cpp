#include "ScriptSelfTests.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptCache.h"
#include "ScriptInvoker.h"
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
    if (std::strcmp(name, "CKAngelScriptInitImportBindOptions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitImportBindOptions);
    }
    if (std::strcmp(name, "CKAngelScriptInitBytecodeSaveOptions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitBytecodeSaveOptions);
    }
    if (std::strcmp(name, "CKAngelScriptInitBytecodeLoadOptions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitBytecodeLoadOptions);
    }
    if (std::strcmp(name, "CKAngelScriptInitModuleFingerprint") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptInitModuleFingerprint);
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
    if (std::strcmp(name, "CKAngelScriptGetImportedFunctionCount") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptGetImportedFunctionCount);
    }
    if (std::strcmp(name, "CKAngelScriptEnumerateImportedFunctions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptEnumerateImportedFunctions);
    }
    if (std::strcmp(name, "CKAngelScriptBindImportedFunction") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptBindImportedFunction);
    }
    if (std::strcmp(name, "CKAngelScriptBindAllImportedFunctions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptBindAllImportedFunctions);
    }
    if (std::strcmp(name, "CKAngelScriptUnbindImportedFunction") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptUnbindImportedFunction);
    }
    if (std::strcmp(name, "CKAngelScriptUnbindAllImportedFunctions") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptUnbindAllImportedFunctions);
    }
    if (std::strcmp(name, "CKAngelScriptSaveModuleBytecode") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptSaveModuleBytecode);
    }
    if (std::strcmp(name, "CKAngelScriptLoadModuleBytecode") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptLoadModuleBytecode);
    }
    if (std::strcmp(name, "CKAngelScriptEnumerateBoundImportEdges") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptEnumerateBoundImportEdges);
    }
    if (std::strcmp(name, "CKAngelScriptEnumerateModuleIncludeEdges") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptEnumerateModuleIncludeEdges);
    }
    if (std::strcmp(name, "CKAngelScriptGetModuleFingerprint") == 0) {
        return reinterpret_cast<CKAngelScriptRawProc>(&CKAngelScriptGetModuleFingerprint);
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

struct ReentrantMetadataProbe {
    CKAngelScriptApi *Api = nullptr;
    const char *ModuleName = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    CKAS_STATUS BytecodeSaveStatus = CKAS_OK;
    CKAS_STATUS BytecodeWriteReentryStatus = CKAS_OK;
    CKAS_STATUS BytecodeNestedSaveStatus = CKAS_OK;
    size_t BytecodeSize = 0;
    CKDWORD CallbackCount = 0;
};

struct ReentrantMetadataStringProbe {
    CKAngelScriptApi *Api = nullptr;
    const char *ModuleName = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    bool GlobalDeclaration = false;
    bool TypePropertyDeclaration = false;
    CKDWORD CallbackCount = 0;
};

struct ImportProbe {
    bool SawImport = false;
    CKDWORD CallbackCount = 0;
};

struct BoundImportEdgeProbe {
    const char *ExpectedImportModuleName = "__CKAS_ImportConsumer";
    const char *ExpectedSourceModuleName = "__CKAS_ImportProvider";
    const char *ExpectedFunctionName = "__ckas_import_add";
    CKDWORD ExpectedImportIndex = 0;
    bool SawEdge = false;
    CKDWORD CallbackCount = 0;
};

struct IncludeEdgeProbe {
    const char *ExpectedModuleName = "__CKAS_ManagerApiSourceSectionsLoadSelfTest";
    const char *ExpectedFromSection = "entry/main.as";
    const char *ExpectedToSection = "entry/lib/helper.as";
    CKBOOL ExpectedResolvedFromSnapshot = TRUE;
    bool SawHelperInclude = false;
    CKDWORD CallbackCount = 0;
};

struct ReentrantImportStringProbe {
    CKAngelScriptApi *Api = nullptr;
    const char *ProviderModuleName = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    bool SawImport = false;
    CKDWORD CallbackCount = 0;
};

struct BytecodeBuffer {
    std::vector<unsigned char> Bytes;
    size_t Offset = 0;
};

struct ReentrantBytecodeWriteProbe {
    BytecodeBuffer Buffer;
    CKAngelScriptApi *Api = nullptr;
    const char *ModuleName = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    CKAS_STATUS BytecodeSaveReentryStatus = CKAS_OK;
};

struct ReentrantExecutionCallbackProbe {
    CKAngelScriptApi *Api = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    CKDWORD CallbackCount = 0;
    int Input = 0;
    int Output = 0;
};

struct ReentrantObjectCallProbe {
    CKAngelScriptApi *Api = nullptr;
    CKAS_STATUS ReentryStatus = CKAS_OK;
    CKDWORD CallbackCount = 0;
    int Input = 0;
    int Output = 0;
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

CKAS_STATUS ConfigureIntArgumentWithReentry(asIScriptContext *ctx, void *userData) {
    auto *probe = static_cast<ReentrantExecutionCallbackProbe *>(userData);
    if (!ctx || !probe || !probe->Api) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    probe->ReentryStatus =
        probe->Api->CompileModule("__CKAS_ExecutionCallbackReentry",
                                  "int __ckas_execution_callback_reentry() { return 1; }\n",
                                  CKAS_COMPILE_REPLACEEXISTING,
                                  nullptr);
    return ctx->SetArgDWord(0, static_cast<asDWORD>(probe->Input)) >= 0
               ? CKAS_OK
               : CKAS_EXECUTIONFAILED;
}

CKAS_STATUS ReadIntReturnWithReentryProbe(asIScriptContext *ctx, void *userData) {
    auto *probe = static_cast<ReentrantExecutionCallbackProbe *>(userData);
    if (!ctx || !probe) {
        return CKAS_INVALIDARGUMENT;
    }
    probe->Output = static_cast<int>(ctx->GetReturnDWord());
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

CKAS_STATUS WriteObjectIntWithReentry(CKAngelScriptArgWriter *writer, void *userData) {
    auto *probe = static_cast<ReentrantObjectCallProbe *>(userData);
    if (!probe || !probe->Api) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    probe->ReentryStatus =
        probe->Api->CompileModule("__CKAS_ObjectCallbackReentry",
                                  "int __ckas_object_callback_reentry() { return 1; }\n",
                                  CKAS_COMPILE_REPLACEEXISTING,
                                  nullptr);
    return CKAngelScriptArgSetInt(writer, 0, probe->Input);
}

CKAS_STATUS ReadObjectInt(CKAngelScriptResultReader *reader, void *userData) {
    auto *data = static_cast<ObjectExecutionData *>(userData);
    return CKAngelScriptResultGetInt(reader, data ? &data->IntOutput : nullptr);
}

CKAS_STATUS ReadObjectIntWithReentryProbe(CKAngelScriptResultReader *reader, void *userData) {
    auto *probe = static_cast<ReentrantObjectCallProbe *>(userData);
    return CKAngelScriptResultGetInt(reader, probe ? &probe->Output : nullptr);
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

CKAS_STATUS ClobberAngelScriptDeclarationScratch(CKAngelScriptApi *api,
                                                 const char *moduleName,
                                                 const char *functionDecl) {
    if (!api || !moduleName || !functionDecl) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptModule *module = nullptr;
    const CKAS_STATUS status = api->BorrowModule(moduleName, &module, nullptr);
    if (status != CKAS_OK || !module) {
        return status == CKAS_OK ? CKAS_NOTFOUND : status;
    }
    asIScriptFunction *function = module->GetFunctionByDecl(functionDecl);
    if (!function) {
        return CKAS_NOTFOUND;
    }
    const char *declaration = function->GetDeclaration(false, true, true);
    return declaration && declaration[0] != '\0' ? CKAS_OK : CKAS_NOTFOUND;
}

CKAS_STATUS ProbeMetadataStringsAfterReadOnlyReentry(const CKAngelScriptMetadataEntry *entry,
                                                     CKDWORD metadataIndex,
                                                     const char *metadata,
                                                     void *userData) {
    auto *probe = static_cast<ReentrantMetadataStringProbe *>(userData);
    if (!probe || !probe->Api || !entry || entry->Size < sizeof(*entry) ||
        metadataIndex >= entry->MetadataCount || !metadata) {
        return CKAS_INVALIDARGUMENT;
    }

    ++probe->CallbackCount;
    if (CkasStringEquals(metadata, "ckas_selftest_global") &&
        entry->Target == CKAS_METADATA_GLOBAL_VARIABLE) {
        probe->ReentryStatus =
            ClobberAngelScriptDeclarationScratch(probe->Api,
                                                 probe->ModuleName,
                                                 "int __ckas_public_metadata_global()");
        if (probe->ReentryStatus != CKAS_OK) {
            return probe->ReentryStatus;
        }
        probe->GlobalDeclaration =
            CkasStringEquals(entry->Name, "__ckas_public_metadata_value") &&
            CkasStringContains(entry->Declaration, "__ckas_public_metadata_value");
    } else if (CkasStringEquals(metadata, "ckas_selftest_property") &&
               entry->Target == CKAS_METADATA_TYPE_PROPERTY) {
        probe->ReentryStatus =
            ClobberAngelScriptDeclarationScratch(probe->Api,
                                                 probe->ModuleName,
                                                 "int __ckas_public_metadata_global()");
        if (probe->ReentryStatus != CKAS_OK) {
            return probe->ReentryStatus;
        }
        probe->TypePropertyDeclaration =
            CkasStringEquals(entry->ParentTypeName, "__CKAS_PublicMetadataType") &&
            CkasStringEquals(entry->Name, "Value") &&
            CkasStringContains(entry->Declaration, "Value");
    }

    return CKAS_OK;
}

CKAS_STATUS WriteBytecodeWithReentry(const void *data, size_t size, void *userData);

CKAS_STATUS MutateFromMetadataCallback(const CKAngelScriptMetadataEntry *, CKDWORD, const char *, void *userData) {
    auto *probe = static_cast<ReentrantMetadataProbe *>(userData);
    if (!probe || !probe->Api) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    probe->ReentryStatus =
        probe->Api->CompileModule("__CKAS_MetadataCallbackReentry",
                                  "int __ckas_metadata_callback_reentry() { return 1; }\n",
                                  CKAS_COMPILE_REPLACEEXISTING,
                                  nullptr);
    if (probe->CallbackCount == 1) {
        ReentrantBytecodeWriteProbe bytecodeWrite;
        bytecodeWrite.Api = probe->Api;
        bytecodeWrite.ModuleName = probe->ModuleName;
        probe->BytecodeSaveStatus =
            probe->Api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(probe->ModuleName,
                                                                                  WriteBytecodeWithReentry,
                                                                                  &bytecodeWrite),
                                           nullptr);
        probe->BytecodeWriteReentryStatus = bytecodeWrite.ReentryStatus;
        probe->BytecodeNestedSaveStatus = bytecodeWrite.BytecodeSaveReentryStatus;
        probe->BytecodeSize = bytecodeWrite.Buffer.Bytes.size();
    }
    return CKAS_OK;
}

CKAS_STATUS ProbeImport(const CKAngelScriptImportEntry *entry, void *userData) {
    auto *probe = static_cast<ImportProbe *>(userData);
    if (!probe || !entry || entry->Size < sizeof(*entry) || !entry->Declaration || !entry->SourceModuleName) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    if (entry->Index == 0 &&
        CkasStringContains(entry->Declaration, "__ckas_import_add") &&
        CkasStringEquals(entry->SourceModuleName, "__CKAS_ImportProvider")) {
        probe->SawImport = true;
    }
    return CKAS_OK;
}

CKAS_STATUS ProbeImportAfterReadOnlyReentry(const CKAngelScriptImportEntry *entry, void *userData) {
    auto *probe = static_cast<ReentrantImportStringProbe *>(userData);
    if (!probe || !probe->Api || !entry || entry->Size < sizeof(*entry)) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    probe->ReentryStatus =
        ClobberAngelScriptDeclarationScratch(probe->Api,
                                             probe->ProviderModuleName,
                                             "int __ckas_import_alias(int)");
    if (probe->ReentryStatus != CKAS_OK) {
        return probe->ReentryStatus;
    }
    if (entry->Index == 0 &&
        CkasStringContains(entry->Declaration, "__ckas_import_add") &&
        CkasStringEquals(entry->SourceModuleName, probe->ProviderModuleName)) {
        probe->SawImport = true;
    }
    return CKAS_OK;
}

CKAS_STATUS ProbeBoundImportEdge(const CKAngelScriptBoundImportEdge *edge, void *userData) {
    auto *probe = static_cast<BoundImportEdgeProbe *>(userData);
    if (!probe || !edge || edge->Size < sizeof(*edge) ||
        !edge->ImportModuleName || !edge->SourceModuleName || !edge->FunctionDecl) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    if (edge->ImportIndex == probe->ExpectedImportIndex &&
        CkasStringEquals(edge->ImportModuleName, probe->ExpectedImportModuleName) &&
        CkasStringEquals(edge->SourceModuleName, probe->ExpectedSourceModuleName) &&
        CkasStringContains(edge->FunctionDecl, probe->ExpectedFunctionName)) {
        probe->SawEdge = true;
    }
    return CKAS_OK;
}

CKAS_STATUS ProbeIncludeEdge(const CKAngelScriptIncludeEdge *edge, void *userData) {
    auto *probe = static_cast<IncludeEdgeProbe *>(userData);
    if (!probe || !edge || edge->Size < sizeof(*edge) ||
        !edge->ModuleName || !edge->FromSection || !edge->ToSection) {
        return CKAS_INVALIDARGUMENT;
    }
    ++probe->CallbackCount;
    if (CkasStringEquals(edge->ModuleName, probe->ExpectedModuleName) &&
        CkasStringEquals(edge->FromSection, probe->ExpectedFromSection) &&
        CkasStringEquals(edge->ToSection, probe->ExpectedToSection) &&
        edge->ResolvedFromSnapshot == probe->ExpectedResolvedFromSnapshot) {
        probe->SawHelperInclude = true;
    }
    return CKAS_OK;
}

CKAS_STATUS WriteBytecode(const void *data, size_t size, void *userData) {
    auto *buffer = static_cast<BytecodeBuffer *>(userData);
    if (!buffer || (!data && size > 0)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (size > 0) {
        const auto *bytes = static_cast<const unsigned char *>(data);
        buffer->Bytes.insert(buffer->Bytes.end(), bytes, bytes + size);
    }
    return CKAS_OK;
}

CKAS_STATUS WriteBytecodeWithReentry(const void *data, size_t size, void *userData) {
    auto *probe = static_cast<ReentrantBytecodeWriteProbe *>(userData);
    if (!probe || !probe->Api) {
        return CKAS_INVALIDARGUMENT;
    }
    probe->ReentryStatus =
        probe->Api->CompileModule("__CKAS_BytecodeCallbackReentry",
                                  "int __ckas_bytecode_callback_reentry() { return 1; }\n",
                                  CKAS_COMPILE_REPLACEEXISTING,
                                  nullptr);
    BytecodeBuffer nestedSaveBuffer;
    probe->BytecodeSaveReentryStatus =
        probe->Api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(probe->ModuleName,
                                                                              WriteBytecode,
                                                                              &nestedSaveBuffer),
                                       nullptr);
    return WriteBytecode(data, size, &probe->Buffer);
}

CKAS_STATUS ReadBytecode(void *data, size_t size, void *userData) {
    auto *buffer = static_cast<BytecodeBuffer *>(userData);
    if (!buffer || (!data && size > 0) || buffer->Offset > buffer->Bytes.size() ||
        size > buffer->Bytes.size() - buffer->Offset) {
        return CKAS_BUFFERTOOSMALL;
    }
    if (size > 0) {
        std::memcpy(data, buffer->Bytes.data() + buffer->Offset, size);
        buffer->Offset += size;
    }
    return CKAS_OK;
}

CKAS_STATUS RejectBytecodeWrite(const void *, size_t, void *) {
    return CKAS_CANCELLED;
}

CKAS_STATUS RejectBytecodeRead(void *, size_t, void *) {
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

bool ExecuteIntFunction(const CKAngelScriptApi &api,
                        const char *moduleName,
                        const char *functionDecl,
                        int &value,
                        CKAngelScriptResult &result,
                        std::string &error) {
    CKAngelScriptFunction *function = nullptr;
    if (api.FindFunction(CKAngelScriptApi::FunctionByDeclOptions(moduleName, functionDecl), &function, &result) != CKAS_OK ||
        !function) {
        error = std::string("CKAngelScript API self-test could not find function: ") +
                (functionDecl ? functionDecl : "<null>");
        return false;
    }

    IntExecutionData data;
    CKAngelScriptFunctionExecutionOptions executeOptions =
        CKAngelScriptApi::FunctionExecutionOptions(function);
    CKAngelScriptExecutionStepOptions stepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &data);
    CKAngelScriptExecution *execution = nullptr;
    const CKAS_STATUS createStatus = api.CreateFunctionExecution(executeOptions, &execution, &result);
    if (createStatus != CKAS_OK || !execution) {
        api.ReleaseFunction(function);
        error = "CKAngelScript API self-test could not create function execution.";
        return false;
    }
    const CKAS_STATUS startStatus = api.StartExecution(execution, stepOptions, &result);
    const CKAS_STATUS releaseExecutionStatus = api.ReleaseExecution(execution);
    api.ReleaseFunction(function);
    if (startStatus != CKAS_OK || releaseExecutionStatus != CKAS_OK) {
        error = std::string("CKAngelScript API self-test function execution failed for ") +
                (functionDecl ? functionDecl : "<null>") +
                ": start=" +
                CKAngelScriptApi::StatusName(startStatus) +
                ", release=" +
                CKAngelScriptApi::StatusName(releaseExecutionStatus) +
                ", result=" +
                CKAngelScriptApi::StatusName(result.Status) +
                ", asCode=" +
                std::to_string(result.AngelScriptCode);
        if (result.ErrorMessage && result.ErrorMessage[0] != '\0') {
            error += ", message=";
            error += result.ErrorMessage;
        }
        return false;
    }
    value = data.Output;
    return true;
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

    CKAngelScriptImportBindOptions initImportBindOptions;
    std::memset(&initImportBindOptions, 0x7f, sizeof(initImportBindOptions));
    CKAngelScriptInitImportBindOptions(&initImportBindOptions);
    if (initImportBindOptions.Size != sizeof(initImportBindOptions) ||
        initImportBindOptions.ImportModuleName ||
        initImportBindOptions.ImportIndex != 0 ||
        initImportBindOptions.SourceModuleName ||
        initImportBindOptions.FunctionDecl ||
        initImportBindOptions.Flags != 0) {
        error = "CKAngelScript API self-test expected ImportBindOptions initializer defaults.";
        return false;
    }

    CKAngelScriptBytecodeSaveOptions initBytecodeSaveOptions;
    std::memset(&initBytecodeSaveOptions, 0x7f, sizeof(initBytecodeSaveOptions));
    CKAngelScriptInitBytecodeSaveOptions(&initBytecodeSaveOptions);
    if (initBytecodeSaveOptions.Size != sizeof(initBytecodeSaveOptions) ||
        initBytecodeSaveOptions.ModuleName ||
        initBytecodeSaveOptions.Write ||
        initBytecodeSaveOptions.UserData ||
        initBytecodeSaveOptions.Flags != CKAS_BYTECODE_DEFAULT) {
        error = "CKAngelScript API self-test expected BytecodeSaveOptions initializer defaults.";
        return false;
    }

    CKAngelScriptBytecodeLoadOptions initBytecodeLoadOptions;
    std::memset(&initBytecodeLoadOptions, 0x7f, sizeof(initBytecodeLoadOptions));
    CKAngelScriptInitBytecodeLoadOptions(&initBytecodeLoadOptions);
    if (initBytecodeLoadOptions.Size != sizeof(initBytecodeLoadOptions) ||
        initBytecodeLoadOptions.ModuleName ||
        initBytecodeLoadOptions.Read ||
        initBytecodeLoadOptions.UserData ||
        initBytecodeLoadOptions.Flags != CKAS_BYTECODE_DEFAULT) {
        error = "CKAngelScript API self-test expected BytecodeLoadOptions initializer defaults.";
        return false;
    }

    CKAngelScriptModuleFingerprint initModuleFingerprint;
    std::memset(&initModuleFingerprint, 0x7f, sizeof(initModuleFingerprint));
    CKAngelScriptInitModuleFingerprint(&initModuleFingerprint);
    if (initModuleFingerprint.Size != sizeof(initModuleFingerprint) ||
        initModuleFingerprint.Kind != CKAS_MODULEKIND_UNKNOWN ||
        initModuleFingerprint.Generation != 0 ||
        initModuleFingerprint.ApiVersion != CKAS_API_VERSION ||
        initModuleFingerprint.AngelScriptVersion ||
        initModuleFingerprint.AngelScriptOptions ||
        initModuleFingerprint.SourceHash != 0 ||
        initModuleFingerprint.IncludeHash != 0 ||
        initModuleFingerprint.DeclaredImportHash != 0 ||
        initModuleFingerprint.BoundImportHash != 0 ||
        initModuleFingerprint.CombinedHash != 0 ||
        initModuleFingerprint.Flags != 0) {
        error = "CKAngelScript API self-test expected ModuleFingerprint initializer defaults.";
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
    CKAngelScriptInitImportBindOptions(nullptr);
    CKAngelScriptInitBytecodeSaveOptions(nullptr);
    CKAngelScriptInitBytecodeLoadOptions(nullptr);
    CKAngelScriptInitModuleFingerprint(nullptr);
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
        CKAS_FEATURE_MODULE_IMPORTS != 21 ||
        CKAS_FEATURE_MODULE_BYTECODE != 22 ||
        CKAS_FEATURE_MODULE_REPLACE_TRANSACTION != 23 ||
        CKAS_FEATURE_MODULE_GRAPH != 24 ||
        CKAS_FEATURE_MODULE_FINGERPRINT != 25 ||
        CKAS_EXECUTION_CANCELLED != 5 ||
        CKAS_MODULEKIND_SOURCE != 1 ||
        CKAS_MODULEKIND_BYTECODE != 2 ||
        CKAS_LOAD_REPLACEEXISTING != 0x00000001 ||
        CKAS_COMPILE_REPLACEEXISTING != 0x00000001 ||
        CKAS_ENGINEEXTENSION_DEFERRED != 0x00000001 ||
        CKAS_CALL_NO_SUSPEND != 0x00000001 ||
        CKAS_HOSTCALL_MUTATES_HOST_STATE != 0x00000001 ||
        CKAS_BYTECODE_DEFAULT != 0 ||
        CKAS_BYTECODE_STRIP_DEBUG_INFO != 0x00000001 ||
        CKAS_BYTECODE_REPLACEEXISTING != 0x00000002 ||
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
        !api->HasFeature(CKAS_FEATURE_HOST_CALL_FILTER) ||
        !api->HasFeature(CKAS_FEATURE_MODULE_IMPORTS) ||
        !api->HasFeature(CKAS_FEATURE_MODULE_BYTECODE) ||
        !api->HasFeature(CKAS_FEATURE_MODULE_REPLACE_TRANSACTION) ||
        !api->HasFeature(CKAS_FEATURE_MODULE_GRAPH) ||
        !api->HasFeature(CKAS_FEATURE_MODULE_FINGERPRINT)) {
        error = "CKAngelScript API self-test found an unexpected feature set.";
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
        softApi.GetApiVersion() != CKAS_API_VERSION ||
        !softApi.InitModuleFingerprint ||
        !softApi.EnumerateBoundImportEdges ||
        !softApi.EnumerateModuleIncludeEdges ||
        !softApi.GetModuleFingerprint) {
        error = "CKAngelScript API self-test failed to soft-load the extension API table.";
        return false;
    }
    CKAngelScriptModuleFingerprint softFingerprint;
    std::memset(&softFingerprint, 0x7f, sizeof(softFingerprint));
    softApi.InitModuleFingerprint(&softFingerprint);
    if (softFingerprint.Size != sizeof(softFingerprint) ||
        softFingerprint.Kind != CKAS_MODULEKIND_UNKNOWN ||
        softFingerprint.ApiVersion != CKAS_API_VERSION) {
        error = "CKAngelScript API self-test expected soft-loaded fingerprint initializer defaults.";
        return false;
    }

    CKAngelScriptResult softResult = CKAngelScriptApi::Result();
    softResult.CompilerMessages =
        reinterpret_cast<const CKAngelScriptCompilerMessage *>(static_cast<uintptr_t>(1));
    softResult.CompilerMessageCount = 99;
    if (CKAngelScriptRegisterEngineExtensionWithApi(nullptr,
                                                    context,
                                                    "__ckas_soft_api_extension",
                                                    RegisterCkasDeferredSelfTestExtension,
                                                    nullptr,
                                                    CKAS_ENGINEEXTENSION_DEFERRED,
                                                    &softResult) != CKAS_INVALIDARGUMENT ||
        softResult.Status != CKAS_INVALIDARGUMENT ||
        softResult.CompilerMessages ||
        softResult.CompilerMessageCount != 0) {
        error = "CKAngelScript API self-test expected an unloaded extension API table to fail registration with a clean result.";
        return false;
    }
    softResult.CompilerMessages =
        reinterpret_cast<const CKAngelScriptCompilerMessage *>(static_cast<uintptr_t>(1));
    softResult.CompilerMessageCount = 99;
    if (CKAngelScriptUnregisterEngineExtensionWithApi(nullptr,
                                                      context,
                                                      "__ckas_soft_api_extension",
                                                      &softResult) != CKAS_INVALIDARGUMENT ||
        softResult.Status != CKAS_INVALIDARGUMENT ||
        softResult.CompilerMessages ||
        softResult.CompilerMessageCount != 0) {
        error = "CKAngelScript API self-test expected an unloaded extension API table to fail unregistration with a clean result.";
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

    CKAngelScriptModuleFingerprint invalidHandleFingerprint = CKAngelScriptApi::ModuleFingerprint();
    invalidHandleFingerprint.Kind = CKAS_MODULEKIND_SOURCE;
    invalidHandleFingerprint.Generation = 123;
    invalidHandleFingerprint.CombinedHash = 456;
    result = {};
    if (CKAngelScriptGetModuleFingerprint(nullptr, "__ckas_invalid", &invalidHandleFingerprint, &result) !=
            CKAS_INVALIDARGUMENT ||
        invalidHandleFingerprint.Size != sizeof(invalidHandleFingerprint) ||
        invalidHandleFingerprint.ApiVersion != CKAS_API_VERSION ||
        invalidHandleFingerprint.Kind != CKAS_MODULEKIND_UNKNOWN ||
        invalidHandleFingerprint.Generation != 0 ||
        invalidHandleFingerprint.CombinedHash != 0 ||
        result.Status != CKAS_INVALIDARGUMENT ||
        !result.ErrorMessage) {
        error = "CKAngelScript API self-test expected invalid public handle fingerprint calls to clear outputs.";
        return false;
    }
    CKAngelScriptModuleFingerprint invalidSizedHandleFingerprint = CKAngelScriptApi::ModuleFingerprint();
    invalidSizedHandleFingerprint.Size = 0;
    invalidSizedHandleFingerprint.Kind = CKAS_MODULEKIND_SOURCE;
    invalidSizedHandleFingerprint.Generation = 123;
    invalidSizedHandleFingerprint.CombinedHash = 456;
    result = {};
    if (CKAngelScriptGetModuleFingerprint(nullptr,
                                          "__ckas_invalid",
                                          &invalidSizedHandleFingerprint,
                                          &result) != CKAS_INVALIDARGUMENT ||
        invalidSizedHandleFingerprint.Size != 0 ||
        invalidSizedHandleFingerprint.Kind != CKAS_MODULEKIND_SOURCE ||
        invalidSizedHandleFingerprint.Generation != 123 ||
        invalidSizedHandleFingerprint.CombinedHash != 456 ||
        result.Status != CKAS_INVALIDARGUMENT ||
        !result.ErrorMessage) {
        error = "CKAngelScript API self-test expected invalid public handle fingerprint calls to respect output size.";
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
        ScriptCache cache;
        constexpr const char *cacheClearModuleName = "__CKAS_CacheClearLifetimeSelfTest";
        std::shared_ptr<CachedScript> retained =
            cache.CompileScript(engine,
                                cacheClearModuleName,
                                "int __ckas_cache_clear_value() { return 1; }\n");
        if (!retained || !retained->module ||
            engine->GetModule(cacheClearModuleName, asGM_ONLY_IF_EXISTS) == nullptr) {
            error = "CKAngelScript API self-test failed to set up ScriptCache clear lifetime test.";
            return false;
        }
        cache.Clear();
        if (retained->module ||
            engine->GetModule(cacheClearModuleName, asGM_ONLY_IF_EXISTS) != nullptr) {
            retained->Discard();
            error = "CKAngelScript API self-test expected ScriptCache::Clear to discard retained cached modules.";
            return false;
        }
    }

    {
        ScriptCache cache;
        constexpr const char *cacheInvalidateModuleName = "__CKAS_CacheInvalidateLifetimeSelfTest";
        std::shared_ptr<CachedScript> retained =
            cache.CompileScript(engine,
                                cacheInvalidateModuleName,
                                "int __ckas_cache_invalidate_value() { return 1; }\n");
        if (!retained || !retained->module ||
            engine->GetModule(cacheInvalidateModuleName, asGM_ONLY_IF_EXISTS) == nullptr) {
            error = "CKAngelScript API self-test failed to set up ScriptCache invalidate lifetime test.";
            return false;
        }
        cache.Invalidate(cacheInvalidateModuleName);
        if (retained->module ||
            engine->GetModule(cacheInvalidateModuleName, asGM_ONLY_IF_EXISTS) != nullptr) {
            retained->Discard();
            error = "CKAngelScript API self-test expected ScriptCache::Invalidate to discard retained cached modules.";
            return false;
        }
    }

    {
        ScriptCache cache;
        constexpr const char *cacheChunkModuleName = "__CKAS_CacheChunkSnapshotSelfTest";
        std::shared_ptr<CachedScript> snapshot = cache.NewCachedScript(cacheChunkModuleName);
        if (!snapshot) {
            error = "CKAngelScript API self-test failed to allocate source snapshot cache entry.";
            return false;
        }
        snapshot->sourceSnapshotSections = true;
        snapshot->AddSection("chunk/main.as",
                             "#include \"lib/helper.as\"\n"
                             "int __ckas_cache_chunk_value() { return __ckas_cache_chunk_helper() + 1; }\n");
        snapshot->AddSection("chunk/lib/helper.as",
                             "int __ckas_cache_chunk_helper() { return 41; }\n");
        if (!snapshot->Build(engine) ||
            engine->GetModule(cacheChunkModuleName, asGM_ONLY_IF_EXISTS) == nullptr) {
            error = "CKAngelScript API self-test failed to build source snapshot cache entry.";
            return false;
        }

        std::unique_ptr<CKStateChunk, void (*)(CKStateChunk *)> chunk(
            CreateCKStateChunk(CKCID_OBJECT, nullptr),
            DeleteCKStateChunk);
        if (!chunk || !snapshot->SaveToChunk(chunk.get())) {
            error = "CKAngelScript API self-test failed to save source snapshot cache entry.";
            return false;
        }
        cache.Clear();

        ScriptCache restoredCache;
        std::shared_ptr<CachedScript> restored = restoredCache.NewCachedScript(cacheChunkModuleName);
        if (!restored ||
            !restored->LoadFromChunk(chunk.get()) ||
            !restored->sourceSnapshotSections ||
            !restored->Build(engine)) {
            restoredCache.Clear();
            error = "CKAngelScript API self-test expected cached source snapshot chunks to restore include semantics.";
            return false;
        }
        int chunkValue = 0;
        if (!ExecuteIntFunction(api,
                                cacheChunkModuleName,
                                "int __ckas_cache_chunk_value()",
                                chunkValue,
                                result,
                                error) ||
            chunkValue != 42) {
            if (error.empty()) {
                error = "CKAngelScript API self-test expected restored source snapshot chunk to execute.";
            }
            restoredCache.Clear();
            return false;
        }
        restoredCache.Clear();
    }

    {
        constexpr const char *invokerRejectedArgsModuleName = "__CKAS_InvokerRejectedArgsSelfTest";
        if (api->CompileModule(invokerRejectedArgsModuleName,
                               "class __CKAS_InvokerRejectedArgsBox {\n"
                               "  void __ckas_invoker_bad_context(int value) {}\n"
                               "  int __ckas_invoker_good_context() { return 42; }\n"
                               "}\n"
                               "int __ckas_invoker_reuse_after_rejected_args() { return 41; }\n",
                               CKAS_COMPILE_REPLACEEXISTING,
                               &result) != CKAS_OK) {
            error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
                ? result.ErrorMessage
                : "CKAngelScript API self-test failed to compile invoker rejected-args test module.";
            return false;
        }

        asIScriptModule *invokerModule = engine->GetModule(invokerRejectedArgsModuleName, asGM_ONLY_IF_EXISTS);
        asIScriptFunction *invokerFunction = invokerModule
            ? invokerModule->GetFunctionByDecl("int __ckas_invoker_reuse_after_rejected_args()")
            : nullptr;
        int invokerResult = 0;
        ScriptInvoker invoker(manager);
        const ScriptInvocationStatus rejectedStatus =
            invoker.SetScript(invokerRejectedArgsModuleName)
                ? invoker.ExecuteScriptStatus(invokerFunction,
                                              [](asIScriptContext *) {
                                                  return false;
                                              },
                                              nullptr)
                : ScriptInvocationStatus::Failed;
        const ScriptInvocationStatus retryStatus =
            invoker.ExecuteScriptStatus(invokerFunction,
                                        nullptr,
                                        [&invokerResult](asIScriptContext *ctx) {
                                            const void *address = ctx ? ctx->GetAddressOfReturnValue() : nullptr;
                                            if (!address) {
                                                return false;
                                            }
                                            invokerResult = *static_cast<const int *>(address);
                                            return true;
                                        });
        asITypeInfo *invokerBoxType = invoker.GetTypeInfoByName("__CKAS_InvokerRejectedArgsBox");
        asIScriptObject *invokerBox = invoker.CreateScriptObject(invokerBoxType);
        asIScriptFunction *badContextMethod = invokerBoxType
            ? invokerBoxType->GetMethodByDecl("void __ckas_invoker_bad_context(int)")
            : nullptr;
        asIScriptFunction *goodContextMethod = invokerBoxType
            ? invokerBoxType->GetMethodByDecl("int __ckas_invoker_good_context()")
            : nullptr;
        CKBehaviorContext emptyBehaviorContext = {};
        const ScriptInvocationStatus rejectedMethodStatus =
            invoker.ExecuteObjectMethodStatus(invokerBox, badContextMethod, emptyBehaviorContext);
        const ScriptInvocationStatus retryMethodStatus =
            invoker.ExecuteObjectMethodStatus(invokerBox, goodContextMethod, emptyBehaviorContext);
        if (invokerBox) {
            engine->ReleaseScriptObject(invokerBox, invokerBoxType);
        }
        invoker.Reset();
        if (!invokerFunction ||
            !invokerBoxType ||
            !badContextMethod ||
            !goodContextMethod ||
            rejectedStatus != ScriptInvocationStatus::Failed ||
            retryStatus != ScriptInvocationStatus::Finished ||
            invokerResult != 41 ||
            rejectedMethodStatus != ScriptInvocationStatus::Failed ||
            retryMethodStatus != ScriptInvocationStatus::Finished ||
            api->UnloadModule(invokerRejectedArgsModuleName, &result) != CKAS_OK) {
            api->UnloadModule(invokerRejectedArgsModuleName, nullptr);
            error = "CKAngelScript API self-test expected ScriptInvoker to clear context after rejected argument callbacks.";
            return false;
        }
    }

    {
        constexpr const char *invokerResetSuspendedModuleName = "__CKAS_InvokerResetSuspendedSelfTest";
        if (api->CompileModule(invokerResetSuspendedModuleName,
                               "int __ckas_invoker_wait_then_reset() {\n"
                               "  AsyncTask<void>@ delay = Async::Delay(100);\n"
                               "  Await(delay);\n"
                               "  return 7;\n"
                               "}\n"
                               "int __ckas_invoker_after_reset() { return 8; }\n",
                               CKAS_COMPILE_REPLACEEXISTING,
                               &result) != CKAS_OK) {
            error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
                ? result.ErrorMessage
                : "CKAngelScript API self-test failed to compile invoker suspended reset test module.";
            return false;
        }
        asIScriptModule *invokerResetModule = engine->GetModule(invokerResetSuspendedModuleName, asGM_ONLY_IF_EXISTS);
        asIScriptFunction *waitFunction = invokerResetModule
            ? invokerResetModule->GetFunctionByDecl("int __ckas_invoker_wait_then_reset()")
            : nullptr;
        asIScriptFunction *afterFunction = invokerResetModule
            ? invokerResetModule->GetFunctionByDecl("int __ckas_invoker_after_reset()")
            : nullptr;
        int afterResetResult = 0;
        ScriptInvoker invoker(manager);
        const bool scriptSet = invoker.SetScript(invokerResetSuspendedModuleName);
        const ScriptInvocationStatus waitStatus = scriptSet
            ? invoker.ExecuteScriptStatus(waitFunction)
            : ScriptInvocationStatus::Failed;
        invoker.Reset();
        const bool scriptReset = invoker.SetScript(invokerResetSuspendedModuleName);
        const ScriptInvocationStatus afterStatus = scriptReset
            ? invoker.ExecuteScriptStatus(afterFunction,
                                          nullptr,
                                          [&afterResetResult](asIScriptContext *ctx) {
                                              const void *address = ctx ? ctx->GetAddressOfReturnValue() : nullptr;
                                              if (!address) {
                                                  return false;
                                              }
                                              afterResetResult = *static_cast<const int *>(address);
                                              return true;
                                          })
            : ScriptInvocationStatus::Failed;
        invoker.Reset();
        if (manager && manager->GetAsyncScheduler()) {
            manager->GetAsyncScheduler()->Clear();
        }
        if (!waitFunction ||
            !afterFunction ||
            waitStatus != ScriptInvocationStatus::Suspended ||
            afterStatus != ScriptInvocationStatus::Finished ||
            afterResetResult != 8 ||
            api->UnloadModule(invokerResetSuspendedModuleName, &result) != CKAS_OK) {
            api->UnloadModule(invokerResetSuspendedModuleName, nullptr);
            error = "CKAngelScript API self-test expected ScriptInvoker reset to abort suspended contexts.";
            return false;
        }
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
        "void __ckas_public_async_spawn_worker() { AsyncTask<void>@ delay = Async::Delay(1000); Await(delay); }\n"
        "void __ckas_public_spawn_async_dependency() { Async::Spawn(AsyncVoidFunc(__ckas_public_async_spawn_worker)); }\n"
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
        "enum __CKAS_PublicFormatEnum { __CKAS_FormatFirst = 7, __CKAS_FormatSecond = 8 }\n"
        "class __CKAS_PublicFormatBox {\n"
        "    string Text;\n"
        "    string opImplConv() const { return Text; }\n"
        "}\n"
        "int __ckas_public_to_string_null_handle() {\n"
        "    __CKAS_PublicFormatBox@ box = null;\n"
        "    if (toString(@box) != \"null\") return 1;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_fmt_null_handle() {\n"
        "    __CKAS_PublicFormatBox@ box = null;\n"
        "    if (fmt(\"{}\", @box) != \"null\") return 2;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_format_enum_first() {\n"
        "    if (toString(__CKAS_FormatFirst) != \"__CKAS_FormatFirst\") return 3;\n"
        "    if (fmt(\"{}\", __CKAS_FormatFirst) != \"__CKAS_FormatFirst\") return 4;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_format_live_handle() {\n"
        "    __CKAS_PublicFormatBox@ live = __CKAS_PublicFormatBox();\n"
        "    live.Text = \"box\";\n"
        "    string text = toString(@live);\n"
        "    if (text == \"null\" || text == \"box\") return 5;\n"
        "    if (fmt(\"{}\", @live) != text) return 6;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_format_const_handle() {\n"
        "    __CKAS_PublicFormatBox@ live = __CKAS_PublicFormatBox();\n"
        "    const __CKAS_PublicFormatBox@ constLive = live;\n"
        "    string text = toString(@constLive);\n"
        "    if (text == \"null\" || text == \"unknown\") return 12;\n"
        "    if (fmt(\"{}\", @constLive) != text) return 13;\n"
        "    if (typeof(@constLive) != \"const __CKAS_PublicFormatBox@\") return 14;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_fmt_numeric() {\n"
        "    if (fmt(\"{:04}\", 12) != \"0012\") return 7;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_format_simple() {\n"
        "    if (format(\"{}\", 12) != \"12\") return 8;\n"
        "    return 0;\n"
        "}\n"
        "int __ckas_public_typeof_values() {\n"
        "    if (typeof(0) != \"int32\") return 9;\n"
        "    __CKAS_PublicFormatBox@ box = null;\n"
        "    if (typeof(@box) != \"null\") return 10;\n"
        "    @box = __CKAS_PublicFormatBox();\n"
        "    if (typeof(@box) != \"__CKAS_PublicFormatBox@\") return 11;\n"
        "    return 0;\n"
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

    const char *formatHelperDecls[] = {
        "int __ckas_public_to_string_null_handle()",
        "int __ckas_public_fmt_null_handle()",
        "int __ckas_public_format_enum_first()",
        "int __ckas_public_format_live_handle()",
        "int __ckas_public_format_const_handle()",
        "int __ckas_public_fmt_numeric()",
        "int __ckas_public_format_simple()",
        "int __ckas_public_typeof_values()",
    };
    for (const char *formatHelperDecl : formatHelperDecls) {
        int formatHelperResult = -1;
        if (!ExecuteIntFunction(api,
                                moduleName,
                                formatHelperDecl,
                                formatHelperResult,
                                result,
                                error) ||
            formatHelperResult != 0) {
            if (error.empty()) {
                error = std::string("CKAngelScript API self-test format helper ") +
                        formatHelperDecl +
                        " returned " +
                        std::to_string(formatHelperResult) +
                        ".";
            }
            return false;
        }
    }

    constexpr const char *rawModuleName = "__CKAS_RawModuleUnloadSelfTest";
    const char *rawModuleSource =
        "int __ckas_raw_module_value() { return 73; }\n";
    asIScriptModule *rawModule = engine->GetModule(rawModuleName, asGM_ALWAYS_CREATE);
    if (!rawModule ||
        rawModule->AddScriptSection(rawModuleName,
                                    rawModuleSource,
                                    static_cast<unsigned int>(std::strlen(rawModuleSource))) < 0 ||
        rawModule->Build() < 0) {
        error = "CKAngelScript API self-test failed to create a raw AngelScript module.";
        if (rawModule) {
            rawModule->Discard();
        }
        return false;
    }
    CKAngelScriptFunction *rawModuleFunction = nullptr;
    if (!api->HasModule(rawModuleName) ||
        api->FindFunction(CKAngelScriptApi::FunctionByDeclOptions(rawModuleName,
                                                                  "int __ckas_raw_module_value()"),
                          &rawModuleFunction,
                          &result) != CKAS_OK ||
        !rawModuleFunction) {
        error = "CKAngelScript API self-test expected raw engine modules to expose public function handles.";
        api->ReleaseFunction(rawModuleFunction);
        rawModule = engine->GetModule(rawModuleName, asGM_ONLY_IF_EXISTS);
        if (rawModule) {
            rawModule->Discard();
        }
        return false;
    }
    CKAngelScriptExecution *rawModuleExecution = nullptr;
    IntExecutionData rawModuleExecutionData;
    CKAngelScriptExecutionStepOptions rawModuleStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(nullptr, ReadIntReturn, &rawModuleExecutionData);
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(rawModuleFunction),
                                     &rawModuleExecution,
                                     &result) != CKAS_OK ||
        !rawModuleExecution ||
        api->StartExecution(rawModuleExecution, rawModuleStepOptions, &result) != CKAS_OK ||
        rawModuleExecutionData.Output != 73) {
        error = "CKAngelScript API self-test expected raw module functions to execute through public handles.";
        if (rawModuleExecution) {
            api->ReleaseExecution(rawModuleExecution);
        }
        api->ReleaseFunction(rawModuleFunction);
        rawModule = engine->GetModule(rawModuleName, asGM_ONLY_IF_EXISTS);
        if (rawModule) {
            rawModule->Discard();
        }
        return false;
    }
    if (api->UnloadModule(rawModuleName, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected raw module execution handles to block unload.";
        api->ReleaseExecution(rawModuleExecution);
        api->ReleaseFunction(rawModuleFunction);
        rawModule = engine->GetModule(rawModuleName, asGM_ONLY_IF_EXISTS);
        if (rawModule) {
            rawModule->Discard();
        }
        return false;
    }
    api->ReleaseExecution(rawModuleExecution);
    if (api->UnloadModule(rawModuleName, &result) != CKAS_OK ||
        api->HasModule(rawModuleName)) {
        error = "CKAngelScript API self-test expected UnloadModule to discard raw engine modules.";
        api->ReleaseFunction(rawModuleFunction);
        rawModule = engine->GetModule(rawModuleName, asGM_ONLY_IF_EXISTS);
        if (rawModule) {
            rawModule->Discard();
        }
        return false;
    }
    rawModuleExecution = reinterpret_cast<CKAngelScriptExecution *>(static_cast<uintptr_t>(1));
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(rawModuleFunction),
                                     &rawModuleExecution,
                                     &result) != CKAS_STALEHANDLE ||
        rawModuleExecution != nullptr) {
        error = "CKAngelScript API self-test expected raw module unload to stale CKAS function handles.";
        if (rawModuleExecution) {
            api->ReleaseExecution(rawModuleExecution);
        }
        api->ReleaseFunction(rawModuleFunction);
        return false;
    }
    api->ReleaseFunction(rawModuleFunction);

    constexpr const char *rawFingerprintModuleName = "__CKAS_RawFingerprintModule";
    const char *rawFingerprintInitialSource =
        "int __ckas_raw_fingerprint_value() { return 1; }\n";
    asIScriptModule *rawFingerprintModule =
        engine->GetModule(rawFingerprintModuleName, asGM_ALWAYS_CREATE);
    if (!rawFingerprintModule ||
        rawFingerprintModule->AddScriptSection(rawFingerprintModuleName,
                                               rawFingerprintInitialSource,
                                               static_cast<unsigned int>(
                                                   std::strlen(rawFingerprintInitialSource))) < 0 ||
        rawFingerprintModule->Build() < 0) {
        error = "CKAngelScript API self-test failed to create a raw fingerprint module.";
        if (rawFingerprintModule) {
            rawFingerprintModule->Discard();
        }
        return false;
    }
    CKAngelScriptModuleFingerprint rawFingerprintBefore = CKAngelScriptApi::ModuleFingerprint();
    if (api->GetModuleFingerprint(rawFingerprintModuleName, &rawFingerprintBefore, &result) != CKAS_OK ||
        rawFingerprintBefore.Kind != CKAS_MODULEKIND_UNKNOWN ||
        rawFingerprintBefore.DeclaredImportHash == 0 ||
        rawFingerprintBefore.CombinedHash == 0) {
        error = "CKAngelScript API self-test expected raw module fingerprint queries to work.";
        api->UnloadModule(rawFingerprintModuleName, nullptr);
        return false;
    }
    const char *rawFingerprintImportSource =
        "import int __ckas_raw_fingerprint_imported() from \"__CKAS_RawFingerprintProvider\";\n"
        "int __ckas_raw_fingerprint_value() { return 2; }\n";
    rawFingerprintModule = engine->GetModule(rawFingerprintModuleName, asGM_ALWAYS_CREATE);
    if (!rawFingerprintModule ||
        rawFingerprintModule->AddScriptSection(rawFingerprintModuleName,
                                               rawFingerprintImportSource,
                                               static_cast<unsigned int>(
                                                   std::strlen(rawFingerprintImportSource))) < 0 ||
        rawFingerprintModule->Build() < 0) {
        error = "CKAngelScript API self-test failed to rebuild a raw fingerprint module.";
        if (rawFingerprintModule) {
            rawFingerprintModule->Discard();
        }
        return false;
    }
    CKAngelScriptModuleFingerprint rawFingerprintAfter = CKAngelScriptApi::ModuleFingerprint();
    if (api->GetModuleFingerprint(rawFingerprintModuleName, &rawFingerprintAfter, &result) != CKAS_OK ||
        rawFingerprintAfter.DeclaredImportHash == rawFingerprintBefore.DeclaredImportHash ||
        rawFingerprintAfter.CombinedHash == rawFingerprintBefore.CombinedHash) {
        error = "CKAngelScript API self-test expected raw module fingerprint declared imports to refresh.";
        api->UnloadModule(rawFingerprintModuleName, nullptr);
        return false;
    }
    api->UnloadModule(rawFingerprintModuleName, nullptr);

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
    ReentrantMetadataStringProbe metadataStringProbe;
    metadataStringProbe.Api = &api;
    metadataStringProbe.ModuleName = moduleName;
    if (api->EnumerateMetadata(moduleName, ProbeMetadataStringsAfterReadOnlyReentry, &metadataStringProbe, &result) != CKAS_OK ||
        metadataStringProbe.CallbackCount == 0 ||
        metadataStringProbe.ReentryStatus != CKAS_OK ||
        !metadataStringProbe.GlobalDeclaration ||
        !metadataStringProbe.TypePropertyDeclaration) {
        error = "CKAngelScript API self-test expected metadata entry strings to survive read-only AngelScript reentry.";
        return false;
    }
    ReentrantMetadataProbe metadataReentry;
    metadataReentry.Api = &api;
    metadataReentry.ModuleName = moduleName;
    if (api->EnumerateMetadata(moduleName, MutateFromMetadataCallback, &metadataReentry, &result) != CKAS_OK ||
        metadataReentry.CallbackCount == 0 ||
        metadataReentry.ReentryStatus != CKAS_INVALIDSTATE ||
        metadataReentry.BytecodeSaveStatus != CKAS_OK ||
        metadataReentry.BytecodeWriteReentryStatus != CKAS_INVALIDSTATE ||
        metadataReentry.BytecodeNestedSaveStatus != CKAS_INVALIDSTATE ||
        metadataReentry.BytecodeSize == 0) {
        error = "CKAngelScript API self-test expected metadata callbacks to allow bytecode save while rejecting module mutation reentry.";
        api->UnloadModule("__CKAS_MetadataCallbackReentry", nullptr);
        api->UnloadModule("__CKAS_BytecodeCallbackReentry", nullptr);
        return false;
    }
    if (api->EnumerateMetadata("__CKAS_MissingMetadataModule", ProbeMetadata, &metadataProbe, &result) != CKAS_NOTFOUND ||
        api->EnumerateMetadata(moduleName, nullptr, nullptr, &result) != CKAS_INVALIDARGUMENT ||
        api->EnumerateMetadata(moduleName, StopMetadata, nullptr, &result) != CKAS_CANCELLED) {
        error = "CKAngelScript API self-test expected metadata enumeration failure statuses.";
        return false;
    }

    constexpr const char *transientCollisionModuleName = "__CKAS_TransientCollisionTarget";
    std::vector<std::string> transientCollisionModules;
    const auto cleanupTransientCollisionModules = [&]() {
        api->UnloadModule(transientCollisionModuleName, nullptr);
        for (const std::string &name : transientCollisionModules) {
            api->UnloadModule(name.c_str(), nullptr);
        }
    };
    for (int i = 1; i <= 16; ++i) {
        std::string collisionModuleName = "__ckas_replace_candidate_";
        collisionModuleName += transientCollisionModuleName;
        collisionModuleName += "_";
        collisionModuleName += std::to_string(i);
        transientCollisionModules.push_back(collisionModuleName);
        std::string collisionSource = "int __ckas_transient_collision_value() { return ";
        collisionSource += std::to_string(1000 + i);
        collisionSource += "; }\n";
        if (api->CompileModule(collisionModuleName.c_str(),
                               collisionSource.c_str(),
                               CKAS_COMPILE_REPLACEEXISTING,
                               &result) != CKAS_OK) {
            error = "CKAngelScript API self-test failed to compile transient collision guard module.";
            cleanupTransientCollisionModules();
            return false;
        }
    }
    int transientCollisionValue = 0;
    if (api->CompileModule(transientCollisionModuleName,
                           "int __ckas_transient_collision_target() { return 1; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(transientCollisionModuleName,
                           "int __ckas_transient_collision_target() { return 2; }\n",
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            transientCollisionModuleName,
                            "int __ckas_transient_collision_target()",
                            transientCollisionValue,
                            result,
                            error) ||
        transientCollisionValue != 2) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected replacement to survive transient name collisions.";
        }
        cleanupTransientCollisionModules();
        return false;
    }
    for (int i = 0; i < static_cast<int>(transientCollisionModules.size()); ++i) {
        if (!ExecuteIntFunction(api,
                                transientCollisionModules[static_cast<size_t>(i)].c_str(),
                                "int __ckas_transient_collision_value()",
                                transientCollisionValue,
                                result,
                                error) ||
            transientCollisionValue != 1001 + i) {
            if (error.empty()) {
                error = "CKAngelScript API self-test expected transient collision modules to remain loaded.";
            }
            cleanupTransientCollisionModules();
            return false;
        }
    }
    cleanupTransientCollisionModules();

    constexpr const char *importProviderModuleName = "__CKAS_ImportProvider";
    constexpr const char *importConsumerModuleName = "__CKAS_ImportConsumer";
    const char *importProviderSource =
        "int __ckas_import_add(int value) { return value + 12; }\n"
        "int __ckas_import_alias(int value) { return value + 30; }\n"
        "void __ckas_import_wrong() {}\n";
    const char *importProviderReplacementSource =
        "int __ckas_import_add(int value) { return value + 20; }\n"
        "int __ckas_import_alias(int value) { return value + 40; }\n"
        "void __ckas_import_wrong() {}\n";
    const char *importConsumerSource =
        "import int __ckas_import_add(int value) from \"__CKAS_ImportProvider\";\n"
        "int __ckas_import_call() { return __ckas_import_add(5); }\n";
    if (api->CompileModule(importProviderModuleName,
                           importProviderSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(importConsumerModuleName,
                           importConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test failed to compile import probe modules.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    CKDWORD importCount = 99;
    ImportProbe importProbe;
    if (api->GetImportedFunctionCount(importConsumerModuleName, &importCount, &result) != CKAS_OK ||
        importCount != 1 ||
        api->EnumerateImportedFunctions(importConsumerModuleName, ProbeImport, &importProbe, &result) != CKAS_OK ||
        importProbe.CallbackCount != 1 ||
        !importProbe.SawImport) {
        error = "CKAngelScript API self-test expected import enumeration to describe the consumer import.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    ReentrantImportStringProbe importStringProbe;
    importStringProbe.Api = &api;
    importStringProbe.ProviderModuleName = importProviderModuleName;
    if (api->EnumerateImportedFunctions(importConsumerModuleName,
                                        ProbeImportAfterReadOnlyReentry,
                                        &importStringProbe,
                                        &result) != CKAS_OK ||
        importStringProbe.CallbackCount != 1 ||
        importStringProbe.ReentryStatus != CKAS_OK ||
        !importStringProbe.SawImport) {
        error = "CKAngelScript API self-test expected import entry strings to survive read-only AngelScript reentry.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->GetImportedFunctionCount(importConsumerModuleName, nullptr, &result) != CKAS_INVALIDARGUMENT ||
        api->EnumerateImportedFunctions(importConsumerModuleName, nullptr, nullptr, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected import inspection invalid arguments to fail.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    CKAngelScriptFunction *preBindImportFunction = nullptr;
    if (api->FindFunction(CKAngelScriptApi::FunctionByDeclOptions(importConsumerModuleName,
                                                                  "int __ckas_import_call()"),
                          &preBindImportFunction,
                          &result) != CKAS_OK ||
        !preBindImportFunction) {
        error = "CKAngelScript API self-test expected to capture an import consumer function before binding.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    int importValue = 0;
    CKAngelScriptExecution *preBindReadyImportExecution = nullptr;
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(preBindImportFunction),
                                     &preBindReadyImportExecution,
                                     &result) != CKAS_OK ||
        !preBindReadyImportExecution) {
        error = "CKAngelScript API self-test expected to capture an import consumer execution before binding.";
        if (preBindReadyImportExecution) {
            api->ReleaseExecution(preBindReadyImportExecution);
        }
        api->ReleaseFunction(preBindImportFunction);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    CKAngelScriptExecution *staleImportExecution = nullptr;
    if (api->BindAllImportedFunctions(importConsumerModuleName, &result) != CKAS_OK ||
        api->StartExecution(preBindReadyImportExecution, nullptr, &result) != CKAS_STALEHANDLE ||
        api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(preBindImportFunction),
                                     &staleImportExecution,
                                     &result) != CKAS_STALEHANDLE ||
        staleImportExecution ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 17) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected BindAllImportedFunctions to relink and stale old consumer handles.";
        }
        if (staleImportExecution) {
            api->ReleaseExecution(staleImportExecution);
        }
        api->ReleaseExecution(preBindReadyImportExecution);
        api->ReleaseFunction(preBindImportFunction);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->ReleaseExecution(preBindReadyImportExecution);
    api->ReleaseFunction(preBindImportFunction);
    BoundImportEdgeProbe boundImportProbe;
    CKAngelScriptModuleFingerprint importFingerprint = CKAngelScriptApi::ModuleFingerprint();
    if (api->EnumerateBoundImportEdges(importConsumerModuleName,
                                       ProbeBoundImportEdge,
                                       &boundImportProbe,
                                       &result) != CKAS_OK ||
        boundImportProbe.CallbackCount != 1 ||
        !boundImportProbe.SawEdge ||
        api->EnumerateBoundImportEdges(importConsumerModuleName, nullptr, nullptr, &result) != CKAS_INVALIDARGUMENT ||
        api->GetModuleFingerprint(importConsumerModuleName, &importFingerprint, &result) != CKAS_OK ||
        importFingerprint.Kind != CKAS_MODULEKIND_SOURCE ||
        importFingerprint.Generation == 0 ||
        importFingerprint.AngelScriptVersion == nullptr ||
        importFingerprint.AngelScriptOptions == nullptr ||
        importFingerprint.DeclaredImportHash == 0 ||
        importFingerprint.BoundImportHash == 0 ||
        importFingerprint.CombinedHash == 0) {
        error = "CKAngelScript API self-test expected module graph APIs to describe bound imports.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    constexpr const char *secondImportConsumerModuleName = "__CKAS_ImportSecondConsumer";
    if (api->CompileModule(secondImportConsumerModuleName,
                           importConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->BindAllImportedFunctions(secondImportConsumerModuleName, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected a second import consumer to bind.";
        api->UnloadModule(secondImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_INUSE ||
        !CkasStringContains(result.ErrorMessage, importConsumerModuleName)) {
        error = "CKAngelScript API self-test expected provider replacement diagnostics to use current consumer order.";
        api->UnloadModule(secondImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnloadModule(importConsumerModuleName, &result) != CKAS_OK ||
        api->CompileModule(importConsumerModuleName,
                           importConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->BindAllImportedFunctions(importConsumerModuleName, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected import consumer unload/reload to update module order.";
        api->UnloadModule(secondImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_INUSE ||
        !CkasStringContains(result.ErrorMessage, secondImportConsumerModuleName)) {
        error = "CKAngelScript API self-test expected reloaded import consumers to move after already-loaded consumers.";
        api->UnloadModule(secondImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnloadModule(secondImportConsumerModuleName, &result) != CKAS_OK ||
        api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_INUSE ||
        api->UnloadModule(importProviderModuleName, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected bound import consumers to block provider replacement.";
        api->UnloadModule(secondImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->CompileModule(importConsumerModuleName,
                           importConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->BindAllImportedFunctions(importConsumerModuleName, &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 25) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected consumer replacement to clear import edges for provider reload.";
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnbindImportedFunction(importConsumerModuleName, 0, &result) != CKAS_OK ||
        api->BindImportedFunction(importConsumerModuleName, 0, nullptr, nullptr, &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 25) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected single import bind to use declaration defaults.";
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnbindImportedFunction(importConsumerModuleName, 0, &result) != CKAS_OK ||
        api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "int __ckas_import_add(int)",
                                  &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 25) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected explicit import bind to work.";
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnbindImportedFunction(importConsumerModuleName, 0, &result) != CKAS_OK ||
        api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "int __ckas_import_alias(int)",
                                  &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 45) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected explicit import binds to support provider aliases.";
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    const CKDWORD failedRebindGeneration = api->GetModuleGeneration(importConsumerModuleName);
    if (api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "void __ckas_import_wrong()",
                                  &result) != CKAS_TYPEMISMATCH ||
        api->GetModuleGeneration(importConsumerModuleName) != failedRebindGeneration ||
        api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_INUSE ||
        !ExecuteIntFunction(api,
                            importConsumerModuleName,
                            "int __ckas_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 45) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected failed import rebind to restore the previous binding.";
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnbindAllImportedFunctions(importConsumerModuleName, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected explicit import unbind-all to work.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    BoundImportEdgeProbe unboundImportProbe;
    CKAngelScriptModuleFingerprint unboundImportFingerprint = CKAngelScriptApi::ModuleFingerprint();
    if (api->EnumerateBoundImportEdges(importConsumerModuleName,
                                       ProbeBoundImportEdge,
                                       &unboundImportProbe,
                                       &result) != CKAS_OK ||
        unboundImportProbe.CallbackCount != 0 ||
        api->GetModuleFingerprint(importConsumerModuleName, &unboundImportFingerprint, &result) != CKAS_OK ||
        unboundImportFingerprint.CombinedHash == importFingerprint.CombinedHash ||
        unboundImportFingerprint.BoundImportHash == importFingerprint.BoundImportHash) {
        error = "CKAngelScript API self-test expected unbinding imports to clear graph edges and change fingerprint.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    const CKDWORD failedUnboundBindGeneration = api->GetModuleGeneration(importConsumerModuleName);
    if (api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "void __ckas_import_wrong()",
                                  &result) != CKAS_TYPEMISMATCH ||
        api->GetModuleGeneration(importConsumerModuleName) != failedUnboundBindGeneration) {
        error = "CKAngelScript API self-test expected failed unbound import bind to preserve generation.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    constexpr const char *partialImportConsumerModuleName = "__CKAS_ImportPartialConsumer";
    const char *partialImportConsumerSource =
        "import int __ckas_import_add(int value) from \"__CKAS_ImportProvider\";\n"
        "import int __ckas_import_missing(int value) from \"__CKAS_ImportProvider\";\n"
        "int __ckas_import_partial_call() { return __ckas_import_add(3); }\n";
    CKAngelScriptFunction *partialImportFunction = nullptr;
    if (api->CompileModule(partialImportConsumerModuleName,
                           partialImportConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->FindFunction(CKAngelScriptApi::FunctionByDeclOptions(partialImportConsumerModuleName,
                                                                  "int __ckas_import_partial_call()"),
                          &partialImportFunction,
                          &result) != CKAS_OK ||
        !partialImportFunction) {
        error = "CKAngelScript API self-test failed to compile partial import consumer.";
        api->ReleaseFunction(partialImportFunction);
        api->UnloadModule(partialImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    const CKDWORD partialImportGeneration = api->GetModuleGeneration(partialImportConsumerModuleName);
    CKAngelScriptExecution *partialImportExecution = nullptr;
    const CKAS_STATUS partialBindStatus = api->BindAllImportedFunctions(partialImportConsumerModuleName, &result);
    const CKAS_STATUS partialExecutionStatus =
        api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(partialImportFunction),
                                     &partialImportExecution,
                                     &result);
    if (partialBindStatus != CKAS_NOTFOUND ||
        api->GetModuleGeneration(partialImportConsumerModuleName) != partialImportGeneration ||
        partialExecutionStatus != CKAS_OK ||
        !partialImportExecution) {
        error = "CKAngelScript API self-test expected failed BindAllImportedFunctions to preserve the consumer module.";
        api->ReleaseExecution(partialImportExecution);
        api->ReleaseFunction(partialImportFunction);
        api->UnloadModule(partialImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->ReleaseExecution(partialImportExecution);
    api->ReleaseFunction(partialImportFunction);
    if (api->CompileModule(importProviderModuleName,
                           importProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected failed BindAllImportedFunctions to leave no provider edge.";
        api->UnloadModule(partialImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->UnloadModule(partialImportConsumerModuleName, nullptr);
    constexpr const char *mismatchImportProviderModuleName = "__CKAS_ImportMismatchProvider";
    constexpr const char *mismatchImportConsumerModuleName = "__CKAS_ImportMismatchConsumer";
    const char *mismatchImportProviderSource =
        "class Foo {}\n"
        "int __ckas_import_mismatch_ok() { return 7; }\n"
        "Foo __ckas_import_mismatch_make() { Foo value; return value; }\n";
    const char *mismatchImportProviderReplacementSource =
        "class Foo {}\n"
        "int __ckas_import_mismatch_ok() { return 8; }\n"
        "Foo __ckas_import_mismatch_make() { Foo value; return value; }\n";
    const char *mismatchImportConsumerSource =
        "class Foo {}\n"
        "import int __ckas_import_mismatch_ok() from \"__CKAS_ImportMismatchProvider\";\n"
        "import Foo __ckas_import_mismatch_make() from \"__CKAS_ImportMismatchProvider\";\n"
        "int __ckas_import_mismatch_call() { return __ckas_import_mismatch_ok(); }\n";
    if (api->CompileModule(mismatchImportProviderModuleName,
                           mismatchImportProviderSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(mismatchImportConsumerModuleName,
                           mismatchImportConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test failed to compile mismatched import probe modules.";
        api->UnloadModule(mismatchImportConsumerModuleName, nullptr);
        api->UnloadModule(mismatchImportProviderModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    BoundImportEdgeProbe mismatchImportProbe;
    const CKDWORD mismatchImportGeneration = api->GetModuleGeneration(mismatchImportConsumerModuleName);
    if (api->BindAllImportedFunctions(mismatchImportConsumerModuleName, &result) != CKAS_TYPEMISMATCH ||
        api->GetModuleGeneration(mismatchImportConsumerModuleName) != mismatchImportGeneration ||
        api->EnumerateBoundImportEdges(mismatchImportConsumerModuleName,
                                       ProbeBoundImportEdge,
                                       &mismatchImportProbe,
                                       &result) != CKAS_OK ||
        mismatchImportProbe.CallbackCount != 0 ||
        api->CompileModule(mismatchImportProviderModuleName,
                           mismatchImportProviderReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected partially failed BindAllImportedFunctions to roll back applied bindings.";
        api->UnloadModule(mismatchImportConsumerModuleName, nullptr);
        api->UnloadModule(mismatchImportProviderModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->UnloadModule(mismatchImportConsumerModuleName, nullptr);
    api->UnloadModule(mismatchImportProviderModuleName, nullptr);
    asIScriptModule *rawImportConsumer = nullptr;
    asIScriptModule *rawImportProvider = nullptr;
    CKAngelScriptFunction *rawBoundImportFunction = nullptr;
    if (api->BorrowModule(importConsumerModuleName, &rawImportConsumer, &result) != CKAS_OK ||
        api->BorrowModule(importProviderModuleName, &rawImportProvider, &result) != CKAS_OK ||
        !rawImportConsumer ||
        !rawImportProvider) {
        error = "CKAngelScript API self-test expected to borrow import modules for raw bind recovery.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    asIScriptFunction *rawImportTarget = rawImportProvider->GetFunctionByDecl("int __ckas_import_add(int)");
    if (!rawImportTarget ||
        rawImportConsumer->BindImportedFunction(0, rawImportTarget) < 0 ||
        api->FindFunction(CKAngelScriptApi::FunctionByDeclOptions(importConsumerModuleName,
                                                                  "int __ckas_import_call()"),
                          &rawBoundImportFunction,
                          &result) != CKAS_OK ||
        !rawBoundImportFunction) {
        error = "CKAngelScript API self-test expected raw import binding setup to succeed.";
        api->UnbindImportedFunction(importConsumerModuleName, 0, nullptr);
        api->ReleaseFunction(rawBoundImportFunction);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    BoundImportEdgeProbe rawBoundImportProbe;
    if (api->EnumerateBoundImportEdges(importConsumerModuleName,
                                       ProbeBoundImportEdge,
                                       &rawBoundImportProbe,
                                       &result) != CKAS_OK ||
        rawBoundImportProbe.CallbackCount != 0) {
        error = "CKAngelScript API self-test expected raw AngelScript import binds to stay outside the CKAS graph.";
        api->UnbindImportedFunction(importConsumerModuleName, 0, nullptr);
        api->ReleaseFunction(rawBoundImportFunction);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    const CKDWORD rawBoundImportGeneration = api->GetModuleGeneration(importConsumerModuleName);
    CKAngelScriptExecution *rawUnbindExecution = nullptr;
    if (api->UnbindImportedFunction(importConsumerModuleName, 0, &result) != CKAS_OK ||
        api->GetModuleGeneration(importConsumerModuleName) != rawBoundImportGeneration + 1 ||
        api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(rawBoundImportFunction),
                                     &rawUnbindExecution,
                                     &result) != CKAS_STALEHANDLE ||
        rawUnbindExecution != nullptr) {
        error = "CKAngelScript API self-test expected CKAS unbind to stale handles after raw import binds.";
        if (rawUnbindExecution) {
            api->ReleaseExecution(rawUnbindExecution);
        }
        api->ReleaseFunction(rawBoundImportFunction);
        api->UnbindImportedFunction(importConsumerModuleName, 0, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->ReleaseFunction(rawBoundImportFunction);
    constexpr const char *rawCkasImportConsumerModuleName = "__CKAS_RawCkasImportConsumer";
    const char *rawCkasImportConsumerSource =
        "import int __ckas_import_add(int value) from \"__CKAS_ImportProvider\";\n"
        "int __ckas_raw_ckas_import_call() { return __ckas_import_add(9); }\n";
    asIScriptModule *rawCkasImportConsumer =
        engine->GetModule(rawCkasImportConsumerModuleName, asGM_ALWAYS_CREATE);
    if (!rawCkasImportConsumer ||
        rawCkasImportConsumer->AddScriptSection(rawCkasImportConsumerModuleName,
                                                rawCkasImportConsumerSource,
                                                static_cast<unsigned int>(std::strlen(rawCkasImportConsumerSource))) < 0 ||
        rawCkasImportConsumer->Build() < 0) {
        error = "CKAngelScript API self-test failed to create a raw import consumer module.";
        if (rawCkasImportConsumer) {
            rawCkasImportConsumer->Discard();
        }
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    BoundImportEdgeProbe rawCkasBoundImportProbe;
    rawCkasBoundImportProbe.ExpectedImportModuleName = rawCkasImportConsumerModuleName;
    const CKAS_STATUS rawCkasBindStatus =
        api->BindAllImportedFunctions(rawCkasImportConsumerModuleName, &result);
    if (rawCkasBindStatus != CKAS_OK) {
        error = "CKAngelScript API self-test expected CKAS binding of a raw import consumer to succeed: ";
        error += std::to_string(static_cast<int>(rawCkasBindStatus));
        api->UnbindAllImportedFunctions(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->EnumerateBoundImportEdges(rawCkasImportConsumerModuleName,
                                       ProbeBoundImportEdge,
                                       &rawCkasBoundImportProbe,
                                       &result) != CKAS_OK ||
        rawCkasBoundImportProbe.CallbackCount != 1 ||
        !rawCkasBoundImportProbe.SawEdge) {
        error = "CKAngelScript API self-test expected CKAS-bound raw import consumers to appear in the bound graph.";
        api->UnbindAllImportedFunctions(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    const CKAS_STATUS rawCkasProviderReplaceStatus =
        api->CompileModule(importProviderModuleName,
                           importProviderSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result);
    if (rawCkasProviderReplaceStatus != CKAS_INUSE ||
        !CkasStringContains(result.ErrorMessage, rawCkasImportConsumerModuleName)) {
        error = "CKAngelScript API self-test expected CKAS-bound raw import consumers to block provider replacement: ";
        error += std::to_string(static_cast<int>(rawCkasProviderReplaceStatus));
        if (result.ErrorMessage) {
            error += " ";
            error += result.ErrorMessage;
        }
        api->UnbindAllImportedFunctions(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->UnbindAllImportedFunctions(rawCkasImportConsumerModuleName, &result) != CKAS_OK ||
        api->CompileModule(importProviderModuleName,
                           importProviderSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->UnloadModule(rawCkasImportConsumerModuleName, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected unbound raw import consumers to leave provider replacement unblocked.";
        api->UnloadModule(rawCkasImportConsumerModuleName, nullptr);
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    if (api->BindImportedFunction(importConsumerModuleName,
                                  3,
                                  importProviderModuleName,
                                  "int __ckas_import_add(int)",
                                  &result) != CKAS_INVALIDARGUMENT ||
        api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  "__CKAS_MissingImportProvider",
                                  "int __ckas_import_add(int)",
                                  &result) != CKAS_NOTFOUND ||
        api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "int __ckas_missing_import(int)",
                                  &result) != CKAS_NOTFOUND ||
        api->BindImportedFunction(importConsumerModuleName,
                                  0,
                                  importProviderModuleName,
                                  "void __ckas_import_wrong()",
                                  &result) != CKAS_TYPEMISMATCH) {
        error = "CKAngelScript API self-test expected import binding failures to report stable statuses.";
        api->UnloadModule(importConsumerModuleName, nullptr);
        api->UnloadModule(importProviderModuleName, nullptr);
        return false;
    }
    api->UnloadModule(importConsumerModuleName, nullptr);
    api->UnloadModule(importProviderModuleName, nullptr);

    constexpr const char *namespaceImportProviderModuleName = "__CKAS_NamespaceImportProvider";
    constexpr const char *namespaceImportConsumerModuleName = "__CKAS_NamespaceImportConsumer";
    const char *namespaceImportProviderSource =
        "namespace CKASImportNamespace {\n"
        "int Add(int value) { return value + 40; }\n"
        "}\n";
    const char *namespaceImportConsumerSource =
        "namespace CKASImportNamespace {\n"
        "import int Add(int value) from \"__CKAS_NamespaceImportProvider\";\n"
        "}\n"
        "int __ckas_namespace_import_call() { return CKASImportNamespace::Add(2); }\n";
    if (api->CompileModule(namespaceImportProviderModuleName,
                           namespaceImportProviderSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(namespaceImportConsumerModuleName,
                           namespaceImportConsumerSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->BindAllImportedFunctions(namespaceImportConsumerModuleName, &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            namespaceImportConsumerModuleName,
                            "int __ckas_namespace_import_call()",
                            importValue,
                            result,
                            error) ||
        importValue != 42) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected default import binding to resolve namespaced declarations.";
        }
        api->UnloadModule(namespaceImportConsumerModuleName, nullptr);
        api->UnloadModule(namespaceImportProviderModuleName, nullptr);
        return false;
    }
    api->UnloadModule(namespaceImportConsumerModuleName, nullptr);
    api->UnloadModule(namespaceImportProviderModuleName, nullptr);

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
    const std::filesystem::path singleIncludeFile = tempDir / "__ckas_public_api_single_include.as";
    const std::filesystem::path multiFileA = tempDir / "__ckas_public_api_multi_a.as";
    const std::filesystem::path multiFileB = tempDir / "__ckas_public_api_multi_b.as";
    const std::filesystem::path defaultFile = std::filesystem::current_path() / "__CKAS_ManagerApiDefaultFileLoadSelfTest.as";
    auto cleanupLoadFiles = [&]() {
        RemoveTextFile(singleFile);
        RemoveTextFile(singleIncludeFile);
        RemoveTextFile(multiFileA);
        RemoveTextFile(multiFileB);
        RemoveTextFile(defaultFile);
    };
    if (!WriteTextFile(singleIncludeFile, "int __ckas_public_file_include_value() { return 10; }\n", error) ||
        !WriteTextFile(singleFile,
                       "#include \"__ckas_public_api_single_include.as\"\n"
                       "int __ckas_public_file_loaded() { return __ckas_public_file_include_value() + 1; }\n",
                       error) ||
        !WriteTextFile(multiFileA, "int __ckas_public_multi_a() { return 12; }\n", error) ||
        !WriteTextFile(multiFileB, "int __ckas_public_multi_b() { return __ckas_public_multi_a() + 1; }\n", error) ||
        !WriteTextFile(defaultFile, "int __ckas_public_default_file_loaded() { return 14; }\n", error)) {
        cleanupLoadFiles();
        return false;
    }

    constexpr const char *singleFileModuleName = "__CKAS_ManagerApiSingleFileLoadSelfTest";
    const std::string singleFilePath = singleFile.string();
    CKAngelScriptLoadOptions singleFileOptions =
        CKAngelScriptApi::LoadFileOptions(singleFileModuleName, singleFilePath.c_str(), CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(singleFileOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(singleFileModuleName, "int __ckas_public_file_loaded()", &borrowedFunction, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected single-file LoadModule to expose its function.";
        cleanupLoadFiles();
        return false;
    }
    const std::string singleFileSection = singleFile.generic_string();
    const std::string singleIncludeFileSection = singleIncludeFile.generic_string();
    IncludeEdgeProbe fileIncludeEdgeProbe;
    fileIncludeEdgeProbe.ExpectedModuleName = singleFileModuleName;
    fileIncludeEdgeProbe.ExpectedFromSection = singleFileSection.c_str();
    fileIncludeEdgeProbe.ExpectedToSection = singleIncludeFileSection.c_str();
    fileIncludeEdgeProbe.ExpectedResolvedFromSnapshot = FALSE;
    if (api->EnumerateModuleIncludeEdges(singleFileModuleName,
                                         ProbeIncludeEdge,
                                         &fileIncludeEdgeProbe,
                                         &result) != CKAS_OK ||
        fileIncludeEdgeProbe.CallbackCount == 0 ||
        !fileIncludeEdgeProbe.SawHelperInclude) {
        error = "CKAngelScript API self-test expected file LoadModule include edges.";
        cleanupLoadFiles();
        return false;
    }
    int fileLoadValue = 0;
    if (!WriteTextFile(singleFile, "int __ckas_public_file_loaded() { return 21; }\n", error) ||
        api->LoadModule(singleFileOptions, &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            singleFileModuleName,
                            "int __ckas_public_file_loaded()",
                            fileLoadValue,
                            result,
                            error) ||
        fileLoadValue != 21) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected single-file LoadModule replacement to read updated file code.";
        }
        cleanupLoadFiles();
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
        cleanupLoadFiles();
        return false;
    }
    int multiFileLoadValue = 0;
    if (!WriteTextFile(multiFileA, "int __ckas_public_multi_a() { return 30; }\n", error) ||
        !WriteTextFile(multiFileB, "int __ckas_public_multi_b() { return __ckas_public_multi_a() + 2; }\n", error) ||
        api->LoadModule(multiFileOptions, &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            multiFileModuleName,
                            "int __ckas_public_multi_b()",
                            multiFileLoadValue,
                            result,
                            error) ||
        multiFileLoadValue != 32) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected multi-file LoadModule replacement to read updated file code.";
        }
        cleanupLoadFiles();
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
        cleanupLoadFiles();
        return false;
    }
    MetadataProbe sourceSectionMetadataProbe;
    if (api->EnumerateMetadata(sourceSectionsModuleName, ProbeMetadata, &sourceSectionMetadataProbe, &result) != CKAS_OK ||
        !sourceSectionMetadataProbe.SourceSectionType) {
        error = "CKAngelScript API self-test expected source-section LoadModule to preserve metadata from included sections.";
        cleanupLoadFiles();
        return false;
    }
    IncludeEdgeProbe includeEdgeProbe;
    CKAngelScriptModuleFingerprint sourceSectionFingerprint = CKAngelScriptApi::ModuleFingerprint();
    if (api->EnumerateModuleIncludeEdges(sourceSectionsModuleName,
                                         ProbeIncludeEdge,
                                         &includeEdgeProbe,
                                         &result) != CKAS_OK ||
        includeEdgeProbe.CallbackCount == 0 ||
        !includeEdgeProbe.SawHelperInclude ||
        api->EnumerateModuleIncludeEdges(sourceSectionsModuleName, nullptr, nullptr, &result) != CKAS_INVALIDARGUMENT ||
        api->GetModuleFingerprint(sourceSectionsModuleName, &sourceSectionFingerprint, &result) != CKAS_OK ||
        sourceSectionFingerprint.Kind != CKAS_MODULEKIND_SOURCE ||
        sourceSectionFingerprint.SourceHash == 0 ||
        sourceSectionFingerprint.IncludeHash == 0 ||
        sourceSectionFingerprint.CombinedHash == 0) {
        error = "CKAngelScript API self-test expected source-section include edges and fingerprint data.";
        cleanupLoadFiles();
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
        cleanupLoadFiles();
        return false;
    }

    CKAngelScriptSourceSection duplicateSourceSections[2] = {};
    duplicateSourceSections[0].Size = sizeof(duplicateSourceSections[0]);
    duplicateSourceSections[0].SectionName = "entry/lib/../duplicate.as";
    duplicateSourceSections[0].Code = sourceSectionEntry;
    duplicateSourceSections[1].Size = sizeof(duplicateSourceSections[1]);
    duplicateSourceSections[1].SectionName = "entry/duplicate.as";
    duplicateSourceSections[1].Code = sourceSectionHelper;
    CKAngelScriptLoadOptions duplicateSourceSectionOptions =
        CKAngelScriptApi::LoadSectionsOptions("__CKAS_ManagerApiDuplicateSourceSectionsSelfTest",
                                              duplicateSourceSections,
                                              2,
                                              CKAS_LOAD_REPLACEEXISTING);
    if (api->LoadModule(duplicateSourceSectionOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with duplicate source sections to fail.";
        cleanupLoadFiles();
        return false;
    }

    constexpr const char *defaultFileModuleName = "__CKAS_ManagerApiDefaultFileLoadSelfTest";
    CKAngelScriptLoadOptions defaultFileOptions = CKAngelScriptApi::LoadOptions();
    defaultFileOptions.ModuleName = defaultFileModuleName;
    defaultFileOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(defaultFileOptions, &result) != CKAS_OK ||
        api->BorrowFunctionByDecl(defaultFileModuleName, "int __ckas_public_default_file_loaded()", &borrowedFunction, &result) != CKAS_OK) {
        error = "CKAngelScript API self-test expected default-file LoadModule to expose its function.";
        cleanupLoadFiles();
        return false;
    }
    api->UnloadModule(defaultFileModuleName, nullptr);

    const char *emptyFileList[] = { multiFilePathA.c_str() };
    CKAngelScriptLoadOptions emptyFileListOptions = CKAngelScriptApi::LoadOptions();
    emptyFileListOptions.ModuleName = "__CKAS_ManagerApiEmptyFileListSelfTest";
    emptyFileListOptions.Filenames = emptyFileList;
    emptyFileListOptions.FileCount = 0;
    emptyFileListOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(emptyFileListOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with Filenames and zero FileCount to fail.";
        cleanupLoadFiles();
        return false;
    }

    CKAngelScriptLoadOptions emptySectionListOptions = CKAngelScriptApi::LoadOptions();
    emptySectionListOptions.ModuleName = "__CKAS_ManagerApiEmptySectionListSelfTest";
    emptySectionListOptions.Sections = sourceSections;
    emptySectionListOptions.SectionCount = 0;
    emptySectionListOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(emptySectionListOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with Sections and zero SectionCount to fail.";
        cleanupLoadFiles();
        return false;
    }

    const char *invalidFiles[] = { multiFilePathA.c_str(), nullptr };
    CKAngelScriptLoadOptions invalidFileListOptions = CKAngelScriptApi::LoadOptions();
    invalidFileListOptions.ModuleName = "__CKAS_ManagerApiInvalidFileListSelfTest";
    invalidFileListOptions.Filenames = invalidFiles;
    invalidFileListOptions.FileCount = 2;
    invalidFileListOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(invalidFileListOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with an invalid file list entry to fail.";
        cleanupLoadFiles();
        return false;
    }

    CKAngelScriptLoadOptions conflictingOptions = CKAngelScriptApi::LoadOptions();
    conflictingOptions.ModuleName = "__CKAS_ManagerApiConflictingLoadSelfTest";
    conflictingOptions.Code = "int __ckas_public_conflict() { return 1; }\n";
    conflictingOptions.Filename = singleFilePath.c_str();
    conflictingOptions.Flags = CKAS_LOAD_REPLACEEXISTING;
    if (api->LoadModule(conflictingOptions, &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected LoadModule with multiple sources to fail.";
        cleanupLoadFiles();
        return false;
    }
    cleanupLoadFiles();

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
        executionResult->CompilerMessages ||
        executionResult->CompilerMessageCount != 0 ||
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

    ReentrantExecutionCallbackProbe executionReentry;
    executionReentry.Api = &api;
    executionReentry.Input = 21;
    CKAngelScriptExecution *reentrantExecution = nullptr;
    CKAngelScriptExecutionStepOptions reentrantStepOptions =
        CKAngelScriptApi::ExecutionStepOptions(ConfigureIntArgumentWithReentry,
                                               ReadIntReturnWithReentryProbe,
                                               &executionReentry);
    if (api->CreateFunctionExecution(executeOptions, &reentrantExecution, &result) != CKAS_OK ||
        !reentrantExecution ||
        api->StartExecution(reentrantExecution, reentrantStepOptions, &result) != CKAS_OK ||
        executionReentry.CallbackCount != 1 ||
        executionReentry.ReentryStatus != CKAS_INVALIDSTATE ||
        executionReentry.Output != 26) {
        error = "CKAngelScript API self-test expected execution callbacks to reject module mutation reentry.";
        api->ReleaseExecution(reentrantExecution);
        api->UnloadModule("__CKAS_ExecutionCallbackReentry", nullptr);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(reentrantExecution);

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

    CKAngelScriptFunctionOptions spawnAsyncOptions = CKAngelScriptApi::FunctionOptions();
    spawnAsyncOptions.ModuleName = moduleName;
    spawnAsyncOptions.FunctionDecl = "void __ckas_public_spawn_async_dependency()";
    CKAngelScriptFunction *spawnAsyncFunction = nullptr;
    if (api->FindFunction(spawnAsyncOptions, &spawnAsyncFunction, &result) != CKAS_OK ||
        !spawnAsyncFunction) {
        error = "CKAngelScript API self-test failed to find async dependency spawner.";
        api->ReleaseFunction(addFunction);
        return false;
    }
    CKAngelScriptExecution *spawnAsyncExecution = nullptr;
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(spawnAsyncFunction),
                                     &spawnAsyncExecution,
                                     &result) != CKAS_OK ||
        !spawnAsyncExecution ||
        api->StartExecution(spawnAsyncExecution, nullptr, &result) != CKAS_OK) {
        error = std::string("CKAngelScript API self-test failed to spawn an internal async module dependency: ") +
                (result.ErrorMessage && result.ErrorMessage[0] ? result.ErrorMessage : CKAngelScriptGetStatusName(result.Status));
        if (result.StackTrace && result.StackTrace[0]) {
            error += " ";
            error += result.StackTrace;
        }
        api->ReleaseExecution(spawnAsyncExecution);
        api->ReleaseFunction(spawnAsyncFunction);
        api->ReleaseFunction(addFunction);
        return false;
    }
    api->ReleaseExecution(spawnAsyncExecution);
    api->ReleaseFunction(spawnAsyncFunction);
    if (api->UnloadModule(moduleName, &result) != CKAS_INUSE ||
        api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected internal async tasks to block module unload/replace.";
        if (manager && manager->GetAsyncScheduler()) {
            manager->GetAsyncScheduler()->Clear();
        }
        api->ReleaseFunction(addFunction);
        return false;
    }
    if (manager && manager->GetAsyncScheduler()) {
        manager->GetAsyncScheduler()->Clear();
    }

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
        !ContainsStructuredCompileDiagnostic(result) ||
        api->HasModule(badModuleName)) {
        error = "CKAngelScript API self-test expected compile errors to include structured AngelScript diagnostics.";
        api->UnloadModule(badModuleName, nullptr);
        return false;
    }
    api->UnloadModule(badModuleName, nullptr);

    constexpr const char *bytecodeSourceModuleName = "__CKAS_BytecodeSourceSelfTest";
    constexpr const char *bytecodeReplacementSourceModuleName = "__CKAS_BytecodeReplacementSourceSelfTest";
    constexpr const char *bytecodeTargetModuleName = "__CKAS_BytecodeTargetSelfTest";
    const char *bytecodeSource =
        "int __ckas_bytecode_value() { return 31; }\n";
    const char *bytecodeReplacementSource =
        "int __ckas_bytecode_value() { return 44; }\n";
    if (api->CompileModule(bytecodeSourceModuleName,
                           bytecodeSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK ||
        api->CompileModule(bytecodeReplacementSourceModuleName,
                           bytecodeReplacementSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test failed to compile bytecode source modules.";
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    BytecodeBuffer bytecode;
    if (api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(bytecodeSourceModuleName,
                                                                      WriteBytecode,
                                                                      &bytecode),
                                &result) != CKAS_OK ||
        bytecode.Bytes.empty() ||
        api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(bytecodeSourceModuleName,
                                                                      RejectBytecodeWrite),
                                &result) != CKAS_CANCELLED) {
        error = "CKAngelScript API self-test expected bytecode save and write failure statuses.";
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    if (api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(bytecodeSourceModuleName,
                                                                      WriteBytecode,
                                                                      &bytecode,
                                                                      CKAS_BYTECODE_REPLACEEXISTING),
                                &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected bytecode save to reject load-only flags.";
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    ReentrantBytecodeWriteProbe reentrantBytecodeWrite;
    reentrantBytecodeWrite.Api = &api;
    reentrantBytecodeWrite.ModuleName = bytecodeSourceModuleName;
    if (api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(bytecodeSourceModuleName,
                                                                      WriteBytecodeWithReentry,
                                                                      &reentrantBytecodeWrite),
                                &result) != CKAS_OK ||
        reentrantBytecodeWrite.Buffer.Bytes.empty() ||
        reentrantBytecodeWrite.ReentryStatus != CKAS_INVALIDSTATE ||
        reentrantBytecodeWrite.BytecodeSaveReentryStatus != CKAS_INVALIDSTATE) {
        error = "CKAngelScript API self-test expected bytecode callbacks to reject module mutation reentry.";
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    BytecodeBuffer bytecodeRead = bytecode;
    int bytecodeValue = 0;
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeTargetModuleName,
                                                                      ReadBytecode,
                                                                      &bytecodeRead),
                                &result) != CKAS_OK ||
        !ExecuteIntFunction(api,
                            bytecodeTargetModuleName,
                            "int __ckas_bytecode_value()",
                            bytecodeValue,
                            result,
                            error) ||
        bytecodeValue != 31) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected bytecode load to create an executable module.";
        }
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    IncludeEdgeProbe bytecodeIncludeProbe;
    CKAngelScriptModuleFingerprint bytecodeFingerprint = CKAngelScriptApi::ModuleFingerprint();
    if (api->EnumerateModuleIncludeEdges(bytecodeTargetModuleName,
                                         ProbeIncludeEdge,
                                         &bytecodeIncludeProbe,
                                         &result) != CKAS_OK ||
        bytecodeIncludeProbe.CallbackCount != 0 ||
        api->GetModuleFingerprint(bytecodeTargetModuleName, &bytecodeFingerprint, &result) != CKAS_OK ||
        bytecodeFingerprint.Kind != CKAS_MODULEKIND_BYTECODE ||
        bytecodeFingerprint.Generation == 0 ||
        bytecodeFingerprint.CombinedHash == 0) {
        error = "CKAngelScript API self-test expected bytecode modules to expose an empty include graph and fingerprint.";
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    bytecodeRead = bytecode;
    const CKDWORD bytecodeGenerationBeforeFailedReplace = api->GetModuleGeneration(bytecodeTargetModuleName);
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeTargetModuleName,
                                                                      ReadBytecode,
                                                                      &bytecodeRead),
                                &result) != CKAS_ALREADYEXISTS ||
        api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeTargetModuleName,
                                                                      RejectBytecodeRead,
                                                                      nullptr,
                                                                      CKAS_BYTECODE_REPLACEEXISTING),
                                &result) != CKAS_CANCELLED ||
        api->GetModuleGeneration(bytecodeTargetModuleName) != bytecodeGenerationBeforeFailedReplace ||
        !ExecuteIntFunction(api,
                            bytecodeTargetModuleName,
                            "int __ckas_bytecode_value()",
                            bytecodeValue,
                            result,
                            error) ||
        bytecodeValue != 31) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected failed bytecode replacement to preserve the old module.";
        }
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    bytecodeRead = bytecode;
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions("__CKAS_BytecodeInvalidFlagSelfTest",
                                                                      ReadBytecode,
                                                                      &bytecodeRead,
                                                                      CKAS_BYTECODE_STRIP_DEBUG_INFO),
                                &result) != CKAS_INVALIDARGUMENT) {
        error = "CKAngelScript API self-test expected bytecode load to reject save-only flags.";
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }

    CKAngelScriptFunction *staleBytecodeFunction = nullptr;
    if (api->FindFunction(CKAngelScriptApi::FunctionByDeclOptions(bytecodeTargetModuleName,
                                                                  "int __ckas_bytecode_value()"),
                          &staleBytecodeFunction,
                          &result) != CKAS_OK ||
        !staleBytecodeFunction) {
        error = "CKAngelScript API self-test expected to capture a bytecode function handle.";
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }

    BytecodeBuffer replacementBytecode;
    if (api->SaveModuleBytecode(CKAngelScriptApi::BytecodeSaveOptions(bytecodeReplacementSourceModuleName,
                                                                      WriteBytecode,
                                                                      &replacementBytecode),
                                &result) != CKAS_OK ||
        replacementBytecode.Bytes.empty()) {
        error = "CKAngelScript API self-test expected replacement bytecode to save.";
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    BytecodeBuffer replacementBytecodeRead = replacementBytecode;
    CKAngelScriptExecution *liveBytecodeExecution = nullptr;
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(staleBytecodeFunction),
                                     &liveBytecodeExecution,
                                     &result) != CKAS_OK ||
        !liveBytecodeExecution) {
        error = "CKAngelScript API self-test expected to create a live bytecode execution handle.";
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeTargetModuleName,
                                                                      ReadBytecode,
                                                                      &replacementBytecodeRead,
                                                                      CKAS_BYTECODE_REPLACEEXISTING),
                                &result) != CKAS_INUSE ||
        api->GetModuleGeneration(bytecodeTargetModuleName) != bytecodeGenerationBeforeFailedReplace ||
        !ExecuteIntFunction(api,
                            bytecodeTargetModuleName,
                            "int __ckas_bytecode_value()",
                            bytecodeValue,
                            result,
                            error) ||
        bytecodeValue != 31) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected live execution handles to block bytecode replacement.";
        }
        api->ReleaseExecution(liveBytecodeExecution);
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    api->ReleaseExecution(liveBytecodeExecution);
    replacementBytecodeRead = replacementBytecode;
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeTargetModuleName,
                                                                      ReadBytecode,
                                                                      &replacementBytecodeRead,
                                                                      CKAS_BYTECODE_REPLACEEXISTING),
                                &result) != CKAS_OK ||
        api->GetModuleGeneration(bytecodeTargetModuleName) != bytecodeGenerationBeforeFailedReplace + 1 ||
        !ExecuteIntFunction(api,
                            bytecodeTargetModuleName,
                            "int __ckas_bytecode_value()",
                            bytecodeValue,
                            result,
                            error) ||
        bytecodeValue != 44) {
        if (error.empty()) {
            error = "CKAngelScript API self-test expected bytecode replacement to commit and bump generation.";
        }
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    CKAngelScriptModuleFingerprint replacedBytecodeFingerprint = CKAngelScriptApi::ModuleFingerprint();
    if (api->GetModuleFingerprint(bytecodeTargetModuleName, &replacedBytecodeFingerprint, &result) != CKAS_OK ||
        replacedBytecodeFingerprint.Kind != CKAS_MODULEKIND_BYTECODE ||
        replacedBytecodeFingerprint.CombinedHash == bytecodeFingerprint.CombinedHash) {
        error = "CKAngelScript API self-test expected bytecode replacement to change the module fingerprint.";
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    CKAngelScriptExecution *staleBytecodeExecution =
        reinterpret_cast<CKAngelScriptExecution *>(static_cast<uintptr_t>(1));
    if (api->CreateFunctionExecution(CKAngelScriptApi::FunctionExecutionOptions(staleBytecodeFunction),
                                     &staleBytecodeExecution,
                                     &result) != CKAS_STALEHANDLE ||
        staleBytecodeExecution != nullptr) {
        error = "CKAngelScript API self-test expected bytecode replacement to stale old function handles.";
        api->ReleaseFunction(staleBytecodeFunction);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    api->ReleaseFunction(staleBytecodeFunction);

    constexpr const char *bytecodeObjectModuleName = "__CKAS_BytecodeObjectSelfTest";
    const char *bytecodeObjectSource =
        "class __CKAS_BytecodeObject {}\n";
    if (api->CompileModule(bytecodeObjectModuleName,
                           bytecodeObjectSource,
                           CKAS_COMPILE_REPLACEEXISTING,
                           &result) != CKAS_OK) {
        error = "CKAngelScript API self-test failed to compile bytecode object module.";
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    CKAngelScriptObject *bytecodeObject = nullptr;
    if (api->CreateObject(CKAngelScriptApi::ObjectOptions(bytecodeObjectModuleName,
                                                         "__CKAS_BytecodeObject"),
                          &bytecodeObject,
                          &result) != CKAS_OK ||
        !bytecodeObject) {
        error = "CKAngelScript API self-test failed to create bytecode object handle.";
        api->UnloadModule(bytecodeObjectModuleName, nullptr);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    replacementBytecodeRead = replacementBytecode;
    if (api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeObjectModuleName,
                                                                      ReadBytecode,
                                                                      &replacementBytecodeRead),
                                &result) != CKAS_ALREADYEXISTS) {
        error = "CKAngelScript API self-test expected bytecode duplicate loads to report already-exists before live handle state.";
        api->ReleaseObject(bytecodeObject);
        api->UnloadModule(bytecodeObjectModuleName, nullptr);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    replacementBytecodeRead = replacementBytecode;
    const CKAS_STATUS liveObjectReplaceStatus =
        api->LoadModuleBytecode(CKAngelScriptApi::BytecodeLoadOptions(bytecodeObjectModuleName,
                                                                      ReadBytecode,
                                                                      &replacementBytecodeRead,
                                                                      CKAS_BYTECODE_REPLACEEXISTING),
                                &result);
    if (liveObjectReplaceStatus != CKAS_INUSE) {
        error = "CKAngelScript API self-test expected live object handles to block bytecode replacement.";
        api->ReleaseObject(bytecodeObject);
        api->UnloadModule(bytecodeObjectModuleName, nullptr);
        api->UnloadModule(bytecodeTargetModuleName, nullptr);
        api->UnloadModule(bytecodeSourceModuleName, nullptr);
        api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);
        return false;
    }
    api->ReleaseObject(bytecodeObject);
    api->UnloadModule(bytecodeObjectModuleName, nullptr);
    api->UnloadModule(bytecodeTargetModuleName, nullptr);
    api->UnloadModule(bytecodeSourceModuleName, nullptr);
    api->UnloadModule(bytecodeReplacementSourceModuleName, nullptr);

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
    const CKAS_STATUS objectCreateStatus = api->CreateObject(objectOptions, &object, &result);
    if (objectCreateStatus != CKAS_OK || !object) {
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
    ReentrantObjectCallProbe objectReentry;
    objectReentry.Api = &api;
    objectReentry.Input = 32;
    objectCall.WriteArgs = WriteObjectIntWithReentry;
    objectCall.ReadResult = ReadObjectIntWithReentryProbe;
    objectCall.UserData = &objectReentry;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_OK ||
        objectReentry.CallbackCount != 1 ||
        objectReentry.ReentryStatus != CKAS_INVALIDSTATE ||
        objectReentry.Output != 42) {
        error = "CKAngelScript API self-test expected object callbacks to reject module mutation reentry.";
        api->UnloadModule("__CKAS_ObjectCallbackReentry", nullptr);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectCall.WriteArgs = WriteObjectInt;
    objectCall.ReadResult = ReadObjectInt;
    objectCall.UserData = &objectData;
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
    CKAngelScriptObject foreignObject = *object;
    foreignObject.Manager = reinterpret_cast<ScriptManager *>(static_cast<uintptr_t>(1));
    objectData.ObjectInput = &foreignObject;
    objectData.IntOutput = 0;
    if (api->CallObjectMethod(objectCall, &result) != CKAS_FOREIGNHANDLE ||
        objectData.IntOutput != 0) {
        error = "CKAngelScript API self-test expected foreign object-handle args to be rejected.";
        api->ReleaseMethod(handleMethod);
        api->ReleaseMethod(addMethod);
        api->ReleaseObject(object);
        return false;
    }
    objectData.ObjectInput = object;
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
