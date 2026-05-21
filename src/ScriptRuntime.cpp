#include "ScriptRuntime.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <unordered_set>

#include <fmt/format.h>

#include "CKAll.h"
#include "CKPathManager.h"
#include "CKTimeManager.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptManager.h"
#include "ScriptParameterRegistry.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"
#include "ScriptUtils.h"
#include "add_on/scriptarray/scriptarray.h"

namespace ScriptRuntimeInternal {

constexpr const char *kModulePrefix = "__CKASRuntime_";

BehaviorGraph *RuntimeBehaviorGraphByRoot(const ScriptRuntimeContext &ctx, const std::string &rootBehaviorName);

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

ScriptRuntimeContext MakeRuntimeContext(CKContext *context,
                                        const ScriptRuntimeManifest &metadata,
                                        float deltaTime,
                                        float timeSeconds) {
    return ScriptRuntimeContext(context,
                                metadata.Id,
                                metadata.Name,
                                metadata.VersionText,
                                ScriptRuntimeMetadata::PathString(metadata.RootPath),
                                metadata.CustomMetadata,
                                deltaTime,
                                timeSeconds);
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

std::string RuntimeVersion(const ScriptRuntimeContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Version(id) : std::string();
}

std::string RuntimeMetadata(const ScriptRuntimeContext &ctx,
                            const std::string &id,
                            const std::string &key,
                            const std::string &fallback) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Metadata(id, key, fallback) : fallback;
}

CScriptArray *RuntimeDependencies(const ScriptRuntimeContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl("array<string>") : nullptr;
    if (!arrayType || !manager || !manager->GetRuntime()) {
        return nullptr;
    }
    std::vector<std::string> dependencies = manager->GetRuntime()->Dependencies(id);
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(dependencies.size()));
    for (asUINT i = 0; i < static_cast<asUINT>(dependencies.size()); ++i) {
        array->SetValue(i, &dependencies[i]);
    }
    return array;
}

} // namespace ScriptRuntimeInternal

struct ScriptRuntime::Module {
    ScriptRuntimeManifest Meta;
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
                                           std::string scriptName,
                                           std::string scriptVersion,
                                           std::string rootPath,
                                           std::vector<ScriptRuntimeMetadataEntry> metadata,
                                           float deltaTime,
                                           float timeSeconds)
    : m_Context(context),
      m_ScriptId(std::move(scriptId)),
      m_ScriptName(std::move(scriptName)),
      m_ScriptVersion(std::move(scriptVersion)),
      m_RootPath(std::move(rootPath)),
      m_Metadata(std::move(metadata)),
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

std::string ScriptRuntimeContext::ScriptName() const {
    return m_ScriptName;
}

std::string ScriptRuntimeContext::ScriptVersion() const {
    return m_ScriptVersion;
}

std::string ScriptRuntimeContext::RootPath() const {
    return m_RootPath;
}

std::string ScriptRuntimeContext::Metadata(const std::string &key, const std::string &fallback) const {
    return ScriptRuntimeMetadata::MetadataValue(m_Metadata, key, fallback);
}

int ScriptRuntimeContext::MetadataCount() const {
    return static_cast<int>(m_Metadata.size());
}

std::string ScriptRuntimeContext::MetadataKey(int index) const {
    return index >= 0 && index < static_cast<int>(m_Metadata.size()) ? m_Metadata[index].Key : std::string();
}

std::string ScriptRuntimeContext::MetadataValue(int index) const {
    return index >= 0 && index < static_cast<int>(m_Metadata.size()) ? m_Metadata[index].Value : std::string();
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
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
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
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
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
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
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
    std::vector<ScriptRuntimeManifest> scripts = Discover(discoveryError);
    if (!discoveryError.empty()) {
        if (error) {
            *error = discoveryError;
        }
        OutputDiagnostic(discoveryError);
    }
    m_Scanned = true;
    return LoadDiscovered(scripts, true);
}

