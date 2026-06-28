#include "ScriptManager.h"

#include <cstring>

#include "ScriptApiSupport.h"

#include "add_on/scriptarray/scriptarray.h"

namespace {

CKAS_STATUS StoreInvalidPublicHandleResult(CKAngelScriptResult *result) {
    return ScriptApiSupport::StoreStatelessPublicResult(result,
                                                        CKAS_INVALIDARGUMENT,
                                                        0,
                                                        "CKAngelScript handle is invalid.");
}

} // namespace

extern "C" CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context) {
    return ToPublicHandle(context ? ScriptManager::GetManager(context) : nullptr);
}

extern "C" CKAS_API CKDWORD CKAngelScriptGetApiVersion() {
    return CKAS_API_VERSION;
}

extern "C" CKAS_API CKBOOL CKAngelScriptHasFeature(CKAS_FEATURE feature) {
    switch (feature) {
        case CKAS_FEATURE_MODULE_LIFECYCLE:
        case CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS:
        case CKAS_FEATURE_FUNCTION_HANDLE:
        case CKAS_FEATURE_FUNCTION_EXECUTION:
        case CKAS_FEATURE_FUNCTION_EXECUTION_RESUME:
        case CKAS_FEATURE_OBJECT_HANDLE:
        case CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL:
        case CKAS_FEATURE_TYPED_ARG_READER_WRITER:
        case CKAS_FEATURE_STACK_TRACE:
        case CKAS_FEATURE_ENGINE_EXTENSION:
        case CKAS_FEATURE_PUBLIC_STRUCT_INITIALIZERS:
        case CKAS_FEATURE_STATUS_TEXT:
        case CKAS_FEATURE_METADATA_REFLECTION:
        case CKAS_FEATURE_OBJECT_TYPE_NAMESPACE:
        case CKAS_FEATURE_OBJECT_METHOD_CONTEXT_ACCESS:
        case CKAS_FEATURE_SCRIPT_ARRAY_ACCESS:
        case CKAS_FEATURE_ACTIVE_CONTEXT_EXCEPTION:
        case CKAS_FEATURE_SOURCE_SECTIONS:
        case CKAS_FEATURE_OBJECT_HANDLE_ARGS:
        case CKAS_FEATURE_HOST_CALL_FILTER:
        case CKAS_FEATURE_MODULE_IMPORTS:
        case CKAS_FEATURE_MODULE_BYTECODE:
        case CKAS_FEATURE_MODULE_REPLACE_TRANSACTION:
        case CKAS_FEATURE_MODULE_GRAPH:
        case CKAS_FEATURE_MODULE_FINGERPRINT:
            return TRUE;
        default:
            return FALSE;
    }
}

extern "C" CKAS_API void CKAngelScriptInitResult(CKAngelScriptResult *result) {
    ScriptApiSupport::InitPublicStruct(result);
}

