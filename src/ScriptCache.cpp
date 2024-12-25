#include "ScriptCache.h"

#include <utility>

#include "ScriptManager.h"
#include "add_on/scriptbuilder/scriptbuilder.h"

static std::vector<std::string> g_EmptyMetadata;

static const std::vector<std::string> &GetVectorMetadata(const std::map<int, std::vector<std::string> > &m, int key) {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return g_EmptyMetadata;
}

static int PragmaCallback(const std::string &pragmaText, CScriptBuilder &builder, void * /*userParam*/) {
    asIScriptEngine *engine = builder.GetEngine();

    // Filter the pragmaText so only what is of interest remains
    // With this the user can add comments and use different whitespaces without affecting the result
    asUINT pos = 0;
    asUINT length = 0;
    std::string cleanText;
    while (pos < pragmaText.size()) {
        asETokenClass tokenClass = engine->ParseToken(pragmaText.c_str() + pos, 0, &length);
        if (tokenClass == asTC_IDENTIFIER || tokenClass == asTC_KEYWORD || tokenClass == asTC_VALUE) {
            std::string token = pragmaText.substr(pos, length);
            cleanText += " " + token;
        }
        if (tokenClass == asTC_UNKNOWN)
            return -1;
        pos += length;
    }

    // The #pragma directive was not accepted
    return -1;
}

std::vector<std::string> &ScriptMetadata::GetMetadataForType(int typeId) {
    auto it = typeMetadataMap.find(typeId);
    if (it != typeMetadataMap.end())
        return it->second;

    return g_EmptyMetadata;
}

std::vector<std::string> &ScriptMetadata::GetMetadataForFunc(asIScriptFunction *func) {
    if (func) {
        auto it = funcMetadataMap.find(func->GetId());
        if (it != funcMetadataMap.end())
            return it->second;
    }

    return g_EmptyMetadata;
}

std::vector<std::string> &ScriptMetadata::GetMetadataForVar(int varIdx) {
    auto it = varMetadataMap.find(varIdx);
    if (it != varMetadataMap.end())
        return it->second;

    return g_EmptyMetadata;
}

std::vector<std::string> &ScriptMetadata::GetMetadataForTypeProperty(int typeId, int varIdx) {
    auto typeIt = classMetadataMap.find(typeId);
    if (typeIt == classMetadataMap.end()) return g_EmptyMetadata;

    auto propIt = typeIt->second.varMetadataMap.find(varIdx);
    if (propIt == typeIt->second.varMetadataMap.end()) return g_EmptyMetadata;

    return propIt->second;
}

std::vector<std::string> &ScriptMetadata::GetMetadataForTypeMethod(int typeId, asIScriptFunction *method) {
    if (method) {
        auto typeIt = classMetadataMap.find(typeId);
        if (typeIt == classMetadataMap.end()) return g_EmptyMetadata;

        auto methodIt = typeIt->second.funcMetadataMap.find(method->GetId());
        if (methodIt == typeIt->second.funcMetadataMap.end()) return g_EmptyMetadata;

        return methodIt->second;
    }

    return g_EmptyMetadata;
}

