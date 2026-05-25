#include "ScriptRuntime.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <unordered_set>
#include <utility>

#include <fmt/format.h>

#include "CKAll.h"
#include "CKPathManager.h"
#include "CKTimeManager.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"
#include "ScriptMessage.h"
#include "ScriptParameterRegistry.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"
#include "ScriptUtils.h"
#include "add_on/scriptarray/scriptarray.h"

namespace ScriptRuntimeInternal {

constexpr const char *kModulePrefix = "__CKASRuntime_";

constexpr const char *kLifecycleNames[] = {
    "OnLoad",
    "Awake",
    "OnEnable",
    "Start",
    "Update",
    "OnPostLoad",
    "OnPostProcess",
    "OnDisable",
    "OnDestroy",
    "OnReset",
    "OnPause",
    "OnResume",
};

bool RuntimeValidateOnly() {
    const char *value = std::getenv("CKAS_RUNTIME_VALIDATE_ONLY");
    if (!value || value[0] == '\0') {
        return false;
    }
    const std::string lowered = ScriptRuntimeMetadata::ToLower(value);
    return lowered != "0" && lowered != "false" && lowered != "off" && lowered != "no";
}

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
                                        const std::string &phase,
                                        const std::string &state,
                                        int generation,
                                        std::uint64_t frameIndex,
                                        float deltaTime,
                                        float timeSeconds) {
    return ScriptRuntimeContext(context,
                                metadata.Id,
                                metadata.Name,
                                metadata.VersionText,
                                ScriptRuntimeMetadata::PathString(metadata.RootPath),
                                ScriptRuntimeMetadata::PathString(metadata.ManifestPath),
                                ScriptRuntimeMetadata::PathString(metadata.EntryPath),
                                metadata.CustomMetadata,
                                phase,
                                state,
                                generation,
                                frameIndex,
                                deltaTime,
                                timeSeconds);
}

ScriptRuntimeContext MakeRuntimeContext(CKContext *context,
                                        const ScriptRuntimeManifest &metadata,
                                        float deltaTime,
                                        float timeSeconds) {
    return MakeRuntimeContext(context, metadata, std::string(), std::string(), 0, 0, deltaTime, timeSeconds);
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

CScriptArray *RuntimeListInfo(const ScriptRuntimeContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl("array<RuntimeScriptInfo>") : nullptr;
    if (!arrayType || !manager || !manager->GetRuntime()) {
        return nullptr;
    }
    std::vector<RuntimeScriptInfo> infos = manager->GetRuntime()->ListInfo();
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(infos.size()));
    for (asUINT i = 0; i < static_cast<asUINT>(infos.size()); ++i) {
        array->SetValue(i, &infos[i]);
    }
    return array;
}

RuntimeScriptInfo RuntimeInfo(const ScriptRuntimeContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Info(id) : RuntimeScriptInfo();
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

CScriptArray *RuntimeDependencyInfos(const ScriptRuntimeContext &ctx, const std::string &id, bool optional) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    asIScriptEngine *engine = manager ? manager->GetScriptEngine() : nullptr;
    asITypeInfo *arrayType = engine ? engine->GetTypeInfoByDecl("array<RuntimeDependencyInfo>") : nullptr;
    if (!arrayType || !manager || !manager->GetRuntime()) {
        return nullptr;
    }
    std::vector<RuntimeDependencyInfo> infos = optional
        ? manager->GetRuntime()->OptionalDependencies(id)
        : manager->GetRuntime()->RequiredDependencies(id);
    CScriptArray *array = CScriptArray::Create(arrayType, static_cast<asUINT>(infos.size()));
    for (asUINT i = 0; i < static_cast<asUINT>(infos.size()); ++i) {
        array->SetValue(i, &infos[i]);
    }
    return array;
}

CScriptArray *RuntimeRequiredDependencies(const ScriptRuntimeContext &ctx, const std::string &id) {
    return RuntimeDependencyInfos(ctx, id, false);
}

CScriptArray *RuntimeOptionalDependencies(const ScriptRuntimeContext &ctx, const std::string &id) {
    return RuntimeDependencyInfos(ctx, id, true);
}

} // namespace ScriptRuntimeInternal

struct ScriptRuntime::Module {
    ScriptRuntimeManifest Meta;
    std::string ModuleName;
    int Generation = 0;
    std::shared_ptr<CachedScript> Cached;
    asIScriptObject *Object = nullptr;
    asITypeInfo *Type = nullptr;
    bool Enabled = true;
    bool Loaded = false;
    bool LoadCalled = false;
    bool AwakeCalled = false;
    bool EnableCalled = false;
    bool StartCalled = false;
    bool Failed = false;
    std::string Error;
    asIScriptContext *ActiveContext = nullptr;
    std::string ActiveInvocation;
    ScriptRuntimeContext ContextStorage;
    ScriptMessage MessageStorage;
    bool PendingDisable = false;
    bool PendingDestroy = false;
    bool PendingErase = false;
    bool PendingPause = false;
    bool PendingReset = false;
    std::unique_ptr<Module> PendingReplacement;
};

RuntimeDependencyInfo::RuntimeDependencyInfo() = default;

RuntimeDependencyInfo::RuntimeDependencyInfo(ScriptRuntimeDependency dependency,
                                             bool optional,
                                             bool present,
                                             bool satisfied,
                                             std::string actualVersion)
    : m_Dependency(std::move(dependency)),
      m_Optional(optional),
      m_Present(present),
      m_Satisfied(satisfied),
      m_ActualVersion(std::move(actualVersion)) {}

std::string RuntimeDependencyInfo::Raw() const {
    return m_Dependency.Raw;
}

std::string RuntimeDependencyInfo::Id() const {
    return m_Dependency.Id;
}

std::string RuntimeDependencyInfo::Operator() const {
    switch (m_Dependency.Op) {
    case ScriptRuntimeVersionOp::Equal:
        return "==";
    case ScriptRuntimeVersionOp::Greater:
        return ">";
    case ScriptRuntimeVersionOp::GreaterEqual:
        return ">=";
    case ScriptRuntimeVersionOp::Less:
        return "<";
    case ScriptRuntimeVersionOp::LessEqual:
        return "<=";
    case ScriptRuntimeVersionOp::Any:
    default:
        return "";
    }
}

std::string RuntimeDependencyInfo::Version() const {
    return m_Dependency.Version.Text;
}

std::string RuntimeDependencyInfo::ActualVersion() const {
    return m_ActualVersion;
}

bool RuntimeDependencyInfo::Optional() const {
    return m_Optional;
}

bool RuntimeDependencyInfo::Present() const {
    return m_Present;
}

