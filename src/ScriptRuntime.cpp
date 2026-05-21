#include "ScriptRuntime.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_set>

#include <fmt/format.h>

#include "CKAll.h"
#include "CKPathManager.h"
#include "CKTimeManager.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptParameterRegistry.h"
#include "ScriptUtils.h"
#include "add_on/scriptarray/scriptarray.h"

namespace ScriptRuntimeInternal {

constexpr const char *kModulePrefix = "__CKASRuntime_";

BehaviorGraph *RuntimeBehaviorGraphByRoot(const ScriptRuntimeContext &ctx, const std::string &rootBehaviorName);

struct KeyValue {
    std::string Key;
    std::string Value;
};

std::string Trim(const std::string &text) {
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string StripQuotes(const std::string &value) {
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::vector<std::string> SplitList(const std::string &value) {
    std::vector<std::string> result;
    std::string current;
    bool quoted = false;
    char quote = '\0';
    for (char c : value) {
        if ((c == '"' || c == '\'') && (!quoted || quote == c)) {
            quoted = !quoted;
            quote = quoted ? c : '\0';
            current.push_back(c);
            continue;
        }
        if (c == ';' && !quoted) {
            std::string item = Trim(StripQuotes(Trim(current)));
            if (!item.empty()) {
                result.push_back(std::move(item));
            }
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    std::string item = Trim(StripQuotes(Trim(current)));
    if (!item.empty()) {
        result.push_back(std::move(item));
    }
    return result;
}

bool ParseBool(const std::string &value, bool fallback) {
    const std::string lowered = ToLower(Trim(StripQuotes(value)));
    if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
        return false;
    }
    return fallback;
}

int ParseInt(const std::string &value, int fallback) {
    char *end = nullptr;
    const std::string text = Trim(StripQuotes(value));
    const long parsed = std::strtol(text.c_str(), &end, 0);
    if (end && *end == '\0') {
        return static_cast<int>(parsed);
    }
    return fallback;
}

std::vector<KeyValue> ParseKeyValues(const std::string &metadataBody) {
    std::vector<KeyValue> result;
    std::size_t pos = 0;
    while (pos < metadataBody.size()) {
        while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
            ++pos;
        }
        if (pos >= metadataBody.size()) {
            break;
        }

        const std::size_t keyStart = pos;
        while (pos < metadataBody.size()) {
            const char c = metadataBody[pos];
            if (std::isspace(static_cast<unsigned char>(c)) || c == '=') {
                break;
            }
            ++pos;
        }
        std::string key = metadataBody.substr(keyStart, pos - keyStart);
        while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
            ++pos;
        }
        std::string value = "true";
        if (pos < metadataBody.size() && metadataBody[pos] == '=') {
            ++pos;
            while (pos < metadataBody.size() && std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
                ++pos;
            }
            if (pos < metadataBody.size() && (metadataBody[pos] == '"' || metadataBody[pos] == '\'')) {
                const char quote = metadataBody[pos++];
                std::string parsed;
                while (pos < metadataBody.size()) {
                    const char c = metadataBody[pos++];
                    if (c == quote) {
                        break;
                    }
                    if (c == '\\' && pos < metadataBody.size()) {
                        parsed.push_back(metadataBody[pos++]);
                    } else {
                        parsed.push_back(c);
                    }
                }
                value = parsed;
            } else {
                const std::size_t valueStart = pos;
                while (pos < metadataBody.size() && !std::isspace(static_cast<unsigned char>(metadataBody[pos]))) {
                    ++pos;
                }
                value = metadataBody.substr(valueStart, pos - valueStart);
            }
        }
        if (!key.empty()) {
            result.push_back({ToLower(key), value});
        }
    }
    return result;
}

std::string SanitizeId(const std::string &id) {
    std::string result;
    result.reserve(id.size());
    for (char c : id) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '_' || c == '-' || c == '.') {
            result.push_back(c);
        } else {
            result.push_back('_');
        }
    }
    return result.empty() ? "script" : result;
}

std::string PathString(const std::filesystem::path &path) {
    return path.string();
}

bool IsSkippedMainFile(const std::filesystem::path &path) {
    const std::string filename = ToLower(path.filename().string());
    return filename.ends_with(".inc.as") || filename.ends_with(".include.as");
}

bool IsSkippedDirectory(const std::filesystem::path &path) {
    const std::string name = path.filename().string();
    return name.empty() || name.front() == '_' || name.front() == '.';
}

std::string ReadTextFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::string();
    }
    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

bool StartsWithWord(const std::string &text, std::size_t pos, const char *word) {
    const std::size_t length = std::strlen(word);
    if (text.compare(pos, length, word) != 0) {
        return false;
    }
    const std::size_t end = pos + length;
    return end >= text.size() || !std::isalnum(static_cast<unsigned char>(text[end]));
}

