//////////////////////////////////
//////////////////////////////////
//
//     Angel Script Component
//
//////////////////////////////////
//////////////////////////////////
#include "CKAll.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ScriptManager.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptParameterConversion.h"
#include "ScriptRunner.h"

CKObjectDeclaration *FillBehaviorAngelScriptComponentDecl();
CKERROR CreateAngelScriptComponentProto(CKBehaviorPrototype **pproto);
int AngelScriptComponent(const CKBehaviorContext &behcontext);
CKERROR AngelScriptComponentCallBack(const CKBehaviorContext &behcontext);

namespace AngelScriptComponentInternal {

constexpr int COMPONENT_STATE = 0;
constexpr int OUTPUT_ERROR_MESSAGE = 1;
constexpr int SCRIPT_PARAM = 0;
constexpr int CLASS_PARAM = 1;
constexpr int SOURCE_PARAM = 2;
constexpr int FILE_PARAM = 3;
constexpr int MANIFEST_PARAM = 4;
constexpr int FIXED_INPUT_PARAMETER_COUNT = 5;

std::string ReadStringParameter(CKBehavior *beh, int index) {
    if (!beh) {
        return {};
    }

    CKSTRING value = (CKSTRING) beh->GetInputParameterReadDataPtr(index);
    return value ? std::string(value) : std::string();
}

std::string TrimString(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) {
        return {};
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool NameEquals(CKSTRING actual, const std::string &expected) {
    return actual && actual[0] != '\0' && actual == expected;
}

std::string StripQuotes(const std::string &value) {
    std::string text = TrimString(value);
    if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        text = text.substr(1, text.size() - 2);
    }
    return text;
}

bool ParseBoolText(const std::string &value, bool fallback = false) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "true" || text == "yes" || text == "on" || text == "1") {
        return true;
    }
    if (text == "false" || text == "no" || text == "off" || text == "0") {
        return false;
    }
    return fallback;
}

std::vector<std::string> TokenizeArguments(const std::string &args) {
    std::vector<std::string> tokens;
    std::string current;
    char quote = '\0';

    auto pushCurrent = [&]() {
        std::string token = TrimString(current);
        if (!token.empty()) {
            tokens.push_back(token);
        }
        current.clear();
    };

    for (char c : args) {
        if (quote != '\0') {
            current.push_back(c);
            if (c == quote) {
                quote = '\0';
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            quote = c;
            current.push_back(c);
            continue;
        }

        if (c == '=' || c == ',' || c == ';' || std::isspace(static_cast<unsigned char>(c))) {
            pushCurrent();
            if (c == '=') {
                tokens.emplace_back("=");
            }
            continue;
        }

        current.push_back(c);
    }

    pushCurrent();
    return tokens;
}

bool IsComponentBindingKeyword(const std::string &keyword) {
    const std::string key = ToLower(keyword);
    return key == "ckinput" ||
           key == "ckparam" ||
           key == "ckcomponentinput" ||
           key == "input" ||
           key == "param" ||
           key == "property" ||
           key == "field" ||
           key == "behavior" ||
           key == "bb" ||
           key == "bbslot" ||
           key == "bbdecl" ||
           key == "bbconfig" ||
           key == "bbinput" ||
           key == "bbin" ||
           key == "bboutput" ||
           key == "bbout" ||
           key == "bbpin" ||
           key == "bbpout" ||
           key == "bbsetting" ||
           key == "bblocal" ||
           key == "bbsource";
}

ScriptParamValueKind ValueKindFromComponentKind(ScriptComponentBindingKind kind) {
    switch (kind) {
        case ScriptComponentBindingKind::Int: return ScriptParamValueKind::Int;
        case ScriptComponentBindingKind::Float: return ScriptParamValueKind::Float;
        case ScriptComponentBindingKind::Bool: return ScriptParamValueKind::Bool;
        case ScriptComponentBindingKind::String: return ScriptParamValueKind::String;
        case ScriptComponentBindingKind::Guid: return ScriptParamValueKind::Guid;
        case ScriptComponentBindingKind::Vector: return ScriptParamValueKind::Vector;
        case ScriptComponentBindingKind::Vector2: return ScriptParamValueKind::Vector2;
        case ScriptComponentBindingKind::Color: return ScriptParamValueKind::Color;
        case ScriptComponentBindingKind::Quaternion: return ScriptParamValueKind::Quaternion;
        case ScriptComponentBindingKind::Matrix: return ScriptParamValueKind::Matrix;
        case ScriptComponentBindingKind::ObjectArray: return ScriptParamValueKind::ObjectArray;
        default: return ScriptParamValueKind::Empty;
    }
}

ScriptComponentBindingKind ComponentKindFromValueKind(ScriptParamValueKind kind) {
    switch (kind) {
        case ScriptParamValueKind::Int: return ScriptComponentBindingKind::Int;
        case ScriptParamValueKind::Float: return ScriptComponentBindingKind::Float;
        case ScriptParamValueKind::Bool: return ScriptComponentBindingKind::Bool;
        case ScriptParamValueKind::String: return ScriptComponentBindingKind::String;
        case ScriptParamValueKind::Guid: return ScriptComponentBindingKind::Guid;
        case ScriptParamValueKind::Vector: return ScriptComponentBindingKind::Vector;
        case ScriptParamValueKind::Vector2: return ScriptComponentBindingKind::Vector2;
        case ScriptParamValueKind::Color: return ScriptComponentBindingKind::Color;
        case ScriptParamValueKind::Quaternion: return ScriptComponentBindingKind::Quaternion;
        case ScriptParamValueKind::Matrix: return ScriptComponentBindingKind::Matrix;
        case ScriptParamValueKind::ObjectArray: return ScriptComponentBindingKind::ObjectArray;
        default: return ScriptComponentBindingKind::Auto;
    }
}

ScriptComponentBindingKind KindFromTypeName(const std::string &typeName) {
    const std::string type = ToLower(StripQuotes(typeName));
    if (type.empty() || type == "auto") {
        return ScriptComponentBindingKind::Auto;
    }
    const ScriptParamValueKind valueKind = ScriptParamValueKindFromTypeName(type);
    if (valueKind != ScriptParamValueKind::Empty) {
        return ComponentKindFromValueKind(valueKind);
    }
    if (type == "behaviorref") {
        return ScriptComponentBindingKind::BehaviorRef;
    }
    if (type == "paramref") {
        return ScriptComponentBindingKind::ParamRef;
    }
    if (type == "paramvalue" || type == "value") {
        return ScriptComponentBindingKind::ParamValue;
    }
    if (type == "paramtype" || type == "paramtypeinfo" || type == "typeinfo") {
        return ScriptComponentBindingKind::ParamTypeInfo;
    }
    if (type == "bb" || type == "bbprototype" || type == "buildingblock") {
        return ScriptComponentBindingKind::BBPrototype;
    }
    if (type == "bbdecl" || type == "BBDecl" || type == "buildingblockdecl") {
        return ScriptComponentBindingKind::BBDecl;
    }
    if (type == "bbslot" || type == "buildingblockslot") {
        return ScriptComponentBindingKind::BBSlot;
    }
    if (type == "bbconfig" || type == "BBConfig" || type == "buildingblockconfig") {
        return ScriptComponentBindingKind::BBConfig;
    }
    if (type == "object" || type == "ckobject" || type == "ckbeobject" || type == "behavior" ||
        type == "ckbehavior" || type == "3dentity" || type == "ck3dentity" || type == "2dentity" ||
        type == "ck2dentity" || type == "camera" || type == "ckcamera" || type == "light" ||
        type == "cklight" || type == "material" || type == "ckmaterial" || type == "texture" ||
        type == "cktexture" || type == "mesh" || type == "ckmesh" || type == "group" ||
        type == "ckgroup" || type == "dataarray" || type == "ckdataarray") {
        return ScriptComponentBindingKind::Object;
    }
    return ScriptComponentBindingKind::Auto;
}

CKGUID GuidFromClassId(CKContext *context, CK_CLASSID cid, CKGUID fallback = CKPGUID_OBJECT) {
    CKParameterManager *parameterManager = context ? context->GetParameterManager() : nullptr;
    if (parameterManager && cid != 0) {
        CKGUID guid = parameterManager->ClassIDToGuid(cid);
        if (guid != CKGUID() && parameterManager->GetParameterTypeDescription(guid)) {
            return guid;
        }
    }
    return fallback;
}

CKGUID GuidFromTypeName(CKContext *context, const std::string &typeName, ScriptComponentBindingKind kind) {
    const std::string type = ToLower(StripQuotes(typeName));
    CKGUID registered = ScriptResolveParameterGuid(context, typeName);
    if (registered != CKGUID()) {
        return registered;
    }
    if (kind == ScriptComponentBindingKind::BBPrototype && (type == "behavior" || type == "ckbehavior")) {
        return GuidFromClassId(context, CKCID_BEHAVIOR, CKPGUID_BEHAVIOR);
    }
    const ScriptParamValueKind valueKind = ValueKindFromComponentKind(kind);
    if (valueKind != ScriptParamValueKind::Empty) {
        return ScriptParameterGuidForValueKind(valueKind);
    }
    if (kind == ScriptComponentBindingKind::BBPrototype ||
        kind == ScriptComponentBindingKind::BBDecl ||
        kind == ScriptComponentBindingKind::BBSlot ||
        kind == ScriptComponentBindingKind::BBConfig) {
        return CKPGUID_STRING;
    }
    if (kind == ScriptComponentBindingKind::ParamRef ||
        kind == ScriptComponentBindingKind::ParamValue ||
        kind == ScriptComponentBindingKind::ParamTypeInfo) {
        return CKPGUID_STRING;
    }
    if (kind == ScriptComponentBindingKind::BehaviorRef || type == "behavior" || type == "ckbehavior") {
        return GuidFromClassId(context, CKCID_BEHAVIOR, CKPGUID_BEHAVIOR);
    }

    const std::string rawType = StripQuotes(typeName);
    CK_CLASSID classId = CKStringToClassID(const_cast<CKSTRING>(rawType.c_str()));
    if (classId != 0) {
        return GuidFromClassId(context, classId);
    }

    if (type == "3dentity" || type == "ck3dentity") {
        return GuidFromClassId(context, CKCID_3DENTITY, CKPGUID_3DENTITY);
    }
    if (type == "2dentity" || type == "ck2dentity") {
        return GuidFromClassId(context, CKCID_2DENTITY, CKPGUID_2DENTITY);
    }
    if (type == "camera" || type == "ckcamera") {
        return GuidFromClassId(context, CKCID_CAMERA, CKPGUID_CAMERA);
    }
    if (type == "light" || type == "cklight") {
        return GuidFromClassId(context, CKCID_LIGHT, CKPGUID_LIGHT);
    }
    if (type == "material" || type == "ckmaterial") {
        return GuidFromClassId(context, CKCID_MATERIAL, CKPGUID_MATERIAL);
    }
    if (type == "texture" || type == "cktexture") {
        return GuidFromClassId(context, CKCID_TEXTURE, CKPGUID_TEXTURE);
    }
    if (type == "mesh" || type == "ckmesh") {
        return GuidFromClassId(context, CKCID_MESH, CKPGUID_MESH);
    }
    if (type == "group" || type == "ckgroup") {
        return GuidFromClassId(context, CKCID_GROUP, CKPGUID_GROUP);
    }
    if (type == "dataarray" || type == "ckdataarray") {
        return GuidFromClassId(context, CKCID_DATAARRAY, CKPGUID_DATAARRAY);
    }
    return GuidFromClassId(context, CKCID_OBJECT, CKPGUID_OBJECT);
}

CKGUID GuidFromPropertyType(CKContext *context, asIScriptEngine *engine, int typeId) {
    asITypeInfo *type = engine ? engine->GetTypeInfoById(typeId) : nullptr;
    if (!type) {
        return GuidFromClassId(context, CKCID_OBJECT, CKPGUID_OBJECT);
    }

    const std::string name = type->GetName() ? type->GetName() : "";
    CKGUID registered = ScriptResolveParameterGuid(context, name);
    if (registered != CKGUID()) {
        return registered;
    }

    CK_CLASSID classId = CKStringToClassID(const_cast<CKSTRING>(name.c_str()));
    if (classId != 0) {
        return GuidFromClassId(context, classId);
    }

    if (name == "CKBehavior") {
        return GuidFromClassId(context, CKCID_BEHAVIOR, CKPGUID_BEHAVIOR);
    }
    if (name == "CK3dEntity" || name == "CK3dObject") {
        return GuidFromClassId(context, CKCID_3DENTITY, CKPGUID_3DENTITY);
    }
    if (name == "CK2dEntity") {
        return GuidFromClassId(context, CKCID_2DENTITY, CKPGUID_2DENTITY);
    }
    if (name == "CKCamera" || name == "CKTargetCamera") {
        return GuidFromClassId(context, CKCID_CAMERA, CKPGUID_CAMERA);
    }
    if (name == "CKLight" || name == "CKTargetLight") {
        return GuidFromClassId(context, CKCID_LIGHT, CKPGUID_LIGHT);
    }
    if (name == "CKMaterial") {
        return GuidFromClassId(context, CKCID_MATERIAL, CKPGUID_MATERIAL);
    }
    if (name == "CKTexture") {
        return GuidFromClassId(context, CKCID_TEXTURE, CKPGUID_TEXTURE);
    }
    if (name == "CKMesh" || name == "CKPatchMesh" || name == "CKProgressiveMesh") {
        return GuidFromClassId(context, CKCID_MESH, CKPGUID_MESH);
    }
    if (name == "CKGroup") {
        return GuidFromClassId(context, CKCID_GROUP, CKPGUID_GROUP);
    }
    if (name == "CKDataArray") {
        return GuidFromClassId(context, CKCID_DATAARRAY, CKPGUID_DATAARRAY);
    }
    return GuidFromClassId(context, CKCID_OBJECT, CKPGUID_OBJECT);
}

CK_CLASSID ClassIdFromPropertyType(asIScriptEngine *engine, int typeId) {
    asITypeInfo *type = engine ? engine->GetTypeInfoById(typeId) : nullptr;
    if (!type || !type->GetName()) {
        return CKCID_OBJECT;
    }

    const std::string name = type->GetName();
    CK_CLASSID sdkClassId = CKStringToClassID(const_cast<CKSTRING>(name.c_str()));
    if (sdkClassId != 0) {
        return sdkClassId;
    }

    if (name == "CKBehavior") {
        return CKCID_BEHAVIOR;
    }
    if (name == "CKBeObject") {
        return CKCID_BEOBJECT;
    }
    if (name == "CK3dObject") {
        return CKCID_3DOBJECT;
    }
    if (name == "CK3dEntity") {
        return CKCID_3DENTITY;
    }
    if (name == "CK2dEntity") {
        return CKCID_2DENTITY;
    }
    if (name == "CKCamera" || name == "CKTargetCamera") {
        return CKCID_CAMERA;
    }
    if (name == "CKLight" || name == "CKTargetLight") {
        return CKCID_LIGHT;
    }
    if (name == "CKMaterial") {
        return CKCID_MATERIAL;
    }
    if (name == "CKTexture") {
        return CKCID_TEXTURE;
    }
    if (name == "CKMesh" || name == "CKPatchMesh" || name == "CKProgressiveMesh") {
        return CKCID_MESH;
    }
    if (name == "CKGroup") {
        return CKCID_GROUP;
    }
    if (name == "CKDataArray") {
        return CKCID_DATAARRAY;
    }
    return CKCID_OBJECT;
}

std::string ClassNameFromPropertyType(asIScriptEngine *engine, int typeId) {
    const char *decl = engine ? engine->GetTypeDeclaration(typeId, true) : nullptr;
    return decl ? std::string(decl) : std::string("CKObject@");
}

ScriptComponentBindingKind InferKindFromProperty(asIScriptEngine *engine, int typeId) {
    if (!engine) {
        return ScriptComponentBindingKind::Auto;
    }

    if (typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32) {
        return ScriptComponentBindingKind::Int;
    }
    if (typeId == asTYPEID_FLOAT) {
        return ScriptComponentBindingKind::Float;
    }
    if (typeId == asTYPEID_BOOL) {
        return ScriptComponentBindingKind::Bool;
    }
    const ScriptParamValueKind valueKind = ScriptParamValueKindFromAngelScriptType(engine, typeId);
    if (valueKind != ScriptParamValueKind::Empty) {
        return ComponentKindFromValueKind(valueKind);
    }
    if (typeId == engine->GetTypeIdByDecl("BehaviorRef@")) {
        return ScriptComponentBindingKind::BehaviorRef;
    }
    if (typeId == engine->GetTypeIdByDecl("ParamRef@")) {
        return ScriptComponentBindingKind::ParamRef;
    }
    if (typeId == engine->GetTypeIdByDecl("ParamValue@")) {
        return ScriptComponentBindingKind::ParamValue;
    }
    if (typeId == engine->GetTypeIdByDecl("ParamTypeInfo@")) {
        return ScriptComponentBindingKind::ParamTypeInfo;
    }
    if (typeId == engine->GetTypeIdByDecl("BBPrototype@")) {
        return ScriptComponentBindingKind::BBPrototype;
    }
    if (typeId == engine->GetTypeIdByDecl("BBDecl@")) {
        return ScriptComponentBindingKind::BBDecl;
    }
    if (typeId == engine->GetTypeIdByDecl("BBSlot@")) {
        return ScriptComponentBindingKind::BBSlot;
    }
    if (typeId == engine->GetTypeIdByDecl("BBConfig@")) {
        return ScriptComponentBindingKind::BBConfig;
    }

    asITypeInfo *type = engine->GetTypeInfoById(typeId);
    asITypeInfo *objectType = engine->GetTypeInfoByName("CKObject");
    if (type && objectType && (type == objectType || type->DerivesFrom(objectType))) {
        return ScriptComponentBindingKind::Object;
    }

    return ScriptComponentBindingKind::Auto;
}

bool IsCompatiblePropertyType(asIScriptEngine *engine,
                              int typeId,
                              ScriptComponentBindingKind kind,
                              std::string &expected) {
    if (!engine) {
        expected = "AngelScript engine";
        return false;
    }

    switch (kind) {
        case ScriptComponentBindingKind::Int:
            expected = "int or uint";
            return typeId == asTYPEID_INT32 || typeId == asTYPEID_UINT32;
        case ScriptComponentBindingKind::Float:
            expected = "float";
            return typeId == asTYPEID_FLOAT;
        case ScriptComponentBindingKind::Bool:
            expected = "bool";
            return typeId == asTYPEID_BOOL;
        case ScriptComponentBindingKind::String:
            expected = "string";
            return typeId == engine->GetTypeIdByDecl("string");
        case ScriptComponentBindingKind::Guid:
        case ScriptComponentBindingKind::Vector:
        case ScriptComponentBindingKind::Vector2:
        case ScriptComponentBindingKind::Color:
        case ScriptComponentBindingKind::Quaternion:
        case ScriptComponentBindingKind::Matrix:
        case ScriptComponentBindingKind::ObjectArray:
            return IsScriptParamValueKindCompatibleWithAngelScriptType(engine, typeId, ValueKindFromComponentKind(kind), expected);
        case ScriptComponentBindingKind::BehaviorRef:
            expected = "BehaviorRef@";
            return typeId == engine->GetTypeIdByDecl("BehaviorRef@");
        case ScriptComponentBindingKind::ParamRef:
            expected = "ParamRef@";
            return typeId == engine->GetTypeIdByDecl("ParamRef@");
        case ScriptComponentBindingKind::ParamValue:
            expected = "ParamValue@";
            return typeId == engine->GetTypeIdByDecl("ParamValue@");
        case ScriptComponentBindingKind::ParamTypeInfo:
            expected = "ParamTypeInfo@";
            return typeId == engine->GetTypeIdByDecl("ParamTypeInfo@");
        case ScriptComponentBindingKind::BBPrototype:
            expected = "BBPrototype@";
            return typeId == engine->GetTypeIdByDecl("BBPrototype@");
        case ScriptComponentBindingKind::BBDecl:
            expected = "BBDecl@";
            return typeId == engine->GetTypeIdByDecl("BBDecl@");
        case ScriptComponentBindingKind::BBSlot:
            expected = "BBSlot@";
            return typeId == engine->GetTypeIdByDecl("BBSlot@");
        case ScriptComponentBindingKind::BBConfig:
            expected = "BBConfig@";
            return typeId == engine->GetTypeIdByDecl("BBConfig@");
        case ScriptComponentBindingKind::Object: {
            expected = "CKObject@ or subclass";
            asITypeInfo *type = engine->GetTypeInfoById(typeId);
            asITypeInfo *objectType = engine->GetTypeInfoByName("CKObject");
            return type && objectType && (type == objectType || type->DerivesFrom(objectType));
        }
        default:
            expected = "supported component field type";
            return false;
    }
}

std::string BindingKindName(ScriptComponentBindingKind kind) {
    switch (kind) {
        case ScriptComponentBindingKind::Auto: return "auto";
        case ScriptComponentBindingKind::Int: return "int";
        case ScriptComponentBindingKind::Float: return "float";
        case ScriptComponentBindingKind::Bool: return "bool";
        case ScriptComponentBindingKind::String: return "string";
        case ScriptComponentBindingKind::Guid: return "CKGUID";
        case ScriptComponentBindingKind::Vector: return "VxVector";
        case ScriptComponentBindingKind::Vector2: return "Vx2DVector";
        case ScriptComponentBindingKind::Color: return "VxColor";
        case ScriptComponentBindingKind::Quaternion: return "VxQuaternion";
        case ScriptComponentBindingKind::Matrix: return "VxMatrix";
        case ScriptComponentBindingKind::ObjectArray: return "XObjectArray";
        case ScriptComponentBindingKind::Object: return "CKObject@";
        case ScriptComponentBindingKind::ParamRef: return "ParamRef@";
        case ScriptComponentBindingKind::ParamValue: return "ParamValue@";
        case ScriptComponentBindingKind::ParamTypeInfo: return "ParamTypeInfo@";
        case ScriptComponentBindingKind::BehaviorRef: return "BehaviorRef@";
        case ScriptComponentBindingKind::BBPrototype: return "BBPrototype@";
        case ScriptComponentBindingKind::BBDecl: return "BBDecl@";
        case ScriptComponentBindingKind::BBSlot: return "BBSlot@";
        case ScriptComponentBindingKind::BBConfig: return "BBConfig@";
        default: return "unknown";
    }
}

bool UsesComponentLifetime(const ScriptComponentBinding &binding) {
    return binding.BBConfigLifetime == ScriptComponentBBConfigLifetime::Component;
}

std::string BBConfigLifetimeText(const ScriptComponentBinding &binding) {
    return UsesComponentLifetime(binding) ? "component" : "manual";
}

std::string BindingSummary(const ScriptComponentBinding &binding, CKContext *context = nullptr) {
    std::ostringstream out;
    out << "field='" << binding.FieldName << "'"
        << ", parameter='" << binding.ParameterName << "'"
        << ", declaredType='" << (binding.TypeName.empty() ? "<auto>" : binding.TypeName) << "'"
        << ", bindingKind=" << BindingKindName(binding.Kind);
    if (binding.ParameterGuid != CKGUID()) {
        out << ", ckParameterType=" << ParameterTypeLabel(context, binding.ParameterGuid)
            << " " << GuidToString(binding.ParameterGuid);
    }
    if (binding.Kind == ScriptComponentBindingKind::BBSlot) {
        out << ", from='" << (binding.SlotFromFieldName.empty() ? "<none>" : binding.SlotFromFieldName) << "'"
            << ", prototype='" << (binding.SlotPrototypeName.empty() ? "<parameter/default>" : binding.SlotPrototypeName) << "'"
            << ", slotKind='" << (binding.SlotKindName.empty() ? "<missing>" : binding.SlotKindName) << "'"
            << ", slotName='" << (binding.SlotName.empty() ? "<missing>" : binding.SlotName) << "'"
            << ", occurrence=" << binding.SlotOccurrence;
    } else if (binding.Kind == ScriptComponentBindingKind::BBConfig) {
        out << ", prototype='" << (binding.SlotPrototypeName.empty() ? "<parameter/default>" : binding.SlotPrototypeName) << "'"
            << ", lifetime=" << BBConfigLifetimeText(binding);
        if (!binding.BindingStartInput.empty()) {
            out << ", start='" << binding.BindingStartInput << "'";
        }
        if (!binding.BindingStopInput.empty()) {
            out << ", stop='" << binding.BindingStopInput << "'";
        }
    }
    return out.str();
}

std::string PublicFieldCandidates(asIScriptEngine *engine, asITypeInfo *type) {
    if (!engine || !type) {
        return std::string();
    }

    std::vector<std::string> fields;
    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || isPrivate || isProtected || isConst) {
            continue;
        }
        const char *typeDecl = engine->GetTypeDeclaration(propertyTypeId, true);
        fields.push_back(std::string(propertyName) + ":" + (typeDecl ? typeDecl : "unknown"));
        if (fields.size() >= 16) {
            fields.push_back("...");
            break;
        }
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) {
            out << ", ";
        }
        out << fields[i];
    }
    return out.str();
}