bool RuntimeDependencyInfo::Satisfied() const {
    return m_Satisfied;
}

RuntimeScriptInfo::RuntimeScriptInfo() = default;

bool RuntimeScriptInfo::Exists() const {
    return m_Exists;
}

std::string RuntimeScriptInfo::Id() const {
    return m_Id;
}

std::string RuntimeScriptInfo::Name() const {
    return m_Name;
}

std::string RuntimeScriptInfo::Version() const {
    return m_Version;
}

std::string RuntimeScriptInfo::Description() const {
    return m_Description;
}

std::string RuntimeScriptInfo::Author() const {
    return m_Author;
}

std::string RuntimeScriptInfo::Category() const {
    return m_Category;
}

int RuntimeScriptInfo::TagCount() const {
    return static_cast<int>(m_Tags.size());
}

std::string RuntimeScriptInfo::Tag(int index) const {
    return index >= 0 && index < static_cast<int>(m_Tags.size()) ? m_Tags[index] : std::string();
}

bool RuntimeScriptInfo::Enabled() const {
    return m_Enabled;
}

bool RuntimeScriptInfo::Loaded() const {
    return m_Loaded;
}

bool RuntimeScriptInfo::Failed() const {
    return m_Failed;
}

std::string RuntimeScriptInfo::State() const {
    return m_State;
}

std::string RuntimeScriptInfo::Phase() const {
    return m_Phase;
}

std::string RuntimeScriptInfo::Error() const {
    return m_Error;
}

std::string RuntimeScriptInfo::RootPath() const {
    return m_RootPath;
}

std::string RuntimeScriptInfo::ManifestPath() const {
    return m_ManifestPath;
}

std::string RuntimeScriptInfo::EntryPath() const {
    return m_EntryPath;
}

int RuntimeScriptInfo::Generation() const {
    return m_Generation;
}

ScriptRuntimeContext::ScriptRuntimeContext() = default;

ScriptRuntimeContext::ScriptRuntimeContext(CKContext *context,
                                           std::string scriptId,
                                           std::string scriptName,
                                           std::string scriptVersion,
                                           std::string rootPath,
                                           std::string manifestPath,
                                           std::string entryPath,
                                           std::vector<ScriptRuntimeMetadataEntry> metadata,
                                           std::string phase,
                                           std::string state,
                                           int generation,
                                           std::uint64_t frameIndex,
                                           float deltaTime,
                                           float timeSeconds)
    : m_Context(context),
      m_ScriptId(std::move(scriptId)),
      m_ScriptName(std::move(scriptName)),
      m_ScriptVersion(std::move(scriptVersion)),
      m_RootPath(std::move(rootPath)),
      m_ManifestPath(std::move(manifestPath)),
      m_EntryPath(std::move(entryPath)),
      m_Metadata(std::move(metadata)),
      m_Phase(std::move(phase)),
      m_State(std::move(state)),
      m_Generation(generation),
      m_FrameIndex(frameIndex),
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

std::string ScriptRuntimeContext::ManifestPath() const {
    return m_ManifestPath;
}

std::string ScriptRuntimeContext::EntryPath() const {
    return m_EntryPath;
}

std::string ScriptRuntimeContext::Phase() const {
    return m_Phase;
}

std::string ScriptRuntimeContext::State() const {
    return m_State;
}

int ScriptRuntimeContext::Generation() const {
    return m_Generation;
}

std::uint64_t ScriptRuntimeContext::FrameIndex() const {
    return m_FrameIndex;
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
    if (ScriptRuntimeInternal::RuntimeValidateOnly()) {
        FinalizePendingModules();
        return;
    }
    ++m_FrameIndex;

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKTimeManager *time = context ? context->GetTimeManager() : nullptr;
    const float deltaTime = time ? time->GetLastDeltaTime() : 0.0f;
    const float timeSeconds = time ? (time->GetTime() * 0.001f) : 0.0f;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            if (m_Paused &&
                !module->PendingDisable &&
                !module->PendingDestroy &&
                !module->PendingPause &&
                !module->PendingReset) {
                continue;
            }
            UpdateModule(*module, deltaTime, timeSeconds);
        }
    }
    FinalizePendingModules();
}

void ScriptRuntime::PostProcess() {
    EnsureScanned();
    if (ScriptRuntimeInternal::RuntimeValidateOnly()) {
        FinalizePendingModules();
        return;
    }

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKTimeManager *time = context ? context->GetTimeManager() : nullptr;
    const float deltaTime = time ? time->GetLastDeltaTime() : 0.0f;
    const float timeSeconds = time ? (time->GetTime() * 0.001f) : 0.0f;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed || !module->Enabled || !module->EnableCalled) {
            continue;
        }
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(context,
                                                                             module->Meta,
                                                                             "OnPostProcess",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             deltaTime,
                                                                             timeSeconds);
        Invoke(*module, "OnPostProcess", ctx);
    }
    FinalizePendingModules();
}

void ScriptRuntime::PostLoad() {
    EnsureScanned();
    if (ScriptRuntimeInternal::RuntimeValidateOnly()) {
        FinalizePendingModules();
        return;
    }
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(context,
                                                                             module->Meta,
                                                                             "OnPostLoad",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        Invoke(*module, "OnPostLoad", ctx);
    }
    FinalizePendingModules();
}

void ScriptRuntime::OnReset() {
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
                                                                             "OnReset",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        ResetModule(*module, ctx);
    }
}

void ScriptRuntime::OnPause() {
    if (m_Paused) {
        return;
    }
    m_Paused = true;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
                                                                             "OnPause",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        PauseModule(*module, ctx);
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
                                                                             "OnResume",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        if (module->PendingPause || module->PendingDisable || module->PendingDestroy || module->PendingReset) {
            continue;
        }
        InvokeFinished(*module, "OnResume", ctx);
        if (!module->EnableCalled) {
            ScriptRuntimeContext enableCtx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                                       module->Meta,
                                                                                       "OnEnable",
                                                                                       ModuleState(*module),
                                                                                       module->Generation,
                                                                                       m_FrameIndex,
                                                                                       0.0f,
                                                                                       0.0f);
            if (InvokeFinished(*module, "OnEnable", enableCtx)) {
                module->EnableCalled = true;
            }
        }
    }
}

void ScriptRuntime::OnEnd() {
    Clear();
}

void ScriptRuntime::Clear() {
    if (m_Manager && m_Manager->GetMessageBus()) {
        for (const std::unique_ptr<Module> &module : m_Modules) {
            if (module) {
                m_Manager->GetMessageBus()->ClearTarget(ScriptMessageBus::RuntimeTarget(module->Meta.Id), "Runtime was cleared.");
            }
        }
    }
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            DestroyModule(*module, true);
        }
    }
    m_Modules.clear();
    m_Scanned = false;
    m_Paused = false;
}