std::string ParseClassAfterMetadata(const std::string &source, std::size_t pos) {
    while (pos < source.size()) {
        while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
            ++pos;
        }
        if (source.compare(pos, 2, "//") == 0) {
            pos = source.find('\n', pos);
            if (pos == std::string::npos) {
                return std::string();
            }
            continue;
        }
        if (source.compare(pos, 2, "/*") == 0) {
            pos = source.find("*/", pos + 2);
            if (pos == std::string::npos) {
                return std::string();
            }
            pos += 2;
            continue;
        }
        break;
    }
    if (!StartsWithWord(source, pos, "class")) {
        return std::string();
    }
    pos += 5;
    while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
        ++pos;
    }
    const std::size_t start = pos;
    while (pos < source.size()) {
        const unsigned char c = static_cast<unsigned char>(source[pos]);
        if (!std::isalnum(c) && source[pos] != '_') {
            break;
        }
        ++pos;
    }
    return source.substr(start, pos - start);
}

bool ExtractScriptMetadata(const std::string &source,
                           std::map<std::string, std::string> &values,
                           std::string &attachedClass,
                           std::string &error) {
    int count = 0;
    std::size_t pos = 0;
    while ((pos = source.find("[script", pos)) != std::string::npos) {
        const std::size_t start = pos;
        pos += 7;
        if (pos < source.size()) {
            const char next = source[pos];
            if (!std::isspace(static_cast<unsigned char>(next)) && next != ']') {
                continue;
            }
        }

        bool quoted = false;
        char quote = '\0';
        std::size_t end = pos;
        for (; end < source.size(); ++end) {
            const char c = source[end];
            if ((c == '"' || c == '\'') && (!quoted || quote == c)) {
                quoted = !quoted;
                quote = quoted ? c : '\0';
            } else if (c == ']' && !quoted) {
                break;
            }
        }
        if (end >= source.size()) {
            error = "unterminated [script] metadata";
            return false;
        }
        ++count;
        if (count > 1) {
            error = "multiple [script] metadata blocks are not allowed";
            return false;
        }
        const std::string body = source.substr(pos, end - pos);
        for (const KeyValue &pair : ParseKeyValues(body)) {
            values[pair.Key] = pair.Value;
        }
        attachedClass = ParseClassAfterMetadata(source, end + 1);
        pos = end + 1;
        (void)start;
    }
    return true;
}

std::string MetadataValue(const std::map<std::string, std::string> &values,
                          const std::string &key,
                          const std::string &fallback = std::string()) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

CKBehavior *FindBehaviorByName(CKContext *context, const std::string &name) {
    if (!context || name.empty()) {
        return nullptr;
    }
    return CKBehavior::Cast(context->GetObjectByName(const_cast<CKSTRING>(name.c_str())));
}

CKBehaviorContext MakeBehaviorContext(const ScriptRuntimeContext &ctx, CKBehavior *behavior = nullptr) {
    CKBehaviorContext behaviorContext = ctx.ToBehaviorContext();
    behaviorContext.Behavior = behavior;
    return behaviorContext;
}

ScriptManager *ManagerFromRuntimeContext(const ScriptRuntimeContext &ctx) {
    return ctx.Context() ? ScriptManager::GetManager(ctx.Context()) : nullptr;
}

ScriptBehaviorBridge *BridgeFromRuntimeContext(const ScriptRuntimeContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

BehaviorBridge *RuntimeBehaviorFrom(const ScriptRuntimeContext &ctx) {
    ScriptBehaviorBridge *bridge = BridgeFromRuntimeContext(ctx);
    return bridge ? new BehaviorBridge(bridge, MakeBehaviorContext(ctx)) : nullptr;
}

BehaviorGraph *RuntimeBehaviorGraph(const ScriptRuntimeContext &ctx) {
    return RuntimeBehaviorGraphByRoot(ctx, std::string());
}

BehaviorGraph *RuntimeBehaviorGraphByRoot(const ScriptRuntimeContext &ctx, const std::string &rootBehaviorName) {
    ScriptBehaviorBridge *bridge = BridgeFromRuntimeContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    CKBehavior *root = nullptr;
    if (!rootBehaviorName.empty()) {
        root = FindBehaviorByName(ctx.Context(), rootBehaviorName);
    }
    return new BehaviorGraph(bridge, MakeBehaviorContext(ctx, root), root ? root->GetID() : 0);
}

BehaviorQuery *RuntimeBehaviorQuery() {
    return new BehaviorQuery();
}

BehaviorRef *RuntimeBehaviorFind(const ScriptRuntimeContext &ctx, const std::string &name) {
    ScriptBehaviorBridge *bridge = BridgeFromRuntimeContext(ctx);
    CKBehavior *behavior = FindBehaviorByName(ctx.Context(), name);
    return bridge ? bridge->WrapBehavior(behavior, 0) : nullptr;
}

BehaviorRef *RuntimeBehaviorFindById(const ScriptRuntimeContext &ctx, CK_ID id) {
    ScriptBehaviorBridge *bridge = BridgeFromRuntimeContext(ctx);
    CKObject *object = ctx.Context() ? CKGetObject(ctx.Context(), id) : nullptr;
    return bridge ? bridge->WrapBehavior(CKBehavior::Cast(object), 0) : nullptr;
}

BBBridge *RuntimeBBFrom(const ScriptRuntimeContext &ctx) {
    ScriptBehaviorBridge *bridge = BridgeFromRuntimeContext(ctx);
    return bridge ? new BBBridge(bridge, MakeBehaviorContext(ctx)) : nullptr;
}

BBDecl *RuntimeBBRequireByName(const ScriptRuntimeContext &ctx, const std::string &query) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *decl = bridge->Require(query);
    bridge->Release();
    return decl;
}

