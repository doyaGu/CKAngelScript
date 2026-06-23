#ifndef CK_ANGELSCRIPT_H
#define CK_ANGELSCRIPT_H

#include "VxDefines.h"
#include "CKTypes.h"

#include <stddef.h>

#include <angelscript.h>

#define CKAS_GUID CKGUID(0x70955bd2,0x30684456)

#if defined(_WIN32)
#if defined(CKAS_BUILDING_DLL) || defined(CKAngelScript_EXPORTS)
#define CKAS_API __declspec(dllexport)
#else
#define CKAS_API __declspec(dllimport)
#endif
#else
#define CKAS_API
#endif

typedef struct CKAngelScriptExecution CKAngelScriptExecution;
typedef struct CKAngelScriptFunction CKAngelScriptFunction;
typedef struct CKAngelScriptObject CKAngelScriptObject;
typedef struct CKAngelScriptMethod CKAngelScriptMethod;
typedef struct CKAngelScriptArgWriter CKAngelScriptArgWriter;
typedef struct CKAngelScriptResultReader CKAngelScriptResultReader;
typedef struct CKBehaviorContext CKBehaviorContext;
typedef struct CKAngelScript CKAngelScript;

#define CKAS_API_VERSION 4

#ifdef __cplusplus
class CKContext;
#else
typedef struct CKContext CKContext;
#endif

typedef enum CKAS_STATUS {
    CKAS_OK = 0,
    CKAS_INVALIDARGUMENT = 1,
    CKAS_NOTINITIALIZED = 2,
    CKAS_NOTFOUND = 3,
    CKAS_COMPILEERROR = 4,
    CKAS_EXECUTIONFAILED = 5,
    CKAS_SUSPENDED = 6,
    CKAS_CANCELLED = 7,
    CKAS_STALEHANDLE = 8,
    CKAS_UNSUPPORTED = 9,
    CKAS_TYPEMISMATCH = 10,
    CKAS_BUFFERTOOSMALL = 11,
    CKAS_INVALIDSTATE = 12,
    CKAS_INUSE = 13,
    CKAS_ALREADYEXISTS = 14,
    CKAS_AMBIGUOUS = 15,
    CKAS_FOREIGNHANDLE = 16
} CKAS_STATUS;

typedef enum CKAS_MESSAGETYPE {
    CKAS_MESSAGE_ERROR = 0,
    CKAS_MESSAGE_WARNING = 1,
    CKAS_MESSAGE_INFORMATION = 2
} CKAS_MESSAGETYPE;

typedef enum CKAS_FEATURE {
    CKAS_FEATURE_MODULE_LIFECYCLE = 1,
    CKAS_FEATURE_RAW_ANGELSCRIPT_ACCESS = 2,
    CKAS_FEATURE_FUNCTION_HANDLE = 3,
    CKAS_FEATURE_FUNCTION_EXECUTION = 4,
    CKAS_FEATURE_FUNCTION_EXECUTION_RESUME = 5,
    CKAS_FEATURE_OBJECT_HANDLE = 6,
    CKAS_FEATURE_SYNC_OBJECT_METHOD_CALL = 7,
    CKAS_FEATURE_TYPED_ARG_READER_WRITER = 8,
    CKAS_FEATURE_STACK_TRACE = 9,
    CKAS_FEATURE_ENGINE_EXTENSION = 10,
    CKAS_FEATURE_PUBLIC_STRUCT_INITIALIZERS = 11,
    CKAS_FEATURE_STATUS_TEXT = 12,
    CKAS_FEATURE_METADATA_REFLECTION = 13,
    CKAS_FEATURE_OBJECT_TYPE_NAMESPACE = 14,
    CKAS_FEATURE_OBJECT_METHOD_CONTEXT_ACCESS = 15,
    CKAS_FEATURE_SCRIPT_ARRAY_ACCESS = 16
} CKAS_FEATURE;

typedef enum CKAS_EXECUTIONSTATE {
    CKAS_EXECUTION_READY = 0,
    CKAS_EXECUTION_RUNNING = 1,
    CKAS_EXECUTION_SUSPENDED = 2,
    CKAS_EXECUTION_FINISHED = 3,
    CKAS_EXECUTION_FAILED = 4,
    CKAS_EXECUTION_CANCELLED = 5
} CKAS_EXECUTIONSTATE;

typedef enum CKAS_LOADFLAGS {
    CKAS_LOAD_DEFAULT = 0,
    CKAS_LOAD_REPLACEEXISTING = 0x00000001
} CKAS_LOADFLAGS;

typedef enum CKAS_COMPILEFLAGS {
    CKAS_COMPILE_DEFAULT = 0,
    CKAS_COMPILE_REPLACEEXISTING = 0x00000001
} CKAS_COMPILEFLAGS;

typedef enum CKAS_ENGINEEXTENSIONFLAGS {
    CKAS_ENGINEEXTENSION_DEFAULT = 0,
    CKAS_ENGINEEXTENSION_DEFERRED = 0x00000001
} CKAS_ENGINEEXTENSIONFLAGS;

typedef enum CKAS_CALLFLAGS {
    CKAS_CALL_DEFAULT = 0,
    CKAS_CALL_NO_SUSPEND = 0x00000001
} CKAS_CALLFLAGS;

typedef enum CKAS_METADATA_TARGET {
    CKAS_METADATA_TYPE = 1,
    CKAS_METADATA_TYPE_METHOD = 2,
    CKAS_METADATA_GLOBAL_FUNCTION = 3,
    CKAS_METADATA_GLOBAL_VARIABLE = 4,
    CKAS_METADATA_TYPE_PROPERTY = 5
} CKAS_METADATA_TARGET;

// Callback user data is never owned by CKAngelScript. See each options struct
// for whether the pointer is borrowed only for the current call or retained by
// a public handle/registration.
typedef CKAS_STATUS (*CKAngelScriptContextCallback)(asIScriptContext *context, void *userData);
typedef int (*CKAngelScriptEngineExtensionCallback)(asIScriptEngine *engine,
                                                    CKAngelScript *angelScript,
                                                    void *userData,
                                                    const char **errorMessage);
typedef CKAS_STATUS (*CKAngelScriptWriteArgsCallback)(CKAngelScriptArgWriter *writer,
                                                      void *userData);
typedef CKAS_STATUS (*CKAngelScriptReadResultCallback)(CKAngelScriptResultReader *reader,
                                                       void *userData);

typedef struct CKAngelScriptMetadataEntry {
    CKDWORD Size;
    CKAS_METADATA_TARGET Target;
    const char *Name;
    const char *Namespace;
    const char *Declaration;
    const char *ParentTypeName;
    const char *ParentTypeNamespace;
    CKDWORD MetadataCount;
} CKAngelScriptMetadataEntry;

// Metadata callback pointers are borrowed only for the current callback
// invocation. Copy entry strings and metadata text if they must outlive the
// callback. The userData pointer passed to EnumerateMetadata is borrowed only
// until CKAngelScriptEnumerateMetadata returns.
typedef CKAS_STATUS (*CKAngelScriptMetadataCallback)(
    const CKAngelScriptMetadataEntry *entry,
    CKDWORD metadataIndex,
    const char *metadata,
    void *userData);

typedef struct CKAngelScriptLoadOptions {
    CKDWORD Size;
    // ModuleName and source strings are borrowed only for the LoadModule call.
    const char *ModuleName;
    const char *Filename;
    const char **Filenames;
    size_t FileCount;
    const char *Code;
    CKDWORD Flags;
} CKAngelScriptLoadOptions;

typedef struct CKAngelScriptFunctionOptions {
    CKDWORD Size;
    // Strings are borrowed only for the FindFunction call. Function handles
    // copy the resolved symbol identity.
    const char *ModuleName;
    const char *FunctionName;
    const char *FunctionDecl;
    CKDWORD Flags;
} CKAngelScriptFunctionOptions;

