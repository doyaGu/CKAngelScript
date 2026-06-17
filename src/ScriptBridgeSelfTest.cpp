#include "ScriptBridgeHandles.h"

#include <fmt/format.h>

#include "add_on/scriptarray/scriptarray.h"

#include <fstream>
#include <functional>

static std::string EscapeAngelScriptString(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

static void WriteBehaviorBridgeSelfTestMarker(const std::string &status,
                                              const std::string &operationName,
                                              const std::string &typeName,
                                              const std::string &message) {
    const char *path = std::getenv("CKAS_SELFTEST_MARKER");
    if (!path || path[0] == '\0') {
        return;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return;
    }

    out << "status=" << status << "\n";
    out << "operation=" << operationName << "\n";
    out << "type=" << typeName << "\n";
    if (!message.empty()) {
        out << "message=" << message << "\n";
    }
}

static bool RunBehaviorBridgeScriptSelfTest(CKContext *context,
                                            CKParameterManager *parameterManager,
                                            asIScriptEngine *engine,
                                            const std::string &operationName,
                                            const std::string &typeName,
                                            std::string &error) {
    constexpr const char *moduleName = "__CKAS_BehaviorBridgeSelfTest";

    if (!engine) {
        error = "AngelScript engine is not available.";
        return false;
    }

    std::string source;
    source += "void ProbeParamHandleApi(ParamRef@ param, ParamValue@ value, ParamStructValue@ structValue) {\n";
    source += "    if (param is null) return;\n";
    source += "    param.IsValid(); bool valid = param.valid; CK_ID id = param.id; int index = param.index; ParamKind kind = param.kind; string name = param.name;\n";
    source += "    param.TypeGuid(); param.TypeName(); param.DataSize(); param.RealSource(); param.DirectSource();\n";
    source += "    ParamSourceLinkRef@ link = param.SetSourceScoped(param); if (link !is null) { link.IsValid(); link.IsCommitted(); link.IsRestored(); link.Commit(); link.Restore(); link.Describe(); }\n";
    source += "    param.SetSource(param); param.Set(value); param.SetInt(1); param.SetFloat(1.0f); param.SetBool(true); param.SetString(\"x\"); param.SetObject(null);\n";
    source += "    param.SetEnum(\"0\"); param.SetEnum(0); param.SetFlags(\"0\"); param.SetFlags(0); param.SetStruct(structValue); param.Get(); param.CopyFrom(param);\n";
    source += "    param.GetText(); param.GetEnumText(); param.GetFlagsText(); param.SetText(\"0\"); NativeBuffer@ raw = param.GetRaw(); param.SetRaw(raw); ParamStructRef@ s = param.Struct(); param.Describe();\n";
    source += "    if (s !is null) { s.IsValid(); s.Count(); s.Info(); s.Member(0); s.FindMember(\"x\"); s.Describe(); }\n";
    source += "    ObjectRef@ paramObject = param;\n";
    source += "    ParamInRef@ paramIn = cast<ParamInRef>(param); if (paramIn !is null) { paramIn.IsValid(); paramIn.valid; paramIn.Error(); paramIn.Describe(); paramIn.Id(); paramIn.Name(); paramIn.ClassId(); paramIn.IsDynamic(); paramIn.Object(); paramIn.TypeGuid(); paramIn.SetInt(1); ParamRef@ base = paramIn; ObjectRef@ object = paramIn; }\n";
    source += "    ParamOutRef@ paramOut = cast<ParamOutRef>(param); if (paramOut !is null) { paramOut.Id(); paramOut.Get(); ParamRef@ base = paramOut; ObjectRef@ object = paramOut; }\n";
    source += "    ParamLocalRef@ paramLocal = cast<ParamLocalRef>(param); if (paramLocal !is null) { paramLocal.Id(); paramLocal.Set(value); ParamRef@ base = paramLocal; ObjectRef@ object = paramLocal; }\n";
    source += "    if (s !is null) { ObjectRef@ structObject = s; ParamRef@ structParam = s; s.Id(); s.Name(); s.ClassId(); s.IsDynamic(); s.Object(); }\n";
    source += "}\n";
    source += "void ProbeSceneApi(const CKBehaviorContext &in ctx) {\n";
    source += "    LevelRef@ currentLevel = Scene::CurrentLevel(ctx);\n";
    source += "    SceneRef@ currentScene = Scene::CurrentScene(ctx);\n";
    source += "    ObjectRef@ targetRef = Scene::Target(ctx);\n";
    source += "    ObjectRef@ ownerRef = Scene::Owner(ctx);\n";
    source += "    ObjectRef@ byId = Scene::ById(ctx, 1);\n";
    source += "    ObjectRef@ found = Scene::Find(ctx, \"__missing__\");\n";
    source += "    ObjectRef@ foundScoped = Scene::Find(ctx, \"__missing__\", CKCID_OBJECT, true, 0, true);\n";
    source += "    ObjectRef@ foundOne = Scene::FindOne(ctx, \"__missing__\", CKCID_OBJECT, true, true);\n";
    source += "    array<ObjectRef@>@ sceneObjects = Scene::FindAll(ctx, \"\", CKCID_OBJECT, true, false);\n";
    source += "    Entity3DRef@ e3d = Scene::FindEntity3D(ctx, \"__missing__\");\n";
    source += "    Entity2DRef@ e2d = Scene::FindEntity2D(ctx, \"__missing__\");\n";
    source += "    Entity3DRef@ e3dScoped = Scene::FindEntity3D(ctx, \"__missing__\", 0, true);\n";
    source += "    Entity2DRef@ e2dScoped = Scene::FindEntity2D(ctx, \"__missing__\", 0, true);\n";
    source += "    Entity3DRef@ one3d = Scene::FindOneEntity3D(ctx, \"__missing__\", true);\n";
    source += "    Entity2DRef@ one2d = Scene::FindOneEntity2D(ctx, \"__missing__\", true);\n";
    source += "    MaterialRef@ oneMaterial = Scene::FindOneMaterial(ctx, \"__missing__\", true);\n";
    source += "    TextureRef@ oneTexture = Scene::FindOneTexture(ctx, \"__missing__\", true);\n";
    source += "    MeshRef@ oneMesh = Scene::FindOneMesh(ctx, \"__missing__\", true);\n";
    source += "    BehaviorRef@ oneBehavior = Scene::FindOneBehavior(ctx, \"__missing__\", true);\n";
    source += "    array<Entity3DRef@>@ all3d = Scene::FindAllEntity3D(ctx, \"__missing__\", true);\n";
    source += "    array<Entity2DRef@>@ all2d = Scene::FindAllEntity2D(ctx, \"__missing__\", true);\n";
    source += "    array<MaterialRef@>@ allMaterials = Scene::FindAllMaterial(ctx, \"__missing__\", true);\n";
    source += "    array<TextureRef@>@ allTextures = Scene::FindAllTexture(ctx, \"__missing__\", true);\n";
    source += "    array<MeshRef@>@ allMeshes = Scene::FindAllMesh(ctx, \"__missing__\", true);\n";
    source += "    array<BehaviorRef@>@ allBehaviors = Scene::FindAllBehavior(ctx, \"__missing__\", true);\n";
    source += "    bool inCurrent = Scene::IsInCurrentScene(ctx, found);\n";
    source += "    bool inScene = Scene::IsInScene(ctx, currentScene, found);\n";
    source += "    Scene::AddToScene(ctx, currentScene, found, true);\n";
    source += "    Scene::RemoveFromScene(ctx, currentScene, found, true);\n";
    source += "    if (e3d !is null) { CK3dEntity@ raw3d = e3d.Entity3D(); if (raw3d !is null) { VxVector p; VxQuaternion q; raw3d.SetPosition(p); raw3d.SetPosition(0.0f, 0.0f, 0.0f); raw3d.GetPosition(p); raw3d.Translate(p); raw3d.Translate(0.0f, 0.0f, 0.0f); raw3d.SetQuaternion(q); raw3d.GetQuaternion(q); raw3d.SetScale(p); raw3d.SetScale(1.0f, 1.0f, 1.0f); raw3d.GetScale(p); raw3d.LookAt(p); raw3d.GetParent(); raw3d.GetChild(0); raw3d.GetChildrenCount(); raw3d.SetParent(null); raw3d.AddChild(null); raw3d.RemoveChild(null); raw3d.SetPickable(); raw3d.IsPickable(); CKMesh@ rawMesh = oneMesh !is null ? oneMesh.Mesh() : null; raw3d.GetCurrentMesh(); raw3d.SetCurrentMesh(rawMesh); raw3d.GetMeshCount(); raw3d.GetMesh(0); raw3d.AddMesh(rawMesh); } }\n";
    source += "    if (e2d !is null) { CK2dEntity@ raw2d = e2d.Entity2D(); if (raw2d !is null) { Vx2DVector p2; VxRect r2; raw2d.SetPosition(p2); raw2d.GetPosition(p2); raw2d.SetSize(p2); raw2d.GetSize(p2); raw2d.SetRect(r2); raw2d.GetRect(r2); raw2d.SetSourceRect(r2); raw2d.GetSourceRect(r2); raw2d.UseSourceRect(); raw2d.IsUsingSourceRect(); CKMaterial@ rawMaterial = oneMaterial !is null ? oneMaterial.Material() : null; raw2d.SetMaterial(rawMaterial); raw2d.GetMaterial(); raw2d.SetPickable(); raw2d.IsPickable(); raw2d.SetClipToParent(); raw2d.IsClipToParent(); raw2d.GetParent(); raw2d.GetChild(0); raw2d.GetChildrenCount(); raw2d.SetParent(null); } }\n";
    source += "    MaterialRef@ material = Scene::FindMaterial(ctx, \"__missing__\");\n";
    source += "    TextureRef@ texture = Scene::FindTexture(ctx, \"__missing__\");\n";
    source += "    MeshRef@ mesh = Scene::FindMesh(ctx, \"__missing__\");\n";
    source += "    BehaviorRef@ behaviorSceneRef = Scene::FindBehavior(ctx, \"__missing__\");\n";
    source += "    if (material !is null) { CKMaterial@ rawMaterial = material.Material(); CKTexture@ rawTexture = texture !is null ? texture.Texture() : null; if (rawMaterial !is null) { rawMaterial.GetTexture(); rawMaterial.SetTexture(rawTexture); } }\n";
    source += "    if (found !is null) { found.IsValid(); bool valid = found.valid; found.Error(); found.Describe(); found.Id(); found.Name(); found.ClassId(); found.IsDynamic(); found.Object(); BehaviorRef@ castBehavior = cast<BehaviorRef>(found); ParamRef@ castParam = cast<ParamRef>(found); SceneObjectRef@ castSceneObject = cast<SceneObjectRef>(found); }\n";
    source += "    SceneObjectRef@ sceneAsObject = currentScene;\n";
    source += "    if (sceneAsObject !is null) { Scene::IsInCurrentScene(ctx, sceneAsObject); Scene::IsInScene(ctx, currentScene, sceneAsObject); }\n";
    source += "    ObjectRef@ behaviorAsObject = behaviorSceneRef;\n";
    source += "    array<ObjectRef@> selectRefs;\n";
    source += "    selectRefs.insertLast(behaviorAsObject);\n";
    source += "    Scene::Select(ctx, selectRefs, true);\n";
    source += "}\n";
    source += "void ProbeBBApi(const CKBehaviorContext &in ctx, ParamValue@ value, ParamRef@ source, ParamOp@ operation) {\n";
    source += "    BBBridge@ bridge = BB::From(ctx); BBPrototype@ missing = BB::Prototype(ctx, \"__missing__\"); CKGUID emptyGuid; BBPrototype@ missingGuid = BB::Prototype(ctx, emptyGuid);\n";
    source += "    BBCallBuilder@ call = BB::Call(ctx, \"__missing__\"); BBTaskBuilder@ spawn = BB::Spawn(ctx, \"__missing__\");\n";
    source += "    int globalCount = BB::Count(ctx); BBPrototype@ globalFirst = BB::At(ctx, 0); BBPrototype@ globalFind = BB::Find(ctx, \"__missing__\");\n";
    source += "    array<BBPrototype@>@ globalAll = BB::FindAll(ctx, \"__missing__\");\n";
    source += "    if (bridge !is null) { bridge.Prototype(\"__missing__\"); bridge.Prototype(emptyGuid); bridge.Count(); bridge.At(0); bridge.Find(\"__missing__\"); array<BBPrototype@>@ bridgeAll = bridge.FindAll(\"__missing__\"); }\n";
    source += "    CKObject@ objectValue = null;\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"__missing__\"); BBDecl@ declGuid = BB::Require(ctx, emptyGuid);\n";
    source += "    if (bridge !is null) { bridge.Require(\"__missing__\"); bridge.Require(emptyGuid); }\n";
    source += "    if (decl !is null) { decl.IsValid(); decl.Error(); decl.Guid(); decl.Name(); decl.Category(); decl.QualifiedName(); decl.BehaviorFlags(); decl.PrototypeFlags(); decl.CompatibleClassId(); decl.NeededManagerCount(); decl.NeededManagerGuid(0); BBLayout@ dl = decl.Layout(); BBSlot@ sin = decl.Input(\"In\"); BBSlot@ sout = decl.Output(\"Out\"); BBSlot@ spin = decl.Pin(\"Value\"); BBSlot@ spout = decl.Pout(\"Result\"); BBSlot@ ssetting = decl.Setting(\"Settings\"); BBSlot@ slocal = decl.Local(\"State\"); BBConfig@ cfg = decl.Configure(); decl.Describe(); if (dl !is null) { dl.InputCount(); dl.OutputCount(); dl.PinCount(); dl.PoutCount(); dl.SettingCount(); dl.LocalCount(); dl.FindSetting(\"Settings\"); dl.Setting(0); dl.Describe(); } if (sin !is null) { sin.IsValid(); sin.Error(); sin.Kind(); sin.Index(); sin.Name(); sin.TypeGuid(); sin.TypeName(); sin.Caps(); sin.LayoutGeneration(); sin.IsSetting(); sin.IsRequired(); sin.IsStart(); sin.IsStop(); sin.HasDefault(); sin.DefaultText(); sin.HasValue(); sin.ValueText(); sin.Describe(); } if (cfg !is null) { cfg.IsValid(); cfg.Error(); cfg.Decl(); cfg.Owner(null); cfg.Target(null); cfg.Set(spin, value); cfg.Set(spin, 1); cfg.Set(spin, 1.0f); cfg.Set(spin, true); cfg.Set(spin, \"x\"); cfg.Set(spin, objectValue); cfg.SetSetting(ssetting, value); cfg.SetSetting(ssetting, \"x\"); cfg.Source(spin, source); cfg.Operation(spin, operation); cfg.Validate(ctx); BBInstance@ instance = cfg.Spawn(ctx); cfg.Raise(ctx); if (instance !is null) { instance.IsValid(); instance.Error(); instance.Decl(); instance.Behavior(); BBLayout@ rl = instance.Layout(); BBSlot@ rin = instance.Input(\"In\"); BBSlot@ rout = instance.Output(\"Out\"); BBSlot@ rpin = instance.PinSlot(\"Value\"); BBSlot@ rpout = instance.PoutSlot(\"Result\"); BBSlot@ rsetting = instance.Setting(\"Settings\"); BBSlot@ rlocal = instance.Local(\"State\"); if (rl !is null) { rl.InputCount(); rl.OutputCount(); rl.PinCount(); rl.PoutCount(); rl.SettingCount(); rl.LocalCount(); } instance.Start(ctx, rin); instance.Start(ctx); instance.Start(sin); instance.Start(); instance.Step(ctx); instance.Stop(ctx); instance.Stop(); instance.OutputActive(rout); instance.Pin(rpin); instance.Pout(rpout); instance.SetSetting(rsetting, \"x\"); instance.Destroy(); instance.Raise(ctx); } } }\n";
    source += "    if (decl !is null) { decl.Configure(); }\n";
    source += "    if (decl !is null) { BBConfig@ c = decl.Configure(); if (c !is null) { BBSlot@ ci = c.Input(\"In\"); BBSlot@ co = c.Output(\"Out\"); BBSlot@ cp = c.Pin(\"Value\"); BBSlot@ cq = c.Pout(\"Result\"); BBSlot@ cs = c.Setting(\"Settings\"); c.Set(cp, value); c.Set(cp, 1); c.Set(cp, 1.0f); c.Set(cp, true); c.Set(cp, \"x\"); c.Set(cp, objectValue); c.SetSetting(cs, \"x\"); c.Source(cp, source); c.Operation(cp, operation); c.Instance(); c.EnsureSpawned(ctx); BBInstance@ si = c.EnsureStarted(ctx); c.Explain(); c.OutputActive(co); c.PinRef(cp); c.PoutRef(cq); c.Destroy(); if (si !is null) { BBSlot@ ri = si.Input(\"In\"); BBSlot@ ro = si.Output(\"Out\"); BBSlot@ rp = si.PinSlot(\"Value\"); BBSlot@ rq = si.PoutSlot(\"Result\"); BBSlot@ rs = si.Setting(\"Settings\"); si.IsAlive(); si.Explain(); si.Start(ctx, ri); si.OutputActive(ro); si.Pin(rp); si.Pout(rq); si.Set(rp, value); si.Set(rp, 1); si.Set(rp, 1.0f); si.Set(rp, true); si.Set(rp, \"x\"); si.Set(rp, objectValue); si.Source(rp, source); si.Operation(rp, operation); si.StepSet(ctx, rp, value); si.StepSet(ctx, rp, \"x\"); si.SetSetting(rs, value); si.SetSetting(rs, \"x\"); si.Stop(ctx); si.Destroy(); } } }\n";
    source += "    if (missing !is null) { missing.IsValid(); missing.GetGuid(); missing.GetName(); missing.GetCategory(); missing.GetQualifiedName(); missing.Layout(); missing.Call(); missing.Spawn(); missing.Describe(); }\n";
    source += "    if (call !is null) { call.Owner(null); call.Target(null); call.Set(0, value); call.SetSource(0, source); call.SetOperation(0, operation); BBResult@ result = call.Run(); if (result !is null) { result.Ok(); bool ok = result.ok; result.ReturnCode(); result.Error(); result.OutputActive(0); result.Pout(0); result.Raise(ctx); } }\n";
    source += "    if (spawn !is null) { spawn.Owner(null); spawn.Target(null); spawn.Set(0, value); spawn.SetSource(0, source); spawn.SetOperation(0, operation); BBTask@ task = spawn.Start(); if (task !is null) { task.IsValid(); task.IsAlive(); task.IsPaused(); task.ReturnCode(); task.Error(); task.OutputActive(0); task.Step(ctx); task.Reset(); task.Behavior(); task.Pout(0); task.Raise(ctx); task.Destroy(); } }\n";
    source += "    BBSlot@ slot = decl !is null ? decl.Pin(\"Value\") : null; BBSlot@ input = decl !is null ? decl.Input(\"In\") : null; BBSlot@ output = decl !is null ? decl.Output(\"Out\") : null;\n";
    source += "    if (call !is null) { call.Set(slot, value); call.Set(slot, 1); call.Set(slot, 1.0f); call.Set(slot, true); call.Set(slot, \"x\"); call.Set(slot, objectValue); call.Source(slot, source); call.Operation(slot, operation); BBResult@ sr = call.Run(input); if (sr !is null) { sr.OutputActive(output); sr.Pout(slot); } }\n";
    source += "    if (spawn !is null) { spawn.Set(slot, value); spawn.Set(slot, 1); spawn.Set(slot, 1.0f); spawn.Set(slot, true); spawn.Set(slot, \"x\"); spawn.Set(slot, objectValue); spawn.Source(slot, source); spawn.Operation(slot, operation); BBTask@ st = spawn.Start(input); if (st !is null) { st.OutputActive(output); st.Step(ctx, input); st.Pout(slot); } }\n";
    source += "}\n";
    source += "int ProbeBBDeclApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 981;\n";
    source += "    CKGUID guid = decl.Guid();\n";
    source += "    if (!guid.IsValid()) return 982;\n";
    source += "    if (decl.Name() == \"\" || decl.Category() == \"\" || decl.QualifiedName() == \"\") return 983;\n";
    source += "    decl.BehaviorFlags(); decl.PrototypeFlags(); decl.CompatibleClassId(); decl.Describe(); decl.Error();\n";
    source += "    int managerCount = decl.NeededManagerCount();\n";
    source += "    if (managerCount < 0) return 984;\n";
    source += "    if (managerCount > 0) { CKGUID managerGuid = decl.NeededManagerGuid(0); if (!managerGuid.IsValid()) return 985; }\n";
    source += "    BBLayout@ layout = decl.Layout();\n";
    source += "    if (layout is null) return 986;\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    BBSlot@ output = decl.Output(\"Out\");\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    if (input is null || !input.IsValid() || output is null || !output.IsValid() || pin is null || !pin.IsValid()) return 987;\n";
    source += "    decl.Pout(\"pOut 0\"); decl.Setting(\"Settings\"); decl.Local(\"State\");\n";
    source += "    BBConfig@ config = decl.Configure();\n";
    source += "    if (config is null || !config.IsValid()) return 988;\n";
    source += "    CKGUID emptyGuid;\n";
    source += "    BBDecl@ missing = BB::Require(ctx, \"__missing__\");\n";
    source += "    BBDecl@ missingGuid = BB::Require(ctx, emptyGuid);\n";
    source += "    if (missing is null || missingGuid is null || missing.IsValid() || missingGuid.IsValid()) return 989;\n";
    source += "    missing.Error(); missing.Describe(); missing.Guid(); missing.Name(); missing.Category(); missing.QualifiedName();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBBridgeApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBBridge@ bridge = BB::From(ctx);\n";
    source += "    if (bridge is null) return 900;\n";
    source += "    int count = bridge.Count();\n";
    source += "    if (count < 0) return 901;\n";
    source += "    BBPrototype@ first = bridge.At(0);\n";
    source += "    if (count > 0 && first is null) return 902;\n";
    source += "    if (first !is null) { first.IsValid(); first.GetGuid(); first.GetName(); first.GetCategory(); first.GetQualifiedName(); first.Describe(); }\n";
    source += "    CKGUID emptyGuid;\n";
    source += "    bridge.Prototype(\"__missing__\");\n";
    source += "    bridge.Prototype(emptyGuid);\n";
    source += "    bridge.Find(\"__missing__\");\n";
    source += "    array<BBPrototype@>@ missingAll = bridge.FindAll(\"__missing__\");\n";
    source += "    if (missingAll is null) return 903;\n";
    source += "    BBDecl@ missingDecl = bridge.Require(\"__missing__\");\n";
    source += "    BBDecl@ missingGuidDecl = bridge.Require(emptyGuid);\n";
    source += "    if (missingDecl is null || missingGuidDecl is null) return 904;\n";
    source += "    missingDecl.IsValid(); missingDecl.Error(); missingDecl.Guid(); missingDecl.Name(); missingDecl.Category(); missingDecl.QualifiedName(); missingDecl.Describe();\n";
    source += "    missingGuidDecl.IsValid(); missingGuidDecl.Error();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBCallBuilderApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 910;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid()) return 911;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 912;\n";
    source += "    CKObject@ objectValue = null;\n";
    source += "    BBCallBuilder@ call = BB::Call(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (call is null) return 913;\n";
    source += "    if (call.Owner(null) is null || call.Target(null) is null || call.Set(0, intValue) is null) return 914;\n";
    source += "    BBResult@ result = call.Run();\n";
    source += "    if (result is null || !result.Ok() || !result.OutputActive(0)) return 915;\n";
    source += "    result.ReturnCode();\n";
    source += "    BBCallBuilder@ slotCall = BB::Call(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (slotCall is null) return 916;\n";
    source += "    slotCall.Set(pin, intValue);\n";
    source += "    slotCall.Set(pin, 8);\n";
    source += "    slotCall.Set(pin, 8.0f);\n";
    source += "    slotCall.Set(pin, true);\n";
    source += "    slotCall.Set(pin, \"8\");\n";
    source += "    slotCall.Set(pin, objectValue);\n";
    source += "    if (slotCall.Set(pin, intValue) is null) return 917;\n";
    source += "    BBResult@ slotResult = slotCall.Run(input);\n";
    source += "    if (slotResult is null || !slotResult.Ok() || !slotResult.OutputActive(0)) return 918;\n";
    source += "    slotResult.ReturnCode();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBConfigApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 940;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    BBSlot@ output = decl.Output(\"Out\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid() || output is null || !output.IsValid()) return 941;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 942;\n";
    source += "    CKObject@ objectValue = null;\n";
    source += "    BBConfig@ cfg = decl.Configure();\n";
    source += "    if (cfg is null || !cfg.IsValid()) return 943;\n";
    source += "    if (cfg.Owner(null) is null || cfg.Target(null) is null || cfg.Set(pin, intValue) is null) return 944;\n";
    source += "    cfg.Set(pin, 8);\n";
    source += "    cfg.Set(pin, 8.0f);\n";
    source += "    cfg.Set(pin, true);\n";
    source += "    cfg.Set(pin, \"8\");\n";
    source += "    cfg.Set(pin, objectValue);\n";
    source += "    if (cfg.Set(pin, intValue) is null || !cfg.Validate(ctx)) return 945;\n";
    source += "    BBInstance@ instance = cfg.SpawnStarted(ctx);\n";
    source += "    if (instance is null || !instance.IsValid()) return 946;\n";
    source += "    if (!cfg.OutputActive(output)) return 947;\n";
    source += "    ParamRef@ pinRef = cfg.PinRef(pin);\n";
    source += "    if (pinRef is null || !pinRef.IsValid()) return 948;\n";
    source += "    BBSlot@ pout = decl.Pout(\"pOut 0\");\n";
    source += "    if (pout !is null && pout.IsValid()) { ParamRef@ poutRef = cfg.PoutRef(pout); if (poutRef is null || !poutRef.IsValid()) return 949; }\n";
    source += "    cfg.Explain();\n";
    source += "    cfg.Destroy();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBResultApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 950;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    BBSlot@ output = decl.Output(\"Out\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid() || output is null || !output.IsValid()) return 951;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 952;\n";
    source += "    BBCallBuilder@ call = BB::Call(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (call is null || call.Set(pin, intValue) is null) return 953;\n";
    source += "    BBResult@ result = call.Run(input);\n";
    source += "    if (result is null || !result.Ok() || !result.OutputActive(output)) return 954;\n";
    source += "    result.ReturnCode(); result.Error();\n";
    source += "    BBSlot@ pout = decl.Pout(\"pOut 0\");\n";
    source += "    if (pout !is null && pout.IsValid()) { ParamRef@ poutRef = result.Pout(pout); if (poutRef is null || !poutRef.IsValid()) return 955; }\n";
    source += "    result.Raise(ctx);\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBTaskApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 960;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    BBSlot@ output = decl.Output(\"Out\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid() || output is null || !output.IsValid()) return 961;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 962;\n";
    source += "    BBTaskBuilder@ spawn = BB::Spawn(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (spawn is null || spawn.Set(pin, intValue) is null) return 963;\n";
    source += "    BBTask@ task = spawn.Start(input);\n";
    source += "    if (task is null || !task.IsValid()) return 964;\n";
    source += "    if (!task.Step(ctx, input) || !task.OutputActive(output)) return 965;\n";
    source += "    task.IsAlive(); task.IsPaused(); task.ReturnCode(); task.Error(); task.Behavior();\n";
    source += "    BBSlot@ pout = decl.Pout(\"pOut 0\");\n";
    source += "    if (pout !is null && pout.IsValid()) { ParamRef@ poutRef = task.Pout(pout); if (poutRef is null || !poutRef.IsValid()) return 966; }\n";
    source += "    task.Raise(ctx);\n";
    source += "    task.Destroy();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBInstanceApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 970;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    BBSlot@ output = decl.Output(\"Out\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid() || output is null || !output.IsValid()) return 971;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 972;\n";
    source += "    BBConfig@ cfg = decl.Configure();\n";
    source += "    if (cfg is null || cfg.Set(pin, intValue) is null) return 973;\n";
    source += "    BBInstance@ instance = cfg.Spawn(ctx);\n";
    source += "    if (instance is null || !instance.IsValid()) return 974;\n";
    source += "    instance.IsAlive(); instance.Error(); instance.Explain(); instance.Decl(); instance.Behavior(); instance.Layout();\n";
    source += "    if (!instance.Set(pin, intValue) || !instance.Set(pin, 8) || !instance.Set(pin, 8.0f) || !instance.Set(pin, true) || !instance.Set(pin, \"8\")) return 975;\n";
    source += "    if (!instance.Set(pin, intValue) || !instance.Start(ctx, input) || !instance.Step(ctx)) return 976;\n";
    source += "    if (!instance.OutputActive(output)) return 977;\n";
    source += "    ParamRef@ pinRef = instance.Pin(pin);\n";
    source += "    if (pinRef is null || !pinRef.IsValid()) return 978;\n";
    source += "    BBSlot@ pout = decl.Pout(\"pOut 0\");\n";
    source += "    if (pout !is null && pout.IsValid()) { ParamRef@ poutRef = instance.Pout(pout); if (poutRef is null || !poutRef.IsValid()) return 979; }\n";
    source += "    if (!instance.StepSet(ctx, pin, intValue) || !instance.StepSet(ctx, pin, 9)) return 980;\n";
    source += "    instance.Stop(ctx);\n";
    source += "    instance.Destroy();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "int ProbeBBTaskBuilderApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BBDecl@ decl = BB::Require(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (decl is null || !decl.IsValid()) return 920;\n";
    source += "    BBSlot@ pin = decl.Pin(\"pIn 0\");\n";
    source += "    BBSlot@ input = decl.Input(\"In\");\n";
    source += "    if (pin is null || !pin.IsValid() || input is null || !input.IsValid()) return 921;\n";
    source += "    ParamValue@ intValue = Param::Int(7);\n";
    source += "    if (intValue is null || !intValue.IsValid()) return 922;\n";
    source += "    CKObject@ objectValue = null;\n";
    source += "    BBTaskBuilder@ spawn = BB::Spawn(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (spawn is null) return 923;\n";
    source += "    if (spawn.Owner(null) is null || spawn.Target(null) is null || spawn.Set(0, intValue) is null) return 924;\n";
    source += "    BBTask@ task = spawn.Start();\n";
    source += "    if (task is null || !task.IsValid()) return 925;\n";
    source += "    if (!task.Step(ctx, 0) || !task.OutputActive(0)) return 926;\n";
    source += "    task.ReturnCode(); task.Destroy();\n";
    source += "    BBTaskBuilder@ slotSpawn = BB::Spawn(ctx, \"Logics/Calculator/Identity\");\n";
    source += "    if (slotSpawn is null) return 927;\n";
    source += "    slotSpawn.Set(pin, intValue);\n";
    source += "    slotSpawn.Set(pin, 8);\n";
    source += "    slotSpawn.Set(pin, 8.0f);\n";
    source += "    slotSpawn.Set(pin, true);\n";
    source += "    slotSpawn.Set(pin, \"8\");\n";
    source += "    slotSpawn.Set(pin, objectValue);\n";
    source += "    if (slotSpawn.Set(pin, intValue) is null) return 928;\n";
    source += "    BBTask@ slotTask = slotSpawn.Start(input);\n";
    source += "    if (slotTask is null || !slotTask.IsValid()) return 929;\n";
    source += "    if (!slotTask.Step(ctx, input) || !slotTask.OutputActive(0)) return 930;\n";
    source += "    slotTask.ReturnCode(); slotTask.Destroy();\n";
    source += "    return 0;\n";
    source += "}\n";
    source += "void ProbeGraphTaskApi(const CKBehaviorContext &in ctx, BehaviorRef@ ref, GraphTask@ task) {\n";
    source += "    BBDecl@ spec = BB::Require(ctx, \"__missing__\"); BBSlot@ input = spec !is null ? spec.Input(\"In\") : null; BBSlot@ output = spec !is null ? spec.Output(\"Out\") : null; BBSlot@ pinSlot = spec !is null ? spec.Pin(\"Value\") : null; BBSlot@ poutSlot = spec !is null ? spec.Pout(\"Value\") : null;\n";
    source += "    if (ref !is null) { BehaviorLayout@ l = ref.Layout(); int p = l.FindPin(\"Value\"); ParamRef@ pin = p >= 0 ? ref.Pin(p) : null; ParamSourceLinkRef@ link = pin !is null ? pin.SetSourceScoped(pin) : null; if (link !is null) { link.IsValid(); link.Commit(); link.Restore(); link.Describe(); } ParamOperationRef@ opRef = null; if (opRef !is null) opRef.Restore(); GraphTask@ s = ref.Start(0); GraphTask@ ss = ref.Start(input); ref.Trigger(input); ref.OutputActive(output); ref.Pin(pinSlot); ref.Pout(poutSlot); GraphTask@ w = ref.Watch(); }\n";
    source += "    GraphTask@ ns = Behavior::Start(ctx, \"__missing__\", 0);\n";
    source += "    GraphTask@ nw = Behavior::Watch(ctx, \"__missing__\");\n";
    source += "    if (task is null) return;\n";
    source += "    task.IsValid(); task.IsAlive(); task.IsPaused(); task.TimedOut(); task.Error(); task.Elapsed();\n";
    source += "    task.Timeout(1.0f); task.Step(ctx); task.Done(); task.Done(0); task.OutputActive(); task.OutputActive(0); task.Cancel(); task.Reset();\n";
    source += "    BehaviorRef@ b = task.Behavior(); task.Raise(ctx);\n";
    source += "    ParamRef@ outp = task.Pout(0); if (outp !is null) { outp.GetText(); ParamValue@ v = outp.Get(); NativeBuffer@ raw = outp.GetRaw(); }\n";
    source += "}\n";
    source += "void ProbeBehaviorGraphApi(const CKBehaviorContext &in ctx) {\n";
    source += "    BehaviorGraph@ graph = Behavior::Graph(ctx);\n";
    source += "    BehaviorBridge@ bridge = Behavior::From(ctx);\n";
    source += "    BehaviorRef@ self = bridge !is null ? bridge.Self() : null;\n";
    source += "    BehaviorGraph@ subGraph = self !is null ? self.AsGraph() : null;\n";
    source += "    BehaviorQuery@ query = Behavior::Query().Name(\"Identity\").Recursive(true).InputCount(-1).OutputCount(-1).PinCount(-1).PoutCount(-1).MaxDepth(16).Occurrence(0);\n";
    source += "    query.PrototypeName(\"Identity\").NameContains(\"Ident\").IncludeRoot(false).TargetName(\"\").TargetId(0);\n";
    source += "    CKGUID emptyGuid; query.PrototypeGuid(emptyGuid).PrototypeQuery(\"guid:0x0,0x0\").Target(null);\n";
    source += "    if (graph !is null) {\n";
    source += "        BehaviorNode@ node = graph.Find(query);\n";
    source += "        BehaviorNode@ required = graph.Require(query);\n";
    source += "        array<BehaviorNode@>@ nodes = graph.FindAll(query);\n";
    source += "        graph.Root(); graph.Describe(); graph.DescribeCandidates(query);\n";
    source += "        BehaviorGraphEdit@ edit = graph.Edit();\n";
    source += "        ParamValue@ value = Param::String(\"x\"); BBSlot@ pinSlot = null; CKObject@ objectValue = null;\n";
    source += "        if (edit !is null) { edit.IsValid(); edit.Error(); edit.Describe(); GraphEditNode@ rootEdit = edit.Import(graph.Root()); GraphEditNode@ clonedRoot = edit.Clone(graph.Root(), \"Clone\"); edit.Target(clonedRoot, null); BBDecl@ missingDecl = BB::Require(ctx, \"__missing__\"); BBConfig@ missingConfig = missingDecl !is null ? missingDecl.Configure() : null; GraphEditNode@ addedDecl = edit.Add(missingDecl); GraphEditNode@ addedConfig = edit.Add(missingConfig, \"Created\"); edit.EnsureInputCount(rootEdit, 1); edit.EnsureOutputCount(rootEdit, 1); GraphEditLink@ pendingLink = edit.Link(rootEdit, 0, rootEdit, 0); GraphEditLink@ pendingSlotLink = edit.Link(rootEdit, null, rootEdit, null); edit.Set(addedDecl, pinSlot, value); edit.Set(addedDecl, pinSlot, 1); edit.Set(addedDecl, pinSlot, 1.0f); edit.Set(addedDecl, pinSlot, true); edit.Set(addedDecl, pinSlot, \"x\"); edit.Set(addedDecl, pinSlot, objectValue); edit.SetSetting(addedDecl, pinSlot, value); edit.SetSetting(addedDecl, pinSlot, \"x\"); edit.Source(addedDecl, pinSlot, null); edit.Operation(addedDecl, pinSlot, null); edit.Remove(graph.Root(), true); edit.Move(graph.Root(), graph); GraphEditResult@ validation = edit.Validate(ctx); GraphEditResult@ applied = edit.Apply(ctx); if (validation !is null) { validation.Ok(); bool ok = validation.ok; validation.IsOk(); validation.Error(); validation.Describe(); validation.CreatedNodes(); validation.CreatedLinks(); validation.Raise(ctx); } if (rootEdit !is null) { rootEdit.IsValid(); rootEdit.Error(); rootEdit.Behavior(); rootEdit.Describe(); } if (pendingLink !is null) { pendingLink.IsValid(); pendingLink.Error(); pendingLink.Link(); pendingLink.Describe(); } }\n";
    source += "        if (node !is null) {\n";
    source += "            node.IsValid(); bool valid = node.valid; node.Error(); node.Describe(); node.Behavior(); node.AsGraph();\n";
    source += "            BehaviorNode@ input = node.Input(0); BehaviorNode@ output = node.Output(0); BehaviorNode@ next = node.Next(query); BehaviorNode@ prev = node.Prev(query); BehaviorNode@ end = node.End(8);\n";
    source += "            array<BehaviorNode@>@ nextAll = node.NextAll(query); array<BehaviorNode@>@ prevAll = node.PrevAll(query);\n";
    source += "            BehaviorLinkRef@ nl = node.NextLink(query); BehaviorLinkRef@ pl = node.PrevLink(query);\n";
    source += "            if (nl !is null) { nl.IsValid(); nl.SourceBehavior(); nl.SourceOutputIndex(); nl.TargetBehavior(); nl.TargetInputIndex(); nl.Delay(); nl.Describe(); }\n";
    source += "            if (pl !is null) { pl.SourceBehavior(); pl.TargetBehavior(); }\n";
    source += "        }\n";
    source += "    }\n";
    source += "    if (subGraph !is null) { subGraph.FindAll(Behavior::Query().Recursive(false)); }\n";
    source += "}\n";
    source += "class ProbeErgonomicComponentFields {\n";
    source += "    BBDecl@ spec;\n";
    source += "    BBSlot@ slot;\n";
    source += "    BBConfig@ binding;\n";
    source += "    BBInstance@ instance;\n";
    source += "}\n";
    source += "int Run(const CKBehaviorContext &in ctx) {\n";
    source += "    const string typeName = \"" + EscapeAngelScriptString(typeName) + "\";\n";
    source += "    const string operationName = \"" + EscapeAngelScriptString(operationName) + "\";\n";
    source += "    const bool hasOperation = operationName != \"\";\n";
    source += "    XObjectArray ids;\n";
    source += "    CK_ID first = 0;\n";
    source += "    CK_ID second = 42;\n";
    source += "    ids.PushBack(first);\n";
    source += "    ids.PushBack(second);\n";
    source += "    if (ids.Size() != 2 || ids[0] != first || ids[1] != second) return 10;\n";
    source += "    ParamTypeInfo@ info = Param::Type(ctx, typeName);\n";
    source += "    if (info is null || !info.IsValid() || info.Describe() == \"\") return 20;\n";
    source += "    int paramCount = Param::Count(ctx);\n";
    source += "    ParamTypeInfo@ firstParamType = Param::At(ctx, 0);\n";
    source += "    ParamTypeInfo@ foundParamType = Param::Find(ctx, typeName);\n";
    source += "    if (paramCount <= 0 || foundParamType is null || foundParamType.Name() != info.Name()) return 21;\n";
    source += "    CKGUID typeGuid = info.Guid();\n";
    source += "    if (!typeGuid.IsValid()) return 30;\n";
    source += "    CKGUID enumGuid(0x6a33ce52, 0x22705502);\n";
    source += "    CKGUID flagsGuid(0x6a33ce53, 0x22705503);\n";
    source += "    CKGUID structGuid(0x6a33ce54, 0x22705504);\n";
    source += "    XGUIDArray memberGuids;\n";
    source += "    memberGuids.PushBack(CKPGUID_INT);\n";
    source += "    memberGuids.PushBack(CKPGUID_STRING);\n";
    source += "    Param::RegisterEnum(ctx, enumGuid, \"__CKAS_ScriptEnum\", \"Alpha=1,Beta=2\");\n";
    source += "    Param::RegisterFlags(ctx, flagsGuid, \"__CKAS_ScriptFlags\", \"A=1,B=2\");\n";
    source += "    Param::RegisterStruct(ctx, structGuid, \"__CKAS_ScriptStruct\", \"Count,Name\", memberGuids);\n";
    source += "    ParamTypeInfo@ enumInfo = Param::Type(ctx, enumGuid);\n";
    source += "    ParamTypeInfo@ flagsInfo = Param::Type(ctx, flagsGuid);\n";
    source += "    ParamTypeInfo@ structInfo = Param::Type(ctx, structGuid);\n";
    source += "    if (enumInfo is null || !enumInfo.IsEnum() || enumInfo.Enum().Find(\"Beta\") != 2) return 31;\n";
    source += "    if (flagsInfo is null || !flagsInfo.IsFlags() || flagsInfo.Flags().Parse(\"A|B\") != 3) return 32;\n";
    source += "    if (structInfo is null || !structInfo.IsStruct() || structInfo.Struct().FindMember(\"Name\") != 1) return 33;\n";
    source += "    if (!Param::Guid(ctx, \"__CKAS_ScriptEnum\").IsValid() || !Param::IsEnum(ctx, enumGuid) || Param::IsEnum(ctx, flagsGuid)) return 37;\n";
    source += "    if (!Param::IsFlags(ctx, flagsGuid) || Param::Value(ctx, enumGuid, \"Beta\", -1) != 2) return 38;\n";
    source += "    if (Param::Flag(ctx, flagsGuid, \"B\", 0) != 2 || Param::FlagsMask(ctx, flagsGuid, \"A|B\", 0) != 3) return 39;\n";
    source += "    if (Param::Text(ctx, enumGuid, 2) != \"Beta\" || Param::Text(ctx, flagsGuid, uint(3)) == \"\" || Param::Describe(ctx, enumGuid) == \"\") return 41;\n";
    source += "    ParamValue@ enumValue = Param::Enum(ctx, enumGuid, \"Alpha\");\n";
    source += "    ParamValue@ flagsValue = Param::Flags(ctx, flagsGuid, \"A,B\");\n";
    source += "    ParamStructValue@ structValue = Param::Struct(ctx, structGuid);\n";
    source += "    if (enumValue is null || enumValue.AsInt() != 1) return 34;\n";
    source += "    if (flagsValue is null || flagsValue.AsInt() != 3) return 35;\n";
    source += "    if (structValue is null || !structValue.Set(0, Param::Int(5)).IsValid() || structValue.Value().AsStruct() is null) return 36;\n";
    source += "    if (hasOperation) {\n";
    source += "        ParamOp@ byName = Param::Operation(ctx, operationName);\n";
    source += "        if (byName is null) return 40;\n";
    source += "        ParamValue@ objectArrayValue = Param::ObjectArray(ids);\n";
    source += "        @byName = byName.Result(typeName).In(0, objectArrayValue).In(1, objectArrayValue);\n";
    source += "        if (byName.Describe() == \"\") return 50;\n";
    source += "        ParamOp@ byNameCtxLast = Param::Operation(ctx, operationName);\n";
    source += "        if (byNameCtxLast is null) return 60;\n";
    source += "    }\n";
    source += "    ParamValue@ textValue = Param::Text(ctx, typeName, \"0\");\n";
    source += "    if (textValue is null || !textValue.IsValid()) return 55;\n";
    source += "    int bbDeclResult = ProbeBBDeclApi(ctx);\n";
    source += "    if (bbDeclResult != 0) return bbDeclResult;\n";
    source += "    int bbBridgeResult = ProbeBBBridgeApi(ctx);\n";
    source += "    if (bbBridgeResult != 0) return bbBridgeResult;\n";
    source += "    int bbCallBuilderResult = ProbeBBCallBuilderApi(ctx);\n";
    source += "    if (bbCallBuilderResult != 0) return bbCallBuilderResult;\n";
    source += "    int bbConfigResult = ProbeBBConfigApi(ctx);\n";
    source += "    if (bbConfigResult != 0) return bbConfigResult;\n";
    source += "    int bbResultResult = ProbeBBResultApi(ctx);\n";
    source += "    if (bbResultResult != 0) return bbResultResult;\n";
    source += "    int bbTaskResult = ProbeBBTaskApi(ctx);\n";
    source += "    if (bbTaskResult != 0) return bbTaskResult;\n";
    source += "    int bbInstanceResult = ProbeBBInstanceApi(ctx);\n";
    source += "    if (bbInstanceResult != 0) return bbInstanceResult;\n";
    source += "    int bbTaskBuilderResult = ProbeBBTaskBuilderApi(ctx);\n";
    source += "    if (bbTaskBuilderResult != 0) return bbTaskBuilderResult;\n";
    source += "    return 0;\n";
    source += "}\n";

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "AngelScript manager is not available.";
        return false;
    }

    CKAngelScriptResult compileResult = {};
    if (manager->CompileModule(moduleName, source.c_str(), CKAS_COMPILE_REPLACEEXISTING, &compileResult) != CKAS_OK) {
        error = compileResult.ErrorMessage && compileResult.ErrorMessage[0] != '\0'
            ? compileResult.ErrorMessage
            : "Failed to build AngelScript self-test module.";
        return false;
    }

    asIScriptModule *module = manager->GetModule(moduleName);
    if (!module) {
        manager->UnloadModule(moduleName, nullptr);
        error = "Failed to retrieve AngelScript self-test module after compilation.";
        return false;
    }

    asIScriptFunction *function = module->GetFunctionByDecl("int Run(const CKBehaviorContext &in ctx)");
    if (!function) {
        error = "Failed to find AngelScript self-test Run() function.";
        manager->UnloadModule(moduleName, nullptr);
        return false;
    }

    CKBehaviorContext behaviorContext;
    behaviorContext.Behavior = nullptr;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.CurrentLevel = nullptr;
    behaviorContext.CurrentScene = nullptr;
    behaviorContext.PreviousScene = nullptr;
    behaviorContext.CurrentRenderContext = nullptr;
    behaviorContext.ParameterManager = parameterManager;
    behaviorContext.MessageManager = nullptr;
    behaviorContext.AttributeManager = nullptr;
    behaviorContext.TimeManager = nullptr;
    behaviorContext.CallbackMessage = 0;
    behaviorContext.CallbackArg = nullptr;

    asIScriptContext *scriptContext = engine->RequestContext();
    if (!scriptContext) {
        error = "Failed to create AngelScript execution context.";
        manager->UnloadModule(moduleName, nullptr);
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
            error = fmt::format("AngelScript self-test returned {}.", returnCode);
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = scriptContext->GetExceptionString();
        error = fmt::format("AngelScript self-test exception: {}.",
                            exception && exception[0] ? exception : "<empty>");
    } else {
        error = fmt::format("AngelScript self-test execution failed ({}).", r);
    }

    scriptContext->Unprepare();
    engine->ReturnContext(scriptContext);
    manager->UnloadModule(moduleName, nullptr);
    return ok;
}

static void DestroySelfTestObject(CKContext *context, CKObject *object) {
    if (context && object && !object->IsToBeDeleted()) {
        context->DestroyObject(object);
    }
}

static CKGUID FindSelfTestOperationGuid(CKContext *context,
                                        CKParameterManager *parameterManager,
                                        CKGUID valueGuid);

static CKGUID FindSelfTestOperationGuid(CKContext *context,
                                        CKParameterManager *parameterManager) {
    return FindSelfTestOperationGuid(context, parameterManager, CKPGUID_INT);
}

static CKGUID FindSelfTestOperationGuid(CKContext *context,
                                        CKParameterManager *parameterManager,
                                        CKGUID valueGuid) {
    if (!context || !parameterManager) {
        return CKGUID();
    }

    for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
        const CKGUID guid = parameterManager->OperationCodeToGuid(i);
        if (!guid.IsValid()) {
            continue;
        }

        CKParameterOperation *operation = context->CreateCKParameterOperation(
            const_cast<CKSTRING>("__CKAS_OperationProbe"),
            guid,
            valueGuid,
            valueGuid,
            valueGuid);
        if (!operation) {
            continue;
        }

        const bool usable = operation->GetOutParameter() != nullptr && operation->GetOperationFunction() != nullptr;
        DestroySelfTestObject(context, operation);
        if (usable) {
            return guid;
        }
    }

    return CKGUID();
}

