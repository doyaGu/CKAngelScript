#include "ScriptSelfTests.h"

#include <string>

#include "CKAngelScript.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"
#include "ScriptRuntimeDependency.h"
#include "ScriptRuntimeMetadata.h"

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
    if (!ScriptMessageSelfTest::RunScriptMessageSelfTest(context, engine, error)) {
        return false;
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
        "CK3dEntity@ __ckas_entity3d_raw(Entity3DRef@ ref) {\n"
        "  ObjectRef@ object = ref;\n"
        "  return object !is null ? cast<CK3dEntity>(object.Object()) : null;\n"
        "}\n"
        "CK2dEntity@ __ckas_entity2d_raw(Entity2DRef@ ref) {\n"
        "  ObjectRef@ object = ref;\n"
        "  return object !is null ? cast<CK2dEntity>(object.Object()) : null;\n"
        "}\n"
        "CKMaterial@ __ckas_material_raw(MaterialRef@ ref) {\n"
        "  ObjectRef@ object = ref;\n"
        "  return object !is null ? cast<CKMaterial>(object.Object()) : null;\n"
        "}\n"
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
        "  if (e3d !is null) { CK3dEntity@ raw3d = __ckas_entity3d_raw(e3d); if (raw3d !is null) { VxVector p; VxQuaternion q; raw3d.SetPosition(p); raw3d.SetPosition(0.0f, 0.0f, 0.0f); raw3d.GetPosition(p); raw3d.Translate(p); raw3d.Translate(0.0f, 0.0f, 0.0f); raw3d.SetQuaternion(q); raw3d.GetQuaternion(q); raw3d.SetScale(p); raw3d.SetScale(1.0f, 1.0f, 1.0f); raw3d.GetScale(p); raw3d.LookAt(p); raw3d.GetParent(); raw3d.GetChild(0); raw3d.GetChildrenCount(); raw3d.SetParent(null); raw3d.AddChild(null); raw3d.RemoveChild(null); raw3d.SetPickable(); raw3d.IsPickable(); CKMesh@ rawMesh = oneMesh !is null ? oneMesh.Mesh() : null; raw3d.GetCurrentMesh(); raw3d.SetCurrentMesh(rawMesh); raw3d.GetMeshCount(); raw3d.GetMesh(0); raw3d.AddMesh(rawMesh); } }\n"
        "  if (e2d !is null) { CK2dEntity@ raw2d = __ckas_entity2d_raw(e2d); if (raw2d !is null) { Vx2DVector p2; VxRect r2; raw2d.SetPosition(p2); raw2d.GetPosition(p2); raw2d.SetSize(p2); raw2d.GetSize(p2); raw2d.SetRect(r2); raw2d.GetRect(r2); raw2d.SetSourceRect(r2); raw2d.GetSourceRect(r2); raw2d.UseSourceRect(); raw2d.IsUsingSourceRect(); CKMaterial@ rawMaterial = oneMaterial !is null ? __ckas_material_raw(oneMaterial) : null; raw2d.SetMaterial(rawMaterial); raw2d.GetMaterial(); raw2d.SetPickable(); raw2d.IsPickable(); raw2d.SetClipToParent(); raw2d.IsClipToParent(); raw2d.GetParent(); raw2d.GetChild(0); raw2d.GetChildrenCount(); raw2d.SetParent(null); } }\n"
        "  MaterialRef@ material = Scene::FindMaterial(ctx, \"__missing__\");\n"
        "  TextureRef@ texture = Scene::FindTexture(ctx, \"__missing__\");\n"
        "  MeshRef@ mesh = Scene::FindMesh(ctx, \"__missing__\");\n"
        "  BehaviorRef@ behaviorSceneRef = Scene::FindBehavior(ctx, \"__missing__\");\n"
        "  if (material !is null) { CKMaterial@ rawMaterial = __ckas_material_raw(material); CKTexture@ rawTexture = texture !is null ? texture.Texture() : null; if (rawMaterial !is null) { rawMaterial.GetTexture(); rawMaterial.SetTexture(rawTexture); } }\n"
        "  if (found !is null) { found.IsValid(); bool valid = found.valid; found.Error(); found.Describe(); found.Id(); found.Name(); found.ClassId(); found.IsDynamic(); found.Object(); BehaviorRef@ castBehavior = cast<BehaviorRef>(found); ParamRef@ castParam = cast<ParamRef>(found); SceneObjectRef@ castSceneObject = cast<SceneObjectRef>(found); }\n"
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
        "  if (behaviorContext.AttributeManager !is null || behaviorContext.TimeManager !is null || behaviorContext.CallbackArg != 0) return 13;\n"
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
    return true;
}