typedef struct CKAngelScriptFunctionExecutionOptions {
    CKDWORD Size;
    CKAngelScriptFunction *Function;
    // BehaviorContext is copied into the execution handle during
    // CreateFunctionExecution; the caller may release its source storage after
    // that call returns.
    const CKBehaviorContext *BehaviorContext;
    CKDWORD Flags;
} CKAngelScriptFunctionExecutionOptions;

typedef struct CKAngelScriptExecutionStepOptions {
    CKDWORD Size;
    // ConfigureContext, ReadResult, and UserData are borrowed only for one
    // CKAngelScriptStartExecution or CKAngelScriptResumeExecution call.
    // Suspend does not retain them; pass fresh step options when resuming.
    CKAngelScriptContextCallback ConfigureContext;
    CKAngelScriptContextCallback ReadResult;
    void *UserData;
} CKAngelScriptExecutionStepOptions;

typedef struct CKAngelScriptObjectOptions {
    CKDWORD Size;
    // Strings are borrowed only for the CreateObject call.
    const char *ModuleName;
    // Use ClassName plus optional ClassNamespace, or use TypeDecl. TypeDecl is
    // an AngelScript type declaration such as "MyNamespace::MyType". v3 object
    // creation is intentionally limited to these textual identities; metadata
    // entries and AngelScript type ids are discovery/runtime details, not public
    // object creation handles.
    const char *ClassName;
    const char *ClassNamespace;
    const char *TypeDecl;
} CKAngelScriptObjectOptions;

typedef struct CKAngelScriptMethodOptions {
    CKDWORD Size;
    // Method lookup is scoped to an existing CKAngelScriptObject. v3 does not
    // expose type-level method handles independent of an object instance.
    CKAngelScriptObject *Object;
    // Strings are borrowed only for the FindObjectMethod call. Method handles
    // copy the resolved symbol identity.
    const char *MethodName;
    const char *MethodDecl;
} CKAngelScriptMethodOptions;

typedef struct CKAngelScriptObjectMethodExecuteOptions {
    CKDWORD Size;
    CKAngelScriptObject *Object;
    CKAngelScriptMethod *Method;
    // WriteArgs, ReadResult, and UserData are borrowed only for the duration of
    // CKAngelScriptCallObjectMethod. CKAngelScript does not retain them after
    // the call returns.
    CKAngelScriptWriteArgsCallback WriteArgs;
    CKAngelScriptReadResultCallback ReadResult;
    // ConfigureContext and ReadContextResult are generic lower-level hooks for
    // hosts that need direct AngelScript marshalling. They run after the method
    // is prepared and after it finishes, respectively. They are borrowed only
    // for the duration of CKAngelScriptCallObjectMethod.
    CKAngelScriptContextCallback ConfigureContext;
    CKAngelScriptContextCallback ReadContextResult;
    void *UserData;
    CKDWORD Flags;
} CKAngelScriptObjectMethodExecuteOptions;

typedef struct CKAngelScriptCompilerMessage {
    CKDWORD Size;
    const char *Section;
    int Row;
    int Column;
    CKAS_MESSAGETYPE Type;
    const char *Message;
} CKAngelScriptCompilerMessage;

typedef struct CKAngelScriptResult {
    CKDWORD Size;
    CKAS_STATUS Status;
    int AngelScriptCode;
    // ErrorMessage and StackTrace are borrowed strings. Results returned
    // through API output parameters and GetLastResult() remain valid until the
    // next CKAngelScript API call that updates the last result. BorrowExecutionResult()
    // strings remain valid until the execution handle is released or started,
    // resumed, or cancelled again.
    const char *ErrorMessage;
    const char *StackTrace;
    // CompilerMessages points to structured AngelScript compiler diagnostics
    // captured during LoadModule/CompileModule. The array and its borrowed
    // strings follow the same lifetime as ErrorMessage.
    const CKAngelScriptCompilerMessage *CompilerMessages;
    size_t CompilerMessageCount;
} CKAngelScriptResult;

typedef struct CKAngelScriptEngineExtension {
    CKDWORD Size;
    const char *Name;
    // Register and UserData are retained until
    // CKAngelScriptUnregisterEngineExtension succeeds or the manager is
    // destroyed. If immediate registration fails, they are not retained.
    CKAngelScriptEngineExtensionCallback Register;
    void *UserData;
    // By default RegisterEngineExtension invokes the callback immediately when
    // the engine is already initialized. Set CKAS_ENGINEEXTENSION_DEFERRED
    // to skip immediate registration and retain the extension for rebuilds.
    // CKAngelScript wraps successful extension registration in an AngelScript
    // config group; callbacks must not call BeginConfigGroup/EndConfigGroup.
    // UnregisterEngineExtension removes that group when it is not in use.
    CKDWORD Flags;
} CKAngelScriptEngineExtension;

typedef void (*CKAngelScriptRawProc)(void);
typedef CKAngelScriptRawProc (*CKAngelScriptSymbolResolver)(void *userData, const char *name);

typedef CKAngelScript *(*CKAngelScriptGetProc)(CKContext *context);
typedef CKDWORD (*CKAngelScriptGetApiVersionProc)(void);
typedef CKBOOL (*CKAngelScriptHasFeatureProc)(CKAS_FEATURE feature);
typedef void (*CKAngelScriptInitResultProc)(CKAngelScriptResult *result);
typedef void (*CKAngelScriptInitEngineExtensionProc)(CKAngelScriptEngineExtension *extension);
typedef const char *(*CKAngelScriptGetStatusNameProc)(CKAS_STATUS status);
typedef const char *(*CKAngelScriptGetStatusDescriptionProc)(CKAS_STATUS status);
typedef CKAS_STATUS (*CKAngelScriptRegisterEngineExtensionProc)(
    CKAngelScript *angelScript,
    const CKAngelScriptEngineExtension *extension,
    CKAngelScriptResult *result);
typedef CKAS_STATUS (*CKAngelScriptUnregisterEngineExtensionProc)(
    CKAngelScript *angelScript,
    const char *name,
    CKAngelScriptResult *result);
typedef CKAS_STATUS (*CKAngelScriptAssignObjectHandleProc)(void **handleSlot,
                                                           void *object,
                                                           asITypeInfo *type);
typedef CKAS_STATUS (*CKAngelScriptCreateArrayProc)(
    CKAngelScript *angelScript,
    const char *arrayDecl,
    CKDWORD count,
    void **outArray);
typedef CKAS_STATUS (*CKAngelScriptCreateArrayByTypeProc)(
    asITypeInfo *arrayType,
    CKDWORD count,
    void **outArray);
typedef CKAS_STATUS (*CKAngelScriptArrayAddRefProc)(void *array);
typedef CKAS_STATUS (*CKAngelScriptArrayReleaseProc)(void *array);
typedef CKAS_STATUS (*CKAngelScriptArrayGetRefCountProc)(void *array, int *outRefCount);
typedef CKAS_STATUS (*CKAngelScriptArrayGetArrayTypeProc)(void *array, asITypeInfo **outType);
typedef CKAS_STATUS (*CKAngelScriptArrayGetArrayTypeIdProc)(void *array, int *outTypeId);
typedef CKAS_STATUS (*CKAngelScriptArrayGetSizeProc)(void *array, CKDWORD *outSize);
typedef CKAS_STATUS (*CKAngelScriptArrayResizeProc)(void *array, CKDWORD size);
typedef CKAS_STATUS (*CKAngelScriptArrayReserveProc)(void *array, CKDWORD capacity);
typedef CKAS_STATUS (*CKAngelScriptArrayGetElementTypeIdProc)(void *array, int *outTypeId);
typedef CKAS_STATUS (*CKAngelScriptArrayGetElementAddressProc)(void *array,
                                                               CKDWORD index,
                                                               void **outAddress);
