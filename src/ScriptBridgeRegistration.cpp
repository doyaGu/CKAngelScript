#include "ScriptBridgeHandles.h"

#include <cassert>

static ScriptBehaviorBridge *BridgeFromContext(const CKBehaviorContext &ctx) {
    ScriptManager *manager = ManagerFromContext(ctx);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

static BehaviorBridge *BehaviorFromContext(const CKBehaviorContext &ctx) {
    if (ScriptBehaviorBridge *bridge = BridgeFromContext(ctx)) {
        return bridge->CreateBehaviorBridge(ctx);
    }
    SetScriptException("AngelScript behavior bridge is not available.");
    return nullptr;
}

static BBBridge *BBFromContext(const CKBehaviorContext &ctx) {
    if (ScriptBehaviorBridge *bridge = BridgeFromContext(ctx)) {
        return new BBBridge(bridge, ctx);
    }
    SetScriptException("AngelScript BB bridge is not available.");
    return nullptr;
}

static BehaviorRef *BehaviorFindByName(const CKBehaviorContext &ctx, const std::string &name) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->Find(name);
    bridge->Release();
    return ref;
}

static BehaviorRef *BehaviorFindOnOwner(const CKBehaviorContext &ctx, CKBeObject *owner, const std::string &name) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->FindOn(owner, name);
    bridge->Release();
    return ref;
}

static BehaviorRef *BehaviorFindById(const CKBehaviorContext &ctx, CK_ID id) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->FindByID(id);
    bridge->Release();
    return ref;
}

static GraphTask *BehaviorStartByName(const CKBehaviorContext &ctx,
                                      const std::string &name,
                                      int inputIndex,
                                      bool reset,
                                      float timeoutSeconds) {
    BehaviorRef *ref = BehaviorFindByName(ctx, name);
    if (!ref) {
        SetScriptException(fmt::format("Behavior '{}' not found.", name));
        return nullptr;
    }
    GraphTask *task = ref->Start(inputIndex, reset, timeoutSeconds);
    ref->Release();
    return task;
}

static GraphTask *BehaviorWatchByName(const CKBehaviorContext &ctx,
                                      const std::string &name,
                                      float timeoutSeconds) {
    BehaviorRef *ref = BehaviorFindByName(ctx, name);
    if (!ref) {
        SetScriptException(fmt::format("Behavior '{}' not found.", name));
        return nullptr;
    }
    GraphTask *task = ref->Watch(timeoutSeconds);
    ref->Release();
    return task;
}

static BBPrototype *BBPrototypeByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->PrototypeByName(name);
    bridge->Release();
    return prototype;
}

static BBPrototype *BBPrototypeByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->PrototypeByGuid(guid);
    bridge->Release();
    return prototype;
}

static BBCallBuilder *BBCallByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBPrototype *prototype = BBPrototypeByName(ctx, name);
    if (!prototype) {
        return nullptr;
    }
    BBCallBuilder *builder = prototype->Call();
    prototype->Release();
    return builder;
}

static BBTaskBuilder *BBSpawnByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBPrototype *prototype = BBPrototypeByName(ctx, name);
    if (!prototype) {
        return nullptr;
    }
    BBTaskBuilder *builder = prototype->Spawn();
    prototype->Release();
    return builder;
}

