#ifndef CK_SCRIPT_PUBLIC_API_INTERNAL_H
#define CK_SCRIPT_PUBLIC_API_INTERNAL_H

#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include <angelscript.h>

#include "CKAngelScript.h"
#include "ScriptCache.h"
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

namespace ScriptManagerModuleReplacementInternal {

class PublicCallbackScope {
public:
    explicit PublicCallbackScope(int &depth)
        : m_Depth(depth) {
        ++m_Depth;
    }

    ~PublicCallbackScope() {
        --m_Depth;
    }

private:
    int &m_Depth;
};

bool SaveModuleByteCode(asIScriptModule *module,
                        std::vector<unsigned char> &byteCode,
                        int &angelScriptCode,
                        bool stripDebugInfo = false);
bool SaveModuleByteCode(asIScriptModule *module,
                        CKAngelScriptBytecodeWriteCallback callback,
                        void *userData,
                        bool stripDebugInfo,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus);
bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        std::vector<unsigned char> &byteCode,
                        asIScriptModule **outModule,
                        int &angelScriptCode);
bool LoadModuleByteCode(asIScriptEngine *engine,
                        const char *moduleName,
                        CKAngelScriptBytecodeReadCallback callback,
                        void *userData,
                        asIScriptModule **outModule,
                        int &angelScriptCode,
                        CKAS_STATUS &callbackStatus);
std::string MakeTransientModuleName(asIScriptEngine *engine, const char *moduleName);

} // namespace ScriptManagerModuleReplacementInternal

asITypeInfo *FindTypeByNameAndNamespace(asIScriptModule *module,
                                        const char *className,
                                        const char *classNamespace);

#endif // CK_SCRIPT_PUBLIC_API_INTERNAL_H