bool ScriptRuntime::Reload(const std::string &id, std::string *error) {
    EnsureScanned();
    std::string discoveryError;
    std::vector<ScriptRuntimeManifest> scripts = Discover(discoveryError);
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    const auto it = std::find_if(scripts.begin(), scripts.end(), [&](const ScriptRuntimeManifest &metadata) {
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

std::string ScriptRuntime::Version(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module && (module->Meta.Id == canonical || module->Meta.Id == id)) {
            return module->Meta.VersionText;
        }
    }
    return std::string();
}

std::string ScriptRuntime::Metadata(const std::string &id, const std::string &key, const std::string &fallback) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module && (module->Meta.Id == canonical || module->Meta.Id == id)) {
            return ScriptRuntimeMetadata::MetadataValue(module->Meta.CustomMetadata, key, fallback);
        }
    }
    return fallback;
}

std::vector<std::string> ScriptRuntime::Dependencies(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (!module || (module->Meta.Id != canonical && module->Meta.Id != id)) {
            continue;
        }
        std::vector<std::string> result;
        result.reserve(module->Meta.RequiredDependencies.size());
        for (const ScriptRuntimeDependency &dependency : module->Meta.RequiredDependencies) {
            result.push_back(ScriptRuntimeMetadata::VersionRequirementText(dependency));
        }
        return result;
    }
    return {};
}

void ScriptRuntime::EnsureScanned() {
    if (m_Scanned) {
        return;
    }
    std::string error;
    std::vector<ScriptRuntimeManifest> scripts = Discover(error);
    if (!error.empty()) {
        OutputDiagnostic(error);
    }
    LoadDiscovered(scripts, true);
    m_Scanned = true;
}

std::vector<ScriptRuntimeManifest> ScriptRuntime::Discover(std::string &error) const {
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
        for (const std::string &root : ScriptRuntimeMetadata::SplitList(envRoots)) {
            roots.emplace_back(root);
        }
    }

    std::vector<ScriptRuntimeManifest> candidatesOut;
    std::unordered_set<std::string> seenRoots;
    std::set<std::string> seenIds;
    for (fs::path root : roots) {
        root = fs::absolute(root) / "Scripts";
        std::error_code ec;
        root = fs::weakly_canonical(root, ec);
        if (ec) {
            root = fs::absolute(root);
        }
        const std::string rootKey = ScriptRuntimeMetadata::ToLower(ScriptRuntimeMetadata::PathString(root));
        if (!seenRoots.insert(rootKey).second || !fs::is_directory(root)) {
            continue;
        }

        std::vector<fs::path> candidates;
        for (const fs::directory_entry &entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                break;
            }
            const fs::path path = entry.path();
            if (entry.is_regular_file() && path.extension() == ".as" && !ScriptRuntimeMetadata::IsSkippedMainFile(path)) {
                candidates.push_back(path);
            } else if (entry.is_directory() && !ScriptRuntimeMetadata::IsSkippedDirectory(path)) {
                fs::path main = path / "main.as";
                if (fs::is_regular_file(main) && !ScriptRuntimeMetadata::IsSkippedMainFile(main)) {
                    candidates.push_back(main);
                }
            }
        }
        std::sort(candidates.begin(), candidates.end());

        for (const fs::path &candidate : candidates) {
            ScriptRuntimeManifest metadata;
            metadata.RootPath = candidate.parent_path();
            metadata.ManifestPath = candidate;
            metadata.EntryPath = candidate;
            metadata.Id = ScriptRuntimeMetadata::ToLower(candidate.filename().string()) == "main.as"
                              ? candidate.parent_path().filename().string()
                              : candidate.stem().string();
            metadata.Name = metadata.Id;
            std::string parseError;
            const std::string source = ScriptRuntimeMetadata::ReadTextFile(candidate);
            if (!ScriptRuntimeMetadata::ParseManifestSource(source, candidate, metadata, parseError)) {
                error += fmt::format("Skipping '{}': {}.\n", ScriptRuntimeMetadata::PathString(candidate), parseError);
                continue;
            }

            metadata.Files.push_back(metadata.EntryPath);
            for (const std::string &file : metadata.FileSpecs) {
                metadata.Files.push_back(metadata.RootPath / file);
            }
            std::vector<fs::path> uniqueFiles;
            std::set<std::string> seenFiles;
            for (fs::path file : metadata.Files) {
                file = fs::absolute(file);
                const std::string fileKey = ScriptRuntimeMetadata::ToLower(ScriptRuntimeMetadata::PathString(file));
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
                                         ScriptRuntimeMetadata::PathString(file));
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
                                     ScriptRuntimeMetadata::PathString(candidate));
                continue;
            }
            candidatesOut.push_back(std::move(metadata));
        }
    }

    std::sort(candidatesOut.begin(), candidatesOut.end(), [](const ScriptRuntimeManifest &lhs, const ScriptRuntimeManifest &rhs) {
        if (lhs.Order != rhs.Order) {
            return lhs.Order < rhs.Order;
        }
        return lhs.Id < rhs.Id;
    });
    ScriptRuntimeLoadPlan plan = ScriptRuntimeDependencyResolver::Resolve(candidatesOut);
    if (!plan.Diagnostics.empty()) {
        error += plan.Diagnostics;
    }
    return plan.Scripts;
}

