#include "ScriptSelfTests.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include "CKAngelScript.h"
#include "ScriptAsync.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(const char *name, const std::string &value) : m_Name(name ? name : "") {
        if (m_Name.empty()) {
            return;
        }
        char *oldValue = nullptr;
        size_t oldSize = 0;
        if (_dupenv_s(&oldValue, &oldSize, m_Name.c_str()) == 0 && oldValue) {
            m_HadValue = true;
            m_OldValue = oldValue;
            std::free(oldValue);
        }
        _putenv_s(m_Name.c_str(), value.c_str());
    }

    ~ScopedEnvVar() {
        if (m_Name.empty()) {
            return;
        }
        _putenv_s(m_Name.c_str(), m_HadValue ? m_OldValue.c_str() : "");
    }

    ScopedEnvVar(const ScopedEnvVar &) = delete;
    ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

private:
    std::string m_Name;
    std::string m_OldValue;
    bool m_HadValue = false;
};

bool WriteRuntimeSelfTestFile(const std::filesystem::path &path, const std::string &text, std::string &error) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        error = "Runtime self-test failed to create script file: " + path.string();
        return false;
    }
    output << text;
    if (!output) {
        error = "Runtime self-test failed to write script file: " + path.string();
        return false;
    }
    return true;
}

bool PrepareRuntimeSelfTestPackage(const std::filesystem::path &root,
                                   const std::string &id,
                                   const std::string &source,
                                   std::string &error) {
    namespace fs = std::filesystem;
    const fs::path package = root / "Scripts" / id;
    std::error_code fsError;
    fs::create_directories(package, fsError);
    if (fsError) {
        error = "Runtime self-test failed to create script package: " + package.string();
        return false;
    }

    const std::string manifest = "[script id=\"" + id +
                                 "\" name=\"Runtime Self Test\" version=\"1.0.0\" entry=\"runtime.as\"]\n";
    return WriteRuntimeSelfTestFile(package / "script.as", manifest, error) &&
           WriteRuntimeSelfTestFile(package / "runtime.as", source, error);
}

bool ReadRuntimeGlobalInt(ScriptManager *manager, const std::string &moduleName, const char *name, int &value) {
    asIScriptModule *module = manager && !moduleName.empty() ? manager->GetModule(moduleName.c_str()) : nullptr;
    const int index = module && name ? module->GetGlobalVarIndexByName(name) : -1;
    void *address = index >= 0 ? module->GetAddressOfGlobalVar(static_cast<asUINT>(index)) : nullptr;
    if (!address) {
        return false;
    }
    value = *static_cast<int *>(address);
    return true;
}

std::string RuntimeSelfTestModuleName(const std::string &id, int generation) {
    return "__CKASRuntime_" + id + "_" + std::to_string(generation);
}

