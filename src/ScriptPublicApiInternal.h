#ifndef CK_SCRIPT_PUBLIC_API_INTERNAL_H
#define CK_SCRIPT_PUBLIC_API_INTERNAL_H

#include <cstring>

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

namespace ScriptManagerInternal {

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

} // namespace ScriptManagerInternal

asITypeInfo *FindTypeByNameAndNamespace(asIScriptModule *module,
                                        const char *className,
                                        const char *classNamespace);

#endif // CK_SCRIPT_PUBLIC_API_INTERNAL_H