struct ScriptComponentSlotName {
    std::string Name;
    int Occurrence = 0;
    bool HasOccurrence = false;
};

ScriptComponentSlotName ParseSlotNameOccurrence(const std::string &value);

void AddRequiredSlotDeclaration(ScriptComponentBinding &binding,
                                const std::string &defaultKind,
                                const std::string &value) {
    std::string token;
    auto flush = [&]() {
        token = TrimString(token);
        if (token.empty()) {
            return;
        }

        ScriptComponentRequiredSlot slot;
        slot.KindName = defaultKind;
        slot.Name = token;
        const std::size_t colon = token.find(':');
        if (colon != std::string::npos) {
            slot.KindName = TrimString(token.substr(0, colon));
            slot.Name = TrimString(token.substr(colon + 1));
        }
        slot.KindName = StripQuotes(slot.KindName);
        slot.Name = StripQuotes(slot.Name);
        const ScriptComponentSlotName parsedName = ParseSlotNameOccurrence(slot.Name);
        slot.Name = parsedName.Name;
        slot.Occurrence = parsedName.Occurrence;
        if (!slot.KindName.empty() && !slot.Name.empty()) {
            binding.RequiredSlots.push_back(slot);
        }
        token.clear();
    };

    for (char c : value) {
        if (c == ',' || c == ';' || c == '|') {
            flush();
        } else {
            token.push_back(c);
        }
    }
    flush();
}

std::vector<std::string> SplitQuotedList(const std::string &value, char delimiter = ';') {
    std::vector<std::string> tokens;
    std::string current;
    char quote = '\0';
    for (char c : value) {
        if (quote != '\0') {
            current.push_back(c);
            if (c == quote) {
                quote = '\0';
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            current.push_back(c);
            continue;
        }
        if (c == delimiter) {
            std::string token = TrimString(current);
            if (!token.empty()) {
                tokens.push_back(token);
            }
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    std::string token = TrimString(current);
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::size_t FindUnquoted(const std::string &value, const std::string &needle) {
    if (needle.empty()) {
        return std::string::npos;
    }
    char quote = '\0';
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (quote != '\0') {
            if (c == quote) {
                quote = '\0';
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            continue;
        }
        if (value.compare(i, needle.size(), needle) == 0) {
            return i;
        }
    }
    return std::string::npos;
}

bool TryParseNonNegativeInt(const std::string &value, int &parsed) {
    const std::string text = TrimString(value);
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    const long result = std::strtol(text.c_str(), &end, 0);
    if (!end || *end != '\0' || result < 0) {
        return false;
    }
    parsed = static_cast<int>(result);
    return true;
}

ScriptComponentSlotName ParseSlotNameOccurrence(const std::string &value) {
    ScriptComponentSlotName slot;
    slot.Name = StripQuotes(value);
    if (slot.Name.size() >= 3 && slot.Name.back() == ']') {
        const std::size_t open = slot.Name.rfind('[');
        int occurrence = 0;
        if (open != std::string::npos &&
            open + 1 < slot.Name.size() - 1 &&
            TryParseNonNegativeInt(slot.Name.substr(open + 1, slot.Name.size() - open - 2), occurrence)) {
            slot.Name = TrimString(slot.Name.substr(0, open));
            slot.Occurrence = occurrence;
            slot.HasOccurrence = true;
            return slot;
        }
    }

    const std::size_t hash = slot.Name.rfind('#');
    int occurrence = 0;
    if (hash != std::string::npos &&
        hash + 1 < slot.Name.size() &&
        TryParseNonNegativeInt(slot.Name.substr(hash + 1), occurrence)) {
        slot.Name = TrimString(slot.Name.substr(0, hash));
        slot.Occurrence = occurrence;
        slot.HasOccurrence = true;
    }
    return slot;
}

void AddRequiredSlot(ScriptComponentBinding &binding, const std::string &kind, const std::string &name, int occurrence = 0) {
    const std::string cleanName = TrimString(name);
    if (kind.empty() || cleanName.empty()) {
        return;
    }
    for (const ScriptComponentRequiredSlot &existing : binding.RequiredSlots) {
        if (ToLower(existing.KindName) == ToLower(kind) && existing.Name == cleanName && existing.Occurrence == occurrence) {
            return;
        }
    }
    ScriptComponentRequiredSlot required;
    required.KindName = kind;
    required.Name = cleanName;
    required.Occurrence = occurrence;
    binding.RequiredSlots.push_back(required);
}

void AddNamedSlotValues(ScriptComponentBinding &binding,
                        std::vector<ScriptComponentNamedSlotValue> &target,
                        const std::string &slotKind,
                        const std::string &value) {
    for (const std::string &entry : SplitQuotedList(value)) {
        const std::size_t equal = FindUnquoted(entry, "=");
        ScriptComponentNamedSlotValue slot;
        const ScriptComponentSlotName parsedName = ParseSlotNameOccurrence(equal == std::string::npos ? entry : entry.substr(0, equal));
        slot.Name = parsedName.Name;
        slot.Occurrence = parsedName.Occurrence;
        if (equal != std::string::npos) {
            slot.Value = StripQuotes(entry.substr(equal + 1));
            slot.HasValue = true;
        }
        if (slot.Name.empty()) {
            continue;
        }
        AddRequiredSlot(binding, slotKind, slot.Name, slot.Occurrence);
        if (slot.HasValue) {
            target.push_back(slot);
        }
    }
}

void AddConfigSourceSlots(ScriptComponentBinding &binding, const std::string &value) {
    for (const std::string &entry : SplitQuotedList(value)) {
        const std::size_t arrow = FindUnquoted(entry, "<-");
        if (arrow == std::string::npos) {
            continue;
        }
        ScriptComponentSourceSlot source;
        const ScriptComponentSlotName pin = ParseSlotNameOccurrence(entry.substr(0, arrow));
        source.PinName = pin.Name;
        source.PinOccurrence = pin.Occurrence;
        std::string rhs = TrimString(entry.substr(arrow + 2));
        const std::size_t dot = FindUnquoted(rhs, ".");
        source.SourceFieldName = StripQuotes(dot == std::string::npos ? rhs : rhs.substr(0, dot));
        const ScriptComponentSlotName sourceSlot = ParseSlotNameOccurrence(dot == std::string::npos ? std::string() : rhs.substr(dot + 1));
        source.SourceSlotName = sourceSlot.Name;
        source.SourceOccurrence = sourceSlot.Occurrence;
        if (!source.PinName.empty() && !source.SourceFieldName.empty()) {
            AddRequiredSlot(binding, "pin", source.PinName, source.PinOccurrence);
            binding.ConfigSources.push_back(source);
        }
    }
}

void AddConfigSourceSlot(ScriptComponentBinding &binding, const std::string &pinName, const std::string &sourceExpression) {
    ScriptComponentSourceSlot source;
    const ScriptComponentSlotName pin = ParseSlotNameOccurrence(pinName);
    source.PinName = pin.Name;
    source.PinOccurrence = pin.Occurrence;
    std::string rhs = TrimString(StripQuotes(sourceExpression));
    const std::size_t dot = FindUnquoted(rhs, ".");
    source.SourceFieldName = StripQuotes(dot == std::string::npos ? rhs : rhs.substr(0, dot));
    const ScriptComponentSlotName sourceSlot = ParseSlotNameOccurrence(dot == std::string::npos ? std::string() : rhs.substr(dot + 1));
    source.SourceSlotName = sourceSlot.Name;
    source.SourceOccurrence = sourceSlot.Occurrence;
    if (!source.PinName.empty() && !source.SourceFieldName.empty()) {
        AddRequiredSlot(binding, "pin", source.PinName, source.PinOccurrence);
        binding.ConfigSources.push_back(source);
    }
}

std::string FormatSlotNameOccurrence(const std::string &name, int occurrence) {
    if (occurrence <= 0 || name.empty()) {
        return name;
    }
    return name + "[" + std::to_string(occurrence) + "]";
}

std::string BuildBBConfigBindingCacheText(const ScriptComponentBinding &binding,
                                          CK_ID sourceId,
                                          const std::string &sourceText) {
    std::string requiredText;
    for (const ScriptComponentRequiredSlot &required : binding.RequiredSlots) {
        requiredText += "|" + required.KindName + ":" +
            FormatSlotNameOccurrence(required.Name, required.Occurrence);
    }

    std::string valueText;
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigPinValues) {
        valueText += "|pin:" + FormatSlotNameOccurrence(entry.Name, entry.Occurrence) + "=" + entry.Value;
    }
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigSettingValues) {
        valueText += "|setting:" + FormatSlotNameOccurrence(entry.Name, entry.Occurrence) + "=" + entry.Value;
    }

    std::string sourceBindingText;
    for (const ScriptComponentSourceSlot &entry : binding.ConfigSources) {
        sourceBindingText += "|source:" + FormatSlotNameOccurrence(entry.PinName, entry.PinOccurrence) +
            "<-" + entry.SourceFieldName + "." + FormatSlotNameOccurrence(entry.SourceSlotName, entry.SourceOccurrence);
    }

    return std::to_string(sourceId) + "|" + sourceText + "|" + binding.SlotPrototypeName + "|" +
        binding.BindingStartInput + "|" + binding.BindingStopInput + "|" +
        "lifetime=" + BBConfigLifetimeText(binding) + "|" + binding.BBConfigOwnerExpression + "|" +
        binding.BBConfigTargetExpression + "|" + std::to_string(binding.AutoStartBBConfig ? 1 : 0) + "|" +
        std::to_string(static_cast<int>(binding.BBStepPolicy)) + requiredText + valueText + sourceBindingText;
}

ScriptComponentBBStepPolicy ParseBBStepPolicy(const std::string &value) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "eachupdate" || text == "each_update" || text == "update" || text == "always") {
        return ScriptComponentBBStepPolicy::EachUpdate;
    }
    if (text == "onchange" || text == "on_change" || text == "changed") {
        return ScriptComponentBBStepPolicy::OnChange;
    }
    return ScriptComponentBBStepPolicy::Manual;
}