bool RunRuntimePauseLifecycleSelfTest(ScriptManager *manager, std::string &error) {
    if (!manager || !manager->GetScriptEngine()) {
        error = "Runtime pause lifecycle self-test requires an initialized ScriptManager.";
        return false;
    }

    namespace fs = std::filesystem;
    constexpr const char *scriptId = "ckas.runtime.pause.selftest";
    const fs::path root = fs::temp_directory_path() / "ckas-runtime-pause-lifecycle-selftest";
    std::error_code fsError;
    fs::remove_all(root, fsError);
    fsError.clear();

    const ScopedEnvVar scriptRoots("CKAS_SCRIPT_ROOTS", root.string());
    const ScopedEnvVar validateOnly("CKAS_SCRIPT_VALIDATE_ONLY", "0");
    const std::string source =
        "int pauseCount = 0;\n"
        "int disableCount = 0;\n"
        "int badPhaseCount = 0;\n"
        "void OnPause(const ScriptContext &in ctx) {\n"
        "  if (ctx.Phase() != \"OnPause\") { badPhaseCount++; }\n"
        "  pauseCount++;\n"
        "}\n"
        "void OnDisable(const ScriptContext &in ctx) {\n"
        "  if (ctx.Phase() != \"OnDisable\") { badPhaseCount++; }\n"
        "  disableCount++;\n"
        "  AsyncTask<void>@ delay = Async::Delay(1);\n"
        "  Await(delay);\n"
        "}\n";
    if (!PrepareRuntimeSelfTestPackage(root, scriptId, source, error)) {
        fs::remove_all(root, fsError);
        return false;
    }

    ScriptRuntime runtime(manager);
    auto cleanup = [&]() {
        runtime.Clear();
        if (manager->GetAsyncScheduler()) {
            manager->GetAsyncScheduler()->Clear();
        }
        fs::remove_all(root, fsError);
    };

    std::string loadError;
    if (!runtime.ReloadAll(&loadError)) {
        cleanup();
        error = loadError.empty() ? "Runtime pause lifecycle self-test failed to load script package." : loadError;
        return false;
    }
    RuntimeScriptInfo info = runtime.Info(scriptId);
    if (!info.Exists() || !info.Loaded() || info.Failed()) {
        cleanup();
        error = "Runtime pause lifecycle self-test did not load the probe script.";
        return false;
    }

    runtime.PreProcess();
    runtime.OnPause();
    info = runtime.Info(scriptId);
    if (info.State() != "paused" || info.Phase() != "OnDisable") {
        cleanup();
        error = "Runtime pause lifecycle self-test did not retain suspended OnDisable state.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Runtime pause lifecycle self-test requires an async scheduler.";
        return false;
    }
    scheduler->Tick();
    runtime.PreProcess();

    const std::string moduleName = runtime.ModuleName(scriptId);
    int pauseCount = 0;
    int disableCount = 0;
    int badPhaseCount = 0;
    if (!ReadRuntimeGlobalInt(manager, moduleName, "pauseCount", pauseCount) ||
        !ReadRuntimeGlobalInt(manager, moduleName, "disableCount", disableCount) ||
        !ReadRuntimeGlobalInt(manager, moduleName, "badPhaseCount", badPhaseCount)) {
        cleanup();
        error = "Runtime pause lifecycle self-test could not read probe counters.";
        return false;
    }
    if (pauseCount != 1 || disableCount != 1 || badPhaseCount != 0) {
        cleanup();
        error = "Runtime pause lifecycle self-test observed pause=" + std::to_string(pauseCount) +
                ", disable=" + std::to_string(disableCount) +
                ", badPhase=" + std::to_string(badPhaseCount) + "; expected 1, 1, 0.";
        return false;
    }

    cleanup();
    return true;
}

