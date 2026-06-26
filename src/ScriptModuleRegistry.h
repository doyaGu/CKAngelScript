#ifndef CK_SCRIPT_MODULE_REGISTRY_H
#define CK_SCRIPT_MODULE_REGISTRY_H

#include <cstddef>
#include <memory>

#include "ScriptCache.h"

class ScriptManager;
class ScriptModuleStateStore;

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

    ScriptCache m_Cache;
};

#endif // CK_SCRIPT_MODULE_REGISTRY_H