bool IsBBConfigFragmentKeyword(const std::string &keyword) {
    const std::string key = ToLower(keyword);
    return key == "bbinput" ||
           key == "bbin" ||
           key == "bboutput" ||
           key == "bbout" ||
           key == "bbpin" ||
           key == "bbpout" ||
           key == "bbsetting" ||
           key == "bblocal" ||
           key == "bbsource";
}

bool IsBBConfigFragmentCommonOption(const std::string &key) {
    return key == "field" ||
           key == "member" ||
           key == "property" ||
           key == "from" ||
           key == "config" ||
           key == "bbconfig";
}

std::string StripBBConfigFragmentCommonOptions(const std::string &args) {
    const std::vector<std::string> tokens = TokenizeArguments(args);
    std::string stripped;
    for (std::size_t i = 0; i < tokens.size();) {
        if (i + 2 < tokens.size() && tokens[i + 1] == "=") {
            const std::string key = ToLower(StripQuotes(tokens[i]));
            if (IsBBConfigFragmentCommonOption(key)) {
                i += 3;
                continue;
            }
        }
        if (!stripped.empty()) {
            stripped += " ";
        }
        stripped += tokens[i];
        ++i;
    }
    return stripped;
}

std::string BBConfigFragmentSlotKind(const std::string &keyword) {
    const std::string key = ToLower(keyword);
    if (key == "bbinput" || key == "bbin") {
        return "input";
    }
    if (key == "bboutput" || key == "bbout") {
        return "output";
    }
    if (key == "bbpin") {
        return "pin";
    }
    if (key == "bbpout") {
        return "pout";
    }
    if (key == "bbsetting") {
        return "setting";
    }
    if (key == "bblocal") {
        return "local";
    }
    return std::string();
}

bool ApplyBBConfigFragmentMetadata(const std::string &keyword,
                                   const std::string &args,
                                   ScriptComponentBinding &binding) {
    const std::string lowerKeyword = ToLower(keyword);
    const std::string fragmentArgs = StripBBConfigFragmentCommonOptions(args);
    if (lowerKeyword == "bbsource") {
        if (FindUnquoted(fragmentArgs, "<-") != std::string::npos) {
            AddConfigSourceSlots(binding, fragmentArgs);
            return true;
        }
        const std::vector<std::string> tokens = TokenizeArguments(fragmentArgs);
        bool added = false;
        for (std::size_t i = 0; i + 2 < tokens.size();) {
            if (tokens[i + 1] == "=") {
                AddConfigSourceSlot(binding, tokens[i], tokens[i + 2]);
                added = true;
                i += 3;
            } else {
                ++i;
            }
        }
        return added;
    }

    const std::string slotKind = BBConfigFragmentSlotKind(lowerKeyword);
    if (slotKind.empty()) {
        return false;
    }

    if (slotKind == "pin") {
        AddNamedSlotValues(binding, binding.ConfigPinValues, "pin", fragmentArgs);
        return true;
    }
    if (slotKind == "setting") {
        AddNamedSlotValues(binding, binding.ConfigSettingValues, "setting", fragmentArgs);
        return true;
    }

    AddRequiredSlotDeclaration(binding, slotKind, fragmentArgs);
    return true;
}

bool ParseBindingMetadata(const std::string &metadata,
                          const std::string &defaultFieldName,
                          ScriptComponentBinding &binding) {
    std::string text = TrimString(metadata);
    if (text.empty()) {
        return false;
    }
    if (text.size() >= 2 && text.front() == '[' && text.back() == ']') {
        text = TrimString(text.substr(1, text.size() - 2));
    }

    std::string keyword;
    std::string args;
    const std::size_t open = text.find('(');
    if (open != std::string::npos) {
        keyword = TrimString(text.substr(0, open));
        const std::size_t close = text.rfind(')');
        args = close != std::string::npos && close > open
            ? text.substr(open + 1, close - open - 1)
            : text.substr(open + 1);
    } else {
        std::istringstream stream(text);
        stream >> keyword;
        std::getline(stream, args);
        args = TrimString(args);
    }

    if (!IsComponentBindingKeyword(keyword)) {
        return false;
    }

    binding.FieldName = defaultFieldName;
    binding.ParameterName = defaultFieldName;
    binding.TypeName.clear();
    binding.DefaultValue.clear();
    binding.HasDefault = false;
    binding.InjectEveryFrame = true;
    binding.HandleInjected = false;
    binding.Kind = ScriptComponentBindingKind::Auto;
    binding.SlotFromFieldName.clear();
    binding.SlotPrototypeName.clear();
    binding.SlotKindName.clear();
    binding.SlotName.clear();
    binding.SlotOccurrence = 0;
    binding.SlotMetadataFlags = 0;
    binding.SlotValue.clear();
    binding.BBConfigLifetime = ScriptComponentBBConfigLifetime::Component;
    binding.HasBBConfigLifetime = false;
    binding.BindingStartInput.clear();
    binding.BindingStopInput.clear();
    binding.RequiredSlots.clear();
    binding.ConfigPinValues.clear();
    binding.ConfigSettingValues.clear();
    binding.ConfigSources.clear();
    binding.BBConfigOwnerExpression.clear();
    binding.BBConfigTargetExpression.clear();
    binding.BBStepPolicy = ScriptComponentBBStepPolicy::Manual;
    binding.AutoStartBBConfig = false;
    binding.HasAutoStartBBConfig = false;
    binding.HasBBStepPolicy = false;
    binding.BBConfigChanged = false;
    binding.MetadataError.clear();

    const std::string lowerKeyword = ToLower(keyword);
    if (lowerKeyword == "behavior") {
        binding.TypeName = "behaviorref";
    } else if (lowerKeyword == "bb") {
        binding.TypeName = "bb";
    } else if (lowerKeyword == "bbslot") {
        binding.TypeName = "bbslot";
    } else if (lowerKeyword == "bbdecl") {
        binding.TypeName = "bbdecl";
    } else if (lowerKeyword == "bbconfig") {
        binding.TypeName = "bbconfig";
    } else if (IsBBConfigFragmentKeyword(lowerKeyword)) {
        binding.TypeName = "bbconfig";
        ApplyBBConfigFragmentMetadata(lowerKeyword, args, binding);
    }

    std::vector<std::string> positional;
    const std::vector<std::string> tokens = TokenizeArguments(args);
    for (std::size_t i = 0; i < tokens.size();) {
        if (i + 2 < tokens.size() && tokens[i + 1] == "=") {
            const std::string key = ToLower(StripQuotes(tokens[i]));
            const std::string value = StripQuotes(tokens[i + 2]);
            if (key == "name" || key == "param" || key == "parameter") {
                binding.ParameterName = value;
            } else if (key == "field" || key == "member" || key == "property") {
                binding.FieldName = value;
            } else if (key == "type" || key == "kind") {
                binding.TypeName = value;
            } else if (key == "default") {
                binding.DefaultValue = value;
                binding.HasDefault = true;
                if (lowerKeyword == "bbslot") {
                    SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::HasDefault, true);
                }
            } else if (key == "value") {
                if (lowerKeyword == "bbslot") {
                    binding.SlotValue = value;
                    SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::HasValue, true);
                } else {
                    binding.DefaultValue = value;
                    binding.HasDefault = true;
                }
            } else if (key == "update" || key == "sync") {
                binding.InjectEveryFrame = ParseBoolText(value, true);
            } else if (key == "owner") {
                binding.BBConfigOwnerExpression = value;
            } else if (key == "target") {
                binding.BBConfigTargetExpression = value;
            } else if (key == "managed") {
                binding.MetadataError = "Component BBConfig metadata key 'managed' was removed. Use lifetime=\"component\" or lifetime=\"manual\".";
            } else if (key == "lifetime") {
                const std::string lifetime = ToLower(value);
                if (lifetime == "manual") {
                    binding.BBConfigLifetime = ScriptComponentBBConfigLifetime::Manual;
                    binding.HasBBConfigLifetime = true;
                } else if (lifetime == "component" || lifetime.empty()) {
                    binding.BBConfigLifetime = ScriptComponentBBConfigLifetime::Component;
                    binding.HasBBConfigLifetime = true;
                } else {
                    binding.MetadataError = "Component BBConfig lifetime must be 'component' or 'manual', got '" + value + "'.";
                }
            } else if (key == "autostart" || key == "auto_start") {
                binding.AutoStartBBConfig = ParseBoolText(value, true);
                binding.HasAutoStartBBConfig = true;
            } else if (key == "step") {
                binding.BBStepPolicy = ParseBBStepPolicy(value);
                binding.HasBBStepPolicy = true;
            } else if (key == "start") {
                if (lowerKeyword == "bbslot") {
                    SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Start, ParseBoolText(value, true));
                } else {
                    binding.BindingStartInput = value;
                }
            } else if (key == "stop") {
                if (lowerKeyword == "bbslot") {
                    SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Stop, ParseBoolText(value, true));
                } else {
                    binding.BindingStopInput = value;
                }
            } else if (key == "required") {
                if (lowerKeyword == "bbslot") {
                    SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Required, ParseBoolText(value, true));
                } else {
                    AddRequiredSlotDeclaration(binding, std::string(), value);
                }
            } else if (key == "from" || key == "config" || key == "bbconfig") {
                binding.SlotFromFieldName = value;
            } else if (key == "prototype" || key == "proto" || key == "bbname" || key == "bbquery") {
                binding.SlotPrototypeName = value;
            } else if (key == "slot" || key == "slotkind") {
                binding.SlotKindName = value;
            } else if (key == "slotname" || key == "slot_name" ||
                       key == "input" || key == "in" ||
                       key == "output" || key == "out" ||
                       key == "pin" || key == "pout" ||
                       key == "local" || key == "setting") {
                const ScriptComponentSlotName parsedSlotName = ParseSlotNameOccurrence(value);
                binding.SlotName = parsedSlotName.Name;
                if (parsedSlotName.HasOccurrence) {
                    binding.SlotOccurrence = parsedSlotName.Occurrence;
                }
                if (key == "input" || key == "in") {
                    binding.SlotKindName = "input";
                } else if (key == "output" || key == "out") {
                    binding.SlotKindName = "output";
                } else if (key == "pin" || key == "pout" || key == "local" || key == "setting") {
                    binding.SlotKindName = key;
                }
            } else if (key == "occurrence" || key == "index") {
                char *end = nullptr;
                const long parsed = std::strtol(value.c_str(), &end, 0);
                binding.SlotOccurrence = end && *end == '\0' && parsed >= 0 ? static_cast<int>(parsed) : 0;
            } else if (key == "requiredsettings" || key == "required_settings") {
                AddRequiredSlotDeclaration(binding, "setting", value);
            } else if (key == "inputs" || key == "ins") {
                AddRequiredSlotDeclaration(binding, "input", value);
            } else if (key == "outputs" || key == "outs") {
                AddRequiredSlotDeclaration(binding, "output", value);
            } else if (key == "pins") {
                if (lowerKeyword == "bbconfig") {
                    AddNamedSlotValues(binding, binding.ConfigPinValues, "pin", value);
                } else {
                    AddRequiredSlotDeclaration(binding, "pin", value);
                }
            } else if (key == "pouts") {
                AddRequiredSlotDeclaration(binding, "pout", value);
            } else if (key == "locals") {
                AddRequiredSlotDeclaration(binding, "local", value);
            } else if (key == "settings") {
                if (lowerKeyword == "bbconfig") {
                    AddNamedSlotValues(binding, binding.ConfigSettingValues, "setting", value);
                } else {
                    AddRequiredSlotDeclaration(binding, "setting", value);
                }
            } else if (key == "setting") {
                if (lowerKeyword == "bbconfig") {
                    AddNamedSlotValues(binding, binding.ConfigSettingValues, "setting", value);
                } else {
                    AddRequiredSlotDeclaration(binding, "setting", value);
                }
            } else if (key == "sources" || key == "source") {
                if (lowerKeyword == "bbconfig") {
                    AddConfigSourceSlots(binding, value);
                }
            }
            i += 3;
            continue;
        }

        const std::string flag = ToLower(StripQuotes(tokens[i]));
        if (lowerKeyword == "bbslot" && flag == "start") {
            SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Start, true);
            ++i;
            continue;
        }
        if (lowerKeyword == "bbslot" && flag == "stop") {
            SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Stop, true);
            ++i;
            continue;
        }
        if (lowerKeyword == "bbslot" && flag == "required") {
            SetScriptBridgeSlotMetadataFlag(binding.SlotMetadataFlags, ScriptBridgeSlotMetadataFlags::Required, true);
            ++i;
            continue;
        }

        positional.push_back(StripQuotes(tokens[i]));
        ++i;
    }

    const bool isBBConfigFragment = IsBBConfigFragmentKeyword(lowerKeyword);
    if (!positional.empty() && !isBBConfigFragment) {
        if (binding.ParameterName.empty() || binding.ParameterName == defaultFieldName) {
            binding.ParameterName = positional[0];
        }
    }
    if (positional.size() > 1 && binding.TypeName.empty()) {
        binding.TypeName = positional[1];
    }
    if (positional.size() > 2 && !binding.HasDefault) {
        binding.DefaultValue = positional[2];
        binding.HasDefault = true;
    }

    if (binding.FieldName.empty()) {
        binding.FieldName = defaultFieldName;
    }
    if (binding.ParameterName.empty()) {
        binding.ParameterName = binding.FieldName;
    }
    if (binding.FieldName.empty()) {
        return false;
    }

    binding.Kind = KindFromTypeName(binding.TypeName);
    return true;
}

bool ParseManifestLine(const std::string &line, ScriptComponentBinding &binding) {
    std::string text = TrimString(line);
    if (text.empty() || text.rfind("//", 0) == 0 || text.rfind("#", 0) == 0) {
        return false;
    }
    if (text.size() >= 2 && text.front() == '[' && text.back() == ']') {
        text = text.substr(1, text.size() - 2);
    }

    if (ParseBindingMetadata(text, std::string(), binding)) {
        return true;
    }

    std::string defaultValue;
    bool hasDefault = false;
    const std::size_t equal = text.find('=');
    if (equal != std::string::npos) {
        defaultValue = StripQuotes(text.substr(equal + 1));
        text = TrimString(text.substr(0, equal));
        hasDefault = true;
    }

    for (char &c : text) {
        if (c == ':') {
            c = ' ';
        }
    }

    const std::vector<std::string> tokens = TokenizeArguments(text);
    if (tokens.size() < 2) {
        return false;
    }

    binding = ScriptComponentBinding();
    binding.FieldName = StripQuotes(tokens[0]);
    binding.TypeName = StripQuotes(tokens[1]);
    binding.ParameterName = binding.FieldName;
    if (tokens.size() >= 3) {
        binding.ParameterName = StripQuotes(tokens[2]);
    }
    binding.DefaultValue = defaultValue;
    binding.HasDefault = hasDefault;
    binding.InjectEveryFrame = true;
    binding.SlotOccurrence = 0;
    binding.Kind = KindFromTypeName(binding.TypeName);
    return !binding.FieldName.empty();
}

void ReplaceOrAppendBinding(std::vector<ScriptComponentBinding> &bindings, const ScriptComponentBinding &binding) {
    for (ScriptComponentBinding &existing : bindings) {
        if (existing.FieldName == binding.FieldName) {
            existing = binding;
            return;
        }
    }
    bindings.push_back(binding);
}

bool IsBBConfigBindingSpec(const ScriptComponentBinding &binding) {
    return binding.Kind == ScriptComponentBindingKind::BBConfig || ToLower(binding.TypeName) == "bbconfig";
}

void AddMergedRequiredSlot(ScriptComponentBinding &binding, const ScriptComponentRequiredSlot &slot) {
    if (slot.KindName.empty() || slot.Name.empty()) {
        return;
    }
    for (const ScriptComponentRequiredSlot &existing : binding.RequiredSlots) {
        if (ToLower(existing.KindName) == ToLower(slot.KindName) &&
            existing.Name == slot.Name &&
            existing.Occurrence == slot.Occurrence) {
            return;
        }
    }
    binding.RequiredSlots.push_back(slot);
}

