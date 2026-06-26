#ifndef CK_SCRIPT_MODULE_REGISTRY_H
#define CK_SCRIPT_MODULE_REGISTRY_H

#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

#include "ScriptCache.h"
#include "ScriptModuleReplacer.h"

class ScriptApiDiagnostics;
class ScriptHandleRegistry;
class ScriptImportBinder;
class ScriptManager;
class ScriptModuleStateStore;
struct CapturedScriptMessage;

class ScriptModuleRegistry {
public:
    void Clear();

    std::shared_ptr<CachedScript> GetCachedScript(const char *scriptName);
    std::shared_ptr<CachedScript> NewCachedScript(const char *scriptName);
    void CacheScript(const char *scriptName, std::shared_ptr<CachedScript> script);
    void Invalidate(const char *scriptName);

    struct MutationContext {
        ScriptManager &Manager;
        ScriptModuleStateStore &StateStore;
        ScriptHandleRegistry &HandleRegistry;
        ScriptImportBinder &ImportBinder;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
    };

    CKAS_STATUS Load(MutationContext &context, const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result);
    CKAS_STATUS Compile(MutationContext &context,
                        const char *moduleName,
                        const char *scriptCode,
                        CKDWORD flags,
                        CKAngelScriptResult *result);
    CKAS_STATUS Unload(MutationContext &context, const char *moduleName, CKAngelScriptResult *result);

    struct QueryContext {
        ScriptManager &Manager;
        ScriptModuleStateStore &StateStore;
        ScriptImportBinder &ImportBinder;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
    };

    bool Has(ScriptManager &manager, const char *moduleName);
    CKDWORD GetGeneration(const ScriptModuleStateStore &stateStore, const char *moduleName) const;

    struct BorrowContext {
        ScriptManager &Manager;
        ScriptApiDiagnostics &Diagnostics;
    };

    struct MetadataContext {
        ScriptManager &Manager;
        ScriptApiDiagnostics &Diagnostics;
        int &PublicCallbackDepth;
    };

    CKAS_STATUS BorrowModule(BorrowContext &context,
                             const char *moduleName,
                             asIScriptModule **outModule,
                             CKAngelScriptResult *result);
    CKAS_STATUS BorrowFunctionByName(BorrowContext &context,
                                     const char *moduleName,
                                     const char *functionName,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result);
    CKAS_STATUS BorrowFunctionByDecl(BorrowContext &context,
                                     const char *moduleName,
                                     const char *functionDecl,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result);
    CKAS_STATUS EnumerateMetadata(MetadataContext &context,
                                  const char *moduleName,
                                  CKAngelScriptMetadataCallback callback,
                                  void *userData,
                                  CKAngelScriptResult *result);
    CKAS_STATUS EnumerateIncludeEdges(QueryContext &context,
                                      const char *moduleName,
                                      CKAngelScriptIncludeEdgeCallback callback,
                                      void *userData,
                                      CKAngelScriptResult *result);
    CKAS_STATUS GetFingerprint(QueryContext &context,
                               const char *moduleName,
                               CKAngelScriptModuleFingerprint *outFingerprint,
                               CKAngelScriptResult *result);
    CKAS_STATUS ReplaceFromBytecode(MutationContext &context,
                                    const char *moduleName,
                                    const std::vector<unsigned char> &byteCode,
                                    CKAngelScriptResult *result);

    int LoadFromDefaultOrFile(ScriptManager &manager, const char *moduleName, const char *filename);
    int LoadFromFiles(ScriptManager &manager, const char *moduleName, const char **filenames, size_t count);
    int CompileFromMemory(ScriptManager &manager, const char *moduleName, const char *scriptCode);
    bool Discard(ScriptManager &manager, const char *moduleName);

    bool RestoreFromChunk(const char *scriptName, CKStateChunk *chunk);
    bool SaveToChunk(const char *scriptName, CKStateChunk *chunk);
    bool ClearCode(const char *scriptName);
    void ApplyCachedIncludeEdges(ScriptModuleStateStore &stateStore, const char *moduleName);
    unsigned long long BuildSourceHash(const char *moduleName);

private:
    bool DiscardCached(const char *moduleName);
    CKAS_STATUS ReplaceFromSections(MutationContext &context,
                                    const char *moduleName,
                                    const std::vector<std::tuple<std::string, std::string>> &sections,
                                    bool sourceSnapshotSections,
                                    CKAngelScriptResult *result);
    CKAS_STATUS CompleteSourceLoad(MutationContext &context,
                                   const char *moduleName,
                                   const std::vector<CapturedScriptMessage> &diagnosticMessages,
                                   CKAngelScriptResult *result);

    ScriptCache m_Cache;
    ScriptModuleReplacer m_Replacer;
};

#endif // CK_SCRIPT_MODULE_REGISTRY_H
