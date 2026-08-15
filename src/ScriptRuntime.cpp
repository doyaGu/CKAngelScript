#include "ScriptRuntime.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>

#include "CKAll.h"
#include "CKPathManager.h"
#include "CKTimeManager.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptBridgeScope.h"
#include "ScriptHostScope.h"
#include "ScriptInvocation.h"
#include "ScriptManager.h"
#include "ScriptInitialLifecycle.h"
#include "ScriptMessage.h"
#include "ScriptScene.h"
#include "ScriptParameterRegistry.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"
#include "ScriptUtils.h"
#include "add_on/scriptarray/scriptarray.h"
#include "ScriptRegistration.h"

namespace {

constexpr const char *kModulePrefix = "__CKASRuntime_";

struct ScriptEntryDiscoveryRoot {
    std::filesystem::path Path;
    bool Recursive = false;
};

enum RuntimeLifecycleIndex {
    LifecycleOnLoad = 0,
    LifecycleAwake,
    LifecycleOnEnable,
    LifecycleStart,
    LifecycleUpdate,
    LifecycleOnPostLoad,
    LifecycleOnPostProcess,
    LifecycleOnDisable,
    LifecycleOnDestroy,
    LifecycleOnReset,
    LifecycleOnPause,
    LifecycleOnResume,
    LifecycleCount
};

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

static_assert(sizeof(kLifecycleNames) / sizeof(kLifecycleNames[0]) == LifecycleCount,
              "runtime lifecycle name table must match RuntimeLifecycleIndex");

constexpr const char *kLoadingState = "loading";
constexpr const char *kStateFailed = "failed";
constexpr const char *kStateDestroying = "destroying";
constexpr const char *kStateDisabling = "disabling";
constexpr const char *kStateSuspended = "suspended";
constexpr const char *kStatePaused = "paused";
constexpr const char *kStateResuming = "resuming";
constexpr const char *kStateResetting = "resetting";
constexpr const char *kStateUnloaded = "unloaded";
constexpr const char *kStateDisabled = "disabled";
constexpr const char *kStateEnabled = "enabled";
constexpr const char *kStateLoaded = "loaded";

bool ScriptEntryValidateOnly() {
    const char *value = std::getenv("CKAS_SCRIPT_VALIDATE_ONLY");
    if (!value || value[0] == '\0') {
        value = std::getenv("CKAS_RUNTIME_VALIDATE_ONLY");
    }
    if (!value || value[0] == '\0') {
        return false;
    }
    const std::string lowered = ScriptRuntimeMetadata::ToLower(value);
    return lowered != "0" && lowered != "false" && lowered != "off" && lowered != "no";
}

BehaviorGraph *RuntimeBehaviorGraphByRoot(const ScriptContext &ctx, const std::string &rootBehaviorName);

ScriptContext MakeRuntimeContext(CKContext *context,
                                        const ScriptRuntimeManifest &metadata,
                                        const char *phase,
                                        const char *state,
                                        int generation,
                                        std::uint64_t frameIndex,
                                        float deltaTime,
                                        float timeSeconds) {
    return ScriptContext(context,
                                &metadata,
                                phase,
                                state,
                                generation,
                                frameIndex,
                                deltaTime,
                                timeSeconds);
}

ScriptContext MakeRuntimeContext(CKContext *context,
                                        const ScriptRuntimeManifest &metadata,
                                        float deltaTime,
                                        float timeSeconds) {
    return MakeRuntimeContext(context, metadata, "", "", 0, 0, deltaTime, timeSeconds);
}

CKBehaviorContext MakeClearedBehaviorContext() {
    CKBehaviorContext ctx;
    ctx.Behavior = nullptr;
    ctx.DeltaTime = 0.0f;
    ctx.Context = nullptr;
    ctx.CurrentLevel = nullptr;
    ctx.CurrentScene = nullptr;
    ctx.PreviousScene = nullptr;
    ctx.CurrentRenderContext = nullptr;
    ctx.ParameterManager = nullptr;
    ctx.MessageManager = nullptr;
    ctx.AttributeManager = nullptr;
    ctx.TimeManager = nullptr;
    ctx.CallbackMessage = 0;
    ctx.CallbackArg = nullptr;
    return ctx;
}

int LifecycleIndexByName(const char *name) {
    if (!name) {
        return -1;
    }
    for (int i = 0; i < LifecycleCount; ++i) {
        if (std::strcmp(name, kLifecycleNames[i]) == 0) {
            return i;
        }
    }
    return -1;
}

void ReleaseScriptFunction(asIScriptFunction *&func) {
    if (func) {
        func->Release();
        func = nullptr;
    }
}

ScriptManager *ManagerFromRuntimeContext(const ScriptContext &ctx) {
    return ScriptHostScope::From(ctx).Manager();
}

bool UsesRuntimeLifecycle(const ScriptRuntimeManifest &metadata) {
    return metadata.Host == ScriptEntryHost::Runtime;
}

BehaviorBridge *RuntimeBehaviorFrom(const ScriptContext &ctx) {
    return ScriptBehaviorAdapter::From(ScriptHostScope::From(ctx));
}

BehaviorGraph *RuntimeBehaviorGraph(const ScriptContext &ctx) {
    return RuntimeBehaviorGraphByRoot(ctx, std::string());
}

BehaviorGraph *RuntimeBehaviorGraphByRoot(const ScriptContext &ctx, const std::string &rootBehaviorName) {
    return ScriptBehaviorAdapter::Graph(ScriptHostScope::From(ctx), rootBehaviorName);
}

BehaviorRef *RuntimeBehaviorFind(const ScriptContext &ctx, const std::string &name) {
    return ScriptBehaviorAdapter::Find(ScriptHostScope::From(ctx), name);
}

BehaviorRef *RuntimeBehaviorFindById(const ScriptContext &ctx, CK_ID id) {
    return ScriptBehaviorAdapter::FindById(ScriptHostScope::From(ctx), id);
}

BBBridge *RuntimeBBFrom(const ScriptContext &ctx) {
    return ScriptBBAdapter::From(ScriptHostScope::From(ctx), ScriptBridgeFailureMode::Quiet);
}

BBDecl *RuntimeBBRequireByName(const ScriptContext &ctx, const std::string &query) {
    return ScriptBBAdapter::Require(ScriptHostScope::From(ctx), query, ScriptBridgeFailureMode::Quiet);
}

BBDecl *RuntimeBBRequireByGuid(const ScriptContext &ctx, CKGUID guid) {
    return ScriptBBAdapter::Require(ScriptHostScope::From(ctx), guid, ScriptBridgeFailureMode::Quiet);
}

int RuntimeBBCount(const ScriptContext &ctx) {
    return ScriptBBAdapter::Count(ScriptHostScope::From(ctx), ScriptBridgeFailureMode::Quiet);
}

BBPrototype *RuntimeBBAt(const ScriptContext &ctx, int index) {
    return ScriptBBAdapter::At(ScriptHostScope::From(ctx), index, ScriptBridgeFailureMode::Quiet);
}

BBPrototype *RuntimeBBFind(const ScriptContext &ctx, const std::string &query, int occurrence) {
    return ScriptBBAdapter::Find(
        ScriptHostScope::From(ctx), query, occurrence, ScriptBridgeFailureMode::Quiet);
}

CScriptArray *RuntimeBBFindAll(const ScriptContext &ctx, const std::string &query) {
    return ScriptBBAdapter::FindAll(ScriptHostScope::From(ctx), query, ScriptBridgeFailureMode::Quiet);
}

ParamTypeInfo *RuntimeParamAt(const ScriptContext &ctx, int index) {
    ScriptParameterRegistry *registry = ScriptHostScope::From(ctx).ParameterRegistry();
    const ScriptParamTypeRecord *record = registry ? registry->At(index) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamFind(const ScriptContext &ctx, const std::string &query, int occurrence) {
    ScriptParameterRegistry *registry = ScriptHostScope::From(ctx).ParameterRegistry();
    const ScriptParamTypeRecord *record = registry ? registry->Find(query, occurrence) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamTypeByName(const ScriptContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptHostScope::From(ctx).ParameterRegistry();
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

ParamTypeInfo *RuntimeParamTypeByGuid(const ScriptContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptHostScope::From(ctx).ParameterRegistry();
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    return record ? new ParamTypeInfo(registry, record->Type) : nullptr;
}

int RuntimeParamCount(const ScriptContext &ctx) {
    ScriptParameterRegistry *registry = ScriptHostScope::From(ctx).ParameterRegistry();
    return registry ? registry->Count() : 0;
}

bool RuntimeReloadAll(const ScriptContext &ctx) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->ReloadAll(&error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

bool RuntimeReload(const ScriptContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->Reload(id, &error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

bool RuntimeEnable(const ScriptContext &ctx, const std::string &id, bool enabled) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    std::string error;
    const bool ok = manager && manager->GetRuntime() && manager->GetRuntime()->Enable(id, enabled, &error);
    if (!ok && !error.empty()) {
        ctx.Raise(error);
    }
    return ok;
}

CScriptArray *RuntimeList(const ScriptContext &ctx) {
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

CScriptArray *RuntimeListInfo(const ScriptContext &ctx) {
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

RuntimeScriptInfo RuntimeInfo(const ScriptContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Info(id) : RuntimeScriptInfo();
}

std::string RuntimeVersion(const ScriptContext &ctx, const std::string &id) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Version(id) : std::string();
}

std::string RuntimeMetadata(const ScriptContext &ctx,
                            const std::string &id,
                            const std::string &key,
                            const std::string &fallback) {
    ScriptManager *manager = ManagerFromRuntimeContext(ctx);
    return manager && manager->GetRuntime() ? manager->GetRuntime()->Metadata(id, key, fallback) : fallback;
}

CScriptArray *RuntimeDependencies(const ScriptContext &ctx, const std::string &id) {
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

CScriptArray *RuntimeDependencyInfos(const ScriptContext &ctx, const std::string &id, bool optional) {
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

CScriptArray *RuntimeRequiredDependencies(const ScriptContext &ctx, const std::string &id) {
    return RuntimeDependencyInfos(ctx, id, false);
}

CScriptArray *RuntimeOptionalDependencies(const ScriptContext &ctx, const std::string &id) {
    return RuntimeDependencyInfos(ctx, id, true);
}

} // namespace

struct ScriptRuntime::Module {
    ScriptRuntimeManifest Meta;
    std::string ModuleName;
    std::string MessageTarget;
    int Generation = 0;
    std::shared_ptr<CachedScript> Cached;
    asIScriptObject *Object = nullptr;
    asITypeInfo *Type = nullptr;
    asIScriptFunction *Lifecycle[LifecycleCount] = {};
    asIScriptFunction *OnMessage = nullptr;
    bool Enabled = true;
    bool Loaded = false;
    ScriptInitialLifecycle InitialLifecycle;
    bool EnableCalled = false;
    bool Failed = false;
    std::string Error;
    std::unique_ptr<ScriptInvocationContext> ActiveCall;
    std::string ActiveInvocation;
    ScriptContext ContextStorage;
    ScriptMessage MessageStorage;
    bool PendingDisable = false;
    bool PendingDestroy = false;
    bool PendingErase = false;
    bool PendingPause = false;
    bool PendingResume = false;
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

std::string RuntimeScriptInfo::Host() const {
    return m_Host;
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

std::string RuntimeScriptInfo::Root() const {
    return m_RootPath;
}

std::string RuntimeScriptInfo::Manifest() const {
    return m_ManifestPath;
}

std::string RuntimeScriptInfo::Entry() const {
    return m_EntryPath;
}

int RuntimeScriptInfo::Generation() const {
    return m_Generation;
}

ScriptContext::ScriptContext() = default;

ScriptContext::ScriptContext(CKContext *context,
                             const ScriptRuntimeManifest *metadata,
                             const char *phase,
                             const char *state,
                             int generation,
                             std::uint64_t frameIndex,
                             float deltaTime,
                             float timeSeconds)
    : m_Context(context),
      m_Metadata(metadata),
      m_Phase(phase ? phase : ""),
      m_State(state ? state : ""),
      m_Generation(generation),
      m_FrameIndex(frameIndex),
      m_DeltaTime(deltaTime),
      m_TimeSeconds(timeSeconds) {}

CKContext *ScriptContext::Context() const {
    return m_Context;
}

float ScriptContext::DeltaTime() const {
    return m_DeltaTime;
}

float ScriptContext::TimeSeconds() const {
    return m_TimeSeconds;
}

std::string ScriptContext::Id() const {
    return m_Metadata ? m_Metadata->Id : std::string();
}

std::string ScriptContext::Name() const {
    return m_Metadata ? m_Metadata->Name : std::string();
}

std::string ScriptContext::Version() const {
    return m_Metadata ? m_Metadata->VersionText : std::string();
}

std::string ScriptContext::Root() const {
    return m_Metadata ? ScriptRuntimeMetadata::PathString(m_Metadata->RootPath) : std::string();
}

std::string ScriptContext::Manifest() const {
    return m_Metadata ? ScriptRuntimeMetadata::PathString(m_Metadata->ManifestPath) : std::string();
}

std::string ScriptContext::Entry() const {
    return m_Metadata ? ScriptRuntimeMetadata::PathString(m_Metadata->EntryPath) : std::string();
}

std::string ScriptContext::Target() const {
    return m_Metadata ? ScriptMessageBus::RuntimeTarget(m_Metadata->Id) : std::string();
}

std::string ScriptContext::Phase() const {
    return m_Phase ? m_Phase : "";
}

std::string ScriptContext::State() const {
    return m_State ? m_State : "";
}

int ScriptContext::Generation() const {
    return m_Generation;
}

std::uint64_t ScriptContext::FrameIndex() const {
    return m_FrameIndex;
}

std::string ScriptContext::Metadata(const std::string &key, const std::string &fallback) const {
    return m_Metadata ? ScriptRuntimeMetadata::MetadataValue(m_Metadata->CustomMetadata, key, fallback) : fallback;
}

int ScriptContext::MetadataCount() const {
    return m_Metadata ? static_cast<int>(m_Metadata->CustomMetadata.size()) : 0;
}

std::string ScriptContext::MetadataKey(int index) const {
    return m_Metadata && index >= 0 && index < static_cast<int>(m_Metadata->CustomMetadata.size()) ? m_Metadata->CustomMetadata[index].Key : std::string();
}

std::string ScriptContext::MetadataValue(int index) const {
    return m_Metadata && index >= 0 && index < static_cast<int>(m_Metadata->CustomMetadata.size()) ? m_Metadata->CustomMetadata[index].Value : std::string();
}

void ScriptContext::Raise(const std::string &message) const {
    if (m_Context) {
        m_Context->OutputToConsoleEx(const_cast<char *>("[AngelScript Runtime:%s] %s"),
                                     Id().c_str(),
                                     message.c_str());
    }
}

CKBehaviorContext ScriptContext::ToBehaviorContext() const {
    CKBehaviorContext ctx = MakeClearedBehaviorContext();
#if CKVERSION == 0x13022002
    if (m_Context) {
        ctx = m_Context->m_BehaviorContext;
    }
#endif
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
    if (ScriptEntryValidateOnly()) {
        FinalizePendingModules();
        return;
    }
    ++m_FrameIndex;

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKTimeManager *time = context ? context->GetTimeManager() : nullptr;
    const float deltaTime = time ? time->GetLastDeltaTime() : 0.0f;
    const float timeSeconds = time ? (time->GetTime() * 0.001f) : 0.0f;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module && UsesRuntimeLifecycle(module->Meta)) {
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
    if (ScriptEntryValidateOnly()) {
        FinalizePendingModules();
        return;
    }

    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKTimeManager *time = context ? context->GetTimeManager() : nullptr;
    const float deltaTime = time ? time->GetLastDeltaTime() : 0.0f;
    const float timeSeconds = time ? (time->GetTime() * 0.001f) : 0.0f;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !UsesRuntimeLifecycle(module->Meta) ||
            !module->Loaded || module->Failed || !module->Enabled || !module->EnableCalled) {
            continue;
        }
        ScriptContext ctx = MakeRuntimeContext(context,
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
    if (ScriptEntryValidateOnly()) {
        FinalizePendingModules();
        return;
    }
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !UsesRuntimeLifecycle(module->Meta) || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptContext ctx = MakeRuntimeContext(context,
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
        if (!module || !UsesRuntimeLifecycle(module->Meta) || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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
        if (!module || !UsesRuntimeLifecycle(module->Meta) || !module->Loaded || module->Failed) {
            continue;
        }
        ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
                                                                             "OnPause",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        module->PendingResume = false;
        PauseModule(*module, ctx);
    }
}

void ScriptRuntime::OnResume() {
    if (!m_Paused) {
        return;
    }
    m_Paused = false;
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (!module || !UsesRuntimeLifecycle(module->Meta) ||
            !module->Loaded || module->Failed || !module->Enabled) {
            continue;
        }
        ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                             module->Meta,
                                                                             "OnResume",
                                                                             ModuleState(*module),
                                                                             module->Generation,
                                                                             m_FrameIndex,
                                                                             0.0f,
                                                                             0.0f);
        module->PendingResume = true;
        if (module->PendingPause || module->PendingDisable || module->PendingDestroy || module->PendingReset) {
            continue;
        }
        ResumeModule(*module, ctx);
    }
}

void ScriptRuntime::OnEnd() {
    Clear();
}

void ScriptRuntime::Clear() {
    if (m_Manager && m_Manager->GetMessageBus()) {
        for (const std::unique_ptr<Module> &module : m_Modules) {
            if (module) {
                m_Manager->GetMessageBus()->ClearTarget(module->MessageTarget, "Runtime was cleared.");
            }
        }
    }
    for (std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            DestroyModule(*module, true);
        }
    }
    m_Modules.clear();
    m_ModulesById.clear();
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
            *error = fmt::format("Script entry '{}' was not found during DATA_PATH Scripts scan.", id);
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
    Module *module = FindModule(id);
    if (module) {
        module->Enabled = enabled;
        module->Meta.Enabled = enabled;
        if (!enabled) {
            DisableModule(*module);
        }
        return true;
    }
    if (error) {
        *error = fmt::format("Script entry '{}' is not loaded.", id);
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
    Module *module = FindModule(id);
    return module ? BuildInfo(*module) : RuntimeScriptInfo();
}

std::string ScriptRuntime::ModuleName(const std::string &id) {
    EnsureScanned();
    Module *module = FindModule(id);
    if (!module || !module->Loaded || module->Failed || !module->Enabled || module->ModuleName.empty()) {
        return std::string();
    }
    return module->ModuleName;
}

std::string ScriptRuntime::Version(const std::string &id) const {
    Module *module = FindModule(id);
    return module ? module->Meta.VersionText : std::string();
}

std::string ScriptRuntime::Metadata(const std::string &id, const std::string &key, const std::string &fallback) const {
    Module *module = FindModule(id);
    return module ? ScriptRuntimeMetadata::MetadataValue(module->Meta.CustomMetadata, key, fallback) : fallback;
}

std::vector<std::string> ScriptRuntime::Dependencies(const std::string &id) const {
    Module *module = FindModule(id);
    if (module) {
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
    Module *module = FindModule(id);
    return module ? DependencyInfo(*module, false) : std::vector<RuntimeDependencyInfo>();
}

std::vector<RuntimeDependencyInfo> ScriptRuntime::OptionalDependencies(const std::string &id) const {
    Module *module = FindModule(id);
    return module ? DependencyInfo(*module, true) : std::vector<RuntimeDependencyInfo>();
}

bool ScriptRuntime::DeliverMessage(const std::string &id, const ScriptMessage &message, bool immediate, std::string &error) {
    Module *module = FindModule(id);
    if (!module) {
        error = "Runtime message target was not found.";
        return false;
    }
    if (!UsesRuntimeLifecycle(module->Meta)) {
        error = "Message target is not a runtime entry.";
        return false;
    }
    if (!module->Loaded || module->Failed || !module->Enabled || !module->EnableCalled) {
        error = "Runtime message target is not ready.";
        return false;
    }
    if (module->ActiveCall) {
        error = "Runtime message target is busy.";
        return false;
    }
    ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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
    std::vector<ScriptEntryDiscoveryRoot> roots;
    CKContext *context = m_Manager ? m_Manager->GetCKContext() : nullptr;
    CKPathManager *paths = context ? context->GetPathManager() : nullptr;
    if (paths) {
        const int count = paths->GetPathCount(DATA_PATH_IDX);
        for (int i = 0; i < count; ++i) {
            XString pathName;
            if (paths->GetPathName(DATA_PATH_IDX, i, pathName) == CK_OK && pathName.Length() > 0) {
                roots.push_back({fs::path(pathName.CStr()) / "Scripts", false});
            }
        }
    }
    if (const char *envRoots = std::getenv("CKAS_SCRIPT_ROOTS")) {
        for (const std::string &root : ScriptRuntimeMetadata::SplitList(envRoots)) {
            fs::path path(root);
            if (ScriptRuntimeMetadata::ToLower(path.filename().string()) == "scripts") {
                roots.push_back({std::move(path), true});
                continue;
            }
            std::error_code ec;
            fs::path scripts = path / "Scripts";
            if (fs::is_directory(scripts, ec)) {
                roots.push_back({std::move(scripts), true});
            } else {
                roots.push_back({std::move(path), true});
            }
        }
    }

    std::vector<ScriptEntryDiscoveryRoot> uniqueRoots;
    std::unordered_map<std::string, std::size_t> rootIndex;
    for (ScriptEntryDiscoveryRoot rootSpec : roots) {
        fs::path root = std::move(rootSpec.Path);
        root = fs::absolute(root);
        std::error_code ec;
        root = fs::weakly_canonical(root, ec);
        if (ec) {
            root = fs::absolute(root);
        }
        const std::string rootKey = ScriptRuntimeMetadata::ToLower(ScriptRuntimeMetadata::PathString(root));
        if (!fs::is_directory(root)) {
            continue;
        }
        const auto [it, inserted] = rootIndex.emplace(rootKey, uniqueRoots.size());
        if (!inserted) {
            uniqueRoots[it->second].Recursive = uniqueRoots[it->second].Recursive || rootSpec.Recursive;
            continue;
        }
        uniqueRoots.push_back({std::move(root), rootSpec.Recursive});
    }

    std::vector<ScriptRuntimeManifest> candidatesOut;
    std::set<std::string> seenIds;
    for (ScriptEntryDiscoveryRoot rootSpec : uniqueRoots) {
        fs::path root = std::move(rootSpec.Path);

        std::vector<fs::path> candidates;
        const fs::path selfManifest = root / "script.as";
        std::error_code selfManifestError;
        if (fs::is_regular_file(selfManifest, selfManifestError) && !ScriptRuntimeMetadata::IsSkippedMainFile(selfManifest)) {
            candidates.push_back(selfManifest);
        } else if (rootSpec.Recursive) {
            std::vector<fs::path> pending{root};
            while (!pending.empty()) {
                const fs::path current = pending.back();
                pending.pop_back();

                std::error_code walkError;
                for (const fs::directory_entry &entry : fs::directory_iterator(current, fs::directory_options::skip_permission_denied, walkError)) {
                    if (walkError) {
                        break;
                    }
                    const fs::path path = entry.path();
                    if (entry.is_regular_file() && path.extension() == ".as" && !ScriptRuntimeMetadata::IsSkippedMainFile(path)) {
                        candidates.push_back(path);
                    } else if (entry.is_directory() && !ScriptRuntimeMetadata::IsSkippedDirectory(path)) {
                        fs::path manifest = path / "script.as";
                        std::error_code manifestError;
                        if (fs::is_regular_file(manifest, manifestError) && !ScriptRuntimeMetadata::IsSkippedMainFile(manifest)) {
                            candidates.push_back(manifest);
                        } else {
                            pending.push_back(path);
                        }
                    }
                }
            }
        } else {
            std::error_code walkError;
            for (const fs::directory_entry &entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, walkError)) {
                if (walkError) {
                    break;
                }
                const fs::path path = entry.path();
                if (entry.is_regular_file() && path.extension() == ".as" && !ScriptRuntimeMetadata::IsSkippedMainFile(path)) {
                    candidates.push_back(path);
                } else if (entry.is_directory() && !ScriptRuntimeMetadata::IsSkippedDirectory(path)) {
                    fs::path manifest = path / "script.as";
                    std::error_code manifestError;
                    if (fs::is_regular_file(manifest, manifestError) && !ScriptRuntimeMetadata::IsSkippedMainFile(manifest)) {
                        candidates.push_back(manifest);
                    }
                }
            }
        }
        std::sort(candidates.begin(), candidates.end());

        for (const fs::path &candidate : candidates) {
            ScriptRuntimeManifest metadata;
            metadata.RootPath = candidate.parent_path();
            metadata.ManifestPath = candidate;
            metadata.EntryPath = candidate;
            metadata.Id = ScriptRuntimeMetadata::DeriveIdFromPath(root, candidate);
            metadata.Name = metadata.Id;
            std::string parseError;
            const std::string source = ScriptRuntimeMetadata::ReadTextFile(candidate);
            if (!ScriptRuntimeMetadata::ParseManifestSource(source, candidate, metadata, parseError)) {
                error += fmt::format("Skipping '{}': {}.\n", ScriptRuntimeMetadata::PathString(candidate), parseError);
                continue;
            }
            if (!metadata.HasScriptManifest) {
                if (ScriptRuntimeMetadata::ToLower(candidate.filename().string()) == "script.as") {
                    error += fmt::format("Skipping '{}': missing main [runtime], [executor], or legacy [script] metadata.\n",
                                         ScriptRuntimeMetadata::PathString(candidate));
                }
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
                error += fmt::format("Duplicate script entry id '{}' from '{}' ignored.\n",
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
    failed->MessageTarget = ScriptMessageBus::RuntimeTarget(metadata.Id);
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
        module->PendingErase = true;
        if (DestroyModule(*module)) {
            m_ModulesById.erase(module->Meta.Id);
            m_Modules.erase(it);
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

    bool changed = false;
    for (auto it = m_Modules.begin(); it != m_Modules.end();) {
        Module *module = it->get();
        if (module && allowedIds.find(module->Meta.Id) == allowedIds.end()) {
            module->PendingErase = true;
            if (DestroyModule(*module)) {
                it = m_Modules.erase(it);
                changed = true;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    if (changed) {
        RebuildModuleIndex();
    }
}

bool ScriptRuntime::LoadModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> &module, std::string &error) {
    if (!m_Manager || !m_Manager->GetScriptEngine()) {
        error = "Script runtime cannot load modules before the AngelScript engine is initialized.";
        return false;
    }

    const int generation = ++m_Generation;
    const std::string moduleName = fmt::format("{}{}_{}", kModulePrefix, metadata.Id, generation);
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
    CKAngelScriptLoadOptions options = {};
    options.Size = sizeof(options);
    options.ModuleName = moduleName.c_str();
    options.Filenames = filePtrs.empty() ? nullptr : filePtrs.data();
    options.FileCount = filePtrs.size();
    options.Flags = CKAS_LOAD_REPLACEEXISTING;
    CKAngelScriptResult loadResult = {};
    if (m_Manager->LoadModule(options, &loadResult) != CKAS_OK) {
        error = loadResult.ErrorMessage && loadResult.ErrorMessage[0] != '\0'
            ? loadResult.ErrorMessage
            : fmt::format("Script entry '{}' failed to compile.", metadata.Id);
        return false;
    }
    std::shared_ptr<CachedScript> cached = m_Manager->GetCachedScript(moduleName.c_str());
    if (!cached || !cached->module) {
        error = fmt::format("Script entry '{}' failed to compile.", metadata.Id);
        m_Manager->UnloadModule(moduleName.c_str(), nullptr);
        return false;
    }

    std::unique_ptr<Module> loaded = std::make_unique<Module>();
    loaded->Meta = metadata;
    loaded->MessageTarget = ScriptMessageBus::RuntimeTarget(metadata.Id);
    loaded->ModuleName = moduleName;
    loaded->Generation = generation;
    loaded->Cached = cached;
    loaded->Enabled = metadata.Enabled;
    loaded->Loaded = true;

    const bool runtimeLifecycle = UsesRuntimeLifecycle(metadata);

    if (!metadata.ClassName.empty()) {
        loaded->Type = cached->module->GetTypeInfoByName(metadata.ClassName.c_str());
        if (!loaded->Type) {
            error = fmt::format("Script module '{}' class '{}' was not found.", metadata.Id, metadata.ClassName);
            m_Manager->UnloadModule(moduleName.c_str(), nullptr);
            return false;
        }
    }
    if (runtimeLifecycle && loaded->Type) {
        void *object = m_Manager->GetScriptEngine()->CreateScriptObject(loaded->Type);
        if (!object) {
            error = fmt::format("Runtime entry '{}' failed to create class '{}'.", metadata.Id, metadata.ClassName);
            m_Manager->UnloadModule(moduleName.c_str(), nullptr);
            return false;
        }
        loaded->Object = static_cast<asIScriptObject *>(object);
    }

    if (runtimeLifecycle && !CacheLifecycleFunctions(*loaded, error)) {
        ReleaseCachedFunctions(*loaded);
        if (loaded->Object) {
            loaded->Object->Release();
            loaded->Object = nullptr;
        }
        m_Manager->UnloadModule(moduleName.c_str(), nullptr);
        return false;
    }

    if (ScriptEntryValidateOnly()) {
        module = std::move(loaded);
        return true;
    }

    if (!runtimeLifecycle) {
        module = std::move(loaded);
        return true;
    }

    ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                                         loaded->Meta,
                                                                         "OnLoad",
                                                                         kLoadingState,
                                                                         loaded->Generation,
                                                                         m_FrameIndex,
                                                                         0.0f,
                                                                         0.0f);
    InvokeStatus status = Invoke(*loaded, "OnLoad", ctx);
    if (status == InvokeStatus::Failed) {
        error = fmt::format("Runtime entry '{}' failed during load: {}", metadata.Id, loaded->Error);
        DestroyModule(*loaded, true);
        return false;
    }
    if (status == InvokeStatus::Finished) {
        loaded->InitialLifecycle.Complete(ScriptInitialPhase::OnLoad);
        ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                        loaded->Meta,
                                                        "Awake",
                                                        kLoadingState,
                                                        loaded->Generation,
                                                        m_FrameIndex,
                                                        0.0f,
                                                        0.0f);
        status = Invoke(*loaded, "Awake", ctx);
        if (status == InvokeStatus::Failed) {
            error = fmt::format("Runtime entry '{}' failed during load: {}", metadata.Id, loaded->Error);
            DestroyModule(*loaded, true);
            return false;
        }
        if (status == InvokeStatus::Finished) {
            loaded->InitialLifecycle.Complete(ScriptInitialPhase::Awake);
        }
    }
    module = std::move(loaded);
    return true;
}

bool ScriptRuntime::CacheLifecycleFunctions(Module &module, std::string &error) const {
    asIScriptModule *scriptModule = module.Cached ? module.Cached->module : nullptr;
    if (!scriptModule) {
        return true;
    }
    ReleaseCachedFunctions(module);
    for (int i = 0; i < LifecycleCount; ++i) {
        const char *name = kLifecycleNames[i];
        const std::string expected = fmt::format("void {}(const ScriptContext &in ctx)", name);
        asIScriptFunction *expectedFunc = module.Type
            ? module.Type->GetMethodByDecl(expected.c_str())
            : scriptModule->GetFunctionByDecl(expected.c_str());
        if (expectedFunc) {
            expectedFunc->AddRef();
            module.Lifecycle[i] = expectedFunc;
        }
        if (module.Type) {
            const asUINT count = module.Type->GetMethodCount();
            for (asUINT i = 0; i < count; ++i) {
                asIScriptFunction *method = module.Type->GetMethodByIndex(i);
                if (method && std::string(method->GetName()) == name && method != expectedFunc) {
                    error = fmt::format("Runtime entry '{}' lifecycle '{}' has invalid signature '{}'; expected '{}'.",
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
                    error = fmt::format("Runtime entry '{}' lifecycle '{}' has invalid signature '{}'; expected '{}'.",
                                        module.Meta.Id,
                                        name,
                                        function->GetDeclaration(),
                                        expected);
                    return false;
                }
            }
        }
    }
    const std::string expectedMessage = "void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx)";
    asIScriptFunction *expectedMessageFunc = module.Type
        ? module.Type->GetMethodByDecl(expectedMessage.c_str())
        : scriptModule->GetFunctionByDecl(expectedMessage.c_str());
    if (expectedMessageFunc) {
        expectedMessageFunc->AddRef();
        module.OnMessage = expectedMessageFunc;
    }
    if (module.Type) {
        const asUINT count = module.Type->GetMethodCount();
        for (asUINT i = 0; i < count; ++i) {
            asIScriptFunction *method = module.Type->GetMethodByIndex(i);
            if (method && std::string(method->GetName()) == "OnMessage" && method != expectedMessageFunc) {
                error = fmt::format("Runtime entry '{}' message handler has invalid signature '{}'; expected '{}'.",
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
                error = fmt::format("Runtime entry '{}' message handler has invalid signature '{}'; expected '{}'.",
                                    module.Meta.Id,
                                    function->GetDeclaration(),
                                    expectedMessage);
                return false;
            }
        }
    }
    return true;
}

void ScriptRuntime::ReleaseCachedFunctions(Module &module) const {
    for (int i = 0; i < LifecycleCount; ++i) {
        ReleaseScriptFunction(module.Lifecycle[i]);
    }
    ReleaseScriptFunction(module.OnMessage);
}

void ScriptRuntime::SubscribeStaticTopics(Module &module) const {
    if (!UsesRuntimeLifecycle(module.Meta) ||
        !m_Manager || !m_Manager->GetMessageBus() || module.Failed || !module.Loaded) {
        return;
    }
    std::string subscribeError;
    for (const std::string &topic : module.Meta.MessageTopics) {
        m_Manager->GetMessageBus()->Subscribe(module.MessageTarget, topic, true, subscribeError);
    }
}

void ScriptRuntime::DiscardPendingReplacement(Module &module) {
    if (!module.PendingReplacement) {
        return;
    }
    DestroyModule(*module.PendingReplacement, true);
    module.PendingReplacement.reset();
}

bool ScriptRuntime::ReplaceModule(const ScriptRuntimeManifest &metadata, std::unique_ptr<Module> module) {
    for (std::unique_ptr<Module> &existing : m_Modules) {
        if (existing && existing->Meta.Id == metadata.Id) {
            DiscardPendingReplacement(*existing);
            if (DestroyModule(*existing)) {
                existing = std::move(module);
                SubscribeStaticTopics(*existing);
                m_ModulesById[metadata.Id] = existing.get();
            } else {
                existing->PendingReplacement = std::move(module);
            }
            return true;
        }
    }
    m_Modules.push_back(std::move(module));
    SubscribeStaticTopics(*m_Modules.back());
    m_ModulesById[metadata.Id] = m_Modules.back().get();
    return true;
}

bool ScriptRuntime::DestroyModule(Module &module, bool hard) {
    if (hard || module.PendingErase) {
        DiscardPendingReplacement(module);
    }
    if (m_Manager && m_Manager->GetMessageBus()) {
        m_Manager->GetMessageBus()->ClearTarget(module.MessageTarget, "Script entry was destroyed.");
    }
    if (hard) {
        CancelActiveInvocation(module);
    }
    module.PendingDestroy = !hard;
    if (UsesRuntimeLifecycle(module.Meta) && module.Loaded && !module.Failed) {
        ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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
        ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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
    ReleaseCachedFunctions(module);
    if (module.Object) {
        module.Object->Release();
        module.Object = nullptr;
    }
    if (m_Manager && !module.ModuleName.empty()) {
        CKAngelScriptResult unloadResult = {};
        const CKAS_STATUS unloadStatus = m_Manager->UnloadOwnedModule(
            module.ModuleName.c_str(),
            ScriptModuleMutationOrigin::RuntimeEntryRetirement,
            &unloadResult);
        if (unloadStatus != CKAS_OK && unloadStatus != CKAS_NOTFOUND) {
            module.PendingDestroy = true;
            module.Error = unloadResult.ErrorMessage && unloadResult.ErrorMessage[0] != '\0'
                ? unloadResult.ErrorMessage
                : fmt::format("Runtime Entry module '{}' could not be retired.", module.ModuleName);
            OutputDiagnostic(fmt::format("{}: {}", module.Meta.Id, module.Error));
            return false;
        }
    }
    module.Cached.reset();
    module.Loaded = false;
    module.PendingDestroy = false;
    module.PendingDisable = false;
    module.PendingPause = false;
    module.PendingResume = false;
    module.PendingReset = false;
    return true;
}

bool ScriptRuntime::DisableModule(Module &module) {
    if (!UsesRuntimeLifecycle(module.Meta)) {
        module.Enabled = false;
        module.PendingDisable = false;
        if (m_Manager && !module.ModuleName.empty()) {
            m_Manager->ReleaseExecutorStatesForModule(module.ModuleName.c_str());
        }
        return true;
    }
    if (m_Manager && m_Manager->GetMessageBus()) {
        m_Manager->GetMessageBus()->ClearDynamicSubscriptions(module.MessageTarget);
        m_Manager->GetMessageBus()->FailPendingForTarget(module.MessageTarget, "Runtime entry was disabled.");
    }
    if (!module.PendingDisable) {
        CancelActiveInvocation(module);
    }
    module.PendingDisable = true;
    if (!module.EnableCalled) {
        module.PendingDisable = false;
        return true;
    }
    ScriptContext ctx = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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

bool ScriptRuntime::PauseModule(Module &module, const ScriptContext &context) {
    if (!UsesRuntimeLifecycle(module.Meta)) {
        module.PendingPause = false;
        return true;
    }
    if (!module.PendingPause) {
        CancelActiveInvocation(module);
    }
    module.PendingPause = true;
    if (!module.Enabled) {
        module.PendingPause = false;
        return true;
    }

    auto makeDisableContext = [&]() {
        return MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                  module.Meta,
                                  "OnDisable",
                                  ModuleState(module),
                                  module.Generation,
                                  m_FrameIndex,
                                  context.DeltaTime(),
                                  context.TimeSeconds());
    };

    if (module.ActiveCall && module.ActiveInvocation == "OnDisable") {
        const InvokeStatus status = Invoke(module, "OnDisable", makeDisableContext());
        if (status == InvokeStatus::Suspended) {
            return false;
        }
        if (module.EnableCalled) {
            module.EnableCalled = false;
        }
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
        status = Invoke(module, "OnDisable", makeDisableContext());
        if (status == InvokeStatus::Suspended) {
            return false;
        }
        module.EnableCalled = false;
    }
    module.PendingPause = false;
    return true;
}

bool ScriptRuntime::ResumeModule(Module &module, const ScriptContext &context) {
    if (!UsesRuntimeLifecycle(module.Meta) || !module.Loaded || module.Failed || !module.Enabled) {
        module.PendingResume = false;
        return true;
    }

    module.PendingResume = true;
    const bool resumingEnable = module.ActiveCall && module.ActiveInvocation == "OnEnable";
    if (!resumingEnable) {
        const InvokeStatus status = Invoke(module, "OnResume", context);
        if (status == InvokeStatus::Suspended) {
            return false;
        }
        if (status == InvokeStatus::Failed) {
            module.PendingResume = false;
            return true;
        }
    }

    if (!module.EnableCalled) {
        ScriptContext enableContext = MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
                                                         module.Meta,
                                                         "OnEnable",
                                                         ModuleState(module),
                                                         module.Generation,
                                                         m_FrameIndex,
                                                         context.DeltaTime(),
                                                         context.TimeSeconds());
        const InvokeStatus status = Invoke(module, "OnEnable", enableContext);
        if (status == InvokeStatus::Suspended) {
            return false;
        }
        if (status == InvokeStatus::Finished) {
            module.EnableCalled = true;
        }
    }
    module.PendingResume = false;
    return true;
}

bool ScriptRuntime::ResetModule(Module &module, const ScriptContext &context) {
    if (!UsesRuntimeLifecycle(module.Meta)) {
        module.PendingResume = false;
        module.PendingReset = false;
        return true;
    }
    if (!module.PendingReset) {
        CancelActiveInvocation(module);
        module.PendingResume = false;
    }
    module.PendingReset = true;
    const InvokeStatus status = Invoke(module, "OnReset", context);
    if (status == InvokeStatus::Suspended) {
        return false;
    }
    module.InitialLifecycle.ResetStart();
    module.PendingReset = false;
    return true;
}

const char *ScriptRuntime::ModuleState(const Module &module) const {
    if (module.Failed) {
        return kStateFailed;
    }
    if (module.PendingDestroy) {
        return kStateDestroying;
    }
    if (module.PendingDisable) {
        return kStateDisabling;
    }
    if (module.PendingPause || m_Paused) {
        return kStatePaused;
    }
    if (module.PendingResume) {
        return kStateResuming;
    }
    if (module.PendingReset) {
        return kStateResetting;
    }
    if (module.ActiveCall) {
        return kStateSuspended;
    }
    if (!module.Loaded) {
        return kStateUnloaded;
    }
    if (!module.Enabled) {
        return kStateDisabled;
    }
    if (module.EnableCalled) {
        return kStateEnabled;
    }
    return kStateLoaded;
}

ScriptRuntime::Module *ScriptRuntime::FindModule(const std::string &id) const {
    const std::string canonical = ScriptRuntimeMetadata::SanitizeId(id);
    auto it = m_ModulesById.find(canonical);
    if (it == m_ModulesById.end() && id != canonical) {
        it = m_ModulesById.find(id);
    }
    return it != m_ModulesById.end() ? it->second : nullptr;
}

RuntimeScriptInfo ScriptRuntime::BuildInfo(const Module &module) const {
    RuntimeScriptInfo info;
    info.m_Exists = true;
    info.m_Id = module.Meta.Id;
    info.m_Name = module.Meta.Name;
    info.m_Version = module.Meta.VersionText;
    info.m_Host = ScriptRuntimeMetadata::HostText(module.Meta.Host);
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

void ScriptRuntime::RebuildModuleIndex() {
    m_ModulesById.clear();
    m_ModulesById.reserve(m_Modules.size());
    for (const std::unique_ptr<Module> &module : m_Modules) {
        if (module) {
            m_ModulesById[module->Meta.Id] = module.get();
        }
    }
}

void ScriptRuntime::FinalizePendingModules() {
    bool changed = false;
    for (auto it = m_Modules.begin(); it != m_Modules.end();) {
        Module *module = it->get();
        if (!module) {
            it = m_Modules.erase(it);
            changed = true;
            continue;
        }
        if (module->PendingErase && !module->PendingDestroy && !module->ActiveCall) {
            DiscardPendingReplacement(*module);
            it = m_Modules.erase(it);
            changed = true;
            continue;
        }
        if (module->PendingReplacement && !module->PendingDestroy && !module->ActiveCall) {
            *it = std::move(module->PendingReplacement);
            SubscribeStaticTopics(**it);
            changed = true;
            ++it;
            continue;
        }
        ++it;
    }
    if (changed) {
        RebuildModuleIndex();
    }
}

void ScriptRuntime::UpdateModule(Module &module, float deltaTime, float timeSeconds) {
    if (!UsesRuntimeLifecycle(module.Meta) || !module.Loaded || module.Failed) {
        return;
    }
    auto makeContext = [&](const char *phase) {
        return MakeRuntimeContext(m_Manager ? m_Manager->GetCKContext() : nullptr,
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
        ScriptContext ctx = makeContext("OnPause");
        PauseModule(module, ctx);
        return;
    }
    if (module.PendingReset) {
        ScriptContext ctx = makeContext("OnReset");
        ResetModule(module, ctx);
        return;
    }
    if (module.PendingResume) {
        ScriptContext ctx = makeContext("OnResume");
        ResumeModule(module, ctx);
        return;
    }
    if (module.InitialLifecycle.Next() == ScriptInitialPhase::OnLoad) {
        ScriptContext ctx = makeContext("OnLoad");
        const InvokeStatus status = Invoke(module, "OnLoad", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.InitialLifecycle.Complete(ScriptInitialPhase::OnLoad);
    }
    if (module.InitialLifecycle.Next() == ScriptInitialPhase::Awake) {
        ScriptContext ctx = makeContext("Awake");
        const InvokeStatus status = Invoke(module, "Awake", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.InitialLifecycle.Complete(ScriptInitialPhase::Awake);
    }
    if (!module.Enabled) {
        return;
    }
    if (!module.EnableCalled) {
        ScriptContext ctx = makeContext("OnEnable");
        const InvokeStatus status = Invoke(module, "OnEnable", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.EnableCalled = true;
    }
    if (module.InitialLifecycle.Next() == ScriptInitialPhase::Start) {
        ScriptContext ctx = makeContext("Start");
        const InvokeStatus status = Invoke(module, "Start", ctx);
        if (status != InvokeStatus::Finished) {
            return;
        }
        module.InitialLifecycle.Complete(ScriptInitialPhase::Start);
    }
    ScriptContext ctx = makeContext("Update");
    Invoke(module, "Update", ctx);
}

ScriptRuntime::InvokeStatus ScriptRuntime::InvokeMessage(Module &module, const ScriptMessage &message, const ScriptContext &context) {
    if (!module.OnMessage) {
        return InvokeStatus::Finished;
    }
    if (!module.ActiveCall) {
        module.ActiveCall = std::make_unique<ScriptInvocationContext>(m_Manager);
        module.ActiveInvocation = "OnMessage";
        module.MessageStorage = message;
    }
    module.ContextStorage = context;

    ScriptInvocationSpec spec;
    spec.Label = "OnMessage";
    spec.Prepare = [&module](asIScriptContext *invocationContext) {
        int result = module.Object ? invocationContext->SetObject(module.Object) : asSUCCESS;
        if (result >= 0) {
            result = invocationContext->SetArgObject(0, &module.MessageStorage);
        }
        if (result >= 0) {
            result = invocationContext->SetArgObject(1, &module.ContextStorage);
        }
        return result >= 0;
    };
    const ScriptInvocationStatus status = module.ActiveCall->IsSuspended()
        ? module.ActiveCall->Resume(spec)
        : module.ActiveCall->Start(module.OnMessage, spec);
    if (status == ScriptInvocationStatus::Suspended) {
        return InvokeStatus::Suspended;
    }
    if (status == ScriptInvocationStatus::Failed) {
        const std::string invocationError = module.ActiveCall->GetStackTrace().empty()
            ? module.ActiveCall->GetErrorMessage()
            : module.ActiveCall->GetStackTrace();
        CancelActiveInvocation(module);
        SetModuleError(module, invocationError);
        return InvokeStatus::Failed;
    }
    CancelActiveInvocation(module);
    return InvokeStatus::Finished;
}

ScriptRuntime::InvokeStatus ScriptRuntime::Invoke(Module &module, const char *name, const ScriptContext &context, bool required) {
    const int lifecycleIndex = LifecycleIndexByName(name);
    if (lifecycleIndex < 0) {
        return required ? InvokeStatus::Failed : InvokeStatus::Finished;
    }

    if (module.ActiveCall && module.ActiveInvocation != name) {
        const std::string active = module.ActiveInvocation;
        const ScriptContext activeContext = module.ContextStorage;
        const InvokeStatus activeStatus = active == "OnMessage"
            ? InvokeMessage(module, module.MessageStorage, activeContext)
            : Invoke(module, active.c_str(), activeContext, false);
        if (activeStatus != InvokeStatus::Finished) {
            return activeStatus;
        }
    }

    asIScriptFunction *func = module.Lifecycle[lifecycleIndex];
    if (!func) {
        return required ? InvokeStatus::Failed : InvokeStatus::Finished;
    }

    if (!module.ActiveCall) {
        module.ActiveCall = std::make_unique<ScriptInvocationContext>(m_Manager);
        module.ActiveInvocation = name;
    }

    module.ContextStorage = context;
    ScriptInvocationSpec spec;
    spec.Label = name;
    spec.Prepare = [&module, func](asIScriptContext *invocationContext) {
        int result = module.Object ? invocationContext->SetObject(module.Object) : asSUCCESS;
        if (result >= 0 && func->GetParamCount() > 0) {
            result = invocationContext->SetArgObject(0, &module.ContextStorage);
        }
        return result >= 0;
    };
    const ScriptInvocationStatus status = module.ActiveCall->IsSuspended()
        ? module.ActiveCall->Resume(spec)
        : module.ActiveCall->Start(func, spec);
    if (status == ScriptInvocationStatus::Suspended) {
        return InvokeStatus::Suspended;
    }
    if (status == ScriptInvocationStatus::Failed) {
        const std::string invocationError = module.ActiveCall->GetStackTrace().empty()
            ? module.ActiveCall->GetErrorMessage()
            : module.ActiveCall->GetStackTrace();
        CancelActiveInvocation(module);
        SetModuleError(module, invocationError);
        return InvokeStatus::Failed;
    }

    CancelActiveInvocation(module);
    return InvokeStatus::Finished;
}

bool ScriptRuntime::InvokeFinished(Module &module, const char *name, const ScriptContext &context, bool required) {
    return Invoke(module, name, context, required) == InvokeStatus::Finished;
}

void ScriptRuntime::CancelActiveInvocation(Module &module) {
    module.ActiveCall.reset();
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
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeDependencyInfo *self) { new(self) RuntimeDependencyInfo(); },
                                                     (RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_CONSTRUCT, "void f(const RuntimeDependencyInfo &in other)",
                                        asFUNCTIONPR([](const RuntimeDependencyInfo &other, RuntimeDependencyInfo *self) {
                                                         new(self) RuntimeDependencyInfo(other);
                                                     },
                                                     (const RuntimeDependencyInfo &, RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeDependencyInfo", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeDependencyInfo *self) { self->~RuntimeDependencyInfo(); },
                                                     (RuntimeDependencyInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "RuntimeDependencyInfo &opAssign(const RuntimeDependencyInfo &in other)",
                                     asMETHODPR(RuntimeDependencyInfo, operator=, (const RuntimeDependencyInfo &), RuntimeDependencyInfo &),
                                     asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Raw() const", asMETHOD(RuntimeDependencyInfo, Raw), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Id() const", asMETHOD(RuntimeDependencyInfo, Id), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Operator() const", asMETHOD(RuntimeDependencyInfo, Operator), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string Version() const", asMETHOD(RuntimeDependencyInfo, Version), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "string ActualVersion() const", asMETHOD(RuntimeDependencyInfo, ActualVersion), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Optional() const", asMETHOD(RuntimeDependencyInfo, Optional), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Present() const", asMETHOD(RuntimeDependencyInfo, Present), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeDependencyInfo", "bool Satisfied() const", asMETHOD(RuntimeDependencyInfo, Satisfied), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectType("RuntimeScriptInfo",
                                   sizeof(RuntimeScriptInfo),
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeScriptInfo *self) { new(self) RuntimeScriptInfo(); },
                                                     (RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_CONSTRUCT, "void f(const RuntimeScriptInfo &in other)",
                                        asFUNCTIONPR([](const RuntimeScriptInfo &other, RuntimeScriptInfo *self) {
                                                         new(self) RuntimeScriptInfo(other);
                                                     },
                                                     (const RuntimeScriptInfo &, RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("RuntimeScriptInfo", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](RuntimeScriptInfo *self) { self->~RuntimeScriptInfo(); },
                                                     (RuntimeScriptInfo *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "RuntimeScriptInfo &opAssign(const RuntimeScriptInfo &in other)",
                                     asMETHODPR(RuntimeScriptInfo, operator=, (const RuntimeScriptInfo &), RuntimeScriptInfo &),
                                     asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Exists() const", asMETHOD(RuntimeScriptInfo, Exists), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Id() const", asMETHOD(RuntimeScriptInfo, Id), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Name() const", asMETHOD(RuntimeScriptInfo, Name), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Version() const", asMETHOD(RuntimeScriptInfo, Version), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Host() const", asMETHOD(RuntimeScriptInfo, Host), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Description() const", asMETHOD(RuntimeScriptInfo, Description), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Author() const", asMETHOD(RuntimeScriptInfo, Author), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Category() const", asMETHOD(RuntimeScriptInfo, Category), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "int TagCount() const", asMETHOD(RuntimeScriptInfo, TagCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Tag(int index) const", asMETHOD(RuntimeScriptInfo, Tag), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Enabled() const", asMETHOD(RuntimeScriptInfo, Enabled), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Loaded() const", asMETHOD(RuntimeScriptInfo, Loaded), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "bool Failed() const", asMETHOD(RuntimeScriptInfo, Failed), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string State() const", asMETHOD(RuntimeScriptInfo, State), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Phase() const", asMETHOD(RuntimeScriptInfo, Phase), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Error() const", asMETHOD(RuntimeScriptInfo, Error), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Root() const", asMETHOD(RuntimeScriptInfo, Root), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Manifest() const", asMETHOD(RuntimeScriptInfo, Manifest), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "string Entry() const", asMETHOD(RuntimeScriptInfo, Entry), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("RuntimeScriptInfo", "int Generation() const", asMETHOD(RuntimeScriptInfo, Generation), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    r = engine->RegisterObjectType("ScriptContext",
                                   sizeof(ScriptContext),
                                   asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("ScriptContext", asBEHAVE_CONSTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptContext *self) { new(self) ScriptContext(); },
                                                     (ScriptContext *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("ScriptContext", asBEHAVE_CONSTRUCT, "void f(const ScriptContext &in other)",
                                        asFUNCTIONPR([](const ScriptContext &other, ScriptContext *self) {
                                                         new(self) ScriptContext(other);
                                                     },
                                                     (const ScriptContext &, ScriptContext *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectBehaviour("ScriptContext", asBEHAVE_DESTRUCT, "void f()",
                                        asFUNCTIONPR([](ScriptContext *self) { self->~ScriptContext(); },
                                                     (ScriptContext *), void),
                                        asCALL_CDECL_OBJLAST); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "ScriptContext &opAssign(const ScriptContext &in other)",
                                     asMETHODPR(ScriptContext, operator=, (const ScriptContext &), ScriptContext &),
                                     asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "CKContext@ Context() const", asMETHOD(ScriptContext, Context), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "float DeltaTime() const", asMETHOD(ScriptContext, DeltaTime), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "float TimeSeconds() const", asMETHOD(ScriptContext, TimeSeconds), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Id() const", asMETHOD(ScriptContext, Id), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Name() const", asMETHOD(ScriptContext, Name), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Version() const", asMETHOD(ScriptContext, Version), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Root() const", asMETHOD(ScriptContext, Root), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Manifest() const", asMETHOD(ScriptContext, Manifest), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Entry() const", asMETHOD(ScriptContext, Entry), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Target() const", asMETHOD(ScriptContext, Target), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Phase() const", asMETHOD(ScriptContext, Phase), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string State() const", asMETHOD(ScriptContext, State), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "int Generation() const", asMETHOD(ScriptContext, Generation), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "uint64 FrameIndex() const", asMETHOD(ScriptContext, FrameIndex), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string Metadata(const string &in key, const string &in fallback = \"\") const", asMETHOD(ScriptContext, Metadata), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "int MetadataCount() const", asMETHOD(ScriptContext, MetadataCount), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string MetadataKey(int index) const", asMETHOD(ScriptContext, MetadataKey), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "string MetadataValue(int index) const", asMETHOD(ScriptContext, MetadataValue), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "void Raise(const string &in message) const", asMETHOD(ScriptContext, Raise), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterObjectMethod("ScriptContext", "CKBehaviorContext ToBehaviorContext() const", asMETHOD(ScriptContext, ToBehaviorContext), asCALL_THISCALL); CKAS_CHECK_REGISTER(r);

    RegisterScriptSceneRuntime(engine);

    const char *previousNamespace = engine->GetDefaultNamespace();
    const std::string previous = previousNamespace ? previousNamespace : "";

    r = engine->SetDefaultNamespace("Behavior"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorBridge@ From(const ScriptContext &in ctx)", asFUNCTION(RuntimeBehaviorFrom), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorGraph@ Graph(const ScriptContext &in ctx)", asFUNCTION(RuntimeBehaviorGraph), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorGraph@ Graph(const ScriptContext &in ctx, const string &in rootBehaviorName)", asFUNCTION(RuntimeBehaviorGraphByRoot), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ Find(const ScriptContext &in ctx, const string &in name)", asFUNCTION(RuntimeBehaviorFind), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindByID(const ScriptContext &in ctx, CK_ID id)", asFUNCTION(RuntimeBehaviorFindById), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace("BB"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BBBridge@ From(const ScriptContext &in ctx)", asFUNCTION(RuntimeBBFrom), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const ScriptContext &in ctx, const string &in query)", asFUNCTION(RuntimeBBRequireByName), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const ScriptContext &in ctx, CKGUID guid)", asFUNCTION(RuntimeBBRequireByGuid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("int Count(const ScriptContext &in ctx)", asFUNCTION(RuntimeBBCount), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BBPrototype@ At(const ScriptContext &in ctx, int index)", asFUNCTION(RuntimeBBAt), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("BBPrototype@ Find(const ScriptContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(RuntimeBBFind), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<BBPrototype@>@ FindAll(const ScriptContext &in ctx, const string &in query = \"\")", asFUNCTION(RuntimeBBFindAll), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace("Param"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("int Count(const ScriptContext &in ctx)", asFUNCTION(RuntimeParamCount), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ At(const ScriptContext &in ctx, int index)", asFUNCTION(RuntimeParamAt), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Find(const ScriptContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(RuntimeParamFind), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const ScriptContext &in ctx, const string &in typeName)", asFUNCTION(RuntimeParamTypeByName), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("ParamTypeInfo@ Type(const ScriptContext &in ctx, CKGUID guid)", asFUNCTION(RuntimeParamTypeByGuid), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace("Runtime"); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool ReloadAll(const ScriptContext &in ctx)", asFUNCTION(RuntimeReloadAll), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Reload(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeReload), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("bool Enable(const ScriptContext &in ctx, const string &in id, bool enabled)", asFUNCTION(RuntimeEnable), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<string>@ List(const ScriptContext &in ctx)", asFUNCTION(RuntimeList), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<RuntimeScriptInfo>@ ListInfo(const ScriptContext &in ctx)", asFUNCTION(RuntimeListInfo), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("RuntimeScriptInfo Info(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeInfo), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string Version(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeVersion), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("string Metadata(const ScriptContext &in ctx, const string &in id, const string &in key, const string &in fallback = \"\")", asFUNCTION(RuntimeMetadata), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<string>@ Dependencies(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeDependencies), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<RuntimeDependencyInfo>@ RequiredDependencies(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeRequiredDependencies), asCALL_CDECL); CKAS_CHECK_REGISTER(r);
    r = engine->RegisterGlobalFunction("array<RuntimeDependencyInfo>@ OptionalDependencies(const ScriptContext &in ctx, const string &in id)", asFUNCTION(RuntimeOptionalDependencies), asCALL_CDECL); CKAS_CHECK_REGISTER(r);

    r = engine->SetDefaultNamespace(previous.c_str()); CKAS_CHECK_REGISTER(r);
}