void MergeBBConfigBinding(ScriptComponentBinding &target, const ScriptComponentBinding &source) {
    if (!source.TypeName.empty()) {
        target.TypeName = source.TypeName;
    }
    if (target.Kind == ScriptComponentBindingKind::Auto || source.Kind == ScriptComponentBindingKind::BBConfig) {
        target.Kind = source.Kind;
    }
    if (!source.ParameterName.empty() && source.ParameterName != source.FieldName) {
        target.ParameterName = source.ParameterName;
    }
    if (!source.DefaultValue.empty() || source.HasDefault) {
        target.DefaultValue = source.DefaultValue;
        target.HasDefault = source.HasDefault;
    }
    if (!source.SlotPrototypeName.empty()) {
        target.SlotPrototypeName = source.SlotPrototypeName;
    }
    if (!source.BBConfigOwnerExpression.empty()) {
        target.BBConfigOwnerExpression = source.BBConfigOwnerExpression;
    }
    if (!source.BBConfigTargetExpression.empty()) {
        target.BBConfigTargetExpression = source.BBConfigTargetExpression;
    }
    if (source.HasBBConfigLifetime) {
        target.BBConfigLifetime = source.BBConfigLifetime;
        target.HasBBConfigLifetime = true;
    }
    if (!source.MetadataError.empty()) {
        target.MetadataError = source.MetadataError;
    }
    if (source.HasAutoStartBBConfig) {
        target.AutoStartBBConfig = source.AutoStartBBConfig;
        target.HasAutoStartBBConfig = true;
    }
    if (source.HasBBStepPolicy) {
        target.BBStepPolicy = source.BBStepPolicy;
        target.HasBBStepPolicy = true;
    }
    if (!source.BindingStartInput.empty()) {
        target.BindingStartInput = source.BindingStartInput;
    }
    if (!source.BindingStopInput.empty()) {
        target.BindingStopInput = source.BindingStopInput;
    }
    for (const ScriptComponentRequiredSlot &slot : source.RequiredSlots) {
        AddMergedRequiredSlot(target, slot);
    }
    target.ConfigPinValues.insert(target.ConfigPinValues.end(), source.ConfigPinValues.begin(), source.ConfigPinValues.end());
    target.ConfigSettingValues.insert(target.ConfigSettingValues.end(), source.ConfigSettingValues.begin(), source.ConfigSettingValues.end());
    target.ConfigSources.insert(target.ConfigSources.end(), source.ConfigSources.begin(), source.ConfigSources.end());
}

void MergeOrAppendMetadataBinding(std::vector<ScriptComponentBinding> &bindings, const ScriptComponentBinding &binding) {
    for (ScriptComponentBinding &existing : bindings) {
        if (existing.FieldName != binding.FieldName) {
            continue;
        }
        if (IsBBConfigBindingSpec(existing) || IsBBConfigBindingSpec(binding)) {
            MergeBBConfigBinding(existing, binding);
        } else {
            existing = binding;
        }
        return;
    }
    bindings.push_back(binding);
}

std::vector<ScriptComponentBinding> BuildComponentBindingSpecs(ScriptComponentState *state, asITypeInfo *type) {
    std::vector<ScriptComponentBinding> bindings;
    if (!state || !state->Runner || !type) {
        return bindings;
    }

    std::shared_ptr<CachedScript> cached = state->Runner->GetCachedScript();
    if (cached) {
        const int typeId = type->GetTypeId();
        for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
            const char *propertyName = nullptr;
            int propertyTypeId = 0;
            bool isPrivate = false;
            bool isProtected = false;
            bool isConst = false;
            type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
            if (!propertyName || isPrivate || isProtected || isConst) {
                continue;
            }

            const int metaCount = cached->GetClassVarMetadataCount(typeId, static_cast<int>(i));
            for (int metaIndex = 0; metaIndex < metaCount; ++metaIndex) {
                const char *metadata = cached->GetClassVarMetadata(typeId, static_cast<int>(i), metaIndex);
                ScriptComponentBinding binding;
                if (metadata && ParseBindingMetadata(metadata, propertyName, binding)) {
                    MergeOrAppendMetadataBinding(bindings, binding);
                }
            }
        }
    }

    std::istringstream manifest(state->Manifest);
    std::string line;
    while (std::getline(manifest, line)) {
        ScriptComponentBinding binding;
        if (ParseManifestLine(line, binding)) {
            ReplaceOrAppendBinding(bindings, binding);
        }
    }

    return bindings;
}

bool ResolveComponentBinding(asIScriptEngine *engine,
                             asITypeInfo *type,
                             CKContext *context,
                             ScriptComponentBinding &binding,
                             std::string &error) {
    if (!engine || !type) {
        error = "AngelScript type information is not available.";
        return false;
    }

    for (asUINT i = 0; i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || binding.FieldName != propertyName) {
            continue;
        }

        if (isPrivate || isProtected || isConst) {
            error = "Component field '" + binding.FieldName + "' must be a writable public class property.";
            return false;
        }

        const ScriptComponentBindingKind inferred = InferKindFromProperty(engine, propertyTypeId);
        const std::string declaredType = ToLower(StripQuotes(binding.TypeName));
        if ((declaredType == "behavior" || declaredType == "ckbehavior") && inferred == ScriptComponentBindingKind::BehaviorRef) {
            binding.Kind = ScriptComponentBindingKind::BehaviorRef;
        } else if ((declaredType == "behavior" || declaredType == "ckbehavior") && inferred == ScriptComponentBindingKind::BBPrototype) {
            binding.Kind = ScriptComponentBindingKind::BBPrototype;
        }
        if (binding.Kind == ScriptComponentBindingKind::Auto) {
            binding.Kind = inferred;
        }
        if (!binding.MetadataError.empty()) {
            error = binding.MetadataError + " (" + BindingSummary(binding, context) + ").";
            return false;
        }
        if (binding.Kind == ScriptComponentBindingKind::BBConfig && !binding.HasBBConfigLifetime) {
            binding.BBConfigLifetime = ScriptComponentBBConfigLifetime::Component;
        }

        std::string expected;
        if (!IsCompatiblePropertyType(engine, propertyTypeId, binding.Kind, expected)) {
            const char *actual = engine->GetTypeDeclaration(propertyTypeId, true);
            error = "Component metadata field type mismatch (" + BindingSummary(binding, context) +
                    "). Expected script field type " + expected + ", got " + (actual ? actual : "unknown") + ".";
            return false;
        }

        binding.PropertyIndex = static_cast<int>(i);
        binding.PropertyTypeId = propertyTypeId;
        binding.ParameterGuid = binding.TypeName.empty()
            ? (binding.Kind == ScriptComponentBindingKind::Object ? GuidFromPropertyType(context, engine, propertyTypeId) : GuidFromTypeName(context, "", binding.Kind))
            : GuidFromTypeName(context, binding.TypeName, binding.Kind);
        if (binding.ParameterName.empty()) {
            binding.ParameterName = binding.FieldName;
        }
        return true;
    }

    error = "Component manifest references missing field (" + BindingSummary(binding, context) + ").";
    const std::string candidates = PublicFieldCandidates(engine, type);
    if (!candidates.empty()) {
        error += " Writable public fields: " + candidates + ".";
    }
    return false;
}

bool SetParameterDefaultValue(CKParameterLocal *local, const ScriptComponentBinding &binding, CKContext *context) {
    if (!local || !binding.HasDefault) {
        return true;
    }

    const ScriptParamValueKind valueKind = ValueKindFromComponentKind(binding.Kind);
    if (valueKind != ScriptParamValueKind::Empty) {
        std::string error;
        return SetParameterDefaultText(local, binding.DefaultValue, error);
    }

    switch (binding.Kind) {
        case ScriptComponentBindingKind::ParamRef:
        case ScriptComponentBindingKind::ParamValue:
        case ScriptComponentBindingKind::ParamTypeInfo: {
            std::string error;
            return SetParameterDefaultText(local, binding.DefaultValue, error);
        }
        case ScriptComponentBindingKind::BBPrototype:
        case ScriptComponentBindingKind::BBDecl:
        case ScriptComponentBindingKind::BBSlot:
        case ScriptComponentBindingKind::BBConfig:
            return local->SetStringValue(const_cast<CKSTRING>(binding.DefaultValue.c_str())) == CK_OK;
        case ScriptComponentBindingKind::Object: {
            CK_ID id = 0;
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(StripQuotes(binding.DefaultValue).c_str(), &end, 0);
            if (end && *end == '\0') {
                id = static_cast<CK_ID>(parsed);
            } else if (context) {
                const std::string name = StripQuotes(binding.DefaultValue);
                CKObject *object = context->GetObjectByName(const_cast<CKSTRING>(name.c_str()));
                id = object ? object->GetID() : 0;
            }
            return local->SetValue(&id, sizeof(id)) == CK_OK;
        }
        case ScriptComponentBindingKind::BehaviorRef: {
            CK_ID id = 0;
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(StripQuotes(binding.DefaultValue).c_str(), &end, 0);
            if (end && *end == '\0') {
                id = static_cast<CK_ID>(parsed);
            } else if (context) {
                const std::string name = StripQuotes(binding.DefaultValue);
                CKBehavior *behavior = FindBehaviorByNameInContext(context, name);
                if (!behavior) {
                    behavior = CKBehavior::Cast(context->GetObjectByName(const_cast<CKSTRING>(name.c_str())));
                }
                id = behavior ? behavior->GetID() : 0;
            }
            return local->SetValue(&id, sizeof(id)) == CK_OK;
        }
        default:
            return true;
    }
}

int FindComponentInputParameterIndex(CKBehavior *beh, const std::string &name) {
    if (!beh) {
        return -1;
    }
    for (int i = 0; i < beh->GetInputParameterCount(); ++i) {
        CKParameterIn *param = beh->GetInputParameter(i);
        if (param && NameEquals(param->GetName(), name)) {
            return i;
        }
    }
    return -1;
}

CKParameter *EnsureInputSource(CKBehavior *beh, CKParameterIn *input, const ScriptComponentBinding &binding) {
    if (!beh || !input) {
        return nullptr;
    }

    CKParameter *source = input->GetRealSource();
    const std::string sourceName = "__CKAS_ComponentInput_" + binding.FieldName;
    if (source) {
        CKParameterLocal *localSource = CKParameterLocal::Cast(source);
        if (localSource && NameEquals(localSource->GetName(), sourceName)) {
            SetParameterDefaultValue(localSource, binding, beh->GetCKContext());
        }
        return source;
    }

    CKParameterLocal *local = nullptr;
    for (int i = 0; i < beh->GetLocalParameterCount(); ++i) {
        CKParameterLocal *candidate = beh->GetLocalParameter(i);
        if (candidate && NameEquals(candidate->GetName(), sourceName)) {
            local = candidate;
            break;
        }
    }

    if (!local) {
        local = beh->CreateLocalParameter(const_cast<CKSTRING>(sourceName.c_str()), input->GetGUID());
    }
    if (!local) {
        return nullptr;
    }

    SetParameterDefaultValue(local, binding, beh->GetCKContext());
    beh->SetInputParameterDefaultValue(input, local);
    if (input->SetDirectSource(local) != CK_OK) {
        return nullptr;
    }
    return local;
}

bool SyncDeclaredInputParameters(CKBehavior *beh, ScriptComponentState *state, std::vector<ScriptComponentBinding> &bindings, std::string &error) {
    if (!beh || !state) {
        error = "Component behavior is not available.";
        return false;
    }

    std::unordered_set<std::string> currentNames;
    for (const ScriptComponentBinding &binding : bindings) {
        if (currentNames.find(binding.ParameterName) != currentNames.end()) {
            error = "Duplicate Component input parameter declaration: " + binding.ParameterName;
            return false;
        }
        currentNames.insert(binding.ParameterName);
    }

    for (const std::string &oldName : state->ManagedInputParameterNames) {
        if (currentNames.find(oldName) != currentNames.end()) {
            continue;
        }

        for (int i = beh->GetInputParameterCount() - 1; i >= FIXED_INPUT_PARAMETER_COUNT; --i) {
            CKParameterIn *param = beh->GetInputParameter(i);
            if (param && NameEquals(param->GetName(), oldName)) {
                CKParameterIn *removed = beh->RemoveInputParameter(i);
                if (removed) {
                    CKDestroyObject(removed);
                }
            }
        }
    }

    state->ManagedInputParameterNames.clear();
    for (ScriptComponentBinding &binding : bindings) {
        int index = FindComponentInputParameterIndex(beh, binding.ParameterName);
        if (index >= 0 && index < FIXED_INPUT_PARAMETER_COUNT) {
            error = "Component input parameter name is reserved: " + binding.ParameterName;
            return false;
        }
        if (index < 0) {
            CKParameterIn *created = beh->CreateInputParameter(const_cast<CKSTRING>(binding.ParameterName.c_str()), binding.ParameterGuid);
            if (!created) {
                error = "Failed to create Component input parameter: " + binding.ParameterName;
                return false;
            }
            index = beh->GetInputParameterPosition(created);
        }

        CKParameterIn *input = beh->GetInputParameter(index);
        if (!input) {
            error = "Component input parameter is not available: " + binding.ParameterName;
            return false;
        }
        if (input->GetGUID() != binding.ParameterGuid) {
            input->SetGUID(binding.ParameterGuid, TRUE, const_cast<CKSTRING>(binding.ParameterName.c_str()));
        }
        if (!EnsureInputSource(beh, input, binding)) {
            error = "Failed to create default source for Component input parameter: " + binding.ParameterName;
            return false;
        }

        binding.InputParameterIndex = index;
        state->ManagedInputParameterNames.push_back(binding.ParameterName);
    }

    return true;
}

bool ReadStringValue(CKParameter *source, std::string &value) {
    return ReadParameterText(source, value);
}

bool IsObjectValueParameter(CKParameter *source) {
    if (!source) {
        return false;
    }

    const CK_CLASSID classId = source->GetParameterClassID();
    return classId != 0 && CKIsChildClassOf(classId, CKCID_OBJECT);
}

CKObject *ReadObjectValue(CKParameter *source, CKContext *context) {
    if (!source) {
        return nullptr;
    }

    const bool objectParameter = IsObjectValueParameter(source);
    if (objectParameter) {
        if (CKObject *object = source->GetValueObject(TRUE)) {
            return object;
        }
    }

    std::string text;
    if (ReadStringValue(source, text)) {
        text = TrimString(text);
        if (!text.empty()) {
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
            if (end && *end == '\0') {
                return context ? CKGetObject(context, static_cast<CK_ID>(parsed)) : nullptr;
            }
            return context ? context->GetObjectByName(const_cast<CKSTRING>(text.c_str())) : nullptr;
        }
    }

    if (objectParameter) {
        CK_ID id = 0;
        if (source->GetValue(&id) == CK_OK && id != 0) {
            return context ? CKGetObject(context, id) : nullptr;
        }
    }

    return nullptr;
}

CKBehavior *ReadBehaviorValue(CKParameter *source, CKContext *context) {
    if (!source) {
        return nullptr;
    }

    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, context))) {
        return behavior;
    }

    std::string text;
    if (ReadStringValue(source, text)) {
        text = StripQuotes(text);
        if (!text.empty()) {
            char *end = nullptr;
            const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
            if (end && *end == '\0') {
                return context ? CKBehavior::Cast(CKGetObject(context, static_cast<CK_ID>(parsed))) : nullptr;
            }
            return FindBehaviorByNameInContext(context, text);
        }
    }

    return nullptr;
}

bool AssignObjectHandle(asIScriptObject *object, int propertyIndex, void *value) {
    if (!object || propertyIndex < 0) {
        return false;
    }

    void *address = object->GetAddressOfProperty(static_cast<asUINT>(propertyIndex));
    if (!address) {
        return false;
    }

    *static_cast<void **>(address) = value;
    return true;
}

void **GetHandleSlot(asIScriptObject *object, int propertyIndex) {
    if (!object || propertyIndex < 0) {
        return nullptr;
    }

    void *address = object->GetAddressOfProperty(static_cast<asUINT>(propertyIndex));
    return address ? static_cast<void **>(address) : nullptr;
}

bool AssignBehaviorRefHandle(ScriptBehaviorBridge *bridge, asIScriptObject *object, int propertyIndex, BehaviorRef *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (bridge && value) {
            bridge->ReleaseBehaviorRef(value);
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (bridge && *slot) {
        bridge->ReleaseBehaviorRef(static_cast<BehaviorRef *>(*slot));
    }
    *slot = value;
    return true;
}

bool AssignBBPrototypeHandle(ScriptBehaviorBridge *bridge, asIScriptObject *object, int propertyIndex, BBPrototype *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (bridge && value) {
            bridge->ReleasePrototype(value);
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (bridge && *slot) {
        bridge->ReleasePrototype(static_cast<BBPrototype *>(*slot));
    }
    *slot = value;
    return true;
}

template <typename T>
bool AssignRefCountedHandle(asIScriptObject *object, int propertyIndex, T *value) {
    void **slot = GetHandleSlot(object, propertyIndex);
    if (!slot) {
        if (value) {
            value->Release();
        }
        return false;
    }

    if (*slot == value) {
        return true;
    }

    if (*slot) {
        static_cast<T *>(*slot)->Release();
    }
    *slot = value;
    return true;
}

BBConfig *GetBBConfigField(ScriptComponentState *state, const ScriptComponentBinding &binding) {
    if (!state || !state->Object || binding.PropertyIndex < 0 || binding.Kind != ScriptComponentBindingKind::BBConfig) {
        return nullptr;
    }
    void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
    return slot ? static_cast<BBConfig *>(*slot) : nullptr;
}

BBConfig *GetBBConfigFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::BBConfig && binding.FieldName == fieldName) {
            return GetBBConfigField(state, binding);
        }
    }
    return nullptr;
}