BBDecl *RuntimeBBRequireByGuid(const ScriptRuntimeContext &ctx, CKGUID guid) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *decl = bridge->RequireGuid(guid);
    bridge->Release();
    return decl;
}

int RuntimeBBCount(const ScriptRuntimeContext &ctx) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return 0;
    }
    const int count = bridge->Count();
    bridge->Release();
    return count;
}

BBPrototype *RuntimeBBAt(const ScriptRuntimeContext &ctx, int index) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->At(index);
    bridge->Release();
    return prototype;
}

BBPrototype *RuntimeBBFind(const ScriptRuntimeContext &ctx, const std::string &query, int occurrence) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->Find(query, occurrence);
    bridge->Release();
    return prototype;
}

CScriptArray *RuntimeBBFindAll(const ScriptRuntimeContext &ctx, const std::string &query) {
    BBBridge *bridge = RuntimeBBFrom(ctx);
    if (!bridge) {
        return nullptr;
    }
    CScriptArray *array = bridge->FindAll(query);
    bridge->Release();
    return array;
}

ParamTypeInfo *RuntimeParamAt(const ScriptRuntimeContext &ctx, int index) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    ScriptParameterRegistry *registry = manager ? manager->GetParameterRegistry() : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->At(index) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamFind(const ScriptRuntimeContext &ctx, const std::string &query, int occurrence) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    ScriptParameterRegistry *registry = manager ? manager->GetParameterRegistry() : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->Find(query, occurrence) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamTypeByName(const ScriptRuntimeContext &ctx, const std::string &typeName) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    ScriptParameterRegistry *registry = manager ? manager->GetParameterRegistry() : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamTypeByGuid(const ScriptRuntimeContext &ctx, CKGUID guid) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    ScriptParameterRegistry *registry = manager ? manager->GetParameterRegistry() : nullptr;
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

int RuntimeParamCount(const ScriptRuntimeContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    ScriptParameterRegistry *registry = manager ? manager->GetParameterRegistry() : nullptr;
    return registry ? registry->Count() : 0;
}

bool RuntimeReloadAll(const ScriptRuntimeContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->ReloadAll(&error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

bool RuntimeReload(const ScriptRuntimeContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->Reload(id, &error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

bool RuntimeEnable(const ScriptRuntimeContext &ctx, const std::string &id, bool enabled) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->Enable(id, enabled, &error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

CScriptArray *RuntimeList(const ScriptRuntimeContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl("array<string>") : nullptr;
    if (!arrayType || !manager || !manager->GetRuntime()) {
        return nullptr;
    }
    std::vector<std::string> ids = manager->GetRuntime()->List();
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(ids.size()));
    for (asUINT i = 0; i < static_cast<asUINT>(ids.size()); ++i) {
        array->SetValue(i, &ids[i]);
    }
    return array;
}

} // namespace ScriptRuntimeInternal

struct ScriptRuntime::Metadata {
    std::string Id;
    std::string Name;
    std::string ClassName;
    std::filesystem::path RootPath;
    std::filesystem::path ManifestPath;
    std::filesystem::path EntryPath;
    std::vector<std::filesystem::path> Files;
    bool Enabled = true;
    int Order = 1000;
    bool Reloadable = true;
    bool HasManifest = false;
    std::string Error;
};

struct ScriptRuntime::Module {
    Metadata Meta;
    std::string ModuleName;
    std::shared_ptr<CachedScript> Cached;
    asIScriptObject *Object = nullptr;
    asITypeInfo *Type = nullptr;
    bool Enabled = true;
    bool Loaded = false;
    bool AwakeCalled = false;
    bool EnableCalled = false;
    bool StartCalled = false;
    bool Failed = false;
    std::string Error;
};

ScriptRuntimeContext::ScriptRuntimeContext() = default;

ScriptRuntimeContext::ScriptRuntimeContext(CKContext *context,
                                           std::string scriptId,
                                           std::string rootPath,
                                           float deltaTime,
                                           float timeSeconds)
    : m_Context(context),
      m_ScriptId(std::move(scriptId)),
      m_RootPath(std::move(rootPath)),
      m_DeltaTime(deltaTime),
      m_TimeSeconds(timeSeconds) {}

CKContext *ScriptRuntimeContext::Context() const {
    return m_Context;
}

float ScriptRuntimeContext::DeltaTime() const {
    return m_DeltaTime;
}

float ScriptRuntimeContext::TimeSeconds() const {
    return m_TimeSeconds;
}

std::string ScriptRuntimeContext::ScriptId() const {
    return m_ScriptId;
}

std::string ScriptRuntimeContext::RootPath() const {
    return m_RootPath;
}

void ScriptRuntimeContext::Raise(const std::string &message) const {
    if (m_Context) {
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Runtime:%s] %s"),
                                     m_ScriptId.c_str(),
                                     message.c_str());
    }
}

CKBehaviorContext ScriptRuntimeContext::ToBehaviorContext() const {
    CKBehaviorContext ctx = m_Context ? m_Context->m_BehaviorContext : CKBehaviorContext();
    ctx.Context = m_Context;
    ctx.DeltaTime = m_DeltaTime;
    if (m_Context) {
        ctx.TimeManager = m_Context->GetTimeManager();
        ctx.ParameterManager = m_Context->GetParameterManager();
        ctx.MessageManager = m_Context->GetMessageManager();
        ctx.AttributeManager = m_Context->GetAttributeManager();
    }
    return ctx;
}

ScriptRuntime::ScriptRuntime(ScriptManager *manager) : m_Manager(manager) {}

ScriptRuntime::~ScriptRuntime() {
    Clear();
}

void ScriptRuntime::PreProcess() {
    EnsureScanned();
    if (m_Paused) {
        return;
    }

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKTimeManager *time = context ? context->GetTimeManager() : nullptr;
    const float deltaTime = time ? time->GetLastDeltaTime() : 0.0f;
    const float timeSeconds = time ? (time->GetTime() * 0.001f) : 0.0f;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            UpdateModule(*module, deltaTime, timeSeconds);
        }
    }
}

