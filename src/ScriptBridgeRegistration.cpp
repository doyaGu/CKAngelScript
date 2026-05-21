#include "ScriptBridgeHandles.h"

#include <cassert>

#include <fmt/format.h>

namespace ScriptBridgeRegistrationInternal {

ScriptBehaviorBridge *BridgeFromContext(const CKBehaviorContext &ctx) {
    ScriptManager *manager = ManagerFromContext(ctx);
    return manager ? manager->GetBehaviorBridge() : nullptr;
}

BehaviorBridge *BehaviorFromContext(const CKBehaviorContext &ctx) {
    if (ScriptBehaviorBridge *bridge = BridgeFromContext(ctx)) {
        return bridge->CreateBehaviorBridge(ctx);
    }
    SetScriptException("AngelScript behavior bridge is not available.");
    return nullptr;
}

BBBridge *BBFromContext(const CKBehaviorContext &ctx) {
    if (ScriptBehaviorBridge *bridge = BridgeFromContext(ctx)) {
        return new BBBridge(bridge, ctx);
    }
    SetScriptException("AngelScript BB bridge is not available.");
    return nullptr;
}

BehaviorRef *BehaviorFindByName(const CKBehaviorContext &ctx, const std::string &name) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->Find(name);
    bridge->Release();
    return ref;
}

BehaviorRef *BehaviorFindOnOwner(const CKBehaviorContext &ctx, CKBeObject *owner, const std::string &name) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->FindOn(owner, name);
    bridge->Release();
    return ref;
}

BehaviorRef *BehaviorFindById(const CKBehaviorContext &ctx, CK_ID id) {
    BehaviorBridge *bridge = BehaviorFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BehaviorRef *ref = bridge->FindByID(id);
    bridge->Release();
    return ref;
}

GraphTask *BehaviorStartByName(const CKBehaviorContext &ctx,
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

GraphTask *BehaviorWatchByName(const CKBehaviorContext &ctx,
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

BBPrototype *BBPrototypeByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->PrototypeByName(name);
    bridge->Release();
    return prototype;
}

BBPrototype *BBPrototypeByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->PrototypeByGuid(guid);
    bridge->Release();
    return prototype;
}

int BBCount(const CKBehaviorContext &ctx) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return 0;
    }
    const int count = bridge->Count();
    bridge->Release();
    return count;
}

BBPrototype *BBAt(const CKBehaviorContext &ctx, int index) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->At(index);
    bridge->Release();
    return prototype;
}

BBPrototype *BBFind(const CKBehaviorContext &ctx, const std::string &query, int occurrence) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBPrototype *prototype = bridge->Find(query, occurrence);
    bridge->Release();
    return prototype;
}

CScriptArray *BBFindAll(const CKBehaviorContext &ctx, const std::string &query) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    CScriptArray *prototypes = bridge->FindAll(query);
    bridge->Release();
    return prototypes;
}

BBCallBuilder *BBCallByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBPrototype *prototype = BBPrototypeByName(ctx, name);
    if (!prototype) {
        return nullptr;
    }
    BBCallBuilder *builder = prototype->Call();
    prototype->Release();
    return builder;
}

BBTaskBuilder *BBSpawnByName(const CKBehaviorContext &ctx, const std::string &name) {
    BBPrototype *prototype = BBPrototypeByName(ctx, name);
    if (!prototype) {
        return nullptr;
    }
    BBTaskBuilder *builder = prototype->Spawn();
    prototype->Release();
    return builder;
}

BBDecl *BBRequireByName(const CKBehaviorContext &ctx, const std::string &query) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *spec = bridge->Require(query);
    bridge->Release();
    return spec;
}

BBDecl *BBRequireByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBDecl *spec = bridge->RequireGuid(guid);
    bridge->Release();
    return spec;
}

BBConfig *BBBindByName(const CKBehaviorContext &ctx, const std::string &query) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBConfig *binding = bridge->Bind(query);
    bridge->Release();
    return binding;
}

BBConfig *BBBindByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    BBBridge *bridge = BBFromContext(ctx);
    if (!bridge) {
        return nullptr;
    }
    BBConfig *binding = bridge->BindGuid(guid);
    bridge->Release();
    return binding;
}

ParamValue *ParamInt(int value) { return new ParamValue(MakeScriptParamInt(value)); }
ParamValue *ParamFloat(float value) { return new ParamValue(MakeScriptParamFloat(value)); }
ParamValue *ParamBool(bool value) { return new ParamValue(MakeScriptParamBool(value)); }
ParamValue *ParamString(const std::string &value) { return new ParamValue(MakeScriptParamString(value)); }
ParamValue *ParamGuid(CKGUID value) { return new ParamValue(MakeScriptParamGuid(value)); }
ParamValue *ParamVector(const VxVector &value) { return new ParamValue(MakeScriptParamVector(value)); }
ParamValue *ParamVector2(const Vx2DVector &value) { return new ParamValue(MakeScriptParamVector2(value)); }
ParamValue *ParamColor(const VxColor &value) { return new ParamValue(MakeScriptParamColor(value)); }
ParamValue *ParamQuaternion(const VxQuaternion &value) { return new ParamValue(MakeScriptParamQuaternion(value)); }
ParamValue *ParamMatrix(const VxMatrix &value) { return new ParamValue(MakeScriptParamMatrix(value)); }
ParamValue *ParamObject(CKObject *value) { return new ParamValue(MakeScriptParamObject(value)); }
ParamValue *ParamObjectArray(const XObjectArray &value) { return new ParamValue(MakeScriptParamObjectArray(value)); }