void ScriptMetadata::Extract(CScriptBuilder &builder, ScriptMetadata &outMetadata) {
#if AS_PROCESS_METADATA == 1
    // For each global type
    {
        // We can query the engine for the number of types, or we can query
        // the builder’s maps if it exposes them publicly. We'll do something
        // simplistic: we’ll look at the builder's typeMetadataMap, etc.

        // The builder in the snippet has internal maps.
        // We can get them by calling the builder’s "GetMetadataForType(typeId)"
        // for each known type ID in the module, etc.

        asIScriptModule *module = builder.GetModule();
        if (!module) return;

        // Extract global variables
        int varCount = module->GetGlobalVarCount();
        for (int i = 0; i < varCount; ++i) {
            std::vector<std::string> meta = builder.GetMetadataForVar(i);
            if (!meta.empty()) {
                outMetadata.varMetadataMap[i] = meta;
            }
        }

        // Extract global functions
        int funcCount = module->GetFunctionCount();
        for (int f = 0; f < funcCount; ++f) {
            asIScriptFunction *func = module->GetFunctionByIndex(f);
            if (!func) continue;

            std::vector<std::string> meta = builder.GetMetadataForFunc(func);
            if (!meta.empty()) {
                // store by function Id
                outMetadata.funcMetadataMap[func->GetId()] = meta;
            }
        }

        // Extract object types
        int typeCount = module->GetObjectTypeCount();
        for (int t = 0; t < typeCount; ++t) {
            asITypeInfo *typeInfo = module->GetObjectTypeByIndex(t);
            if (!typeInfo) continue;

            int typeId = typeInfo->GetTypeId();
            std::vector<std::string> meta = builder.GetMetadataForType(typeId);
            if (!meta.empty()) {
                outMetadata.typeMetadataMap[typeId] = meta;
            }

            // Now for each property in the type
            int propCount = typeInfo->GetPropertyCount();
            for (int p = 0; p < propCount; ++p) {
                std::vector<std::string> propMeta = builder.GetMetadataForTypeProperty(typeId, p);
                if (!propMeta.empty()) {
                    // If we haven't created a class metadata yet, do so
                    auto itClass = outMetadata.classMetadataMap.find(typeId);
                    if (itClass == outMetadata.classMetadataMap.end()) {
                        // Use the type name
                        ScriptMetadata::ClassMetadata cm(typeInfo->GetName());
                        outMetadata.classMetadataMap[typeId] = cm;
                    }
                    outMetadata.classMetadataMap[typeId].varMetadataMap[p] = propMeta;
                }
            }

            // And each method
            int methCount = typeInfo->GetMethodCount();
            for (int m = 0; m < methCount; ++m) {
                asIScriptFunction *method = typeInfo->GetMethodByIndex(m);
                if (!method) continue;

                std::vector<std::string> methodMeta = builder.GetMetadataForTypeMethod(typeId, method);
                if (!methodMeta.empty()) {
                    auto itClass = outMetadata.classMetadataMap.find(typeId);
                    if (itClass == outMetadata.classMetadataMap.end()) {
                        ScriptMetadata::ClassMetadata cm(typeInfo->GetName());
                        outMetadata.classMetadataMap[typeId] = cm;
                    }
                    outMetadata.classMetadataMap[typeId].funcMetadataMap[method->GetId()] = methodMeta;
                }
            }
        }
    }
#else
    (void)builder;
    (void)outMetadata;
    // If metadata processing is disabled, do nothing.
#endif
}

CachedScript::~CachedScript() {
    if (module) {
        module->Discard();
        module = nullptr;
    }
}

asIScriptModule *CachedScript::GetScriptModule() const { return module; }

bool CachedScript::Build(asIScriptEngine *engine) {
    if (module)
        return true;

    if (!engine) {
        return false;
    }

    auto *man = static_cast<ScriptManager *>(engine->GetUserData());

    // We will only initialize the global variables once we're
    // ready to execute, so disable the automatic initialization
    // m_ScriptEngine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

    // Prepare the script builder
    CScriptBuilder builder;
    // builder.SetIncludeCallback(MyIncludeCallback, nullptr);
    builder.SetPragmaCallback(PragmaCallback, nullptr);

#if CKVERSION == 0x13022002
    builder.DefineWord("VT21");
#elif CKVERSION == 0x05082002
    builder.DefineWord("VT25");
#endif

    // Start a new module
    int r = builder.StartNewModule(engine, name.c_str());
    if (r < 0) {
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to create module");
        return false;
    }

    // Add the script file
    for (auto &section: sections) {
        std::string &filename = std::get<0>(section);
        std::string &code = std::get<1>(section);

        XString resolvedFilename = filename.c_str();
        man->ResolveScriptFileName(resolvedFilename);

        bool exists = false;
        FILE *fp = fopen(resolvedFilename.CStr(), "rb");
        if (fp) {
            exists = true;
            fclose(fp);
        }

        if (!exists && code.size() != 0) {
            r = builder.AddSectionFromMemory(filename.c_str(), code.c_str(), code.size(), 0);
            if (r < 0) {
                engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR,
                                     "[CachedScript] Failed to load section from memory");
                return false;
            }
        } else {
            r = builder.AddSectionFromFile(resolvedFilename.CStr());
            if (r < 0) {
                engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR,
                                     "[CachedScript] Failed to load section from file");
                return false;
            }
        }
    }

    // Build the module
    r = builder.BuildModule();
    if (r < 0) {
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to build module");
        return false;
    }

    // Retrieve the compiled module
    module = builder.GetModule();
    if (!module) {
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to retrieve module");
        return false;
    }

    // Extract metadata from the builder
#if AS_PROCESS_METADATA == 1
    ScriptMetadata::Extract(builder, metadata);
#endif

    return true;
}

const char *CachedScript::GetName() const {
    return name.c_str();
}

int CachedScript::GetSectionCount() const {
    return static_cast<int>(sections.size());
}

const char *CachedScript::GetSectionFilename(int index) const {
    if (index < 0 || index >= (int) sections.size())
        return nullptr;
    return std::get<0>(sections[index]).c_str();
}

const char *CachedScript::GetSectionCode(int index) const {
    if (index < 0 || index >= (int) sections.size())
        return nullptr;
    return std::get<1>(sections[index]).c_str();
}

