#ifndef SCRIPTCACHE_H
#define SCRIPTCACHE_H

#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <utility>

#include <angelscript.h>

#include "CKStateChunk.h"

class CScriptBuilder;

#define SCRIPTCACHE_IDENTIFIER 0x41535343 // 'ASSC'
#define SCRIPTCACHE_VERSION1 1

class ICachedScript {
public:
    virtual ~ICachedScript() = default;

    //--------------------------------------------------------------------------
    //  BASIC SCRIPT ACCESS
    //--------------------------------------------------------------------------
    virtual asIScriptModule *GetScriptModule() const = 0;

    virtual bool Build(asIScriptEngine *engine) = 0;

    virtual const char *GetName() const = 0;

    virtual int GetSectionCount() const = 0;
    virtual const char *GetSectionFilename(int index) const = 0;
    virtual const char *GetSectionCode(int index) const = 0;

    virtual bool LoadFromChunk(CKStateChunk *chunk) = 0;
    virtual bool SaveToChunk(CKStateChunk *chunk) = 0;

    //--------------------------------------------------------------------------
    //  METADATA ACCESS
    //--------------------------------------------------------------------------
    // Global type metadata
    virtual int GetTypeMetadataCount(int typeId) const = 0;
    virtual const char *GetTypeMetadata(int typeId, int metaIndex) const = 0;

    // Global function metadata
    virtual int GetFuncMetadataCount(asIScriptFunction *func) const = 0;
    virtual const char *GetFuncMetadata(asIScriptFunction *func, int metaIndex) const = 0;

    // Global variable metadata
    // (varIdx is the index within the module: module->GetGlobalVarCount())
    virtual int GetVarMetadataCount(int varIdx) const = 0;
    virtual const char *GetVarMetadata(int varIdx, int metaIndex) const = 0;

    //--------------------------------------------------------------------------
    //  CLASS (object type) METADATA
    //--------------------------------------------------------------------------
    // Return the class's name (if you want it in a separate method).
    // Or skip this if you just want metadata. Shown for completeness:
    virtual const char *GetNameOfClass(int typeId) const = 0;

    // Class method metadata
    virtual int GetClassMethodMetadataCount(int typeId, asIScriptFunction *method) const = 0;
    virtual const char *GetClassMethodMetadata(int typeId, asIScriptFunction *method, int metaIndex) const = 0;

    // Class variable (property) metadata
    virtual int GetClassVarMetadataCount(int typeId, int varIdx) const = 0;
    virtual const char *GetClassVarMetadata(int typeId, int varIdx, int metaIndex) const = 0;
};

struct ScriptMetadata {
    // Metadata for global declarations
    std::map<int, std::vector<std::string>> typeMetadataMap;
    std::map<int, std::vector<std::string>> funcMetadataMap;
    std::map<int, std::vector<std::string>> varMetadataMap;

    // Metadata for class member declarations
    struct ClassMetadata {
        std::string className;
        std::map<int, std::vector<std::string>> funcMetadataMap;
        std::map<int, std::vector<std::string>> varMetadataMap;
    };

    std::map<int, ClassMetadata> classMetadataMap;

    // Get metadata declared for classes, interfaces, and enums
    std::vector<std::string> &GetMetadataForType(int typeId);

    // Get metadata declared for functions
    std::vector<std::string> &GetMetadataForFunc(asIScriptFunction *func);

    // Get metadata declared for global variables
    std::vector<std::string> &GetMetadataForVar(int varIdx);

    // Get metadata declared for class variables
    std::vector<std::string> &GetMetadataForTypeProperty(int typeId, int varIdx);

    // Get metadata declared for class methods
    std::vector<std::string> &GetMetadataForTypeMethod(int typeId, asIScriptFunction *method);

    // Optional: clear all stored metadata
    void Clear() {
        typeMetadataMap.clear();
        funcMetadataMap.clear();
        varMetadataMap.clear();
        classMetadataMap.clear();
    }

    static void Extract(CScriptBuilder &builder, ScriptMetadata &outMetadata);
};

struct CachedScript : ICachedScript {
    asIScriptModule *module = nullptr;
    std::string name;
    std::vector<std::tuple<std::string, std::string>> sections;
    ScriptMetadata metadata;

    ~CachedScript() override;

    asIScriptModule *GetScriptModule() const override;

    bool Build(asIScriptEngine *engine) override;

    const char *GetName() const override;

    int GetSectionCount() const override;
    const char *GetSectionFilename(int index) const override;
    const char *GetSectionCode(int index) const override;

    bool AddSection(const std::string &name, const std::string &code = "");

    bool LoadFromChunk(CKStateChunk *chunk) override;
    bool SaveToChunk(CKStateChunk *chunk) override;

    int GetTypeMetadataCount(int typeId) const override;
    const char *GetTypeMetadata(int typeId, int metaIndex) const override;

    int GetFuncMetadataCount(asIScriptFunction *func) const override;
    const char *GetFuncMetadata(asIScriptFunction *func, int metaIndex) const override;

    int GetVarMetadataCount(int varIdx) const override;
    const char *GetVarMetadata(int varIdx, int metaIndex) const override;

    const char *GetNameOfClass(int typeId) const override;

    int GetClassMethodMetadataCount(int typeId, asIScriptFunction *method) const override;
    const char *GetClassMethodMetadata(int typeId, asIScriptFunction *method, int metaIndex) const override;

    int GetClassVarMetadataCount(int typeId, int varIdx) const override;
    const char *GetClassVarMetadata(int typeId, int varIdx, int metaIndex) const override;
};

class ScriptCache {
public:
    ScriptCache();
    ~ScriptCache();

    std::shared_ptr<CachedScript> NewCachedScript(const std::string &scriptName);

    std::shared_ptr<CachedScript> GetCachedScript(const std::string &scriptName);

    void CacheScript(const std::string &scriptName, std::shared_ptr<CachedScript> script);

    void Invalidate(const std::string &scriptName);

    std::shared_ptr<CachedScript> LoadScript(asIScriptEngine *engine,
                                             const std::string &scriptName, const std::string &filename);

    std::shared_ptr<CachedScript> LoadScript(asIScriptEngine *engine,
                                             const std::string &scriptName, const std::vector<std::string> &filenames);

    std::shared_ptr<CachedScript> CompileScript(asIScriptEngine *engine,
                                                const std::string &scriptName, const std::string &scriptCode);

private:
    std::unordered_map<std::string, std::shared_ptr<CachedScript>> m_CachedScripts;
    std::mutex m_Mutex;
};

#endif //SCRIPTCACHE_H
