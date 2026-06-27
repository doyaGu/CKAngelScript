#include "ScriptCache.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstring>
#include <unordered_map>
#include <utility>

#include "ScriptAngelScriptGc.h"
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

namespace {

void DiscardCachedScript(const std::shared_ptr<CachedScript> &script) {
    if (script) {
        script->Discard();
    }
}

void DiscardCachedScripts(const std::vector<std::shared_ptr<CachedScript>> &scripts) {
    for (const std::shared_ptr<CachedScript> &script : scripts) {
        DiscardCachedScript(script);
    }
}

} // namespace

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

namespace {

std::string NormalizeSnapshotSectionName(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');

    std::string prefix;
    size_t start = 0;
    if (path.size() >= 2 && path[1] == ':') {
        prefix = path.substr(0, 2);
        start = 2;
        if (start < path.size() && path[start] == '/')
            ++start;
    } else if (!path.empty() && path[0] == '/') {
        prefix = "/";
        start = 1;
    }

    std::vector<std::string> parts;
    while (start <= path.size()) {
        const size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part.empty() || part == ".") {
            // skip
        } else if (part == "..") {
            if (!parts.empty())
                parts.pop_back();
        } else {
            parts.push_back(part);
        }
        if (end == std::string::npos)
            break;
        start = end + 1;
    }

    std::string normalized = prefix;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (!normalized.empty() && normalized.back() != '/')
            normalized.push_back('/');
        normalized += parts[i];
    }
    if (normalized.empty())
        normalized = ".";

#ifdef _WIN32
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
#endif
    return normalized;
}

std::string ResolveSnapshotIncludeName(const char *include, const char *from) {
    std::string includeName = include ? include : "";
    std::replace(includeName.begin(), includeName.end(), '\\', '/');
    const bool includeAbsolute = !includeName.empty() &&
                                 (includeName[0] == '/' ||
                                  (includeName.size() >= 2 && includeName[1] == ':'));
    if (includeAbsolute)
        return NormalizeSnapshotSectionName(includeName);

    std::string base = from ? from : "";
    std::replace(base.begin(), base.end(), '\\', '/');
    const size_t slash = base.find_last_of('/');
    if (slash != std::string::npos)
        includeName = base.substr(0, slash + 1) + includeName;
    return NormalizeSnapshotSectionName(includeName);
}

struct SnapshotSection {
    std::string Name;
    std::string Code;
};

struct SnapshotIncludeContext {
    std::unordered_map<std::string, SnapshotSection> Sections;
    std::vector<ScriptIncludeEdge> *IncludeEdges = nullptr;
};

int SnapshotIncludeCallback(const char *include,
                            const char *from,
                            CScriptBuilder *builder,
                            void *userParam) {
    auto *context = static_cast<SnapshotIncludeContext *>(userParam);
    if (!context || !builder)
        return -1;

    const std::string key = ResolveSnapshotIncludeName(include, from);
    const auto it = context->Sections.find(key);
    if (it == context->Sections.end()) {
        asIScriptEngine *engine = builder->GetEngine();
        if (engine) {
            const std::string message = "Failed to resolve snapshot include '" +
                                        std::string(include ? include : "") + "'";
            engine->WriteMessage(from ? from : "", 0, 0, asMSGTYPE_ERROR, message.c_str());
        }
        return -1;
    }

    const SnapshotSection &section = it->second;
    if (context->IncludeEdges) {
        ScriptIncludeEdge edge;
        edge.FromSection = from ? from : "";
        edge.ToSection = section.Name;
        edge.ResolvedFromSnapshot = true;
        context->IncludeEdges->push_back(std::move(edge));
    }
    return builder->AddSectionFromMemory(section.Name.c_str(),
                                         section.Code.c_str(),
                                         static_cast<unsigned int>(section.Code.size()),
                                         0);
}

} // namespace

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