bool ScriptRuntime::ReloadAll(std::string *error) {
    std::string discoveryError;
    ScriptRuntimeLoadPlan plan = Discover(discoveryError);
    if (!discoveryError.empty()) {
        if (error) {
            *error = discoveryError;
        }
        OutputDiagnostic(discoveryError);
    }
    m_Scanned = true;
    return LoadDiscovered(plan, true);
}

bool ScriptRuntime::Reload(const std::string &id, std::string *error) {
    EnsureScanned();
    std::string discoveryError;
    ScriptRuntimeLoadPlan plan = Discover(discoveryError);
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    const auto skippedIt = std::find_if(plan.SkippedScripts.begin(), plan.SkippedScripts.end(), [&](const ScriptRuntimeSkippedScript &skipped) {
        return skipped.Manifest.Id == canonical || skipped.Manifest.Id == id;
    });
    std::vector<std::string> failedIds;
    failedIds.reserve(plan.SkippedScripts.size());
    for (const ScriptRuntimeSkippedScript &skipped : plan.SkippedScripts) {
        failedIds.push_back(skipped.Manifest.Id);
    }
    if (skippedIt != plan.SkippedScripts.end()) {
        ReplaceWithFailedModule(skippedIt->Manifest, skippedIt->Error);
        if (error) {
            *error = skippedIt->Error;
        }
        return false;
    }
    const auto it = std::find_if(plan.Scripts.begin(), plan.Scripts.end(), [&](const ScriptRuntimeManifest &metadata) {
        return metadata.Id == canonical || metadata.Id == id;
    });
    if (it == plan.Scripts.end()) {
        if (error) {
            *error = fmt::format("Runtime script '{}' was not found during DATA_PATH Scripts scan.", id);
        }
        return false;
    }
    std::string dependencyError;
    if (ScriptRuntimeDependencyResolver::HasDependencyFailure(*it, failedIds, dependencyError)) {
        ReplaceWithFailedModule(*it, dependencyError);
        if (error) {
            *error = dependencyError;
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

std::vector<RuntimeScriptInfo> ScriptRuntime::ListInfo() const {
    std::vector<RuntimeScriptInfo> result;
    result.reserve(m_Modules.size());
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            result.push_back(BuildInfo(*module));
        }
    }
    return result;
}

RuntimeScriptInfo ScriptRuntime::Info(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module && (module->Meta.Id == canonical || module->Meta.Id == id)) {
            return BuildInfo(*module);
        }
    }
    return RuntimeScriptInfo();
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

std::vector<RuntimeDependencyInfo> ScriptRuntime::RequiredDependencies(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module && (module->Meta.Id == canonical || module->Meta.Id == id)) {
            return DependencyInfo(*module, false);
        }
    }
    return {};
}

std::vector<RuntimeDependencyInfo> ScriptRuntime::OptionalDependencies(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module && (module->Meta.Id == canonical || module->Meta.Id == id)) {
            return DependencyInfo(*module, true);
        }
    }
    return {};
}

bool ScriptRuntime::DeliverMessage(const std::string &id, const ScriptMessage &message, bool immediate, std::string &error) {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || (module->Meta.Id != canonical && module->Meta.Id != id)) {
            continue;
        }
        if (!module->Loaded || module->Failed || !module->Enabled || !module->EnableCalled) {
            error = "Runtime message target is not ready.";
            return false;
        }
        if (module->ActiveContext) {
            error = "Runtime message target is busy.";
            return false;
        }
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
                                                                             "OnMessage",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        const InvokeStatus status = InvokeMessage(*module, message, ctx);
        if (status == InvokeStatus::Suspended) {
            return true;
        }
        if (status == InvokeStatus::Failed) {
            error = module->Error.empty() ? "Runtime message handler failed." : module->Error;
            return false;
        }
        return true;
    }
    error = "Runtime message target was not found.";
    return false;
}

void ScriptRuntime::EnsureScanned() {
    if (m_Scanned) {
        return;
    }
    std::string error;
    ScriptRuntimeLoadPlan plan = Discover(error);
    if (!error.empty()) {
        OutputDiagnostic(error);
    }
    LoadDiscovered(plan, true);
    m_Scanned = true;
}

ScriptRuntimeLoadPlan ScriptRuntime::Discover(std::string &error) const {
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
                fs::path manifest = path / "script.as";
                if (fs::is_regular_file(manifest) && !ScriptRuntimeMetadata::IsSkippedMainFile(manifest)) {
                    candidates.push_back(manifest);
                }
            }
        }
        std::sort(candidates.begin(), candidates.end());

        for (const fs::path &candidate : candidates) {
            ScriptRuntimeManifest metadata;
            metadata.RootPath = candidate.parent_path();
            metadata.ManifestPath = candidate;
            metadata.EntryPath = candidate;
            metadata.Id = ScriptRuntimeMetadata::ToLower(candidate.filename().string()) == "script.as"
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
    return plan;
}

bool ScriptRuntime::LoadDiscovered(const ScriptRuntimeLoadPlan &plan, bool reconcileModules) {
    if (reconcileModules) {
        std::vector<ScriptRuntimeManifest> allScripts = plan.Scripts;
        allScripts.reserve(plan.Scripts.size() + plan.SkippedScripts.size());
        for (const ScriptRuntimeSkippedScript &skipped : plan.SkippedScripts) {
            allScripts.push_back(skipped.Manifest);
        }
        RemoveModulesNotIn(allScripts);
    }

    bool ok = true;
    std::vector<std::string> failedIds;
    for (const ScriptRuntimeSkippedScript &skipped : plan.SkippedScripts) {
        ok = false;
        failedIds.push_back(skipped.Manifest.Id);
        ReplaceWithFailedModule(skipped.Manifest, skipped.Error);
        OutputDiagnostic(skipped.Error);
    }
    for (const ScriptRuntimeManifest &metadata : plan.Scripts) {
        std::string dependencyError;
        if (ScriptRuntimeDependencyResolver::HasDependencyFailure(metadata, failedIds, dependencyError)) {
            ok = false;
            failedIds.push_back(metadata.Id);
            ReplaceWithFailedModule(metadata, dependencyError);
            OutputDiagnostic(dependencyError);
            continue;
        }
        std::unique_ptr<Module> module;
        std::string error;
        if (!LoadModule(metadata, module, error)) {
            ok = false;
            failedIds.push_back(metadata.Id);
            ReplaceWithFailedModule(metadata, error);
            OutputDiagnostic(error);
            continue;
        }
        ReplaceModule(metadata, std::move(module));
    }
    return ok;
}