static ScriptParamValue MakeSelfTestOperationValue(CKContext *context, CKGUID typeGuid, int value) {
    CKParameterManager *pm = context ? context->GetParameterManager() : nullptr;
    if (typeGuid == CKPGUID_FLOAT) {
        return MakeScriptParamFloat(static_cast<float>(value));
    }
    if (typeGuid == CKPGUID_BOOL) {
        return MakeScriptParamBool(value != 0);
    }
    if (typeGuid == CKPGUID_INT) {
        return MakeScriptParamInt(value);
    }
    if (pm && pm->IsTypeCompatible(typeGuid, CKPGUID_INT)) {
        return MakeScriptParamInt(value);
    }
    if (pm && pm->IsTypeCompatible(typeGuid, CKPGUID_FLOAT)) {
        return MakeScriptParamFloat(static_cast<float>(value));
    }
    if (pm && pm->IsTypeCompatible(typeGuid, CKPGUID_BOOL)) {
        return MakeScriptParamBool(value != 0);
    }
    return MakeScriptParamInt(value);
}

static ParamOp *CreateSelfTestParamOperation(ScriptBehaviorBridge *bridge,
                                             const CKBehaviorContext &ctx,
                                             CKGUID operationGuid,
                                             CKGUID typeGuid,
                                             bool invalidInput) {
    CKContext *context = ctx.Context ? ctx.Context : (bridge && bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr);
    ParamOp *operation = new ParamOp(context, operationGuid);
    ParamOp *returnedOperation = operation->Result(typeGuid);
    if (returnedOperation) {
        returnedOperation->Release();
    }

    int rawValue = 7;
    ParamValue *firstValue = invalidInput
        ? new ParamValue(MakeScriptParamRaw(typeGuid, DescribeScriptParamType(ctx.Context, typeGuid), &rawValue, 1))
        : new ParamValue(MakeSelfTestOperationValue(ctx.Context, typeGuid, 1));
    ParamValue *secondValue = new ParamValue(MakeSelfTestOperationValue(ctx.Context, typeGuid, 2));
    returnedOperation = operation->InValue(0, firstValue);
    if (returnedOperation) {
        returnedOperation->Release();
    }
    returnedOperation = operation->InValue(1, secondValue);
    if (returnedOperation) {
        returnedOperation->Release();
    }
    firstValue->Release();
    secondValue->Release();
    return operation;
}