bool RunRuntimePendingReplacementSelfTest(ScriptManager *manager, std::string &error) {
    if (!manager || !manager->GetScriptEngine()) {
        error = "Runtime pending replacement self-test requires an initialized ScriptManager.";
        return false;
    }

    namespace fs = std::filesystem;
    constexpr const char *scriptId = "ckas.runtime.pending.selftest";
    const fs::path root = fs::temp_directory_path() / "ckas-runtime-pending-replacement-selftest";
    std::error_code fsError;
    fs::remove_all(root, fsError);
    fsError.clear();

    const ScopedEnvVar scriptRoots("CKAS_SCRIPT_ROOTS", root.string());
    const ScopedEnvVar validateOnly("CKAS_SCRIPT_VALIDATE_ONLY", "0");
    const std::string oldSource =
        "void OnDestroy(const ScriptContext &in ctx) {\n"
        "  AsyncTask<void>@ delay = Async::Delay(1);\n"
        "  Await(delay);\n"
        "}\n";
    const std::string replacementSource = "int replacementLoaded = 1;\n";
    if (!PrepareRuntimeSelfTestPackage(root, scriptId, oldSource, error)) {
        fs::remove_all(root, fsError);
        return false;
    }

    ScriptRuntime runtime(manager);
    auto cleanup = [&]() {
        runtime.Clear();
        if (manager->GetAsyncScheduler()) {
            manager->GetAsyncScheduler()->Clear();
        }
        fs::remove_all(root, fsError);
    };

    std::string loadError;
    if (!runtime.ReloadAll(&loadError)) {
        cleanup();
        error = loadError.empty() ? "Runtime pending replacement self-test failed to load old script." : loadError;
        return false;
    }
    RuntimeScriptInfo oldInfo = runtime.Info(scriptId);
    const std::string oldModuleName = runtime.ModuleName(scriptId);
    if (!oldInfo.Exists() || !oldInfo.Loaded() || oldInfo.Failed() || oldModuleName.empty()) {
        cleanup();
        error = "Runtime pending replacement self-test did not load the old script.";
        return false;
    }

    if (!PrepareRuntimeSelfTestPackage(root, scriptId, replacementSource, error)) {
        cleanup();
        return false;
    }
    std::string reloadError;
    if (!runtime.Reload(scriptId, &reloadError)) {
        cleanup();
        error = reloadError.empty() ? "Runtime pending replacement self-test failed to reload replacement script." : reloadError;
        return false;
    }
    const std::string replacementModuleName = RuntimeSelfTestModuleName(scriptId, oldInfo.Generation() + 1);
    if (!manager->HasModule(replacementModuleName.c_str())) {
        cleanup();
        error = "Runtime pending replacement self-test did not retain a pending replacement module.";
        return false;
    }
    RuntimeScriptInfo destroyingInfo = runtime.Info(scriptId);
    if (destroyingInfo.State() != "destroying" || destroyingInfo.Phase() != "OnDestroy") {
        cleanup();
        error = "Runtime pending replacement self-test did not retain suspended OnDestroy state.";
        return false;
    }

    fs::remove_all(root / "Scripts" / scriptId, fsError);
    if (fsError) {
        cleanup();
        error = "Runtime pending replacement self-test failed to remove probe script package.";
        return false;
    }
    std::string reconcileError;
    if (!runtime.ReloadAll(&reconcileError)) {
        cleanup();
        error = reconcileError.empty() ? "Runtime pending replacement self-test failed to reconcile removed script." : reconcileError;
        return false;
    }
    if (manager->HasModule(replacementModuleName.c_str())) {
        cleanup();
        error = "Runtime pending replacement self-test leaked a pending replacement module after erase.";
        return false;
    }

    ScriptAsyncScheduler *scheduler = manager->GetAsyncScheduler();
    if (!scheduler) {
        cleanup();
        error = "Runtime pending replacement self-test requires an async scheduler.";
        return false;
    }
    scheduler->Tick();
    runtime.PreProcess();
    if (runtime.Info(scriptId).Exists()) {
        cleanup();
        error = "Runtime pending replacement self-test retained an erased script.";
        return false;
    }
    if (manager->HasModule(oldModuleName.c_str())) {
        cleanup();
        error = "Runtime pending replacement self-test leaked the old module after erase.";
        return false;
    }

    cleanup();
    return true;
}

} // namespace

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
    asITypeInfo *behaviorContextType = engine->GetTypeInfoByDecl("CKBehaviorContext");
    if (!behaviorContextType) {
        error = "CKBehaviorContext type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < behaviorContextType->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        if (behaviorContextType->GetProperty(i, &propertyName) >= 0 &&
            propertyName && std::strcmp(propertyName, "CallbackArg") == 0) {
            error = "CKBehaviorContext still exposes writable CallbackArg storage.";
            return false;
        }
    }
    if (!behaviorContextType->GetMethodByDecl("NativePointer get_CallbackArg() const")) {
        error = "CKBehaviorContext CallbackArg getter is not registered.";
        return false;
    }
    for (asUINT i = 0; i < behaviorContextType->GetMethodCount(); ++i) {
        asIScriptFunction *method = behaviorContextType->GetMethodByIndex(i);
        if (method && std::string(method->GetDeclaration()).find("set_CallbackArg") != std::string::npos) {
            error = "CKBehaviorContext still exposes writable CallbackArg setter.";
            return false;
        }
    }

    ScriptContext defaultContext;
    CKBehaviorContext defaultBehaviorContext = defaultContext.ToBehaviorContext();
    if (defaultBehaviorContext.Behavior ||
        defaultBehaviorContext.DeltaTime != 0.0f ||
        defaultBehaviorContext.Context ||
        defaultBehaviorContext.CurrentLevel ||
        defaultBehaviorContext.CurrentScene ||
        defaultBehaviorContext.PreviousScene ||
        defaultBehaviorContext.CurrentRenderContext ||
        defaultBehaviorContext.ParameterManager ||
        defaultBehaviorContext.MessageManager ||
        defaultBehaviorContext.AttributeManager ||
        defaultBehaviorContext.TimeManager ||
        defaultBehaviorContext.CallbackMessage != 0 ||
        defaultBehaviorContext.CallbackArg) {
        error = "Default ScriptContext produced a non-empty CKBehaviorContext.";
        return false;
    }

    const char *source =
        "void __ckas_runtime_compile_probe(const ScriptContext &in ctx) {\n"
        "  CKContext@ c = ctx.Context();\n"
        "  float dt = ctx.DeltaTime();\n"
        "  string id = ctx.Id();\n"
        "  string name = ctx.Name();\n"
        "  string version = ctx.Version();\n"
        "  string target = ctx.Target();\n"
        "  string phase = ctx.Phase();\n"
        "  string state = ctx.State();\n"
        "  int generation = ctx.Generation();\n"
        "  uint64 frame = ctx.FrameIndex();\n"
        "  string manifestPath = ctx.Manifest();\n"
        "  string entryPath = ctx.Entry();\n"
        "  string custom = ctx.Metadata(\"custom\", \"fallback\");\n"
        "  int metadataCount = ctx.MetadataCount();\n"
        "  string metadataKey = ctx.MetadataKey(0);\n"
        "  string metadataValue = ctx.MetadataValue(0);\n"
        "  CKBehaviorContext behaviorContext = ctx.ToBehaviorContext();\n"
        "  NativePointer callbackArg = behaviorContext.CallbackArg;\n"
        "  BehaviorGraph@ graph = Behavior::Graph(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behavior = Behavior::Find(ctx, \"__missing__\");\n"
        "  BBDecl@ decl = BB::Require(ctx, \"__missing__\");\n"
        "  int bbCount = BB::Count(ctx);\n"
        "  ParamTypeInfo@ param = Param::Find(ctx, \"int\");\n"
        "  LevelRef@ currentLevel = Scene::CurrentLevel(ctx);\n"
        "  SceneRef@ currentScene = Scene::CurrentScene(ctx);\n"
        "  ObjectRef@ targetRef = Scene::Target(ctx);\n"
        "  ObjectRef@ ownerRef = Scene::Owner(ctx);\n"
        "  ObjectRef@ byId = Scene::ById(ctx, 1);\n"
        "  ObjectRef@ found = Scene::Find(ctx, \"__missing__\");\n"
        "  ObjectRef@ foundScoped = Scene::Find(ctx, \"__missing__\", CKCID_OBJECT, true, 0, true);\n"
        "  ObjectRef@ foundOne = Scene::FindOne(ctx, \"__missing__\", CKCID_OBJECT, true, true);\n"
        "  array<ObjectRef@>@ sceneObjects = Scene::FindAll(ctx, \"\", CKCID_OBJECT, true, false);\n"
        "  Entity3DRef@ e3d = Scene::FindEntity3D(ctx, \"__missing__\");\n"
        "  Entity2DRef@ e2d = Scene::FindEntity2D(ctx, \"__missing__\");\n"
        "  Entity3DRef@ e3dScoped = Scene::FindEntity3D(ctx, \"__missing__\", 0, true);\n"
        "  Entity2DRef@ e2dScoped = Scene::FindEntity2D(ctx, \"__missing__\", 0, true);\n"
        "  Entity3DRef@ one3d = Scene::FindOneEntity3D(ctx, \"__missing__\", true);\n"
        "  Entity2DRef@ one2d = Scene::FindOneEntity2D(ctx, \"__missing__\", true);\n"
        "  MaterialRef@ oneMaterial = Scene::FindOneMaterial(ctx, \"__missing__\", true);\n"
        "  TextureRef@ oneTexture = Scene::FindOneTexture(ctx, \"__missing__\", true);\n"
        "  MeshRef@ oneMesh = Scene::FindOneMesh(ctx, \"__missing__\", true);\n"
        "  BehaviorRef@ oneBehavior = Scene::FindOneBehavior(ctx, \"__missing__\", true);\n"
        "  array<Entity3DRef@>@ all3d = Scene::FindAllEntity3D(ctx, \"__missing__\", true);\n"
        "  array<Entity2DRef@>@ all2d = Scene::FindAllEntity2D(ctx, \"__missing__\", true);\n"
        "  array<MaterialRef@>@ allMaterials = Scene::FindAllMaterial(ctx, \"__missing__\", true);\n"
        "  array<TextureRef@>@ allTextures = Scene::FindAllTexture(ctx, \"__missing__\", true);\n"
        "  array<MeshRef@>@ allMeshes = Scene::FindAllMesh(ctx, \"__missing__\", true);\n"
        "  array<BehaviorRef@>@ allBehaviors = Scene::FindAllBehavior(ctx, \"__missing__\", true);\n"
        "  bool inCurrent = Scene::IsInCurrentScene(ctx, found);\n"
        "  bool inScene = Scene::IsInScene(ctx, currentScene, found);\n"
        "  Scene::AddToScene(ctx, currentScene, found, true);\n"
        "  Scene::RemoveFromScene(ctx, currentScene, found, true);\n"
        "  MaterialRef@ material = Scene::FindMaterial(ctx, \"__missing__\");\n"
        "  TextureRef@ texture = Scene::FindTexture(ctx, \"__missing__\");\n"
        "  MeshRef@ mesh = Scene::FindMesh(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behaviorSceneRef = Scene::FindBehavior(ctx, \"__missing__\");\n"
        "  if (found !is null) { found.IsValid(); bool valid = found.valid; found.Error(); found.Describe(); found.Id(); found.Name(); found.ClassId(); found.IsDynamic(); BehaviorRef@ castBehavior = cast<BehaviorRef>(found); ParamRef@ castParam = cast<ParamRef>(found); SceneObjectRef@ castSceneObject = cast<SceneObjectRef>(found); }\n"
        "  SceneObjectRef@ sceneAsObject = currentScene;\n"
        "  if (sceneAsObject !is null) { Scene::IsInCurrentScene(ctx, sceneAsObject); Scene::IsInScene(ctx, currentScene, sceneAsObject); }\n"
        "  ObjectRef@ behaviorAsObject = behavior;\n"
        "  ParamInRef@ paramIn = cast<ParamInRef>(found); if (paramIn !is null) { paramIn.IsValid(); paramIn.valid; paramIn.Error(); paramIn.Describe(); paramIn.Id(); paramIn.Name(); paramIn.ClassId(); paramIn.IsDynamic(); ParamRef@ paramBase = paramIn; ObjectRef@ paramObject = paramIn; }\n"
        "  ParamOutRef@ paramOut = cast<ParamOutRef>(found); if (paramOut !is null) { paramOut.Id(); ParamRef@ paramBase = paramOut; ObjectRef@ paramObject = paramOut; }\n"
        "  ParamLocalRef@ paramLocal = cast<ParamLocalRef>(found); if (paramLocal !is null) { paramLocal.Id(); ParamRef@ paramBase = paramLocal; ObjectRef@ paramObject = paramLocal; }\n"
        "  ParamStructRef@ paramStruct = cast<ParamStructRef>(found); if (paramStruct !is null) { paramStruct.Id(); ParamRef@ paramBase = paramStruct; ObjectRef@ paramObject = paramStruct; }\n"
        "  ParamOperationRef@ paramOperation = cast<ParamOperationRef>(found); if (paramOperation !is null) { paramOperation.Id(); ObjectRef@ operationObject = paramOperation; }\n"
        "  BehaviorLinkRef@ behaviorLink = cast<BehaviorLinkRef>(found); if (behaviorLink !is null) { behaviorLink.Id(); ObjectRef@ linkObject = behaviorLink; }\n"
        "  array<ObjectRef@> selectRefs;\n"
        "  selectRefs.insertLast(behaviorAsObject);\n"
        "  Scene::Select(ctx, selectRefs, true);\n"
        "  array<string>@ scripts = Runtime::List(ctx);\n"
        "  array<RuntimeScriptInfo>@ infos = Runtime::ListInfo(ctx);\n"
        "  RuntimeScriptInfo info = Runtime::Info(ctx, \"ckas.runtime.smoke\");\n"
        "  string root = info.Root();\n"
        "  string manifest = info.Manifest();\n"
        "  string entry = info.Entry();\n"
        "  bool exists = info.Exists();\n"
        "  string infoId = info.Id();\n"
        "  string infoHost = info.Host();\n"
        "  string infoState = info.State();\n"
        "  int tagCount = info.TagCount();\n"
        "  string firstTag = info.Tag(0);\n"
        "  string runtimeVersion = Runtime::Version(ctx, \"ckas.runtime.smoke\");\n"
        "  string runtimeMetadata = Runtime::Metadata(ctx, \"ckas.runtime.smoke\", \"custom\", \"fallback\");\n"
        "  array<string>@ deps = Runtime::Dependencies(ctx, \"ckas.runtime.smoke\");\n"
        "  array<RuntimeDependencyInfo>@ requiredDeps = Runtime::RequiredDependencies(ctx, \"ckas.runtime.smoke\");\n"
        "  array<RuntimeDependencyInfo>@ optionalDeps = Runtime::OptionalDependencies(ctx, \"ckas.runtime.smoke\");\n"
        "}\n"
        "class __CKAS_RuntimeLifecycleProbe {\n"
        "  void OnLoad(const ScriptContext &in ctx) {}\n"
        "  void Awake(const ScriptContext &in ctx) {}\n"
        "  void OnEnable(const ScriptContext &in ctx) {}\n"
        "  void Start(const ScriptContext &in ctx) {}\n"
        "  void Update(const ScriptContext &in ctx) {}\n"
        "  void OnPostLoad(const ScriptContext &in ctx) {}\n"
        "  void OnPostProcess(const ScriptContext &in ctx) {}\n"
        "  void OnDisable(const ScriptContext &in ctx) {}\n"
        "  void OnDestroy(const ScriptContext &in ctx) {}\n"
        "  void OnReset(const ScriptContext &in ctx) {}\n"
        "  void OnPause(const ScriptContext &in ctx) {}\n"
        "  void OnResume(const ScriptContext &in ctx) {}\n"
        "}\n"
        "int __ckas_runtime_execute_probe() {\n"
        "  ScriptContext ctx;\n"
        "  if (ctx.Context() !is null) return 1;\n"
        "  if (ctx.DeltaTime() != 0.0f || ctx.TimeSeconds() != 0.0f) return 2;\n"
        "  if (ctx.Id() != \"\" || ctx.Name() != \"\" || ctx.Version() != \"\") return 3;\n"
        "  if (ctx.Root() != \"\" || ctx.Manifest() != \"\" || ctx.Entry() != \"\") return 4;\n"
        "  if (ctx.Target() != \"\" || ctx.Phase() != \"\" || ctx.State() != \"\") return 5;\n"
        "  if (ctx.Generation() != 0 || ctx.FrameIndex() != 0) return 6;\n"
        "  if (ctx.Metadata(\"missing\", \"fallback\") != \"fallback\") return 7;\n"
        "  if (ctx.MetadataCount() != 0 || ctx.MetadataKey(0) != \"\" || ctx.MetadataValue(0) != \"\") return 8;\n"
        "  CKBehaviorContext behaviorContext = ctx.ToBehaviorContext();\n"
        "  if (behaviorContext.Behavior !is null || behaviorContext.Context !is null) return 9;\n"
        "  if (behaviorContext.DeltaTime != 0.0f || behaviorContext.CallbackMessage != 0) return 10;\n"
        "  if (behaviorContext.CurrentLevel !is null || behaviorContext.CurrentScene !is null || behaviorContext.PreviousScene !is null) return 11;\n"
        "  if (behaviorContext.CurrentRenderContext !is null || behaviorContext.ParameterManager !is null || behaviorContext.MessageManager !is null) return 12;\n"
        "  if (behaviorContext.AttributeManager !is null || behaviorContext.TimeManager !is null || !behaviorContext.CallbackArg.IsNull()) return 13;\n"
        "  RuntimeScriptInfo info;\n"
        "  RuntimeScriptInfo copied(info);\n"
        "  RuntimeScriptInfo assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.Exists() || assigned.Enabled() || assigned.Loaded() || assigned.Failed()) return 14;\n"
        "  if (assigned.Id() != \"\" || assigned.Name() != \"\" || assigned.Version() != \"\") return 15;\n"
        "  if (assigned.Description() != \"\" || assigned.Author() != \"\" || assigned.Category() != \"\") return 16;\n"
        "  if (assigned.State() != \"\" || assigned.Phase() != \"\" || assigned.Error() != \"\") return 17;\n"
        "  if (assigned.Root() != \"\" || assigned.Manifest() != \"\" || assigned.Entry() != \"\") return 18;\n"
        "  if (assigned.TagCount() != 0 || assigned.Tag(0) != \"\" || assigned.Tag(-1) != \"\") return 19;\n"
        "  if (assigned.Generation() != 0) return 20;\n"
        "  return 0;\n"
        "}\n";
    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Runtime self-test could not retrieve CKAngelScript.";
        return false;
    }
    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Runtime self-test could not retrieve ScriptManager.";
        return false;
    }
    const char *moduleName = "__CKAS_RuntimeCompileSelfTest";
    CKAngelScriptResult result = {};
    if (api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &result) != CKAS_OK) {
        error = result.ErrorMessage && result.ErrorMessage[0] != '\0'
            ? result.ErrorMessage
            : "Runtime script API compile probe failed.";
        return false;
    }
    asIScriptModule *module = manager->GetModule(moduleName);
    asIScriptFunction *function = module ? module->GetFunctionByDecl("int __ckas_runtime_execute_probe()") : nullptr;
    if (!function) {
        api->UnloadModule(moduleName, nullptr);
        error = "Runtime ScriptContext execute probe function was not found.";
        return false;
    }
    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        api->UnloadModule(moduleName, nullptr);
        error = "Runtime ScriptContext execute probe could not create an execution context.";
        return false;
    }
    int r = scriptContext->Prepare(function);
    if (r >= 0) {
        r = scriptContext->Execute();
    }
    bool executeOk = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        if (returnCode == 0) {
            executeOk = true;
        } else {
            error = "Runtime ScriptContext execute probe returned " + std::to_string(returnCode) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = std::string("Runtime ScriptContext execute probe exception: ") +
                (exception && exception[0] ? exception : "<empty>") + ".";
    } else {
        error = "Runtime ScriptContext execute probe failed with code " + std::to_string(r) + ".";
    }
    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    if (!executeOk) {
        api->UnloadModule(moduleName, nullptr);
        return false;
    }
    api->UnloadModule(moduleName, nullptr);
    const char *oldApiSource =
        "void __ckas_runtime_old_api_probe(const ScriptContext &in ctx) {\n"
        "  string id = ctx.ScriptId();\n"
        "}\n";
    const char *oldApiModuleName = "__CKAS_RuntimeOldApiNegativeSelfTest";
    result = {};
    if (api->CompileModule(oldApiModuleName, oldApiSource, CKAS_COMPILE_REPLACEEXISTING, &result) == CKAS_OK) {
        api->UnloadModule(oldApiModuleName, nullptr);
        error = "Runtime script old ScriptContext API unexpectedly compiled.";
        return false;
    }
    api->UnloadModule(oldApiModuleName, nullptr);

    if (!RunRuntimePauseLifecycleSelfTest(manager, error)) {
        return false;
    }
    if (!RunRuntimePendingReplacementSelfTest(manager, error)) {
        return false;
    }
    return true;
}