bool CachedScript::AddSection(const std::string &name, const std::string &code) {
    auto it = std::find_if(sections.begin(), sections.end(),
                           [&name](const std::tuple<std::string, std::string> &section) {
                               return std::get<0>(section) == name;
                           });
    if (it != sections.end()) {
        if (!code.empty()) {
            std::get<1>(*it) = code;
            return true;
        } else {
            return false;
        }
    }

    sections.emplace_back(name, code);
    return true;
}

bool CachedScript::LoadFromChunk(CKStateChunk *chunk) {
    chunk->StartRead();

    if (!chunk->SeekIdentifier(SCRIPTCACHE_IDENTIFIER)) {
        chunk->CloseChunk();
        return false;
    }

    int version = chunk->ReadInt();
    if (version != SCRIPTCACHE_VERSION1) {
        chunk->CloseChunk();
        return false;
    }

    // Read the script name
    char *str;
    chunk->ReadString(&str);
    if (!str) {
        chunk->CloseChunk();
        return false;
    }
    name = str;
    CKDeletePointer(str);
    str = nullptr;

    const int numSections = (int) chunk->ReadDword();
    for (int i = 0; i < numSections; ++i) {
        chunk->ReadString(&str);
        if (!str) {
            str = nullptr;
            continue;
        }
        std::string filename = str;

        std::string buffer;
        int size = chunk->ReadInt();
        if (size != 0) {
            buffer.resize(size);
            chunk->ReadAndFillBuffer(size, buffer.data());
        }

        sections.emplace_back(std::move(filename), std::move(buffer));
    }

    return true;
}

bool CachedScript::SaveToChunk(CKStateChunk *chunk) {
    chunk->StartWrite();
    // start identifier
    chunk->WriteIdentifier(SCRIPTCACHE_IDENTIFIER);
    // version
    chunk->WriteInt(SCRIPTCACHE_VERSION1);

    // Write the script name
    chunk->WriteString(name.data());

    // Write the number of sections
    chunk->WriteInt((int) sections.size());

    for (auto section: sections) {
        std::string &filename = std::get<0>(section);
        chunk->WriteString(filename.data());

        std::string &code = std::get<1>(section);

        chunk->WriteDword(code.size());
        if (code.size() != 0) {
            chunk->WriteBufferNoSize(code.size(), code.data());
        }
    }

    chunk->CloseChunk();
    return true;
}

int CachedScript::GetTypeMetadataCount(int typeId) const {
    const auto &vec = GetVectorMetadata(metadata.typeMetadataMap, typeId);
    return static_cast<int>(vec.size());
}

const char *CachedScript::GetTypeMetadata(int typeId, int metaIndex) const {
    const auto &vec = GetVectorMetadata(metadata.typeMetadataMap, typeId);
    if (metaIndex < 0 || metaIndex >= static_cast<int>(vec.size())) {
        return nullptr;
    }
    return vec[metaIndex].c_str();
}

int CachedScript::GetFuncMetadataCount(asIScriptFunction *func) const {
    if (!func) return 0;
    int funcId = func->GetId();
    const auto &vec = GetVectorMetadata(metadata.funcMetadataMap, funcId);
    return static_cast<int>(vec.size());
}

const char *CachedScript::GetFuncMetadata(asIScriptFunction *func, int metaIndex) const {
    if (!func) return nullptr;
    int funcId = func->GetId();
    const auto &vec = GetVectorMetadata(metadata.funcMetadataMap, funcId);
    if (metaIndex < 0 || metaIndex >= static_cast<int>(vec.size())) {
        return nullptr;
    }

    return vec[metaIndex].c_str();
}

int CachedScript::GetVarMetadataCount(int varIdx) const {
    const auto &vec = GetVectorMetadata(metadata.varMetadataMap, varIdx);
    return static_cast<int>(vec.size());
}

const char *CachedScript::GetVarMetadata(int varIdx, int metaIndex) const {
    const auto &vec = GetVectorMetadata(metadata.varMetadataMap, varIdx);
    if (metaIndex < 0 || metaIndex >= static_cast<int>(vec.size())) {
        return nullptr;
    }
    return vec[metaIndex].c_str();
}

const char *CachedScript::GetNameOfClass(int typeId) const {
    const auto it = metadata.classMetadataMap.find(typeId);
    if (it == metadata.classMetadataMap.end())
        return nullptr;

    return it->second.className.c_str();
}

int CachedScript::GetClassMethodMetadataCount(int typeId, asIScriptFunction *method) const {
    if (!method) return 0;

    auto it = metadata.classMetadataMap.find(typeId);
    if (it == metadata.classMetadataMap.end())
        return 0;

    int methodId = method->GetId();
    const auto &vec = GetVectorMetadata(it->second.funcMetadataMap, methodId);
    return static_cast<int>(vec.size());
}