ParamValue *ParamTextByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &text) {
    CKContext *context = ctx.Context;
    return new ParamValue(MakeScriptParamText(text, ScriptResolveParameterGuid(context, typeName, CKPGUID_STRING), typeName));
}

ParamValue *ParamTextByGuid(const CKBehaviorContext &, CKGUID guid, const std::string &text) {
    return new ParamValue(MakeScriptParamText(text, guid, std::string()));
}

ParamValue *ParamRawByName(const CKBehaviorContext &ctx, const std::string &typeName, NativeBuffer *buffer) {
    CKContext *context = ctx.Context;
    CKGUID guid = ScriptResolveParameterGuid(context, typeName);
    return new ParamValue(MakeScriptParamRaw(guid, typeName, buffer ? buffer->Data() : nullptr, buffer ? static_cast<int>(buffer->Size()) : 0));
}

ParamValue *ParamRawByGuid(const CKBehaviorContext &, CKGUID guid, NativeBuffer *buffer) {
    return new ParamValue(MakeScriptParamRaw(guid, std::string(), buffer ? buffer->Data() : nullptr, buffer ? static_cast<int>(buffer->Size()) : 0));
}

ParamValue *ParamEnumByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &nameOrValue) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    int value = 0;
    std::string error;
    if (!record || !registry->ParseEnumValue(record->Guid, nameOrValue, value, error)) {
        SetScriptException(error.empty() ? fmt::format("Enum type '{}' was not found.", typeName) : error);
        return nullptr;
    }
    return new ParamValue(MakeScriptParamEnum(record->Guid, record->Name, static_cast<CKDWORD>(value)));
}

ParamValue *ParamEnumByGuid(const CKBehaviorContext &ctx, CKGUID guid, const std::string &nameOrValue) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    int value = 0;
    std::string error;
    if (!record || !registry->ParseEnumValue(guid, nameOrValue, value, error)) {
        SetScriptException(error.empty() ? fmt::format("Enum type '{}' was not found.", GuidToString(guid)) : error);
        return nullptr;
    }
    return new ParamValue(MakeScriptParamEnum(record->Guid, record->Name, static_cast<CKDWORD>(value)));
}

ParamValue *ParamEnumIntByName(const CKBehaviorContext &ctx, const std::string &typeName, int value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::EnumLike)) {
        SetScriptException(fmt::format("Enum type '{}' was not found.", typeName));
        return nullptr;
    }
    return new ParamValue(MakeScriptParamEnum(record->Guid, record->Name, static_cast<CKDWORD>(value)));
}

ParamValue *ParamEnumIntByGuid(const CKBehaviorContext &ctx, CKGUID guid, int value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::EnumLike)) {
        SetScriptException(fmt::format("Enum type '{}' was not found.", GuidToString(guid)));
        return nullptr;
    }
    return new ParamValue(MakeScriptParamEnum(record->Guid, record->Name, static_cast<CKDWORD>(value)));
}

ParamValue *ParamFlagsByName(const CKBehaviorContext &ctx, const std::string &typeName, const std::string &namesOrMask) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    CKDWORD value = 0;
    std::string error;
    if (!record || !registry->ParseFlagsValue(record->Guid, namesOrMask, value, error)) {
        SetScriptException(error.empty() ? fmt::format("Flags type '{}' was not found.", typeName) : error);
        return nullptr;
    }
    return new ParamValue(MakeScriptParamFlags(record->Guid, record->Name, value));
}

ParamValue *ParamFlagsByGuid(const CKBehaviorContext &ctx, CKGUID guid, const std::string &namesOrMask) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    CKDWORD value = 0;
    std::string error;
    if (!record || !registry->ParseFlagsValue(guid, namesOrMask, value, error)) {
        SetScriptException(error.empty() ? fmt::format("Flags type '{}' was not found.", GuidToString(guid)) : error);
        return nullptr;
    }
    return new ParamValue(MakeScriptParamFlags(record->Guid, record->Name, value));
}

ParamValue *ParamFlagsMaskByName(const CKBehaviorContext &ctx, const std::string &typeName, CKDWORD value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::FlagsLike)) {
        SetScriptException(fmt::format("Flags type '{}' was not found.", typeName));
        return nullptr;
    }
    return new ParamValue(MakeScriptParamFlags(record->Guid, record->Name, value));
}

ParamValue *ParamFlagsMaskByGuid(const CKBehaviorContext &ctx, CKGUID guid, CKDWORD value) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::FlagsLike)) {
        SetScriptException(fmt::format("Flags type '{}' was not found.", GuidToString(guid)));
        return nullptr;
    }
    return new ParamValue(MakeScriptParamFlags(record->Guid, record->Name, value));
}

ParamStructValue *ParamStructByName(const CKBehaviorContext &ctx, const std::string &typeName) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(typeName) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::StructLike)) {
        SetScriptException(fmt::format("Struct type '{}' was not found.", typeName));
        return nullptr;
    }
    return new ParamStructValue(MakeScriptParamStruct(record->Guid, record->Name));
}

