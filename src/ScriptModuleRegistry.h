#ifndef CK_SCRIPT_MODULE_REGISTRY_H
#define CK_SCRIPT_MODULE_REGISTRY_H

#include <cstddef>
#include <memory>

#include "ScriptCache.h"

class ScriptManager;

class ScriptModuleRegistry {
public:
    void Clear();

    std::shared_ptr<CachedScript> GetCachedScript(const char *scriptName);
    std::shared_ptr<CachedScript> NewCachedScript(const char *scriptName);
    void CacheScript(const char *scriptName, std::shared_ptr<CachedScript> script);
    void Invalidate(const char *scriptName);

    int LoadFromDefaultOrFile(ScriptManager &manager, const char *moduleName, const char *filename);
    int LoadFromFiles(ScriptManager &manager, const char *moduleName, const char **filenames, size_t count);
    int CompileFromMemory(ScriptManager &manager, const char *moduleName, const char *scriptCode);
    bool DiscardCached(const char *moduleName);

    bool RestoreFromChunk(const char *scriptName, CKStateChunk *chunk);
    bool SaveToChunk(const char *scriptName, CKStateChunk *chunk);
    bool ClearCode(const char *scriptName);
    unsigned long long BuildSourceHash(const char *moduleName);

private:
    ScriptCache m_Cache;
};

#endif // CK_SCRIPT_MODULE_REGISTRY_H