const char *CachedScript::GetClassMethodMetadata(int typeId, asIScriptFunction *method, int metaIndex) const {
    if (!method) return nullptr;
    const auto it = metadata.classMetadataMap.find(typeId);
    if (it == metadata.classMetadataMap.end())
        return nullptr;

    int methodId = method->GetId();
    const auto &vec = GetVectorMetadata(it->second.funcMetadataMap, methodId);
    if (metaIndex < 0 || metaIndex >= static_cast<int>(vec.size())) {
        return nullptr;
    }

    return vec[metaIndex].c_str();
}

int CachedScript::GetClassVarMetadataCount(int typeId, int varIdx) const {
    auto it = metadata.classMetadataMap.find(typeId);
    if (it == metadata.classMetadataMap.end())
        return 0;

    const auto &vec = GetVectorMetadata(it->second.varMetadataMap, varIdx);
    return static_cast<int>(vec.size());
}

const char *CachedScript::GetClassVarMetadata(int typeId, int varIdx, int metaIndex) const {
    auto it = metadata.classMetadataMap.find(typeId);
    if (it == metadata.classMetadataMap.end())
        return nullptr;

    const auto &vec = GetVectorMetadata(it->second.varMetadataMap, varIdx);
    if (metaIndex < 0 || metaIndex >= static_cast<int>(vec.size()))
        return nullptr;

    return vec[metaIndex].c_str();
}

ScriptCache::ScriptCache() = default;

ScriptCache::~ScriptCache() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CachedScripts.clear();
}

std::shared_ptr<CachedScript> ScriptCache::NewCachedScript(const std::string &scriptName) {
    auto script = GetCachedScript(scriptName);
    if (!script) {
        script = std::make_shared<CachedScript>();
        script->name = scriptName;
        CacheScript(scriptName, script);
    }
    return script;
}

std::shared_ptr<CachedScript> ScriptCache::GetCachedScript(const std::string &scriptName) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_CachedScripts.find(scriptName);
    if (it != m_CachedScripts.end()) {
        return it->second;
    }
    return nullptr;
}

void ScriptCache::CacheScript(const std::string &scriptName, std::shared_ptr<CachedScript> script) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    // If there's an existing module with this name, release it first
    auto it = m_CachedScripts.find(scriptName);
    if (it != m_CachedScripts.end()) {
        it->second.reset();
    }
    m_CachedScripts[scriptName] = std::move(script);
}

void ScriptCache::Invalidate(const std::string &scriptName) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_CachedScripts.find(scriptName);
    if (it != m_CachedScripts.end()) {
        m_CachedScripts.erase(it);
    }
}

std::shared_ptr<CachedScript> ScriptCache::LoadScript(asIScriptEngine *engine,
                                                      const std::string &scriptName, const std::string &filename) {
    if (!engine) {
        return nullptr;
    }

    if (auto cached = GetCachedScript(scriptName)) {
        if (!cached->module) {
            if (!cached->Build(engine)) {
                return nullptr;
            }
        }
        return cached; // Return existing
    }

    auto newScript = std::make_shared<CachedScript>();
    newScript->name = scriptName;
    newScript->AddSection(filename);

    if (!newScript->Build(engine)) {
        return nullptr;
    }

    // Store in cache
    CacheScript(scriptName, newScript);

    return newScript;
}

std::shared_ptr<CachedScript> ScriptCache::LoadScript(asIScriptEngine *engine,
    const std::string &scriptName, const std::vector<std::string> &filenames) {
    if (!engine) {
        return nullptr;
    }

    if (auto cached = GetCachedScript(scriptName)) {
        if (!cached->module) {
            if (!cached->Build(engine)) {
                return nullptr;
            }
        }
        return cached; // Return existing
    }

    auto newScript = std::make_shared<CachedScript>();
    newScript->name = scriptName;
    for (auto &filename: filenames) {
        newScript->AddSection(filename);
    }

    if (!newScript->Build(engine)) {
        return nullptr;
    }

    // Store in cache
    CacheScript(scriptName, newScript);

    return newScript;
}

std::shared_ptr<CachedScript> ScriptCache::CompileScript(asIScriptEngine *engine,
    const std::string &scriptName, const std::string &scriptCode) {
    if (!engine) {
        return nullptr;
    }

    if (auto cached = GetCachedScript(scriptName)) {
        if (!cached->module) {
            if (!cached->Build(engine)) {
                return nullptr;
            }
        }
        return cached; // Return existing
    }

    auto newScript = std::make_shared<CachedScript>();
    newScript->name = scriptName;
    newScript->AddSection(scriptName, scriptCode);

    if (!newScript->Build(engine)) {
        return nullptr;
    }

    // Store in cache
    CacheScript(scriptName, newScript);

    return newScript;
}