namespace {

static std::string QualifiedName(const char *nameSpace, const char *name) {
    std::string result;
    if (nameSpace && nameSpace[0] != '\0') {
        result += nameSpace;
        result += "::";
    }
    result += name ? name : "";
    return result;
}

static asITypeInfo *FindMatchingType(asIScriptModule *module, asITypeInfo *sourceType) {
    if (!module || !sourceType)
        return nullptr;

    const std::string qualified = QualifiedName(sourceType->GetNamespace(), sourceType->GetName());
    asITypeInfo *target = module->GetTypeInfoByDecl(qualified.c_str());
    if (target)
        return target;

    const char *sourceName = sourceType->GetName();
    const char *sourceNamespace = sourceType->GetNamespace();
    for (asUINT i = 0; i < module->GetObjectTypeCount(); ++i) {
        asITypeInfo *candidate = module->GetObjectTypeByIndex(i);
        if (!candidate)
            continue;
        const char *candidateName = candidate->GetName();
        const char *candidateNamespace = candidate->GetNamespace();
        if (std::strcmp(candidateName ? candidateName : "", sourceName ? sourceName : "") == 0 &&
            std::strcmp(candidateNamespace ? candidateNamespace : "", sourceNamespace ? sourceNamespace : "") == 0) {
            return candidate;
        }
    }
    return nullptr;
}

static asIScriptFunction *FindMatchingGlobalFunction(asIScriptModule *module, asIScriptFunction *sourceFunction) {
    if (!module || !sourceFunction)
        return nullptr;

    const char *decl = sourceFunction->GetDeclaration(false, true, false);
    if (decl && decl[0] != '\0') {
        asIScriptFunction *target = module->GetFunctionByDecl(decl);
        if (target)
            return target;
    }

    const char *name = sourceFunction->GetName();
    if (name && name[0] != '\0')
        return module->GetFunctionByName(name);
    return nullptr;
}

static int FindMatchingGlobalVar(asIScriptModule *module, const char *declaration) {
    if (!module || !declaration || declaration[0] == '\0')
        return -1;
    const int index = module->GetGlobalVarIndexByDecl(declaration);
    if (index >= 0)
        return index;

    for (asUINT i = 0; i < module->GetGlobalVarCount(); ++i) {
        const char *candidateDecl = module->GetGlobalVarDeclaration(i, true);
        if (candidateDecl && std::strcmp(candidateDecl, declaration) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

static int FindMatchingProperty(asITypeInfo *type, const char *declaration) {
    if (!type || !declaration || declaration[0] == '\0')
        return -1;
    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *candidateDecl = type->GetPropertyDeclaration(i, false);
        if (candidateDecl && std::strcmp(candidateDecl, declaration) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace

bool ScriptMetadata::RemapForModule(asIScriptModule *fromModule,
                                    asIScriptModule *toModule,
                                    const ScriptMetadata &fromMetadata,
                                    ScriptMetadata &outMetadata) {
    outMetadata.Clear();
    if (!fromModule || !toModule)
        return false;

    asIScriptEngine *engine = fromModule->GetEngine();
    if (!engine)
        return false;

    bool complete = true;
    for (const auto &entry : fromMetadata.typeMetadataMap) {
        asITypeInfo *sourceType = engine->GetTypeInfoById(entry.first);
        asITypeInfo *targetType = FindMatchingType(toModule, sourceType);
        if (targetType) {
            outMetadata.typeMetadataMap[targetType->GetTypeId()] = entry.second;
        } else {
            complete = false;
        }
    }

    for (const auto &entry : fromMetadata.funcMetadataMap) {
        asIScriptFunction *sourceFunction = engine->GetFunctionById(entry.first);
        asIScriptFunction *targetFunction = FindMatchingGlobalFunction(toModule, sourceFunction);
        if (targetFunction) {
            outMetadata.funcMetadataMap[targetFunction->GetId()] = entry.second;
        } else {
            complete = false;
        }
    }

    for (const auto &entry : fromMetadata.varMetadataMap) {
        const char *declaration = fromModule->GetGlobalVarDeclaration(static_cast<asUINT>(entry.first), true);
        const int targetIndex = FindMatchingGlobalVar(toModule, declaration);
        if (targetIndex >= 0) {
            outMetadata.varMetadataMap[targetIndex] = entry.second;
        } else {
            complete = false;
        }
    }

    for (const auto &entry : fromMetadata.classMetadataMap) {
        asITypeInfo *sourceType = engine->GetTypeInfoById(entry.first);
        asITypeInfo *targetType = FindMatchingType(toModule, sourceType);
        if (!sourceType || !targetType) {
            complete = false;
            continue;
        }

        ScriptMetadata::ClassMetadata classMetadata(targetType->GetName() ? targetType->GetName() : "");
        for (const auto &methodEntry : entry.second.funcMetadataMap) {
            asIScriptFunction *sourceMethod = engine->GetFunctionById(methodEntry.first);
            const char *declaration = sourceMethod ? sourceMethod->GetDeclaration(false, false, false) : nullptr;
            asIScriptFunction *targetMethod = declaration ? targetType->GetMethodByDecl(declaration, false) : nullptr;
            if (targetMethod) {
                classMetadata.funcMetadataMap[targetMethod->GetId()] = methodEntry.second;
            } else {
                complete = false;
            }
        }

        for (const auto &propertyEntry : entry.second.varMetadataMap) {
            const char *declaration = sourceType->GetPropertyDeclaration(static_cast<asUINT>(propertyEntry.first), false);
            const int targetIndex = FindMatchingProperty(targetType, declaration);
            if (targetIndex >= 0) {
                classMetadata.varMetadataMap[targetIndex] = propertyEntry.second;
            } else {
                complete = false;
            }
        }

        outMetadata.classMetadataMap[targetType->GetTypeId()] = std::move(classMetadata);
    }

    return complete;
}

CachedScript::~CachedScript() {
    if (module) {
        ScriptDiscardModuleWithGarbageCollection(module);
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

    auto *man = ScriptManager::GetManager(engine);

    // We will only initialize the global variables once we're
    // ready to execute, so disable the automatic initialization
    // m_ScriptEngine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

    // Prepare the script builder
    CScriptBuilder builder;
    includeEdges.clear();
    SnapshotIncludeContext snapshotIncludeContext;
    if (sourceSnapshotSections) {
        snapshotIncludeContext.IncludeEdges = &includeEdges;
        for (const auto &section : sections) {
            SnapshotSection snapshotSection;
            snapshotSection.Name = std::get<0>(section);
            snapshotSection.Code = std::get<1>(section);
            snapshotIncludeContext.Sections[NormalizeSnapshotSectionName(snapshotSection.Name)] =
                std::move(snapshotSection);
        }
        builder.SetIncludeCallback(SnapshotIncludeCallback, &snapshotIncludeContext);
    }
    builder.SetPragmaCallback(PragmaCallback, nullptr);

#if CKVERSION == 0x13022002
    builder.DefineWord("VT21");
#elif CKVERSION == 0x05082002
    builder.DefineWord("VT25");
#elif CKVERSION == 0x26052005
    builder.DefineWord("VT35");
#endif

    // Start a new module
    int r = builder.StartNewModule(engine, name.c_str());
    auto discardBuilderModule = [&builder]() {
        asIScriptModule *builderModule = builder.GetModule();
        if (builderModule) {
            ScriptDiscardModuleWithGarbageCollection(builderModule);
        }
    };
    if (r < 0) {
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to create module");
        return false;
    }

    // Add the script file. Snapshot sections use section[0] as the entry and
    // resolve any included sections from the in-memory map above.
    const size_t sectionCount = sourceSnapshotSections && !sections.empty() ? 1 : sections.size();
    for (size_t i = 0; i < sectionCount; ++i) {
        auto &section = sections[i];
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

        if (sourceSnapshotSections || code.size() != 0) {
            r = builder.AddSectionFromMemory(filename.c_str(),
                                             code.c_str(),
                                             static_cast<unsigned int>(code.size()),
                                             0);
            if (r < 0) {
                discardBuilderModule();
                engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to load section from memory");
                return false;
            }
        } else {
            r = builder.AddSectionFromFile(resolvedFilename.CStr());
            if (r < 0) {
                discardBuilderModule();
                engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to load section from file");
                return false;
            }
        }
    }

    // Build the module
    r = builder.BuildModule();
    if (r < 0) {
        discardBuilderModule();
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to build module");
        return false;
    }

    // Retrieve the compiled module
    module = builder.GetModule();
    if (!module) {
        discardBuilderModule();
        engine->WriteMessage(name.c_str(), 0, 0, asMSGTYPE_ERROR, "[CachedScript] Failed to retrieve module");
        return false;
    }

    // Extract metadata from the builder
#if AS_PROCESS_METADATA == 1
    ScriptMetadata::Extract(builder, metadata);
#endif

    return true;
}

bool CachedScript::Discard() {
    if (module) {
        ScriptDiscardModuleWithGarbageCollection(module);
        module = nullptr;
        ClearMetadata();
        return true;
    }
    return false;
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

void CachedScript::ClearCodeCache() {
    for (auto &section: sections) {
        std::get<1>(section).clear();
    }
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

    const int numSections = chunk->ReadInt();
    if (numSections < 0) {
        chunk->CloseChunk();
        return false;
    }

    for (int i = 0; i < numSections; ++i) {
        chunk->ReadString(&str);
        if (!str) {
            str = nullptr;
            continue;
        }
        std::string filename = str;
        CKDeletePointer(str);
        str = nullptr;

        std::string buffer;
        CKDWORD size = chunk->ReadDword();
        if (size > static_cast<CKDWORD>(INT_MAX)) {
            chunk->CloseChunk();
            return false;
        }
        if (size != 0) {
            buffer.resize(size);
            chunk->ReadAndFillBuffer(static_cast<int>(size), buffer.data());
        }

        sections.emplace_back(std::move(filename), std::move(buffer));
    }

    chunk->CloseChunk();
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

        if (code.size() > static_cast<size_t>(INT_MAX)) {
            chunk->CloseChunk();
            return false;
        }

        chunk->WriteDword(static_cast<CKDWORD>(code.size()));
        if (code.size() != 0) {
            chunk->WriteBufferNoSize(static_cast<int>(code.size()), code.data());
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

void CachedScript::ClearMetadata() {
    metadata.Clear();
}

ScriptCache::ScriptCache() = default;

ScriptCache::~ScriptCache() {
    Clear();
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
    std::shared_ptr<CachedScript> previous;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_CachedScripts.find(scriptName);
        if (it != m_CachedScripts.end() && it->second != script) {
            previous = it->second;
        }
        m_CachedScripts[scriptName] = std::move(script);
    }
    DiscardCachedScript(previous);
}

void ScriptCache::Invalidate(const std::string &scriptName) {
    std::shared_ptr<CachedScript> removed;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_CachedScripts.find(scriptName);
        if (it != m_CachedScripts.end()) {
            removed = it->second;
            m_CachedScripts.erase(it);
        }
    }
    DiscardCachedScript(removed);
}

void ScriptCache::Clear() {
    std::vector<std::shared_ptr<CachedScript>> scripts;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        scripts.reserve(m_CachedScripts.size());
        for (const auto &entry : m_CachedScripts) {
            scripts.push_back(entry.second);
        }
        m_CachedScripts.clear();
    }
    DiscardCachedScripts(scripts);
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

bool ScriptCache::UnloadScript(const std::string &scriptName) {
    std::shared_ptr<CachedScript> cached;

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_CachedScripts.find(scriptName);
        if (it == m_CachedScripts.end()) {
            return false;
        }
        cached = it->second;
    }

    if (cached) {
        cached->Discard();
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_CachedScripts.find(scriptName);
        if (it != m_CachedScripts.end() && it->second == cached) {
            m_CachedScripts.erase(it);
        }
    }

    return true;
}
