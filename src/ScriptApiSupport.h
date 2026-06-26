#ifndef CK_SCRIPT_API_SUPPORT_H
#define CK_SCRIPT_API_SUPPORT_H

#include <cstring>
#include <functional>
#include <string>

#include <angelscript.h>

#include "CKAngelScript.h"
#include "ScriptApiHandles.h"

class ScriptManager;

inline ScriptManager *FromPublicHandle(CKAngelScript *angelScript) {
    return reinterpret_cast<ScriptManager *>(angelScript);
}

inline const ScriptManager *FromPublicHandle(const CKAngelScript *angelScript) {
    return reinterpret_cast<const ScriptManager *>(angelScript);
}

inline CKAngelScript *ToPublicHandle(ScriptManager *manager) {
    return reinterpret_cast<CKAngelScript *>(manager);
}

namespace ScriptApiSupport {

inline bool IsNonEmpty(const char *value) {
    return value && value[0] != '\0';
}

class CallbackDepthScope {
public:
    explicit CallbackDepthScope(int &depth)
        : m_Depth(depth) {
        ++m_Depth;
    }

    CallbackDepthScope(const CallbackDepthScope &) = delete;
    CallbackDepthScope &operator=(const CallbackDepthScope &) = delete;

    ~CallbackDepthScope() {
        --m_Depth;
    }

private:
    int &m_Depth;
};

struct ObjectCallOutcome {
    CKAS_STATUS Status = CKAS_OK;
    int AngelScriptCode = 0;
    std::string ErrorMessage;
    std::string StackTrace;
};

const char *StatusName(CKAS_STATUS status);
const char *StatusMessage(CKAS_STATUS status);
CKAS_STATUS StoreStatelessPublicResult(CKAngelScriptResult *out,
                                       CKAS_STATUS status,
                                       int angelScriptCode,
                                       const char *errorMessage);

bool ValidateArgIndex(const CKAngelScriptArgWriter *writer, CKDWORD index);
bool IsStringType(asIScriptEngine *engine, int typeId);
bool IsIntOrEnumType(asIScriptEngine *engine, int typeId);
bool IsValidStringParam(asIScriptEngine *engine, int typeId, asDWORD flags);
bool IsValidBorrowedObjectParam(int typeId, asDWORD flags);
bool IsValidObjectHandleParam(int typeId, asDWORD flags);
bool IsCompatibleObjectHandle(asIScriptEngine *engine,
                              int expectedTypeId,
                              const CKAngelScriptObject *objectHandle);
bool HasPublicFlag(CKDWORD flags, CKDWORD flag);

CKAS_STATUS DispatchMetadata(const CKAngelScriptMetadataEntry &entry,
                             CKDWORD metadataCount,
                             const std::function<const char *(CKDWORD)> &metadataAt,
                             CKAngelScriptMetadataCallback callback,
                             void *userData);
CKAS_STATUS DispatchImport(const CKAngelScriptImportEntry &entry,
                           CKAngelScriptImportCallback callback,
                           void *userData);
CKAS_STATUS DispatchBoundImportEdge(const CKAngelScriptBoundImportEdge &edge,
                                    CKAngelScriptBoundImportEdgeCallback callback,
                                    void *userData);
CKAS_STATUS DispatchIncludeEdge(const CKAngelScriptIncludeEdge &edge,
                                CKAngelScriptIncludeEdgeCallback callback,
                                void *userData);

extern const unsigned long long kFnvOffsetBasis;
void HashString(unsigned long long &hash, const std::string &value);
void HashString(unsigned long long &hash, const char *value);
void HashValue(unsigned long long &hash, unsigned long long value);
void HashValue(unsigned long long &hash, CKDWORD value);
void HashBool(unsigned long long &hash, bool value);
CKAS_STATUS StatusFromImportBindResult(int code);
CKAngelScriptResult MakeExecutionResult(CKAngelScriptExecution *execution,
                                        CKAS_STATUS status,
                                        int angelScriptCode = 0,
                                        const std::string &errorMessage = std::string(),
                                        const std::string &stackTrace = std::string());
ObjectCallOutcome ExecutePreparedObjectMethod(ScriptManager *manager,
                                              int &publicCallbackDepth,
                                              asIScriptObject *object,
                                              asIScriptFunction *function,
                                              const CKAngelScriptMethod *method,
                                              CKAngelScriptWriteArgsCallback writeArgs,
                                              CKAngelScriptReadResultCallback readResult,
                                              CKAngelScriptContextCallback configureContext,
                                              CKAngelScriptContextCallback readContextResult,
                                              void *userData,
                                              CKDWORD flags);
CKAS_STATUS RunExecution(CKAngelScriptExecution *execution,
                         const CKAngelScriptExecutionStepOptions *options,
                         int &publicCallbackDepth);
asITypeInfo *FindTypeByNameAndNamespace(asIScriptModule *module,
                                        const char *className,
                                        const char *classNamespace);

template <typename T>
void InitPublicStruct(T *value) {
    if (!value) {
        return;
    }
    std::memset(value, 0, sizeof(T));
    value->Size = static_cast<CKDWORD>(sizeof(T));
}

template <typename T, typename M>
bool HasPublicField(const T &value, M T::*field) {
    const CKDWORD size = value.Size;
    const char *base = reinterpret_cast<const char *>(&value);
    const char *member = reinterpret_cast<const char *>(&(value.*field));
    const size_t offset = static_cast<size_t>(member - base);
    return size >= offset + sizeof(M);
}

template <typename T>
bool HasCompletePublicStruct(const T &value) {
    return value.Size >= static_cast<CKDWORD>(sizeof(T));
}

template <typename T, typename M>
M PublicField(const T &value, M T::*field, M fallback = M()) {
    return HasPublicField(value, field) ? value.*field : fallback;
}

} // namespace ScriptApiSupport

#endif // CK_SCRIPT_API_SUPPORT_H