void ScriptRuntime::PostLoad() {
    EnsureScanned();
}

void ScriptRuntime::OnReset() {
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                 module->Meta.Id,
                                 ScriptRuntimeInternal::PathString(module->Meta.RootPath),
                                 0.0f,
                                 0.0f);
        Invoke(*module, "OnReset", ctx);
        module->StartCalled = false;
    }
}

void ScriptRuntime::OnPause() {
    if (m_Paused) {
        return;
    }
    m_Paused = true;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed || !module->Enabled) {
            continue;
        }
        ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                 module->Meta.Id,
                                 ScriptRuntimeInternal::PathString(module->Meta.RootPath),
                                 0.0f,
                                 0.0f);
        Invoke(*module, "OnPause", ctx);
        if (module->EnableCalled) {
            Invoke(*module, "OnDisable", ctx);
            module->EnableCalled = false;
        }
    }
}

void ScriptRuntime::OnResume() {
    if (!m_Paused) {
        return;
    }
    m_Paused = false;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed || !module->Enabled) {
            continue;
        }
        ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                 module->Meta.Id,
                                 ScriptRuntimeInternal::PathString(module->Meta.RootPath),
                                 0.0f,
                                 0.0f);
        Invoke(*module, "OnResume", ctx);
        if (!module->EnableCalled) {
            Invoke(*module, "OnEnable", ctx);
            module->EnableCalled = true;
        }
    }
}

void ScriptRuntime::OnEnd() {
    Clear();
}

void ScriptRuntime::Clear() {
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            DestroyModule(*module);
        }
    }
    m_Modules.clear();
    m_Scanned = false;
    m_Paused = false;
}

bool ScriptRuntime::ReloadAll(std::string *error) {
    std::string discoveryError;
    std::vector<Metadata> scripts = Discover(discoveryError);
    if (!discoveryError.empty()) {
        if (error) {
            *error = discoveryError;
        }
        OutputDiagnostic(discoveryError);
    }
    m_Scanned = true;
    return LoadDiscovered(scripts);
}

bool ScriptRuntime::Reload(const std::string &id, std::string *error) {
    EnsureScanned();
    std::string discoveryError;
    std::vector<Metadata> scripts = Discover(discoveryError);
    const std::string canonical = ScriptRuntimeInternal::SanitizeId(id);
    const auto it = std::find_if(scripts.begin(), scripts.end(), [&](const Metadata &metadata) {
        return metadata.Id == canonical || metadata.Id == id;
    });
    if (it == scripts.end()) {
        if (error) {
            *error = fmt::format("Runtime script '{}' was not found during DATA_PATH Scripts scan.", id);
        }
        return false;
    }

    std::unique_ptr<Module> module;
    std::string loadError;
    if (!LoadModule(*it, module, loadError)) {
        if (error) {
            *error = loadError;
        }
        return false;
    }
    ReplaceModule(*it, std::move(module));
    return true;
}