bool ScriptRuntime::ReplaceWithFailedModule(const ScriptRuntimeManifest &metadata, const std::string &error) {
    std::unique_ptr<Module> failed = std::make_unique<Module>();
    failed->Meta = metadata;
    failed->Enabled = metadata.Enabled;
    failed->Loaded = false;
    failed->Failed = true;
    failed->Error = error;
    return ReplaceModule(metadata, std::move(failed));
}

bool ScriptRuntime::RemoveModuleById(const std::string &id) {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    for (auto it = m_Modules.begin(); it != m_Modules.end(); ++it) {
        Module *module = it->get();
        if (!module || (module->Meta.Id != canonical && module->Meta.Id != id)) {
            continue;
        }
        if (DestroyModule(*module)) {
            m_Modules.erase(it);
        } else {
            module->PendingErase = true;
        }
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
            if (DestroyModule(*module)) {
                it = m_Modules.erase(it);
            } else {
                module->PendingErase = true;
                ++it;
            }
        } else {
            ++it;
        }
    }
}

bool ScriptRuntime::LoadModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> &module, std::string &error) {
    if (!m_Manager || !m_Manager->GetScriptEngine()) {
        error = "Script runtime cannot load modules before the AngelScript engine is initialized.";
        return false;
    }

    const int generation = ++m_Generation;
    const std::string moduleName = fmt::format("{}{}_{}", ScriptRuntimeInternal::kModulePrefix, metadata.Id, generation);
    std::vector<std::string> files;
    files.reserve(metadata.Files.size());
    for (const std::filesystem::path &file : metadata.Files) {
        files.push_back(ScriptRuntimeMetadata::PathString(file));
    }
    std::vector<const char *> filePtrs;
    filePtrs.reserve(files.size());
    for (const std::string &file : files) {
        filePtrs.push_back(file.c_str());
    }
    AngelScriptLoadOptions options = {};
    options.ModuleName = moduleName.c_str();
    options.Filenames = filePtrs.empty() ? nullptr : filePtrs.data();
    options.FileCount = filePtrs.size();
    options.ReplaceExisting = true;
    AngelScriptResult loadResult = {};
    if (m_Manager->LoadModule(options, &loadResult) != ANGELSCRIPT_STATUS_OK) {
        error = loadResult.ErrorMessage && loadResult.ErrorMessage[0] != '\0'
            ? loadResult.ErrorMessage
            : fmt::format("Runtime script '{}' failed to compile.", metadata.Id);
        return false;
    }
    std::shared_ptr<CachedScript> cached = m_Manager->GetCachedScript(moduleName.c_str());
    if (!cached || !cached->module) {
        error = fmt::format("Runtime script '{}' failed to compile.", metadata.Id);
        m_Manager->UnloadModule(moduleName.c_str(), nullptr);
        return false;
    }

    std::unique_ptr<Module> loaded = std::make_unique<Module>();
    loaded->Meta = metadata;
    loaded->ModuleName = moduleName;
    loaded->Generation = generation;
    loaded->Cached = cached;
    loaded->Enabled = metadata.Enabled;
    loaded->Loaded = true;

    if (!metadata.ClassName.empty()) {
        loaded->Type = cached->module->GetTypeInfoByName(metadata.ClassName.c_str());
        if (!loaded->Type) {
            error = fmt::format("Runtime script '{}' class '{}' was not found.", metadata.Id, metadata.ClassName);
            m_Manager->UnloadModule(moduleName.c_str(), nullptr);
            return false;
        }
        void *object = m_Manager->GetScriptEngine()->CreateScriptObject(loaded->Type);
        if (!object) {
            error = fmt::format("Runtime script '{}' failed to create class '{}'.", metadata.Id, metadata.ClassName);
            m_Manager->UnloadModule(moduleName.c_str(), nullptr);
            return false;
        }
        loaded->Object = static_cast<asIScriptObject *>(object);
    }

    if (!ValidateLifecycleSignatures(*loaded, error)) {
        if (loaded->Object) {
            loaded->Object->Release();
            loaded->Object = nullptr;
        }
        m_Manager->UnloadModule(moduleName.c_str(), nullptr);
        return false;
    }

    if (ScriptRuntimeInternal::RuntimeValidateOnly()) {
        module = std::move(loaded);
        return true;
    }

    ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         metadata,
                                                                         "OnLoad",
                                                                         "loading",
                                                                         loaded->Generation,
                                                                         m_FrameIndex,
                                                                         0.0f,
                                                                         0.0f);
    InvokeStatus status = Invoke(*loaded, "OnLoad", ctx);
    if (status == InvokeStatus::Failed) {
        error = fmt::format("Runtime script '{}' failed during load: {}", metadata.Id, loaded->Error);
        DestroyModule(*loaded, true);
        return false;
    }
    if (status == InvokeStatus::Finished) {
        loaded->LoadCalled = true;
        ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                        metadata,
                                                        "Awake",
                                                        "loading",
                                                        loaded->Generation,
                                                        m_FrameIndex,
                                                        0.0f,
                                                        0.0f);
        status = Invoke(*loaded, "Awake", ctx);
        if (status == InvokeStatus::Failed) {
            error = fmt::format("Runtime script '{}' failed during load: {}", metadata.Id, loaded->Error);
            DestroyModule(*loaded, true);
            return false;
        }
        if (status == InvokeStatus::Finished) {
            loaded->AwakeCalled = true;
        }
    }
    module = std::move(loaded);
    if (m_Manager && m_Manager->GetMessageBus()) {
        std::string subscribeError;
        const std::string target = ScriptMessageBus::RuntimeTarget(metadata.Id);
        for (const std::string &topic : metadata.MessageTopics) {
            m_Manager->GetMessageBus()->Subscribe(target, topic, true, subscribeError);
        }
    }
    return true;
}

