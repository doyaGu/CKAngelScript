#include "ScriptBridgeHandles.h"

#include <fmt/format.h>
#include "ScriptRunner.h"

#include <fstream>

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

struct BridgeSelfTestMessageCollector {
    std::string Text;
};

static void BridgeSelfTestMessageCallback(const asSMessageInfo *msg, void *param) {
    auto *collector = static_cast<BridgeSelfTestMessageCollector *>(param);
    if (!collector || !msg) {
        return;
    }
    if (!collector->Text.empty()) {
        collector->Text += "\n";
    }
    collector->Text += fmt::format("{}({},{}) : {}",
                                   msg->section ? msg->section : "<script>",
                                   msg->row,
                                   msg->col,
                                   msg->message ? msg->message : "<empty>");
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
    source += "    if (decl !is null) { BBConfig@ c = decl.Configure(); if (c !is null) { BBSlot@ ci = c.Input(\"In\"); BBSlot@ co = c.Output(\"Out\"); BBSlot@ cp = c.Pin(\"Value\"); BBSlot@ cq = c.Pout(\"Result\"); BBSlot@ cs = c.Setting(\"Settings\"); c.Set(cp, value); c.Set(cp, 1); c.Set(cp, 1.0f); c.Set(cp, true); c.Set(cp, \"x\"); c.Set(cp, objectValue); c.SetSetting(cs, \"x\"); c.Source(cp, source); c.Operation(cp, operation); BBInstance@ si = c.SpawnStarted(ctx); c.OutputActive(co); c.PinRef(cp); c.PoutRef(cq); c.Destroy(); if (si !is null) { BBSlot@ ri = si.Input(\"In\"); BBSlot@ ro = si.Output(\"Out\"); BBSlot@ rp = si.PinSlot(\"Value\"); BBSlot@ rq = si.PoutSlot(\"Result\"); BBSlot@ rs = si.Setting(\"Settings\"); si.Start(ctx, ri); si.OutputActive(ro); si.Pin(rp); si.Pout(rq); si.SetSetting(rs, value); si.SetSetting(rs, \"x\"); si.Stop(ctx); si.Destroy(); } } }\n";
    source += "    if (missing !is null) { missing.IsValid(); missing.GetGuid(); missing.GetName(); missing.GetCategory(); missing.GetQualifiedName(); missing.Layout(); missing.Call(); missing.Spawn(); missing.Describe(); }\n";
    source += "    if (call !is null) { call.Owner(null); call.Target(null); call.Set(0, value); call.SetSource(0, source); call.SetOperation(0, operation); BBResult@ result = call.Run(); if (result !is null) { result.Ok(); bool ok = result.ok; result.ReturnCode(); result.Error(); result.OutputActive(0); result.Pout(0); result.Raise(ctx); } }\n";
    source += "    if (spawn !is null) { spawn.Owner(null); spawn.Target(null); spawn.Set(0, value); spawn.SetSource(0, source); spawn.SetOperation(0, operation); BBTask@ task = spawn.Start(); if (task !is null) { task.IsValid(); task.IsAlive(); task.IsPaused(); task.ReturnCode(); task.Error(); task.OutputActive(0); task.Step(ctx); task.Reset(); task.Behavior(); task.Pout(0); task.Raise(ctx); task.Destroy(); } }\n";
    source += "    BBSlot@ slot = decl !is null ? decl.Pin(\"Value\") : null; BBSlot@ input = decl !is null ? decl.Input(\"In\") : null; BBSlot@ output = decl !is null ? decl.Output(\"Out\") : null;\n";
    source += "    if (call !is null) { call.Set(slot, value); call.Set(slot, 1); call.Set(slot, 1.0f); call.Set(slot, true); call.Set(slot, \"x\"); call.Set(slot, objectValue); call.Source(slot, source); call.Operation(slot, operation); BBResult@ sr = call.Run(input); if (sr !is null) { sr.OutputActive(output); sr.Pout(slot); } }\n";
    source += "    if (spawn !is null) { spawn.Set(slot, value); spawn.Set(slot, 1); spawn.Set(slot, 1.0f); spawn.Set(slot, true); spawn.Set(slot, \"x\"); spawn.Set(slot, objectValue); spawn.Source(slot, source); spawn.Operation(slot, operation); BBTask@ st = spawn.Start(input); if (st !is null) { st.OutputActive(output); st.Step(ctx, input); st.Pout(slot); } }\n";
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
    source += "    return 0;\n";
    source += "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "Failed to create AngelScript self-test module.";
        return false;
    }

    int r = module->AddScriptSection("behavior_bridge_selftest.as", source.c_str(), static_cast<unsigned int>(source.size()));
    if (r < 0) {
        error = fmt::format("Failed to add AngelScript self-test section ({}).", r);
        engine->DiscardModule(moduleName);
        return false;
    }

    BridgeSelfTestMessageCollector messageCollector;
    ScriptManager *manager = ScriptManager::GetManager(context);
    engine->SetMessageCallback(asFUNCTION(BridgeSelfTestMessageCallback), &messageCollector, asCALL_CDECL);
    r = module->Build();
    if (manager) {
        engine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), manager, asCALL_THISCALL);
    }
    if (r < 0) {
        error = fmt::format("Failed to build AngelScript self-test module ({}).{}{}",
                            r,
                            messageCollector.Text.empty() ? "" : "\n",
                            messageCollector.Text);
        engine->DiscardModule(moduleName);
        return false;
    }

    asIScriptFunction *function = module->GetFunctionByDecl("int Run(const CKBehaviorContext &in ctx)");
    if (!function) {
        error = "Failed to find AngelScript self-test Run() function.";
        engine->DiscardModule(moduleName);
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

    asIScriptContext *scriptContext = engine->CreateContext();
    if (!scriptContext) {
        error = "Failed to create AngelScript execution context.";
        engine->DiscardModule(moduleName);
        return false;
    }

    r = scriptContext->Prepare(function);
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

    scriptContext->Release();
    engine->DiscardModule(moduleName);
    return ok;
}