bool ScriptRuntime::LoadDiscovered(const std::vector<ScriptRuntimeManifest> &scripts, bool reconcileModules) {
    if (reconcileModules) {
        RemoveModulesNotIn(scripts);
    }

    bool ok = true;
    std::vector<std::string> failedIds;
    for (const ScriptRuntimeManifest &metadata : scripts) {
        std::string dependencyError;
        if (ScriptRuntimeDependencyResolver::HasDependencyFailure(metadata, failedIds, dependencyError)) {
            ok = false;
            failedIds.push_back(metadata.Id);
            RemoveModuleById(metadata.Id);
            OutputDiagnostic(dependencyError);
            continue;
        }
        std::unique_ptr<Module> module;
        std::string error;
        if (!LoadModule(metadata, module, error)) {
            ok = false;
            failedIds.push_back(metadata.Id);
            RemoveModuleById(metadata.Id);
            OutputDiagnostic(error);
            continue;
        }
        ReplaceModule(metadata, std::move(module));
    }
    return ok;
}

bool ScriptRuntime::RemoveModuleById(const std::string &id) {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (auto it = m_Modules.begin(); it != m_Modules.end(); ++it) {
        Module *module = it->get();
        if (!module || (module->Meta.Id != canonical && module->Meta.Id != id)) {
            continue;
        }
        DestroyModule(*module);
        m_Modules.erase(it);
        return true;
    }
    return false;
}

void ScriptRuntime::RemoveModulesNotIn(const std::vector<ScriptRuntimeManifest> &scripts) {
    std::set<std::string> allowedIds;
    for (const ScriptRuntimeManifest &script : scripts) {
        allowedIds.insert(script.Id);
    }

    for (auto it = m_Modules.begin(); it != m_Modules.end();) {
        Module *module = it->get();
        if (module && allowedIds.find(module->Meta.Id) == allowedIds.end()) {
            DestroyModule(*module);
            it = m_Modules.erase(it);
        } else {
            ++it;
        }
    }
}

bool ScriptRuntime::LoadModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> &module, std::string &error) {
    asIScriptEngine *engine = m_Manager ? m_Manager->GetScriptEngine() : nullptr;
    if (!engine) {
        error = "Script runtime cannot load modules before the AngelScript engine is initialized.";
        return false;
    }

    const std::string moduleName = fmt::format("{}{}_{}", ScriptRuntimeInternal::kModulePrefix, metadata.Id, ++m_Generation);
    std::vector<std::string> files;
    files.reserve(metadata.Files.size());
    for (const std::filesystem::path &file : metadata.Files) {
        files.push_back(ScriptRuntimeMetadata::PathString(file));
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

    ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         metadata,
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

bool ScriptRuntime::ReplaceModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> module) {
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
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module.Meta,
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
    ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         module.Meta,
                                                                         0.0f,
                                                                         0.0f);
    Invoke(module, "OnDisable", ctx);
    module.EnableCalled = false;
}

