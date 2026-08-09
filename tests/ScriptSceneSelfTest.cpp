#include "ScriptSelfTests.h"

#include <string>
#include <vector>

#include <fmt/format.h>

#include "CKAngelScript.h"
#include "ScriptHostScope.h"
#include "ScriptManager.h"
#include "ScriptRuntime.h"

namespace {

void CleanupNamedObject(CKContext *context, const char *name) {
    if (!context || !name) {
        return;
    }
    std::vector<CKObject *> objects;
    CKObject *previous = nullptr;
    while (CKObject *object = context->GetObjectByName(const_cast<CKSTRING>(name), previous)) {
        previous = object;
        objects.push_back(object);
    }
    for (CKObject *object : objects) {
        if (object->IsToBeDeleted()) {
            continue;
        }
        context->ChangeObjectDynamic(object, TRUE);
        context->DestroyObject(object);
    }
}

} // namespace

bool RunScriptSceneSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Scene self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    constexpr const char *objectName = "__CKAS_SceneInteropDynamic";
    constexpr const char *persistentName = "__CKAS_SceneInteropPersistent";
    constexpr const char *parent3DName = "__CKAS_SceneInteropParent3D";
    constexpr const char *child3DName = "__CKAS_SceneInteropChild3D";
    constexpr const char *parent2DName = "__CKAS_SceneInteropParent2D";
    constexpr const char *child2DName = "__CKAS_SceneInteropChild2D";
    constexpr const char *duplicateName = "__CKAS_SceneInteropDuplicate";
    constexpr const char *scopedMaterialName = "__CKAS_SceneInteropScopedMaterial";
    constexpr const char *scopedSpriteName = "__CKAS_SceneInteropScopedSprite";
    constexpr const char *scopedTextureName = "__CKAS_SceneInteropScopedTexture";
    constexpr const char *scoped2DName = "__CKAS_SceneInteropScoped2D";
    constexpr const char *scoped2DMaterialName = "__CKAS_SceneInteropScoped2DMaterial";
    constexpr const char *scopedMeshName = "__CKAS_SceneInteropScopedMesh";
    constexpr const char *scopedBehaviorName = "__CKAS_SceneInteropScopedBehavior";
    constexpr const char *scopeProbeBehaviorName = "__CKAS_SceneScopeProbeBehavior";
    CleanupNamedObject(context, objectName);
    CleanupNamedObject(context, persistentName);
    CleanupNamedObject(context, parent3DName);
    CleanupNamedObject(context, child3DName);
    CleanupNamedObject(context, parent2DName);
    CleanupNamedObject(context, child2DName);
    CleanupNamedObject(context, duplicateName);
    CleanupNamedObject(context, scopedMaterialName);
    CleanupNamedObject(context, scopedSpriteName);
    CleanupNamedObject(context, scopedTextureName);
    CleanupNamedObject(context, scoped2DName);
    CleanupNamedObject(context, scoped2DMaterialName);
    CleanupNamedObject(context, scopedMeshName);
    CleanupNamedObject(context, scopedBehaviorName);
    CleanupNamedObject(context, scopeProbeBehaviorName);

    CKAngelScriptApi api = CKAngelScriptApi::Get(context);
    if (!api.IsValid()) {
        error = "Scene self-test could not retrieve CKAngelScript.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_SceneInteropSelfTest";
    const char *source =
        "int Run(const CKBehaviorContext &in ctx) {\n"
        "  const string objectName = \"__CKAS_SceneInteropDynamic\";\n"
        "  const string persistentName = \"__CKAS_SceneInteropPersistent\";\n"
        "  const string parent3DName = \"__CKAS_SceneInteropParent3D\";\n"
        "  const string child3DName = \"__CKAS_SceneInteropChild3D\";\n"
        "  const string parent2DName = \"__CKAS_SceneInteropParent2D\";\n"
        "  const string child2DName = \"__CKAS_SceneInteropChild2D\";\n"
        "  const string duplicateName = \"__CKAS_SceneInteropDuplicate\";\n"
        "  const string scopedMaterialName = \"__CKAS_SceneInteropScopedMaterial\";\n"
        "  const string scopedSpriteName = \"__CKAS_SceneInteropScopedSprite\";\n"
        "  const string scopedTextureName = \"__CKAS_SceneInteropScopedTexture\";\n"
        "  const string scoped2DName = \"__CKAS_SceneInteropScoped2D\";\n"
        "  const string scoped2DMaterialName = \"__CKAS_SceneInteropScoped2DMaterial\";\n"
        "  const string scopedMeshName = \"__CKAS_SceneInteropScopedMesh\";\n"
        "  const string scopedBehaviorName = \"__CKAS_SceneInteropScopedBehavior\";\n"
        "  CKRenderContext@ renderContext = ctx.Context.GetPlayerRenderContext();\n"
        "  if (renderContext !is null) {\n"
        "    VxStats renderStats;\n"
        "    renderContext.GetStats(renderStats);\n"
        "    VxStats copiedRenderStats(renderStats);\n"
        "    VxStats assignedRenderStats;\n"
        "    assignedRenderStats = copiedRenderStats;\n"
        "  }\n"
        "  Entity3DRef@ created = Scene::CreateEntity3D(ctx, objectName, true);\n"
        "  if (created is null || !created.valid) return 10;\n"
        "  if (!created.IsDynamic() || created.Name() != objectName || created.ClassId() != CKCID_3DENTITY) return 11;\n"
        "  CK_ID createdId = created.Id();\n"
        "  ObjectRef@ objectRef = created;\n"
        "  Entity3DRef@ castCreated = cast<Entity3DRef>(objectRef);\n"
        "  if (castCreated is null || castCreated.Id() != createdId) return 12;\n"
        "  SceneObjectRef@ sceneObject = cast<SceneObjectRef>(objectRef);\n"
        "  if (sceneObject is null || sceneObject.Id() != createdId || !sceneObject.valid) return 107;\n"
        "  ObjectRef@ sceneObjectAsObject = sceneObject;\n"
        "  Entity3DRef@ sceneObjectAs3D = cast<Entity3DRef>(sceneObject);\n"
        "  if (sceneObjectAsObject is null || sceneObjectAsObject.Id() != createdId || sceneObjectAs3D is null || sceneObjectAs3D.Id() != createdId) return 108;\n"
        "  MaterialRef@ wrongMaterial = cast<MaterialRef>(objectRef);\n"
        "  BehaviorRef@ wrongBehavior = cast<BehaviorRef>(objectRef);\n"
        "  if (wrongMaterial !is null || wrongBehavior !is null) return 30;\n"
        "  for (uint castI = 0; castI < 8; ++castI) {\n"
        "    ObjectRef@ transientObject = created;\n"
        "    Entity3DRef@ transientEntity = cast<Entity3DRef>(transientObject);\n"
        "    if (transientEntity is null || transientEntity.Id() != createdId) return 105;\n"
        "    @transientEntity = null;\n"
        "    @transientObject = null;\n"
        "    if (!created.valid || created.Id() != createdId) return 106;\n"
        "  }\n"
        "  ObjectRef@ byId = Scene::ById(ctx, createdId);\n"
        "  if (byId is null || !byId.valid || byId.Id() != createdId) return 13;\n"
        "  ObjectRef@ found = Scene::Find(ctx, objectName, CKCID_3DENTITY, true, 0);\n"
        "  if (found is null || !found.valid || found.Id() != createdId) return 14;\n"
        "  Entity3DRef@ typedFound = Scene::FindEntity3D(ctx, objectName, 0);\n"
        "  if (typedFound is null || !typedFound.valid || typedFound.Id() != createdId) return 15;\n"
        "  ObjectRef@ uniqueFound = Scene::FindOne(ctx, objectName, CKCID_3DENTITY, true, false);\n"
        "  if (uniqueFound is null || !uniqueFound.valid || uniqueFound.Id() != createdId) return 61;\n"
        "  Entity3DRef@ uniqueTypedFound = Scene::FindOneEntity3D(ctx, objectName, false);\n"
        "  if (uniqueTypedFound is null || !uniqueTypedFound.valid || uniqueTypedFound.Id() != createdId) return 62;\n"
        "  array<ObjectRef@>@ all = Scene::FindAll(ctx, objectName, CKCID_3DENTITY, true, false);\n"
        "  bool sawCreated = false;\n"
        "  for (uint i = 0; all !is null && i < all.length(); ++i) { if (all[i] !is null && all[i].Id() == createdId) sawCreated = true; }\n"
        "  if (!sawCreated) return 16;\n"
        "  array<Entity3DRef@>@ all3D = Scene::FindAllEntity3D(ctx, objectName, false);\n"
        "  if (all3D is null || all3D.length() != 1 || all3D[0] is null || all3D[0].Id() != createdId) return 63;\n"
        "  array<BehaviorRef@>@ allBehaviors = Scene::FindAllBehavior(ctx, \"__missing__\", false);\n"
        "  if (allBehaviors is null) return 64;\n"
        "  Entity3DRef@ dupA = Scene::CreateEntity3D(ctx, duplicateName, true);\n"
        "  Entity3DRef@ dupB = Scene::CreateEntity3D(ctx, duplicateName, true);\n"
        "  if (dupA is null || dupB is null || !dupA.valid || !dupB.valid || dupA.Id() == dupB.Id()) return 65;\n"
        "  if (Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 0, false).Id() == Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 1, false).Id()) return 66;\n"
        "  Entity3DRef@ duplicateUnique = Scene::FindOneEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicateUnique is null || duplicateUnique.valid || duplicateUnique.Error().findFirst(\"match count=2\") < 0) return 67;\n"
        "  array<Entity3DRef@>@ duplicatesAll = Scene::FindAllEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicatesAll is null || duplicatesAll.length() != 2) return 68;\n"
        "  SceneRef@ scene = Scene::CurrentScene(ctx);\n"
        "  if (scene !is null && scene.valid) {\n"
        "    ObjectRef@ sceneAsObject = scene;\n"
        "    SceneObjectRef@ sceneAsSceneObject = scene;\n"
        "    if (sceneAsObject is null || sceneAsObject.Id() != scene.Id() || sceneAsSceneObject is null || sceneAsSceneObject.Id() != scene.Id()) return 109;\n"
        "    LevelRef@ level = Scene::CurrentLevel(ctx);\n"
        "    if (level !is null && level.valid) {\n"
        "      ObjectRef@ levelAsObject = level;\n"
        "      if (levelAsObject is null || levelAsObject.Id() != level.Id()) return 110;\n"
        "    }\n"
        "    if (!Scene::AddToCurrentScene(ctx, created, true)) return 17;\n"
        "    array<ObjectRef@>@ sceneOnly = Scene::FindAll(ctx, objectName, CKCID_3DENTITY, true, true);\n"
        "    if (sceneOnly is null || sceneOnly.length() == 0) return 18;\n"
        "    if (!Scene::RemoveFromCurrentScene(ctx, created, true)) return 19;\n"
        "    Scene::RemoveFromScene(ctx, scene, dupA, true);\n"
        "    Scene::RemoveFromScene(ctx, scene, dupB, true);\n"
        "    if (!Scene::AddToScene(ctx, scene, dupA, true)) return 70;\n"
        "    if (!Scene::IsInScene(ctx, scene, dupA) || !Scene::IsInCurrentScene(ctx, dupA)) return 71;\n"
        "    Entity3DRef@ scopedUnique = Scene::FindOneEntity3D(ctx, duplicateName, true);\n"
        "    if (scopedUnique is null || !scopedUnique.valid || scopedUnique.Id() != dupA.Id()) return 72;\n"
        "    array<Entity3DRef@>@ scopedAll = Scene::FindAllEntity3D(ctx, duplicateName, true);\n"
        "    if (scopedAll is null || scopedAll.length() != 1 || scopedAll[0].Id() != dupA.Id()) return 73;\n"
        "    MaterialRef@ scopedMaterial = Scene::CreateMaterial(ctx, scopedMaterialName, true);\n"
        "    TextureRef@ scopedTexture = Scene::CreateTexture(ctx, scopedTextureName, true);\n"
        "    Entity2DRef@ scoped2D = Scene::CreateEntity2D(ctx, scoped2DName, true);\n"
        "    Entity3DRef@ scopedSprite = cast<Entity3DRef>(Scene::Create(ctx, CKCID_SPRITE3D, scopedSpriteName, true));\n"
        "    if (scopedMaterial is null || scopedTexture is null || scoped2D is null || scopedSprite is null || !scopedMaterial.valid || !scopedTexture.valid || !scoped2D.valid || !scopedSprite.valid) return 76;\n"
        "    if (!Scene::AddToScene(ctx, scene, scoped2D, true)) return 78;\n"
        "    if (!Scene::IsInScene(ctx, scene, scoped2D) || !Scene::IsInCurrentScene(ctx, scoped2D)) return 92;\n"
        "    MaterialRef@ scopedMaterialOne = Scene::FindOneMaterial(ctx, scopedMaterialName, false);\n"
        "    if (scopedMaterialOne is null || !scopedMaterialOne.valid || scopedMaterialOne.Id() != scopedMaterial.Id()) return 80;\n"
        "    array<MaterialRef@>@ scopedMaterials = Scene::FindAllMaterial(ctx, scopedMaterialName, false);\n"
        "    if (scopedMaterials is null || scopedMaterials.length() != 1 || scopedMaterials[0].Id() != scopedMaterial.Id()) return 81;\n"
        "    TextureRef@ scopedTextureOne = Scene::FindOneTexture(ctx, scopedTextureName, false);\n"
        "    if (scopedTextureOne is null || !scopedTextureOne.valid || scopedTextureOne.Id() != scopedTexture.Id()) return 93;\n"
        "    array<TextureRef@>@ scopedTextures = Scene::FindAllTexture(ctx, scopedTextureName, false);\n"
        "    if (scopedTextures is null || scopedTextures.length() != 1 || scopedTextures[0].Id() != scopedTexture.Id()) return 79;\n"
        "    MaterialRef@ scoped2DMaterial = Scene::CreateMaterial(ctx, scoped2DMaterialName, true);\n"
        "    if (scoped2DMaterial is null || !scoped2DMaterial.valid) return 94;\n"
        "    if (Scene::FindOneMaterial(ctx, scoped2DMaterialName, false).Id() != scoped2DMaterial.Id()) return 96;\n"
        "    MeshRef@ scopedMesh = Scene::CreateMesh(ctx, scopedMeshName, true);\n"
        "    if (scopedMesh is null || !scopedMesh.valid) return 97;\n"
        "    if (Scene::FindOneMesh(ctx, scopedMeshName, false).Id() != scopedMesh.Id()) return 100;\n"
        "    ObjectRef@ behaviorObject = Scene::Create(ctx, CKCID_BEHAVIOR, scopedBehaviorName, true);\n"
        "    BehaviorRef@ scopedBehavior = cast<BehaviorRef>(behaviorObject);\n"
        "    ObjectRef@ scopedBehaviorObject = scopedBehavior;\n"
        "    if (scopedBehavior is null || scopedBehaviorObject is null || scopedBehaviorObject.Id() != scopedBehavior.Id()) return 101;\n"
        "    if (Scene::FindOneBehavior(ctx, scopedBehaviorName, false).Id() != scopedBehavior.Id()) return 102;\n"
        "    if (Scene::AddToScene(ctx, scene, scopedMaterial, true)) return 82;\n"
        "    if (scopedMaterial.Error().findFirst(\"requires a CKSceneObject\") < 0) return 85;\n"
        "    if (!Scene::RemoveFromScene(ctx, scene, scoped2D, true)) return 103;\n"
        "    if (Scene::IsInScene(ctx, scene, scoped2DMaterial)) return 104;\n"
        "    if (Scene::IsInScene(ctx, scene, scopedMaterial) || Scene::IsInScene(ctx, scene, scopedTexture)) return 83;\n"
        "    if (!Scene::RemoveFromScene(ctx, scene, dupA, true)) return 86;\n"
        "    if (Scene::IsInScene(ctx, scene, dupA) || Scene::IsInCurrentScene(ctx, dupA)) return 75;\n"
        "  }\n"
        "  Entity3DRef@ parent3D = Scene::CreateEntity3D(ctx, parent3DName, true);\n"
        "  Entity3DRef@ child3D = Scene::CreateEntity3D(ctx, child3DName, true);\n"
        "  if (parent3D is null || child3D is null || !parent3D.valid || !child3D.valid) return 31;\n"
        "  Entity2DRef@ parent2D = Scene::CreateEntity2D(ctx, parent2DName, true);\n"
        "  Entity2DRef@ child2D = Scene::CreateEntity2D(ctx, child2DName, true);\n"
        "  if (parent2D is null || child2D is null || !parent2D.valid || !child2D.valid) return 49;\n"
        "  array<ObjectRef@> selected;\n"
        "  selected.insertLast(created);\n"
        "  if (!Scene::Select(ctx, selected, true)) return 20;\n"
        "  if (!Scene::Destroy(ctx, created)) return 25;\n"
        "  if (created.IsValid()) return 26;\n"
        "  if (!Scene::Destroy(ctx, child3D)) return 56;\n"
        "  if (!Scene::Destroy(ctx, child2D)) return 57;\n"
        "  Scene::Destroy(ctx, dupA, true);\n"
        "  Scene::Destroy(ctx, dupB, true);\n"
        "  if (child3D.valid || child2D.valid) return 58;\n"
        "  Entity3DRef@ replacement = Scene::CreateEntity3D(ctx, objectName + \".replacement\", true);\n"
        "  if (replacement is null || !replacement.valid) return 27;\n"
        "  if (replacement.Id() == createdId && created.valid) return 28;\n"
        "  if (!Scene::Destroy(ctx, replacement)) return 29;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeScopes(const CKBehaviorContext &in behaviorCtx, const ScriptContext &in runtimeCtx, CKContext@ rawCtx, CK_ID objectId) {\n"
        "  ObjectRef@ rawObject = Scene::ById(rawCtx, objectId);\n"
        "  ObjectRef@ runtimeObject = Scene::ById(runtimeCtx, objectId);\n"
        "  ObjectRef@ behaviorObject = Scene::ById(behaviorCtx, objectId);\n"
        "  if (rawObject is null || runtimeObject is null || behaviorObject is null) return 201;\n"
        "  if (!rawObject.valid || !runtimeObject.valid || !behaviorObject.valid) return 202;\n"
        "  if (rawObject.Id() != objectId || runtimeObject.Id() != objectId || behaviorObject.Id() != objectId) return 203;\n"
        "  SceneRef@ rawScene = Scene::CurrentScene(rawCtx);\n"
        "  SceneRef@ runtimeScene = Scene::CurrentScene(runtimeCtx);\n"
        "  SceneRef@ behaviorScene = Scene::CurrentScene(behaviorCtx);\n"
        "  if (rawScene is null || runtimeScene is null || behaviorScene is null) return 204;\n"
        "  if (rawScene.valid != runtimeScene.valid || rawScene.valid != behaviorScene.valid) return 205;\n"
        "  if (rawScene.valid && (rawScene.Id() != runtimeScene.Id() || rawScene.Id() != behaviorScene.Id())) return 206;\n"
        "  LevelRef@ rawLevel = Scene::CurrentLevel(rawCtx);\n"
        "  LevelRef@ runtimeLevel = Scene::CurrentLevel(runtimeCtx);\n"
        "  LevelRef@ behaviorLevel = Scene::CurrentLevel(behaviorCtx);\n"
        "  if (rawLevel is null || runtimeLevel is null || behaviorLevel is null) return 207;\n"
        "  if (rawLevel.valid != runtimeLevel.valid || rawLevel.valid != behaviorLevel.valid) return 208;\n"
        "  if (rawLevel.valid && (rawLevel.Id() != runtimeLevel.Id() || rawLevel.Id() != behaviorLevel.Id())) return 209;\n"
        "  ObjectRef@ rawTarget = Scene::Target(rawCtx);\n"
        "  ObjectRef@ rawOwner = Scene::Owner(rawCtx);\n"
        "  if (rawTarget is null || rawTarget.valid || rawTarget.Error() != \"CKContext has no behavior target.\") return 210;\n"
        "  if (rawOwner is null || rawOwner.valid || rawOwner.Error() != \"CKContext has no behavior owner.\") return 211;\n"
        "  return 0;\n"
        "}\n"
        "int ProbeNullScope() {\n"
        "  CKContext@ missingContext = null;\n"
        "  BehaviorRef@ missingBehavior = Scene::FindBehavior(missingContext, \"__missing__\", 0, false);\n"
        "  if (missingBehavior is null || missingBehavior.valid || missingBehavior.Error().findFirst(\"CKContext is not available\") < 0) return 220;\n"
        "  ObjectRef@ missingObject = Scene::ById(missingContext, 1);\n"
        "  if (missingObject is null || missingObject.valid || missingObject.Error().findFirst(\"CKContext is not available\") < 0) return 221;\n"
        "  SceneRef@ missingScene = Scene::CurrentScene(missingContext);\n"
        "  if (missingScene is null || missingScene.valid || missingScene.Error().findFirst(\"CKContext is not available\") < 0) return 222;\n"
        "  return 0;\n"
        "}\n";

    CKAngelScriptResult compileResult = {};
    if (api->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &compileResult) != CKAS_OK) {
        error = compileResult.ErrorMessage && compileResult.ErrorMessage[0] != '\0'
            ? compileResult.ErrorMessage
            : "Scene API self-test compile probe failed.";
        CleanupNamedObject(context, objectName);
        CleanupNamedObject(context, persistentName);
        CleanupNamedObject(context, parent3DName);
        CleanupNamedObject(context, child3DName);
        CleanupNamedObject(context, parent2DName);
        CleanupNamedObject(context, child2DName);
        CleanupNamedObject(context, duplicateName);
        CleanupNamedObject(context, scopedMaterialName);
        CleanupNamedObject(context, scopedSpriteName);
        CleanupNamedObject(context, scopedTextureName);
        CleanupNamedObject(context, scoped2DName);
        CleanupNamedObject(context, scoped2DMaterialName);
        CleanupNamedObject(context, scopedMeshName);
        CleanupNamedObject(context, scopedBehaviorName);
        return false;
    }

    asIScriptModule *module = nullptr;
    api->BorrowModule(moduleName, &module, nullptr);
    asIScriptFunction *function = module ? module->GetFunctionByDecl("int Run(const CKBehaviorContext &in ctx)") : nullptr;
    asIScriptFunction *scopeFunction = module ? module->GetFunctionByDecl(
        "int ProbeScopes(const CKBehaviorContext &in, const ScriptContext &in, CKContext@, CK_ID)") : nullptr;
    asIScriptFunction *nullScopeFunction = module ? module->GetFunctionByDecl("int ProbeNullScope()") : nullptr;
    if (!function || !scopeFunction || !nullScopeFunction) {
        error = "Scene API self-test could not find one or more probe functions.";
        api->UnloadModule(moduleName, nullptr);
        CleanupNamedObject(context, objectName);
        CleanupNamedObject(context, persistentName);
        CleanupNamedObject(context, parent3DName);
        CleanupNamedObject(context, child3DName);
        CleanupNamedObject(context, parent2DName);
        CleanupNamedObject(context, child2DName);
        CleanupNamedObject(context, duplicateName);
        CleanupNamedObject(context, scopedMaterialName);
        CleanupNamedObject(context, scopedSpriteName);
        CleanupNamedObject(context, scopedTextureName);
        CleanupNamedObject(context, scoped2DName);
        CleanupNamedObject(context, scoped2DMaterialName);
        CleanupNamedObject(context, scopedMeshName);
        CleanupNamedObject(context, scopedBehaviorName);
        return false;
    }

    CKBehaviorContext behaviorContext = {};
    behaviorContext.Context = context;
    behaviorContext.CurrentLevel = context->GetCurrentLevel();
    behaviorContext.CurrentScene = context->GetCurrentScene();

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "Scene API self-test could not create AngelScript execution context.";
        api->UnloadModule(moduleName, nullptr);
        CleanupNamedObject(context, objectName);
        CleanupNamedObject(context, persistentName);
        CleanupNamedObject(context, parent3DName);
        CleanupNamedObject(context, child3DName);
        CleanupNamedObject(context, parent2DName);
        CleanupNamedObject(context, child2DName);
        CleanupNamedObject(context, duplicateName);
        CleanupNamedObject(context, scopedMaterialName);
        CleanupNamedObject(context, scopedSpriteName);
        CleanupNamedObject(context, scopedTextureName);
        CleanupNamedObject(context, scoped2DName);
        CleanupNamedObject(context, scoped2DMaterialName);
        CleanupNamedObject(context, scopedMeshName);
        CleanupNamedObject(context, scopedBehaviorName);
        return false;
    }

    CKBehavior *scopeProbeBehavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>(scopeProbeBehaviorName),
        CK_OBJECTCREATION_DYNAMIC));
    ScriptContext runtimeContext(context, nullptr, "SelfTest", "Running", 1, 0, 0.0f, 0.0f);
    CKBehaviorContext fallbackBehaviorContext = behaviorContext;
    fallbackBehaviorContext.Context = nullptr;
    fallbackBehaviorContext.Behavior = scopeProbeBehavior;

    int r = scopeProbeBehavior ? 0 : asERROR;
    if (!scopeProbeBehavior) {
        error = "Scene API self-test could not create its scope probe behavior.";
    } else {
        const ScriptHostScope rawScope = ScriptHostScope::From(context);
        const ScriptHostScope runtimeScope = ScriptHostScope::From(runtimeContext);
        const ScriptHostScope behaviorScope = ScriptHostScope::From(fallbackBehaviorContext);
        const ScriptHostScope nullScope = ScriptHostScope::From(static_cast<CKContext *>(nullptr));
        if (rawScope.Kind() != ScriptHostScopeKind::Raw ||
            runtimeScope.Kind() != ScriptHostScopeKind::Runtime ||
            behaviorScope.Kind() != ScriptHostScopeKind::Behavior ||
            rawScope.Context() != context || runtimeScope.Context() != context || behaviorScope.Context() != context ||
            rawScope.CurrentLevel() != context->GetCurrentLevel() || runtimeScope.CurrentLevel() != context->GetCurrentLevel() ||
            behaviorScope.CurrentLevel() != context->GetCurrentLevel() ||
            rawScope.CurrentScene() != context->GetCurrentScene() || runtimeScope.CurrentScene() != context->GetCurrentScene() ||
            behaviorScope.CurrentScene() != context->GetCurrentScene() ||
            behaviorScope.Behavior() != scopeProbeBehavior || behaviorScope.ComponentId() != scopeProbeBehavior->GetID() ||
            nullScope.IsValid() || nullScope.Context() || nullScope.Manager() || nullScope.Bridge()) {
            error = "Scene API self-test detected an invalid ScriptHostScope resolution invariant.";
            r = asERROR;
        }
    }

    behaviorContext.Behavior = scopeProbeBehavior;
    if (r >= 0) {
        r = scriptContext->Prepare(function);
    }
    if (r >= 0) {
        r = scriptContext->SetArgObject(0, &behaviorContext);
    }
    if (r >= 0) {
        r = scriptContext->Execute();
    }

    bool ok = false;
    if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
        if (returnCode == 0) {
            ok = true;
        } else {
            error = fmt::format("Scene API self-test returned {}.", returnCode);
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = fmt::format("Scene API self-test exception: {}.",
                            exception && exception[0] ? exception : "<empty>");
    } else {
        error = fmt::format("Scene API self-test execution failed ({}).", r);
    }

    if (ok) {
        scriptContext->Unprepare();
        r = scriptContext->Prepare(scopeFunction);
        if (r >= 0) r = scriptContext->SetArgObject(0, &fallbackBehaviorContext);
        if (r >= 0) r = scriptContext->SetArgObject(1, &runtimeContext);
        if (r >= 0) r = scriptContext->SetArgObject(2, context);
        if (r >= 0) r = scriptContext->SetArgDWord(3, scopeProbeBehavior->GetID());
        if (r >= 0) r = scriptContext->Execute();
        if (r == asEXECUTION_FINISHED) {
            const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
            ok = returnCode == 0;
            if (!ok) error = fmt::format("Scene scope equivalence probe returned {}.", returnCode);
        } else if (r == asEXECUTION_EXCEPTION) {
            const char *exception = scriptContext->GetExceptionString();
            ok = false;
            error = fmt::format("Scene scope equivalence probe exception: {}.",
                                exception && exception[0] ? exception : "<empty>");
        } else {
            ok = false;
            error = fmt::format("Scene scope equivalence probe execution failed ({}).", r);
        }
    }

    if (ok) {
        scriptContext->Unprepare();
        r = scriptContext->Prepare(nullScopeFunction);
        if (r >= 0) r = scriptContext->Execute();
        if (r == asEXECUTION_FINISHED) {
            const int returnCode = static_cast<int>(scriptContext->GetReturnDWord());
            ok = returnCode == 0;
            if (!ok) error = fmt::format("Scene null-scope probe returned {}.", returnCode);
        } else if (r == asEXECUTION_EXCEPTION) {
            const char *exception = scriptContext->GetExceptionString();
            ok = false;
            error = fmt::format("Scene null-scope probe exception: {}.",
                                exception && exception[0] ? exception : "<empty>");
        } else {
            ok = false;
            error = fmt::format("Scene null-scope probe execution failed ({}).", r);
        }
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    engine->GarbageCollect(asGC_FULL_CYCLE);
    api->UnloadModule(moduleName, nullptr);
    CleanupNamedObject(context, objectName);
    CleanupNamedObject(context, persistentName);
    CleanupNamedObject(context, parent3DName);
    CleanupNamedObject(context, child3DName);
    CleanupNamedObject(context, parent2DName);
    CleanupNamedObject(context, child2DName);
    CleanupNamedObject(context, duplicateName);
    CleanupNamedObject(context, scopedMaterialName);
    CleanupNamedObject(context, scopedSpriteName);
    CleanupNamedObject(context, scopedTextureName);
    CleanupNamedObject(context, scoped2DName);
    CleanupNamedObject(context, scoped2DMaterialName);
    CleanupNamedObject(context, scopedMeshName);
    CleanupNamedObject(context, scopedBehaviorName);
    CleanupNamedObject(context, scopeProbeBehaviorName);
    return ok;
}