static bool RunLiveOperationReplacementSelfTest(CKContext *context,
                                                ScriptBehaviorBridge *bridge,
                                                const CKBehaviorContext &ctx,
                                                bool throughConfig,
                                                std::string &error) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    if (!context || !bridge || !parameterManager) {
        error = "Live operation replacement self-test requires CKContext, bridge, and CKParameterManager.";
        return false;
    }

    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(ctx);
    request.PrototypeName = "Logics/Calculator/Identity";
    BBConfig *config = new BBConfig(bridge, ctx, request);
    BBSlot *pin = config ? config->Pin("pIn 0") : nullptr;
    BBInstance *instance = nullptr;
    ParamOp *goodOperation = nullptr;
    ParamOp *badOperation = nullptr;

    auto cleanup = [&]() {
        if (badOperation) {
            badOperation->Release();
            badOperation = nullptr;
        }
        if (goodOperation) {
            goodOperation->Release();
            goodOperation = nullptr;
        }
        if (instance) {
            instance->Destroy();
            instance->Release();
            instance = nullptr;
        }
        if (pin) {
            pin->Release();
            pin = nullptr;
        }
        if (config) {
            config->Release();
            config = nullptr;
        }
    };

    if (!config || !config->IsValid() || !pin || !pin->IsValid()) {
        error = fmt::format("{} operation replacement self-test could not resolve Identity.pIn 0: {}.",
                            throughConfig ? "BBConfig" : "BBInstance",
                            config ? config->Error() : std::string("<null config>"));
        cleanup();
        return false;
    }

    instance = config->SpawnInstance(ctx);
    CKBehavior *behavior = instance ? bridge->GetInstanceBehavior(instance->BridgeInstanceId(), instance->BridgeGeneration()) : nullptr;
    if (!instance || !behavior) {
        error = fmt::format("{} operation replacement self-test could not spawn Identity instance: {}.",
                            throughConfig ? "BBConfig" : "BBInstance",
                            instance ? instance->Error() : config->Error());
        cleanup();
        return false;
    }

    CKParameterIn *createdPin = behavior->CreateInputParameter(const_cast<CKSTRING>("__CKAS_OperationReplacementPin"), CKPGUID_INT);
    bridge->InvalidateBehaviorLayout(behavior->GetID());
    pin->Release();
    pin = instance->PinSlot("__CKAS_OperationReplacementPin");
    CKParameterIn *targetPin = createdPin && pin && pin->IsValid() && pin->Index() >= 0 && pin->Index() < behavior->GetInputParameterCount()
        ? behavior->GetInputParameter(pin->Index())
        : nullptr;
    if (!createdPin || !pin || !pin->IsValid() || targetPin != createdPin) {
        error = fmt::format("{} operation replacement self-test could not create runtime operation pin: {}.",
                            throughConfig ? "BBConfig" : "BBInstance",
                            pin ? pin->Error() : std::string("<null slot>"));
        cleanup();
        return false;
    }

    const CKGUID valueGuid = targetPin->GetGUID();
    CKGUID operationGuid;
    for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
        const CKGUID candidateGuid = parameterManager->OperationCodeToGuid(i);
        if (!candidateGuid.IsValid()) {
            continue;
        }

        ParamOp *candidate = CreateSelfTestParamOperation(bridge, ctx, candidateGuid, valueGuid, false);
        const bool accepted = throughConfig
            ? [&]() {
                  BBConfig *returnedConfig = config->OperationSlot(pin, candidate);
                  const bool acceptedConfig = returnedConfig != nullptr;
                  if (returnedConfig) {
                      returnedConfig->Release();
                  }
                  return acceptedConfig;
              }()
            : instance->OperationSlot(pin, candidate);
        if (accepted && targetPin->GetDirectSource()) {
            goodOperation = candidate;
            operationGuid = candidateGuid;
            break;
        }
        candidate->Release();
    }

    CKParameter *goodSource = targetPin->GetDirectSource();
    CKParameterOperation *installedOperation = goodSource ? CKParameterOperation::Cast(goodSource->GetOwner()) : nullptr;
    if (!operationGuid.IsValid() || !goodSource || !installedOperation || installedOperation->DoOperation() != CK_OK) {
        error = fmt::format("{} operation replacement self-test could not install any operation on Identity.pIn 0 (type {}).",
                            throughConfig ? "BBConfig" : "BBInstance",
                            ParameterTypeLabel(context, valueGuid));
        cleanup();
        return false;
    }

    badOperation = CreateSelfTestParamOperation(bridge, ctx, operationGuid, valueGuid, true);
    const bool badAccepted = throughConfig
        ? [&]() {
              BBConfig *returnedConfig = config->OperationSlot(pin, badOperation);
              const bool accepted = returnedConfig != nullptr;
              if (returnedConfig) {
                  returnedConfig->Release();
              }
              return accepted;
          }()
        : instance->OperationSlot(pin, badOperation);
    if (badAccepted ||
        targetPin->GetDirectSource() != goodSource ||
        installedOperation->IsToBeDeleted() ||
        installedOperation->DoOperation() != CK_OK) {
        error = fmt::format("{} runtime self-test did not preserve live operation after failed operation replacement.",
                            throughConfig ? "BBConfig" : "BBInstance");
        cleanup();
        return false;
    }

    cleanup();
    return true;
}