ParamRef *GetParamRefFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::ParamRef && binding.FieldName == fieldName) {
            void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
            ParamRef *ref = slot ? static_cast<ParamRef *>(*slot) : nullptr;
            if (ref) {
                ref->AddRef();
            }
            return ref;
        }
    }
    return nullptr;
}

BehaviorRef *GetBehaviorRefFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind == ScriptComponentBindingKind::BehaviorRef && binding.FieldName == fieldName) {
            void **slot = GetHandleSlot(state->Object, binding.PropertyIndex);
            BehaviorRef *ref = slot ? static_cast<BehaviorRef *>(*slot) : nullptr;
            if (ref) {
                ref->AddRef();
            }
            return ref;
        }
    }
    return nullptr;
}

BBInstance *GetBBInstanceFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    asITypeInfo *type = state->Object->GetObjectType();
    asIScriptEngine *engine = type ? type->GetEngine() : nullptr;
    for (asUINT i = 0; type && i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || fieldName != propertyName || isPrivate || isProtected || isConst) {
            continue;
        }
        const char *decl = engine ? engine->GetTypeDeclaration(propertyTypeId, true) : nullptr;
        if (!decl || std::string(decl) != "BBInstance@") {
            return nullptr;
        }
        void **slot = GetHandleSlot(state->Object, static_cast<int>(i));
        BBInstance *instance = slot ? static_cast<BBInstance *>(*slot) : nullptr;
        if (instance) {
            instance->AddRef();
        }
        return instance;
    }
    return nullptr;
}

CKBeObject *GetBeObjectFieldByName(ScriptComponentState *state, const std::string &fieldName) {
    if (!state || !state->Object || fieldName.empty()) {
        return nullptr;
    }
    asITypeInfo *type = state->Object->GetObjectType();
    for (asUINT i = 0; type && i < type->GetPropertyCount(); ++i) {
        const char *propertyName = nullptr;
        int propertyTypeId = 0;
        bool isPrivate = false;
        bool isProtected = false;
        bool isConst = false;
        type->GetProperty(i, &propertyName, &propertyTypeId, &isPrivate, &isProtected, nullptr, nullptr, nullptr, nullptr, nullptr, &isConst);
        if (!propertyName || fieldName != propertyName || isPrivate || isProtected || isConst) {
            continue;
        }
        if (InferKindFromProperty(type->GetEngine(), propertyTypeId) != ScriptComponentBindingKind::Object) {
            return nullptr;
        }
        void **slot = GetHandleSlot(state->Object, static_cast<int>(i));
        return slot ? CKBeObject::Cast(static_cast<CKObject *>(*slot)) : nullptr;
    }
    return nullptr;
}

CKBeObject *ResolveBBConfigObjectExpression(ScriptComponentState *state,
                                            const CKBehaviorContext &behcontext,
                                            const std::string &expression) {
    const std::string text = StripQuotes(expression);
    if (text.empty()) {
        return nullptr;
    }
    if (text == "$owner") {
        return behcontext.Behavior ? behcontext.Behavior->GetOwner() : nullptr;
    }
    if (text == "$target") {
        return behcontext.Behavior ? behcontext.Behavior->GetTarget() : nullptr;
    }
    if (text == "$level") {
        return behcontext.CurrentLevel ? behcontext.CurrentLevel : (behcontext.Context ? behcontext.Context->GetCurrentLevel() : nullptr);
    }
    return GetBeObjectFieldByName(state, text);
}

void StopComponentLifetimeBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    if (!state || !state->Object) {
        return;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (!UsesComponentLifetime(binding)) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (bbinding) {
            bbinding->Stop(behcontext);
        }
    }
}

void DestroyComponentLifetimeBBConfigs(ScriptComponentState *state) {
    if (!state || !state->Object) {
        return;
    }
    for (const ScriptComponentBinding &binding : state->Bindings) {
        if (!UsesComponentLifetime(binding)) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (bbinding) {
            bbinding->Destroy();
        }
    }
}

bool ValidateObjectFieldValue(asIScriptEngine *engine, const ScriptComponentBinding &binding, CKObject *value, std::string &error) {
    if (!value) {
        return true;
    }

    const CK_CLASSID expected = ClassIdFromPropertyType(engine, binding.PropertyTypeId);
    if (expected == CKCID_OBJECT || CKIsChildClassOf(value, expected)) {
        return true;
    }

    error = "Component parameter '" + binding.ParameterName + "' value '" +
            (value->GetName() ? value->GetName() : "<unnamed>") +
            "' is not compatible with field '" + binding.FieldName + "' (" +
            ClassNameFromPropertyType(engine, binding.PropertyTypeId) + ").";
    return false;
}

BBPrototype *CreatePrototypeFromParameter(ScriptBehaviorBridge *bridge,
                                          const CKBehaviorContext &behcontext,
                                          CKParameter *source,
                                          std::string &error) {
    if (!source) {
        error = "BBPrototype Component parameter source is not available.";
        return nullptr;
    }

    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context))) {
        const CKGUID guid = behavior->GetPrototypeGuid();
        if (guid != CKGUID()) {
            return bridge ? bridge->CreatePrototype(behcontext, guid) : nullptr;
        }
        error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' references behavior '" + SafeString(behavior->GetName()) +
                "' without a prototype GUID.";
        return nullptr;
    }

    std::string prototypeName;
    if (!ReadStringValue(source, prototypeName)) {
        error = "Failed to read BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' as text. CK type=" + ParameterTypeLabel(behcontext.Context, source) + ".";
        return nullptr;
    }

    prototypeName = TrimString(prototypeName);
    if (prototypeName.empty() || !bridge) {
        if (prototypeName.empty()) {
            error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                    "' is empty. Expected BB name, Category/Name, GUID, or CKBehavior object.";
        } else {
            error = "BBPrototype Component injection requires ScriptBehaviorBridge.";
        }
        return nullptr;
    }

    CKGUID guid;
    BBPrototype *prototype = ParseScriptGuidString(prototypeName, guid)
        ? bridge->CreatePrototype(behcontext, guid)
        : bridge->CreatePrototype(behcontext, prototypeName);
    if (!prototype) {
        error = "BBPrototype Component parameter '" + SafeString(source->GetName()) +
                "' did not resolve to a BB prototype: '" + prototypeName + "'.";
    }
    return prototype;
}

ScriptBridgeSlotKind SlotKindFromText(const std::string &value) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "input" || text == "in") {
        return ScriptBridgeSlotKind::Input;
    }
    if (text == "output" || text == "out") {
        return ScriptBridgeSlotKind::Output;
    }
    if (text == "pin" || text == "inputparam" || text == "inputparameter") {
        return ScriptBridgeSlotKind::Pin;
    }
    if (text == "pout" || text == "outputparam" || text == "outputparameter") {
        return ScriptBridgeSlotKind::Pout;
    }
    if (text == "setting" || text == "settings") {
        return ScriptBridgeSlotKind::Setting;
    }
    if (text == "local" || text == "plocal" || text == "localparam" || text == "localparameter") {
        return ScriptBridgeSlotKind::Local;
    }
    return ScriptBridgeSlotKind::Standalone;
}

struct ScriptComponentSourceSelector {
    ScriptBridgeSlotKind Kind = ScriptBridgeSlotKind::Pout;
    std::string Name;
    std::string Prefix;
};

ScriptComponentSourceSelector ParseSourceSelector(const std::string &value) {
    ScriptComponentSourceSelector selector;
    selector.Name = StripQuotes(value);
    const std::size_t separator = selector.Name.find(':');
    if (separator != std::string::npos) {
        selector.Prefix = StripQuotes(selector.Name.substr(0, separator));
        selector.Kind = SlotKindFromText(selector.Prefix);
        selector.Name = StripQuotes(selector.Name.substr(separator + 1));
    }
    return selector;
}

std::string SourceSelectorKindName(const ScriptComponentSourceSelector &selector) {
    if (!selector.Prefix.empty()) {
        return selector.Prefix;
    }
    switch (selector.Kind) {
        case ScriptBridgeSlotKind::Pin: return "pin";
        case ScriptBridgeSlotKind::Pout: return "pout";
        case ScriptBridgeSlotKind::Local: return "local";
        default: return "source";
    }
}

BBDecl *CreateSpecFromParameter(ScriptBehaviorBridge *bridge,
                                const CKBehaviorContext &behcontext,
                                CKParameter *source,
                                std::string &error) {
    if (!bridge) {
        error = "BBDecl Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }
    if (!source) {
        error = "BBDecl Component parameter source is not available.";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    if (CKBehavior *behavior = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context))) {
        const CKGUID guid = behavior->GetPrototypeGuid();
        if (guid.IsValid()) {
            return bb.RequireGuid(guid);
        }
        error = "BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' references behavior '" + SafeString(behavior->GetName()) +
                "' without a prototype GUID.";
        return nullptr;
    }

    std::string query;
    if (!ReadStringValue(source, query)) {
        error = "Failed to read BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' as text. CK type=" + ParameterTypeLabel(behcontext.Context, source) + ".";
        return nullptr;
    }

    query = TrimString(query);
    if (query.empty()) {
        error = "BBDecl Component parameter '" + SafeString(source->GetName()) +
                "' is empty. Expected BB name, Category/Name, GUID, or CKBehavior object.";
        return nullptr;
    }

    BBDecl *spec = bb.Require(query);
    if (!spec || !spec->IsValid()) {
        error = spec ? spec->Error() : "Failed to create BBDecl.";
        if (spec) {
            spec->Release();
        }
        return nullptr;
    }
    return spec;
}