static ParamValue *ParamInt(int value) { return new ParamValue(MakeBridgeParamValue(MakeIntValue(value))); }
static ParamValue *ParamFloat(float value) { return new ParamValue(MakeBridgeParamValue(MakeFloatValue(value))); }
static ParamValue *ParamBool(bool value) { return new ParamValue(MakeBridgeParamValue(MakeBoolValue(value))); }
static ParamValue *ParamString(const std::string &value) { return new ParamValue(MakeBridgeParamValue(MakeStringValue(value))); }
static ParamValue *ParamGuid(CKGUID value) { return new ParamValue(MakeBridgeParamValue(MakeGuidValue(value))); }
static ParamValue *ParamVector(const VxVector &value) { return new ParamValue(MakeBridgeParamValue(MakeVectorValue(value))); }
static ParamValue *ParamVector2(const Vx2DVector &value) { return new ParamValue(MakeBridgeParamValue(MakeVector2Value(value))); }
static ParamValue *ParamColor(const VxColor &value) { return new ParamValue(MakeBridgeParamValue(MakeColorValue(value))); }
static ParamValue *ParamQuaternion(const VxQuaternion &value) { return new ParamValue(MakeBridgeParamValue(MakeQuaternionValue(value))); }
static ParamValue *ParamMatrix(const VxMatrix &value) { return new ParamValue(MakeBridgeParamValue(MakeMatrixValue(value))); }
static ParamValue *ParamObject(CKObject *value) { return new ParamValue(MakeBridgeParamValue(MakeObjectValue(value))); }
static ParamValue *ParamObjectArray(const XObjectArray &value) { return new ParamValue(MakeBridgeParamValue(MakeObjectArrayValue(value))); }

static ParamValue *ParamTextByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &text) {
    CKContext *context = ctx.Context;
    return new ParamValue(MakeBridgeTextValue(text, ScriptResolveParameterGuid(context, typeName, ScriptBridgeValueKind::String), typeName));
}

static ParamValue *ParamTextByGuid(const CKBehaviorContext &, CKGUID guid, const std::string &text) {
    return new ParamValue(MakeBridgeTextValue(text, guid, std::string()));
}

static ParamValue *ParamRawByName(const CKBehaviorContext &ctx, const std::string &typeName, NativeBuffer *buffer) {
    CKContext *context = ctx.Context;
    CKGUID guid = ScriptResolveParameterGuid(context, typeName, ScriptBridgeValueKind::None);
    return new ParamValue(MakeBridgeRawValue(guid, typeName, buffer ? buffer->Data() : nullptr, buffer ? static_cast<int>(buffer->Size()) : 0));
}

static ParamValue *ParamRawByGuid(const CKBehaviorContext &, CKGUID guid, NativeBuffer *buffer) {
    return new ParamValue(MakeBridgeRawValue(guid, std::string(), buffer ? buffer->Data() : nullptr, buffer ? static_cast<int>(buffer->Size()) : 0));
}

static ParamOp *ParamOperationByName(const CKBehaviorContext &ctx, const std::string &name) {
    ScriptBehaviorBridge *bridge = BridgeFromContext(ctx);
    if (!bridge) {
        SetScriptException("AngelScript behavior bridge is not available.");
        return nullptr;
    }

    std::string error;
    CKContext *context = ctx.Context ? ctx.Context : (bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr);
    if (!ResolveOperationGuid(context, CKGUID(), name, error).IsValid()) {
        SetScriptException(error);
        return nullptr;
    }
    return new ParamOp(bridge, ctx, name);
}

static ParamOp *ParamOperationByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptBehaviorBridge *bridge = BridgeFromContext(ctx);
    if (!bridge) {
        SetScriptException("AngelScript behavior bridge is not available.");
        return nullptr;
    }

    std::string error;
    CKContext *context = ctx.Context ? ctx.Context : (bridge->GetManager() ? bridge->GetManager()->GetCKContext() : nullptr);
    if (!ResolveOperationGuid(context, guid, std::string(), error).IsValid()) {
        SetScriptException(error);
        return nullptr;
    }
    return new ParamOp(bridge, ctx, guid);
}

template <typename T>
static void RegisterRefCountedHandle(asIScriptEngine *engine, const char *typeName) {
    int r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);
    assert(r >= 0);
}