ParamStructValue *ParamStructByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
    ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(ctx.Context);
    const ScriptParamTypeRecord *record = registry ? registry->GetType(guid) : nullptr;
    if (!record || !record->Has(ScriptParamTypeCaps::StructLike)) {
        SetScriptException(fmt::format("Struct type '{}' was not found.", GuidToString(guid)));
        return nullptr;
    }
    return new ParamStructValue(MakeScriptParamStruct(record->Guid, record->Name));
}

ParamOp *ParamOperationByName(const CKBehaviorContext &ctx, const std::string &name) {
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

ParamOp *ParamOperationByGuid(const CKBehaviorContext &ctx, CKGUID guid) {
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
void RegisterRefCountedHandle(asIScriptEngine *engine, const char *typeName) {
    int r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);
    assert(r >= 0);
}

template <typename T>
void RegisterObjectTypeAndRefCount(asIScriptEngine *engine, const char *typeName) {
    int r = engine->RegisterObjectType(typeName, 0, asOBJ_REF);
    assert(r >= 0);
    RegisterRefCountedHandle<T>(engine, typeName);
}

void RegisterParamValueFactories(asIScriptEngine *engine, int &r) {
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
    r = engine->RegisterGlobalFunction("ParamValue@ Enum(const CKBehaviorContext &in ctx, const string &in typeName, const string &in nameOrValue)", asFUNCTION(ParamEnumByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Enum(const CKBehaviorContext &in ctx, CKGUID typeGuid, const string &in nameOrValue)", asFUNCTION(ParamEnumByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Enum(const CKBehaviorContext &in ctx, const string &in typeName, int value)", asFUNCTION(ParamEnumIntByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Enum(const CKBehaviorContext &in ctx, CKGUID typeGuid, int value)", asFUNCTION(ParamEnumIntByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Flags(const CKBehaviorContext &in ctx, const string &in typeName, const string &in namesOrMask)", asFUNCTION(ParamFlagsByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Flags(const CKBehaviorContext &in ctx, CKGUID typeGuid, const string &in namesOrMask)", asFUNCTION(ParamFlagsByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Flags(const CKBehaviorContext &in ctx, const string &in typeName, uint mask)", asFUNCTION(ParamFlagsMaskByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamValue@ Flags(const CKBehaviorContext &in ctx, CKGUID typeGuid, uint mask)", asFUNCTION(ParamFlagsMaskByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamStructValue@ Struct(const CKBehaviorContext &in ctx, const string &in typeName)", asFUNCTION(ParamStructByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamStructValue@ Struct(const CKBehaviorContext &in ctx, CKGUID typeGuid)", asFUNCTION(ParamStructByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamOp@ Operation(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(ParamOperationByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("ParamOp@ Operation(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(ParamOperationByGuid), asCALL_CDECL); assert(r >= 0);
}

void RegisterParamKindEnum(asIScriptEngine *engine, int &r) {
    r = engine->RegisterEnum("ParamKind"); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Input", static_cast<int>(ScriptBridgeSlotKind::Input)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Output", static_cast<int>(ScriptBridgeSlotKind::Output)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Pin", static_cast<int>(ScriptBridgeSlotKind::Pin)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Pout", static_cast<int>(ScriptBridgeSlotKind::Pout)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Setting", static_cast<int>(ScriptBridgeSlotKind::Setting)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Local", static_cast<int>(ScriptBridgeSlotKind::Local)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "OperationIn", static_cast<int>(ScriptBridgeSlotKind::OperationIn)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "OperationOut", static_cast<int>(ScriptBridgeSlotKind::OperationOut)); assert(r >= 0);
    r = engine->RegisterEnumValue("ParamKind", "Standalone", static_cast<int>(ScriptBridgeSlotKind::Standalone)); assert(r >= 0);
}

void RegisterBridgeObjectTypes(asIScriptEngine *engine) {
    RegisterObjectTypeAndRefCount<ParamInfo>(engine, "ParamInfo");
    RegisterObjectTypeAndRefCount<ParamValue>(engine, "ParamValue");
    RegisterObjectTypeAndRefCount<ParamStructValue>(engine, "ParamStructValue");
    RegisterObjectTypeAndRefCount<ParamRef>(engine, "ParamRef");
    RegisterObjectTypeAndRefCount<ParamSourceLinkRef>(engine, "ParamSourceLinkRef");
    RegisterObjectTypeAndRefCount<ParamStructRef>(engine, "ParamStructRef");
    RegisterObjectTypeAndRefCount<ParamOp>(engine, "ParamOp");
    RegisterObjectTypeAndRefCount<ParamOperationRef>(engine, "ParamOperationRef");
    RegisterObjectTypeAndRefCount<BehaviorLayout>(engine, "BehaviorLayout");
    RegisterObjectTypeAndRefCount<BehaviorLayout>(engine, "BBLayout");
    RegisterObjectTypeAndRefCount<BehaviorRef>(engine, "BehaviorRef");
    RegisterObjectTypeAndRefCount<BehaviorBridge>(engine, "BehaviorBridge");
    RegisterObjectTypeAndRefCount<BBSlot>(engine, "BBSlot");
    RegisterObjectTypeAndRefCount<BBDecl>(engine, "BBDecl");
    RegisterObjectTypeAndRefCount<BBConfig>(engine, "BBConfig");
    RegisterObjectTypeAndRefCount<BBInstance>(engine, "BBInstance");
    RegisterObjectTypeAndRefCount<BBPrototype>(engine, "BBPrototype");
    RegisterObjectTypeAndRefCount<BBBridge>(engine, "BBBridge");
    RegisterObjectTypeAndRefCount<BBCallBuilder>(engine, "BBCallBuilder");
    RegisterObjectTypeAndRefCount<BBTaskBuilder>(engine, "BBTaskBuilder");
    RegisterObjectTypeAndRefCount<BBResult>(engine, "BBResult");
    RegisterObjectTypeAndRefCount<BBTask>(engine, "BBTask");
    RegisterObjectTypeAndRefCount<GraphTask>(engine, "GraphTask");
}

void RegisterParamHandleMethods(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectMethod("ParamInfo", "ParamKind get_kind() const", asMETHOD(ParamInfo, GetKind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "int get_index() const", asMETHOD(ParamInfo, GetIndex), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string get_name() const", asMETHOD(ParamInfo, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "CKGUID TypeGuid() const", asMETHOD(ParamInfo, GetTypeGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string TypeName() const", asMETHOD(ParamInfo, GetTypeName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "int DataSize() const", asMETHOD(ParamInfo, GetDataSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamInfo", "string Describe() const", asMETHOD(ParamInfo, Describe), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("ParamValue", "ParamStructValue@ AsStruct() const", asMETHOD(ParamValue, AsStruct), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("ParamStructValue", "bool IsValid() const", asMETHOD(ParamStructValue, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "CKGUID TypeGuid() const", asMETHOD(ParamStructValue, TypeGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "string TypeName() const", asMETHOD(ParamStructValue, TypeName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "ParamStructValue@ Set(int index, ParamValue@ value)", asMETHOD(ParamStructValue, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "ParamValue@ Value() const", asMETHOD(ParamStructValue, Value), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "ParamValue@ AsValue() const", asMETHOD(ParamStructValue, AsValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructValue", "string Describe() const", asMETHOD(ParamStructValue, Describe), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("ParamRef", "ParamSourceLinkRef@ SetSourceScoped(ParamRef@ source)", asMETHOD(ParamRef, SetSourceScoped), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetSource(ParamRef@ source)", asMETHOD(ParamRef, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool Set(ParamValue@ value)", asMETHOD(ParamRef, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetInt(int value)", asMETHOD(ParamRef, SetInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetFloat(float value)", asMETHOD(ParamRef, SetFloat), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetBool(bool value)", asMETHOD(ParamRef, SetBool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetString(const string &in value)", asMETHOD(ParamRef, SetString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetObject(CKObject@ value)", asMETHOD(ParamRef, SetObject), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetEnum(const string &in nameOrValue)", asMETHOD(ParamRef, SetEnum), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetEnum(int value)", asMETHOD(ParamRef, SetEnumInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetFlags(const string &in namesOrMask)", asMETHOD(ParamRef, SetFlags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetFlags(uint mask)", asMETHOD(ParamRef, SetFlagsMask), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetStruct(ParamStructValue@ value)", asMETHOD(ParamRef, SetStruct), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamValue@ Get() const", asMETHOD(ParamRef, GetValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool CopyFrom(ParamRef@ source)", asMETHOD(ParamRef, CopyFrom), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string GetText() const", asMETHOD(ParamRef, GetText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string GetEnumText() const", asMETHOD(ParamRef, GetEnumText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string GetFlagsText() const", asMETHOD(ParamRef, GetFlagsText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetText(const string &in text)", asMETHOD(ParamRef, SetText), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "NativeBuffer@ GetRaw() const", asMETHOD(ParamRef, GetRaw), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "bool SetRaw(NativeBuffer@ data)", asMETHOD(ParamRef, SetRaw), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "ParamStructRef@ Struct()", asMETHOD(ParamRef, Struct), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamRef", "string Describe() const", asMETHOD(ParamRef, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "bool IsValid() const", asMETHOD(ParamSourceLinkRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "bool IsCommitted() const", asMETHOD(ParamSourceLinkRef, IsCommitted), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "bool IsRestored() const", asMETHOD(ParamSourceLinkRef, IsRestored), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "bool Commit()", asMETHOD(ParamSourceLinkRef, Commit), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "bool Restore()", asMETHOD(ParamSourceLinkRef, Restore), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamSourceLinkRef", "string Describe() const", asMETHOD(ParamSourceLinkRef, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("ParamStructRef", "bool IsValid() const", asMETHOD(ParamStructRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructRef", "int Count() const", asMETHOD(ParamStructRef, Count), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructRef", "ParamStructInfo@ Info() const", asMETHOD(ParamStructRef, Info), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructRef", "ParamRef@ Member(int index) const", asMETHOD(ParamStructRef, Member), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructRef", "int FindMember(const string &in name, int occurrence = 0) const", asMETHOD(ParamStructRef, FindMember), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamStructRef", "string Describe() const", asMETHOD(ParamStructRef, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ Result(CKGUID guid)", asMETHODPR(ParamOp, Result, (CKGUID), ParamOp *), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ Result(const string &in typeName)", asMETHOD(ParamOp, ResultName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ In(int slot, ParamRef@ source)", asMETHOD(ParamOp, InRef), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "ParamOp@ In(int slot, ParamValue@ value)", asMETHOD(ParamOp, InValue), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOp", "string Describe() const", asMETHOD(ParamOp, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("ParamOperationRef", "bool IsValid() const", asMETHOD(ParamOperationRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ Out() const", asMETHOD(ParamOperationRef, Out), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ In1() const", asMETHOD(ParamOperationRef, In1), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "ParamRef@ In2() const", asMETHOD(ParamOperationRef, In2), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "CKERROR Do() const", asMETHOD(ParamOperationRef, Do), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "bool Restore()", asMETHOD(ParamOperationRef, Restore), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "bool Destroy()", asMETHOD(ParamOperationRef, Destroy), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("ParamOperationRef", "string Describe() const", asMETHOD(ParamOperationRef, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterLayoutMethods(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectMethod("BehaviorLayout", "int InputCount() const", asMETHOD(BehaviorLayout, InputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int OutputCount() const", asMETHOD(BehaviorLayout, OutputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int PinCount() const", asMETHOD(BehaviorLayout, PinCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int PoutCount() const", asMETHOD(BehaviorLayout, PoutCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int SettingCount() const", asMETHOD(BehaviorLayout, SettingCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int LocalCount() const", asMETHOD(BehaviorLayout, LocalCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string InputName(int index) const", asMETHOD(BehaviorLayout, InputName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string OutputName(int index) const", asMETHOD(BehaviorLayout, OutputNameAt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Pin(int index) const", asMETHOD(BehaviorLayout, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Pout(int index) const", asMETHOD(BehaviorLayout, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Setting(int index) const", asMETHOD(BehaviorLayout, Setting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "ParamInfo@ Local(int index) const", asMETHOD(BehaviorLayout, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindInput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindInput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindOutput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindOutput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindPin(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindPout(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindSetting(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindSetting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "int FindLocal(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindLocal), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorLayout", "string Describe() const", asMETHOD(BehaviorLayout, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBLayout", "int InputCount() const", asMETHOD(BehaviorLayout, InputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int OutputCount() const", asMETHOD(BehaviorLayout, OutputCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int PinCount() const", asMETHOD(BehaviorLayout, PinCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int PoutCount() const", asMETHOD(BehaviorLayout, PoutCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int SettingCount() const", asMETHOD(BehaviorLayout, SettingCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int LocalCount() const", asMETHOD(BehaviorLayout, LocalCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "string InputName(int index) const", asMETHOD(BehaviorLayout, InputName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "string OutputName(int index) const", asMETHOD(BehaviorLayout, OutputNameAt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "ParamInfo@ Pin(int index) const", asMETHOD(BehaviorLayout, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "ParamInfo@ Pout(int index) const", asMETHOD(BehaviorLayout, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "ParamInfo@ Setting(int index) const", asMETHOD(BehaviorLayout, Setting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "ParamInfo@ Local(int index) const", asMETHOD(BehaviorLayout, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindInput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindInput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindOutput(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindOutput), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindPin(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindPout(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindPout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindSetting(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindSetting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "int FindLocal(const string &in name, int occurrence = 0) const", asMETHOD(BehaviorLayout, FindLocal), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBLayout", "string Describe() const", asMETHOD(BehaviorLayout, Describe), asCALL_THISCALL); assert(r >= 0);
}

void RegisterBehaviorMethods(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectMethod("BehaviorRef", "bool IsValid() const", asMETHOD(BehaviorRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool get_valid() const", asMETHOD(BehaviorRef, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "CK_ID get_id() const", asMETHOD(BehaviorRef, GetID), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "string get_name() const", asMETHOD(BehaviorRef, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "BehaviorLayout@ Layout() const", asMETHOD(BehaviorRef, Layout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool Trigger(int inputIndex = 0, bool reset = false)", asMETHOD(BehaviorRef, Trigger), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool Trigger(BBSlot@ input, bool reset = false)", asMETHOD(BehaviorRef, TriggerSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool InputActive(int inputIndex) const", asMETHOD(BehaviorRef, InputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool InputActive(BBSlot@ input) const", asMETHOD(BehaviorRef, InputActiveSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool OutputActive(int outputIndex) const", asMETHOD(BehaviorRef, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "bool OutputActive(BBSlot@ output) const", asMETHOD(BehaviorRef, OutputActiveSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pin(int index) const", asMETHOD(BehaviorRef, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pin(BBSlot@ slot) const", asMETHOD(BehaviorRef, PinSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pout(int index) const", asMETHOD(BehaviorRef, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Pout(BBSlot@ slot) const", asMETHOD(BehaviorRef, PoutSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Local(int index) const", asMETHOD(BehaviorRef, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamRef@ Local(BBSlot@ slot) const", asMETHOD(BehaviorRef, LocalSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "GraphTask@ Start(int inputIndex = 0, bool reset = false, float timeout = 0.0f)", asMETHOD(BehaviorRef, Start), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "GraphTask@ Start(BBSlot@ input, bool reset = false, float timeout = 0.0f)", asMETHOD(BehaviorRef, StartSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "GraphTask@ Watch(float timeout = 0.0f)", asMETHOD(BehaviorRef, Watch), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamOperationRef@ ConnectOperation(int pinIndex, ParamOp@ op)", asMETHOD(BehaviorRef, ConnectOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "ParamOperationRef@ ConnectOperation(BBSlot@ pin, ParamOp@ op)", asMETHOD(BehaviorRef, ConnectOperationSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorRef", "string Describe() const", asMETHOD(BehaviorRef, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ Self() const", asMETHOD(BehaviorBridge, Self), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ OwnerScript() const", asMETHOD(BehaviorBridge, OwnerScript), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ Find(const string &in name) const", asMETHOD(BehaviorBridge, Find), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ FindOn(CKBeObject@ owner, const string &in name) const", asMETHOD(BehaviorBridge, FindOn), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BehaviorBridge", "BehaviorRef@ FindByID(CK_ID id) const", asMETHOD(BehaviorBridge, FindByID), asCALL_THISCALL); assert(r >= 0);
}

void RegisterBBMethods(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectMethod("BBSlot", "bool IsValid() const", asMETHOD(BBSlot, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "bool get_valid() const", asMETHOD(BBSlot, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "string Error() const", asMETHOD(BBSlot, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "ParamKind Kind() const", asMETHOD(BBSlot, Kind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "int Index() const", asMETHOD(BBSlot, Index), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "string Name() const", asMETHOD(BBSlot, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "CKGUID TypeGuid() const", asMETHOD(BBSlot, TypeGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "string TypeName() const", asMETHOD(BBSlot, TypeName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "int DataSize() const", asMETHOD(BBSlot, DataSize), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "uint Caps() const", asMETHOD(BBSlot, Caps), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "int LayoutGeneration() const", asMETHOD(BBSlot, LayoutGeneration), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "bool IsSetting() const", asMETHOD(BBSlot, IsSetting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBSlot", "string Describe() const", asMETHOD(BBSlot, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBDecl", "bool IsValid() const", asMETHOD(BBDecl, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "bool get_valid() const", asMETHOD(BBDecl, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "string Error() const", asMETHOD(BBDecl, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "CKGUID Guid() const", asMETHOD(BBDecl, Guid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "string Name() const", asMETHOD(BBDecl, Name), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "string Category() const", asMETHOD(BBDecl, Category), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "string QualifiedName() const", asMETHOD(BBDecl, QualifiedName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "uint BehaviorFlags() const", asMETHOD(BBDecl, BehaviorFlags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "uint PrototypeFlags() const", asMETHOD(BBDecl, PrototypeFlags), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "CK_CLASSID CompatibleClassId() const", asMETHOD(BBDecl, CompatibleClassId), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "int NeededManagerCount() const", asMETHOD(BBDecl, NeededManagerCount), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "CKGUID NeededManagerGuid(int index) const", asMETHOD(BBDecl, NeededManagerGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBLayout@ Layout() const", asMETHOD(BBDecl, Layout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Input(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Input), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Output(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Output), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Pin(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Pout(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Setting(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Setting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBSlot@ Local(const string &in name, int occurrence = 0) const", asMETHOD(BBDecl, Local), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "BBConfig@ Configure()", asMETHOD(BBDecl, Configure), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBDecl", "string Describe() const", asMETHOD(BBDecl, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBConfig", "bool IsValid() const", asMETHOD(BBConfig, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "bool get_valid() const", asMETHOD(BBConfig, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "string Error() const", asMETHOD(BBConfig, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "string Describe() const", asMETHOD(BBConfig, Describe), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBDecl@ Decl() const", asMETHOD(BBConfig, Decl), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBConfig, Raise), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Owner(CKBeObject@ owner)", asMETHOD(BBConfig, Owner), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Target(CKBeObject@ target)", asMETHOD(BBConfig, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, ParamValue@ value)", asMETHOD(BBConfig, SetSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, int value)", asMETHOD(BBConfig, SetSlotInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, float value)", asMETHOD(BBConfig, SetSlotFloat), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, bool value)", asMETHOD(BBConfig, SetSlotBool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, const string &in value)", asMETHOD(BBConfig, SetSlotString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Set(BBSlot@ pin, CKObject@ value)", asMETHOD(BBConfig, SetSlotObject), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ SetSetting(BBSlot@ setting, ParamValue@ value)", asMETHOD(BBConfig, SetSetting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ SetSetting(BBSlot@ setting, const string &in value)", asMETHOD(BBConfig, SetSettingString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Source(BBSlot@ pin, ParamRef@ source)", asMETHOD(BBConfig, SourceSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBConfig@ Operation(BBSlot@ pin, ParamOp@ operation)", asMETHOD(BBConfig, OperationSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "bool Validate(const CKBehaviorContext &in ctx) const", asMETHOD(BBConfig, Validate), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBConfig", "BBInstance@ Spawn(const CKBehaviorContext &in ctx)", asMETHOD(BBConfig, SpawnInstance), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBInstance", "bool IsValid() const", asMETHOD(BBInstance, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool get_valid() const", asMETHOD(BBInstance, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "string Error() const", asMETHOD(BBInstance, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "BBDecl@ Decl() const", asMETHOD(BBInstance, Decl), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "BehaviorRef@ Behavior() const", asMETHOD(BBInstance, Behavior), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool Start(BBSlot@ input)", asMETHOD(BBInstance, Start), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool Step(const CKBehaviorContext &in ctx)", asMETHOD(BBInstance, Step), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool Stop()", asMETHOD(BBInstance, Stop), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool OutputActive(BBSlot@ output) const", asMETHOD(BBInstance, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "ParamRef@ Pin(BBSlot@ pin) const", asMETHOD(BBInstance, Pin), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "ParamRef@ Pout(BBSlot@ pout) const", asMETHOD(BBInstance, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool SetSetting(BBSlot@ setting, const string &in value)", asMETHOD(BBInstance, SetSetting), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool Destroy()", asMETHOD(BBInstance, Destroy), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBInstance", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBInstance, Raise), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBPrototype", "bool IsValid() const", asMETHOD(BBPrototype, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "CKGUID GetGuid() const", asMETHOD(BBPrototype, GetGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string GetName() const", asMETHOD(BBPrototype, GetName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string GetCategory() const", asMETHOD(BBPrototype, GetCategory), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string GetQualifiedName() const", asMETHOD(BBPrototype, GetQualifiedName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BehaviorLayout@ Layout() const", asMETHOD(BBPrototype, Layout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BBCallBuilder@ Call()", asMETHOD(BBPrototype, Call), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "BBTaskBuilder@ Spawn()", asMETHOD(BBPrototype, Spawn), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBPrototype", "string Describe() const", asMETHOD(BBPrototype, Describe), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ Prototype(const string &in name) const", asMETHOD(BBBridge, PrototypeByName), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ Prototype(CKGUID guid) const", asMETHOD(BBBridge, PrototypeByGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "int Count() const", asMETHOD(BBBridge, Count), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ At(int index) const", asMETHOD(BBBridge, At), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBPrototype@ Find(const string &in query, int occurrence = 0) const", asMETHOD(BBBridge, Find), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "array<BBPrototype@>@ FindAll(const string &in query = \"\") const", asMETHOD(BBBridge, FindAll), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBDecl@ Require(const string &in query) const", asMETHOD(BBBridge, Require), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBDecl@ Require(CKGUID guid) const", asMETHOD(BBBridge, RequireGuid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBConfig@ Bind(const string &in query) const", asMETHOD(BBBridge, Bind), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBBridge", "BBConfig@ Bind(CKGUID guid) const", asMETHOD(BBBridge, BindGuid), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Owner(CKBeObject@ owner)", asMETHOD(BBCallBuilder, Owner), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Target(CKBeObject@ target)", asMETHOD(BBCallBuilder, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(int pinIndex, ParamValue@ value)", asMETHOD(BBCallBuilder, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, ParamValue@ value)", asMETHOD(BBCallBuilder, SetSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, int value)", asMETHOD(BBCallBuilder, SetSlotInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, float value)", asMETHOD(BBCallBuilder, SetSlotFloat), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, bool value)", asMETHOD(BBCallBuilder, SetSlotBool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, const string &in value)", asMETHOD(BBCallBuilder, SetSlotString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Set(BBSlot@ pin, CKObject@ value)", asMETHOD(BBCallBuilder, SetSlotObject), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ SetSource(int pinIndex, ParamRef@ source)", asMETHOD(BBCallBuilder, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Source(BBSlot@ pin, ParamRef@ source)", asMETHOD(BBCallBuilder, Source), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ SetOperation(int pinIndex, ParamOp@ operation)", asMETHOD(BBCallBuilder, SetOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBCallBuilder@ Operation(BBSlot@ pin, ParamOp@ operation)", asMETHOD(BBCallBuilder, Operation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBResult@ Run(int inputIndex = 0)", asMETHOD(BBCallBuilder, Run), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBCallBuilder", "BBResult@ Run(BBSlot@ input)", asMETHOD(BBCallBuilder, RunSlot), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Owner(CKBeObject@ owner)", asMETHOD(BBTaskBuilder, Owner), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Target(CKBeObject@ target)", asMETHOD(BBTaskBuilder, Target), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(int pinIndex, ParamValue@ value)", asMETHOD(BBTaskBuilder, Set), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, ParamValue@ value)", asMETHOD(BBTaskBuilder, SetSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, int value)", asMETHOD(BBTaskBuilder, SetSlotInt), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, float value)", asMETHOD(BBTaskBuilder, SetSlotFloat), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, bool value)", asMETHOD(BBTaskBuilder, SetSlotBool), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, const string &in value)", asMETHOD(BBTaskBuilder, SetSlotString), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Set(BBSlot@ pin, CKObject@ value)", asMETHOD(BBTaskBuilder, SetSlotObject), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ SetSource(int pinIndex, ParamRef@ source)", asMETHOD(BBTaskBuilder, SetSource), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Source(BBSlot@ pin, ParamRef@ source)", asMETHOD(BBTaskBuilder, Source), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ SetOperation(int pinIndex, ParamOp@ operation)", asMETHOD(BBTaskBuilder, SetOperation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTaskBuilder@ Operation(BBSlot@ pin, ParamOp@ operation)", asMETHOD(BBTaskBuilder, Operation), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTask@ Start(int inputIndex = 0)", asMETHOD(BBTaskBuilder, Start), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTaskBuilder", "BBTask@ Start(BBSlot@ input)", asMETHOD(BBTaskBuilder, StartSlot), asCALL_THISCALL); assert(r >= 0);
}

void RegisterTaskMethods(asIScriptEngine *engine, int &r) {
    r = engine->RegisterObjectMethod("BBResult", "bool Ok() const", asMETHOD(BBResult, Ok), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool get_ok() const", asMETHOD(BBResult, Ok), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "int ReturnCode() const", asMETHOD(BBResult, ReturnCode), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "string Error() const", asMETHOD(BBResult, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool OutputActive(int outputIndex) const", asMETHOD(BBResult, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool OutputActive(BBSlot@ output) const", asMETHOD(BBResult, OutputActiveSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "ParamRef@ Pout(int index) const", asMETHOD(BBResult, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "ParamRef@ Pout(BBSlot@ pout) const", asMETHOD(BBResult, PoutSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBResult", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBResult, Raise), asCALL_THISCALL); assert(r >= 0);

    r = engine->RegisterObjectMethod("BBTask", "bool IsValid() const", asMETHOD(BBTask, IsValid), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool IsAlive() const", asMETHOD(BBTask, IsAlive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool IsPaused() const", asMETHOD(BBTask, IsPaused), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "int ReturnCode() const", asMETHOD(BBTask, ReturnCode), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "string Error() const", asMETHOD(BBTask, Error), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool OutputActive(int outputIndex) const", asMETHOD(BBTask, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool OutputActive(BBSlot@ output) const", asMETHOD(BBTask, OutputActiveSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Step(const CKBehaviorContext &in ctx, int inputIndex = -1)", asMETHOD(BBTask, Step), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Step(const CKBehaviorContext &in ctx, BBSlot@ input)", asMETHOD(BBTask, StepSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Reset()", asMETHOD(BBTask, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Destroy()", asMETHOD(BBTask, Destroy), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "BehaviorRef@ Behavior() const", asMETHOD(BBTask, Behavior), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "ParamRef@ Pout(int index) const", asMETHOD(BBTask, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "ParamRef@ Pout(BBSlot@ pout) const", asMETHOD(BBTask, PoutSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("BBTask", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(BBTask, Raise), asCALL_THISCALL); assert(r >= 0);

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
    r = engine->RegisterObjectMethod("GraphTask", "bool Done(BBSlot@ output) const", asMETHOD(GraphTask, DoneSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool OutputActive(int outputIndex = -1) const", asMETHOD(GraphTask, OutputActive), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool OutputActive(BBSlot@ output) const", asMETHOD(GraphTask, OutputActiveSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Cancel()", asMETHOD(GraphTask, Cancel), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Reset()", asMETHOD(GraphTask, Reset), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "BehaviorRef@ Behavior() const", asMETHOD(GraphTask, Behavior), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "ParamRef@ Pout(int index) const", asMETHOD(GraphTask, Pout), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "ParamRef@ Pout(BBSlot@ pout) const", asMETHOD(GraphTask, PoutSlot), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("GraphTask", "bool Raise(const CKBehaviorContext &in ctx) const", asMETHOD(GraphTask, Raise), asCALL_THISCALL); assert(r >= 0);
}

void RegisterBridgeNamespaces(asIScriptEngine *engine, int &r) {
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
    r = engine->RegisterGlobalFunction("int Count(const CKBehaviorContext &in ctx)", asFUNCTION(BBCount), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ At(const CKBehaviorContext &in ctx, int index)", asFUNCTION(BBAt), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBPrototype@ Find(const CKBehaviorContext &in ctx, const string &in query, int occurrence = 0)", asFUNCTION(BBFind), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("array<BBPrototype@>@ FindAll(const CKBehaviorContext &in ctx, const string &in query = \"\")", asFUNCTION(BBFindAll), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const CKBehaviorContext &in ctx, const string &in query)", asFUNCTION(BBRequireByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBDecl@ Require(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(BBRequireByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBConfig@ Bind(const CKBehaviorContext &in ctx, const string &in query)", asFUNCTION(BBBindByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBConfig@ Bind(const CKBehaviorContext &in ctx, CKGUID guid)", asFUNCTION(BBBindByGuid), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBCallBuilder@ Call(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BBCallByName), asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction("BBTaskBuilder@ Spawn(const CKBehaviorContext &in ctx, const string &in name)", asFUNCTION(BBSpawnByName), asCALL_CDECL); assert(r >= 0);

    r = engine->SetDefaultNamespace("Param"); assert(r >= 0);
    RegisterParamValueFactories(engine, r);

    r = engine->SetDefaultNamespace(previous.c_str()); assert(r >= 0);
}

void RegisterScriptBehaviorBridgeInternal(asIScriptEngine *engine) {
    assert(engine != nullptr);
    int r = 0;

    RegisterParamKindEnum(engine, r);
    RegisterBridgeObjectTypes(engine);
    RegisterParamHandleMethods(engine, r);
    RegisterLayoutMethods(engine, r);
    RegisterBehaviorMethods(engine, r);
    RegisterBBMethods(engine, r);
    RegisterTaskMethods(engine, r);
    RegisterBridgeNamespaces(engine, r);
}

} // namespace ScriptBridgeRegistrationInternal

void RegisterScriptBehaviorBridge(asIScriptEngine *engine) {
    ScriptBridgeRegistrationInternal::RegisterScriptBehaviorBridgeInternal(engine);
}