extern "C" CKAS_API void CKAngelScriptInitLoadOptions(CKAngelScriptLoadOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitImportBindOptions(CKAngelScriptImportBindOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitBytecodeSaveOptions(CKAngelScriptBytecodeSaveOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitBytecodeLoadOptions(CKAngelScriptBytecodeLoadOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitModuleFingerprint(CKAngelScriptModuleFingerprint *fingerprint) {
    ScriptApiSupport::InitPublicStruct(fingerprint);
    if (fingerprint) {
        fingerprint->Kind = CKAS_MODULEKIND_UNKNOWN;
        fingerprint->ApiVersion = CKAS_API_VERSION;
    }
}

extern "C" CKAS_API void CKAngelScriptInitFunctionOptions(CKAngelScriptFunctionOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitFunctionExecutionOptions(
    CKAngelScriptFunctionExecutionOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitExecutionStepOptions(
    CKAngelScriptExecutionStepOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitObjectOptions(CKAngelScriptObjectOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitMethodOptions(CKAngelScriptMethodOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitObjectMethodExecuteOptions(
    CKAngelScriptObjectMethodExecuteOptions *options) {
    ScriptApiSupport::InitPublicStruct(options);
}

extern "C" CKAS_API void CKAngelScriptInitEngineExtension(CKAngelScriptEngineExtension *extension) {
    ScriptApiSupport::InitPublicStruct(extension);
}

extern "C" CKAS_API const char *CKAngelScriptGetStatusName(CKAS_STATUS status) {
    return ScriptApiSupport::StatusName(status);
}

extern "C" CKAS_API const char *CKAngelScriptGetStatusDescription(CKAS_STATUS status) {
    return ScriptApiSupport::StatusMessage(status);
}

extern "C" CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetVersion() : nullptr;
}

extern "C" CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetOptions() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowEngine(CKAngelScript *angelScript,
                                                          asIScriptEngine **outEngine,
                                                          CKAngelScriptResult *result) {
    if (outEngine) {
        *outEngine = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->BorrowEngine(outEngine, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowActiveContext(CKAngelScript *angelScript,
                                                                 asIScriptContext **outContext,
                                                                 CKAngelScriptResult *result) {
    if (outContext) {
        *outContext = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->BorrowActiveContext(outContext, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptSetActiveContextException(CKAngelScript *angelScript,
                                                                       const char *message,
                                                                       CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->SetActiveContextException(message, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptSetHostCallFilter(
    CKAngelScript *angelScript,
    CKAngelScriptHostCallFilterCallback callback,
    void *userData,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->SetHostCallFilter(callback, userData, result)
               : StoreInvalidPublicHandleResult(result);
}

static CScriptArray *AsPublicArray(void *array) {
    return static_cast<CScriptArray *>(array);
}

static const CScriptArray *AsPublicArray(const void *array) {
    return static_cast<const CScriptArray *>(array);
}

static bool IsArrayType(asITypeInfo *type) {
    return type && type->GetSubTypeCount() == 1 && type->GetName() &&
           std::strcmp(type->GetName(), "array") == 0;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptAssignObjectHandle(void **handleSlot,
                                                                 void *object,
                                                                 asITypeInfo *type) {
    if (!handleSlot || !type) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = type->GetEngine();
    if (!engine) {
        return CKAS_INVALIDARGUMENT;
    }

    if (*handleSlot == object) {
        return CKAS_OK;
    }
    if (*handleSlot) {
        engine->ReleaseScriptObject(*handleSlot, type);
        *handleSlot = nullptr;
    }
    if (object) {
        *handleSlot = object;
        engine->AddRefScriptObject(*handleSlot, type);
    }
    return CKAS_OK;
}

static CKAS_STATUS GetArrayElementAddress(CScriptArray *array, CKDWORD index, void **outAddress) {
    if (outAddress) {
        *outAddress = nullptr;
    }
    if (!array || !outAddress) {
        return CKAS_INVALIDARGUMENT;
    }
    if (index >= array->GetSize()) {
        return CKAS_INVALIDARGUMENT;
    }
    void *address = array->At(static_cast<asUINT>(index));
    if (!address) {
        return CKAS_INVALIDARGUMENT;
    }
    *outAddress = address;
    return CKAS_OK;
}

static CKAS_STATUS GetConstArrayElementAddress(const CScriptArray *array,
                                               CKDWORD index,
                                               const void **outAddress) {
    if (outAddress) {
        *outAddress = nullptr;
    }
    if (!array || !outAddress) {
        return CKAS_INVALIDARGUMENT;
    }
    if (index >= array->GetSize()) {
        return CKAS_INVALIDARGUMENT;
    }
    const void *address = array->At(static_cast<asUINT>(index));
    if (!address) {
        return CKAS_INVALIDARGUMENT;
    }
    *outAddress = address;
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateArrayByType(asITypeInfo *arrayType,
                                                                CKDWORD count,
                                                                void **outArray) {
    if (outArray) {
        *outArray = nullptr;
    }
    if (!outArray || !IsArrayType(arrayType)) {
        return CKAS_INVALIDARGUMENT;
    }
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(count));
    if (!array) {
        return CKAS_EXECUTIONFAILED;
    }
    *outArray = array;
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateArray(CKAngelScript *angelScript,
                                                          const char *arrayDecl,
                                                          CKDWORD count,
                                                          void **outArray) {
    if (outArray) {
        *outArray = nullptr;
    }
    if (!arrayDecl || !outArray) {
        return CKAS_INVALIDARGUMENT;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = scriptManager->GetScriptEngine();
    if (!engine) {
        return CKAS_NOTINITIALIZED;
    }
    asITypeInfo *arrayType = engine->GetTypeInfoByDecl(arrayDecl);
    if (!arrayType) {
        return CKAS_NOTFOUND;
    }
    return CKAngelScriptCreateArrayByType(arrayType, count, outArray);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayAddRef(void *array) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->AddRef();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayRelease(void *array) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->Release();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetRefCount(void *array, int *outRefCount) {
    if (outRefCount) {
        *outRefCount = 0;
    }
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !outRefCount) {
        return CKAS_INVALIDARGUMENT;
    }
    *outRefCount = scriptArray->GetRefCount();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetArrayType(void *array, asITypeInfo **outType) {
    if (outType) {
        *outType = nullptr;
    }
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !outType) {
        return CKAS_INVALIDARGUMENT;
    }
    *outType = scriptArray->GetArrayObjectType();
    return *outType ? CKAS_OK : CKAS_INVALIDSTATE;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetArrayTypeId(void *array, int *outTypeId) {
    if (outTypeId) {
        *outTypeId = 0;
    }
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !outTypeId) {
        return CKAS_INVALIDARGUMENT;
    }
    *outTypeId = scriptArray->GetArrayTypeId();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetSize(void *array, CKDWORD *outSize) {
    if (outSize) {
        *outSize = 0;
    }
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !outSize) {
        return CKAS_INVALIDARGUMENT;
    }
    *outSize = static_cast<CKDWORD>(scriptArray->GetSize());
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayResize(void *array, CKDWORD size) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->Resize(static_cast<asUINT>(size));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayReserve(void *array, CKDWORD capacity) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->Reserve(static_cast<asUINT>(capacity));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetElementTypeId(void *array, int *outTypeId) {
    if (outTypeId) {
        *outTypeId = 0;
    }
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !outTypeId) {
        return CKAS_INVALIDARGUMENT;
    }
    *outTypeId = scriptArray->GetElementTypeId();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetElementAddress(void *array,
                                                                     CKDWORD index,
                                                                     void **outAddress) {
    return GetArrayElementAddress(AsPublicArray(array), index, outAddress);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayGetConstElementAddress(const void *array,
                                                                          CKDWORD index,
                                                                          const void **outAddress) {
    return GetConstArrayElementAddress(AsPublicArray(array), index, outAddress);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArraySetElementValue(void *array,
                                                                   CKDWORD index,
                                                                   const void *value) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !value || index >= scriptArray->GetSize()) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->SetValue(static_cast<asUINT>(index), const_cast<void *>(value));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayInsertAt(void *array,
                                                            CKDWORD index,
                                                            const void *value) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !value || index > scriptArray->GetSize()) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->InsertAt(static_cast<asUINT>(index), const_cast<void *>(value));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayInsertLast(void *array, const void *value) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->InsertLast(const_cast<void *>(value));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayRemoveAt(void *array, CKDWORD index) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || index >= scriptArray->GetSize()) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->RemoveAt(static_cast<asUINT>(index));
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayRemoveLast(void *array) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray || scriptArray->GetSize() == 0) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->RemoveLast();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArrayClear(void *array) {
    CScriptArray *scriptArray = AsPublicArray(array);
    if (!scriptArray) {
        return CKAS_INVALIDARGUMENT;
    }
    scriptArray->Resize(0);
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                                             const CKAngelScriptLoadOptions *options,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->LoadModule(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "LoadModule options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCompileModule(CKAngelScript *angelScript,
                                                                const char *moduleName,
                                                                const char *scriptCode,
                                                                CKDWORD flags,
                                                                CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->CompileModule(moduleName, scriptCode, flags, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnloadModule(CKAngelScript *angelScript,
                                                               const char *moduleName,
                                                               CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->UnloadModule(moduleName, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKBOOL CKAngelScriptHasModule(CKAngelScript *angelScript, const char *moduleName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager && scriptManager->HasModule(moduleName) ? TRUE : FALSE;
}

extern "C" CKAS_API CKDWORD CKAngelScriptGetModuleGeneration(CKAngelScript *angelScript,
                                                             const char *moduleName) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetModuleGeneration(moduleName) : 0;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowModule(CKAngelScript *angelScript,
                                                          const char *moduleName,
                                                          asIScriptModule **outModule,
                                                          CKAngelScriptResult *result) {
    if (outModule) {
        *outModule = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager
               ? scriptManager->BorrowModule(moduleName, outModule, result)
               : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByName(CKAngelScript *angelScript,
                                                                  const char *moduleName,
                                                                  const char *functionName,
                                                                  asIScriptFunction **outFunction,
                                                                  CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowFunctionByName(moduleName, functionName, outFunction, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByDecl(CKAngelScript *angelScript,
                                                                  const char *moduleName,
                                                                  const char *functionDecl,
                                                                  asIScriptFunction **outFunction,
                                                                  CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowFunctionByDecl(moduleName, functionDecl, outFunction, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptEnumerateMetadata(CKAngelScript *angelScript,
                                                               const char *moduleName,
                                                               CKAngelScriptMetadataCallback callback,
                                                               void *userData,
                                                               CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->EnumerateMetadata(moduleName, callback, userData, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptGetImportedFunctionCount(CKAngelScript *angelScript,
                                                                      const char *moduleName,
                                                                      CKDWORD *outCount,
                                                                      CKAngelScriptResult *result) {
    if (outCount) {
        *outCount = 0;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetImportedFunctionCount(moduleName, outCount, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptEnumerateImportedFunctions(CKAngelScript *angelScript,
                                                                        const char *moduleName,
                                                                        CKAngelScriptImportCallback callback,
                                                                        void *userData,
                                                                        CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->EnumerateImportedFunctions(moduleName, callback, userData, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBindImportedFunction(CKAngelScript *angelScript,
                                                                  const CKAngelScriptImportBindOptions *options,
                                                                  CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->BindImportedFunction(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "BindImportedFunction options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBindAllImportedFunctions(CKAngelScript *angelScript,
                                                                      const char *moduleName,
                                                                      CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BindAllImportedFunctions(moduleName, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnbindImportedFunction(CKAngelScript *angelScript,
                                                                    const char *moduleName,
                                                                    CKDWORD importIndex,
                                                                    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnbindImportedFunction(moduleName, importIndex, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnbindAllImportedFunctions(CKAngelScript *angelScript,
                                                                        const char *moduleName,
                                                                        CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnbindAllImportedFunctions(moduleName, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptSaveModuleBytecode(CKAngelScript *angelScript,
                                                                const CKAngelScriptBytecodeSaveOptions *options,
                                                                CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->SaveModuleBytecode(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "SaveModuleBytecode options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptLoadModuleBytecode(CKAngelScript *angelScript,
                                                                const CKAngelScriptBytecodeLoadOptions *options,
                                                                CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->LoadModuleBytecode(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "LoadModuleBytecode options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptEnumerateBoundImportEdges(
    CKAngelScript *angelScript,
    const char *moduleName,
    CKAngelScriptBoundImportEdgeCallback callback,
    void *userData,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->EnumerateBoundImportEdges(moduleName, callback, userData, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptEnumerateModuleIncludeEdges(
    CKAngelScript *angelScript,
    const char *moduleName,
    CKAngelScriptIncludeEdgeCallback callback,
    void *userData,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->EnumerateModuleIncludeEdges(moduleName, callback, userData, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptGetModuleFingerprint(
    CKAngelScript *angelScript,
    const char *moduleName,
    CKAngelScriptModuleFingerprint *outFingerprint,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        if (outFingerprint && ScriptApiSupport::HasCompletePublicStruct(*outFingerprint)) {
            CKAngelScriptInitModuleFingerprint(outFingerprint);
        }
        return StoreInvalidPublicHandleResult(result);
    }
    return scriptManager->GetModuleFingerprint(moduleName, outFingerprint, result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptFindFunction(CKAngelScript *angelScript,
                                                          const CKAngelScriptFunctionOptions *options,
                                                          CKAngelScriptFunction **outFunction,
                                                          CKAngelScriptResult *result) {
    if (outFunction) {
        *outFunction = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->FindFunction(*options, outFunction, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "FindFunction options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseFunction(CKAngelScript *angelScript,
                                                             CKAngelScriptFunction *function,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseFunction(function, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateObject(CKAngelScript *angelScript,
                                                          const CKAngelScriptObjectOptions *options,
                                                          CKAngelScriptObject **outObject,
                                                          CKAngelScriptResult *result) {
    if (outObject) {
        *outObject = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->CreateObject(*options, outObject, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CreateObject options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                           CKAngelScriptObject *object,
                                                           CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseObject(object, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptFindObjectMethod(CKAngelScript *angelScript,
                                                              const CKAngelScriptMethodOptions *options,
                                                              CKAngelScriptMethod **outMethod,
                                                              CKAngelScriptResult *result) {
    if (outMethod) {
        *outMethod = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->FindObjectMethod(*options, outMethod, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "FindObjectMethod options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                           CKAngelScriptMethod *method,
                                                           CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseMethod(method, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options ? scriptManager->CallObjectMethod(*options, result)
                   : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CallObjectMethod options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetBool(CKAngelScriptArgWriter *writer,
                                                        CKDWORD index,
                                                        CKBOOL value) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (writer->Method->ParamTypes[index] != asTYPEID_BOOL) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgByte(static_cast<asUINT>(index), value ? 1 : 0);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetInt(CKAngelScriptArgWriter *writer,
                                                       CKDWORD index,
                                                       int value) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = writer->Context->GetEngine();
    if (!ScriptApiSupport::IsIntOrEnumType(engine, writer->Method->ParamTypes[index])) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgDWord(static_cast<asUINT>(index), static_cast<asDWORD>(value));
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetFloat(CKAngelScriptArgWriter *writer,
                                                         CKDWORD index,
                                                         float value) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (writer->Method->ParamTypes[index] != asTYPEID_FLOAT) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgFloat(static_cast<asUINT>(index), value);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetString(CKAngelScriptArgWriter *writer,
                                                          CKDWORD index,
                                                          const char *value) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = writer->Context->GetEngine();
    if (!ScriptApiSupport::IsValidStringParam(engine, writer->Method->ParamTypes[index], writer->Method->ParamFlags[index])) {
        return CKAS_TYPEMISMATCH;
    }
    CKAngelScriptStringArg *stringArg = nullptr;
    for (CKAngelScriptStringArg &arg : writer->StringArgs) {
        if (arg.Index == index) {
            stringArg = &arg;
            break;
        }
    }
    if (!stringArg) {
        if (writer->StringArgs.capacity() == 0) {
            writer->StringArgs.reserve(writer->Method->ParamTypes.size());
        }
        writer->StringArgs.push_back(CKAngelScriptStringArg());
        stringArg = &writer->StringArgs.back();
        stringArg->Index = index;
    }
    stringArg->Value = value ? value : "";
    const int r = writer->Context->SetArgObject(static_cast<asUINT>(index), &stringArg->Value);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetBorrowedObject(CKAngelScriptArgWriter *writer,
                                                                  CKDWORD index,
                                                                  void *object) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index) || !object) {
        return CKAS_INVALIDARGUMENT;
    }
    if (!ScriptApiSupport::IsValidBorrowedObjectParam(writer->Method->ParamTypes[index], writer->Method->ParamFlags[index])) {
        return CKAS_TYPEMISMATCH;
    }
    const int r = writer->Context->SetArgObject(static_cast<asUINT>(index), object);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptArgSetObjectHandle(CKAngelScriptArgWriter *writer,
                                                                CKDWORD index,
                                                                void *object) {
    if (!ScriptApiSupport::ValidateArgIndex(writer, index)) {
        return CKAS_INVALIDARGUMENT;
    }
    if (!ScriptApiSupport::IsValidObjectHandleParam(writer->Method->ParamTypes[index], writer->Method->ParamFlags[index])) {
        return CKAS_TYPEMISMATCH;
    }
    asIScriptObject *scriptObject = nullptr;
    if (object) {
        auto *objectHandle = static_cast<CKAngelScriptObject *>(object);
        ScriptManager *owner = ScriptManager::GetManager(writer->Context->GetEngine());
        if (!owner || !objectHandle->Manager) {
            return CKAS_INVALIDARGUMENT;
        }
        if (objectHandle->Manager != owner) {
            return CKAS_FOREIGNHANDLE;
        }
        if (!owner->OwnsObjectHandle(objectHandle) || !objectHandle->Object) {
            return CKAS_INVALIDARGUMENT;
        }
        if (!owner->HasModule(objectHandle->ModuleName.c_str()) ||
            owner->GetModuleGeneration(objectHandle->ModuleName.c_str()) != objectHandle->ModuleGeneration) {
            return CKAS_STALEHANDLE;
        }
        if (!ScriptApiSupport::IsCompatibleObjectHandle(writer->Context->GetEngine(),
                                      writer->Method->ParamTypes[index],
                                      objectHandle)) {
            return CKAS_TYPEMISMATCH;
        }
        scriptObject = objectHandle->Object;
    }
    const int r = writer->Context->SetArgObject(static_cast<asUINT>(index), scriptObject);
    return r < 0 ? CKAS_TYPEMISMATCH : CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetBool(CKAngelScriptResultReader *reader,
                                                           CKBOOL *value) {
    if (value) {
        *value = FALSE;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    if (reader->Method->ReturnType != asTYPEID_BOOL) {
        return CKAS_TYPEMISMATCH;
    }
    *value = reader->Context->GetReturnByte() ? TRUE : FALSE;
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetInt(CKAngelScriptResultReader *reader,
                                                          int *value) {
    if (value) {
        *value = 0;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = reader->Context->GetEngine();
    if (!ScriptApiSupport::IsIntOrEnumType(engine, reader->Method->ReturnType)) {
        return CKAS_TYPEMISMATCH;
    }
    *value = static_cast<int>(reader->Context->GetReturnDWord());
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetFloat(CKAngelScriptResultReader *reader,
                                                            float *value) {
    if (value) {
        *value = 0.0f;
    }
    if (!reader || !reader->Context || !reader->Method || !value) {
        return CKAS_INVALIDARGUMENT;
    }
    if (reader->Method->ReturnType != asTYPEID_FLOAT) {
        return CKAS_TYPEMISMATCH;
    }
    *value = reader->Context->GetReturnFloat();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetStringView(CKAngelScriptResultReader *reader,
                                                                 const char **data,
                                                                 size_t *size) {
    if (data) {
        *data = nullptr;
    }
    if (size) {
        *size = 0;
    }
    if (!reader || !reader->Context || !reader->Method || !data || !size) {
        return CKAS_INVALIDARGUMENT;
    }
    asIScriptEngine *engine = reader->Context->GetEngine();
    if (!ScriptApiSupport::IsStringType(engine, reader->Method->ReturnType)) {
        return CKAS_TYPEMISMATCH;
    }
    auto *value = static_cast<std::string *>(reader->Context->GetReturnObject());
    if (!value) {
        *data = nullptr;
        *size = 0;
        return CKAS_OK;
    }
    *data = value->data();
    *size = value->size();
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResultGetString(CKAngelScriptResultReader *reader,
                                                             char *buffer,
                                                             size_t bufferSize,
                                                             size_t *outRequiredSize) {
    if (outRequiredSize) {
        *outRequiredSize = 0;
    }
    if (buffer && bufferSize > 0) {
        buffer[0] = '\0';
    }
    const char *data = nullptr;
    size_t size = 0;
    const CKAS_STATUS status = CKAngelScriptResultGetStringView(reader, &data, &size);
    if (status != CKAS_OK) {
        return status;
    }
    const size_t required = size + 1;
    if (outRequiredSize) {
        *outRequiredSize = required;
    }
    if (!buffer || bufferSize < required) {
        return CKAS_BUFFERTOOSMALL;
    }
    if (size > 0 && data) {
        std::memcpy(buffer, data, size);
    }
    buffer[size] = '\0';
    return CKAS_OK;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCreateFunctionExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionExecutionOptions *options,
    CKAngelScriptExecution **outExecution,
    CKAngelScriptResult *result) {
    if (outExecution) {
        *outExecution = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return options
               ? scriptManager->CreateFunctionExecution(*options, outExecution, result)
               : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "CreateFunctionExecution options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                            CKAngelScriptExecution *execution,
                                                            const CKAngelScriptExecutionStepOptions *options,
                                                            CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->StartExecution(execution, options, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                             CKAngelScriptExecution *execution,
                                                             const CKAngelScriptExecutionStepOptions *options,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ResumeExecution(execution, options, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                             CKAngelScriptExecution *execution,
                                                             CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->CancelExecution(execution, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                              CKAngelScriptExecution *execution,
                                                              CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->ReleaseExecution(execution, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                               const CKAngelScriptExecution *execution,
                                                               CKAS_EXECUTIONSTATE *outState,
                                                               CKAngelScriptResult *result) {
    if (outState) {
        *outState = CKAS_EXECUTION_FAILED;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetExecutionState(execution, outState, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptBorrowExecutionResult(CKAngelScript *angelScript,
                                                                   const CKAngelScriptExecution *execution,
                                                                   const CKAngelScriptResult **outResult,
                                                                   CKAngelScriptResult *result) {
    if (outResult) {
        *outResult = nullptr;
    }
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->BorrowExecutionResult(execution, outResult, result)
                         : StoreInvalidPublicHandleResult(result);
}

extern "C" CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript) {
    const ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->GetLastResult() : nullptr;
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                                          const CKAngelScriptEngineExtension *extension,
                                                                          CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    if (!scriptManager) {
        return StoreInvalidPublicHandleResult(result);
    }
    return extension
               ? scriptManager->RegisterEngineExtension(*extension, result)
               : scriptManager->StoreApiResult(result, CKAS_INVALIDARGUMENT, 0, "Engine extension options are required.");
}

extern "C" CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                                       const char *name,
                                                                       CKAngelScriptResult *result) {
    ScriptManager *scriptManager = FromPublicHandle(angelScript);
    return scriptManager ? scriptManager->UnregisterEngineExtension(name, result)
                         : StoreInvalidPublicHandleResult(result);
}