static void DestroySelfTestObject(CKContext *context, CKObject *object) {
    if (context && object && !object->IsToBeDeleted()) {
        context->DestroyObject(object);
    }
}

static CKGUID FindSelfTestOperationGuid(CKContext *context,
                                        CKParameterManager *parameterManager) {
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
            CKPGUID_INT,
            CKPGUID_INT,
            CKPGUID_INT);
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

    ParamRef *inputRef = new ParamRef(bridge, input->GetID(), ScriptBridgeSlotKind::Pin, 0);
    ParamRef *sourceRef = new ParamRef(bridge, sourceB->GetID(), ScriptBridgeSlotKind::Standalone, -1);
    ParamSourceLinkRef *link = inputRef->SetSourceScoped(sourceRef);
    sourceRef->Release();
    inputRef->Release();
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

    if (!link->Restore() || input->GetDirectSource() != sourceA) {
        link->Release();
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

static int ReadSelfTestGlobalInt(asIScriptModule *module,
                                 const char *name,
                                 std::string &error) {
    if (!module || !name || name[0] == '\0') {
        error = "Invalid module or global name.";
        return 0;
    }

    const int index = module->GetGlobalVarIndexByName(name);
    if (index < 0) {
        error = fmt::format("Global variable '{}' was not found.", name);
        return 0;
    }

    void *address = module->GetAddressOfGlobalVar(static_cast<asUINT>(index));
    if (!address) {
        error = fmt::format("Global variable '{}' has no address.", name);
        return 0;
    }

    return *static_cast<int *>(address);
}

static bool ProbeRuntimeRunnerInputBinding(CKContext *context,
                                           const ScriptBridgeBBInvocationSpec &request,
                                           const char *expectedScript,
                                           const char *expectedFunction,
                                           std::string &error) {
    if (!context) {
        error = "Runtime Runner input probe requires CKContext.";
        return false;
    }

    CKBehavior *behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "__CKAS_RuntimeBBInputProbe", CK_OBJECTCREATION_DYNAMIC));
    if (!behavior) {
        error = "Runtime Runner input probe failed to create behavior.";
        return false;
    }

    bool createCallbackSent = false;
    auto cleanup = [&]() {
        if (!behavior) {
            return;
        }
        if (createCallbackSent) {
            CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORDELETE);
        }
        context->DestroyObject(behavior);
        behavior = nullptr;
    };

    behavior->UseFunction();
    CKERROR err = behavior->InitFromGuid(CKGUID(0x53295cee, 0x1a795bb8));
    if (err != CK_OK) {
        error = fmt::format("Runtime Runner input probe InitFromGuid failed with CKERROR {}.", err);
        cleanup();
        return false;
    }

    err = behavior->SetOwner(nullptr, FALSE);
    if (err != CK_OK) {
        error = fmt::format("Runtime Runner input probe SetOwner failed with CKERROR {}.", err);
        cleanup();
        return false;
    }

    err = CallBridgeBehaviorCallback(behavior, CKM_BEHAVIORCREATE);
    if (err != CK_OK) {
        error = fmt::format("Runtime Runner input probe CREATE failed with CKERROR {}.", err);
        cleanup();
        return false;
    }
    createCallbackSent = true;

    ScriptBridgeInputSourceBindings inputSources;
    if (!ApplyIndexedInputParameters(behavior, request.IndexedParameters, error, &inputSources)) {
        error = "Runtime Runner input probe failed to apply inputs: " + error;
        cleanup();
        return false;
    }

    const char *script = static_cast<const char *>(behavior->GetInputParameterReadDataPtr(0));
    const char *function = static_cast<const char *>(behavior->GetInputParameterReadDataPtr(1));
    const std::string scriptText = script ? std::string(script) : std::string();
    const std::string functionText = function ? std::string(function) : std::string();
    if (scriptText != expectedScript || functionText != expectedFunction) {
        error = fmt::format("Runtime Runner input probe read mismatch: script='{}' function='{}'.",
                            scriptText,
                            functionText);
        cleanup();
        return false;
    }

    ScriptRunner *runner = nullptr;
    behavior->GetLocalParameterValue(0, &runner);
    if (!runner) {
        CKBehaviorPrototype *prototype = behavior->GetPrototype();
        error = fmt::format("Runtime Runner input probe CREATE did not install ScriptRunner local state (locals={}, prototype='{}', callback={}).",
                            behavior->GetLocalParameterCount(),
                            SafeString(prototype ? prototype->GetName() : nullptr),
                            prototype && prototype->GetBehaviorCallbackFct() ? "present" : "missing");
        cleanup();
        return false;
    }

    if (!runner->Attach(behavior, true)) {
        error = "Runtime Runner input probe Attach failed: " + runner->GetErrorMessage();
        cleanup();
        return false;
    }

    asIScriptFunction *func = nullptr;
    behavior->GetLocalParameterValue(1, &func);
    if (!func) {
        error = "Runtime Runner input probe Attach did not cache the function.";
        cleanup();
        return false;
    }

    cleanup();
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
    const ScriptBridgeLayoutRecord *refreshed = bridge->GetBehaviorLayout(behavior->GetID(), stamp);
    if (!refreshed ||
        refreshed->Pins.size() != 2 ||
        refreshed->Pins[1].Name != "Second" ||
        refreshed->Signature == firstSignature) {
        DestroySelfTestObject(context, behavior);
        error = "Layout cache refresh self-test failed.";
        return false;
    }

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

