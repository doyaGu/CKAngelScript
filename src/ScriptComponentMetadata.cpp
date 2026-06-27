#include "ScriptComponentMetadata.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include "ScriptBridgeCommon.h"
#include "ScriptBehaviorBridge.h"
#include "ScriptBridgeHandles.h"
#include "ScriptInvoker.h"

namespace ScriptComponentSupport {

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

bool ParseBoolText(const std::string &value, bool fallback) {
    const std::string text = ToLower(StripQuotes(value));
    if (text == "true" || text == "yes" || text == "on" || text == "1") {
        return true;
    }
    if (text == "false" || text == "no" || text == "off" || text == "0") {
        return false;
    }
    return fallback;
}

int ObjectBaseTypeIdFromProperty(int typeId) {
    static constexpr int kHandleTypeFlags = asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST;
    return (typeId & asTYPEID_OBJHANDLE) ? (typeId & ~kHandleTypeFlags) : typeId;
}

asITypeInfo *TypeInfoFromPropertyType(asIScriptEngine *engine, int typeId) {
    return engine ? engine->GetTypeInfoById(ObjectBaseTypeIdFromProperty(typeId)) : nullptr;
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

bool IsCKObjectPropertyType(asIScriptEngine *engine, int typeId) {
    asITypeInfo *type = TypeInfoFromPropertyType(engine, typeId);
    if (!type || !type->GetName()) {
        return false;
    }

    if (KindFromTypeName(type->GetName()) == ScriptComponentBindingKind::Object) {
        return true;
    }

    asITypeInfo *objectType = engine ? engine->GetTypeInfoByName("CKObject") : nullptr;
    return objectType && (type == objectType || type->DerivesFrom(objectType));
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
    asITypeInfo *type = TypeInfoFromPropertyType(engine, typeId);
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
    asITypeInfo *type = TypeInfoFromPropertyType(engine, typeId);
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

    if (IsCKObjectPropertyType(engine, typeId)) {
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
            return IsCKObjectPropertyType(engine, typeId);
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

std::string BindingSummary(const ScriptComponentBinding &binding, CKContext *context) {
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
    if (!state || !state->Invoker || !type) {
        return bindings;
    }

    std::shared_ptr<CachedScript> cached = state->Invoker->GetCachedScript();
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

} // namespace ScriptComponentSupport