bool ScriptRuntime::Enable(const std::string &id, bool enabled, std::string *error) {
    EnsureScanned();
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module && module->Meta.Id == id) {
            module->Enabled = enabled;
            module->Meta.Enabled = enabled;
            if (!enabled) {
                DisableModule(*module);
            }
            return true;
        }
    }
    if (error) {
        *error = fmt::format("Runtime script '{}' is not loaded.", id);
    }
    return false;
}

std::vector<std::string> ScriptRuntime::List() const {
    std::vector<std::string> result;
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            result.push_back(module->Meta.Id);
        }
    }
    return result;
}

void ScriptRuntime::EnsureScanned() {
    if (m_Scanned) {
        return;
    }
    std::string error;
    std::vector<Metadata> scripts = Discover(error);
    if (!error.empty()) {
        OutputDiagnostic(error);
    }
    LoadDiscovered(scripts);
    m_Scanned = true;
}

std::vector<ScriptRuntime::Metadata> ScriptRuntime::Discover(std::string &error) const {
    namespace fs = std::filesystem;
    std::vector<fs::path> roots;
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKPathManager *paths = context ? context->GetPathManager() : nullptr;
    if (paths) {
        const int count = paths->GetPathCount(DATA_PATH_IDX);
        for (int i = 0; i < count; ++i) {
            XString pathName;
            if (paths->GetPathName(DATA_PATH_IDX, i, pathName) == CK_OK && pathName.Length() > 0) {
                roots.emplace_back(pathName.CStr());
            }
        }
    }
    if (const char *envRoots = std::getenv("CKAS_SCRIPT_ROOTS")) {
        for (const std::string &root : ScriptRuntimeInternal::SplitList(envRoots)) {
            roots.emplace_back(root);
        }
    }

    std::vector<Metadata> result;
    std::unordered_set<std::string> seenRoots;
    std::set<std::string> seenIds;
    for (fs::path root : roots) {
        root = fs::absolute(root) / "Scripts";
        std::error_code ec;
        root = fs::weakly_canonical(root, ec);
        if (ec) {
            root = fs::absolute(root);
        }
        const std::string rootKey = ScriptRuntimeInternal::ToLower(ScriptRuntimeInternal::PathString(root));
        if (!seenRoots.insert(rootKey).second || !fs::is_directory(root)) {
            continue;
        }

        std::vector<fs::path> candidates;
        for (const fs::directory_entry &entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                break;
            }
            const fs::path path = entry.path();
            if (entry.is_regular_file() && path.extension() == ".as" && !ScriptRuntimeInternal::IsSkippedMainFile(path)) {
                candidates.push_back(path);
            } else if (entry.is_directory() && !ScriptRuntimeInternal::IsSkippedDirectory(path)) {
                fs::path main = path / "main.as";
                if (fs::is_regular_file(main) && !ScriptRuntimeInternal::IsSkippedMainFile(main)) {
                    candidates.push_back(main);
                }
            }
        }
        std::sort(candidates.begin(), candidates.end());

        for (const fs::path &candidate : candidates) {
            Metadata metadata;
            metadata.RootPath = candidate.parent_path();
            metadata.ManifestPath = candidate;
            metadata.EntryPath = candidate;
            metadata.Id = candidate.stem().string();
            metadata.Name = metadata.Id;

            std::map<std::string, std::string> values;
            std::string attachedClass;
            std::string parseError;
            const std::string source = ScriptRuntimeInternal::ReadTextFile(candidate);
            if (!ScriptRuntimeInternal::ExtractScriptMetadata(source, values, attachedClass, parseError)) {
                error += fmt::format("Skipping '{}': {}.\n", ScriptRuntimeInternal::PathString(candidate), parseError);
                continue;
            }
            metadata.HasManifest = !values.empty();
            if (metadata.HasManifest) {
                metadata.Id = ScriptRuntimeInternal::MetadataValue(values, "id", metadata.Id);
                metadata.Name = ScriptRuntimeInternal::MetadataValue(values, "name", metadata.Id);
                metadata.ClassName = ScriptRuntimeInternal::MetadataValue(values, "class", attachedClass);
                metadata.Enabled = ScriptRuntimeInternal::ParseBool(ScriptRuntimeInternal::MetadataValue(values, "enabled", "true"), true);
                metadata.Order = ScriptRuntimeInternal::ParseInt(ScriptRuntimeInternal::MetadataValue(values, "order", "1000"), 1000);
                metadata.Reloadable = ScriptRuntimeInternal::ParseBool(ScriptRuntimeInternal::MetadataValue(values, "reload", "true"), true);
                const std::string entry = ScriptRuntimeInternal::MetadataValue(values, "entry");
                if (!entry.empty()) {
                    metadata.EntryPath = metadata.RootPath / entry;
                }
            }
            metadata.Id = ScriptRuntimeInternal::SanitizeId(metadata.Id);

            metadata.Files.push_back(metadata.EntryPath);
            const std::string filesText = ScriptRuntimeInternal::MetadataValue(values, "files");
            for (const std::string &file : ScriptRuntimeInternal::SplitList(filesText)) {
                metadata.Files.push_back(metadata.RootPath / file);
            }
            std::vector<fs::path> uniqueFiles;
            std::set<std::string> seenFiles;
            for (fs::path file : metadata.Files) {
                file = fs::absolute(file);
                const std::string fileKey = ScriptRuntimeInternal::ToLower(ScriptRuntimeInternal::PathString(file));
                if (seenFiles.insert(fileKey).second) {
                    uniqueFiles.push_back(file);
                }
            }
            metadata.Files = std::move(uniqueFiles);

            bool missing = false;
            for (const fs::path &file : metadata.Files) {
                if (!fs::is_regular_file(file)) {
                    error += fmt::format("Skipping '{}': source file '{}' is missing.\n",
                                         metadata.Id,
                                         ScriptRuntimeInternal::PathString(file));
                    missing = true;
                    break;
                }
            }
            if (missing) {
                continue;
            }
            if (!metadata.Enabled) {
                continue;
            }
            if (!seenIds.insert(metadata.Id).second) {
                error += fmt::format("Duplicate runtime script id '{}' from '{}' ignored.\n",
                                     metadata.Id,
                                     ScriptRuntimeInternal::PathString(candidate));
                continue;
            }
            result.push_back(std::move(metadata));
        }
    }

    std::sort(result.begin(), result.end(), [](const Metadata &lhs, const Metadata &rhs) {
        if (lhs.Order != rhs.Order) {
            return lhs.Order < rhs.Order;
        }
        return lhs.Id < rhs.Id;
    });
    return result;
}

