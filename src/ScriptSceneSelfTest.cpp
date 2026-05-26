#include "ScriptSelfTests.h"

#include <string>
#include <vector>

#include <fmt/format.h>

#include "AngelScriptManager.h"
#include "ScriptManager.h"

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

    AngelScriptManager *manager = AngelScriptManager::GetManager(context);
    if (!manager) {
        error = "Scene self-test could not retrieve AngelScriptManager.";
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
        "  Entity3DRef@ created = Scene::CreateEntity3D(ctx, objectName, true);\n"
        "  if (created is null || !created.valid || created.Entity3D() is null) return 10;\n"
        "  if (!created.IsDynamic() || created.Name() != objectName || created.ClassId() != CKCID_3DENTITY) return 11;\n"
        "  CKObject@ createdObject = created.Object();\n"
        "  if (createdObject is null) return 87;\n"
        "  createdObject.SetName(objectName + \".renamed\"); if (created.Name() != objectName + \".renamed\") return 88;\n"
        "  createdObject.SetName(objectName); if (created.Name() != objectName) return 89;\n"
        "  ctx.Context.ChangeObjectDynamic(createdObject, true); createdObject.Show(); if (!createdObject.IsVisible()) return 90;\n"
        "  createdObject.GetObjectFlags(); createdObject.ModifyObjectFlags(0, 0);\n"
        "  CK_ID createdId = created.Id();\n"
        "  ObjectRef@ objectRef = created;\n"
        "  Entity3DRef@ castCreated = cast<Entity3DRef>(objectRef);\n"
        "  if (castCreated is null || castCreated.Id() != createdId) return 12;\n"
        "  MaterialRef@ wrongMaterial = cast<MaterialRef>(objectRef);\n"
        "  BehaviorRef@ wrongBehavior = cast<BehaviorRef>(objectRef);\n"
        "  if (wrongMaterial !is null || wrongBehavior !is null) return 30;\n"
        "  ObjectRef@ rawRef = Scene::Ref(ctx, created.Object());\n"
        "  if (rawRef is null || rawRef.Id() != createdId) return 12;\n"
        "  ObjectRef@ byId = Scene::ById(ctx, createdId);\n"
        "  if (byId is null || !byId.valid || byId.Id() != createdId) return 13;\n"
        "  ObjectRef@ found = Scene::Find(ctx, objectName, CKCID_3DENTITY, true, 0);\n"
        "  if (found is null || !found.valid || found.Id() != createdId) return 14;\n"
        "  Entity3DRef@ typedFound = Scene::FindEntity3D(ctx, objectName, 0);\n"
        "  if (typedFound is null || typedFound.Entity3D() is null || typedFound.Id() != createdId) return 15;\n"
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
        "  Entity3DRef@ dupA = Scene::CreateEntity3D(ctx, duplicateName + \".A\", true);\n"
        "  Entity3DRef@ dupB = Scene::CreateEntity3D(ctx, duplicateName + \".B\", true);\n"
        "  if (dupA is null || dupB is null || !dupA.valid || !dupB.valid || dupA.Id() == dupB.Id()) return 65;\n"
        "  dupA.Object().SetName(duplicateName);\n"
        "  dupB.Object().SetName(duplicateName);\n"
        "  ctx.Context.ChangeObjectDynamic(dupA.Object(), true);\n"
        "  ctx.Context.ChangeObjectDynamic(dupB.Object(), true);\n"
        "  if (Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 0, false).Id() == Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 1, false).Id()) return 66;\n"
        "  Entity3DRef@ duplicateUnique = Scene::FindOneEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicateUnique is null || duplicateUnique.valid || duplicateUnique.Error().findFirst(\"match count=2\") < 0) return 67;\n"
        "  array<Entity3DRef@>@ duplicatesAll = Scene::FindAllEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicatesAll is null || duplicatesAll.length() != 2) return 68;\n"
        "  SceneRef@ scene = Scene::CurrentScene(ctx);\n"
        "  if (scene !is null && scene.valid) {\n"
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
        "    CKSprite3D@ scopedSpriteRaw = cast<CKSprite3D>(scopedSprite.Object());\n"
        "    CKMaterial@ scopedMaterialRaw = scopedMaterial.Material();\n"
        "    CKTexture@ scopedTextureRaw = scopedTexture.Texture();\n"
        "    if (scopedMaterialRaw is null || scopedTextureRaw is null || scopedSpriteRaw is null) return 77;\n"
        "    scopedMaterialRaw.SetTexture(scopedTextureRaw); if (scopedMaterialRaw.GetTexture() is null || scopedMaterialRaw.GetTexture().GetID() != scopedTexture.Id()) return 91;\n"
        "    scopedSpriteRaw.SetMaterial(scopedMaterial.Material());\n"
        "    CK2dEntity@ scoped2DRaw = scoped2D.Entity2D();\n"
        "    if (scoped2DRaw is null) return 78;\n"
        "    scoped2DRaw.SetMaterial(scopedMaterialRaw); if (!Scene::AddToScene(ctx, scene, scoped2D, true)) return 78;\n"
        "    if (!Scene::IsInScene(ctx, scene, scoped2D) || !Scene::IsInCurrentScene(ctx, scoped2D)) return 92;\n"
        "    if (!Scene::IsInScene(ctx, scene, scopedMaterial) || !Scene::IsInScene(ctx, scene, scopedTexture)) return 79;\n"
        "    MaterialRef@ scopedMaterialOne = Scene::FindOneMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterialOne is null || !scopedMaterialOne.valid || scopedMaterialOne.Id() != scopedMaterial.Id()) return 80;\n"
        "    array<MaterialRef@>@ scopedMaterials = Scene::FindAllMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterials is null || scopedMaterials.length() != 1 || scopedMaterials[0].Id() != scopedMaterial.Id()) return 81;\n"
        "    TextureRef@ scopedTextureOne = Scene::FindOneTexture(ctx, scopedTextureName, true);\n"
        "    if (scopedTextureOne is null || !scopedTextureOne.valid || scopedTextureOne.Id() != scopedTexture.Id()) return 93;\n"
        "    MaterialRef@ scoped2DMaterial = Scene::CreateMaterial(ctx, scoped2DMaterialName, true);\n"
        "    if (scoped2DMaterial is null || !scoped2DMaterial.valid) return 94;\n"
        "    scoped2DRaw.SetMaterial(scoped2DMaterial.Material());\n"
        "    if (!Scene::IsInScene(ctx, scene, scoped2DMaterial)) return 96;\n"
        "    MeshRef@ scopedMesh = Scene::CreateMesh(ctx, scopedMeshName, true);\n"
        "    if (scopedMesh is null || !scopedMesh.valid || scopedMesh.Mesh() is null) return 97;\n"
        "    CK3dEntity@ dupARaw = dupA.Entity3D();\n"
        "    if (dupARaw is null || dupARaw.AddMesh(scopedMesh.Mesh()) != CK_OK || dupARaw.GetMeshCount() <= 0 || dupARaw.GetMesh(0) is null) return 99;\n"
        "    dupARaw.SetCurrentMesh(scopedMesh.Mesh(), false); CKMesh@ maybeCurrentMesh = dupARaw.GetCurrentMesh();\n"
        "    if (!Scene::IsInScene(ctx, scene, scopedMesh) || Scene::FindOneMesh(ctx, scopedMeshName, true).Id() != scopedMesh.Id()) return 100;\n"
        "    ObjectRef@ behaviorObject = Scene::Create(ctx, CKCID_BEHAVIOR, scopedBehaviorName, true);\n"
        "    BehaviorRef@ scopedBehavior = cast<BehaviorRef>(behaviorObject);\n"
        "    CKBehavior@ scopedBehaviorRaw = scopedBehavior !is null ? cast<CKBehavior>(scopedBehavior.Object()) : null;\n"
        "    CKBeObject@ behaviorOwner = cast<CKBeObject>(dupA.Object());\n"
        "    if (scopedBehavior is null || scopedBehaviorRaw is null || behaviorOwner is null || scopedBehaviorRaw.SetOwner(behaviorOwner, true) != CK_OK) return 101;\n"
        "    if (Scene::FindOneBehavior(ctx, scopedBehaviorName, true).Id() != scopedBehavior.Id()) return 102;\n"
        "    if (Scene::AddToScene(ctx, scene, scopedMaterial, true)) return 82;\n"
        "    if (scopedMaterial.Error().findFirst(\"requires a CKSceneObject\") < 0) return 85;\n"
        "    if (!Scene::RemoveFromScene(ctx, scene, scoped2D, true)) return 103;\n"
        "    if (Scene::IsInScene(ctx, scene, scoped2DMaterial)) return 104;\n"
        "    scoped2DRaw.SetMaterial(scopedMaterialRaw);\n"
        "    if (Scene::IsInScene(ctx, scene, scopedMaterial) || Scene::IsInScene(ctx, scene, scopedTexture)) return 83;\n"
        "    MaterialRef@ scopedMaterialMissing = Scene::FindOneMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterialMissing is null || scopedMaterialMissing.valid || scopedMaterialMissing.Error().findFirst(\"match count=0\") < 0) return 84;\n"
        "    if (!Scene::RemoveFromScene(ctx, scene, dupA, true)) return 86;\n"
        "    if (Scene::IsInScene(ctx, scene, dupA) || Scene::IsInCurrentScene(ctx, dupA)) return 75;\n"
        "  }\n"
        "  Entity3DRef@ parent3D = Scene::CreateEntity3D(ctx, parent3DName, true);\n"
        "  Entity3DRef@ child3D = Scene::CreateEntity3D(ctx, child3DName, true);\n"
        "  if (parent3D is null || child3D is null || !parent3D.valid || !child3D.valid) return 31;\n"
        "  CK3dEntity@ parent3DRaw = parent3D.Entity3D();\n"
        "  CK3dEntity@ child3DRaw = child3D.Entity3D();\n"
        "  if (parent3DRaw is null || child3DRaw is null) return 32;\n"
        "  if (!child3DRaw.SetParent(parent3DRaw, true)) return 33;\n"
        "  if (child3DRaw.GetParent() is null || child3DRaw.GetParent().GetID() != parent3D.Id()) return 34;\n"
        "  if (parent3DRaw.GetChildrenCount() != 1) return 35;\n"
        "  if (parent3DRaw.GetChild(0) is null || parent3DRaw.GetChild(0).GetID() != child3D.Id()) return 36;\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  VxVector outPos;\n"
        "  child3DRaw.SetPosition(pos, parent3DRaw, true); child3DRaw.GetPosition(outPos, parent3DRaw);\n"
        "  if (outPos.x != 1.0f || outPos.y != 2.0f || outPos.z != 3.0f) return 39;\n"
        "  child3DRaw.Translate(1.0f, 0.0f, 0.0f, parent3DRaw, true); child3DRaw.GetPosition(outPos, parent3DRaw);\n"
        "  if (outPos.x != 2.0f || outPos.y != 2.0f || outPos.z != 3.0f) return 41;\n"
        "  VxQuaternion quat(VxVector(0.0f, 1.0f, 0.0f), 0.25f);\n"
        "  VxQuaternion outQuat;\n"
        "  child3DRaw.SetQuaternion(quat, parent3DRaw, true, false); child3DRaw.GetQuaternion(outQuat, parent3DRaw);\n"
        "  VxVector scale(2.0f, 3.0f, 4.0f);\n"
        "  VxVector outScale;\n"
        "  child3DRaw.SetScale(scale, true, true); child3DRaw.GetScale(outScale, true);\n"
        "  if (outScale.x != 2.0f || outScale.y != 3.0f || outScale.z != 4.0f) return 44;\n"
        "  child3DRaw.LookAt(VxVector(5.0f, 2.0f, 3.0f), parent3DRaw, true);\n"
        "  if (!parent3DRaw.RemoveChild(child3DRaw)) return 46;\n"
        "  if (child3DRaw.GetParent() !is null) return 47;\n"
        "  if (!parent3DRaw.AddChild(child3DRaw, true)) return 48;\n"
        "  Entity2DRef@ parent2D = Scene::CreateEntity2D(ctx, parent2DName, true);\n"
        "  Entity2DRef@ child2D = Scene::CreateEntity2D(ctx, child2DName, true);\n"
        "  if (parent2D is null || child2D is null || !parent2D.valid || !child2D.valid) return 49;\n"
        "  CK2dEntity@ parent2DRaw = parent2D.Entity2D();\n"
        "  CK2dEntity@ child2DRaw = child2D.Entity2D();\n"
        "  if (parent2DRaw is null || child2DRaw is null || !child2DRaw.SetParent(parent2DRaw)) return 50;\n"
        "  if (child2DRaw.GetParent() is null || child2DRaw.GetParent().GetID() != parent2D.Id()) return 51;\n"
        "  if (parent2DRaw.GetChildrenCount() != 1) return 52;\n"
        "  if (parent2DRaw.GetChild(0) is null || parent2DRaw.GetChild(0).GetID() != child2D.Id()) return 53;\n"
        "  array<ObjectRef@> selected;\n"
        "  selected.insertLast(created);\n"
        "  if (!Scene::Select(ctx, selected, true)) return 20;\n"
        "  Entity3DRef@ persistent = Scene::CreateEntity3D(ctx, persistentName, true);\n"
        "  if (persistent is null || !persistent.valid) return 21;\n"
        "  ctx.Context.ChangeObjectDynamic(persistent.Object(), false);\n"
        "  if (persistent.IsDynamic()) return 22;\n"
        "  if (Scene::Destroy(ctx, persistent)) return 23;\n"
        "  if (!Scene::Destroy(ctx, persistent, true)) {\n"
        "    ctx.Context.ChangeObjectDynamic(persistent.Object(), true);\n"
        "    if (!Scene::Destroy(ctx, persistent, true)) return 24;\n"
        "  }\n"
        "  if (!Scene::Destroy(ctx, created)) return 25;\n"
        "  if (created.IsValid() || created.Object() !is null || created.Entity3D() !is null) return 26;\n"
        "  if (!Scene::Destroy(ctx, child3D)) return 56;\n"
        "  if (!Scene::Destroy(ctx, child2D)) return 57;\n"
        "  Scene::Destroy(ctx, dupA, true);\n"
        "  Scene::Destroy(ctx, dupB, true);\n"
        "  if (child3D.Entity3D() !is null) return 58;\n"
        "  if (child2D.Entity2D() !is null) return 60;\n"
        "  Entity3DRef@ replacement = Scene::CreateEntity3D(ctx, objectName + \".replacement\", true);\n"
        "  if (replacement is null || !replacement.valid) return 27;\n"
        "  if (replacement.Id() == createdId && created.valid) return 28;\n"
        "  if (!Scene::Destroy(ctx, replacement)) return 29;\n"
        "  return 0;\n"
        "}\n";

    AngelScriptResult compileResult = {};
    if (manager->CompileModule(moduleName, source, true, &compileResult) != ANGELSCRIPT_STATUS_OK) {
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

    asIScriptModule *module = manager->GetModule(moduleName);
    asIScriptFunction *function = module ? module->GetFunctionByDecl("int Run(const CKBehaviorContext &in ctx)") : nullptr;
    if (!function) {
        error = "Scene API self-test could not find Run() function.";
        manager->UnloadModule(moduleName, nullptr);
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

    asIScriptContext *scriptContext = engine->CreateContext();
    if (!scriptContext) {
        error = "Scene API self-test could not create AngelScript execution context.";
        manager->UnloadModule(moduleName, nullptr);
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

    int r = scriptContext->Prepare(function);
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

    scriptContext->Release();
    manager->UnloadModule(moduleName, nullptr);
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
    return ok;
}
