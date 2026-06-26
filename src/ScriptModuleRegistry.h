#ifndef CK_SCRIPT_MODULE_REGISTRY_H
#define CK_SCRIPT_MODULE_REGISTRY_H

#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

#include "ScriptCache.h"
#include "ScriptModuleReplacer.h"

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

    CKAS_STATUS Load(ScriptManager &manager, const CKAngelScriptLoadOptions &options, CKAngelScriptResult *result);
    CKAS_STATUS Compile(ScriptManager &manager,
                        const char *moduleName,
                        const char *scriptCode,
                        CKDWORD flags,
                        CKAngelScriptResult *result);
    CKAS_STATUS Unload(ScriptManager &manager, const char *moduleName, CKAngelScriptResult *result);
    bool Has(ScriptManager &manager, const char *moduleName);
    CKDWORD GetGeneration(const ScriptManager &manager, const char *moduleName) const;

    CKAS_STATUS BorrowModule(ScriptManager &manager,
                             const char *moduleName,
                             asIScriptModule **outModule,
                             CKAngelScriptResult *result);
    CKAS_STATUS BorrowFunctionByName(ScriptManager &manager,
                                     const char *moduleName,
                                     const char *functionName,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result);
    CKAS_STATUS BorrowFunctionByDecl(ScriptManager &manager,
                                     const char *moduleName,
                                     const char *functionDecl,
                                     asIScriptFunction **outFunction,
                                     CKAngelScriptResult *result);
    CKAS_STATUS EnumerateMetadata(ScriptManager &manager,
                                  const char *moduleName,
                                  CKAngelScriptMetadataCallback callback,
                                  void *userData,
                                  CKAngelScriptResult *result);
    CKAS_STATUS EnumerateIncludeEdges(ScriptManager &manager,
                                      const char *moduleName,
                                      CKAngelScriptIncludeEdgeCallback callback,
                                      void *userData,
                                      CKAngelScriptResult *result);
    CKAS_STATUS GetFingerprint(ScriptManager &manager,
                               const char *moduleName,
                               CKAngelScriptModuleFingerprint *outFingerprint,
                               CKAngelScriptResult *result);
    CKAS_STATUS ReplaceFromBytecode(ScriptManager &manager,
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
    CKAS_STATUS ReplaceFromSections(ScriptManager &manager,
                                    const char *moduleName,
                                    const std::vector<std::tuple<std::string, std::string>> &sections,
                                    bool sourceSnapshotSections,
                                    CKAngelScriptResult *result);
    CKAS_STATUS CompleteSourceLoad(ScriptManager &manager,
                                   const char *moduleName,
                                   const std::vector<CapturedScriptMessage> &diagnosticMessages,
                                   CKAngelScriptResult *result);

    ScriptCache m_Cache;
    ScriptModuleReplacer m_Replacer;
};

#endif // CK_SCRIPT_MODULE_REGISTRY_H