bool ScriptRuntime::LoadDiscovered(const std::vector<Metadata> &scripts) {
    bool ok = true;
    for (const Metadata &metadata : scripts) {
        std::unique_ptr<Module> module;
        std::string error;
        if (!LoadModule(metadata, module, error)) {
            ok = false;
            OutputDiagnostic(error);
            continue;
        }
        ReplaceModule(metadata, std::move(module));
    }
    return ok;
}

bool ScriptRuntime::LoadModule(const Metadata &metadata, std::unique_ptr<Module> &module, std::string &error) {
    asIScriptEngine *engine = m_Manager ? m_Manager->GetScriptEngine() : nullptr;
    if (!engine) {
        error = "Script runtime cannot load modules before the AngelScript engine is initialized.";
        return false;
    }

    const std::string moduleName = fmt::format("{}{}_{}", ScriptRuntimeInternal::kModulePrefix, metadata.Id, ++m_Generation);
    std::vector<std::string> files;
    files.reserve(metadata.Files.size());
    for (const std::filesystem::path &file : metadata.Files) {
        files.push_back(ScriptRuntimeInternal::PathString(file));
    }
    std::shared_ptr<CachedScript> cached = m_Manager->GetScriptCache().LoadScript(engine, moduleName, files);
    if (!cached || !cached->module) {
        error = fmt::format("Runtime script '{}' failed to compile.", metadata.Id);
        return false;
    }

    std::unique_ptr<Module> loaded = std::make_unique<Module>();
    loaded->Meta = metadata;
    loaded->ModuleName = moduleName;
    loaded->Cached = cached;
    loaded->Enabled = metadata.Enabled;
    loaded->Loaded = true;

    if (!metadata.ClassName.empty()) {
        loaded->Type = cached->module->GetTypeInfoByName(metadata.ClassName.c_str());
        if (!loaded->Type) {
            error = fmt::format("Runtime script '{}' class '{}' was not found.", metadata.Id, metadata.ClassName);
            m_Manager->GetScriptCache().UnloadScript(moduleName);
            return false;
        }
        void *object = engine->CreateScriptObject(loaded->Type);
        if (!object) {
            error = fmt::format("Runtime script '{}' failed to create class '{}'.", metadata.Id, metadata.ClassName);
            m_Manager->GetScriptCache().UnloadScript(moduleName);
            return false;
        }
        loaded->Object = static_cast<asIScriptObject *>(object);
    }

    ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                             metadata.Id,
                             ScriptRuntimeInternal::PathString(metadata.RootPath),
                             0.0f,
                             0.0f);
    if (!Invoke(*loaded, "OnLoad", ctx) || !Invoke(*loaded, "Awake", ctx)) {
        error = fmt::format("Runtime script '{}' failed during load: {}", metadata.Id, loaded->Error);
        DestroyModule(*loaded);
        return false;
    }
    loaded->AwakeCalled = true;
    module = std::move(loaded);
    return true;
}

bool ScriptRuntime::ReplaceModule(const Metadata &metadata, std::unique_ptr<Module> module) {
    for (std::unique_ptr<Module> &existing : m_Modules) {
        if (existing && existing->Meta.Id == metadata.Id) {
            DestroyModule(*existing);
            existing = std::move(module);
            return true;
        }
    }
    m_Modules.push_back(std::move(module));
    return true;
}