typedef CKAS_STATUS (*CKAngelScriptArrayGetConstElementAddressProc)(const void *array,
                                                                    CKDWORD index,
                                                                    const void **outAddress);
typedef CKAS_STATUS (*CKAngelScriptArraySetElementValueProc)(void *array,
                                                             CKDWORD index,
                                                             const void *value);
typedef CKAS_STATUS (*CKAngelScriptArrayInsertAtProc)(void *array,
                                                      CKDWORD index,
                                                      const void *value);
typedef CKAS_STATUS (*CKAngelScriptArrayInsertLastProc)(void *array, const void *value);
typedef CKAS_STATUS (*CKAngelScriptArrayRemoveAtProc)(void *array, CKDWORD index);
typedef CKAS_STATUS (*CKAngelScriptArrayRemoveLastProc)(void *array);
typedef CKAS_STATUS (*CKAngelScriptArrayClearProc)(void *array);

typedef struct CKAngelScriptExtensionApi {
    CKDWORD Size;
    CKAngelScriptGetProc GetAngelScript;
    CKAngelScriptGetApiVersionProc GetApiVersion;
    CKAngelScriptHasFeatureProc HasFeature;
    CKAngelScriptInitResultProc InitResult;
    CKAngelScriptInitEngineExtensionProc InitEngineExtension;
    CKAngelScriptGetStatusNameProc GetStatusName;
    CKAngelScriptGetStatusDescriptionProc GetStatusDescription;
    CKAngelScriptRegisterEngineExtensionProc RegisterEngineExtension;
    CKAngelScriptUnregisterEngineExtensionProc UnregisterEngineExtension;
} CKAngelScriptExtensionApi;

static inline void CKAngelScriptInitExtensionApi(CKAngelScriptExtensionApi *api) {
    if (!api) {
        return;
    }
    api->Size = (CKDWORD)sizeof(*api);
    api->GetAngelScript = NULL;
    api->GetApiVersion = NULL;
    api->HasFeature = NULL;
    api->InitResult = NULL;
    api->InitEngineExtension = NULL;
    api->GetStatusName = NULL;
    api->GetStatusDescription = NULL;
    api->RegisterEngineExtension = NULL;
    api->UnregisterEngineExtension = NULL;
}

static inline CKBOOL CKAngelScriptExtensionApiIsLoaded(const CKAngelScriptExtensionApi *api) {
    return api &&
           api->Size >= (CKDWORD)sizeof(*api) &&
           api->GetAngelScript &&
           api->GetApiVersion &&
           api->HasFeature &&
           api->InitResult &&
           api->InitEngineExtension &&
           api->GetStatusName &&
           api->GetStatusDescription &&
           api->RegisterEngineExtension &&
           api->UnregisterEngineExtension
               ? TRUE
               : FALSE;
}

static inline CKBOOL CKAngelScriptLoadExtensionApi(CKAngelScriptExtensionApi *api,
                                                   CKAngelScriptSymbolResolver resolver,
                                                   void *userData) {
    CKAngelScriptInitExtensionApi(api);
    if (!api || !resolver) {
        return FALSE;
    }

    api->GetAngelScript =
        (CKAngelScriptGetProc)resolver(userData, "CKGetAngelScript");
    api->GetApiVersion =
        (CKAngelScriptGetApiVersionProc)resolver(userData, "CKAngelScriptGetApiVersion");
    api->HasFeature =
        (CKAngelScriptHasFeatureProc)resolver(userData, "CKAngelScriptHasFeature");
    api->InitResult =
        (CKAngelScriptInitResultProc)resolver(userData, "CKAngelScriptInitResult");
    api->InitEngineExtension =
        (CKAngelScriptInitEngineExtensionProc)resolver(userData, "CKAngelScriptInitEngineExtension");
    api->GetStatusName =
        (CKAngelScriptGetStatusNameProc)resolver(userData, "CKAngelScriptGetStatusName");
    api->GetStatusDescription =
        (CKAngelScriptGetStatusDescriptionProc)resolver(userData, "CKAngelScriptGetStatusDescription");
    api->RegisterEngineExtension =
        (CKAngelScriptRegisterEngineExtensionProc)resolver(userData, "CKAngelScriptRegisterEngineExtension");
    api->UnregisterEngineExtension =
        (CKAngelScriptUnregisterEngineExtensionProc)resolver(userData, "CKAngelScriptUnregisterEngineExtension");

    if (!CKAngelScriptExtensionApiIsLoaded(api) ||
        api->GetApiVersion() != CKAS_API_VERSION ||
        !api->HasFeature(CKAS_FEATURE_ENGINE_EXTENSION) ||
        !api->HasFeature(CKAS_FEATURE_PUBLIC_STRUCT_INITIALIZERS) ||
        !api->HasFeature(CKAS_FEATURE_STATUS_TEXT)) {
        CKAngelScriptInitExtensionApi(api);
        return FALSE;
    }
    return TRUE;
}

static inline CKAS_STATUS CKAngelScriptRegisterEngineExtensionWithApi(
    const CKAngelScriptExtensionApi *api,
    CKContext *context,
    const char *name,
    CKAngelScriptEngineExtensionCallback callback,
    void *userData,
    CKDWORD flags,
    CKAngelScriptResult *result) {
    if (result) {
        result->Size = (CKDWORD)sizeof(*result);
        result->Status = CKAS_OK;
        result->AngelScriptCode = 0;
        result->ErrorMessage = NULL;
        result->StackTrace = NULL;
    }
    if (!CKAngelScriptExtensionApiIsLoaded(api)) {
        if (result) {
            result->Status = CKAS_INVALIDARGUMENT;
            result->ErrorMessage = "CKAngelScript extension API table is not loaded.";
        }
        return CKAS_INVALIDARGUMENT;
    }

    api->InitResult(result);
    CKAngelScript *angelScript = api->GetAngelScript(context);
    if (!angelScript) {
        if (result) {
            result->Status = CKAS_NOTINITIALIZED;
            result->ErrorMessage = "CKAngelScript manager is not available.";
        }
        return CKAS_NOTINITIALIZED;
    }

    CKAngelScriptEngineExtension extension;
    api->InitEngineExtension(&extension);
    extension.Name = name;
    extension.Register = callback;
    extension.UserData = userData;
    extension.Flags = flags;
    return api->RegisterEngineExtension(angelScript, &extension, result);
}

static inline CKAS_STATUS CKAngelScriptUnregisterEngineExtensionWithApi(
    const CKAngelScriptExtensionApi *api,
    CKContext *context,
    const char *name,
    CKAngelScriptResult *result) {
    if (result) {
        result->Size = (CKDWORD)sizeof(*result);
        result->Status = CKAS_OK;
        result->AngelScriptCode = 0;
        result->ErrorMessage = NULL;
        result->StackTrace = NULL;
    }
    if (!CKAngelScriptExtensionApiIsLoaded(api)) {
        if (result) {
            result->Status = CKAS_INVALIDARGUMENT;
            result->ErrorMessage = "CKAngelScript extension API table is not loaded.";
        }
        return CKAS_INVALIDARGUMENT;
    }

    api->InitResult(result);
    CKAngelScript *angelScript = api->GetAngelScript(context);
    if (!angelScript) {
        if (result) {
            result->Status = CKAS_NOTINITIALIZED;
            result->ErrorMessage = "CKAngelScript manager is not available.";
        }
        return CKAS_NOTINITIALIZED;
    }
    return api->UnregisterEngineExtension(angelScript, name, result);
}