bool ScriptRuntime::ValidateLifecycleSignatures(const Module &module, std::string &error) const {
    asIScriptModule *scriptModule = module.Cached ? module.Cached->module : nullptr;
    if (!scriptModule) {
        return true;
    }
    for (const char *name : ScriptRuntimeInternal::kLifecycleNames) {
        const std::string expected = fmt::format("void {}(const ScriptRuntimeContext &in ctx)", name);
        asIScriptFunction *expectedFunc = module.Type
            ? module.Type->GetMethodByDecl(expected.c_str())
            : scriptModule->GetFunctionByDecl(expected.c_str());
        if (module.Type) {
            const asUINT count = module.Type->GetMethodCount();
            for (asUINT i = 0; i < count; ++i) {
                asIScriptFunction *method = module.Type->GetMethodByIndex(i);
                if (method && std::string(method->GetName()) == name && method != expectedFunc) {
                    error = fmt::format("Runtime script '{}' lifecycle '{}' has invalid signature '{}'; expected '{}'.",
                                        module.Meta.Id,
                                        name,
                                        method->GetDeclaration(),
                                        expected);
                    return false;
                }
            }
        } else {
            const asUINT count = scriptModule->GetFunctionCount();
            for (asUINT i = 0; i < count; ++i) {
                asIScriptFunction *function = scriptModule->GetFunctionByIndex(i);
                if (function && std::string(function->GetName()) == name && function != expectedFunc) {
                    error = fmt::format("Runtime script '{}' lifecycle '{}' has invalid signature '{}'; expected '{}'.",
                                        module.Meta.Id,
                                        name,
                                        function->GetDeclaration(),
                                        expected);
                    return false;
                }
            }
        }
    }
    const std::string expectedMessage = "void OnMessage(const ScriptMessage &in msg, const ScriptRuntimeContext &in ctx)";
    asIScriptFunction *expectedMessageFunc = module.Type
        ? module.Type->GetMethodByDecl(expectedMessage.c_str())
        : scriptModule->GetFunctionByDecl(expectedMessage.c_str());
    if (module.Type) {
        const asUINT count = module.Type->GetMethodCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *method = module.Type->GetMethodByIndex(i);
            if (method && std::string(method->GetName()) == "OnMessage" && method != expectedMessageFunc) {
                error = fmt::format("Runtime script '{}' message handler has invalid signature '{}'; expected '{}'.",
                                    module.Meta.Id,
                                    method->GetDeclaration(),
                                    expectedMessage);
                return false;
            }
        }
    } else {
        const asUINT count = scriptModule->GetFunctionCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *function = scriptModule->GetFunctionByIndex(i);
            if (function && std::string(function->GetName()) == "OnMessage" && function != expectedMessageFunc) {
                error = fmt::format("Runtime script '{}' message handler has invalid signature '{}'; expected '{}'.",
                                    module.Meta.Id,
                                    function->GetDeclaration(),
                                    expectedMessage);
                return false;
            }
        }
    }
    return true;
}

bool ScriptRuntime::ReplaceModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> module) {
    for (std::unique_ptr<Module> &existing : m_Modules) {
        if (existing && existing->Meta.Id == metadata.Id) {
            if (DestroyModule(*existing)) {
                existing = std::move(module);
            } else {
                existing->PendingReplacement = std::move(module);
            }
            return true;
        }
    }
    m_Modules.push_back(std::move(module));
    return true;
}

bool ScriptRuntime::DestroyModule(Module &module, bool hard) {
    if (m_Manager && m_Manager->GetMessageBus()) {
        m_Manager->GetMessageBus()->ClearTarget(ScriptMessageBus::RuntimeTarget(module.Meta.Id), "Runtime script was destroyed.");
    }
    if (hard) {
        CancelActiveInvocation(module);
    }
    module.PendingDestroy = !hard;
    if (module.Loaded && !module.Failed) {
        ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module.Meta,
                                                                             "OnDisable",
                                                                             ModuleState(module),
                                                                             module.Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        if (module.EnableCalled) {
            const InvokeStatus status = Invoke(module, "OnDisable", ctx);
            if (status == InvokeStatus::Suspended) {
                if (!hard) {
                    return false;
                }
                CancelActiveInvocation(module);
            }
            module.EnableCalled = false;
        }
        ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                        module.Meta,
                                                        "OnDestroy",
                                                        ModuleState(module),
                                                        module.Generation,
                                                        m_FrameIndex,
                                                        0.0f,
                                                        0.0f);
        const InvokeStatus status = Invoke(module, "OnDestroy", ctx);
        if (status == InvokeStatus::Suspended) {
            if (!hard) {
                return false;
            }
            CancelActiveInvocation(module);
        }
    }
    if (module.Object) {
        module.Object->Release();
        module.Object = nullptr;
    }
    if (m_Manager && !module.ModuleName.empty()) {
        m_Manager->UnloadModule(module.ModuleName.c_str(), nullptr);
    }
    module.Cached.reset();
    module.Loaded = false;
    module.PendingDestroy = false;
    module.PendingDisable = false;
    module.PendingPause = false;
    module.PendingReset = false;
    return true;
}

bool ScriptRuntime::DisableModule(Module &module) {
    if (m_Manager && m_Manager->GetMessageBus()) {
        const std::string target = ScriptMessageBus::RuntimeTarget(module.Meta.Id);
        m_Manager->GetMessageBus()->ClearDynamicSubscriptions(target);
        m_Manager->GetMessageBus()->FailPendingForTarget(target, "Runtime script was disabled.");
    }
    if (!module.PendingDisable) {
        CancelActiveInvocation(module);
    }
    module.PendingDisable = true;
    if (!module.EnableCalled) {
        module.PendingDisable = false;
        return true;
    }
    ScriptRuntimeContext ctx = ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         module.Meta,
                                                                         "OnDisable",
                                                                         ModuleState(module),
                                                                         module.Generation,
                                                                         m_FrameIndex,
                                                                         0.0f,
                                                                         0.0f);
    const InvokeStatus status = Invoke(module, "OnDisable", ctx);
    if (status == InvokeStatus::Suspended) {
        return false;
    }
    module.EnableCalled = false;
    module.PendingDisable = false;
    return true;
}

bool ScriptRuntime::PauseModule(Module &module, const ScriptRuntimeContext &context) {
    if (!module.PendingPause) {
        CancelActiveInvocation(module);
    }
    module.PendingPause = true;
    if (!module.Enabled) {
        module.PendingPause = false;
        return true;
    }
    InvokeStatus status = Invoke(module, "OnPause", context);
    if (status == InvokeStatus::Suspended) {
        return false;
    }
    if (status == InvokeStatus::Failed) {
        module.PendingPause = false;
        return true;
    }
    if (module.EnableCalled) {
        status = Invoke(module, "OnDisable", context);
        if (status == InvokeStatus::Suspended) {
            return false;
        }
        module.EnableCalled = false;
    }
    module.PendingPause = false;
    return true;
}

bool ScriptRuntime::ResetModule(Module &module, const ScriptRuntimeContext &context) {
    if (!module.PendingReset) {
        CancelActiveInvocation(module);
    }
    module.PendingReset = true;
    const InvokeStatus status = Invoke(module, "OnReset", context);
    if (status == InvokeStatus::Suspended) {
        return false;
    }
    module.StartCalled = false;
    module.PendingReset = false;
    return true;
}