void ScriptRuntime::DestroyModule(Module &module) {
    if (module.Loaded && !module.Failed) {
        ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                 module.Meta.Id,
                                 ScriptRuntimeInternal::PathString(module.Meta.RootPath),
                                 0.0f,
                                 0.0f);
        if (module.EnableCalled) {
            Invoke(module, "OnDisable", ctx);
            module.EnableCalled = false;
        }
        Invoke(module, "OnDestroy", ctx);
    }
    if (module.Object) {
        module.Object->Release();
        module.Object = nullptr;
    }
    if (m_Manager && !module.ModuleName.empty()) {
        m_Manager->GetScriptCache().UnloadScript(module.ModuleName);
    }
    module.Cached.reset();
    module.Loaded = false;
}

void ScriptRuntime::DisableModule(Module &module) {
    if (!module.EnableCalled) {
        return;
    }
    ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                             module.Meta.Id,
                             ScriptRuntimeInternal::PathString(module.Meta.RootPath),
                             0.0f,
                             0.0f);
    Invoke(module, "OnDisable", ctx);
    module.EnableCalled = false;
}

void ScriptRuntime::UpdateModule(Module &module, float deltaTime, float timeSeconds) {
    if (!module.Loaded || !module.Enabled || module.Failed) {
        return;
    }
    ScriptRuntimeContext ctx(m_Manager ? m_Manager->GetCKContext() : nullptr,
                             module.Meta.Id,
                             ScriptRuntimeInternal::PathString(module.Meta.RootPath),
                             deltaTime,
                             timeSeconds);
    if (!module.EnableCalled) {
        if (!Invoke(module, "OnEnable", ctx)) {
            return;
        }
        module.EnableCalled = true;
    }
    if (!module.StartCalled) {
        if (!Invoke(module, "Start", ctx)) {
            return;
        }
        module.StartCalled = true;
    }
    Invoke(module, "Update", ctx);
}

bool ScriptRuntime::Invoke(Module &module, const char *name, const ScriptRuntimeContext &context, bool required) {
    if (!module.Cached || !module.Cached->module || !name || name[0] == '\0') {
        return !required;
    }

    asIScriptFunction *func = nullptr;
    std::string decl = fmt::format("void {}(const ScriptRuntimeContext &in ctx)", name);
    if (module.Type) {
        func = module.Type->GetMethodByDecl(decl.c_str());
        if (!func) {
            decl = fmt::format("void {}()", name);
            func = module.Type->GetMethodByDecl(decl.c_str());
        }
    } else {
        func = module.Cached->module->GetFunctionByDecl(decl.c_str());
        if (!func) {
            decl = fmt::format("void {}()", name);
            func = module.Cached->module->GetFunctionByDecl(decl.c_str());
        }
    }
    if (!func) {
        return !required;
    }

    asIScriptEngine *engine = func->GetEngine();
    asIScriptContext *ctx = engine ? engine->RequestContext() : nullptr;
    if (!ctx) {
        SetModuleError(module, fmt::format("{}: failed to request AngelScript context", name));
        return false;
    }

    int r = ctx->Prepare(func);
    if (r >= 0 && module.Object) {
        r = ctx->SetObject(module.Object);
    }
    if (r >= 0 && func->GetParamCount() > 0) {
        r = ctx->SetArgObject(0, const_cast<ScriptRuntimeContext *>(&context));
    }
    if (r >= 0) {
        r = ctx->Execute();
    }
    if (r != asEXECUTION_FINISHED) {
        std::string message;
        if (r == asEXECUTION_EXCEPTION) {
            const char *section = nullptr;
            int col = 0;
            const int row = ctx->GetExceptionLineNumber(&col, &section);
            asIScriptFunction *exFunc = ctx->GetExceptionFunction();
            message = fmt::format("{} threw in {} at {}({},{}): {}",
                                  name,
                                  exFunc ? exFunc->GetDeclaration() : "<unknown>",
                                  section ? section : "<unknown>",
                                  row,
                                  col,
                                  ctx->GetExceptionString() ? ctx->GetExceptionString() : "");
        } else {
            message = fmt::format("{} failed with AngelScript result {}", name, r);
        }
        engine->ReturnContext(ctx);
        SetModuleError(module, message);
        return false;
    }
    engine->ReturnContext(ctx);
    return true;
}

void ScriptRuntime::SetModuleError(Module &module, const std::string &error) {
    module.Error = error;
    module.Failed = true;
    module.Enabled = false;
    OutputDiagnostic(fmt::format("{}: {}", module.Meta.Id, error));
}

void ScriptRuntime::OutputDiagnostic(const std::string &message) const {
    if (!message.empty() && m_Manager && m_Manager->GetCKContext()) {
        m_Manager->GetCKContext()->OutputToConsoleEx(const_cast<char *>("[AngelScript Runtime] %s"), message.c_str());
    }
}