void ScriptRuntime::UpdateModule(Module &module, float deltaTime, float timeSeconds) {
    if (!module.Loaded || !module.Enabled || module.Failed) {
        return;
    }
    ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         module.Meta,
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
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string ScriptName() const", asMETHOD(ScriptRuntimeContext, ScriptName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string ScriptVersion() const", asMETHOD(ScriptRuntimeContext, ScriptVersion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string RootPath() const", asMETHOD(ScriptRuntimeContext, RootPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string Metadata(const string &in key, const string &in fallback = \"\") const", asMETHOD(ScriptRuntimeContext, Metadata), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "int MetadataCount() const", asMETHOD(ScriptRuntimeContext, MetadataCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string MetadataKey(int index) const", asMETHOD(ScriptRuntimeContext, MetadataKey), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string MetadataValue(int index) const", asMETHOD(ScriptRuntimeContext, MetadataValue), asCALL_THISCALL); assert(r >= 0);
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
    r = engine->RegisterGlobalFunction("string Version(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeVersion), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Metadata(const ScriptRuntimeContext &in ctx, const string &in id, const string &in key, const string &in fallback = \"\")", asFUNCTION(ScriptRuntimeInternal::RuntimeMetadata), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<string>@ Dependencies(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeDependencies), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

#if CKAS_BUILD_SELF_TESTS
bool RunScriptRuntimeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Runtime self-test requires a CKContext and AngelScript engine.";
        return false;
    }
    if (!ScriptRuntimeMetadata::RunScriptRuntimeMetadataSelfTest(error)) {
        return false;
    }
    if (!ScriptRuntimeDependencyResolver::RunScriptRuntimeDependencySelfTest(error)) {
        return false;
    }
    {
        ScriptRuntime runtime(nullptr);
        std::unique_ptr<ScriptRuntime::Module> oldDependency = std::make_unique<ScriptRuntime::Module>();
        oldDependency->Meta.Id = "dep";
        oldDependency->Meta.Name = "dep";
        runtime.m_Modules.push_back(std::move(oldDependency));

        std::unique_ptr<ScriptRuntime::Module> oldDependent = std::make_unique<ScriptRuntime::Module>();
        oldDependent->Meta.Id = "app";
        oldDependent->Meta.Name = "app";
        runtime.m_Modules.push_back(std::move(oldDependent));

        ScriptRuntimeManifest dep;
        dep.Id = "dep";
        dep.Name = "dep";
        dep.Version = ScriptRuntimeMetadata::ParseVersion("1.0.0");
        dep.VersionText = dep.Version.Text;

        ScriptRuntimeManifest app;
        app.Id = "app";
        app.Name = "app";
        app.Version = ScriptRuntimeMetadata::ParseVersion("1.0.0");
        app.VersionText = app.Version.Text;
        ScriptRuntimeDependency dependency;
        std::string parseError;
        if (!ScriptRuntimeMetadata::ParseDependencySpec("dep", dependency, parseError)) {
            error = parseError;
            return false;
        }
        app.RequiredDependencies.push_back(dependency);

        if (runtime.LoadDiscovered({dep, app}, true)) {
            error = "Runtime reconciliation self-test expected load failure without a script manager.";
            return false;
        }
        if (!runtime.m_Modules.empty()) {
            error = "Runtime reconciliation self-test left stale modules after dependency load failure.";
            return false;
        }
    }
    const char *source =
        "void __ckas_runtime_compile_probe(const ScriptRuntimeContext &in ctx) {\n"
        "  CKContext@ c = ctx.Context();\n"
        "  float dt = ctx.DeltaTime();\n"
        "  string id = ctx.ScriptId();\n"
        "  string name = ctx.ScriptName();\n"
        "  string version = ctx.ScriptVersion();\n"
        "  string custom = ctx.Metadata(\"custom\", \"fallback\");\n"
        "  int metadataCount = ctx.MetadataCount();\n"
        "  string metadataKey = ctx.MetadataKey(0);\n"
        "  string metadataValue = ctx.MetadataValue(0);\n"
        "  BehaviorGraph@ graph = Behavior::Graph(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behavior = Behavior::Find(ctx, \"__missing__\");\n"
        "  BBDecl@ decl = BB::Require(ctx, \"__missing__\");\n"
        "  int bbCount = BB::Count(ctx);\n"
        "  ParamTypeInfo@ param = Param::Find(ctx, \"int\");\n"
        "  array<string>@ scripts = Runtime::List(ctx);\n"
        "  string runtimeVersion = Runtime::Version(ctx, \"ckas.runtime.smoke\");\n"
        "  string runtimeMetadata = Runtime::Metadata(ctx, \"ckas.runtime.smoke\", \"custom\", \"fallback\");\n"
        "  array<string>@ deps = Runtime::Dependencies(ctx, \"ckas.runtime.smoke\");\n"
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
#endif