static bool RunLiveMixedLinkReplacementSelfTest(CKContext *context,
                                                ScriptBehaviorBridge *bridge,
                                                const CKBehaviorContext &ctx,
                                                bool throughConfig,
                                                std::string &error) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    if (!context || !bridge || !parameterManager) {
        error = "Mixed link replacement self-test requires CKContext, bridge, and CKParameterManager.";
        return false;
    }

    auto runWithTarget = [&](const char *scenario, const std::function<bool(BBConfig *, BBInstance *, BBSlot *, CKParameterIn *)> &body) -> bool {
        ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(ctx);
        request.PrototypeName = "Logics/Calculator/Identity";
        BBConfig *config = new BBConfig(bridge, ctx, request);
        BBInstance *instance = config ? config->SpawnInstance(ctx) : nullptr;
        CKBehavior *behavior = instance ? bridge->GetInstanceBehavior(instance->BridgeInstanceId(), instance->BridgeGeneration()) : nullptr;
        CKParameterIn *createdPin = behavior ? behavior->CreateInputParameter(const_cast<CKSTRING>("__CKAS_MixedReplacementPin"), CKPGUID_INT) : nullptr;
        if (behavior) {
            bridge->InvalidateBehaviorLayout(behavior->GetID());
        }
        BBSlot *pin = instance ? instance->PinSlot("__CKAS_MixedReplacementPin") : nullptr;
        CKParameterIn *targetPin = createdPin && pin && pin->IsValid() && pin->Index() >= 0 && behavior && pin->Index() < behavior->GetInputParameterCount()
            ? behavior->GetInputParameter(pin->Index())
            : nullptr;

        bool ok = config && config->IsValid() && instance && instance->IsValid() && behavior && targetPin == createdPin;
        if (!ok) {
            error = fmt::format("{} mixed link replacement setup failed: {}.",
                                scenario,
                                pin ? pin->Error() : (instance ? instance->Error() : (config ? config->Error() : std::string("<null config>"))));
        } else {
            ok = body(config, instance, pin, targetPin);
        }

        if (pin) {
            pin->Release();
        }
        if (instance) {
            instance->Destroy();
            instance->Release();
        }
        if (config) {
            config->Release();
        }
        return ok;
    };

    auto installOperation = [&](BBConfig *config,
                                BBInstance *instance,
                                BBSlot *pin,
                                CKParameterIn *targetPin,
                                CKGUID &operationGuid,
                                CKParameterOperation *&installedOperation) -> bool {
        operationGuid = CKGUID();
        installedOperation = nullptr;
        const CKGUID valueGuid = targetPin->GetGUID();
        for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
            const CKGUID candidateGuid = parameterManager->OperationCodeToGuid(i);
            if (!candidateGuid.IsValid()) {
                continue;
            }

            ParamOp *candidate = CreateSelfTestParamOperation(bridge, ctx, candidateGuid, valueGuid, false);
            const bool accepted = throughConfig
                ? [&]() {
                      BBConfig *returnedConfig = config->OperationSlot(pin, candidate);
                      const bool acceptedConfig = returnedConfig != nullptr;
                      if (returnedConfig) {
                          returnedConfig->Release();
                      }
                      return acceptedConfig;
                  }()
                : instance->OperationSlot(pin, candidate);
            candidate->Release();
            CKParameter *source = targetPin->GetDirectSource();
            CKParameterOperation *operation = source ? CKParameterOperation::Cast(source->GetOwner()) : nullptr;
            if (accepted && operation && operation->DoOperation() == CK_OK) {
                operationGuid = candidateGuid;
                installedOperation = operation;
                return true;
            }
        }
        return false;
    };

    auto makeSource = [&](const char *name, CKParameterLocal *&source, ParamRef *&ref) -> bool {
        source = context->CreateCKParameterLocal(const_cast<CKSTRING>(name), CKPGUID_INT, TRUE);
        ref = source ? new ParamRef(bridge, source->GetID(), ScriptBridgeSlotKind::Standalone, -1) : nullptr;
        return source && ref;
    };

    if (!runWithTarget(throughConfig ? "BBConfig operation-to-source" : "BBInstance operation-to-source",
                       [&](BBConfig *config, BBInstance *instance, BBSlot *pin, CKParameterIn *targetPin) {
                           CKGUID operationGuid;
                           CKParameterOperation *installedOperation = nullptr;
                           if (!installOperation(config, instance, pin, targetPin, operationGuid, installedOperation)) {
                               error = "Mixed link replacement self-test could not install initial operation.";
                               return false;
                           }

                           CKParameterLocal *source = nullptr;
                           ParamRef *sourceRef = nullptr;
                           if (!makeSource("__CKAS_OperationToSource", source, sourceRef)) {
                               if (sourceRef) sourceRef->Release();
                               DestroySelfTestObject(context, source);
                               error = "Mixed link replacement self-test could not create replacement source.";
                               return false;
                           }
                           const bool accepted = throughConfig
                               ? [&]() {
                                     BBConfig *returnedConfig = config->SourceSlot(pin, sourceRef);
                                     const bool acceptedConfig = returnedConfig != nullptr;
                                     if (returnedConfig) {
                                         returnedConfig->Release();
                                     }
                                     return acceptedConfig;
                                 }()
                               : instance->SourceSlot(pin, sourceRef);
                           sourceRef->Release();
                           const bool ok = accepted && targetPin->GetDirectSource() == source && installedOperation->IsToBeDeleted();
                           DestroySelfTestObject(context, source);
                           if (!ok) {
                               error = fmt::format("{} mixed link replacement did not detached-destroy old operation when replacing it with a source.",
                                                   throughConfig ? "BBConfig" : "BBInstance");
                           }
                           return ok;
                       })) {
        return false;
    }

    if (!runWithTarget(throughConfig ? "BBConfig source-to-operation" : "BBInstance source-to-operation",
                       [&](BBConfig *config, BBInstance *instance, BBSlot *pin, CKParameterIn *targetPin) {
                           CKParameterLocal *source = nullptr;
                           ParamRef *sourceRef = nullptr;
                           if (!makeSource("__CKAS_SourceToOperation", source, sourceRef)) {
                               if (sourceRef) sourceRef->Release();
                               DestroySelfTestObject(context, source);
                               error = "Mixed link replacement self-test could not create initial source.";
                               return false;
                           }
                           const bool sourceAccepted = throughConfig
                               ? [&]() {
                                     BBConfig *returnedConfig = config->SourceSlot(pin, sourceRef);
                                     const bool acceptedConfig = returnedConfig != nullptr;
                                     if (returnedConfig) {
                                         returnedConfig->Release();
                                     }
                                     return acceptedConfig;
                                 }()
                               : instance->SourceSlot(pin, sourceRef);
                           sourceRef->Release();
                           if (!sourceAccepted || targetPin->GetDirectSource() != source) {
                               DestroySelfTestObject(context, source);
                               error = "Mixed link replacement self-test could not install initial source.";
                               return false;
                           }

                           CKGUID operationGuid;
                           CKParameterOperation *installedOperation = nullptr;
                           const bool ok = installOperation(config, instance, pin, targetPin, operationGuid, installedOperation) &&
                                           targetPin->GetDirectSource() &&
                                           CKParameterOperation::Cast(targetPin->GetDirectSource()->GetOwner()) == installedOperation;
                           DestroySelfTestObject(context, source);
                           if (!ok) {
                               error = fmt::format("{} mixed link replacement did not preserve new operation when replacing a source.",
                                                   throughConfig ? "BBConfig" : "BBInstance");
                           }
                           return ok;
                       })) {
        return false;
    }

    if (!runWithTarget(throughConfig ? "BBConfig operation-to-value" : "BBInstance operation-to-value",
                       [&](BBConfig *config, BBInstance *instance, BBSlot *pin, CKParameterIn *targetPin) {
                           CKGUID operationGuid;
                           CKParameterOperation *installedOperation = nullptr;
                           if (!installOperation(config, instance, pin, targetPin, operationGuid, installedOperation)) {
                               error = "Mixed link replacement self-test could not install operation before value write.";
                               return false;
                           }

                           const bool accepted = throughConfig
                               ? [&]() {
                                     BBConfig *returnedConfig = config->SetSlotInt(pin, 42);
                                     const bool acceptedConfig = returnedConfig != nullptr;
                                     if (returnedConfig) {
                                         returnedConfig->Release();
                                     }
                                     return acceptedConfig;
                                 }()
                               : instance->SetSlotInt(pin, 42);
                           CKParameter *valueSource = targetPin->GetDirectSource();
                           if (!accepted ||
                               !valueSource ||
                               CKParameterOperation::Cast(valueSource->GetOwner()) == installedOperation ||
                               !installedOperation->IsToBeDeleted()) {
                               error = fmt::format("{} mixed link replacement did not clear old operation on value write.",
                                                   throughConfig ? "BBConfig" : "BBInstance");
                               return false;
                           }
                           return true;
                       })) {
        return false;
    }

    return true;
}