void RegisterScriptRuntime(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;
    r = engine->RegisterObjectType("ScriptRuntimeContext",
                                   sizeof(ScriptRuntimeContext),
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptRuntimeContext", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptRuntimeContext *self) { new(self) ScriptRuntimeContext(); },
                                                     (ScriptRuntimeContext *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptRuntimeContext", asBEHAVE_CONSTRUCT, "void f(const ScriptRuntimeContext &in other)",
                                        asFUNCTIONPR([](const ScriptRuntimeContext &other, ScriptRuntimeContext *self) {
                                                         new(self) ScriptRuntimeContext(other);
                                                     },
                                                     (const ScriptRuntimeContext &, ScriptRuntimeContext *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("ScriptRuntimeContext", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptRuntimeContext *self) { self->~ScriptRuntimeContext(); },
                                                     (ScriptRuntimeContext *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "ScriptRuntimeContext &opAssign(const ScriptRuntimeContext &in other)",
                                     asMETHODPR(ScriptRuntimeContext, operator=, (const ScriptRuntimeContext &), ScriptRuntimeContext &),
                                     asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "CKContext@ Context() const", asMETHOD(ScriptRuntimeContext, Context), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "float DeltaTime() const", asMETHOD(ScriptRuntimeContext, DeltaTime), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "float TimeSeconds() const", asMETHOD(ScriptRuntimeContext, TimeSeconds), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string ScriptId() const", asMETHOD(ScriptRuntimeContext, ScriptId), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string RootPath() const", asMETHOD(ScriptRuntimeContext, RootPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "void Raise(const string &in message) const", asMETHOD(ScriptRuntimeContext, Raise), asCALL_THISCALL); assert(r >= 0);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";

    r = engine->SetDefaultNamespace("Behavior"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorBridge@ From(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeBehaviorFrom), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorGraph@ Graph(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeBehaviorGraph), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorGraph@ Graph(const ScriptRuntimeContext &in ctx, const string &in rootBehaviorName)", asFUNCTION(ScriptRuntimeInternal::RuntimeBehaviorGraphByRoot), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ Find(const ScriptRuntimeContext &in ctx, const string &in name)", asFUNCTION(ScriptRuntimeInternal::RuntimeBehaviorFind), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindByID(const ScriptRuntimeContext &in ctx, CK_ID id)", asFUNCTION(ScriptRuntimeInternal::RuntimeBehaviorFindById), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("BB"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBBridge@ From(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBFrom), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const ScriptRuntimeContext &in ctx, const string &in query)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBRequireByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const ScriptRuntimeContext &in ctx, CKGUID guid)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBRequireByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Count(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBCount), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ At(const ScriptRuntimeContext &in ctx, int index)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBAt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ Find(const ScriptRuntimeContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(ScriptRuntimeInternal::RuntimeBBFind), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<BBPrototype@>@ FindAll(const ScriptRuntimeContext &in ctx, const string &in query = \"\")", asFUNCTION(ScriptRuntimeInternal::RuntimeBBFindAll), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("Param"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int Count(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeParamCount), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ At(const ScriptRuntimeContext &in ctx, int index)", asFUNCTION(ScriptRuntimeInternal::RuntimeParamAt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Find(const ScriptRuntimeContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(ScriptRuntimeInternal::RuntimeParamFind), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const ScriptRuntimeContext &in ctx, const string &in typeName)", asFUNCTION(ScriptRuntimeInternal::RuntimeParamTypeByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const ScriptRuntimeContext &in ctx, CKGUID guid)", asFUNCTION(ScriptRuntimeInternal::RuntimeParamTypeByGuid), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("Runtime"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool ReloadAll(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeReloadAll), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Reload(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeReload), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("bool Enable(const ScriptRuntimeContext &in ctx, const string &in id, bool enabled)", asFUNCTION(ScriptRuntimeInternal::RuntimeEnable), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<string>@ List(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeList), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Runtime self-test requires a CKContext and AngelScript engine.";
        return false;
    }
    const char *source =
        "void __ckas_runtime_compile_probe(const ScriptRuntimeContext &in ctx) {\n"
        "  CKContext@ c = ctx.Context();\n"
        "  float dt = ctx.DeltaTime();\n"
        "  string id = ctx.ScriptId();\n"
        "  BehaviorGraph@ graph = Behavior::Graph(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behavior = Behavior::Find(ctx, \"__missing__\");\n"
        "  BBDecl@ decl = BB::Require(ctx, \"__missing__\");\n"
        "  int bbCount = BB::Count(ctx);\n"
        "  ParamTypeInfo@ param = Param::Find(ctx, \"int\");\n"
        "  array<string>@ scripts = Runtime::List(ctx);\n"
        "}\n";
    const std::string moduleName = "__CKAS_RuntimeCompileSelfTest";
    std::shared_ptr<CachedScript> script = ScriptManager::GetManager(context)->GetScriptCache().CompileScript(engine, moduleName, source);
    if (!script || !script->module) {
        error = "Runtime script API compile probe failed.";
        return false;
    }
    ScriptManager::GetManager(context)->GetScriptCache().UnloadScript(moduleName);
    return true;
}
