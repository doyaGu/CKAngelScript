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
        "  if (!created.SetName(objectName + \".renamed\") || created.Name() != objectName + \".renamed\") return 87;\n"
        "  if (!created.SetName(objectName) || created.Name() != objectName) return 88;\n"
        "  if (!created.SetDynamic(true) || !created.Show() || !created.IsVisible()) return 89;\n"
        "  created.ObjectFlags(); if (!created.ModifyObjectFlags(0, 0)) return 90;\n"
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
        "  if (dupA is null || dupB is null) return 138;\n"
        "  if (!dupA.valid || !dupB.valid) return 139;\n"
        "  if (dupA.Id() == dupB.Id()) return 65;\n"
        "  if (!dupA.SetName(duplicateName)) return 142;\n"
        "  if (!dupB.SetName(duplicateName)) return 143;\n"
        "  ctx.Context.ChangeObjectDynamic(dupA.Object(), true);\n"
        "  ctx.Context.ChangeObjectDynamic(dupB.Object(), true);\n"
        "  if (Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 0, false).Id() == Scene::Find(ctx, duplicateName, CKCID_3DENTITY, true, 1, false).Id()) return 66;\n"
        "  Entity3DRef@ duplicateUnique = Scene::FindOneEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicateUnique is null || duplicateUnique.valid || duplicateUnique.Error().findFirst(\"match count=2\") < 0) return 67;\n"
        "  array<Entity3DRef@>@ duplicatesAll = Scene::FindAllEntity3D(ctx, duplicateName, false);\n"
        "  if (duplicatesAll is null || duplicatesAll.length() != 2) return 68;\n"
        "  if (!Scene::Destroy(ctx, dupB, true)) return 69;\n"
        "  SceneRef@ scene = Scene::CurrentScene(ctx);\n"
        "  if (scene !is null && scene.valid) {\n"
        "    if (!Scene::AddToCurrentScene(ctx, created, true)) return 17;\n"
        "    array<ObjectRef@>@ sceneOnly = Scene::FindAll(ctx, objectName, CKCID_3DENTITY, true, true);\n"
        "    if (sceneOnly is null || sceneOnly.length() == 0) return 18;\n"
        "    if (!Scene::RemoveFromCurrentScene(ctx, created, true)) return 19;\n"
        "    Scene::RemoveFromScene(ctx, scene, dupA, true);\n"
        "    Scene::RemoveFromScene(ctx, scene, dupB, true);\n"
        "    bool dupAInScene = Scene::IsInScene(ctx, scene, dupA) || Scene::AddToScene(ctx, scene, dupA, true);\n"
        "    if (dupAInScene) {\n"
        "      if (!Scene::IsInCurrentScene(ctx, dupA)) return 71;\n"
        "      Entity3DRef@ scopedUnique = Scene::FindOneEntity3D(ctx, duplicateName, true);\n"
        "      if (scopedUnique !is null && scopedUnique.valid && scopedUnique.Id() != dupA.Id()) return 72;\n"
        "      array<Entity3DRef@>@ scopedAll = Scene::FindAllEntity3D(ctx, duplicateName, true);\n"
        "      bool sawScopedDupA = false;\n"
        "      for (uint i = 0; scopedAll !is null && i < scopedAll.length(); ++i) { if (scopedAll[i] !is null && scopedAll[i].Id() == dupA.Id()) sawScopedDupA = true; }\n"
        "      if (!sawScopedDupA) return 73;\n"
        "    }\n"
        "    MaterialRef@ scopedMaterial = Scene::CreateMaterial(ctx, scopedMaterialName, true);\n"
        "    TextureRef@ scopedTexture = Scene::CreateTexture(ctx, scopedTextureName, true);\n"
        "    Entity2DRef@ scoped2D = Scene::CreateEntity2D(ctx, scoped2DName, true);\n"
        "    Entity3DRef@ scopedSprite = cast<Entity3DRef>(Scene::Create(ctx, CKCID_SPRITE3D, scopedSpriteName, true));\n"
        "    if (scopedMaterial is null || scopedTexture is null || scoped2D is null || scopedSprite is null || !scopedMaterial.valid || !scopedTexture.valid || !scoped2D.valid || !scopedSprite.valid) return 76;\n"
        "    CKSprite3D@ scopedSpriteRaw = cast<CKSprite3D>(scopedSprite.Object());\n"
        "    if (scopedMaterial.Material() is null || scopedSpriteRaw is null) return 77;\n"
        "    if (!scopedMaterial.SetTexture(scopedTexture) || scopedMaterial.Texture() is null || scopedMaterial.Texture().Id() != scopedTexture.Id()) return 91;\n"
        "    VxColor diffuse(0.25f, 0.5f, 0.75f, 1.0f);\n"
        "    if (!scopedMaterial.SetDiffuse(diffuse) || scopedMaterial.Diffuse() != diffuse) return 106;\n"
        "    if (!scopedMaterial.SetAmbient(diffuse) || scopedMaterial.Ambient() != diffuse) return 107;\n"
        "    if (!scopedMaterial.SetSpecular(diffuse) || scopedMaterial.Specular() != diffuse) return 108;\n"
        "    if (!scopedMaterial.SetEmissive(diffuse) || scopedMaterial.Emissive() != diffuse) return 109;\n"
        "    if (!scopedMaterial.SetPower(4.0f) || scopedMaterial.Power() != 4.0f) return 110;\n"
        "    if (!scopedMaterial.SetTextureBlendMode(VXTEXTUREBLEND_MODULATE) || scopedMaterial.TextureBlendMode() != VXTEXTUREBLEND_MODULATE) return 111;\n"
        "    if (!scopedMaterial.SetSourceBlend(VXBLEND_SRCALPHA) || scopedMaterial.SourceBlend() != VXBLEND_SRCALPHA) return 112;\n"
        "    if (!scopedMaterial.SetDestBlend(VXBLEND_INVSRCALPHA) || scopedMaterial.DestBlend() != VXBLEND_INVSRCALPHA) return 113;\n"
        "    if (!scopedMaterial.EnableAlphaBlend(true) || !scopedMaterial.AlphaBlendEnabled()) return 114;\n"
        "    if (!scopedMaterial.SetAlphaFunc(VXCMP_ALWAYS) || scopedMaterial.AlphaFunc() != VXCMP_ALWAYS) return 115;\n"
        "    if (!scopedMaterial.SetAlphaRef(7) || scopedMaterial.AlphaRef() != 7) return 116;\n"
        "    scopedSpriteRaw.SetMaterial(scopedMaterial.Material());\n"
        "    if (!scoped2D.SetMaterial(scopedMaterial) || !Scene::AddToScene(ctx, scene, scoped2D, true)) return 78;\n"
        "    SceneObjectRef@ scoped2DSceneObject = scoped2D;\n"
        "    if (!scoped2DSceneObject.IsInScene(scene) || !scoped2DSceneObject.IsInCurrentScene()) return 92;\n"
        "    if (!Scene::IsInScene(ctx, scene, scopedMaterial) || !Scene::IsInScene(ctx, scene, scopedTexture)) return 79;\n"
        "    MaterialRef@ scopedMaterialOne = Scene::FindOneMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterialOne is null || !scopedMaterialOne.valid || scopedMaterialOne.Id() != scopedMaterial.Id()) return 80;\n"
        "    array<MaterialRef@>@ scopedMaterials = Scene::FindAllMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterials is null || scopedMaterials.length() != 1 || scopedMaterials[0].Id() != scopedMaterial.Id()) return 81;\n"
        "    TextureRef@ scopedTextureOne = Scene::FindOneTexture(ctx, scopedTextureName, true);\n"
        "    if (scopedTextureOne is null || !scopedTextureOne.valid || scopedTextureOne.Id() != scopedTexture.Id()) return 93;\n"
        "    MaterialRef@ scoped2DMaterial = Scene::CreateMaterial(ctx, scoped2DMaterialName, true);\n"
        "    if (scoped2DMaterial is null || !scoped2DMaterial.valid) return 94;\n"
        "    if (!scoped2D.SetMaterial(scoped2DMaterial)) return 95;\n"
        "    if (!Scene::IsInScene(ctx, scene, scoped2DMaterial)) return 96;\n"
        "    if (dupAInScene) {\n"
        "      MeshRef@ scopedMesh = Scene::CreateMesh(ctx, scopedMeshName, true);\n"
        "      if (scopedMesh is null || !scopedMesh.valid || scopedMesh.Mesh() is null) return 97;\n"
        "      if (scopedMesh.Mesh().AddChannel(scopedMaterial.Material()) < 0) return 117;\n"
        "      if (!dupA.AddMesh(scopedMesh) || dupA.MeshCount() <= 0 || dupA.Mesh(0) is null) return 99;\n"
        "      MaterialRef@ meshMaterial = dupA.Material(0, 0);\n"
        "      if (meshMaterial is null) return 118;\n"
        "      if (meshMaterial.valid && meshMaterial.Id() != scopedMaterial.Id()) return 118;\n"
        "      MaterialRef@ badMeshMaterial = dupA.Material(0, 99);\n"
        "      if (badMeshMaterial is null || badMeshMaterial.valid) return 118;\n"
        "      dupA.SetCurrentMesh(scopedMesh, false); MeshRef@ maybeCurrentMesh = dupA.CurrentMesh();\n"
        "      if (!Scene::IsInScene(ctx, scene, scopedMesh) || Scene::FindOneMesh(ctx, scopedMeshName, true).Id() != scopedMesh.Id()) return 100;\n"
        "      ObjectRef@ behaviorObject = Scene::Create(ctx, CKCID_BEHAVIOR, scopedBehaviorName, true);\n"
        "      BehaviorRef@ scopedBehavior = cast<BehaviorRef>(behaviorObject);\n"
        "      CKBehavior@ scopedBehaviorRaw = scopedBehavior !is null ? cast<CKBehavior>(scopedBehavior.Object()) : null;\n"
        "      CKBeObject@ behaviorOwner = cast<CKBeObject>(dupA.Object());\n"
        "      if (scopedBehavior is null || scopedBehaviorRaw is null || behaviorOwner is null || scopedBehaviorRaw.SetOwner(behaviorOwner, true) != CK_OK) return 101;\n"
        "      BehaviorRef@ foundScopedBehavior = Scene::FindOneBehavior(ctx, scopedBehaviorName, true);\n"
        "      if (foundScopedBehavior is null) return 140;\n"
        "      if (!foundScopedBehavior.valid) return 141;\n"
        "      if (foundScopedBehavior.Id() != scopedBehavior.Id()) return 102;\n"
        "      if (!Scene::RemoveFromScene(ctx, scene, dupA, true)) return 86;\n"
        "      dupAInScene = false;\n"
        "    }\n"
        "    if (Scene::AddToScene(ctx, scene, scopedMaterial, true)) return 82;\n"
        "    if (scopedMaterial.Error().findFirst(\"requires a CKSceneObject\") < 0) return 85;\n"
        "    if (!Scene::RemoveFromScene(ctx, scene, scoped2D, true)) return 103;\n"
        "    if (Scene::IsInScene(ctx, scene, scoped2DMaterial)) return 104;\n"
        "    if (!scoped2D.SetMaterial(scopedMaterial)) return 105;\n"
        "    if (Scene::IsInScene(ctx, scene, scopedMaterial) || Scene::IsInScene(ctx, scene, scopedTexture)) return 83;\n"
        "    MaterialRef@ scopedMaterialMissing = Scene::FindOneMaterial(ctx, scopedMaterialName, true);\n"
        "    if (scopedMaterialMissing is null || scopedMaterialMissing.valid || scopedMaterialMissing.Error().findFirst(\"match count=0\") < 0) return 84;\n"
        "    if (dupAInScene && (!Scene::RemoveFromScene(ctx, scene, dupA, true) || Scene::IsInScene(ctx, scene, dupA) || Scene::IsInCurrentScene(ctx, dupA))) return 75;\n"
        "  }\n"
        "  Entity3DRef@ parent3D = Scene::CreateEntity3D(ctx, parent3DName, true);\n"
        "  Entity3DRef@ child3D = Scene::CreateEntity3D(ctx, child3DName, true);\n"
        "  if (parent3D is null || child3D is null || !parent3D.valid || !child3D.valid) return 31;\n"
        "  if (!child3D.SetParent(parent3D, true)) return 32;\n"
        "  Entity3DRef@ resolvedParent3D = child3D.Parent();\n"
        "  if (resolvedParent3D is null || !resolvedParent3D.valid || resolvedParent3D.Id() != parent3D.Id()) return 33;\n"
        "  if (parent3D.ChildCount() != 1) return 34;\n"
        "  Entity3DRef@ resolvedChild3D = parent3D.Child(0);\n"
        "  if (resolvedChild3D is null || !resolvedChild3D.valid || resolvedChild3D.Id() != child3D.Id()) return 35;\n"
        "  array<Entity3DRef@>@ children3D = parent3D.Children();\n"
        "  if (children3D is null || children3D.length() != 1 || children3D[0] is null || children3D[0].Id() != child3D.Id()) return 36;\n"
        "  if (!parent3D.IsAncestorOf(child3D) || !child3D.IsDescendantOf(parent3D) || child3D.IsAncestorOf(parent3D)) return 37;\n"
        "  VxVector pos(1.0f, 2.0f, 3.0f);\n"
        "  VxVector outPos;\n"
        "  if (!child3D.SetPosition(pos, parent3D, true) || !child3D.GetPosition(outPos, parent3D)) return 38;\n"
        "  if (outPos.x != 1.0f || outPos.y != 2.0f || outPos.z != 3.0f) return 39;\n"
        "  if (!child3D.Translate(1.0f, 0.0f, 0.0f, parent3D, true) || !child3D.GetPosition(outPos, parent3D)) return 40;\n"
        "  if (outPos.x != 2.0f || outPos.y != 2.0f || outPos.z != 3.0f) return 41;\n"
        "  VxQuaternion quat(VxVector(0.0f, 1.0f, 0.0f), 0.25f);\n"
        "  VxQuaternion outQuat;\n"
        "  if (!child3D.SetQuaternion(quat, parent3D, true, false) || !child3D.GetQuaternion(outQuat, parent3D)) return 42;\n"
        "  VxVector scale(2.0f, 3.0f, 4.0f);\n"
        "  VxVector outScale;\n"
        "  if (!child3D.SetScale(scale, true, true) || !child3D.GetScale(outScale, true)) return 43;\n"
        "  if (outScale.x != 2.0f || outScale.y != 3.0f || outScale.z != 4.0f) return 44;\n"
        "  if (!child3D.LookAt(VxVector(5.0f, 2.0f, 3.0f), parent3D, true)) return 45;\n"
        "  VxVector dir(0.0f, 0.0f, 1.0f); VxVector up(0.0f, 1.0f, 0.0f); VxVector right(1.0f, 0.0f, 0.0f);\n"
        "  if (!child3D.SetOrientation(dir, up, right, parent3D, true) || !child3D.GetOrientation(dir, up, right, parent3D)) return 119;\n"
        "  if (!child3D.SetDirection(VxVector(1.0f, 0.0f, 0.0f), parent3D, true) || !child3D.GetDirection(dir, parent3D)) return 120;\n"
        "  if (!child3D.SetUp(VxVector(0.0f, 1.0f, 0.0f), parent3D, true) || !child3D.GetUp(up, parent3D)) return 121;\n"
        "  if (!child3D.SetRight(VxVector(0.0f, 0.0f, 1.0f), parent3D, true) || !child3D.GetRight(right, parent3D)) return 122;\n"
        "  VxBbox bbox(VxVector(-1.0f, -1.0f, -1.0f), VxVector(1.0f, 1.0f, 1.0f)); VxBbox outBox;\n"
        "  if (!child3D.SetBoundingBox(bbox, true) || !child3D.GetBoundingBox(outBox, true)) return 123;\n"
        "  child3D.Radius();\n"
        "  if (!parent3D.RemoveChild(child3D)) return 46;\n"
        "  if (child3D.Parent() !is null && child3D.Parent().valid) return 47;\n"
        "  if (!parent3D.AddChild(child3D, true)) return 48;\n"
        "  Entity2DRef@ parent2D = Scene::CreateEntity2D(ctx, parent2DName, true);\n"
        "  Entity2DRef@ child2D = Scene::CreateEntity2D(ctx, child2DName, true);\n"
        "  if (parent2D is null || child2D is null || !parent2D.valid || !child2D.valid) return 49;\n"
        "  if (!child2D.SetParent(parent2D)) return 50;\n"
        "  Entity2DRef@ resolvedParent2D = child2D.Parent();\n"
        "  if (resolvedParent2D is null || !resolvedParent2D.valid || resolvedParent2D.Id() != parent2D.Id()) return 51;\n"
        "  if (parent2D.ChildCount() != 1) return 52;\n"
        "  Entity2DRef@ resolvedChild2D = parent2D.Child(0);\n"
        "  if (resolvedChild2D is null || !resolvedChild2D.valid || resolvedChild2D.Id() != child2D.Id()) return 53;\n"
        "  array<Entity2DRef@>@ children2D = parent2D.Children();\n"
        "  if (children2D is null || children2D.length() != 1 || children2D[0] is null || children2D[0].Id() != child2D.Id()) return 54;\n"
        "  if (!parent2D.IsAncestorOf(child2D) || !child2D.IsDescendantOf(parent2D) || child2D.IsAncestorOf(parent2D)) return 55;\n"
        "  if (!child2D.SetBackground(true)) return 124;\n"
        "  child2D.IsBackground();\n"
        "  if (!child2D.SetBackground(false)) return 125;\n"
        "  child2D.IsBackground();\n"
        "  if (!child2D.EnableRatioOffset(true) || !child2D.IsRatioOffset()) return 126;\n"
        "  if (!child2D.EnableRatioOffset(false) || child2D.IsRatioOffset()) return 127;\n"
        "  if (!child2D.EnableClipToCamera(true) || !child2D.IsClippedToCamera()) return 128;\n"
        "  if (!child2D.EnableClipToCamera(false) || child2D.IsClippedToCamera()) return 129;\n"
        "  CKDWORD old2DFlags = child2D.Flags();\n"
        "  if (!child2D.SetFlags(0) || child2D.Flags() != 0) return 130;\n"
        "  if (!child2D.ModifyFlags(0, 0)) return 131;\n"
        "  child2D.SetFlags(old2DFlags);\n"
        "  VxRect extSrc(0.0f, 0.0f, 8.0f, 8.0f); VxRect extRect(1.0f, 2.0f, 9.0f, 10.0f); VxRect outSrc; VxRect outRect;\n"
        "  if (!child2D.SetExtents(extSrc, extRect) || !child2D.GetExtents(outSrc, outRect)) return 132;\n"
        "  if (outSrc.left != 0.0f || outRect.left != 1.0f || outRect.bottom != 10.0f) return 133;\n"
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
        "  if (child3D.SetPosition(0.0f, 0.0f, 0.0f)) return 58;\n"
        "  if (child3D.SetDirection(VxVector(0.0f, 0.0f, 1.0f))) return 134;\n"
        "  if (child3D.Parent() !is null && child3D.Parent().valid) return 59;\n"
        "  if (child2D.Parent() !is null && child2D.Parent().valid) return 60;\n"
        "  if (child2D.SetBackground(true)) return 135;\n"
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