void RegisterScriptBehaviorBridge(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;

    r = engine->RegisterEnum("ParamKind"); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Input", static_cast<int>(ScriptBridgeSlotKind::Input)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Output", static_cast<int>(ScriptBridgeSlotKind::Output)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Pin", static_cast<int>(ScriptBridgeSlotKind::Pin)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Pout", static_cast<int>(ScriptBridgeSlotKind::Pout)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Local", static_cast<int>(ScriptBridgeSlotKind::Local)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "OperationIn", static_cast<int>(ScriptBridgeSlotKind::OperationIn)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "OperationOut", static_cast<int>(ScriptBridgeSlotKind::OperationOut)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Standalone", static_cast<int>(ScriptBridgeSlotKind::Standalone)); assert(r >= 0);

    r = engine->RegisterObjectType("ParamInfo", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamValue", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamRef", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamOp", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("ParamOperationRef", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BehaviorLayout", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BehaviorRef", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BehaviorBridge", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBPrototype", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBBridge", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBCallBuilder", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBTaskBuilder", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBResult", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("BBTask", 0, asOBJ_REF); assert(r >= 0);
    r = engine->RegisterObjectType("GraphTask", 0, asOBJ_REF); assert(r >= 0);

    RegisterRefCountedHandle<ParamInfo>(engine, "ParamInfo");
    r = engine->RegisterObjectMethod("ParamInfo", "ParamKind get_kind() const", asMETHOD(ParamInfo, GetKind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "int get_index() const", asMETHOD(ParamInfo, GetIndex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string get_name() const", asMETHOD(ParamInfo, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "CKGUID TypeGuid() const", asMETHOD(ParamInfo, GetTypeGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string TypeName() const", asMETHOD(ParamInfo, GetTypeName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "int DataSize() const", asMETHOD(ParamInfo, GetDataSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string Describe() const", asMETHOD(ParamInfo, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<ParamValue>(engine, "ParamValue");
    r = engine->RegisterObjectMethod("ParamValue", "bool IsValid() const", asMETHOD(ParamValue, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "int AsInt() const", asMETHOD(ParamValue, AsInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "float AsFloat() const", asMETHOD(ParamValue, AsFloat), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "bool AsBool() const", asMETHOD(ParamValue, AsBool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "string AsString() const", asMETHOD(ParamValue, AsString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "string AsText() const", asMETHOD(ParamValue, AsText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "CKGUID AsGuid() const", asMETHOD(ParamValue, AsGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "VxVector AsVector() const", asMETHOD(ParamValue, AsVector), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "Vx2DVector AsVector2() const", asMETHOD(ParamValue, AsVector2), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "VxColor AsColor() const", asMETHOD(ParamValue, AsColor), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "VxQuaternion AsQuaternion() const", asMETHOD(ParamValue, AsQuaternion), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "VxMatrix AsMatrix() const", asMETHOD(ParamValue, AsMatrix), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamValue", "NativeBuffer@ AsRaw() const", asMETHOD(ParamValue, AsRaw), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<ParamRef>(engine, "ParamRef");
    r = engine->RegisterObjectMethod("ParamRef", "bool IsValid() const", asMETHOD(ParamRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool get_valid() const", asMETHOD(ParamRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "CK_ID get_id() const", asMETHOD(ParamRef, GetID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "int get_index() const", asMETHOD(ParamRef, GetIndex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamKind get_kind() const", asMETHOD(ParamRef, GetKind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string get_name() const", asMETHOD(ParamRef, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "CKGUID TypeGuid() const", asMETHOD(ParamRef, TypeGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string TypeName() const", asMETHOD(ParamRef, TypeName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "int DataSize() const", asMETHOD(ParamRef, DataSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamRef@ RealSource() const", asMETHOD(ParamRef, RealSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamRef@ DirectSource() const", asMETHOD(ParamRef, DirectSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetSource(ParamRef@ source)", asMETHOD(ParamRef, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool Set(ParamValue@ value)", asMETHOD(ParamRef, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamValue@ Get() const", asMETHOD(ParamRef, GetValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool CopyFrom(ParamRef@ source)", asMETHOD(ParamRef, CopyFrom), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string GetText() const", asMETHOD(ParamRef, GetText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetText(const string &in text)", asMETHOD(ParamRef, SetText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "NativeBuffer@ GetRaw() const", asMETHOD(ParamRef, GetRaw), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetRaw(NativeBuffer@ data)", asMETHOD(ParamRef, SetRaw), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string Describe() const", asMETHOD(ParamRef, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<ParamOp>(engine, "ParamOp");
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ Result(CKGUID guid)", asMETHODPR(ParamOp, Result, (CKGUID), ParamOp *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ Result(const string &in typeName)", asMETHOD(ParamOp, ResultName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ In(int slot, ParamRef@ source)", asMETHOD(ParamOp, InRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ In(int slot, ParamValue@ value)", asMETHOD(ParamOp, InValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "string Describe() const", asMETHOD(ParamOp, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<ParamOperationRef>(engine, "ParamOperationRef");
    r = engine->RegisterObjectMethod("ParamOperationRef", "bool IsValid() const", asMETHOD(ParamOperationRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ Out() const", asMETHOD(ParamOperationRef, Out), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ In1() const", asMETHOD(ParamOperationRef, In1), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ In2() const", asMETHOD(ParamOperationRef, In2), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "CKERROR Do() const", asMETHOD(ParamOperationRef, Do), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "bool Destroy()", asMETHOD(ParamOperationRef, Destroy), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "string Describe() const", asMETHOD(ParamOperationRef, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BehaviorLayout>(engine, "BehaviorLayout");
    r = engine->RegisterObjectMethod("BehaviorLayout", "int InputCount() const", asMETHOD(BehaviorLayout, InputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int OutputCount() const", asMETHOD(BehaviorLayout, OutputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int PinCount() const", asMETHOD(BehaviorLayout, PinCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int PoutCount() const", asMETHOD(BehaviorLayout, PoutCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int LocalCount() const", asMETHOD(BehaviorLayout, LocalCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string InputName(int index) const", asMETHOD(BehaviorLayout, InputName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string OutputName(int index) const", asMETHOD(BehaviorLayout, OutputNameAt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Pin(int index) const", asMETHOD(BehaviorLayout, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Pout(int index) const", asMETHOD(BehaviorLayout, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Local(int index) const", asMETHOD(BehaviorLayout, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindInput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindInput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindOutput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindOutput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindPin(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindPout(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindLocal(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindLocal), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string Describe() const", asMETHOD(BehaviorLayout, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BehaviorRef>(engine, "BehaviorRef");
    r = engine->RegisterObjectMethod("BehaviorRef", "bool IsValid() const", asMETHOD(BehaviorRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool get_valid() const", asMETHOD(BehaviorRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "CK_ID get_id() const", asMETHOD(BehaviorRef, GetID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "string get_name() const", asMETHOD(BehaviorRef, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "BehaviorLayout@ Layout() const", asMETHOD(BehaviorRef, Layout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool Trigger(int inputIndex = 0, bool reset = false)", asMETHOD(BehaviorRef, Trigger), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool InputActive(int inputIndex) const", asMETHOD(BehaviorRef, InputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool OutputActive(int outputIndex) const", asMETHOD(BehaviorRef, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pin(int index) const", asMETHOD(BehaviorRef, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pout(int index) const", asMETHOD(BehaviorRef, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Local(int index) const", asMETHOD(BehaviorRef, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "GraphTask@ Start(int inputIndex = 0, bool reset = false, float timeout = 0.0f)", asMETHOD(BehaviorRef, Start), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "GraphTask@ Watch(float timeout = 0.0f)", asMETHOD(BehaviorRef, Watch), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamOperationRef@ ConnectOperation(int pinIndex, ParamOp@ op)", asMETHOD(BehaviorRef, ConnectOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "string Describe() const", asMETHOD(BehaviorRef, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BehaviorBridge>(engine, "BehaviorBridge");
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ Self() const", asMETHOD(BehaviorBridge, Self), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ OwnerScript() const", asMETHOD(BehaviorBridge, OwnerScript), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ Find(const string &in name) const", asMETHOD(BehaviorBridge, Find), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ FindOn(CKBeObject@ owner, const string &in name) const", asMETHOD(BehaviorBridge, FindOn), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ FindByID(CK_ID id) const", asMETHOD(BehaviorBridge, FindByID), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBPrototype>(engine, "BBPrototype");
    r = engine->RegisterObjectMethod("BBPrototype", "bool IsValid() const", asMETHOD(BBPrototype, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "CKGUID GetGuid() const", asMETHOD(BBPrototype, GetGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string GetName() const", asMETHOD(BBPrototype, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string GetQualifiedName() const", asMETHOD(BBPrototype, GetQualifiedName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BehaviorLayout@ Layout() const", asMETHOD(BBPrototype, Layout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BBCallBuilder@ Call()", asMETHOD(BBPrototype, Call), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BBTaskBuilder@ Spawn()", asMETHOD(BBPrototype, Spawn), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string Describe() const", asMETHOD(BBPrototype, Describe), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBBridge>(engine, "BBBridge");
    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ Prototype(const string &in name) const", asMETHOD(BBBridge, PrototypeByName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ Prototype(CKGUID guid) const", asMETHOD(BBBridge, PrototypeByGuid), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBCallBuilder>(engine, "BBCallBuilder");
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Owner(CKBeObject@ owner)", asMETHOD(BBCallBuilder, Owner), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Target(CKBeObject@ target)", asMETHOD(BBCallBuilder, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(int pinIndex, ParamValue@ value)", asMETHOD(BBCallBuilder, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ SetSource(int pinIndex, ParamRef@ source)", asMETHOD(BBCallBuilder, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ SetOperation(int pinIndex, ParamOp@ operation)", asMETHOD(BBCallBuilder, SetOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBResult@ Run(int inputIndex = 0)", asMETHOD(BBCallBuilder, Run), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBTaskBuilder>(engine, "BBTaskBuilder");
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Owner(CKBeObject@ owner)", asMETHOD(BBTaskBuilder, Owner), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Target(CKBeObject@ target)", asMETHOD(BBTaskBuilder, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(int pinIndex, ParamValue@ value)", asMETHOD(BBTaskBuilder, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ SetSource(int pinIndex, ParamRef@ source)", asMETHOD(BBTaskBuilder, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ SetOperation(int pinIndex, ParamOp@ operation)", asMETHOD(BBTaskBuilder, SetOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTask@ Start(int inputIndex = 0)", asMETHOD(BBTaskBuilder, Start), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBResult>(engine, "BBResult");
    r = engine->RegisterObjectMethod("BBResult", "bool Ok() const", asMETHOD(BBResult, Ok), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool get_ok() const", asMETHOD(BBResult, Ok), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "int ReturnCode() const", asMETHOD(BBResult, ReturnCode), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "string Error() const", asMETHOD(BBResult, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool OutputActive(int outputIndex) const", asMETHOD(BBResult, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "ParamRef@ Pout(int index) const", asMETHOD(BBResult, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBResult, Raise), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<BBTask>(engine, "BBTask");
    r = engine->RegisterObjectMethod("BBTask", "bool IsValid() const", asMETHOD(BBTask, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool IsAlive() const", asMETHOD(BBTask, IsAlive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool IsPaused() const", asMETHOD(BBTask, IsPaused), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "int ReturnCode() const", asMETHOD(BBTask, ReturnCode), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "string Error() const", asMETHOD(BBTask, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool OutputActive(int outputIndex) const", asMETHOD(BBTask, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Step(const CKBehaviorContext &in ctx, int inputIndex = -1)", asMETHOD(BBTask, Step), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Reset()", asMETHOD(BBTask, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Destroy()", asMETHOD(BBTask, Destroy), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "BehaviorRef@ Behavior() const", asMETHOD(BBTask, Behavior), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "ParamRef@ Pout(int index) const", asMETHOD(BBTask, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBTask, Raise), asCALL_THISCALL); assert(r >= 0);

    RegisterRefCountedHandle<GraphTask>(engine, "GraphTask");
    r = engine->RegisterObjectMethod("GraphTask", "bool IsValid() const", asMETHOD(GraphTask, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool get_valid() const", asMETHOD(GraphTask, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool IsAlive() const", asMETHOD(GraphTask, IsAlive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool IsPaused() const", asMETHOD(GraphTask, IsPaused), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool TimedOut() const", asMETHOD(GraphTask, TimedOut), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "string Error() const", asMETHOD(GraphTask, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "float Elapsed() const", asMETHOD(GraphTask, Elapsed), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "GraphTask@ Timeout(float seconds)", asMETHOD(GraphTask, Timeout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Step(const CKBehaviorContext &in ctx)", asMETHOD(GraphTask, Step), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Done(int outputIndex = -1) const", asMETHOD(GraphTask, Done), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool OutputActive(int outputIndex = -1) const", asMETHOD(GraphTask, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Cancel()", asMETHOD(GraphTask, Cancel), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Reset()", asMETHOD(GraphTask, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "BehaviorRef@ Behavior() const", asMETHOD(GraphTask, Behavior), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "ParamRef@ Pout(int index) const", asMETHOD(GraphTask, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(GraphTask, Raise), asCALL_THISCALL); assert(r >= 0);

    const char *previousNamespace = engine->GetDefaultNamespace();
    std::string previous = previousNamespace ? previousNamespace : "";

    r = engine->SetDefaultNamespace("Behavior"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorBridge@ From(const CKBehaviorContext &in ctx)", asFUNCTION(BehaviorFromContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ Find(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BehaviorFindByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ Find(const CKBehaviorContext &in ctx, CKBeObject@ owner, const string &in name)", asFUNCTION(BehaviorFindOnOwner), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BehaviorRef@ FindByID(const CKBehaviorContext &in ctx, CK_ID id)", asFUNCTION(BehaviorFindById), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("GraphTask@ Start(const CKBehaviorContext &in ctx, const string &in name, int inputIndex = 0, bool reset = false, float timeout = 0.0f)", asFUNCTION(BehaviorStartByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("GraphTask@ Watch(const CKBehaviorContext &in ctx, const string &in name, float timeout = 0.0f)", asFUNCTION(BehaviorWatchByName), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("BB"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBBridge@ From(const CKBehaviorContext &in ctx)", asFUNCTION(BBFromContext), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ Prototype(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BBPrototypeByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ Prototype(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(BBPrototypeByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBCallBuilder@ Call(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BBCallByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBTaskBuilder@ Spawn(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BBSpawnByName), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("Param"); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Int(int value)", asFUNCTION(ParamInt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Float(float value)", asFUNCTION(ParamFloat), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Bool(bool value)", asFUNCTION(ParamBool), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ String(const string &in value)", asFUNCTION(ParamString), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Guid(CKGUID value)", asFUNCTION(ParamGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Vector(const VxVector &in value)", asFUNCTION(ParamVector), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Vector2(const Vx2DVector &in value)", asFUNCTION(ParamVector2), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Color(const VxColor &in value)", asFUNCTION(ParamColor), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Quaternion(const VxQuaternion &in value)", asFUNCTION(ParamQuaternion), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Matrix(const VxMatrix &in value)", asFUNCTION(ParamMatrix), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Object(CKObject@ value)", asFUNCTION(ParamObject), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ ObjectArray(const XObjectArray &in value)", asFUNCTION(ParamObjectArray), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Text(const CKBehaviorContext &in ctx, const string &in typeName, const string &in text)", asFUNCTION(ParamTextByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Text(const CKBehaviorContext &in ctx, CKGUID typeGuid, const string &in text)", asFUNCTION(ParamTextByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Raw(const CKBehaviorContext &in ctx, const string &in typeName, NativeBuffer@ data)", asFUNCTION(ParamRawByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Raw(const CKBehaviorContext &in ctx, CKGUID typeGuid, NativeBuffer@ data)", asFUNCTION(ParamRawByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamOp@ Operation(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(ParamOperationByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamOp@ Operation(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ParamOperationByGuid), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}