std::string ScriptRuntime::ModuleState(const Module &module) const {
    if (module.Failed) {
        return "failed";
    }
    if (module.PendingDestroy) {
        return "destroying";
    }
    if (module.PendingDisable) {
        return "disabling";
    }
    if (module.PendingPause || m_Paused) {
        return "paused";
    }
    if (module.PendingReset) {
        return "resetting";
    }
    if (module.ActiveContext) {
        return "suspended";
    }
    if (!module.Loaded) {
        return "unloaded";
    }
    if (!module.Enabled) {
        return "disabled";
    }
    if (module.EnableCalled) {
        return "enabled";
    }
    return "loaded";
}

RuntimeScriptInfo ScriptRuntime::BuildInfo(const Module &module) const {
    RuntimeScriptInfo info;
    info.m_Exists = true;
    info.m_Id = module.Meta.Id;
    info.m_Name = module.Meta.Name;
    info.m_Version = module.Meta.VersionText;
    info.m_Description = module.Meta.Description;
    info.m_Author = module.Meta.Author;
    info.m_Category = module.Meta.Category;
    info.m_Tags = module.Meta.Tags;
    info.m_Enabled = module.Enabled;
    info.m_Loaded = module.Loaded;
    info.m_Failed = module.Failed;
    info.m_State = ModuleState(module);
    info.m_Phase = module.ActiveInvocation;
    info.m_Error = module.Error;
    info.m_RootPath = ScriptRuntimeMetadata::PathString(module.Meta.RootPath);
    info.m_ManifestPath = ScriptRuntimeMetadata::PathString(module.Meta.ManifestPath);
    info.m_EntryPath = ScriptRuntimeMetadata::PathString(module.Meta.EntryPath);
    info.m_Generation = module.Generation;
    return info;
}

std::vector<RuntimeDependencyInfo> ScriptRuntime::DependencyInfo(const Module &module, bool optional) const {
    const std::vector<ScriptRuntimeDependency> &dependencies = optional
        ? module.Meta.OptionalDependencies
        : module.Meta.RequiredDependencies;
    std::vector<RuntimeDependencyInfo> result;
    result.reserve(dependencies.size());
    for (const ScriptRuntimeDependency &dependency : dependencies) {
        bool present = false;
        bool satisfied = false;
        std::string actualVersion;
        for (const std::unique_ptr<Module> &candidate : m_Modules) {
            if (!candidate || candidate->Meta.Id != dependency.Id) {
                continue;
            }
            present = true;
            actualVersion = candidate->Meta.VersionText;
            satisfied = ScriptRuntimeMetadata::SatisfiesVersion(candidate->Meta.Version, dependency);
            break;
        }
        result.emplace_back(dependency, optional, present, satisfied, actualVersion);
    }
    return result;
}

void ScriptRuntime::FinalizePendingModules() {
    for (auto it = m_Modules.begin(); it != m_Modules.end();) {
        Module *module = it->get();
        if (!module) {
            it = m_Modules.erase(it);
            continue;
        }
        if (module->PendingReplacement && !module->PendingDestroy && !module->ActiveContext) {
            *it = std::move(module->PendingReplacement);
            ++it;
            continue;
        }
        if (module->PendingErase && !module->PendingDestroy && !module->ActiveContext) {
            it = m_Modules.erase(it);
            continue;
        }
        ++it;
    }
}