static bool RunBehaviorBridgeNativeMutationSelfTest(CKContext *context,
                                                    CKParameterManager *parameterManager,
                                                    std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !parameterManager || !bridge) {
        error = "Bridge native mutation self-test requires CKContext, CKParameterManager, and ScriptBehaviorBridge.";
        return false;
    }

    CKParameterIn *input = context->CreateCKParameterIn(const_cast<CKSTRING>("__CKAS_LinkInput"), CKPGUID_INT, TRUE);
    CKParameterOut *sourceA = context->CreateCKParameterOut(const_cast<CKSTRING>("__CKAS_LinkSourceA"), CKPGUID_INT, TRUE);
    CKParameterOut *sourceB = context->CreateCKParameterOut(const_cast<CKSTRING>("__CKAS_LinkSourceB"), CKPGUID_INT, TRUE);
    if (!input || !sourceA || !sourceB) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Failed to create parameters for scoped source self-test.";
        return false;
    }

    if (input->SetDirectSource(sourceA) != CK_OK || input->GetDirectSource() != sourceA) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Scoped source self-test failed to initialize direct source.";
        return false;
    }

    auto installScopedSource = [&]() -> ParamSourceLinkRef * {
        ParamRef *inputRef = new ParamRef(bridge, input->GetID(), ScriptBridgeSlotKind::Pin, 0);
        ParamRef *sourceRef = new ParamRef(bridge, sourceB->GetID(), ScriptBridgeSlotKind::Standalone, -1);
        ParamSourceLinkRef *link = inputRef->SetSourceScoped(sourceRef);
        sourceRef->Release();
        inputRef->Release();
        return link;
    };

    ParamSourceLinkRef *link = installScopedSource();
    if (!link || input->GetDirectSource() != sourceB) {
        if (link) {
            link->Release();
        }
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "SetSourceScoped did not install the new source.";
        return false;
    }
    link->Release();
    if (input->GetDirectSource() != sourceA) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamSourceLinkRef destructor did not restore an uncommitted source link.";
        return false;
    }

    link = installScopedSource();
    if (!link || !link->Commit()) {
        if (link) {
            link->Release();
        }
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamSourceLinkRef.Commit failed.";
        return false;
    }
    link->Release();
    if (input->GetDirectSource() != sourceB) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamSourceLinkRef.Commit did not keep the installed source.";
        return false;
    }
    input->SetDirectSource(sourceA);

    link = installScopedSource();
    if (!link || !link->DestroyDetached()) {
        if (link) {
            link->Release();
        }
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamSourceLinkRef.DestroyDetached failed.";
        return false;
    }
    link->Release();
    if (input->GetDirectSource() != sourceB) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamSourceLinkRef.DestroyDetached restored the target source.";
        return false;
    }
    input->SetDirectSource(sourceA);

    link = installScopedSource();
    if (!link || !link->Restore() || input->GetDirectSource() != sourceA) {
        if (link) {
            link->Release();
        }
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "SetSourceScoped did not restore the previous source.";
        return false;
    }
    link->Release();

    const CKGUID operationGuid = FindSelfTestOperationGuid(context, parameterManager);
    if (!operationGuid.IsValid()) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        return true;
    }

    CKParameterOperation *operation = context->CreateCKParameterOperation(
        const_cast<CKSTRING>("__CKAS_RollbackOperation"),
        operationGuid,
        CKPGUID_INT,
        CKPGUID_INT,
        CKPGUID_INT);
    if (!operation || !operation->GetOutParameter()) {
        DestroySelfTestObject(context, operation);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Failed to create operation for rollback self-test.";
        return false;
    }

    if (input->SetDirectSource(operation->GetOutParameter()) != CK_OK ||
        input->GetDirectSource() != operation->GetOutParameter()) {
        DestroySelfTestObject(context, operation);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Operation rollback self-test failed to install operation output.";
        return false;
    }

    ParamOperationRef *operationRef = new ParamOperationRef(bridge, operation->GetID(), input, sourceA);
    if (!operationRef->Restore() || input->GetDirectSource() != sourceA) {
        operationRef->Release();
        DestroySelfTestObject(context, operation);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamOperationRef.Restore did not restore the previous source.";
        return false;
    }

    if (!operationRef->Destroy() || operationRef->IsValid()) {
        operationRef->Release();
        DestroySelfTestObject(context, operation);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamOperationRef.Destroy did not destroy the operation.";
        return false;
    }
    operationRef->Release();

    operation = context->CreateCKParameterOperation(
        const_cast<CKSTRING>("__CKAS_DetachedOperation"),
        operationGuid,
        CKPGUID_INT,
        CKPGUID_INT,
        CKPGUID_INT);
    if (!operation || !operation->GetOutParameter() ||
        input->SetDirectSource(operation->GetOutParameter()) != CK_OK ||
        input->GetDirectSource() != operation->GetOutParameter()) {
        DestroySelfTestObject(context, operation);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Operation detached-destroy self-test failed to install operation output.";
        return false;
    }
    operationRef = new ParamOperationRef(bridge, operation->GetID(), input, sourceA);
    const bool detachedOk = operationRef->DestroyDetached();
    const bool detachedStillValid = operationRef->IsValid();
    const bool detachedRestoredPrevious = input->GetDirectSource() == sourceA;
    if (!detachedOk || detachedRestoredPrevious || detachedStillValid) {
        operationRef->Release();
        if (detachedStillValid) {
            DestroySelfTestObject(context, operation);
        }
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "ParamOperationRef.DestroyDetached restored the previous source or kept the operation valid.";
        return false;
    }
    operationRef->Release();
    input->SetDirectSource(sourceA);

    CKBehavior *graphBehavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_OperationFailureRollback"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!graphBehavior) {
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Failed to create graph behavior for operation failure rollback self-test.";
        return false;
    }
    graphBehavior->UseGraph();
    CKParameterIn *targetPin = graphBehavior->CreateInputParameter(const_cast<CKSTRING>("Target"), CKPGUID_INT);
    if (!targetPin || targetPin->SetDirectSource(sourceA) != CK_OK) {
        DestroySelfTestObject(context, graphBehavior);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = "Failed to initialize graph behavior input for operation failure rollback self-test.";
        return false;
    }

    const int localCountBeforeFailure = graphBehavior->GetLocalParameterCount();
    const int operationCountBeforeFailure = graphBehavior->GetParameterOperationCount();
    ScriptBridgeOperationSpec failingRequest;
    failingRequest.TargetPinIndex = 0;
    failingRequest.OperationGuid = operationGuid;
    failingRequest.ResultTypeGuid = CKPGUID_INT;
    int rawValue = 7;
    failingRequest.In1.Kind = ScriptBridgeInputBindingKind::Value;
    failingRequest.In1.Value = MakeScriptParamRaw(CKPGUID_INT, "Integer", &rawValue, 1);
    failingRequest.In2.Kind = ScriptBridgeInputBindingKind::Value;
    failingRequest.In2.Value = MakeScriptParamInt(1);

    std::string failureMessage;
    std::vector<CK_ID> operationIds;
    ParamOperationRef *failedOperation = ConnectOperationToInput(
        bridge,
        graphBehavior,
        0,
        failingRequest,
        failureMessage,
        false,
        &operationIds);
    if (failedOperation ||
        targetPin->GetDirectSource() != sourceA ||
        graphBehavior->GetLocalParameterCount() != localCountBeforeFailure ||
        graphBehavior->GetParameterOperationCount() != operationCountBeforeFailure ||
        !operationIds.empty()) {
        if (failedOperation) {
            failedOperation->Release();
        }
        DestroySelfTestObject(context, graphBehavior);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = fmt::format("Operation failure rollback self-test left graph mutations behind: sourceRestored={} locals {}->{}, ops {}->{}, ids={}.",
                            targetPin->GetDirectSource() == sourceA ? "true" : "false",
                            localCountBeforeFailure,
                            graphBehavior->GetLocalParameterCount(),
                            operationCountBeforeFailure,
                            graphBehavior->GetParameterOperationCount(),
                            operationIds.size());
        return false;
    }

    ScriptBridgeOperationSpec successfulRequest;
    successfulRequest.TargetPinIndex = 0;
    successfulRequest.OperationGuid = operationGuid;
    successfulRequest.ResultTypeGuid = CKPGUID_INT;
    successfulRequest.In1.Kind = ScriptBridgeInputBindingKind::Value;
    successfulRequest.In1.Value = MakeScriptParamInt(1);
    successfulRequest.In2.Kind = ScriptBridgeInputBindingKind::Value;
    successfulRequest.In2.Value = MakeScriptParamInt(2);

    std::string successMessage;
    ParamOperationRef *successfulOperation = ConnectOperationToInput(
        bridge,
        graphBehavior,
        0,
        successfulRequest,
        successMessage,
        false,
        nullptr);
    if (!successfulOperation ||
        targetPin->GetDirectSource() == sourceA ||
        graphBehavior->GetLocalParameterCount() != localCountBeforeFailure + 2 ||
        graphBehavior->GetParameterOperationCount() != operationCountBeforeFailure + 1) {
        if (successfulOperation) {
            successfulOperation->Destroy();
            successfulOperation->Release();
        }
        DestroySelfTestObject(context, graphBehavior);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = fmt::format("Operation success setup self-test failed: message='{}', locals {}->{}, ops {}->{}.",
                            successMessage,
                            localCountBeforeFailure,
                            graphBehavior->GetLocalParameterCount(),
                            operationCountBeforeFailure,
                            graphBehavior->GetParameterOperationCount());
        return false;
    }

    if (!successfulOperation->Destroy() ||
        targetPin->GetDirectSource() != sourceA ||
        graphBehavior->GetLocalParameterCount() != localCountBeforeFailure ||
        graphBehavior->GetParameterOperationCount() != operationCountBeforeFailure) {
        successfulOperation->Release();
        DestroySelfTestObject(context, graphBehavior);
        DestroySelfTestObject(context, input);
        DestroySelfTestObject(context, sourceA);
        DestroySelfTestObject(context, sourceB);
        error = fmt::format("Operation destroy cleanup self-test failed: sourceRestored={} locals {}->{}, ops {}->{}.",
                            targetPin->GetDirectSource() == sourceA ? "true" : "false",
                            localCountBeforeFailure,
                            graphBehavior->GetLocalParameterCount(),
                            operationCountBeforeFailure,
                            graphBehavior->GetParameterOperationCount());
        return false;
    }
    successfulOperation->Release();

    DestroySelfTestObject(context, graphBehavior);
    DestroySelfTestObject(context, input);
    DestroySelfTestObject(context, sourceA);
    DestroySelfTestObject(context, sourceB);
    return true;
}

static bool RunBehaviorBridgeNativeGraphTaskSelfTest(CKContext *context,
                                                     std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "GraphTask native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_GraphTaskSelfTest"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = "Failed to create behavior for GraphTask self-test.";
        return false;
    }

    if (!behavior->CreateOutput(const_cast<CKSTRING>("Done"))) {
        DestroySelfTestObject(context, behavior);
        error = "Failed to create output for GraphTask self-test.";
        return false;
    }

    GraphTask *task = bridge->CreateGraphWatch(behavior, 0, 0.0f);
    if (!task || !task->IsValid() || !task->IsAlive() || task->Done(-1) || task->OutputActive(-1)) {
        if (task) {
            task->Release();
        }
        DestroySelfTestObject(context, behavior);
        error = "GraphTask initial state self-test failed.";
        return false;
    }

    CKBehaviorContext behaviorContext;
    behaviorContext.Behavior = behavior;
    behaviorContext.DeltaTime = 0.0f;
    behaviorContext.Context = context;
    behaviorContext.CurrentLevel = nullptr;
    behaviorContext.CurrentScene = nullptr;
    behaviorContext.PreviousScene = nullptr;
    behaviorContext.CurrentRenderContext = nullptr;
    behaviorContext.ParameterManager = context->GetParameterManager();
    behaviorContext.MessageManager = nullptr;
    behaviorContext.AttributeManager = nullptr;
    behaviorContext.TimeManager = nullptr;
    behaviorContext.CallbackMessage = 0;
    behaviorContext.CallbackArg = nullptr;

    behavior->ActivateOutput(0, TRUE);
    if (!task->Step(behaviorContext) || !task->OutputActive(0) || !task->Done(0)) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask active output self-test failed.";
        return false;
    }

    behavior->ActivateOutput(0, FALSE);
    if (!task->Step(behaviorContext) || task->OutputActive(0) || !task->Done(0)) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask sticky output self-test failed.";
        return false;
    }

    if (!task->Reset() || task->Done(0) || task->TimedOut() || !task->IsAlive()) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask reset self-test failed.";
        return false;
    }

    task->Timeout(0.1f);
    behaviorContext.DeltaTime = 0.2f;
    if (task->Step(behaviorContext) || !task->TimedOut() || task->IsAlive() || task->Error().empty()) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask timeout self-test failed.";
        return false;
    }

    if (!task->Reset() || task->TimedOut() || !task->IsAlive()) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask timeout reset self-test failed.";
        return false;
    }

    if (!task->Cancel() || task->IsValid()) {
        task->Release();
        DestroySelfTestObject(context, behavior);
        error = "GraphTask cancel self-test failed.";
        return false;
    }

    task->Release();
    DestroySelfTestObject(context, behavior);
    return true;
}