BBSlot *CreateSlotFromBinding(ScriptBehaviorBridge *bridge,
                              const CKBehaviorContext &behcontext,
                              CKParameter *source,
                              const ScriptComponentBinding &binding,
                              std::string &error) {
    if (!bridge) {
        error = "BBSlot Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }

    const ScriptBridgeSlotKind kind = SlotKindFromText(binding.SlotKindName);
    if (kind == ScriptBridgeSlotKind::Standalone) {
        error = "BBSlot Component binding has invalid or missing slot kind (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }
    if (binding.SlotName.empty()) {
        error = "BBSlot Component binding has no slotName (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    BBDecl *spec = nullptr;
    if (!binding.SlotPrototypeName.empty()) {
        spec = bb.Require(binding.SlotPrototypeName);
    } else {
        spec = CreateSpecFromParameter(bridge, behcontext, source, error);
    }
    if (!spec) {
        return nullptr;
    }
    if (!spec->IsValid()) {
        error = spec->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
        spec->Release();
        return nullptr;
    }

    BBSlot *slot = nullptr;
    switch (kind) {
        case ScriptBridgeSlotKind::Input: slot = spec->In(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Output: slot = spec->Out(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pin: slot = spec->Pin(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pout: slot = spec->Pout(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Setting: slot = spec->Setting(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Local: slot = spec->Local(binding.SlotName, binding.SlotOccurrence); break;
        default: break;
    }
    if (!slot || !slot->IsValid()) {
        error = (slot ? slot->Error() : "Failed to create BBSlot.") + " (" + BindingSummary(binding, behcontext.Context) + ")";
        if (slot) {
            slot->Release();
        }
        spec->Release();
        return nullptr;
    }

    spec->Release();
    return slot;
}

BBSlot *CreateSlotFromConfig(BBConfig *config,
                             const ScriptComponentBinding &binding,
                             const CKBehaviorContext &behcontext,
                             std::string &error) {
    if (!config || !config->IsValid()) {
        error = "BBSlot Component binding references unavailable BBConfig field '" +
                binding.SlotFromFieldName + "' (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    const ScriptBridgeSlotKind kind = SlotKindFromText(binding.SlotKindName);
    if (kind == ScriptBridgeSlotKind::Standalone) {
        error = "BBSlot Component binding has invalid or missing slot kind (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }
    if (binding.SlotName.empty()) {
        error = "BBSlot Component binding has no slotName (" + BindingSummary(binding, behcontext.Context) + ").";
        return nullptr;
    }

    BBSlot *slot = nullptr;
    switch (kind) {
        case ScriptBridgeSlotKind::Input: slot = config->In(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Output: slot = config->Out(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pin: slot = config->Pin(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Pout: slot = config->Pout(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Local: slot = config->Local(binding.SlotName, binding.SlotOccurrence); break;
        case ScriptBridgeSlotKind::Setting: {
            BBDecl *decl = config->Decl();
            slot = decl ? decl->Setting(binding.SlotName, binding.SlotOccurrence) : nullptr;
            if (decl) {
                decl->Release();
            }
            break;
        }
        default: break;
    }
    if (!slot || !slot->IsValid()) {
        error = (slot ? slot->Error() : "Failed to create BBSlot.") + " (" + BindingSummary(binding, behcontext.Context) + ")";
        if (slot) {
            slot->Release();
        }
        return nullptr;
    }
    return slot;
}

void ApplySlotBindingMetadata(BBSlot *slot, const ScriptComponentBinding &binding) {
    if (!slot) {
        return;
    }
    slot->SetMetadata(binding.SlotMetadataFlags, binding.DefaultValue, binding.SlotValue);
}

bool ApplyBBConfigSlotValues(BBConfig *bbinding,
                             const ScriptComponentBinding &binding,
                             std::string &error) {
    if (!bbinding) {
        error = "BBConfig metadata application requires a config.";
        return false;
    }
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigPinValues) {
        BBSlot *slot = bbinding->Pin(entry.Name, entry.Occurrence);
        if (!slot || !slot->IsValid()) {
            error = "BBConfig pin metadata failed for '" + entry.Name + "': " +
                    (slot ? slot->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, nullptr) + ")";
            if (slot) {
                slot->Release();
            }
            return false;
        }
        if (entry.HasValue && !bbinding->SetSlotString(slot, entry.Value)) {
            error = "BBConfig pin value failed for '" + entry.Name + "': " + bbinding->Error() + " (" + BindingSummary(binding, nullptr) + ")";
            slot->Release();
            return false;
        }
        slot->Release();
    }
    for (const ScriptComponentNamedSlotValue &entry : binding.ConfigSettingValues) {
        BBSlot *slot = bbinding->Setting(entry.Name, entry.Occurrence);
        if (!slot || !slot->IsValid()) {
            error = "BBConfig setting metadata failed for '" + entry.Name + "': " +
                    (slot ? slot->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, nullptr) + ")";
            if (slot) {
                slot->Release();
            }
            return false;
        }
        if (entry.HasValue && !bbinding->SetSettingString(slot, entry.Value)) {
            error = "BBConfig setting value failed for '" + entry.Name + "': " + bbinding->Error() + " (" + BindingSummary(binding, nullptr) + ")";
            slot->Release();
            return false;
        }
        slot->Release();
    }
    return true;
}

ParamRef *ResolveBBInstanceSourceRef(BBInstance *instance,
                                     const ScriptComponentSourceSelector &selector,
                                     const std::string &fieldName,
                                     int occurrence,
                                     std::string &error) {
    if (!instance) {
        error = "BBConfig source instance '" + fieldName + "' is not available.";
        return nullptr;
    }
    if (selector.Kind != ScriptBridgeSlotKind::Pin &&
        selector.Kind != ScriptBridgeSlotKind::Pout &&
        selector.Kind != ScriptBridgeSlotKind::Local) {
        error = "BBConfig source instance '" + fieldName +
                "' source selector '" + SourceSelectorKindName(selector) +
                "' must use pin:, pout:, or local:.";
        return nullptr;
    }

    BBSlot *slot = nullptr;
    ParamRef *ref = nullptr;
    switch (selector.Kind) {
        case ScriptBridgeSlotKind::Pin:
            slot = instance->PinSlot(selector.Name, occurrence);
            ref = slot ? instance->Pin(slot) : nullptr;
            break;
        case ScriptBridgeSlotKind::Pout:
            slot = instance->PoutSlot(selector.Name, occurrence);
            ref = slot ? instance->Pout(slot) : nullptr;
            break;
        case ScriptBridgeSlotKind::Local: {
            slot = instance->Local(selector.Name, occurrence);
            int index = -1;
            std::string slotError;
            if (slot && slot->ResolveIndex(ScriptBridgeSlotKind::Local, index, slotError)) {
                BehaviorRef *behavior = instance->Behavior();
                ref = behavior ? behavior->Local(index) : nullptr;
                if (behavior) {
                    behavior->Release();
                }
            } else if (!slotError.empty()) {
                error = slotError;
            }
            break;
        }
        default:
            break;
    }

    if (!ref && error.empty()) {
        error = "BBConfig source instance '" + fieldName + "' has no " +
                SourceSelectorKindName(selector) + " '" + selector.Name + "'.";
    }
    if (slot) {
        slot->Release();
    }
    return ref;
}

ParamRef *ResolveBBConfigSourceRef(ScriptComponentState *state,
                                   const CKBehaviorContext &behcontext,
                                   const ScriptComponentSourceSlot &source,
                                   std::string &error) {
    if (source.SourceSlotName.empty()) {
        ParamRef *ref = GetParamRefFieldByName(state, source.SourceFieldName);
        if (!ref) {
            error = "BBConfig source '" + source.SourceFieldName + "' is not a ParamRef@ field.";
        }
        return ref;
    }

    if (BehaviorRef *behavior = GetBehaviorRefFieldByName(state, source.SourceFieldName)) {
        const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
        ParamRef *ref = nullptr;
        BehaviorLayout *layout = behavior->Layout();
        int index = -1;
        if (!layout) {
            error = "BBConfig source behavior '" + source.SourceFieldName + "' has no layout.";
        } else if (selector.Kind == ScriptBridgeSlotKind::Pin) {
            index = layout->FindPin(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Pin(index) : nullptr;
        } else if (selector.Kind == ScriptBridgeSlotKind::Pout) {
            index = layout->FindPout(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Pout(index) : nullptr;
        } else if (selector.Kind == ScriptBridgeSlotKind::Local) {
            index = layout->FindLocal(selector.Name, source.SourceOccurrence);
            ref = index >= 0 ? behavior->Local(index) : nullptr;
        } else {
            error = "BBConfig source behavior '" + source.SourceFieldName +
                    "' source selector '" + source.SourceSlotName +
                    "' must use pin:, pout:, or local:.";
        }

        if (!ref && error.empty()) {
            error = "BBConfig source behavior '" + source.SourceFieldName +
                    "' has no " + SourceSelectorKindName(selector) +
                    " '" + selector.Name + "'.";
        }
        if (layout) {
            layout->Release();
        }
        behavior->Release();
        return ref;
    }

    if (BBConfig *config = GetBBConfigFieldByName(state, source.SourceFieldName)) {
        BBInstance *instance = config->Instance();
        if (!instance) {
            const std::string configError = config->Error();
            error = "BBConfig source config '" + source.SourceFieldName +
                    "' has no live instance. Ensure the source config autostarts or call EnsureStarted() before binding sources.";
            if (!configError.empty()) {
                error += " Current config error: " + configError;
            }
            return nullptr;
        }
        const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
        ParamRef *ref = ResolveBBInstanceSourceRef(instance, selector, source.SourceFieldName, source.SourceOccurrence, error);
        instance->Release();
        return ref;
    }

    BBInstance *instance = GetBBInstanceFieldByName(state, source.SourceFieldName);
    if (!instance) {
        error = "BBConfig source '" + source.SourceFieldName + "' is not a BehaviorRef@, BBConfig@, or BBInstance@ field.";
        return nullptr;
    }
    const ScriptComponentSourceSelector selector = ParseSourceSelector(source.SourceSlotName);
    ParamRef *ref = ResolveBBInstanceSourceRef(instance, selector, source.SourceFieldName, source.SourceOccurrence, error);
    instance->Release();
    return ref;
}

bool ApplyBBConfigSourceBindings(const CKBehaviorContext &behcontext,
                                 ScriptComponentState *state,
                                 const ScriptComponentBinding &binding,
                                 BBConfig *bbinding,
                                 bool requireResolved,
                                 std::string &error) {
    if (!bbinding) {
        error = "BBConfig source binding requires a config.";
        return false;
    }
    for (const ScriptComponentSourceSlot &source : binding.ConfigSources) {
        std::string sourceError;
        ParamRef *ref = ResolveBBConfigSourceRef(state, behcontext, source, sourceError);
        if (!ref) {
            if (requireResolved) {
                error = "BBConfig source '" + source.PinName + "<-" + source.SourceFieldName +
                        (source.SourceSlotName.empty() ? std::string() : "." + source.SourceSlotName) +
                        "' failed: " + sourceError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                return false;
            }
            continue;
        }

        BBSlot *pin = bbinding->Pin(source.PinName, source.PinOccurrence);
        if (!pin || !pin->IsValid()) {
            error = "BBConfig source pin '" + source.PinName + "' failed: " +
                    (pin ? pin->Error() : std::string("slot was not created")) + " (" + BindingSummary(binding, behcontext.Context) + ")";
            if (pin) {
                pin->Release();
            }
            ref->Release();
            return false;
        }
        const bool ok = bbinding->SourceSlot(pin, ref) != nullptr;
        pin->Release();
        ref->Release();
        if (!ok) {
            error = "BBConfig source binding failed for pin '" + source.PinName + "': " + bbinding->Error();
            return false;
        }
    }
    return true;
}

BBConfig *CreateBBConfigFromBinding(ScriptBehaviorBridge *bridge,
                                      ScriptComponentState *state,
                                      const CKBehaviorContext &behcontext,
                                      CKParameter *source,
                                      const ScriptComponentBinding &binding,
                                      std::string &error) {
    if (!bridge) {
        error = "BBConfig Component injection requires ScriptBehaviorBridge.";
        return nullptr;
    }

    BBBridge bb(bridge, behcontext);
    BBDecl *spec = nullptr;
    if (!binding.SlotPrototypeName.empty()) {
        spec = bb.Require(binding.SlotPrototypeName);
    } else {
        spec = CreateSpecFromParameter(bridge, behcontext, source, error);
    }
    if (!spec) {
        return nullptr;
    }
    if (!spec->IsValid()) {
        error = spec->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
        spec->Release();
        return nullptr;
    }

    BBConfig *bbinding = spec->Configure();
    spec->Release();
    if (!bbinding || !bbinding->IsValid()) {
        error = bbinding ? bbinding->Error() : "Failed to create BBConfig.";
        if (bbinding) {
            bbinding->Release();
        }
        return nullptr;
    }

    bbinding->SetComponentLifetime(UsesComponentLifetime(binding));
    bbinding->SetDefaultStart(binding.BindingStartInput);
    bbinding->SetDefaultStop(binding.BindingStopInput);
    if (!binding.BBConfigOwnerExpression.empty()) {
        bbinding->Owner(ResolveBBConfigObjectExpression(state, behcontext, binding.BBConfigOwnerExpression));
    }
    if (!binding.BBConfigTargetExpression.empty()) {
        bbinding->Target(ResolveBBConfigObjectExpression(state, behcontext, binding.BBConfigTargetExpression));
    }

    if (!ApplyBBConfigSlotValues(bbinding, binding, error)) {
        bbinding->Release();
        return nullptr;
    }

    for (const ScriptComponentRequiredSlot &required : binding.RequiredSlots) {
        const ScriptBridgeSlotKind kind = SlotKindFromText(required.KindName);
        if (kind == ScriptBridgeSlotKind::Standalone) {
            error = "BBConfig required slot has invalid kind '" + required.KindName + "' (" + BindingSummary(binding, behcontext.Context) + ").";
            bbinding->Release();
            return nullptr;
        }
        if (!bbinding->RequireSlot(kind, required.Name, required.Occurrence)) {
            error = "BBConfig required slot failed: " + bbinding->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
            bbinding->Release();
            return nullptr;
        }
    }

    if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, false, error)) {
        bbinding->Release();
        return nullptr;
    }

    return bbinding;
}

bool AssignComponentValueField(const ScriptParamValue &value,
                               void *propertyAddress,
                               const ScriptComponentBinding &binding,
                               std::string &error) {
    if (!propertyAddress) {
        error = "Component field address is not available: " + binding.FieldName;
        return false;
    }

    switch (value.Kind) {
        case ScriptParamValueKind::Int:
            if (binding.PropertyTypeId == asTYPEID_UINT32) {
                *static_cast<asUINT *>(propertyAddress) = static_cast<asUINT>(value.Data.IntValue);
            } else {
                *static_cast<int *>(propertyAddress) = value.Data.IntValue;
            }
            return true;
        case ScriptParamValueKind::Float:
            *static_cast<float *>(propertyAddress) = value.Data.FloatValue;
            return true;
        case ScriptParamValueKind::Bool:
            *static_cast<bool *>(propertyAddress) = value.Data.BoolValue;
            return true;
        case ScriptParamValueKind::String:
            *static_cast<std::string *>(propertyAddress) = value.Text();
            return true;
        case ScriptParamValueKind::Guid:
            *static_cast<CKGUID *>(propertyAddress) = value.Data.GuidValue;
            return true;
        case ScriptParamValueKind::Vector:
            *static_cast<VxVector *>(propertyAddress) = value.Data.VectorValue;
            return true;
        case ScriptParamValueKind::Vector2:
            *static_cast<Vx2DVector *>(propertyAddress) = value.Data.Vector2Value;
            return true;
        case ScriptParamValueKind::Color:
            *static_cast<VxColor *>(propertyAddress) = value.Data.ColorValue;
            return true;
        case ScriptParamValueKind::Quaternion:
            *static_cast<VxQuaternion *>(propertyAddress) = value.Data.QuaternionValue;
            return true;
        case ScriptParamValueKind::Matrix:
            *static_cast<VxMatrix *>(propertyAddress) = value.Data.MatrixValue;
            return true;
        case ScriptParamValueKind::ObjectArray: {
            XObjectArray *array = static_cast<XObjectArray *>(propertyAddress);
            array->Clear();
            for (CK_ID id : value.ObjectIds()) {
                array->PushBack(id);
            }
            return true;
        }
        default:
            error = "Unsupported Component binding kind for field: " + binding.FieldName;
            return false;
    }
}

bool InjectComponentParameters(const CKBehaviorContext &behcontext,
                               ScriptComponentState *state,
                               bool initial,
                               std::string &error) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh || !state || !state->Object) {
        error = "Component object is not ready for parameter injection.";
        return false;
    }

    ScriptBehaviorBridge *bridge = nullptr;
    ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
    if (man) {
        bridge = man->GetBehaviorBridge();
    }
    asIScriptEngine *engine = state->Runner && state->Runner->GetModule() ? state->Runner->GetModule()->GetEngine() : nullptr;

    for (ScriptComponentBinding &binding : state->Bindings) {
        if (!initial && !binding.InjectEveryFrame) {
            continue;
        }

        CKParameterIn *input = binding.InputParameterIndex >= 0 && binding.InputParameterIndex < beh->GetInputParameterCount()
            ? beh->GetInputParameter(binding.InputParameterIndex)
            : nullptr;
        if (!input) {
            error = "Component input parameter is not available: " + binding.ParameterName;
            return false;
        }

        CKParameter *source = input->GetRealSource();
        if (!source) {
            error = "Component input parameter has no source: " + binding.ParameterName;
            return false;
        }

        void *propertyAddress = state->Object->GetAddressOfProperty(static_cast<asUINT>(binding.PropertyIndex));
        if (!propertyAddress) {
            error = "Component field address is not available: " + binding.FieldName;
            return false;
        }

        const ScriptParamValueKind valueKind = ValueKindFromComponentKind(binding.Kind);
        if (valueKind != ScriptParamValueKind::Empty) {
            ScriptParamValue value;
            std::string readError;
            if (!ReadParameterValueAs(source, valueKind, value, readError)) {
                error = readError + " (" + binding.ParameterName + ")";
                return false;
            }
            if (!AssignComponentValueField(value, propertyAddress, binding, error)) {
                return false;
            }
            continue;
        }

        switch (binding.Kind) {
            case ScriptComponentBindingKind::Int: {
                int value = 0;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read int Component parameter: " + binding.ParameterName;
                    return false;
                }
                if (binding.PropertyTypeId == asTYPEID_UINT32) {
                    *static_cast<asUINT *>(propertyAddress) = static_cast<asUINT>(value);
                } else {
                    *static_cast<int *>(propertyAddress) = value;
                }
                break;
            }
            case ScriptComponentBindingKind::Float: {
                float value = 0.0f;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read float Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<float *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Bool: {
                CKBOOL value = FALSE;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read bool Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<bool *>(propertyAddress) = value != FALSE;
                break;
            }
            case ScriptComponentBindingKind::String: {
                std::string value;
                if (!ReadStringValue(source, value)) {
                    error = "Failed to read string Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<std::string *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Guid: {
                std::string text;
                if (!ReadStringValue(source, text)) {
                    error = "Failed to read CKGUID Component parameter: " + binding.ParameterName;
                    return false;
                }
                CKGUID value;
                if (!TrimString(text).empty() && !ParseScriptGuidString(text, value)) {
                    error = "Failed to parse CKGUID Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<CKGUID *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Vector: {
                VxVector value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxVector Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxVector *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Vector2: {
                Vx2DVector value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read Vx2DVector Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<Vx2DVector *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Color: {
                VxColor value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxColor Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxColor *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Quaternion: {
                VxQuaternion value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxQuaternion Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxQuaternion *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Matrix: {
                VxMatrix value;
                if (source->GetValue(&value) != CK_OK) {
                    error = "Failed to read VxMatrix Component parameter: " + binding.ParameterName;
                    return false;
                }
                *static_cast<VxMatrix *>(propertyAddress) = value;
                break;
            }
            case ScriptComponentBindingKind::Object: {
                CKObject *objectValue = ReadObjectValue(source, behcontext.Context);
                if (!ValidateObjectFieldValue(engine, binding, objectValue, error)) {
                    return false;
                }
                if (!AssignObjectHandle(state->Object, binding.PropertyIndex, objectValue)) {
                    error = "Failed to assign object Component field: " + binding.FieldName;
                    return false;
                }
                break;
            }
            case ScriptComponentBindingKind::ParamRef: {
                if (!bridge) {
                    error = "ParamRef Component injection requires ScriptBehaviorBridge.";
                    return false;
                }
                const CK_ID inputId = input->GetID();
                if (!initial && binding.HandleInjected && binding.LastObjectId == inputId) {
                    break;
                }
                ParamRef *ref = new ParamRef(bridge, inputId, ScriptBridgeSlotKind::Pin, binding.InputParameterIndex, state->BehaviorId);
                if (!AssignRefCountedHandle<ParamRef>(state->Object, binding.PropertyIndex, ref)) {
                    error = "Failed to assign ParamRef Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = inputId;
                break;
            }
            case ScriptComponentBindingKind::ParamValue: {
                std::string readError;
                ScriptParamValue readValue = ReadParameterValue(source, &readError);
                if (readValue.Kind == ScriptParamValueKind::Empty && !readError.empty()) {
                    error = readError + " (" + binding.ParameterName + ")";
                    return false;
                }
                readValue.TypeGuid = source->GetGUID();
                readValue.Type = source->GetType();
                ParamValue *value = new ParamValue(readValue);
                if (!AssignRefCountedHandle<ParamValue>(state->Object, binding.PropertyIndex, value)) {
                    error = "Failed to assign ParamValue Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                break;
            }
            case ScriptComponentBindingKind::ParamTypeInfo: {
                const CKGUID sourceGuid = source->GetGUID();
                const std::string sourceGuidText = GuidToString(sourceGuid);
                if (!initial && binding.HandleInjected && binding.LastTextValue == sourceGuidText) {
                    break;
                }

                ScriptParameterRegistry *registry = ScriptParameterRegistry::FromContext(behcontext.Context);
                const ScriptParamTypeRecord *record = registry ? registry->GetType(sourceGuid) : nullptr;
                if (!registry || !record) {
                    error = "Failed to resolve ParamTypeInfo Component field (" + BindingSummary(binding, behcontext.Context) +
                            "). Source parameter='" + SafeString(source->GetName()) +
                            "', source CK type=" + ParameterTypeLabel(behcontext.Context, source) +
                            " " + sourceGuidText + ".";
                    return false;
                }

                ParamTypeInfo *info = new ParamTypeInfo(registry, record->Type);
                if (!AssignRefCountedHandle<ParamTypeInfo>(state->Object, binding.PropertyIndex, info)) {
                    error = "Failed to assign ParamTypeInfo Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastTextValue = sourceGuidText;
                break;
            }
            case ScriptComponentBindingKind::BehaviorRef: {
                CKBehavior *target = ReadBehaviorValue(source, behcontext.Context);
                const CK_ID targetId = target ? target->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == targetId) {
                    break;
                }
                if (!bridge && target) {
                    error = "BehaviorRef Component injection requires ScriptBehaviorBridge.";
                    return false;
                }
                BehaviorRef *ref = bridge && target ? bridge->WrapBehavior(target, state->BehaviorId) : nullptr;
                if (!AssignBehaviorRefHandle(bridge, state->Object, binding.PropertyIndex, ref)) {
                    error = "Failed to assign BehaviorRef Component field: " + binding.FieldName;
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = targetId;
                break;
            }
            case ScriptComponentBindingKind::BBPrototype: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == sourceId && binding.LastTextValue == sourceText) {
                    break;
                }

                if (!bridge && (behaviorSource || !sourceText.empty())) {
                    error = "BBPrototype Component injection requires ScriptBehaviorBridge (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                std::string prototypeError;
                BBPrototype *prototype = CreatePrototypeFromParameter(bridge, behcontext, source, prototypeError);
                if (!prototypeError.empty()) {
                    error = prototypeError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    return false;
                }
                if (!AssignBBPrototypeHandle(bridge, state->Object, binding.PropertyIndex, prototype)) {
                    error = "Failed to assign BBPrototype Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = sourceText;
                break;
            }
            case ScriptComponentBindingKind::BBDecl: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                if (!initial && binding.HandleInjected && binding.LastObjectId == sourceId && binding.LastTextValue == sourceText) {
                    break;
                }

                std::string specError;
                BBDecl *spec = CreateSpecFromParameter(bridge, behcontext, source, specError);
                if (!spec) {
                    error = specError + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    return false;
                }
                if (!AssignRefCountedHandle<BBDecl>(state->Object, binding.PropertyIndex, spec)) {
                    error = "Failed to assign BBDecl Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = sourceText;
                break;
            }
            case ScriptComponentBindingKind::BBSlot: {
                std::string sourceText;
                ReadStringValue(source, sourceText);
                sourceText = TrimString(sourceText);
                const std::string cacheText = sourceText + "|" + binding.SlotPrototypeName + "|" +
                    binding.SlotFromFieldName + "|" + binding.SlotKindName + "|" + binding.SlotName + "|" +
                    std::to_string(binding.SlotOccurrence) + "|" + std::to_string(binding.SlotMetadataFlags) + "|" +
                    binding.DefaultValue + "|" + binding.SlotValue;
                if (!initial && binding.HandleInjected && binding.LastTextValue == cacheText) {
                    break;
                }

                std::string slotError;
                BBConfig *ownerConfig = nullptr;
                BBSlot *slot = nullptr;
                if (!binding.SlotFromFieldName.empty()) {
                    ownerConfig = GetBBConfigFieldByName(state, binding.SlotFromFieldName);
                    slot = CreateSlotFromConfig(ownerConfig, binding, behcontext, slotError);
                } else {
                    slot = CreateSlotFromBinding(bridge, behcontext, source, binding, slotError);
                }
                if (!slot) {
                    error = slotError;
                    return false;
                }
                ApplySlotBindingMetadata(slot, binding);
                if (ownerConfig && !ownerConfig->RegisterSlot(slot)) {
                    error = "Failed to register BBSlot with BBConfig '" + binding.SlotFromFieldName + "': " +
                            ownerConfig->Error() + " (" + BindingSummary(binding, behcontext.Context) + ")";
                    slot->Release();
                    return false;
                }
                if (!AssignRefCountedHandle<BBSlot>(state->Object, binding.PropertyIndex, slot)) {
                    error = "Failed to assign BBSlot Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.LastTextValue = cacheText;
                break;
            }
            case ScriptComponentBindingKind::BBConfig: {
                CKBehavior *behaviorSource = CKBehavior::Cast(ReadObjectValue(source, behcontext.Context));
                std::string sourceText;
                if (!behaviorSource) {
                    ReadStringValue(source, sourceText);
                    sourceText = TrimString(sourceText);
                }
                const CK_ID sourceId = behaviorSource ? behaviorSource->GetID() : 0;
                const std::string cacheText = BuildBBConfigBindingCacheText(binding, sourceId, sourceText);
                if (!initial && binding.HandleInjected && binding.LastTextValue == cacheText) {
                    break;
                }

                std::string bindingError;
                BBConfig *bbinding = CreateBBConfigFromBinding(bridge, state, behcontext, source, binding, bindingError);
                if (!bbinding) {
                    error = bindingError;
                    return false;
                }
                if (!AssignRefCountedHandle<BBConfig>(state->Object, binding.PropertyIndex, bbinding)) {
                    error = "Failed to assign BBConfig Component field (" + BindingSummary(binding, behcontext.Context) + ").";
                    return false;
                }
                binding.HandleInjected = true;
                binding.BBConfigChanged = true;
                binding.LastObjectId = sourceId;
                binding.LastTextValue = cacheText;
                break;
            }
            default:
                error = "Unsupported Component binding kind for field: " + binding.FieldName;
                return false;
        }
    }

    return true;
}

std::string BuildRuntimeModuleName(CKBehavior *beh, const std::string &scriptName) {
    return "__CKASComponent_" + std::to_string(beh ? beh->GetID() : 0) + "_" + scriptName;
}

bool IsOutputErrorEnabled(CKBehavior *beh) {
    if (!beh) {
        return false;
    }

    CKBOOL outputError = FALSE;
    beh->GetLocalParameterValue(OUTPUT_ERROR_MESSAGE, &outputError);
    return outputError != FALSE;
}

void SetErrorOutput(CKBehavior *beh, ScriptComponentState *state, const std::string &message, const std::string &stackTrace = std::string()) {
    if (state) {
        state->Failed = true;
    }

    if (!beh) {
        return;
    }

    if (!message.empty()) {
        if (CKContext *context = beh->GetCKContext()) {
            context->OutputToConsoleEx(const_cast<char *>("[AngelScript Component] %s(%d): %s"),
                beh->GetName() ? beh->GetName() : "<unnamed>",
                beh->GetID(),
                message.c_str());
            if (!stackTrace.empty()) {
                context->OutputToConsoleEx(const_cast<char *>("[AngelScript Component] stack: %s"), stackTrace.c_str());
            }
        }
    }

    if (IsOutputErrorEnabled(beh) && beh->GetOutputParameterCount() >= 2) {
        beh->SetOutputParameterValue(0, message.c_str());
        beh->SetOutputParameterValue(1, stackTrace.c_str());
    }

    beh->ActivateOutput(2);
}

void SetRunnerErrorOutput(CKBehavior *beh, ScriptComponentState *state, const char *context) {
    std::string message = context ? context : "AngelScript Component failed.";
    std::string stackTrace;

    if (state && state->Runner) {
        const std::string &runnerError = state->Runner->GetErrorMessage();
        if (!runnerError.empty()) {
            message += ": ";
            message += runnerError;
        }
        stackTrace = state->Runner->GetStackTrace();
    }

    SetErrorOutput(beh, state, message, stackTrace);
}

bool EnsureAutoStartedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error) {
    if (!state || !state->Object) {
        return true;
    }

    std::vector<std::size_t> pending;
    for (std::size_t i = 0; i < state->Bindings.size(); ++i) {
        const ScriptComponentBinding &binding = state->Bindings[i];
        if (binding.Kind == ScriptComponentBindingKind::BBConfig && binding.AutoStartBBConfig) {
            pending.push_back(i);
        }
    }

    std::string lastError;
    while (!pending.empty()) {
        bool madeProgress = false;
        for (std::size_t i = 0; i < pending.size();) {
            ScriptComponentBinding &binding = state->Bindings[pending[i]];
            BBConfig *bbinding = GetBBConfigField(state, binding);
            if (!bbinding) {
                error = "Autostart BBConfig field is not available (" + BindingSummary(binding, behcontext.Context) + ").";
                return false;
            }

            std::string attemptError;
            if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, true, attemptError)) {
                lastError = attemptError;
                ++i;
                continue;
            }
            BBInstance *instance = bbinding->EnsureStarted(behcontext);
            if (!instance) {
                lastError = "Autostart BBConfig failed: " + bbinding->Error() + " (" + BindingSummary(binding, behcontext.Context) + ").";
                ++i;
                continue;
            }
            instance->Release();
            pending.erase(pending.begin() + static_cast<std::vector<std::size_t>::difference_type>(i));
            madeProgress = true;
        }

        if (!madeProgress) {
            error = lastError.empty()
                ? "Autostart BBConfig dependencies could not be resolved."
                : lastError;
            return false;
        }
    }
    return true;
}

bool StepAutomatedBBConfigs(const CKBehaviorContext &behcontext, ScriptComponentState *state, std::string &error) {
    if (!state || !state->Object) {
        return true;
    }
    for (ScriptComponentBinding &binding : state->Bindings) {
        if (binding.Kind != ScriptComponentBindingKind::BBConfig || binding.BBStepPolicy == ScriptComponentBBStepPolicy::Manual) {
            continue;
        }
        BBConfig *bbinding = GetBBConfigField(state, binding);
        if (!bbinding) {
            error = "Automated BBConfig field is not available (" + BindingSummary(binding, behcontext.Context) + ").";
            return false;
        }
        BBInstance *instance = bbinding->Instance();
        if (!instance) {
            continue;
        }
        if (!ApplyBBConfigSourceBindings(behcontext, state, binding, bbinding, true, error)) {
            instance->Release();
            return false;
        }
        const bool shouldStep = binding.BBStepPolicy == ScriptComponentBBStepPolicy::EachUpdate ||
                                (binding.BBStepPolicy == ScriptComponentBBStepPolicy::OnChange && binding.BBConfigChanged);
        if (shouldStep && !instance->Step(behcontext)) {
            error = "Automated BBConfig step failed: " + instance->Error() + " (" + BindingSummary(binding, behcontext.Context) + ").";
            instance->Release();
            return false;
        }
        binding.BBConfigChanged = false;
        instance->Release();
    }
    return true;
}

ScriptComponentState *GetState(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh) {
        return nullptr;
    }

    ScriptComponentState *state = nullptr;
    beh->GetLocalParameterValue(COMPONENT_STATE, &state);
    if (state) {
        return state;
    }

    ScriptManager *man = ScriptManager::GetManager(behcontext.Context);
    if (!man) {
        return nullptr;
    }

    state = man->GetOrCreateComponentState(beh);
    beh->SetLocalParameterValue(COMPONENT_STATE, &state);
    return state;
}

bool IsContextLifecycleMethod(asIScriptFunction *func) {
    if (!func || func->GetReturnTypeId() != asTYPEID_VOID || func->GetParamCount() != 1) {
        return false;
    }

    int typeId = 0;
    asDWORD flags = 0;
    if (func->GetParam(0, &typeId, &flags) < 0) {
        return false;
    }

    int contextTypeId = func->GetEngine()->GetTypeIdByDecl("CKBehaviorContext");
    asDWORD refFlags = flags & asTM_INOUTREF;
    return typeId == contextTypeId && refFlags == asTM_INREF && (flags & asTM_CONST) != 0;
}

bool CacheLifecycleMethod(asITypeInfo *type, const char *name, bool required, asIScriptFunction *&out, ScriptRunner *runner) {
    out = nullptr;
    bool sawName = false;
    std::string invalidDecl;

    for (asUINT i = 0; i < type->GetMethodCount(); ++i) {
        asIScriptFunction *method = type->GetMethodByIndex(i);
        if (!method || std::strcmp(method->GetName(), name) != 0) {
            continue;
        }

        sawName = true;
        if (invalidDecl.empty()) {
            invalidDecl = method->GetDeclaration(false, false, true);
        }

        if (IsContextLifecycleMethod(method)) {
            method->AddRef();
            out = method;
            return true;
        }
    }

    if (sawName || required) {
        std::string message = "Invalid or missing lifecycle method: void ";
        message += name;
        message += "(const CKBehaviorContext &in ctx)";
        if (!invalidDecl.empty()) {
            message += " (found ";
            message += invalidDecl;
            message += ")";
        }
        if (runner) {
            runner->SetErrorMessage(message);
        }
        return false;
    }

    return true;
}

bool CacheComponentMethods(ScriptComponentState *state, asITypeInfo *type) {
    if (!state || !state->Runner || !type) {
        return false;
    }

    return CacheLifecycleMethod(type, "OnLoad", false, state->OnLoad, state->Runner) &&
           CacheLifecycleMethod(type, "Awake", false, state->Awake, state->Runner) &&
           CacheLifecycleMethod(type, "OnEnable", false, state->OnEnable, state->Runner) &&
           CacheLifecycleMethod(type, "Start", false, state->Start, state->Runner) &&
           CacheLifecycleMethod(type, "Update", true, state->Update, state->Runner) &&
           CacheLifecycleMethod(type, "OnDisable", false, state->OnDisable, state->Runner) &&
           CacheLifecycleMethod(type, "OnDestroy", false, state->OnDestroy, state->Runner) &&
           CacheLifecycleMethod(type, "OnReset", false, state->OnReset, state->Runner);
}

bool InvokeLifecycle(CKBehavior *beh, ScriptComponentState *state, asIScriptFunction *method, const CKBehaviorContext &behcontext, const char *name) {
    if (!method) {
        return true;
    }

    if (!state || !state->Runner || !state->Object) {
        SetErrorOutput(beh, state, std::string("Cannot execute lifecycle method: ") + name);
        return false;
    }

    if (!state->Runner->ExecuteObjectMethod(state->Object, method, behcontext)) {
        SetRunnerErrorOutput(beh, state, name);
        return false;
    }

    return true;
}

bool ComponentIdentityChanged(ScriptComponentState *state,
                              const std::string &scriptName,
                              const std::string &className,
                              const std::string &source,
                              const std::string &file,
                              const std::string &manifest,
                              const std::string &runtimeModuleName,
                              bool privateModule) {
    return !state->Loaded ||
           state->ScriptName != scriptName ||
           state->ClassName != className ||
           state->Source != source ||
           state->File != file ||
           state->Manifest != manifest ||
           state->RuntimeModuleName != runtimeModuleName ||
           state->PrivateModule != privateModule;
}

bool EnsureComponentReady(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh || !context || !state) {
        return false;
    }

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man) {
        SetErrorOutput(beh, state, "Can not get script manager.");
        return false;
    }

    const std::string scriptName = ReadStringParameter(beh, SCRIPT_PARAM);
    const std::string className = ReadStringParameter(beh, CLASS_PARAM);
    const std::string source = ReadStringParameter(beh, SOURCE_PARAM);
    const std::string file = ReadStringParameter(beh, FILE_PARAM);
    const std::string manifest = ReadStringParameter(beh, MANIFEST_PARAM);

    if (scriptName.empty()) {
        SetErrorOutput(beh, state, "No script module specified.");
        return false;
    }

    if (className.empty()) {
        SetErrorOutput(beh, state, "No component class specified.");
        return false;
    }

    const bool privateModule = !source.empty() || !file.empty();
    const std::string runtimeModuleName = privateModule ? BuildRuntimeModuleName(beh, scriptName) : scriptName;

    if (ComponentIdentityChanged(state, scriptName, className, source, file, manifest, runtimeModuleName, privateModule)) {
        if (man->GetBehaviorBridge()) {
            man->GetBehaviorBridge()->DestroyComponentTasks(state->BehaviorId);
        }
        man->ResetComponentStateRuntime(state, true);
        state->ScriptName = scriptName;
        state->ClassName = className;
        state->Source = source;
        state->File = file;
        state->Manifest = manifest;
        state->RuntimeModuleName = runtimeModuleName;
        state->PrivateModule = privateModule;
        state->Failed = false;
    }

    if (state->Failed) {
        return false;
    }

    if (state->Loaded && state->Object && state->Update) {
        std::string injectError;
        if (!InjectComponentParameters(behcontext, state, false, injectError)) {
            SetErrorOutput(beh, state, injectError);
            return false;
        }
        return true;
    }

    if (privateModule) {
        man->UnloadScript(runtimeModuleName.c_str());
        int r = source.empty()
            ? man->LoadScript(runtimeModuleName.c_str(), file.c_str())
            : man->CompileScript(runtimeModuleName.c_str(), source.c_str());
        if (r < 0) {
            SetErrorOutput(beh, state, "Failed to load component script module.");
            return false;
        }
    }

    if (!state->Runner) {
        state->Runner = new ScriptRunner(man);
    }

    if (!state->Runner->SetScript(runtimeModuleName.c_str())) {
        SetRunnerErrorOutput(beh, state, "Failed to attach component script");
        return false;
    }

    asITypeInfo *type = state->Runner->GetTypeInfoByName(className.c_str());
    if (!type) {
        SetErrorOutput(beh, state, "Component class not found: " + className);
        return false;
    }

    std::vector<ScriptComponentBinding> bindings = BuildComponentBindingSpecs(state, type);
    for (ScriptComponentBinding &binding : bindings) {
        std::string bindingError;
        if (!ResolveComponentBinding(state->Runner->GetModule()->GetEngine(), type, behcontext.Context, binding, bindingError)) {
            SetErrorOutput(beh, state, bindingError);
            return false;
        }
    }

    std::string syncError;
    if (!SyncDeclaredInputParameters(beh, state, bindings, syncError)) {
        SetErrorOutput(beh, state, syncError);
        return false;
    }
    state->Bindings = std::move(bindings);

    state->Object = state->Runner->CreateScriptObject(type);
    if (!state->Object) {
        SetRunnerErrorOutput(beh, state, "Failed to instantiate component class");
        return false;
    }

    std::string injectError;
    if (!InjectComponentParameters(behcontext, state, true, injectError)) {
        SetErrorOutput(beh, state, injectError);
        return false;
    }

    if (!CacheComponentMethods(state, type)) {
        SetRunnerErrorOutput(beh, state, "Failed to cache component lifecycle methods");
        return false;
    }

    state->Loaded = true;

    if (!state->OnLoadCalled) {
        if (!InvokeLifecycle(beh, state, state->OnLoad, behcontext, "OnLoad")) {
            return false;
        }
        state->OnLoadCalled = true;
    }

    if (!state->AwakeCalled) {
        if (!InvokeLifecycle(beh, state, state->Awake, behcontext, "Awake")) {
            return false;
        }
        state->AwakeCalled = true;
    }

    return true;
}

bool EnableInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    if (!state || state->InstanceEnabled || state->Paused || !state->ScriptActive) {
        return true;
    }

    if (!EnsureComponentReady(behcontext, state)) {
        return false;
    }

    if (!InvokeLifecycle(beh, state, state->OnEnable, behcontext, "OnEnable")) {
        return false;
    }

    state->InstanceEnabled = true;
    return true;
}

bool DisableInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    CKBehavior *beh = behcontext.Behavior;
    if (!state || !state->InstanceEnabled) {
        return true;
    }

    if (!InvokeLifecycle(beh, state, state->OnDisable, behcontext, "OnDisable")) {
        state->InstanceEnabled = false;
        return false;
    }

    StopComponentLifetimeBBConfigs(behcontext, state);
    state->InstanceEnabled = false;
    return true;
}

void DestroyInstance(const CKBehaviorContext &behcontext, ScriptComponentState *state) {
    if (!state || !state->Object) {
        return;
    }

    DisableInstance(behcontext, state);
    InvokeLifecycle(behcontext.Behavior, state, state->OnDestroy, behcontext, "OnDestroy");
    DestroyComponentLifetimeBBConfigs(state);
}

void SyncErrorOutputParameters(CKBehavior *beh) {
    if (!beh) {
        return;
    }

    if (IsOutputErrorEnabled(beh)) {
        while (beh->GetOutputParameterCount() > 0) {
            CKParameterOut *removed = beh->RemoveOutputParameter(0);
            if (removed) {
                CKDestroyObject(removed);
            }
        }
        beh->CreateOutputParameter("Error", CKPGUID_STRING);
        beh->CreateOutputParameter("StackTrace", CKPGUID_STRING);
    } else {
        while (beh->GetOutputParameterCount() > 0) {
            CKParameterOut *removed = beh->RemoveOutputParameter(0);
            if (removed) {
                CKDestroyObject(removed);
            }
        }
    }
}

} // namespace AngelScriptComponentInternal

bool RunScriptComponentMetadataSelfTest(std::string &error) {
    std::vector<ScriptComponentBinding> bindings;
    auto addMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding binding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "TextConfig", binding)) {
            error = "Component metadata self-test failed to parse '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(bindings, binding);
        return true;
    };

    if (!addMetadata("bbconfig prototype=\"Interface/Text/2D Text\" lifetime=\"component\"") ||
        !addMetadata("bbsetting \"Text Properties\"=\"Screen Proportionnal,WordWrap\"") ||
        !addMetadata("bbpin \"Text\"=\"FPS: ...\"") ||
        !addMetadata("bbsource \"Font\"=\"FontConfig.Font Created\"") ||
        !addMetadata("bboutput \"Out\"") ||
        !addMetadata("bbpout \"Rendered\"")) {
        return false;
    }

    if (bindings.size() != 1) {
        error = "Component metadata self-test did not merge stacked metadata.";
        return false;
    }
    const ScriptComponentBinding &binding = bindings.front();
    if (binding.Kind != ScriptComponentBindingKind::BBConfig ||
        binding.FieldName != "TextConfig" ||
        binding.SlotPrototypeName != "Interface/Text/2D Text" ||
        binding.BBConfigLifetime != ScriptComponentBBConfigLifetime::Component ||
        binding.ConfigPinValues.size() != 1 ||
        binding.ConfigPinValues[0].Name != "Text" ||
        binding.ConfigPinValues[0].Value != "FPS: ..." ||
        binding.ConfigSettingValues.size() != 1 ||
        binding.ConfigSettingValues[0].Name != "Text Properties" ||
        binding.ConfigSettingValues[0].Value != "Screen Proportionnal,WordWrap" ||
        binding.ConfigSources.size() != 1 ||
        binding.ConfigSources[0].PinName != "Font" ||
        binding.ConfigSources[0].SourceFieldName != "FontConfig" ||
        binding.ConfigSources[0].SourceSlotName != "Font Created") {
        error = "Component metadata self-test merged BBConfig fields incorrectly.";
        return false;
    }

    bool sawOutput = false;
    bool sawPout = false;
    bool sawFontPin = false;
    bool sawSetting = false;
    for (const ScriptComponentRequiredSlot &slot : binding.RequiredSlots) {
        sawOutput = sawOutput || (slot.KindName == "output" && slot.Name == "Out");
        sawPout = sawPout || (slot.KindName == "pout" && slot.Name == "Rendered");
        sawFontPin = sawFontPin || (slot.KindName == "pin" && slot.Name == "Font");
        sawSetting = sawSetting || (slot.KindName == "setting" && slot.Name == "Text Properties");
    }
    if (!sawOutput || !sawPout || !sawFontPin || !sawSetting) {
        error = "Component metadata self-test missed required slot fragments.";
        return false;
    }

    std::vector<ScriptComponentBinding> repeatedSlotBindings;
    for (const std::string &fieldName : {std::string("FirstConfig"), std::string("SecondConfig")}) {
        ScriptComponentBinding configBinding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata(
                "bbconfig prototype=\"Logics/Calculator/Identity\"",
                fieldName,
                configBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot config metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, configBinding);

        ScriptComponentBinding slotBinding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata("bbpout \"pOut 0\"", fieldName, slotBinding)) {
            error = "Component metadata self-test failed to parse repeated-slot fragment metadata.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(repeatedSlotBindings, slotBinding);
    }
    if (repeatedSlotBindings.size() != 2 ||
        repeatedSlotBindings[0].ParameterName != "FirstConfig" ||
        repeatedSlotBindings[1].ParameterName != "SecondConfig") {
        error = "Component metadata self-test let BBConfig fragment slot names override field parameter names.";
        return false;
    }

    ScriptComponentBinding legacyManaged;
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
            "bbconfig prototype=\"Interface/Text/2D Text\" managed=true",
            "LegacyConfig",
            legacyManaged)) {
        error = "Component metadata self-test failed to parse legacy managed= diagnostic metadata.";
        return false;
    }
    if (legacyManaged.MetadataError.find("lifetime=\"component\"") == std::string::npos ||
        legacyManaged.MetadataError.find("lifetime=\"manual\"") == std::string::npos) {
        error = "Component metadata self-test did not reject managed= with a lifetime replacement diagnostic.";
        return false;
    }

    std::vector<ScriptComponentBinding> aggregateBindings;
    auto addAggregateMetadata = [&](const std::string &metadata) -> bool {
        ScriptComponentBinding aggregateBinding;
        if (!AngelScriptComponentInternal::ParseBindingMetadata(metadata, "AggregateConfig", aggregateBinding)) {
            error = "Component metadata self-test failed to parse aggregate metadata '" + metadata + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(aggregateBindings, aggregateBinding);
        return true;
    };
    if (!addAggregateMetadata("bbconfig prototype=\"Interface/Text/2D Text\" pins=\"Text='Aggregate'\" settings=\"Text Properties='Screen Proportionnal'\" sources=\"Font<-FontConfig.Font Created\"") ||
        !addAggregateMetadata("bbpin \"Text\"=\"Fragment\"")) {
        return false;
    }
    if (aggregateBindings.size() != 1 ||
        aggregateBindings[0].ConfigPinValues.size() != 2 ||
        aggregateBindings[0].ConfigPinValues.back().Name != "Text" ||
        aggregateBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        aggregateBindings[0].ConfigSettingValues.size() != 1 ||
        aggregateBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not preserve aggregate metadata with fragment overwrite order.";
        return false;
    }

    std::vector<ScriptComponentBinding> manifestBindings;
    for (const std::string &line : {
             std::string("bbconfig field=ManifestConfig prototype=\"Interface/Text/2D Text\" lifetime=manual pins=\"Text='Aggregate'\""),
             std::string("bbpin field=ManifestConfig \"Text\"=\"Fragment\""),
             std::string("bbsetting field=ManifestConfig \"Text Properties\"=\"Screen Proportionnal\""),
             std::string("bbsource field=ManifestConfig \"Font\"=\"FontConfig.Font Created\"")}) {
        ScriptComponentBinding manifestBinding;
        if (!AngelScriptComponentInternal::ParseManifestLine(line, manifestBinding)) {
            error = "Component metadata self-test failed to parse manifest line '" + line + "'.";
            return false;
        }
        AngelScriptComponentInternal::MergeOrAppendMetadataBinding(manifestBindings, manifestBinding);
    }
    if (manifestBindings.size() != 1 ||
        manifestBindings[0].BBConfigLifetime != ScriptComponentBBConfigLifetime::Manual ||
        manifestBindings[0].ConfigPinValues.size() != 2 ||
        manifestBindings[0].ConfigPinValues.back().Name != "Text" ||
        manifestBindings[0].ConfigPinValues.back().Value != "Fragment" ||
        manifestBindings[0].ConfigSettingValues.size() != 1 ||
        manifestBindings[0].ConfigSources.size() != 1) {
        error = "Component metadata self-test did not merge manifest BBConfig fragments correctly.";
        return false;
    }

    ScriptComponentBinding occurrenceBinding;
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
            "bbconfig prototype=\"Logics/Calculator/Identity\" pins=\"Value[3]='42'\" sources=\"pIn 0[1]<-SourceConfig.pout:Value[2]\" required=\"pout:Out[4]\"",
            "OccurrenceConfig",
            occurrenceBinding)) {
        error = "Component metadata self-test failed to parse occurrence metadata.";
        return false;
    }
    if (occurrenceBinding.ConfigPinValues.size() != 1 ||
        occurrenceBinding.ConfigPinValues[0].Name != "Value" ||
        occurrenceBinding.ConfigPinValues[0].Occurrence != 3 ||
        occurrenceBinding.ConfigSources.size() != 1 ||
        occurrenceBinding.ConfigSources[0].PinName != "pIn 0" ||
        occurrenceBinding.ConfigSources[0].PinOccurrence != 1 ||
        occurrenceBinding.ConfigSources[0].SourceFieldName != "SourceConfig" ||
        occurrenceBinding.ConfigSources[0].SourceSlotName != "pout:Value" ||
        occurrenceBinding.ConfigSources[0].SourceOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBConfig slot occurrences.";
        return false;
    }
    bool sawPoutOccurrence = false;
    for (const ScriptComponentRequiredSlot &slot : occurrenceBinding.RequiredSlots) {
        sawPoutOccurrence = sawPoutOccurrence || (slot.KindName == "pout" && slot.Name == "Out" && slot.Occurrence == 4);
    }
    if (!sawPoutOccurrence) {
        error = "Component metadata self-test did not preserve required slot occurrence.";
        return false;
    }

    ScriptComponentBinding slotOccurrenceBinding;
    if (!AngelScriptComponentInternal::ParseBindingMetadata(
            "bbslot from=\"OccurrenceConfig\" pout=\"Out[2]\"",
            "OutSlot",
            slotOccurrenceBinding)) {
        error = "Component metadata self-test failed to parse BBSlot occurrence metadata.";
        return false;
    }
    if (slotOccurrenceBinding.Kind != ScriptComponentBindingKind::BBSlot ||
        slotOccurrenceBinding.SlotName != "Out" ||
        slotOccurrenceBinding.SlotOccurrence != 2) {
        error = "Component metadata self-test did not preserve BBSlot field occurrence.";
        return false;
    }
    const std::string occurrenceCacheText = AngelScriptComponentInternal::BuildBBConfigBindingCacheText(occurrenceBinding, 0, std::string());
    if (occurrenceCacheText.find("pin:Value[3]=") == std::string::npos ||
        occurrenceCacheText.find("source:pIn 0[1]<-SourceConfig.pout:Value[2]") == std::string::npos) {
        error = "Component metadata self-test did not include slot occurrences in BBConfig cache text.";
        return false;
    }

    return true;
}

CKObjectDeclaration *FillBehaviorAngelScriptComponentDecl() {
    CKObjectDeclaration *od = CreateCKObjectDeclaration("AngelScript Component");
    od->SetDescription("Run an AngelScript class as a component");
    od->SetCategory("AngelScript");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x5f5d4a84, 0x3dfd4d19));
    od->SetAuthorGuid(CKGUID(0x3a086b4d, 0x2f4a4f01));
    od->SetAuthorName("Kakuty");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAngelScriptComponentProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateAngelScriptComponentProto(CKBehaviorPrototype **pproto) {
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("AngelScript Component");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Enable");
    proto->DeclareInput("Disable");

    proto->DeclareOutput("Enabled");
    proto->DeclareOutput("Disabled");
    proto->DeclareOutput("Error");

    proto->DeclareInParameter("Script", CKPGUID_STRING);
    proto->DeclareInParameter("Class", CKPGUID_STRING);
    proto->DeclareInParameter("Source", CKPGUID_STRING);
    proto->DeclareInParameter("File", CKPGUID_STRING);
    proto->DeclareInParameter("Manifest", CKPGUID_STRING);

    proto->DeclareLocalParameter(nullptr, CKPGUID_POINTER);
    proto->DeclareSetting("Output Error Message", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AngelScriptComponent);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS) (CKBEHAVIOR_INTERNALLYCREATEDINPUTS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTS |
                                                 CKBEHAVIOR_MESSAGESENDER |
                                                 CKBEHAVIOR_MESSAGERECEIVER |
                                                 CKBEHAVIOR_TARGETABLE |
                                                 CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS |
                                                 CKBEHAVIOR_INTERNALLYCREATEDLOCALPARAMS));
    proto->SetBehaviorCallbackFct(AngelScriptComponentCallBack);

    *pproto = proto;
    return CK_OK;
}

int AngelScriptComponent(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    if (!beh) {
        return CKBR_PARAMETERERROR;
    }

    ScriptComponentState *state = AngelScriptComponentInternal::GetState(behcontext);
    if (!state) {
        beh->ActivateOutput(2);
        return CKBR_OWNERERROR;
    }

    if (beh->IsInputActive(1)) {
        beh->ActivateInput(1, FALSE);
        state->DesiredEnabled = false;
        AngelScriptComponentInternal::DisableInstance(behcontext, state);
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    if (beh->IsInputActive(0)) {
        beh->ActivateInput(0, FALSE);
        state->DesiredEnabled = true;
        state->ScriptActive = true;
        state->Paused = false;
        state->Failed = false;

        if (!AngelScriptComponentInternal::EnsureComponentReady(behcontext, state) ||
            !AngelScriptComponentInternal::EnableInstance(behcontext, state)) {
            return CKBR_OK;
        }

        beh->ActivateOutput(0);
    }

    if (!state->DesiredEnabled || !state->ScriptActive || state->Paused || state->Failed) {
        return CKBR_OK;
    }

    if (!AngelScriptComponentInternal::EnsureComponentReady(behcontext, state) ||
        !AngelScriptComponentInternal::EnableInstance(behcontext, state)) {
        return CKBR_OK;
    }

    if (!state->StartCalled) {
        if (!AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->Start, behcontext, "Start")) {
            return CKBR_OK;
        }
        state->StartCalled = true;
    }

    {
        std::string automationError;
        if (!AngelScriptComponentInternal::EnsureAutoStartedBBConfigs(behcontext, state, automationError)) {
            AngelScriptComponentInternal::SetErrorOutput(beh, state, automationError);
            return CKBR_OK;
        }
    }

    if (!AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->Update, behcontext, "Update")) {
        return CKBR_OK;
    }

    {
        std::string automationError;
        if (!AngelScriptComponentInternal::StepAutomatedBBConfigs(behcontext, state, automationError)) {
            AngelScriptComponentInternal::SetErrorOutput(beh, state, automationError);
            return CKBR_OK;
        }
    }

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR AngelScriptComponentCallBack(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    if (!beh) {
        return CKBR_PARAMETERERROR;
    }

    ScriptManager *man = ScriptManager::GetManager(context);
    if (!man) {
        return CKBR_OWNERERROR;
    }

    ScriptComponentState *state = nullptr;

    switch (behcontext.CallbackMessage) {
        case CKM_BEHAVIORCREATE:
        case CKM_BEHAVIORLOAD: {
            state = man->GetOrCreateComponentState(beh);
            beh->SetLocalParameterValue(AngelScriptComponentInternal::COMPONENT_STATE, &state);
        }
        break;

        case CKM_BEHAVIORACTIVATESCRIPT: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->ScriptActive = true;
                if (state->DesiredEnabled) {
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIORDEACTIVATESCRIPT: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->ScriptActive = false;
                AngelScriptComponentInternal::DisableInstance(behcontext, state);
            }
        }
        break;

        case CKM_BEHAVIORPAUSE: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->PauseComponentTasks(beh->GetID(), true);
                }
                state->Paused = true;
                AngelScriptComponentInternal::DisableInstance(behcontext, state);
            }
        }
        break;

        case CKM_BEHAVIORRESUME: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->PauseComponentTasks(beh->GetID(), false);
                }
                state->Paused = false;
                if (state->DesiredEnabled && state->ScriptActive) {
                    AngelScriptComponentInternal::EnableInstance(behcontext, state);
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIORRESET: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                AngelScriptComponentInternal::DestroyComponentLifetimeBBConfigs(state);
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->ResetComponentTasks(beh->GetID());
                }
                state->Failed = false;
                state->StartCalled = false;
                if (state->Object &&
                    !AngelScriptComponentInternal::InvokeLifecycle(beh, state, state->OnReset, behcontext, "OnReset")) {
                    return CKBR_OK;
                }
                if (state->DesiredEnabled && state->ScriptActive && !state->Paused) {
                    beh->Activate(TRUE);
                }
            }
        }
        break;

        case CKM_BEHAVIOREDITED: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                if (man->GetBehaviorBridge()) {
                    man->GetBehaviorBridge()->DestroyComponentTasks(beh->GetID());
                }
                man->ResetComponentStateRuntime(state, true);
            }
        }
        break;

        case CKM_BEHAVIORSETTINGSEDITED: {
            AngelScriptComponentInternal::SyncErrorOutputParameters(beh);
        }
        break;

        case CKM_BEHAVIORDELETE: {
            state = AngelScriptComponentInternal::GetState(behcontext);
            if (state) {
                state->DesiredEnabled = false;
                state->ScriptActive = false;
                AngelScriptComponentInternal::DestroyInstance(behcontext, state);
            }
            man->ReleaseComponentState(beh);
        }
        break;

        default:
            break;
    }

    return CKBR_OK;
}