// C ABI. CKAngelScript is an opaque pointer owned by the plugin; never allocate
// or free it from host code. Borrowed AngelScript pointers are never AddRef'd by
// CKAngelScript and must not be released by the caller. CKAngelScript* handles
// returned from Find/Create calls must be released with the matching release API.
#ifdef __cplusplus
extern "C" {
#endif

CKAS_API CKAngelScript *CKGetAngelScript(CKContext *context);

CKAS_API CKDWORD CKAngelScriptGetApiVersion();
CKAS_API CKBOOL CKAngelScriptHasFeature(CKAS_FEATURE feature);
CKAS_API void CKAngelScriptInitResult(CKAngelScriptResult *result);
CKAS_API void CKAngelScriptInitLoadOptions(CKAngelScriptLoadOptions *options);
CKAS_API void CKAngelScriptInitFunctionOptions(CKAngelScriptFunctionOptions *options);
CKAS_API void CKAngelScriptInitFunctionExecutionOptions(CKAngelScriptFunctionExecutionOptions *options);
CKAS_API void CKAngelScriptInitExecutionStepOptions(CKAngelScriptExecutionStepOptions *options);
CKAS_API void CKAngelScriptInitObjectOptions(CKAngelScriptObjectOptions *options);
CKAS_API void CKAngelScriptInitMethodOptions(CKAngelScriptMethodOptions *options);
CKAS_API void CKAngelScriptInitObjectMethodExecuteOptions(CKAngelScriptObjectMethodExecuteOptions *options);
CKAS_API void CKAngelScriptInitEngineExtension(CKAngelScriptEngineExtension *extension);
CKAS_API const char *CKAngelScriptGetStatusName(CKAS_STATUS status);
CKAS_API const char *CKAngelScriptGetStatusDescription(CKAS_STATUS status);
CKAS_API const char *CKAngelScriptGetVersion(CKAngelScript *angelScript);
CKAS_API const char *CKAngelScriptGetOptions(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptBorrowEngine(CKAngelScript *angelScript,
                                               asIScriptEngine **outEngine,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowActiveContext(CKAngelScript *angelScript,
                                                      asIScriptContext **outContext,
                                                      CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptAssignObjectHandle(void **handleSlot,
                                                     void *object,
                                                     asITypeInfo *type);

// Generic script array object access for hosts that need to marshal values
// through CKAngelScript-owned array objects without depending on CScriptArray.
// Arrays created by CKAngelScriptCreateArray or CKAngelScriptCreateArrayByType
// are returned with one reference and must be released with
// CKAngelScriptArrayRelease. Arrays received from script are borrowed unless the
// caller explicitly retains them with CKAngelScriptArrayAddRef. Element
// addresses are borrowed from the array and are invalidated by resize/insert/
// remove/clear or release. SetElementValue, InsertAt, and InsertLast follow the
// AngelScript array add-on value convention: pass the address of the value to
// copy; for handle arrays pass the address of the handle variable.
CKAS_API CKAS_STATUS CKAngelScriptCreateArray(CKAngelScript *angelScript,
                                              const char *arrayDecl,
                                              CKDWORD count,
                                              void **outArray);
CKAS_API CKAS_STATUS CKAngelScriptCreateArrayByType(asITypeInfo *arrayType,
                                                    CKDWORD count,
                                                    void **outArray);
CKAS_API CKAS_STATUS CKAngelScriptArrayAddRef(void *array);
CKAS_API CKAS_STATUS CKAngelScriptArrayRelease(void *array);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetRefCount(void *array, int *outRefCount);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetArrayType(void *array, asITypeInfo **outType);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetArrayTypeId(void *array, int *outTypeId);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetSize(void *array, CKDWORD *outSize);
CKAS_API CKAS_STATUS CKAngelScriptArrayResize(void *array, CKDWORD size);
CKAS_API CKAS_STATUS CKAngelScriptArrayReserve(void *array, CKDWORD capacity);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetElementTypeId(void *array, int *outTypeId);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetElementAddress(void *array,
                                                        CKDWORD index,
                                                        void **outAddress);
CKAS_API CKAS_STATUS CKAngelScriptArrayGetConstElementAddress(const void *array,
                                                             CKDWORD index,
                                                             const void **outAddress);
CKAS_API CKAS_STATUS CKAngelScriptArraySetElementValue(void *array,
                                                      CKDWORD index,
                                                      const void *value);
CKAS_API CKAS_STATUS CKAngelScriptArrayInsertAt(void *array, CKDWORD index, const void *value);
CKAS_API CKAS_STATUS CKAngelScriptArrayInsertLast(void *array, const void *value);
CKAS_API CKAS_STATUS CKAngelScriptArrayRemoveAt(void *array, CKDWORD index);
CKAS_API CKAS_STATUS CKAngelScriptArrayRemoveLast(void *array);
CKAS_API CKAS_STATUS CKAngelScriptArrayClear(void *array);

CKAS_API CKAS_STATUS CKAngelScriptLoadModule(CKAngelScript *angelScript,
                                             const CKAngelScriptLoadOptions *options,
                                             CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptCompileModule(CKAngelScript *angelScript,
                                                const char *moduleName,
                                                const char *scriptCode,
                                                CKDWORD flags,
                                                CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnloadModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               CKAngelScriptResult *result);
CKAS_API CKBOOL CKAngelScriptHasModule(CKAngelScript *angelScript, const char *moduleName);
CKAS_API CKDWORD CKAngelScriptGetModuleGeneration(CKAngelScript *angelScript,
                                                  const char *moduleName);
CKAS_API CKAS_STATUS CKAngelScriptBorrowModule(CKAngelScript *angelScript,
                                               const char *moduleName,
                                               asIScriptModule **outModule,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByName(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionName,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowFunctionByDecl(CKAngelScript *angelScript,
                                                       const char *moduleName,
                                                       const char *functionDecl,
                                                       asIScriptFunction **outFunction,
                                                       CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptEnumerateMetadata(CKAngelScript *angelScript,
                                                    const char *moduleName,
                                                    CKAngelScriptMetadataCallback callback,
                                                    void *userData,
                                                    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptFindFunction(CKAngelScript *angelScript,
                                               const CKAngelScriptFunctionOptions *options,
                                               CKAngelScriptFunction **outFunction,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseFunction(CKAngelScript *angelScript,
                                                  CKAngelScriptFunction *function,
                                                  CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptCreateObject(CKAngelScript *angelScript,
                                               const CKAngelScriptObjectOptions *options,
                                               CKAngelScriptObject **outObject,
                                               CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseObject(CKAngelScript *angelScript,
                                                CKAngelScriptObject *object,
                                                CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptFindObjectMethod(CKAngelScript *angelScript,
                                                   const CKAngelScriptMethodOptions *options,
                                                   CKAngelScriptMethod **outMethod,
                                                   CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseMethod(CKAngelScript *angelScript,
                                                CKAngelScriptMethod *method,
                                                CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptArgSetBool(CKAngelScriptArgWriter *writer,
                                             CKDWORD index,
                                             CKBOOL value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetInt(CKAngelScriptArgWriter *writer,
                                            CKDWORD index,
                                            int value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetFloat(CKAngelScriptArgWriter *writer,
                                              CKDWORD index,
                                              float value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetString(CKAngelScriptArgWriter *writer,
                                               CKDWORD index,
                                               const char *value);
CKAS_API CKAS_STATUS CKAngelScriptArgSetBorrowedObject(CKAngelScriptArgWriter *writer,
                                                       CKDWORD index,
                                                       void *object);

CKAS_API CKAS_STATUS CKAngelScriptResultGetBool(CKAngelScriptResultReader *reader,
                                                CKBOOL *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetInt(CKAngelScriptResultReader *reader,
                                               int *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetFloat(CKAngelScriptResultReader *reader,
                                                 float *value);
CKAS_API CKAS_STATUS CKAngelScriptResultGetString(CKAngelScriptResultReader *reader,
                                                  char *buffer,
                                                  size_t bufferSize,
                                                  size_t *outRequiredSize);
CKAS_API CKAS_STATUS CKAngelScriptResultGetStringView(CKAngelScriptResultReader *reader,
                                                      const char **data,
                                                      size_t *size);

CKAS_API CKAS_STATUS CKAngelScriptCallObjectMethod(
    CKAngelScript *angelScript,
    const CKAngelScriptObjectMethodExecuteOptions *options,
    CKAngelScriptResult *result);

CKAS_API CKAS_STATUS CKAngelScriptCreateFunctionExecution(
    CKAngelScript *angelScript,
    const CKAngelScriptFunctionExecutionOptions *options,
    CKAngelScriptExecution **outExecution,
    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptStartExecution(CKAngelScript *angelScript,
                                                 CKAngelScriptExecution *execution,
                                                 const CKAngelScriptExecutionStepOptions *options,
                                                 CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptResumeExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution,
                                                  const CKAngelScriptExecutionStepOptions *options,
                                                  CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptCancelExecution(CKAngelScript *angelScript,
                                                  CKAngelScriptExecution *execution,
                                                  CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptReleaseExecution(CKAngelScript *angelScript,
                                                   CKAngelScriptExecution *execution,
                                                   CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptGetExecutionState(CKAngelScript *angelScript,
                                                    const CKAngelScriptExecution *execution,
                                                    CKAS_EXECUTIONSTATE *outState,
                                                    CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptBorrowExecutionResult(CKAngelScript *angelScript,
                                                        const CKAngelScriptExecution *execution,
                                                        const CKAngelScriptResult **outResult,
                                                        CKAngelScriptResult *result);
CKAS_API const CKAngelScriptResult *CKAngelScriptGetLastResult(CKAngelScript *angelScript);

CKAS_API CKAS_STATUS CKAngelScriptRegisterEngineExtension(CKAngelScript *angelScript,
                                                          const CKAngelScriptEngineExtension *extension,
                                                          CKAngelScriptResult *result);
CKAS_API CKAS_STATUS CKAngelScriptUnregisterEngineExtension(CKAngelScript *angelScript,
                                                            const char *name,
                                                            CKAngelScriptResult *result);

#ifdef __cplusplus
}

class CKAngelScriptObjectHandle {
public:
    CKAngelScriptObjectHandle() = default;
    CKAngelScriptObjectHandle(CKAngelScript *angelScript, CKAngelScriptObject *object)
        : m_AngelScript(angelScript), m_Object(object) {}
    CKAngelScriptObjectHandle(const CKAngelScriptObjectHandle &) = delete;
    CKAngelScriptObjectHandle &operator=(const CKAngelScriptObjectHandle &) = delete;
    CKAngelScriptObjectHandle(CKAngelScriptObjectHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptObjectHandle &operator=(CKAngelScriptObjectHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptObjectHandle() {
        Reset();
    }
    explicit operator bool() const {
        return m_Object != nullptr;
    }
    CKAngelScript *Owner() const {
        return m_AngelScript;
    }
    CKAngelScriptObject *Get() const {
        return m_Object;
    }
    CKAngelScriptObject *Release() {
        CKAngelScriptObject *object = m_Object;
        m_Object = nullptr;
        m_AngelScript = nullptr;
        return object;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptObject *object = nullptr) {
        if (m_Object) {
            (void)CKAngelScriptReleaseObject(m_AngelScript, m_Object, nullptr);
        }
        m_AngelScript = angelScript;
        m_Object = object;
    }
private:
    void MoveFrom(CKAngelScriptObjectHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Object = other.m_Object;
        other.m_AngelScript = nullptr;
        other.m_Object = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptObject *m_Object = nullptr;
};

class CKAngelScriptFunctionHandle {
public:
    CKAngelScriptFunctionHandle() = default;
    CKAngelScriptFunctionHandle(CKAngelScript *angelScript, CKAngelScriptFunction *function)
        : m_AngelScript(angelScript), m_Function(function) {}
    CKAngelScriptFunctionHandle(const CKAngelScriptFunctionHandle &) = delete;
    CKAngelScriptFunctionHandle &operator=(const CKAngelScriptFunctionHandle &) = delete;
    CKAngelScriptFunctionHandle(CKAngelScriptFunctionHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptFunctionHandle &operator=(CKAngelScriptFunctionHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptFunctionHandle() {
        Reset();
    }
    explicit operator bool() const {
        return m_Function != nullptr;
    }
    CKAngelScript *Owner() const {
        return m_AngelScript;
    }
    CKAngelScriptFunction *Get() const {
        return m_Function;
    }
    CKAngelScriptFunction *Release() {
        CKAngelScriptFunction *function = m_Function;
        m_Function = nullptr;
        m_AngelScript = nullptr;
        return function;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptFunction *function = nullptr) {
        if (m_Function) {
            (void)CKAngelScriptReleaseFunction(m_AngelScript, m_Function, nullptr);
        }
        m_AngelScript = angelScript;
        m_Function = function;
    }
private:
    void MoveFrom(CKAngelScriptFunctionHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Function = other.m_Function;
        other.m_AngelScript = nullptr;
        other.m_Function = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptFunction *m_Function = nullptr;
};

class CKAngelScriptMethodHandle {
public:
    CKAngelScriptMethodHandle() = default;
    CKAngelScriptMethodHandle(CKAngelScript *angelScript, CKAngelScriptMethod *method)
        : m_AngelScript(angelScript), m_Method(method) {}
    CKAngelScriptMethodHandle(const CKAngelScriptMethodHandle &) = delete;
    CKAngelScriptMethodHandle &operator=(const CKAngelScriptMethodHandle &) = delete;
    CKAngelScriptMethodHandle(CKAngelScriptMethodHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptMethodHandle &operator=(CKAngelScriptMethodHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptMethodHandle() {
        Reset();
    }
    explicit operator bool() const {
        return m_Method != nullptr;
    }
    CKAngelScript *Owner() const {
        return m_AngelScript;
    }
    CKAngelScriptMethod *Get() const {
        return m_Method;
    }
    CKAngelScriptMethod *Release() {
        CKAngelScriptMethod *method = m_Method;
        m_Method = nullptr;
        m_AngelScript = nullptr;
        return method;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptMethod *method = nullptr) {
        if (m_Method) {
            (void)CKAngelScriptReleaseMethod(m_AngelScript, m_Method, nullptr);
        }
        m_AngelScript = angelScript;
        m_Method = method;
    }
private:
    void MoveFrom(CKAngelScriptMethodHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Method = other.m_Method;
        other.m_AngelScript = nullptr;
        other.m_Method = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptMethod *m_Method = nullptr;
};

class CKAngelScriptExecutionHandle {
public:
    CKAngelScriptExecutionHandle() = default;
    CKAngelScriptExecutionHandle(CKAngelScript *angelScript, CKAngelScriptExecution *execution)
        : m_AngelScript(angelScript), m_Execution(execution) {}
    CKAngelScriptExecutionHandle(const CKAngelScriptExecutionHandle &) = delete;
    CKAngelScriptExecutionHandle &operator=(const CKAngelScriptExecutionHandle &) = delete;
    CKAngelScriptExecutionHandle(CKAngelScriptExecutionHandle &&other) noexcept {
        MoveFrom(other);
    }
    CKAngelScriptExecutionHandle &operator=(CKAngelScriptExecutionHandle &&other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(other);
        }
        return *this;
    }
    ~CKAngelScriptExecutionHandle() {
        Reset();
    }
    explicit operator bool() const {
        return m_Execution != nullptr;
    }
    CKAngelScript *Owner() const {
        return m_AngelScript;
    }
    CKAngelScriptExecution *Get() const {
        return m_Execution;
    }
    CKAngelScriptExecution *Release() {
        CKAngelScriptExecution *execution = m_Execution;
        m_Execution = nullptr;
        m_AngelScript = nullptr;
        return execution;
    }
    void Reset(CKAngelScript *angelScript = nullptr, CKAngelScriptExecution *execution = nullptr) {
        if (m_Execution) {
            (void)CKAngelScriptReleaseExecution(m_AngelScript, m_Execution, nullptr);
        }
        m_AngelScript = angelScript;
        m_Execution = execution;
    }
private:
    void MoveFrom(CKAngelScriptExecutionHandle &other) {
        m_AngelScript = other.m_AngelScript;
        m_Execution = other.m_Execution;
        other.m_AngelScript = nullptr;
        other.m_Execution = nullptr;
    }
    CKAngelScript *m_AngelScript = nullptr;
    CKAngelScriptExecution *m_Execution = nullptr;
};

// C++ convenience wrapper over the C ABI. This wrapper owns nothing and is safe
// to copy; it is invalid when Handle() returns nullptr.
class CKAngelScriptApi {
public:
    CKAngelScriptApi() = default;
    explicit CKAngelScriptApi(CKAngelScript *angelScript) : m_AngelScript(angelScript) {}

    static CKAngelScriptApi Get(CKContext *context) {
        return CKAngelScriptApi(CKGetAngelScript(context));
    }

    static CKAngelScriptResult Result() {
        CKAngelScriptResult result;
        CKAngelScriptInitResult(&result);
        return result;
    }

    static CKAngelScriptLoadOptions LoadOptions() {
        CKAngelScriptLoadOptions options;
        CKAngelScriptInitLoadOptions(&options);
        return options;
    }

    static CKAngelScriptLoadOptions LoadCodeOptions(const char *moduleName,
                                                    const char *code,
                                                    CKDWORD flags = CKAS_LOAD_DEFAULT) {
        CKAngelScriptLoadOptions options = LoadOptions();
        options.ModuleName = moduleName;
        options.Code = code;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptLoadOptions LoadFileOptions(const char *moduleName,
                                                    const char *filename,
                                                    CKDWORD flags = CKAS_LOAD_DEFAULT) {
        CKAngelScriptLoadOptions options = LoadOptions();
        options.ModuleName = moduleName;
        options.Filename = filename;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptLoadOptions LoadFilesOptions(const char *moduleName,
                                                     const char **filenames,
                                                     size_t fileCount,
                                                     CKDWORD flags = CKAS_LOAD_DEFAULT) {
        CKAngelScriptLoadOptions options = LoadOptions();
        options.ModuleName = moduleName;
        options.Filenames = filenames;
        options.FileCount = fileCount;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptFunctionOptions FunctionOptions() {
        CKAngelScriptFunctionOptions options;
        CKAngelScriptInitFunctionOptions(&options);
        return options;
    }

    static CKAngelScriptFunctionOptions FunctionByNameOptions(const char *moduleName,
                                                              const char *functionName) {
        CKAngelScriptFunctionOptions options = FunctionOptions();
        options.ModuleName = moduleName;
        options.FunctionName = functionName;
        return options;
    }

    static CKAngelScriptFunctionOptions FunctionByDeclOptions(const char *moduleName,
                                                              const char *functionDecl) {
        CKAngelScriptFunctionOptions options = FunctionOptions();
        options.ModuleName = moduleName;
        options.FunctionDecl = functionDecl;
        return options;
    }

    static CKAngelScriptFunctionExecutionOptions FunctionExecutionOptions() {
        CKAngelScriptFunctionExecutionOptions options;
        CKAngelScriptInitFunctionExecutionOptions(&options);
        return options;
    }

    static CKAngelScriptFunctionExecutionOptions FunctionExecutionOptions(
        CKAngelScriptFunction *function,
        CKDWORD flags = CKAS_CALL_DEFAULT) {
        CKAngelScriptFunctionExecutionOptions options = FunctionExecutionOptions();
        options.Function = function;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptExecutionStepOptions ExecutionStepOptions() {
        CKAngelScriptExecutionStepOptions options;
        CKAngelScriptInitExecutionStepOptions(&options);
        return options;
    }

    static CKAngelScriptExecutionStepOptions ExecutionStepOptions(
        CKAngelScriptContextCallback configure,
        CKAngelScriptContextCallback read = nullptr,
        void *userData = nullptr) {
        CKAngelScriptExecutionStepOptions options = ExecutionStepOptions();
        options.ConfigureContext = configure;
        options.ReadResult = read;
        options.UserData = userData;
        return options;
    }

    static CKAngelScriptObjectOptions ObjectOptions() {
        CKAngelScriptObjectOptions options;
        CKAngelScriptInitObjectOptions(&options);
        return options;
    }

    static CKAngelScriptObjectOptions ObjectOptions(const char *moduleName, const char *className) {
        CKAngelScriptObjectOptions options = ObjectOptions();
        options.ModuleName = moduleName;
        options.ClassName = className;
        return options;
    }

    static CKAngelScriptObjectOptions ObjectOptionsByNamespace(const char *moduleName,
                                                               const char *classNamespace,
                                                               const char *className) {
        CKAngelScriptObjectOptions options = ObjectOptions(moduleName, className);
        options.ClassNamespace = classNamespace;
        return options;
    }

    static CKAngelScriptObjectOptions ObjectOptionsByDecl(const char *moduleName, const char *typeDecl) {
        CKAngelScriptObjectOptions options = ObjectOptions();
        options.ModuleName = moduleName;
        options.TypeDecl = typeDecl;
        return options;
    }

    static CKAngelScriptMethodOptions MethodOptions() {
        CKAngelScriptMethodOptions options;
        CKAngelScriptInitMethodOptions(&options);
        return options;
    }

    static CKAngelScriptMethodOptions MethodByNameOptions(CKAngelScriptObject *object,
                                                          const char *methodName) {
        CKAngelScriptMethodOptions options = MethodOptions();
        options.Object = object;
        options.MethodName = methodName;
        return options;
    }

    static CKAngelScriptMethodOptions MethodByDeclOptions(CKAngelScriptObject *object,
                                                          const char *methodDecl) {
        CKAngelScriptMethodOptions options = MethodOptions();
        options.Object = object;
        options.MethodDecl = methodDecl;
        return options;
    }

    static CKAngelScriptObjectMethodExecuteOptions ObjectMethodExecuteOptions() {
        CKAngelScriptObjectMethodExecuteOptions options;
        CKAngelScriptInitObjectMethodExecuteOptions(&options);
        return options;
    }

    static CKAngelScriptObjectMethodExecuteOptions ObjectMethodExecuteOptions(
        CKAngelScriptObject *object,
        CKAngelScriptMethod *method,
        CKAngelScriptWriteArgsCallback write = nullptr,
        CKAngelScriptReadResultCallback read = nullptr,
        void *userData = nullptr,
        CKDWORD flags = CKAS_CALL_DEFAULT) {
        CKAngelScriptObjectMethodExecuteOptions options = ObjectMethodExecuteOptions();
        options.Object = object;
        options.Method = method;
        options.WriteArgs = write;
        options.ReadResult = read;
        options.UserData = userData;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptObjectMethodExecuteOptions ObjectMethodContextExecuteOptions(
        CKAngelScriptObject *object,
        CKAngelScriptMethod *method,
        CKAngelScriptContextCallback configure = nullptr,
        CKAngelScriptContextCallback read = nullptr,
        void *userData = nullptr,
        CKDWORD flags = CKAS_CALL_DEFAULT) {
        CKAngelScriptObjectMethodExecuteOptions options = ObjectMethodExecuteOptions();
        options.Object = object;
        options.Method = method;
        options.ConfigureContext = configure;
        options.ReadContextResult = read;
        options.UserData = userData;
        options.Flags = flags;
        return options;
    }

    static CKAngelScriptEngineExtension EngineExtension() {
        CKAngelScriptEngineExtension extension;
        CKAngelScriptInitEngineExtension(&extension);
        return extension;
    }

    static CKAngelScriptEngineExtension EngineExtension(
        const char *name,
        CKAngelScriptEngineExtensionCallback callback,
        void *userData = nullptr,
        CKDWORD flags = CKAS_ENGINEEXTENSION_DEFAULT) {
        CKAngelScriptEngineExtension extension = EngineExtension();
        extension.Name = name;
        extension.Register = callback;
        extension.UserData = userData;
        extension.Flags = flags;
        return extension;
    }

    static const char *StatusName(CKAS_STATUS status) {
        return CKAngelScriptGetStatusName(status);
    }

    static const char *StatusDescription(CKAS_STATUS status) {
        return CKAngelScriptGetStatusDescription(status);
    }

    CKAngelScript *Handle() const {
        return m_AngelScript;
    }

    CKBOOL IsValid() const {
        return m_AngelScript ? TRUE : FALSE;
    }

    CKAngelScriptApi *operator->() {
        return this;
    }

    const CKAngelScriptApi *operator->() const {
        return this;
    }

    CKDWORD GetApiVersion() const {
        (void)m_AngelScript;
        return CKAngelScriptGetApiVersion();
    }

    CKBOOL HasFeature(CKAS_FEATURE feature) const {
        (void)m_AngelScript;
        return CKAngelScriptHasFeature(feature);
    }

    const char *GetVersion() const {
        return CKAngelScriptGetVersion(m_AngelScript);
    }

    const char *GetOptions() const {
        return CKAngelScriptGetOptions(m_AngelScript);
    }

    CKAS_STATUS BorrowEngine(asIScriptEngine **outEngine, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowEngine(m_AngelScript, outEngine, result);
    }

    CKAS_STATUS BorrowActiveContext(asIScriptContext **outContext,
                                    CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowActiveContext(m_AngelScript, outContext, result);
    }

    static CKAS_STATUS AssignObjectHandle(void **handleSlot, void *object, asITypeInfo *type) {
        return CKAngelScriptAssignObjectHandle(handleSlot, object, type);
    }

    CKAS_STATUS CreateArray(const char *arrayDecl, CKDWORD count, void **outArray) const {
        return CKAngelScriptCreateArray(m_AngelScript, arrayDecl, count, outArray);
    }

    static CKAS_STATUS CreateArrayByType(asITypeInfo *arrayType, CKDWORD count, void **outArray) {
        return CKAngelScriptCreateArrayByType(arrayType, count, outArray);
    }

    static CKAS_STATUS ArrayAddRef(void *array) {
        return CKAngelScriptArrayAddRef(array);
    }

    static CKAS_STATUS ArrayRelease(void *array) {
        return CKAngelScriptArrayRelease(array);
    }

    static CKAS_STATUS ArrayGetRefCount(void *array, int *outRefCount) {
        return CKAngelScriptArrayGetRefCount(array, outRefCount);
    }

    static CKAS_STATUS ArrayGetArrayType(void *array, asITypeInfo **outType) {
        return CKAngelScriptArrayGetArrayType(array, outType);
    }

    static CKAS_STATUS ArrayGetArrayTypeId(void *array, int *outTypeId) {
        return CKAngelScriptArrayGetArrayTypeId(array, outTypeId);
    }

    static CKAS_STATUS ArrayGetSize(void *array, CKDWORD *outSize) {
        return CKAngelScriptArrayGetSize(array, outSize);
    }

    static CKAS_STATUS ArrayResize(void *array, CKDWORD size) {
        return CKAngelScriptArrayResize(array, size);
    }

    static CKAS_STATUS ArrayReserve(void *array, CKDWORD capacity) {
        return CKAngelScriptArrayReserve(array, capacity);
    }

    static CKAS_STATUS ArrayGetElementTypeId(void *array, int *outTypeId) {
        return CKAngelScriptArrayGetElementTypeId(array, outTypeId);
    }

    static CKAS_STATUS ArrayGetElementAddress(void *array, CKDWORD index, void **outAddress) {
        return CKAngelScriptArrayGetElementAddress(array, index, outAddress);
    }

    static CKAS_STATUS ArrayGetConstElementAddress(const void *array, CKDWORD index, const void **outAddress) {
        return CKAngelScriptArrayGetConstElementAddress(array, index, outAddress);
    }

    static CKAS_STATUS ArraySetElementValue(void *array, CKDWORD index, const void *value) {
        return CKAngelScriptArraySetElementValue(array, index, value);
    }

    static CKAS_STATUS ArrayInsertAt(void *array, CKDWORD index, const void *value) {
        return CKAngelScriptArrayInsertAt(array, index, value);
    }

    static CKAS_STATUS ArrayInsertLast(void *array, const void *value) {
        return CKAngelScriptArrayInsertLast(array, value);
    }

    static CKAS_STATUS ArrayRemoveAt(void *array, CKDWORD index) {
        return CKAngelScriptArrayRemoveAt(array, index);
    }

    static CKAS_STATUS ArrayRemoveLast(void *array) {
        return CKAngelScriptArrayRemoveLast(array);
    }

    static CKAS_STATUS ArrayClear(void *array) {
        return CKAngelScriptArrayClear(array);
    }

    CKAS_STATUS LoadModule(const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptLoadModule(m_AngelScript, &options, result);
    }

    CKAS_STATUS CompileModule(const char *moduleName,
                              const char *scriptCode,
                              CKDWORD flags = CKAS_COMPILE_DEFAULT,
                              CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCompileModule(m_AngelScript, moduleName, scriptCode, flags, result);
    }

    CKAS_STATUS UnloadModule(const char *moduleName, CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptUnloadModule(m_AngelScript, moduleName, result);
    }

    CKBOOL HasModule(const char *moduleName) const {
        return CKAngelScriptHasModule(m_AngelScript, moduleName);
    }

    CKDWORD GetModuleGeneration(const char *moduleName) const {
        return CKAngelScriptGetModuleGeneration(m_AngelScript, moduleName);
    }

    CKAS_STATUS BorrowModule(const char *moduleName,
                             asIScriptModule **outModule,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowModule(m_AngelScript, moduleName, outModule, result);
    }

    CKAS_STATUS BorrowFunctionByName(const char *moduleName,
                                     const char *functionName,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowFunctionByName(m_AngelScript, moduleName, functionName, outFunction, result);
    }

    CKAS_STATUS BorrowFunctionByDecl(const char *moduleName,
                                     const char *functionDecl,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowFunctionByDecl(m_AngelScript, moduleName, functionDecl, outFunction, result);
    }

    CKAS_STATUS EnumerateMetadata(const char *moduleName,
                                  CKAngelScriptMetadataCallback callback,
                                  void *userData = nullptr,
                                  CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptEnumerateMetadata(m_AngelScript, moduleName, callback, userData, result);
    }

    CKAS_STATUS FindFunction(const CKAngelScriptFunctionOptions &options,
                             CKAngelScriptFunction **outFunction,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptFindFunction(m_AngelScript, &options, outFunction, result);
    }

    CKAS_STATUS FindFunction(const CKAngelScriptFunctionOptions &options,
                             CKAngelScriptFunctionHandle &outFunction,
                             CKAngelScriptResult *result = nullptr) const {
        outFunction.Reset();
        CKAngelScriptFunction *function = nullptr;
        const CKAS_STATUS status = CKAngelScriptFindFunction(m_AngelScript, &options, &function, result);
        if (status == CKAS_OK && function) {
            outFunction.Reset(m_AngelScript, function);
        }
        return status;
    }

    CKAS_STATUS FindFunctionByName(const char *moduleName,
                                   const char *functionName,
                                   CKAngelScriptFunctionHandle &outFunction,
                                   CKAngelScriptResult *result = nullptr) const {
        return FindFunction(FunctionByNameOptions(moduleName, functionName), outFunction, result);
    }

    CKAS_STATUS FindFunctionByDecl(const char *moduleName,
                                   const char *functionDecl,
                                   CKAngelScriptFunctionHandle &outFunction,
                                   CKAngelScriptResult *result = nullptr) const {
        return FindFunction(FunctionByDeclOptions(moduleName, functionDecl), outFunction, result);
    }

    CKAS_STATUS ReleaseFunction(CKAngelScriptFunction *function,
                                CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptReleaseFunction(m_AngelScript, function, result);
    }

    CKAS_STATUS CreateObject(const CKAngelScriptObjectOptions &options,
                             CKAngelScriptObject **outObject,
                             CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCreateObject(m_AngelScript, &options, outObject, result);
    }

    CKAS_STATUS CreateObject(const CKAngelScriptObjectOptions &options,
                             CKAngelScriptObjectHandle &outObject,
                             CKAngelScriptResult *result = nullptr) const {
        outObject.Reset();
        CKAngelScriptObject *object = nullptr;
        const CKAS_STATUS status = CKAngelScriptCreateObject(m_AngelScript, &options, &object, result);
        if (status == CKAS_OK && object) {
            outObject.Reset(m_AngelScript, object);
        }
        return status;
    }

    CKAS_STATUS CreateObject(const char *moduleName,
                             const char *className,
                             CKAngelScriptObjectHandle &outObject,
                             CKAngelScriptResult *result = nullptr) const {
        return CreateObject(ObjectOptions(moduleName, className), outObject, result);
    }

    CKAS_STATUS CreateObject(const char *moduleName,
                             const char *classNamespace,
                             const char *className,
                             CKAngelScriptObjectHandle &outObject,
                             CKAngelScriptResult *result = nullptr) const {
        return CreateObject(ObjectOptionsByNamespace(moduleName, classNamespace, className), outObject, result);
    }

    CKAS_STATUS CreateObjectByDecl(const char *moduleName,
                                   const char *typeDecl,
                                   CKAngelScriptObjectHandle &outObject,
                                   CKAngelScriptResult *result = nullptr) const {
        return CreateObject(ObjectOptionsByDecl(moduleName, typeDecl), outObject, result);
    }

    CKAS_STATUS ReleaseObject(CKAngelScriptObject *object,
                              CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptReleaseObject(m_AngelScript, object, result);
    }

    CKAS_STATUS FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                 CKAngelScriptMethod **outMethod,
                                 CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptFindObjectMethod(m_AngelScript, &options, outMethod, result);
    }

    CKAS_STATUS FindObjectMethod(const CKAngelScriptMethodOptions &options,
                                 CKAngelScriptMethodHandle &outMethod,
                                 CKAngelScriptResult *result = nullptr) const {
        outMethod.Reset();
        CKAngelScriptMethod *method = nullptr;
        const CKAS_STATUS status = CKAngelScriptFindObjectMethod(m_AngelScript, &options, &method, result);
        if (status == CKAS_OK && method) {
            outMethod.Reset(m_AngelScript, method);
        }
        return status;
    }

    CKAS_STATUS FindObjectMethodByName(CKAngelScriptObject *object,
                                       const char *methodName,
                                       CKAngelScriptMethodHandle &outMethod,
                                       CKAngelScriptResult *result = nullptr) const {
        return FindObjectMethod(MethodByNameOptions(object, methodName), outMethod, result);
    }

    CKAS_STATUS FindObjectMethodByDecl(CKAngelScriptObject *object,
                                       const char *methodDecl,
                                       CKAngelScriptMethodHandle &outMethod,
                                       CKAngelScriptResult *result = nullptr) const {
        return FindObjectMethod(MethodByDeclOptions(object, methodDecl), outMethod, result);
    }

    CKAS_STATUS ReleaseMethod(CKAngelScriptMethod *method,
                              CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptReleaseMethod(m_AngelScript, method, result);
    }

    CKAS_STATUS CallObjectMethod(const CKAngelScriptObjectMethodExecuteOptions &options,
                                 CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCallObjectMethod(m_AngelScript, &options, result);
    }

    CKAS_STATUS CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                        CKAngelScriptExecution **outExecution,
                                        CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCreateFunctionExecution(m_AngelScript, &options, outExecution, result);
    }

    CKAS_STATUS CreateFunctionExecution(const CKAngelScriptFunctionExecutionOptions &options,
                                        CKAngelScriptExecutionHandle &outExecution,
                                        CKAngelScriptResult *result = nullptr) const {
        outExecution.Reset();
        CKAngelScriptExecution *execution = nullptr;
        const CKAS_STATUS status =
            CKAngelScriptCreateFunctionExecution(m_AngelScript, &options, &execution, result);
        if (status == CKAS_OK && execution) {
            outExecution.Reset(m_AngelScript, execution);
        }
        return status;
    }

    CKAS_STATUS StartExecution(CKAngelScriptExecution *execution,
                               const CKAngelScriptExecutionStepOptions *options = nullptr,
                               CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptStartExecution(m_AngelScript, execution, options, result);
    }

    CKAS_STATUS StartExecution(CKAngelScriptExecution *execution,
                               const CKAngelScriptExecutionStepOptions &options,
                               CKAngelScriptResult *result = nullptr) const {
        return StartExecution(execution, &options, result);
    }

    CKAS_STATUS ResumeExecution(CKAngelScriptExecution *execution,
                                const CKAngelScriptExecutionStepOptions *options = nullptr,
                                CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptResumeExecution(m_AngelScript, execution, options, result);
    }

    CKAS_STATUS ResumeExecution(CKAngelScriptExecution *execution,
                                const CKAngelScriptExecutionStepOptions &options,
                                CKAngelScriptResult *result = nullptr) const {
        return ResumeExecution(execution, &options, result);
    }

    CKAS_STATUS CancelExecution(CKAngelScriptExecution *execution,
                                CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptCancelExecution(m_AngelScript, execution, result);
    }

    CKAS_STATUS ReleaseExecution(CKAngelScriptExecution *execution,
                                 CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptReleaseExecution(m_AngelScript, execution, result);
    }

    CKAS_STATUS GetExecutionState(const CKAngelScriptExecution *execution,
                                  CKAS_EXECUTIONSTATE *outState,
                                  CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptGetExecutionState(m_AngelScript, execution, outState, result);
    }

    CKAS_STATUS BorrowExecutionResult(const CKAngelScriptExecution *execution,
                                      const CKAngelScriptResult **outResult,
                                      CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptBorrowExecutionResult(m_AngelScript, execution, outResult, result);
    }

    const CKAngelScriptResult *GetLastResult() const {
        return CKAngelScriptGetLastResult(m_AngelScript);
    }

    CKAS_STATUS RegisterEngineExtension(const CKAngelScriptEngineExtension &extension,
                                        CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptRegisterEngineExtension(m_AngelScript, &extension, result);
    }

    CKAS_STATUS UnregisterEngineExtension(const char *name,
                                          CKAngelScriptResult *result = nullptr) const {
        return CKAngelScriptUnregisterEngineExtension(m_AngelScript, name, result);
    }

private:
    CKAngelScript *m_AngelScript = nullptr;
};
#endif // __cplusplus

#endif // CK_ANGELSCRIPT_H