static bool RunBehaviorBridgeNativeGraphSearchSelfTest(CKContext *context,
                                                       std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "Graph search native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *root = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphSearchRoot", CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *source = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphSearchSource", CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *target = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphSearchTarget", CK_OBJECTCREATION_DYNAMIC));
    CKBehaviorLink *link = CKBehaviorLink::Cast(context->CreateObject(CKCID_BEHAVIORLINK, "__CKAS_GraphSearchLink", CK_OBJECTCREATION_DYNAMIC));
    if (!root || !source || !target || !link) {
        DestroySelfTestObject(context, link);
        DestroySelfTestObject(context, target);
        DestroySelfTestObject(context, source);
        DestroySelfTestObject(context, root);
        error = "Graph search self-test failed to create graph objects.";
        return false;
    }

    root->UseGraph();
    source->CreateOutput("Out");
    source->CreateInputParameter("Value", CKPGUID_INT);
    target->CreateInput("In");
    target->CreateOutputParameter("Result", CKPGUID_INT);
    root->AddSubBehavior(source);
    root->AddSubBehavior(target);
    link->SetInBehaviorIO(source->GetOutput(0));
    link->SetOutBehaviorIO(target->GetInput(0));
    link->SetActivationDelay(2);
    root->AddSubBehaviorLink(link);

    CKBehaviorContext ctx = {};
    ctx.Context = context;
    ctx.Behavior = root;
    ctx.ParameterManager = context->GetParameterManager();

    auto cleanup = [&]() {
        if (root && link) {
            root->RemoveSubBehaviorLink(link);
        }
        if (root && source) {
            root->RemoveSubBehavior(source);
        }
        if (root && target) {
            root->RemoveSubBehavior(target);
        }
        DestroySelfTestObject(context, link);
        DestroySelfTestObject(context, target);
        DestroySelfTestObject(context, source);
        DestroySelfTestObject(context, root);
    };

    BehaviorGraph *graph = new BehaviorGraph(bridge, ctx, root->GetID());
    BehaviorRef *rootRef = bridge->WrapBehavior(root, root->GetID());
    BehaviorGraph *graphFromRef = rootRef ? rootRef->AsGraph() : nullptr;
    BehaviorNode *rootFromRef = graphFromRef ? graphFromRef->Root() : nullptr;
    if (!graphFromRef || !graphFromRef->IsValid() ||
        !rootFromRef || !rootFromRef->IsValid() ||
        rootFromRef->BehaviorId() != root->GetID()) {
        error = "Graph search self-test failed BehaviorRef.AsGraph root lookup.";
        if (rootFromRef) rootFromRef->Release();
        if (graphFromRef) graphFromRef->Release();
        if (rootRef) rootRef->Release();
        graph->Release();
        cleanup();
        return false;
    }
    rootFromRef->Release();
    graphFromRef->Release();
    rootRef->Release();

    BehaviorQuery *sourceQuery = (new BehaviorQuery())->Name("__CKAS_GraphSearchSource")->Recursive(true)->PinCount(1);
    BehaviorNode *sourceNode = graph->Require(sourceQuery);
    if (!graph->IsValid() || !sourceNode || !sourceNode->IsValid()) {
        error = "Graph search self-test failed exact Require: " + (sourceNode ? sourceNode->Error() : std::string("<null>"));
        if (sourceNode) sourceNode->Release();
        sourceQuery->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorNode *next = sourceNode->Next(nullptr);
    BehaviorLinkRef *nextLink = sourceNode->NextLink(nullptr);
    if (!next || !next->IsValid() || !nextLink || !nextLink->IsValid() ||
        next->BehaviorId() != target->GetID() ||
        nextLink->SourceOutputIndex() != 0 ||
        nextLink->TargetInputIndex() != 0 ||
        nextLink->Delay() != 2) {
        error = "Graph search self-test failed next/link traversal.";
        if (next) next->Release();
        if (nextLink) nextLink->Release();
        sourceNode->Release();
        sourceQuery->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorQuery *containsQuery = (new BehaviorQuery())->NameContains("GraphSearch")->Recursive(false);
    CScriptArray *all = graph->FindAll(containsQuery);
    BehaviorQuery *duplicateQuery = (new BehaviorQuery())->NameContains("GraphSearch")->Recursive(true);
    BehaviorNode *requiredDuplicate = graph->Require(duplicateQuery);
    if (!all || all->GetSize() != 2 || !requiredDuplicate || requiredDuplicate->IsValid() || requiredDuplicate->Error().find("found") == std::string::npos) {
        error = "Graph search self-test failed FindAll or multi-candidate diagnostics.";
        if (requiredDuplicate) requiredDuplicate->Release();
        duplicateQuery->Release();
        if (all) all->Release();
        containsQuery->Release();
        next->Release();
        nextLink->Release();
        sourceNode->Release();
        sourceQuery->Release();
        graph->Release();
        cleanup();
        return false;
    }

    requiredDuplicate->Release();
    duplicateQuery->Release();
    all->Release();
    containsQuery->Release();
    next->Release();
    nextLink->Release();
    sourceNode->Release();
    sourceQuery->Release();
    graph->Release();
    cleanup();
    return true;
}

static int CountGraphEditInputSources(CKBehavior *behavior) {
    int count = 0;
    for (int i = 0; behavior && i < behavior->GetLocalParameterCount(); ++i) {
        CKParameterLocal *local = behavior->GetLocalParameter(i);
        if (local && SafeString(local->GetName()).find("__CKAS_GraphEditInput_") == 0) {
            ++count;
        }
    }
    return count;
}

static CKObjectDeclaration *FindSelfTestPrototypeDeclarationByName(const char *name) {
    for (int i = 0; i < CKGetPrototypeDeclarationCount(); ++i) {
        CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
        if (decl && SafeString(decl->GetName()) == SafeString(const_cast<CKSTRING>(name))) {
            return decl;
        }
    }
    return nullptr;
}

static CKBehavior *CreateSelfTestPrototypeBehavior(CKContext *context,
                                                   const char *objectName,
                                                   const char *prototypeName,
                                                   std::string &error) {
    CKObjectDeclaration *decl = FindSelfTestPrototypeDeclarationByName(prototypeName);
    if (!decl) {
        error = fmt::format("Graph edit dynamic layout self-test could not find prototype '{}'.", prototypeName);
        return nullptr;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, const_cast<CKSTRING>(objectName), CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = fmt::format("Graph edit dynamic layout self-test failed to create behavior '{}'.", objectName);
        return nullptr;
    }

    behavior->UseFunction();
    const CKERROR err = behavior->InitFromGuid(decl->GetGuid());
    if (err != CK_OK) {
        error = fmt::format("Graph edit dynamic layout self-test failed to initialize '{}' from prototype '{}' (CKERROR {}).",
                            objectName,
                            prototypeName,
                            err);
        DestroySelfTestObject(context, behavior);
        return nullptr;
    }
    behavior->SetName(const_cast<CKSTRING>(objectName), TRUE);
    return behavior;
}

static bool RunBehaviorBridgeNativeGraphEditDynamicLayoutSelfTest(CKContext *context,
                                                                  std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "Graph edit dynamic layout self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *root = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphEditDynamicRoot", CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *switcher = CreateSelfTestPrototypeBehavior(context, "__CKAS_GraphEditDynamicSwitch", "Switch On Parameter", error);
    CKBehavior *selector = CreateSelfTestPrototypeBehavior(context, "__CKAS_GraphEditDynamicSelector", "Parameter Selector", error);
    if (!root || !switcher || !selector) {
        DestroySelfTestObject(context, selector);
        DestroySelfTestObject(context, switcher);
        DestroySelfTestObject(context, root);
        return false;
    }

    root->UseGraph();
    root->AddSubBehavior(switcher);
    root->AddSubBehavior(selector);
    bool switcherCreated = false;
    bool selectorCreated = false;
    CKERROR callbackErr = CallBridgeBehaviorCallback(switcher, CKM_BEHAVIORCREATE);
    if (callbackErr == CK_OK) {
        switcherCreated = true;
        callbackErr = CallBridgeBehaviorCallback(selector, CKM_BEHAVIORCREATE);
    }
    if (callbackErr != CK_OK) {
        error = fmt::format("Graph edit dynamic layout self-test CREATE callback failed with CKERROR {}.", callbackErr);
        if (switcherCreated) {
            CallBridgeBehaviorCallback(switcher, CKM_BEHAVIORDELETE);
        }
        root->RemoveSubBehavior(selector);
        root->RemoveSubBehavior(switcher);
        DestroySelfTestObject(context, selector);
        DestroySelfTestObject(context, switcher);
        DestroySelfTestObject(context, root);
        return false;
    }
    selectorCreated = true;

    const int targetOutputCount = std::max(switcher->GetOutputCount() + 1, 6);
    const int targetInputCount = std::max(selector->GetInputCount() + 1, 5);

    CKBehaviorContext ctx;
    ctx.Context = context;
    ctx.Behavior = root;
    ctx.ParameterManager = context->GetParameterManager();

    auto cleanup = [&]() {
        if (root) {
            while (root->GetSubBehaviorLinkCount() > 0) {
                CKBehaviorLink *link = root->GetSubBehaviorLink(0);
                root->RemoveSubBehaviorLink(link);
                DestroySelfTestObject(context, link);
            }
            if (switcher && !switcher->IsToBeDeleted()) root->RemoveSubBehavior(switcher);
            if (selector && !selector->IsToBeDeleted()) root->RemoveSubBehavior(selector);
        }
        if (selectorCreated && selector && !selector->IsToBeDeleted()) {
            CallBridgeBehaviorCallback(selector, CKM_BEHAVIORDELETE);
        }
        if (switcherCreated && switcher && !switcher->IsToBeDeleted()) {
            CallBridgeBehaviorCallback(switcher, CKM_BEHAVIORDELETE);
        }
        DestroySelfTestObject(context, selector);
        DestroySelfTestObject(context, switcher);
        DestroySelfTestObject(context, root);
    };

    BehaviorGraph *graph = new BehaviorGraph(bridge, ctx, root->GetID());
    BehaviorQuery *switchQuery = (new BehaviorQuery())->Name("__CKAS_GraphEditDynamicSwitch")->Recursive(false);
    BehaviorQuery *selectorQuery = (new BehaviorQuery())->Name("__CKAS_GraphEditDynamicSelector")->Recursive(false);
    BehaviorNode *switchNode = graph->Require(switchQuery);
    BehaviorNode *selectorNode = graph->Require(selectorQuery);
    switchQuery->Release();
    selectorQuery->Release();
    if (!switchNode || !switchNode->IsValid() || !selectorNode || !selectorNode->IsValid()) {
        error = "Graph edit dynamic layout self-test failed to resolve behavior nodes.";
        if (selectorNode) selectorNode->Release();
        if (switchNode) switchNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorGraphEdit *edit = graph->Edit();
    GraphEditNode *editSwitch = edit->Import(switchNode);
    GraphEditNode *editSelector = edit->Import(selectorNode);
    edit->EnsureOutputCount(editSwitch, targetOutputCount)->Release();
    edit->EnsureInputCount(editSelector, targetInputCount)->Release();
    GraphEditLink *plannedLink = edit->Link(editSwitch, targetOutputCount - 1, editSelector, targetInputCount - 1, 1);
    GraphEditResult *validation = edit->Validate(ctx);
    GraphEditResult *applied = edit->Apply(ctx);
    if (!plannedLink || !plannedLink->IsValid() || !validation || !validation->Ok() || !applied || !applied->Ok()) {
        error = fmt::format("Graph edit dynamic layout self-test failed apply: validation={} apply={}.",
                            validation && validation->Ok() ? "ok" : (validation ? validation->Error() : "<null>"),
                            applied && applied->Ok() ? "ok" : (applied ? applied->Error() : "<null>"));
        if (applied) applied->Release();
        if (validation) validation->Release();
        if (plannedLink) plannedLink->Release();
        editSelector->Release();
        editSwitch->Release();
        edit->Release();
        selectorNode->Release();
        switchNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    const bool switchOk = switcher->GetOutputCount() == targetOutputCount &&
        switcher->GetInputParameterCount() == targetOutputCount;
    const bool selectorOk = selector->GetInputCount() == targetInputCount &&
        selector->GetInputParameterCount() == targetInputCount;
    CKBehaviorLink *link = root->GetSubBehaviorLinkCount() == 1 ? root->GetSubBehaviorLink(0) : nullptr;
    CKBehaviorIO *sourceIo = link ? link->GetInBehaviorIO() : nullptr;
    CKBehaviorIO *targetIo = link ? link->GetOutBehaviorIO() : nullptr;
    const bool linkOk = link &&
        switcher->GetOutputPosition(sourceIo) == targetOutputCount - 1 &&
        selector->GetInputPosition(targetIo) == targetInputCount - 1;
    if (!switchOk || !selectorOk || !linkOk) {
        error = fmt::format("Graph edit dynamic layout self-test count mismatch: switch outputs={} pins={} selector inputs={} pins={} link={}.",
                            switcher->GetOutputCount(),
                            switcher->GetInputParameterCount(),
                            selector->GetInputCount(),
                            selector->GetInputParameterCount(),
                            linkOk ? "ok" : "bad");
        applied->Release();
        validation->Release();
        plannedLink->Release();
        editSelector->Release();
        editSwitch->Release();
        edit->Release();
        selectorNode->Release();
        switchNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    applied->Release();
    validation->Release();
    plannedLink->Release();
    editSelector->Release();
    editSwitch->Release();
    edit->Release();
    selectorNode->Release();
    switchNode->Release();
    graph->Release();
    cleanup();
    return true;
}

static bool RunBehaviorBridgeNativeGraphEditSelfTest(CKContext *context,
                                                     std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "Graph edit native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *root = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphEditRoot", CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *source = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphEditSource", CK_OBJECTCREATION_DYNAMIC));
    CKBehavior *target = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_GraphEditTarget", CK_OBJECTCREATION_DYNAMIC));
    if (!root || !source || !target) {
        DestroySelfTestObject(context, target);
        DestroySelfTestObject(context, source);
        DestroySelfTestObject(context, root);
        error = "Graph edit self-test failed to create graph objects.";
        return false;
    }

    root->UseGraph();
    source->CreateOutput("Out");
    target->CreateInput("In");
    CKParameterIn *targetValue = target->CreateInputParameter(const_cast<CKSTRING>("Value"), CKPGUID_INT);
    root->AddSubBehavior(source);
    root->AddSubBehavior(target);

    CKBehaviorContext ctx;
    ctx.Context = context;
    ctx.Behavior = root;
    ctx.ParameterManager = context->GetParameterManager();

    auto cleanup = [&]() {
        if (root && source) root->RemoveSubBehavior(source);
        if (root && target && !target->IsToBeDeleted()) root->RemoveSubBehavior(target);
        DestroySelfTestObject(context, target);
        DestroySelfTestObject(context, source);
        DestroySelfTestObject(context, root);
    };

    BehaviorGraph *graph = new BehaviorGraph(bridge, ctx, root->GetID());
    BehaviorQuery *sourceQuery = (new BehaviorQuery())->Name("__CKAS_GraphEditSource")->Recursive(false);
    BehaviorQuery *targetQuery = (new BehaviorQuery())->Name("__CKAS_GraphEditTarget")->Recursive(false);
    BehaviorNode *sourceNode = graph->Require(sourceQuery);
    BehaviorNode *targetNode = graph->Require(targetQuery);
    sourceQuery->Release();
    targetQuery->Release();
    if (!sourceNode || !sourceNode->IsValid() || !targetNode || !targetNode->IsValid()) {
        error = "Graph edit self-test failed to resolve graph nodes.";
        if (targetNode) targetNode->Release();
        if (sourceNode) sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorGraphEdit *edit = graph->Edit();
    GraphEditNode *editSource = edit->Import(sourceNode);
    GraphEditNode *editTarget = edit->Import(targetNode);
    GraphEditLink *pendingLink = edit->Link(editSource, 0, editTarget, 0, 3);
    GraphEditResult *validation = edit->Validate(ctx);
    GraphEditResult *applied = edit->Apply(ctx);
    if (!pendingLink || !pendingLink->IsValid() || !validation || !validation->Ok() || !applied || !applied->Ok() ||
        root->GetSubBehaviorLinkCount() != 1) {
        error = "Graph edit self-test failed link apply.";
        if (applied) applied->Release();
        if (validation) validation->Release();
        if (pendingLink) pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorLinkRef *createdLink = sourceNode->NextLink(nullptr);
    if (!createdLink || !createdLink->IsValid() || createdLink->SourceOutputIndex() != 0 ||
        createdLink->TargetInputIndex() != 0 || createdLink->Delay() != 3) {
        error = "Graph edit self-test failed created link inspection.";
        if (createdLink) createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    BehaviorGraphEdit *unlinkEdit = graph->Edit();
    unlinkEdit->Unlink(createdLink)->Release();
    GraphEditResult *unlinkResult = unlinkEdit->Apply(ctx);
    if (!unlinkResult || !unlinkResult->Ok() || root->GetSubBehaviorLinkCount() != 0) {
        error = "Graph edit self-test failed unlink apply.";
        if (unlinkResult) unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    GraphEditResult *secondApply = unlinkEdit->Apply(ctx);
    if (!secondApply || secondApply->Ok() || secondApply->Error().find("already applied") == std::string::npos) {
        error = "Graph edit self-test failed to reject repeated Apply.";
        if (secondApply) secondApply->Release();
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }
    secondApply->Release();

    target->UseGraph();
    CKBehaviorContext targetGraphCtx = ctx;
    targetGraphCtx.Behavior = target;
    BehaviorGraph *targetGraph = new BehaviorGraph(bridge, targetGraphCtx, target->GetID());
    BehaviorGraphEdit *selfMoveEdit = graph->Edit();
    selfMoveEdit->Move(targetNode, targetGraph)->Release();
    GraphEditResult *selfMoveValidation = selfMoveEdit->Validate(ctx);
    if (!selfMoveValidation || selfMoveValidation->Ok() ||
        selfMoveValidation->Error().find("itself") == std::string::npos) {
        error = fmt::format("Graph edit self-test failed self-move validation: {}.",
                            selfMoveValidation ? selfMoveValidation->Error() : std::string("<null>"));
        if (selfMoveValidation) selfMoveValidation->Release();
        selfMoveEdit->Release();
        targetGraph->Release();
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }
    selfMoveValidation->Release();
    selfMoveEdit->Release();
    targetGraph->Release();

    CKParameterLocal *internalGraphEditTarget = target->CreateLocalParameter(
        const_cast<CKSTRING>("__CKAS_GraphEdit_Target"),
        CKPGUID_INT);
    const ScriptBridgeLayoutRecord *internalFilteredLayout = bridge->GetBehaviorLayout(target->GetID(), CaptureBridgeObjectStamp(target));
    bool exposedInternalLocal = false;
    if (internalFilteredLayout) {
        for (const ScriptBridgeLayoutParamSlot &local : internalFilteredLayout->Locals) {
            if (local.Name == "__CKAS_GraphEdit_Target") {
                exposedInternalLocal = true;
                break;
            }
        }
    }
    if (!internalGraphEditTarget || exposedInternalLocal) {
        error = "Graph edit self-test exposed internal graph edit target local.";
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }

    const ScriptBridgeLayoutRecord *targetLayout = bridge->GetBehaviorLayout(target->GetID(), CaptureBridgeObjectStamp(target));
    BBSlot *valueSlot = targetLayout
        ? new BBSlot(bridge,
                     ctx,
                     MakeDefaultRequest(ctx),
                     ScriptBridgeSlotKind::Pin,
                     0,
                     "Value",
                     CKPGUID_INT,
                     "Integer",
                     sizeof(int),
                     0,
                     targetLayout->LayoutGeneration,
                     targetLayout->Signature,
                     std::string(),
                     target->GetID(),
                     CaptureBridgeObjectStamp(target))
        : nullptr;
    BehaviorGraphEdit *setEdit = graph->Edit();
    GraphEditNode *setTarget = setEdit->Import(targetNode);
    setEdit->SetSlotInt(setTarget, valueSlot, 11)->Release();
    GraphEditResult *setResult = setEdit->Apply(ctx);
    BehaviorGraphEdit *replaceSetEdit = graph->Edit();
    GraphEditNode *replaceSetTarget = replaceSetEdit->Import(targetNode);
    replaceSetEdit->SetSlotInt(replaceSetTarget, valueSlot, 12)->Release();
    GraphEditResult *replaceSetResult = replaceSetEdit->Apply(ctx);
    CKParameter *directSource = targetValue ? targetValue->GetDirectSource() : nullptr;
    if (!targetValue || !valueSlot || !setResult || !setResult->Ok() ||
        !replaceSetResult || !replaceSetResult->Ok() ||
        !directSource ||
        CountGraphEditInputSources(target) != 1) {
        error = fmt::format("Graph edit self-test failed Set replacement cleanup: first={} second={} direct={} bridgeSources={}.",
                            setResult && setResult->Ok() ? "ok" : (setResult ? setResult->Error() : "<null>"),
                            replaceSetResult && replaceSetResult->Ok() ? "ok" : (replaceSetResult ? replaceSetResult->Error() : "<null>"),
                            directSource ? SafeString(directSource->GetName()) : std::string("<null>"),
                            CountGraphEditInputSources(target));
        if (replaceSetResult) replaceSetResult->Release();
        replaceSetTarget->Release();
        replaceSetEdit->Release();
        if (setResult) setResult->Release();
        setTarget->Release();
        setEdit->Release();
        if (valueSlot) valueSlot->Release();
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }
    replaceSetResult->Release();
    replaceSetTarget->Release();
    replaceSetEdit->Release();
    setResult->Release();
    setTarget->Release();
    setEdit->Release();

    CKParameterLocal *graphEditReplacementSource = context->CreateCKParameterLocal(
        const_cast<CKSTRING>("__CKAS_GraphEditReplacementSource"),
        CKPGUID_INT,
        TRUE);
    ParamRef *graphEditReplacementRef = graphEditReplacementSource
        ? new ParamRef(bridge, graphEditReplacementSource->GetID(), ScriptBridgeSlotKind::Standalone, -1)
        : nullptr;
    const CKGUID graphEditOperationGuid = FindSelfTestOperationGuid(context, context->GetParameterManager(), CKPGUID_INT);
    ScriptBridgeOperationSpec graphEditOperationRequest;
    graphEditOperationRequest.TargetPinIndex = 0;
    graphEditOperationRequest.OperationGuid = graphEditOperationGuid;
    graphEditOperationRequest.ResultTypeGuid = CKPGUID_INT;
    graphEditOperationRequest.In1.Kind = ScriptBridgeInputBindingKind::Value;
    graphEditOperationRequest.In1.Value = MakeScriptParamInt(1);
    graphEditOperationRequest.In2.Kind = ScriptBridgeInputBindingKind::Value;
    graphEditOperationRequest.In2.Value = MakeScriptParamInt(2);
    std::string graphEditOperationError;
    ParamOperationRef *graphEditOperation = graphEditOperationGuid.IsValid()
        ? ConnectOperationToInput(bridge, target, 0, graphEditOperationRequest, graphEditOperationError, true, nullptr)
        : nullptr;
    CKParameterOperation *graphEditOperationObject = graphEditOperation ? graphEditOperation->Get() : nullptr;
    BehaviorGraphEdit *replaceOperationSourceEdit = graph->Edit();
    GraphEditNode *replaceOperationSourceTarget = replaceOperationSourceEdit->Import(targetNode);
    if (graphEditReplacementRef) {
        replaceOperationSourceEdit->Source(replaceOperationSourceTarget, valueSlot, graphEditReplacementRef)->Release();
    }
    GraphEditResult *replaceOperationSourceResult = replaceOperationSourceEdit->Apply(ctx);
    if (!graphEditReplacementSource ||
        !graphEditReplacementRef ||
        !graphEditOperation ||
        !graphEditOperationObject ||
        !replaceOperationSourceResult ||
        !replaceOperationSourceResult->Ok() ||
        targetValue->GetDirectSource() != graphEditReplacementSource ||
        !graphEditOperationObject->IsToBeDeleted()) {
        error = fmt::format("Graph edit self-test failed operation replacement cleanup: op={} apply={} source={} deleted={}.",
                            graphEditOperation ? (graphEditOperationObject ? "ok" : "missing") : graphEditOperationError,
                            replaceOperationSourceResult && replaceOperationSourceResult->Ok() ? "ok" : (replaceOperationSourceResult ? replaceOperationSourceResult->Error() : "<null>"),
                            targetValue && targetValue->GetDirectSource() == graphEditReplacementSource ? "ok" : "wrong",
                            graphEditOperationObject && graphEditOperationObject->IsToBeDeleted() ? "true" : "false");
        if (replaceOperationSourceResult) replaceOperationSourceResult->Release();
        replaceOperationSourceTarget->Release();
        replaceOperationSourceEdit->Release();
        if (graphEditOperation) graphEditOperation->Release();
        if (graphEditReplacementRef) graphEditReplacementRef->Release();
        DestroySelfTestObject(context, graphEditReplacementSource);
        if (valueSlot) valueSlot->Release();
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }
    replaceOperationSourceResult->Release();
    replaceOperationSourceTarget->Release();
    replaceOperationSourceEdit->Release();
    graphEditOperation->Release();
    graphEditReplacementRef->Release();
    DestroySelfTestObject(context, graphEditReplacementSource);

    valueSlot->Release();

    BehaviorGraphEdit *removeEdit = graph->Edit();
    removeEdit->Remove(targetNode, true)->Release();
    GraphEditResult *removeResult = removeEdit->Apply(ctx);
    if (!removeResult || !removeResult->Ok() || targetNode->IsValid()) {
        error = "Graph edit self-test failed remove apply.";
        if (removeResult) removeResult->Release();
        removeEdit->Release();
        unlinkResult->Release();
        unlinkEdit->Release();
        createdLink->Release();
        applied->Release();
        validation->Release();
        pendingLink->Release();
        editTarget->Release();
        editSource->Release();
        edit->Release();
        targetNode->Release();
        sourceNode->Release();
        graph->Release();
        cleanup();
        return false;
    }
    target = nullptr;

    removeResult->Release();
    removeEdit->Release();
    unlinkResult->Release();
    unlinkEdit->Release();
    createdLink->Release();
    applied->Release();
    validation->Release();
    pendingLink->Release();
    editTarget->Release();
    editSource->Release();
    edit->Release();
    targetNode->Release();
    sourceNode->Release();
    graph->Release();
    cleanup();
    return true;
}

static bool RunInitialSettingsEditedSelfTest(CKContext *context,
                                             ScriptBehaviorBridge *bridge,
                                             const CKBehaviorContext &behaviorContext,
                                             std::string &error) {
    const CKGUID genericOrbitGuid(0x104d0913, 0x766563dd);
    bool hasGenericOrbit = CKGetPrototypeFromGuid(genericOrbitGuid) != nullptr;
    if (!hasGenericOrbit) {
        for (int i = 0; i < CKGetPrototypeDeclarationCount(); ++i) {
            CKObjectDeclaration *decl = CKGetPrototypeDeclaration(i);
            if (decl && decl->GetGuid() == genericOrbitGuid) {
                hasGenericOrbit = true;
                break;
            }
        }
    }
    if (!hasGenericOrbit) {
        return true;
    }

    CKBeObject *target = CKBeObject::Cast(context->CreateObject(CKCID_3DENTITY,
                                                                 "__CKAS_SettingsEditedTarget",
                                                                 CK_OBJECTCREATION_DYNAMIC));
    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(behaviorContext);
    request.PrototypeKind = ScriptBridgePrototypeKind::Guid;
    request.Guid = genericOrbitGuid;
    request.TargetId = target ? target->GetID() : 0;

    BBConfig *config = new BBConfig(bridge, behaviorContext, request);
    BBSlot *returnsSetting = config ? config->Setting("Returns") : nullptr;
    BBSlot *returnSpeedPin = config ? config->Pin("Return Speed") : nullptr;
    ParamValue *falseValue = new ParamValue(MakeScriptParamBool(false));
    BBInstance *instance = nullptr;

    auto cleanup = [&]() {
        if (instance) {
            instance->Destroy();
            instance->Release();
            instance = nullptr;
        }
        if (falseValue) {
            falseValue->Release();
            falseValue = nullptr;
        }
        if (returnSpeedPin) {
            returnSpeedPin->Release();
            returnSpeedPin = nullptr;
        }
        if (returnsSetting) {
            returnsSetting->Release();
            returnsSetting = nullptr;
        }
        if (config) {
            config->Release();
            config = nullptr;
        }
        DestroySelfTestObject(context, target);
    };

    int returnSpeedIndex = -1;
    std::string slotError;
    if (!target ||
        !config ||
        !returnsSetting ||
        !returnSpeedPin ||
        !returnSpeedPin->ResolveIndex(ScriptBridgeSlotKind::Pin, returnSpeedIndex, slotError)) {
        cleanup();
        error = slotError.empty() ? "Initial setting edit self-test setup failed." : slotError;
        return false;
    }

    BBConfig *returnedConfig = config->SetSetting(returnsSetting, falseValue);
    const bool settingAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    instance = settingAccepted ? config->SpawnInstance(behaviorContext) : nullptr;
    CKBehavior *behavior = instance ? bridge->GetInstanceBehavior(instance->BridgeInstanceId(), instance->BridgeGeneration()) : nullptr;
    const bool returnSpeedEnabled = behavior && returnSpeedIndex >= 0 && returnSpeedIndex < behavior->GetInputParameterCount()
        ? behavior->IsInputParameterEnabled(returnSpeedIndex) != FALSE
        : true;

    if (!settingAccepted || !instance || !behavior || returnSpeedEnabled) {
        const std::string configError = instance ? instance->Error() : (config ? config->Error() : std::string());
        cleanup();
        error = fmt::format("Initial setting edit self-test failed: setting={} instance={} behavior={} returnSpeedEnabled={}{}{}",
                            settingAccepted ? "ok" : "failed",
                            instance ? "ok" : "null",
                            behavior ? "ok" : "null",
                            returnSpeedEnabled ? "true" : "false",
                            configError.empty() ? "" : " error=",
                            configError);
        return false;
    }

    cleanup();
    error.clear();
    return true;
}

static bool RunBehaviorBridgeNativeLayoutCacheSelfTest(CKContext *context,
                                                       std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "Layout cache native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(
        CKCID_BEHAVIOR,
        const_cast<CKSTRING>("__CKAS_LayoutCacheSelfTest"),
        CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = "Failed to create behavior for layout cache self-test.";
        return false;
    }

    behavior->CreateInput(const_cast<CKSTRING>("In"));
    behavior->CreateOutput(const_cast<CKSTRING>("Out"));
    behavior->CreateInputParameter(const_cast<CKSTRING>("Value"), CKPGUID_INT);
    behavior->CreateOutputParameter(const_cast<CKSTRING>("Result"), CKPGUID_FLOAT);
    behavior->CreateLocalParameter(const_cast<CKSTRING>("State"), CKPGUID_BOOL);
    bridge->InvalidateBehaviorLayout(behavior->GetID());

    const ScriptBridgeObjectStamp stamp = CaptureBridgeObjectStamp(behavior);
    const ScriptBridgeLayoutRecord *layout = bridge->GetBehaviorLayout(behavior->GetID(), stamp);
    if (!layout ||
        layout->Inputs.size() != 1 ||
        layout->Outputs.size() != 1 ||
        layout->Pins.size() != 1 ||
        layout->Pouts.size() != 1 ||
        layout->Locals.size() != 1 ||
        layout->Pins[0].Name != "Value" ||
        layout->Pouts[0].TypeGuid != CKPGUID_FLOAT) {
        DestroySelfTestObject(context, behavior);
        error = "Layout cache initial record self-test failed.";
        return false;
    }

    const std::string firstSignature = layout->Signature;
    WriteBehaviorBridgeSelfTestMarker("running", "layout-cache", "layout-handle", std::string());
    BehaviorLayout layoutHandle(bridge, behavior->GetID());
    ParamInfo *pinInfo = layoutHandle.Pin(0);
    if (layoutHandle.InputCount() != 1 ||
        layoutHandle.OutputCount() != 1 ||
        layoutHandle.PinCount() != 1 ||
        layoutHandle.PoutCount() != 1 ||
        layoutHandle.LocalCount() != 1 ||
        layoutHandle.FindPin("Value", 0) != 0 ||
        layoutHandle.FindPout("Result", 0) != 0 ||
        !pinInfo ||
        pinInfo->GetTypeGuid() != CKPGUID_INT) {
        if (pinInfo) {
            pinInfo->Release();
        }
        DestroySelfTestObject(context, behavior);
        error = "BehaviorLayout cache-backed query self-test failed.";
        return false;
    }
    pinInfo->Release();

    behavior->CreateInputParameter(const_cast<CKSTRING>("Second"), CKPGUID_STRING);
    bridge->InvalidateBehaviorLayout(behavior->GetID());
    WriteBehaviorBridgeSelfTestMarker("running", "layout-cache", "layout-refresh", std::string());
    const ScriptBridgeLayoutRecord *refreshed = bridge->GetBehaviorLayout(behavior->GetID(), stamp);
    if (!refreshed ||
        refreshed->Pins.size() != 2 ||
        refreshed->Pins[1].Name != "Second" ||
        refreshed->Signature == firstSignature) {
        DestroySelfTestObject(context, behavior);
        error = "Layout cache refresh self-test failed.";
        return false;
    }

    behavior->CreateInput(const_cast<CKSTRING>("In"));
    bridge->InvalidateBehaviorLayout(behavior->GetID());
    WriteBehaviorBridgeSelfTestMarker("running", "layout-cache", "layout-duplicate", std::string());
    const ScriptBridgeLayoutRecord *duplicateInputLayout = bridge->GetBehaviorLayout(behavior->GetID(), stamp);
    CKBehaviorContext emptyBehaviorContext = {};
    ScriptBridgeBBInvocationSpec emptyRequest = MakeDefaultRequest(emptyBehaviorContext);
    BBSlot *duplicateInput = duplicateInputLayout
        ? new BBSlot(bridge,
                     emptyBehaviorContext,
                     emptyRequest,
                     ScriptBridgeSlotKind::Input,
                     1,
                     "In",
                     CKGUID(),
                     std::string(),
                     0,
                     0,
                     duplicateInputLayout->LayoutGeneration,
                     duplicateInputLayout->Signature,
                     std::string(),
                     behavior->GetID(),
                     stamp)
        : nullptr;
    const bool duplicateInputValid = duplicateInput && duplicateInput->IsValid();
    if (!duplicateInputValid) {
        if (duplicateInput) {
            duplicateInput->Release();
        }
        DestroySelfTestObject(context, behavior);
        error = "Layout cache duplicate input slot setup self-test failed.";
        return false;
    }

    CKDWORD defaultInputFlags = 0;
    SetScriptBridgeSlotMetadataFlag(defaultInputFlags, ScriptBridgeSlotMetadataFlags::Start, true);
    SetScriptBridgeSlotMetadataFlag(defaultInputFlags, ScriptBridgeSlotMetadataFlags::Stop, true);
    duplicateInput->SetMetadata(defaultInputFlags, std::string(), std::string());

    BBConfig defaultInputConfig(bridge, emptyBehaviorContext, emptyRequest);
    WriteBehaviorBridgeSelfTestMarker("running", "layout-cache", "layout-register", std::string());
    if (!defaultInputConfig.RegisterSlot(duplicateInput)) {
        error = "BBConfig duplicate input metadata self-test failed to register slot: " + defaultInputConfig.Error();
        duplicateInput->Release();
        DestroySelfTestObject(context, behavior);
        return false;
    }
    const std::string defaultInputText = defaultInputConfig.Explain();
    if (defaultInputText.find("Default start input: 'In[1]'") == std::string::npos ||
        defaultInputText.find("Default stop input: 'In[1]'") == std::string::npos) {
        error = "BBConfig duplicate input metadata self-test lost start/stop occurrence.";
        duplicateInput->Release();
        DestroySelfTestObject(context, behavior);
        return false;
    }
    duplicateInput->Release();

    WriteBehaviorBridgeSelfTestMarker("running", "layout-cache", "layout-destroy", std::string());
    DestroySelfTestObject(context, behavior);
    return true;
}

static bool RunBehaviorBridgeNativePrototypeDiscoverySelfTest(CKContext *context,
                                                              std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !bridge) {
        error = "Prototype discovery native self-test requires CKContext and ScriptBehaviorBridge.";
        return false;
    }

    const int count = CKGetPrototypeDeclarationCount();
    if (count <= 0) {
        error = "Prototype discovery count self-test failed.";
        return false;
    }

    CKObjectDeclaration *first = CKGetPrototypeDeclaration(0);
    if (!first || !first->GetGuid().IsValid()) {
        error = "Prototype discovery At(0) self-test failed.";
        return false;
    }

    return true;
}

static bool RunBehaviorBridgeNativeInternalShapeSelfTest(asIScriptEngine *engine, std::string &error) {
    if (engine) {
        if (!engine->GetTypeInfoByName("BBDecl") || !engine->GetTypeInfoByName("BBConfig") ||
            !engine->GetTypeInfoByName("BBInstance")) {
            error = "Bridge v3 public type registration self-test failed.";
            return false;
        }
        if (engine->GetTypeInfoByName("BBSpec") || engine->GetTypeInfoByName("BBBinding")) {
            error = "Bridge v3 clean-break self-test found legacy BBSpec/BBBinding types.";
            return false;
        }
    }

    ScriptBridgeInputSourceBindings bindings;
    bindings.Set(4, 44);
    bindings.Set(1, 11);
    bindings.Set(4, 45);
    if (bindings.Items.size() != 2 ||
        bindings.Items[0].PinIndex != 1 ||
        bindings.Items[1].PinIndex != 4 ||
        bindings.Find(1) != 11 ||
        bindings.Find(4) != 45) {
        error = "Sorted input source bindings self-test failed.";
        return false;
    }
    bindings.Remove(1);
    bindings.Remove(99);
    if (bindings.Find(1) != 0 ||
        bindings.Find(4) != 45 ||
        bindings.Items.size() != 1 ||
        bindings.Items[0].PinIndex != 4) {
        error = "Sorted input source binding removal self-test failed.";
        return false;
    }

    std::vector<ScriptBridgeIndexedValue> indexedValues;
    ScriptBridgeSetIndexedValue(indexedValues, 3, MakeScriptParamInt(30));
    ScriptBridgeSetIndexedValue(indexedValues, 0, MakeScriptParamInt(10));
    ScriptBridgeSetIndexedValue(indexedValues, 3, MakeScriptParamInt(31));
    if (indexedValues.size() != 2 ||
        indexedValues[0].PinIndex != 0 ||
        indexedValues[0].Value.Data.IntValue != 10 ||
        indexedValues[1].PinIndex != 3 ||
        indexedValues[1].Value.Data.IntValue != 31) {
        error = "Sorted indexed value overwrite self-test failed.";
        return false;
    }

    ParamValue textValue(MakeScriptParamString("not an int"));
    if (textValue.AsInt() != 0) {
        error = "ParamValue wrong-kind AsInt fallback self-test failed.";
        return false;
    }

    ParamValue intValue(MakeScriptParamInt(3));
    if (intValue.AsFloat() != 3.0f) {
        error = "ParamValue int-to-float safe conversion self-test failed.";
        return false;
    }

    ParamValue typedText(MakeScriptParamText("1", CKPGUID_BOOL, "Boolean"));
    if (typedText.TypeName() != "Boolean") {
        error = "Typed ParamValue text TypeName self-test failed.";
        return false;
    }

    ParamValue typedEnum(MakeScriptParamEnum(CKPGUID_INT, "Test Enum", 2));
    if (typedEnum.TypeName() != "Test Enum" || typedEnum.AsInt() != 2) {
        error = "Typed ParamValue enum TypeName self-test failed.";
        return false;
    }

    VxVector rawVector(1.0f, 2.0f, 3.0f);
    ParamValue typedRaw(MakeScriptParamRaw(CKPGUID_VECTOR, "Vector", &rawVector, sizeof(rawVector)));
    if (typedRaw.TypeName() != "Vector") {
        error = "Typed ParamValue raw TypeName self-test failed.";
        return false;
    }

    return true;
}

static bool RunBehaviorBridgeNativeBBInvocationSelfTest(CKContext *context,
                                                     std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !manager || !bridge) {
        error = "BB invocation self-test requires CKContext, ScriptManager, and ScriptBehaviorBridge.";
        return false;
    }

    CKBehaviorContext behaviorContext;
    behaviorContext.Behavior = nullptr;
    behaviorContext.DeltaTime = 0.016f;
    behaviorContext.Context = context;
    behaviorContext.CurrentLevel = nullptr;
    behaviorContext.CurrentScene = nullptr;
    behaviorContext.PreviousScene = nullptr;
    behaviorContext.CurrentRenderContext = nullptr;
    behaviorContext.ParameterManager = context->GetParameterManager();
    behaviorContext.MessageManager = nullptr;
    behaviorContext.AttributeManager = nullptr;
    behaviorContext.TimeManager = nullptr;
    behaviorContext.CallbackMessage = 0;
    behaviorContext.CallbackArg = nullptr;

    if (!RunInitialSettingsEditedSelfTest(context, bridge, behaviorContext, error)) {
        return false;
    }

    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(behaviorContext);
    request.PrototypeName = "Logics/Calculator/Identity";
    ScriptBridgeSetIndexedValue(request.IndexedParameters, 0, MakeScriptParamInt(7));

    BBResult *result = bridge->RunCall(request, behaviorContext, 0);
    if (!result) {
        error = "BB.Call Identity self-test did not return a result.";
        return false;
    }

    const bool callStateOk = result->Ok();
    const int callReturnCode = result->ReturnCode();
    const bool callOutActive = result->OutputActive(0);
    const bool callOk = callStateOk && callReturnCode == CKBR_OK && callOutActive;
    std::string callError = result->Error();
    result->Release();
    if (!callOk) {
        error = fmt::format("BB.Call Identity self-test failed: ok={} rc={} out={} error={}",
                            callStateOk ? "true" : "false",
                            callReturnCode,
                            callOutActive ? "true" : "false",
                            callError.empty() ? "<empty>" : callError);
        return false;
    }

    BBConfig *replacementConfig = new BBConfig(bridge, behaviorContext, request);
    BBSlot *replacementPin = replacementConfig ? replacementConfig->Pin("pIn 0") : nullptr;
    CKParameterLocal *wrongSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_WrongIdentitySource"), CKPGUID_STRING, TRUE);
    ParamRef *wrongSourceRef = wrongSource ? new ParamRef(bridge, wrongSource->GetID(), ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    auto cleanupReplacement = [&]() {
        if (wrongSourceRef) {
            wrongSourceRef->Release();
            wrongSourceRef = nullptr;
        }
        if (replacementPin) {
            replacementPin->Release();
            replacementPin = nullptr;
        }
        if (replacementConfig) {
            replacementConfig->Release();
            replacementConfig = nullptr;
        }
        DestroySelfTestObject(context, wrongSource);
        wrongSource = nullptr;
    };
    if (!replacementConfig || !replacementConfig->IsValid() || !replacementPin || !replacementPin->IsValid() || !wrongSourceRef) {
        error = replacementConfig ? replacementConfig->Error() : "BBConfig replacement precedence self-test setup failed.";
        cleanupReplacement();
        return false;
    }
    BBConfig *returnedConfig = replacementConfig->SourceSlot(replacementPin, wrongSourceRef);
    const bool wrongSourceAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    returnedConfig = replacementConfig->SetSlotInt(replacementPin, 12);
    const bool valueAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    BBInstance *replacementInstance = valueAccepted ? replacementConfig->SpawnStarted(behaviorContext) : nullptr;
    const bool replacementOk = !wrongSourceAccepted && replacementInstance && replacementInstance->IsValid();
    const std::string replacementError = replacementInstance ? replacementInstance->Error() : replacementConfig->Error();
    if (replacementInstance) {
        replacementInstance->Destroy();
        replacementInstance->Release();
    }
    cleanupReplacement();
    if (!replacementOk) {
        error = fmt::format("BBConfig Identity replacement precedence self-test failed: wrongSourceAccepted={} valueAccepted={} error={}",
                            wrongSourceAccepted ? "true" : "false",
                            valueAccepted ? "true" : "false",
                            replacementError.empty() ? "<empty>" : replacementError);
        return false;
    }

    CKParameterLocal *configInitialSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_ConfigInitialSource"), CKPGUID_FLOAT, TRUE);
    CKParameterLocal *configLiveSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_ConfigLiveSource"), CKPGUID_FLOAT, TRUE);
    CKParameterLocal *configBadSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_ConfigBadSource"), CKPGUID_STRING, TRUE);
    BBConfig *liveConfig = new BBConfig(bridge, behaviorContext, request);
    BBSlot *liveConfigPin = liveConfig ? liveConfig->Pin("pIn 0") : nullptr;
    ParamRef *configInitialRef = configInitialSource ? new ParamRef(bridge, configInitialSource->GetID(), ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    ParamRef *configLiveRef = configLiveSource ? new ParamRef(bridge, configLiveSource->GetID(), ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    ParamRef *configBadRef = configBadSource ? new ParamRef(bridge, configBadSource->GetID(), ScriptBridgeSlotKind::Standalone, -1) : nullptr;
    BBInstance *liveConfigInstance = nullptr;
    auto cleanupLiveConfig = [&]() {
        if (liveConfigInstance) {
            liveConfigInstance->Destroy();
            liveConfigInstance->Release();
            liveConfigInstance = nullptr;
        }
        if (liveConfigPin) {
            liveConfigPin->Release();
            liveConfigPin = nullptr;
        }
        if (configInitialRef) {
            configInitialRef->Release();
            configInitialRef = nullptr;
        }
        if (configLiveRef) {
            configLiveRef->Release();
            configLiveRef = nullptr;
        }
        if (configBadRef) {
            configBadRef->Release();
            configBadRef = nullptr;
        }
        if (liveConfig) {
            liveConfig->Release();
            liveConfig = nullptr;
        }
        DestroySelfTestObject(context, configBadSource);
        DestroySelfTestObject(context, configLiveSource);
        DestroySelfTestObject(context, configInitialSource);
    };
    if (!configInitialSource || !configLiveSource || !configBadSource || !liveConfig || !liveConfigPin || !configInitialRef || !configLiveRef || !configBadRef) {
        cleanupLiveConfig();
        error = "BBConfig live source replacement self-test setup failed.";
        return false;
    }
    returnedConfig = liveConfig->SourceSlot(liveConfigPin, configInitialRef);
    const bool initialSourceAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    liveConfigInstance = initialSourceAccepted ? liveConfig->SpawnInstance(behaviorContext) : nullptr;
    CKBehavior *liveConfigBehavior = liveConfigInstance ? bridge->GetInstanceBehavior(liveConfigInstance->BridgeInstanceId(), liveConfigInstance->BridgeGeneration()) : nullptr;
    CKParameterIn *liveConfigInput = liveConfigBehavior && liveConfigPin->Index() >= 0 && liveConfigPin->Index() < liveConfigBehavior->GetInputParameterCount()
        ? liveConfigBehavior->GetInputParameter(liveConfigPin->Index())
        : nullptr;
    if (!liveConfigInstance || !liveConfigBehavior || !liveConfigInput || liveConfigInput->GetDirectSource() != configInitialSource) {
        const std::string configError = liveConfigInstance ? liveConfigInstance->Error() : liveConfig->Error();
        cleanupLiveConfig();
        error = configError.empty() ? "BBConfig live source replacement self-test failed to install initial source." : configError;
        return false;
    }
    returnedConfig = liveConfig->SourceSlot(liveConfigPin, configLiveRef);
    const bool liveSourceAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    if (!liveSourceAccepted || liveConfigInput->GetDirectSource() != configLiveSource) {
        cleanupLiveConfig();
        error = "BBConfig live source replacement self-test failed to install live source.";
        return false;
    }
    returnedConfig = liveConfig->SourceSlot(liveConfigPin, configBadRef);
    const bool badConfigSourceAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    if (badConfigSourceAccepted || liveConfigInput->GetDirectSource() != configLiveSource) {
        cleanupLiveConfig();
        error = "BBConfig Identity self-test did not preserve live source after failed source replacement.";
        return false;
    }
    returnedConfig = liveConfig->SetSlotObject(liveConfigPin, liveConfigBehavior);
    const bool badConfigValueAccepted = returnedConfig != nullptr;
    if (returnedConfig) returnedConfig->Release();
    if (badConfigValueAccepted || liveConfigInput->GetDirectSource() != configLiveSource) {
        cleanupLiveConfig();
        error = "BBConfig Identity self-test did not preserve live source after failed value replacement.";
        return false;
    }
    cleanupLiveConfig();

    if (!RunLiveOperationReplacementSelfTest(context, bridge, behaviorContext, true, error)) {
        return false;
    }
    if (!RunLiveOperationReplacementSelfTest(context, bridge, behaviorContext, false, error)) {
        return false;
    }
    if (!RunLiveMixedLinkReplacementSelfTest(context, bridge, behaviorContext, true, error)) {
        return false;
    }
    if (!RunLiveMixedLinkReplacementSelfTest(context, bridge, behaviorContext, false, error)) {
        return false;
    }

    BBTask *task = bridge->StartTask(request, behaviorContext, 0);
    if (!task) {
        error = "BB.Spawn Identity self-test did not return a task.";
        return false;
    }
    const bool taskStepOk = task->Step(behaviorContext, 0);
    const int taskStepReturnCode = task->ReturnCode();
    const bool taskStepOutActive = task->OutputActive(0);
    if (!taskStepOk || taskStepReturnCode != CKBR_OK || !taskStepOutActive) {
        std::string taskError = task->Error();
        task->Destroy();
        task->Release();
        error = fmt::format("BB.Spawn Identity self-test Step failed: ok={} rc={} out={} error={}",
                            taskStepOk ? "true" : "false",
                            taskStepReturnCode,
                            taskStepOutActive ? "true" : "false",
                            taskError.empty() ? "<empty>" : taskError);
        return false;
    }
    const bool taskIdleStepOk = task->Step(behaviorContext, -1);
    const bool taskIdleOutActive = task->OutputActive(0);
    if (!taskIdleStepOk || taskIdleOutActive) {
        std::string taskError = task->Error();
        task->Destroy();
        task->Release();
        error = fmt::format("BB.Spawn Identity self-test kept output active after inactive idle Step: ok={} out={} error={}",
                            taskIdleStepOk ? "true" : "false",
                            taskIdleOutActive ? "true" : "false",
                            taskError.empty() ? "<empty>" : taskError);
        return false;
    }
    if (!task->Destroy() || task->IsValid()) {
        task->Release();
        error = "BB.Spawn Identity self-test task did not destroy cleanly.";
        return false;
    }
    task->Release();

    int instanceGeneration = 0;
    std::string instanceError;
    CK_ID instanceId = bridge->CreateInstance(request, behaviorContext, instanceGeneration, instanceError);
    CKBehavior *instanceBehavior = instanceId ? bridge->GetInstanceBehavior(instanceId, instanceGeneration) : nullptr;
    BBInstance liveInstance(bridge, behaviorContext, request, instanceId, instanceGeneration, std::string(), 0, std::string(), 0);
    BBSlot *livePin = instanceId ? liveInstance.PinSlot("pIn 0") : nullptr;
    CKParameterIn *valuePin = instanceBehavior && livePin && livePin->Index() >= 0 && livePin->Index() < instanceBehavior->GetInputParameterCount()
        ? instanceBehavior->GetInputParameter(livePin->Index())
        : nullptr;
    CKParameter *previousSource = valuePin ? valuePin->GetDirectSource() : nullptr;
    CKParameterLocal *liveSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_InstanceLiveSource"), CKPGUID_FLOAT, TRUE);
    CKParameterLocal *badSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_InstanceBadSource"), CKPGUID_STRING, TRUE);
    if (!instanceId || !instanceBehavior || !livePin || !valuePin || !previousSource || !liveSource || !badSource) {
        if (livePin) livePin->Release();
        if (instanceId) {
            bridge->DestroyInstance(instanceId, instanceGeneration);
        }
        DestroySelfTestObject(context, badSource);
        DestroySelfTestObject(context, liveSource);
        error = instanceError.empty() ? "BBInstance Identity self-test setup failed." : instanceError;
        return false;
    }

    ParamRef *targetRef = new ParamRef(bridge, valuePin->GetID(), ScriptBridgeSlotKind::Pin, livePin->Index(), instanceBehavior->GetID());
    ParamRef *sourceRef = new ParamRef(bridge, liveSource->GetID(), ScriptBridgeSlotKind::Standalone, -1);
    ParamSourceLinkRef *link = targetRef->SetSourceScoped(sourceRef);
    const bool linkStored = link && link->IsValid() && bridge->StoreInstanceSourceLink(instanceId, instanceGeneration, livePin->Index(), link);
    if (!linkStored || valuePin->GetDirectSource() != liveSource) {
        if (link) {
            link->Restore();
            link->Release();
        }
        livePin->Release();
        targetRef->Release();
        sourceRef->Release();
        bridge->DestroyInstance(instanceId, instanceGeneration);
        DestroySelfTestObject(context, badSource);
        DestroySelfTestObject(context, liveSource);
        error = "BBInstance Identity self-test failed to install owned source link.";
        return false;
    }
    link = nullptr;

    ParamRef *badSourceRef = new ParamRef(bridge, badSource->GetID(), ScriptBridgeSlotKind::Standalone, -1);
    const bool badSourceAccepted = liveInstance.SourceSlot(livePin, badSourceRef);
    if (badSourceAccepted || valuePin->GetDirectSource() != liveSource) {
        const CK_ID directId = valuePin->GetDirectSource() ? valuePin->GetDirectSource()->GetID() : 0;
        const CK_ID liveId = liveSource ? liveSource->GetID() : 0;
        badSourceRef->Release();
        livePin->Release();
        targetRef->Release();
        sourceRef->Release();
        bridge->DestroyInstance(instanceId, instanceGeneration);
        DestroySelfTestObject(context, badSource);
        DestroySelfTestObject(context, liveSource);
        error = fmt::format("BBInstance Identity self-test did not preserve live source after failed source replacement: accepted={} direct={} live={}.",
                            badSourceAccepted ? "true" : "false",
                            directId,
                            liveId);
        return false;
    }
    const bool badValueAccepted = liveInstance.SetSlotObject(livePin, instanceBehavior);
    if (badValueAccepted || valuePin->GetDirectSource() != liveSource) {
        badSourceRef->Release();
        livePin->Release();
        targetRef->Release();
        sourceRef->Release();
        bridge->DestroyInstance(instanceId, instanceGeneration);
        DestroySelfTestObject(context, badSource);
        DestroySelfTestObject(context, liveSource);
        error = "BBInstance Identity self-test did not preserve live source after failed value replacement.";
        return false;
    }
    badSourceRef->Release();
    livePin->Release();
    targetRef->Release();
    sourceRef->Release();
    if (!bridge->DestroyInstance(instanceId, instanceGeneration) || valuePin->GetDirectSource() != previousSource) {
        DestroySelfTestObject(context, badSource);
        DestroySelfTestObject(context, liveSource);
        error = "BBInstance Identity self-test did not restore owned source link on Destroy.";
        return false;
    }
    DestroySelfTestObject(context, badSource);
    DestroySelfTestObject(context, liveSource);

    error.clear();
    return true;
}
bool RunScriptBehaviorBridgeSelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context) {
        error = "CKContext is not available.";
        return false;
    }

    CKParameterManager *parameterManager = context->GetParameterManager();
    if (!parameterManager) {
        error = "CKParameterManager is not available.";
        return false;
    }

    std::string operationName;
    for (int i = 0; i < parameterManager->GetParameterOperationCount(); ++i) {
        const CKGUID guid = parameterManager->OperationCodeToGuid(i);
        if (!guid.IsValid()) {
            continue;
        }

        CKSTRING name = parameterManager->OperationGuidToName(guid);
        if (!name || name[0] == '\0') {
            continue;
        }

        std::string resolveError;
        const CKGUID byName = ResolveOperationGuid(context, CKGUID(), name, resolveError);
        if (byName != guid) {
            error = fmt::format("Parameter operation lookup failed for '{}'.", name);
            return false;
        }
        operationName = name;
        break;
    }
    XObjectArray objects;
    objects.PushBack(0);
    objects.PushBack(42);
    const ScriptParamValue value = MakeScriptParamObjectArray(objects);
    if (value.Kind != ScriptParamValueKind::ObjectArray ||
        value.ObjectIds().size() != 2 ||
        value.ObjectIds()[0] != 0 ||
        value.ObjectIds()[1] != 42 ||
        ScriptParameterGuidForValueKind(ScriptParamValueKind::ObjectArray) != CKPGUID_OBJECTARRAY) {
        error = "XObjectArray conversion self-test failed.";
        return false;
    }

    CKParameterLocal *stampParam = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_StampSelfTest"), CKPGUID_INT, TRUE);
    if (!stampParam) {
        error = "Failed to create parameter stamp self-test parameter.";
        return false;
    }
    const CK_ID stampParamId = stampParam->GetID();
    ScriptBridgeObjectStamp stamp = CaptureBridgeObjectStamp(stampParam);
    if (GetStampedCKObjectById(context, stampParamId, stamp) != stampParam) {
        context->DestroyObject(stampParam);
        error = "Parameter stamp self-test failed to resolve the original object.";
        return false;
    }
    stamp.TypeGuid = CKPGUID_FLOAT;
    if (GetStampedCKObjectById(context, stampParamId, stamp) != nullptr) {
        context->DestroyObject(stampParam);
        error = "Parameter stamp self-test failed to reject a mismatched type.";
        return false;
    }
    context->DestroyObject(stampParam);

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-mutation", std::string());
    if (!RunBehaviorBridgeNativeMutationSelfTest(context, parameterManager, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-mutation", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-graphtask", std::string());
    if (!RunBehaviorBridgeNativeGraphTaskSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-graphtask", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-graph-search", std::string());
    if (!RunBehaviorBridgeNativeGraphSearchSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-graph-search", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-graph-edit", std::string());
    if (!RunBehaviorBridgeNativeGraphEditSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-graph-edit", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-layout-cache", std::string());
    if (!RunBehaviorBridgeNativeLayoutCacheSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-layout-cache", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-graph-edit-dynamic-layout", std::string());
    if (!RunBehaviorBridgeNativeGraphEditDynamicLayoutSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-graph-edit-dynamic-layout", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-prototype-discovery", std::string());
    if (!RunBehaviorBridgeNativePrototypeDiscoverySelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-prototype-discovery", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-internal-shape", std::string());
    if (!RunBehaviorBridgeNativeInternalShapeSelfTest(engine, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-internal-shape", error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-runtime-bb", std::string());
    if (!RunBehaviorBridgeNativeBBInvocationSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-runtime-bb", error);
        return false;
    }

    std::string typeName;
    for (int i = 0; i < parameterManager->GetParameterTypesCount(); ++i) {
        CKSTRING name = parameterManager->ParameterTypeToName(i);
        if (name && name[0] != '\0') {
            typeName = name;
            break;
        }
    }
    if (typeName.empty()) {
        error = "No registered parameter type was found.";
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "script-compile", std::string());
    if (!RunBehaviorBridgeScriptSelfTest(context, parameterManager, engine, operationName, typeName, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, typeName, error);
        return false;
    }

    WriteBehaviorBridgeSelfTestMarker("ok", operationName, typeName, std::string());

    error.clear();
    return true;
}