static bool RunBehaviorBridgeNativeRuntimeBBSelfTest(CKContext *context,
                                                     std::string &error) {
    ScriptManager *manager = ScriptManager::GetManager(context);
    ScriptBehaviorBridge *bridge = manager ? manager->GetBehaviorBridge() : nullptr;
    if (!context || !manager || !bridge) {
        error = "Runtime BB self-test requires CKContext, ScriptManager, and ScriptBehaviorBridge.";
        return false;
    }

    constexpr const char *scriptName = "__CKAS_RuntimeBBSelfTest";
    manager->UnloadScript(scriptName);

    const char *source =
        "int g_BridgeRunnerCount = 0;\n"
        "int BridgeRunnerEntry(const CKBehaviorContext &in ctx) {\n"
        "    g_BridgeRunnerCount += 1;\n"
        "    return 0;\n"
        "}\n";

    if (manager->CompileScript(scriptName, source) < 0) {
        error = "Failed to compile runtime BB self-test script.";
        return false;
    }

    asIScriptModule *module = manager->GetScript(scriptName);
    if (!module) {
        manager->UnloadScript(scriptName);
        error = "Runtime BB self-test script module was not found after compilation.";
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

    ScriptBridgeBBInvocationSpec request = MakeDefaultRequest(behaviorContext);
    request.PrototypeName = "AngelScript Runner";
    ScriptBridgeSetIndexedValue(request.IndexedParameters, 0, MakeScriptParamString(scriptName));
    ScriptBridgeSetIndexedValue(request.IndexedParameters, 1, MakeScriptParamString("BridgeRunnerEntry"));

    if (!ProbeRuntimeRunnerInputBinding(context, request, scriptName, "BridgeRunnerEntry", error)) {
        manager->UnloadScript(scriptName);
        return false;
    }

    BBResult *result = bridge->RunCall(request, behaviorContext, 0);
    if (!result) {
        manager->UnloadScript(scriptName);
        error = "BB.Call runtime self-test did not return a result.";
        return false;
    }

    const bool callStateOk = result->Ok();
    const int callReturnCode = result->ReturnCode();
    const bool callOutActive = result->OutputActive(0);
    const bool callErrorActive = result->OutputActive(1);
    const bool callOk = callStateOk && callReturnCode == CKBR_OK && callOutActive;
    std::string callError = result->Error();
    result->Release();
    if (!callOk) {
        manager->UnloadScript(scriptName);
        error = fmt::format("BB.Call runtime self-test failed: ok={} rc={} out={} errorOut={} error={}",
                            callStateOk ? "true" : "false",
                            callReturnCode,
                            callOutActive ? "true" : "false",
                            callErrorActive ? "true" : "false",
                            callError.empty() ? "<empty>" : callError);
        return false;
    }

    int count = ReadSelfTestGlobalInt(module, "g_BridgeRunnerCount", error);
    if (!error.empty() || count != 1) {
        manager->UnloadScript(scriptName);
        if (error.empty()) {
            error = fmt::format("BB.Call runtime self-test expected count 1, got {}.", count);
        }
        return false;
    }

    BBTask *task = bridge->StartTask(request, behaviorContext, 0);
    if (!task) {
        manager->UnloadScript(scriptName);
        error = "BB.Spawn runtime self-test did not return a task.";
        return false;
    }

    const bool taskValid = task->IsValid();
    const bool taskAlive = task->IsAlive();
    const int taskReturnCode = task->ReturnCode();
    const bool taskOutActive = task->OutputActive(0);
    const bool taskErrorActive = task->OutputActive(1);
    const bool taskOk = taskValid && taskAlive && taskReturnCode == CKBR_OK && taskOutActive;
    std::string taskError = task->Error();
    const bool taskStepOk = task->Step(behaviorContext, 0);
    const int taskStepReturnCode = task->ReturnCode();
    const bool taskStepOutActive = task->OutputActive(0);
    const bool taskStepErrorActive = task->OutputActive(1);
    if (!taskStepOk || taskStepReturnCode != CKBR_OK || !taskStepOutActive) {
        taskError = task->Error();
        task->Destroy();
        task->Release();
        manager->UnloadScript(scriptName);
        error = fmt::format("BB.Spawn runtime self-test Step failed: ok={} rc={} out={} errorOut={} error={}",
                            taskStepOk ? "true" : "false",
                            taskStepReturnCode,
                            taskStepOutActive ? "true" : "false",
                            taskStepErrorActive ? "true" : "false",
                            taskError.empty() ? "<empty>" : taskError);
        return false;
    }
    if (!task->Destroy() || task->IsValid()) {
        task->Release();
        manager->UnloadScript(scriptName);
        error = "BB.Spawn runtime self-test task did not destroy cleanly.";
        return false;
    }
    task->Release();

    if (!taskOk) {
        manager->UnloadScript(scriptName);
        error = fmt::format("BB.Spawn runtime self-test failed: valid={} alive={} rc={} out={} errorOut={} error={}",
                            taskValid ? "true" : "false",
                            taskAlive ? "true" : "false",
                            taskReturnCode,
                            taskOutActive ? "true" : "false",
                            taskErrorActive ? "true" : "false",
                            taskError.empty() ? "<empty>" : taskError);
        return false;
    }

    int instanceGeneration = 0;
    std::string instanceError;
    CK_ID instanceId = bridge->CreateInstance(request, behaviorContext, instanceGeneration, instanceError);
    CKBehavior *instanceBehavior = instanceId ? bridge->GetInstanceBehavior(instanceId, instanceGeneration) : nullptr;
    CKParameterIn *scriptPin = instanceBehavior ? instanceBehavior->GetInputParameter(0) : nullptr;
    CKParameter *previousSource = scriptPin ? scriptPin->GetDirectSource() : nullptr;
    CKParameterLocal *liveSource = context->CreateCKParameterLocal(const_cast<CKSTRING>("__CKAS_InstanceLiveSource"), CKPGUID_STRING, TRUE);
    if (!instanceId || !instanceBehavior || !scriptPin || !previousSource || !liveSource ||
        liveSource->SetStringValue(const_cast<CKSTRING>(scriptName)) != CK_OK) {
        if (instanceId) {
            bridge->DestroyInstance(instanceId, instanceGeneration);
        }
        DestroySelfTestObject(context, liveSource);
        manager->UnloadScript(scriptName);
        error = instanceError.empty() ? "BBInstance runtime self-test setup failed." : instanceError;
        return false;
    }

    ParamRef *targetRef = new ParamRef(bridge, scriptPin->GetID(), ScriptBridgeSlotKind::Pin, 0, instanceBehavior->GetID());
    ParamRef *sourceRef = new ParamRef(bridge, liveSource->GetID(), ScriptBridgeSlotKind::Standalone, -1);
    ParamSourceLinkRef *link = targetRef->SetSourceScoped(sourceRef);
    const bool linkStored = link && link->IsValid() && bridge->StoreInstanceSourceLink(instanceId, instanceGeneration, 0, link);
    if (!linkStored || scriptPin->GetDirectSource() != liveSource) {
        if (link) {
            link->Restore();
            link->Release();
        }
        targetRef->Release();
        sourceRef->Release();
        bridge->DestroyInstance(instanceId, instanceGeneration);
        DestroySelfTestObject(context, liveSource);
        manager->UnloadScript(scriptName);
        error = "BBInstance runtime self-test failed to install owned source link.";
        return false;
    }

    targetRef->Release();
    sourceRef->Release();
    if (!bridge->DestroyInstance(instanceId, instanceGeneration) || scriptPin->GetDirectSource() != previousSource) {
        DestroySelfTestObject(context, liveSource);
        manager->UnloadScript(scriptName);
        error = "BBInstance runtime self-test did not restore owned source link on Destroy.";
        return false;
    }
    DestroySelfTestObject(context, liveSource);

    error.clear();
    count = ReadSelfTestGlobalInt(module, "g_BridgeRunnerCount", error);
    if (!error.empty() || count != 3) {
        manager->UnloadScript(scriptName);
        if (error.empty()) {
            error = fmt::format("BB.Spawn runtime self-test expected count 3, got {}.", count);
        }
        return false;
    }

    manager->UnloadScript(scriptName);
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

    WriteBehaviorBridgeSelfTestMarker("running", operationName, "native-layout-cache", std::string());
    if (!RunBehaviorBridgeNativeLayoutCacheSelfTest(context, error)) {
        WriteBehaviorBridgeSelfTestMarker("failed", operationName, "native-layout-cache", error);
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
    if (!RunBehaviorBridgeNativeRuntimeBBSelfTest(context, error)) {
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