void ScriptRuntime::UpdateModule(Module &module, float deltaTime, float timeSeconds) {
    if (!module.Loaded || module.Failed) {
        return;
    }
    auto makeContext = [&](const char *phase) {
        return ScriptRuntimeInternal::MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                        module.Meta,
                                                        phase ? phase : "",
                                                        ModuleState(module),
                                                        module.Generation,
                                                        m_FrameIndex,
                                                        deltaTime,
                                                        timeSeconds);
    };
    if (module.PendingDestroy) {
        DestroyModule(module);
        return;
    }
    if (module.PendingDisable) {
        DisableModule(module);
        return;
    }
    if (module.PendingPause) {
        ScriptRuntimeContext ctx = makeContext("OnPause");
        PauseModule(module, ctx);
        return;
    }
    if (module.PendingReset) {
        ScriptRuntimeContext ctx = makeContext("OnReset");
        ResetModule(module, ctx);
        return;
    }
    if (!module.LoadCalled) {
        ScriptRuntimeContext ctx = makeContext("OnLoad");
        const InvokeStatus status = Invoke(module, "OnLoad", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.LoadCalled = true;
    }
    if (!module.AwakeCalled) {
        ScriptRuntimeContext ctx = makeContext("Awake");
        const InvokeStatus status = Invoke(module, "Awake", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.AwakeCalled = true;
    }
    if (!module.Enabled) {
        return;
    }
    if (!module.EnableCalled) {
        ScriptRuntimeContext ctx = makeContext("OnEnable");
        const InvokeStatus status = Invoke(module, "OnEnable", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.EnableCalled = true;
    }
    if (!module.StartCalled) {
        ScriptRuntimeContext ctx = makeContext("Start");
        const InvokeStatus status = Invoke(module, "Start", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.StartCalled = true;
    }
    ScriptRuntimeContext ctx = makeContext("Update");
    Invoke(module, "Update", ctx);
}

ScriptRuntime::InvokeStatus ScriptRuntime::InvokeMessage(Module &module, const ScriptMessage &message, const ScriptRuntimeContext &context) {
    if (!module.Cached || !module.Cached->module) {
        return InvokeStatus::Finished;
    }
    asIScriptFunction *func = nullptr;
    if (!module.ActiveContext) {
        const std::string decl = "void OnMessage(const ScriptMessage &in msg, const ScriptRuntimeContext &in ctx)";
        func = module.Type
            ? module.Type->GetMethodByDecl(decl.c_str())
            : module.Cached->module->GetFunctionByDecl(decl.c_str());
        if (!func) {
            return InvokeStatus::Finished;
        }
    }
    asIScriptEngine *engine = module.ActiveContext ? module.ActiveContext->GetEngine() : (func ? func->GetEngine() : nullptr);
    asIScriptContext *ctx = module.ActiveContext;
    if (!ctx) {
        ctx = engine ? engine->RequestContext() : nullptr;
        if (!ctx) {
            SetModuleError(module, "OnMessage: failed to request AngelScript context");
            return InvokeStatus::Failed;
        }
        module.ActiveContext = ctx;
        module.ActiveInvocation = "OnMessage";
        module.MessageStorage = message;
    }
    module.ContextStorage = context;
    int r = 0;
    if (ctx->GetState() == asEXECUTION_SUSPENDED) {
        std::string waitError;
        ScriptAsyncScheduler *scheduler = m_Manager ? m_Manager->GetAsyncScheduler() : nullptr;
        const ScriptAsyncScheduler::ResumeState resume = scheduler
            ? scheduler->PrepareContextResume(ctx, waitError)
            : ScriptAsyncScheduler::ResumeState::Ready;
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            return InvokeStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            CancelActiveInvocation(module);
            SetModuleError(module, waitError.empty() ? "Awaited async task failed." : waitError);
            return InvokeStatus::Failed;
        }
    } else {
        r = ctx->Prepare(func);
        if (r >= 0 && module.Object) {
            r = ctx->SetObject(module.Object);
        }
        if (r >= 0) {
            r = ctx->SetArgObject(0, &module.MessageStorage);
        }
        if (r >= 0) {
            r = ctx->SetArgObject(1, &module.ContextStorage);
        }
        if (r < 0) {
            CancelActiveInvocation(module);
            SetModuleError(module, "OnMessage: failed to prepare AngelScript context");
            return InvokeStatus::Failed;
        }
    }
    r = ctx->Execute();
    if (r == asEXECUTION_SUSPENDED) {
        return InvokeStatus::Suspended;
    }
    if (r != asEXECUTION_FINISHED) {
        std::string messageText = fmt::format("OnMessage failed with AngelScript result {}", r);
        if (r == asEXECUTION_EXCEPTION && ctx->GetExceptionString()) {
            messageText = ctx->GetExceptionString();
        }
        CancelActiveInvocation(module);
        SetModuleError(module, messageText);
        return InvokeStatus::Failed;
    }
    CancelActiveInvocation(module);
    return InvokeStatus::Finished;
}

ScriptRuntime::InvokeStatus ScriptRuntime::Invoke(Module &module, const char *name, const ScriptRuntimeContext &context, bool required) {
    if (!module.Cached || !module.Cached->module || !name || name[0] == '\0') {
        return required ? InvokeStatus::Failed : InvokeStatus::Finished;
    }

    if (module.ActiveContext && module.ActiveInvocation != name) {
        const std::string active = module.ActiveInvocation;
        const ScriptRuntimeContext activeContext = module.ContextStorage;
        const InvokeStatus activeStatus = active == "OnMessage"
            ? InvokeMessage(module, module.MessageStorage, activeContext)
            : Invoke(module, active.c_str(), activeContext, false);
        if (activeStatus != InvokeStatus::Finished) {
            return activeStatus;
        }
    }

    asIScriptFunction *func = nullptr;
    if (!module.ActiveContext) {
        std::string decl = fmt::format("void {}(const ScriptRuntimeContext &in ctx)", name);
        if (module.Type) {
            func = module.Type->GetMethodByDecl(decl.c_str());
        } else {
            func = module.Cached->module->GetFunctionByDecl(decl.c_str());
        }
        if (!func) {
            asIScriptFunction *wrongSignature = module.Type
                ? module.Type->GetMethodByName(name)
                : module.Cached->module->GetFunctionByName(name);
            if (wrongSignature) {
                SetModuleError(module,
                               fmt::format("{} has invalid runtime lifecycle signature '{}'; expected '{}'.",
                                           name,
                                           wrongSignature->GetDeclaration(),
                                           decl));
                return InvokeStatus::Failed;
            }
            return required ? InvokeStatus::Failed : InvokeStatus::Finished;
        }
    }

    asIScriptEngine *engine = module.ActiveContext ? module.ActiveContext->GetEngine() : (func ? func->GetEngine() : nullptr);
    asIScriptContext *ctx = module.ActiveContext;
    if (!ctx) {
        ctx = engine ? engine->RequestContext() : nullptr;
        if (!ctx) {
            SetModuleError(module, fmt::format("{}: failed to request AngelScript context", name));
            return InvokeStatus::Failed;
        }
        module.ActiveContext = ctx;
        module.ActiveInvocation = name;
    }

    module.ContextStorage = context;
    int r = 0;
    if (ctx->GetState() == asEXECUTION_SUSPENDED) {
        std::string waitError;
        ScriptAsyncScheduler *scheduler = m_Manager ? m_Manager->GetAsyncScheduler() : nullptr;
        const ScriptAsyncScheduler::ResumeState resume = scheduler
            ? scheduler->PrepareContextResume(ctx, waitError)
            : ScriptAsyncScheduler::ResumeState::Ready;
        if (resume == ScriptAsyncScheduler::ResumeState::Pending) {
            return InvokeStatus::Suspended;
        }
        if (resume == ScriptAsyncScheduler::ResumeState::Failed) {
            CancelActiveInvocation(module);
            SetModuleError(module, waitError.empty() ? "Awaited async task failed." : waitError);
            return InvokeStatus::Failed;
        }
    } else {
        r = ctx->Prepare(func);
        if (r >= 0 && module.Object) {
            r = ctx->SetObject(module.Object);
        }
        if (r >= 0 && func->GetParamCount() > 0) {
            r = ctx->SetArgObject(0, &module.ContextStorage);
        }
        if (r < 0) {
            CancelActiveInvocation(module);
            SetModuleError(module, fmt::format("{}: failed to prepare AngelScript context", name));
            return InvokeStatus::Failed;
        }
    }

    r = ctx->Execute();
    if (r == asEXECUTION_SUSPENDED) {
        return InvokeStatus::Suspended;
    }
    if (r != asEXECUTION_FINISHED) {
        std::string message;
        if (r == asEXECUTION_EXCEPTION) {
            const char *section = nullptr;
            int col = 0;
            const int row = ctx->GetExceptionLineNumber(&col, &section);
            asIScriptFunction *exFunc = ctx->GetExceptionFunction();
            message = fmt::format("{} threw in {} at {}({},{}): {}",
                                  module.ActiveInvocation.empty() ? name : module.ActiveInvocation,
                                  exFunc ? exFunc->GetDeclaration() : "<unknown>",
                                  section ? section : "<unknown>",
                                  row,
                                  col,
                                  ctx->GetExceptionString() ? ctx->GetExceptionString() : "");
        } else {
            message = fmt::format("{} failed with AngelScript result {}",
                                  module.ActiveInvocation.empty() ? name : module.ActiveInvocation,
                                  r);
        }
        CancelActiveInvocation(module);
        SetModuleError(module, message);
        return InvokeStatus::Failed;
    }

    CancelActiveInvocation(module);
    return InvokeStatus::Finished;
}

bool ScriptRuntime::InvokeFinished(Module &module, const char *name, const ScriptRuntimeContext &context, bool required) {
    return Invoke(module, name, context, required) == InvokeStatus::Finished;
}

void ScriptRuntime::CancelActiveInvocation(Module &module) {
    if (!module.ActiveContext) {
        module.ActiveInvocation.clear();
        return;
    }
    asIScriptContext *ctx = module.ActiveContext;
    module.ActiveContext = nullptr;
    if (m_Manager && m_Manager->GetAsyncScheduler()) {
        m_Manager->GetAsyncScheduler()->ForgetContext(ctx);
    }
    const int state = ctx->GetState();
    if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED || state == asEXECUTION_PREPARED) {
        ctx->Abort();
    }
    if (ctx->GetEngine()) {
        ctx->GetEngine()->ReturnContext(ctx);
    } else {
        ctx->Release();
    }
    module.ActiveInvocation.clear();
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
    r = engine->RegisterObjectType("RuntimeDependencyInfo",
                                   sizeof(RuntimeDependencyInfo),
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeDependencyInfo *self) { new(self) RuntimeDependencyInfo(); },
                                                     (RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_CONSTRUCT, "void f(const RuntimeDependencyInfo &in other)",
                                        asFUNCTIONPR([](const RuntimeDependencyInfo &other, RuntimeDependencyInfo *self) {
                                                         new(self) RuntimeDependencyInfo(other);
                                                     },
                                                     (const RuntimeDependencyInfo &, RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeDependencyInfo *self) { self->~RuntimeDependencyInfo(); },
                                                     (RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "RuntimeDependencyInfo &opAssign(const RuntimeDependencyInfo &in other)",
                                     asMETHODPR(RuntimeDependencyInfo, operator=, (const RuntimeDependencyInfo &), RuntimeDependencyInfo &),
                                     asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Raw() const", asMETHOD(RuntimeDependencyInfo, Raw), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Id() const", asMETHOD(RuntimeDependencyInfo, Id), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Operator() const", asMETHOD(RuntimeDependencyInfo, Operator), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Version() const", asMETHOD(RuntimeDependencyInfo, Version), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string ActualVersion() const", asMETHOD(RuntimeDependencyInfo, ActualVersion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Optional() const", asMETHOD(RuntimeDependencyInfo, Optional), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Present() const", asMETHOD(RuntimeDependencyInfo, Present), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Satisfied() const", asMETHOD(RuntimeDependencyInfo, Satisfied), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectType("RuntimeScriptInfo",
                                   sizeof(RuntimeScriptInfo),
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeScriptInfo *self) { new(self) RuntimeScriptInfo(); },
                                                     (RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_CONSTRUCT, "void f(const RuntimeScriptInfo &in other)",
                                        asFUNCTIONPR([](const RuntimeScriptInfo &other, RuntimeScriptInfo *self) {
                                                         new(self) RuntimeScriptInfo(other);
                                                     },
                                                     (const RuntimeScriptInfo &, RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeScriptInfo *self) { self->~RuntimeScriptInfo(); },
                                                     (RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "RuntimeScriptInfo &opAssign(const RuntimeScriptInfo &in other)",
                                     asMETHODPR(RuntimeScriptInfo, operator=, (const RuntimeScriptInfo &), RuntimeScriptInfo &),
                                     asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Exists() const", asMETHOD(RuntimeScriptInfo, Exists), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Id() const", asMETHOD(RuntimeScriptInfo, Id), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Name() const", asMETHOD(RuntimeScriptInfo, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Version() const", asMETHOD(RuntimeScriptInfo, Version), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Description() const", asMETHOD(RuntimeScriptInfo, Description), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Author() const", asMETHOD(RuntimeScriptInfo, Author), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Category() const", asMETHOD(RuntimeScriptInfo, Category), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "int TagCount() const", asMETHOD(RuntimeScriptInfo, TagCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Tag(int index) const", asMETHOD(RuntimeScriptInfo, Tag), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Enabled() const", asMETHOD(RuntimeScriptInfo, Enabled), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Loaded() const", asMETHOD(RuntimeScriptInfo, Loaded), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Failed() const", asMETHOD(RuntimeScriptInfo, Failed), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string State() const", asMETHOD(RuntimeScriptInfo, State), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Phase() const", asMETHOD(RuntimeScriptInfo, Phase), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Error() const", asMETHOD(RuntimeScriptInfo, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string RootPath() const", asMETHOD(RuntimeScriptInfo, RootPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string ManifestPath() const", asMETHOD(RuntimeScriptInfo, ManifestPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string EntryPath() const", asMETHOD(RuntimeScriptInfo, EntryPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "int Generation() const", asMETHOD(RuntimeScriptInfo, Generation), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string ManifestPath() const", asMETHOD(ScriptRuntimeContext, ManifestPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string EntryPath() const", asMETHOD(ScriptRuntimeContext, EntryPath), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string Phase() const", asMETHOD(ScriptRuntimeContext, Phase), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string State() const", asMETHOD(ScriptRuntimeContext, State), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "int Generation() const", asMETHOD(ScriptRuntimeContext, Generation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "uint64 FrameIndex() const", asMETHOD(ScriptRuntimeContext, FrameIndex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string Metadata(const string &in key, const string &in fallback = \"\") const", asMETHOD(ScriptRuntimeContext, Metadata), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "int MetadataCount() const", asMETHOD(ScriptRuntimeContext, MetadataCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string MetadataKey(int index) const", asMETHOD(ScriptRuntimeContext, MetadataKey), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "string MetadataValue(int index) const", asMETHOD(ScriptRuntimeContext, MetadataValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "void Raise(const string &in message) const", asMETHOD(ScriptRuntimeContext, Raise), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ScriptRuntimeContext", "CKBehaviorContext ToBehaviorContext() const", asMETHOD(ScriptRuntimeContext, ToBehaviorContext), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterGlobalFunction("array<RuntimeScriptInfo>@ ListInfo(const ScriptRuntimeContext &in ctx)", asFUNCTION(ScriptRuntimeInternal::RuntimeListInfo), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("RuntimeScriptInfo Info(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeInfo), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Version(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeVersion), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string Metadata(const ScriptRuntimeContext &in ctx, const string &in id, const string &in key, const string &in fallback = \"\")", asFUNCTION(ScriptRuntimeInternal::RuntimeMetadata), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<string>@ Dependencies(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeDependencies), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<RuntimeDependencyInfo>@ RequiredDependencies(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeRequiredDependencies), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<RuntimeDependencyInfo>@ OptionalDependencies(const ScriptRuntimeContext &in ctx, const string &in id)", asFUNCTION(ScriptRuntimeInternal::RuntimeOptionalDependencies), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}
